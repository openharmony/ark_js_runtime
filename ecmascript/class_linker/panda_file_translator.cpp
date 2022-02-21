/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "panda_file_translator.h"

#include <string>
#include <string_view>
#include <vector>

#include "ecmascript/class_info_extractor.h"
#include "ecmascript/class_linker/program_object-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/literal_data_extractor.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array.h"
#include "libpandabase/mem/mem.h"
#include "libpandabase/utils/logger.h"
#include "libpandabase/utils/utf.h"
#include "libpandafile/bytecode_instruction-inl.h"
#include "libpandafile/class_data_accessor-inl.h"

namespace panda::ecmascript {
PandaFileTranslator::PandaFileTranslator(EcmaVM *vm)
    : ecmaVm_(vm), factory_(vm->GetFactory()), thread_(vm->GetJSThread())
{
}

JSHandle<Program> PandaFileTranslator::TranslatePandaFile(EcmaVM *vm, const panda_file::File &pf,
                                                          const CString &methodName)
{
    PandaFileTranslator translator(vm);
    std::vector<BytecodeTranslationInfo> infoList {};
    translator.TranslateClasses(pf, methodName, infoList);
    auto result = translator.GenerateProgram(pf);
    return JSHandle<Program>(translator.thread_, result);
}

void PandaFileTranslator::TranslateAndCollectPandaFile(EcmaVM *vm, const panda_file::File &pf,
                                                       const CString &methodName,
                                                       std::vector<BytecodeTranslationInfo> &infoList)
{
    PandaFileTranslator translator(vm);
    translator.TranslateClasses(pf, methodName, infoList);
}

template<class T, class... Args>
static T *InitializeMemory(T *mem, Args... args)
{
    return new (mem) T(std::forward<Args>(args)...);
}

const JSMethod *PandaFileTranslator::FindMethods(uint32_t offset) const
{
    Span<JSMethod> methods = GetMethods();
    auto pred = [offset](const JSMethod &method) { return method.GetFileId().GetOffset() == offset; };
    auto it = std::find_if(methods.begin(), methods.end(), pred);
    if (it != methods.end()) {
        return &*it;
    }
    return nullptr;
}

void PandaFileTranslator::TranslateClasses(const panda_file::File &pf, const CString &methodName,
                                           std::vector<BytecodeTranslationInfo> &infoList)
{
    NativeAreaAllocator *allocator = ecmaVm_->GetNativeAreaAllocator();
    Span<const uint32_t> classIndexes = pf.GetClasses();
    uint32_t numMethods = 0;

    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf.IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(pf, classId);
        numMethods += cda.GetMethodsNumber();
    }

    auto methodsData = allocator->AllocateBuffer(sizeof(JSMethod) * numMethods);
    Span<JSMethod> methods {static_cast<JSMethod *>(methodsData), numMethods};
    size_t methodIdx = 0;

    panda_file::File::StringData sd = {static_cast<uint32_t>(methodName.size()),
                                       reinterpret_cast<const uint8_t *>(methodName.c_str())};
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf.IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(pf, classId);
        cda.EnumerateMethods([this, &sd, &methods, &methodIdx, &pf, &infoList](panda_file::MethodDataAccessor &mda) {
            auto codeId = mda.GetCodeId();
            ASSERT(codeId.has_value());

            JSMethod *method = &methods[methodIdx++];
            panda_file::CodeDataAccessor codeDataAccessor(pf, codeId.value());
            uint32_t codeSize = codeDataAccessor.GetCodeSize();

            if (mainMethodIndex_ == 0 && pf.GetStringData(mda.GetNameId()) == sd) {
                mainMethodIndex_ = mda.GetMethodId().GetOffset();
            }

            panda_file::ProtoDataAccessor pda(pf, mda.GetProtoId());
            InitializeMemory(method, nullptr, &pf, mda.GetMethodId(), codeDataAccessor.GetCodeId(),
                             mda.GetAccessFlags(), codeDataAccessor.GetNumArgs(), nullptr);
            method->SetHotnessCounter(EcmaInterpreter::METHOD_HOTNESS_THRESHOLD);
            method->SetCallTypeFromAnnotation();
            const uint8_t *insns = codeDataAccessor.GetInstructions();
            if (this->translated_code_.find(insns) == this->translated_code_.end()) {
                this->translated_code_.insert(insns);
                this->TranslateBytecode(codeSize, insns, pf, method, infoList);
            }
        });
    }

    SetMethods(methods, numMethods);
}

Program *PandaFileTranslator::GenerateProgram(const panda_file::File &pf)
{
    EcmaHandleScope handleScope(thread_);

    JSHandle<Program> program = factory_->NewProgram();
    JSHandle<EcmaString> location = factory_->NewFromStdStringUnCheck(pf.GetFilename(), true);

    // +1 for program
    JSHandle<ConstantPool> constpool = factory_->NewConstantPool(constpoolIndex_ + 1);
    program->SetConstantPool(thread_, constpool.GetTaggedValue());
    program->SetLocation(thread_, location.GetTaggedValue());

    JSHandle<GlobalEnv> env = ecmaVm_->GetGlobalEnv();
    JSHandle<JSHClass> dynclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithProto());
    JSHandle<JSHClass> normalDynclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
    JSHandle<JSHClass> asyncDynclass = JSHandle<JSHClass>::Cast(env->GetAsyncFunctionClass());
    JSHandle<JSHClass> generatorDynclass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());

    for (const auto &it : constpoolMap_) {
        ConstPoolValue value(it.second);
        if (value.GetConstpoolType() == ConstPoolType::STRING) {
            panda_file::File::EntityId id(it.first);
            auto foundStr = pf.GetStringData(id);
            auto string = factory_->GetRawStringFromStringTable(foundStr.data,
                                                                foundStr.utf16_length, foundStr.is_ascii);
            constpool->Set(thread_, value.GetConstpoolIndex(), JSTaggedValue(string));
        } else if (value.GetConstpoolType() == ConstPoolType::BASE_FUNCTION) {
            ASSERT(mainMethodIndex_ != it.first);
            panda_file::File::EntityId id(it.first);
            auto method = const_cast<JSMethod *>(FindMethods(it.first));
            ASSERT(method != nullptr);

            JSHandle<JSFunction> jsFunc =
                factory_->NewJSFunctionByDynClass(method, dynclass, FunctionKind::BASE_CONSTRUCTOR);
            constpool->Set(thread_, value.GetConstpoolIndex(), jsFunc.GetTaggedValue());
            jsFunc->SetConstantPool(thread_, constpool.GetTaggedValue());
        } else if (value.GetConstpoolType() == ConstPoolType::NC_FUNCTION) {
            ASSERT(mainMethodIndex_ != it.first);
            panda_file::File::EntityId id(it.first);
            auto method = const_cast<JSMethod *>(FindMethods(it.first));
            ASSERT(method != nullptr);

            JSHandle<JSFunction> jsFunc =
                factory_->NewJSFunctionByDynClass(method, normalDynclass, FunctionKind::NORMAL_FUNCTION);
            constpool->Set(thread_, value.GetConstpoolIndex(), jsFunc.GetTaggedValue());
            jsFunc->SetConstantPool(thread_, constpool.GetTaggedValue());
        } else if (value.GetConstpoolType() == ConstPoolType::GENERATOR_FUNCTION) {
            ASSERT(mainMethodIndex_ != it.first);
            panda_file::File::EntityId id(it.first);
            auto method = const_cast<JSMethod *>(FindMethods(it.first));
            ASSERT(method != nullptr);

            JSHandle<JSFunction> jsFunc =
                factory_->NewJSFunctionByDynClass(method, generatorDynclass, FunctionKind::GENERATOR_FUNCTION);
            // 26.3.4.3 prototype
            // Whenever a GeneratorFunction instance is created another ordinary object is also created and
            // is the initial value of the generator function's "prototype" property.
            JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
            JSHandle<JSObject> initialGeneratorFuncPrototype =
                factory_->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
            JSObject::SetPrototype(thread_, initialGeneratorFuncPrototype, env->GetGeneratorPrototype());
            jsFunc->SetProtoOrDynClass(thread_, initialGeneratorFuncPrototype);

            constpool->Set(thread_, value.GetConstpoolIndex(), jsFunc.GetTaggedValue());
            jsFunc->SetConstantPool(thread_, constpool.GetTaggedValue());
        } else if (value.GetConstpoolType() == ConstPoolType::ASYNC_FUNCTION) {
            ASSERT(mainMethodIndex_ != it.first);
            panda_file::File::EntityId id(it.first);
            auto method = const_cast<JSMethod *>(FindMethods(it.first));
            ASSERT(method != nullptr);

            JSHandle<JSFunction> jsFunc =
                factory_->NewJSFunctionByDynClass(method, asyncDynclass, FunctionKind::ASYNC_FUNCTION);
            constpool->Set(thread_, value.GetConstpoolIndex(), jsFunc.GetTaggedValue());
            jsFunc->SetConstantPool(thread_, constpool.GetTaggedValue());
        } else if (value.GetConstpoolType() == ConstPoolType::CLASS_FUNCTION) {
            ASSERT(mainMethodIndex_ != it.first);
            panda_file::File::EntityId id(it.first);
            auto method = const_cast<JSMethod *>(FindMethods(it.first));
            ASSERT(method != nullptr);
            JSHandle<ClassInfoExtractor> classInfoExtractor = factory_->NewClassInfoExtractor(method);
            constpool->Set(thread_, value.GetConstpoolIndex(), classInfoExtractor.GetTaggedValue());
        } else if (value.GetConstpoolType() == ConstPoolType::METHOD) {
            ASSERT(mainMethodIndex_ != it.first);
            panda_file::File::EntityId id(it.first);
            auto method = const_cast<JSMethod *>(FindMethods(it.first));
            ASSERT(method != nullptr);

            JSHandle<JSFunction> jsFunc =
                factory_->NewJSFunctionByDynClass(method, normalDynclass, FunctionKind::NORMAL_FUNCTION);
            constpool->Set(thread_, value.GetConstpoolIndex(), jsFunc.GetTaggedValue());
            jsFunc->SetConstantPool(thread_, constpool.GetTaggedValue());
        } else if (value.GetConstpoolType() == ConstPoolType::OBJECT_LITERAL) {
            size_t index = it.first;
            JSMutableHandle<TaggedArray> elements(thread_, JSTaggedValue::Undefined());
            JSMutableHandle<TaggedArray> properties(thread_, JSTaggedValue::Undefined());
            LiteralDataExtractor::ExtractObjectDatas(thread_, &pf, index, elements, properties, this);
            JSHandle<JSObject> obj = JSObject::CreateObjectFromProperties(thread_, properties);

            JSMutableHandle<JSTaggedValue> key(thread_, JSTaggedValue::Undefined());
            JSMutableHandle<JSTaggedValue> valueHandle(thread_, JSTaggedValue::Undefined());
            size_t elementsLen = elements->GetLength();
            for (size_t i = 0; i < elementsLen; i += 2) {  // 2: Each literal buffer contains a pair of key-value.
                key.Update(elements->Get(i));
                if (key->IsHole()) {
                    break;
                }
                valueHandle.Update(elements->Get(i + 1));
                JSObject::DefinePropertyByLiteral(thread_, obj, key, valueHandle);
            }
            constpool->Set(thread_, value.GetConstpoolIndex(), obj.GetTaggedValue());
        } else if (value.GetConstpoolType() == ConstPoolType::ARRAY_LITERAL) {
            size_t index = it.first;
            JSHandle<TaggedArray> literal =
                LiteralDataExtractor::GetDatasIgnoreType(thread_, &pf, static_cast<size_t>(index));
            uint32_t length = literal->GetLength();

            JSHandle<JSArray> arr(JSArray::ArrayCreate(thread_, JSTaggedNumber(length)));
            arr->SetElements(thread_, literal);
            constpool->Set(thread_, value.GetConstpoolIndex(), arr.GetTaggedValue());
        } else if (value.GetConstpoolType() == ConstPoolType::CLASS_LITERAL) {
            size_t index = it.first;
            JSHandle<TaggedArray> literal =
                LiteralDataExtractor::GetDatasIgnoreType(thread_, &pf, static_cast<size_t>(index), this);
            constpool->Set(thread_, value.GetConstpoolIndex(), literal.GetTaggedValue());
        }
    }
    {
        auto method = const_cast<JSMethod *>(FindMethods(mainMethodIndex_));
        ASSERT(method != nullptr);
        JSHandle<JSFunction> mainFunc =
            factory_->NewJSFunctionByDynClass(method, dynclass, FunctionKind::BASE_CONSTRUCTOR);
        mainFunc->SetConstantPool(thread_, constpool.GetTaggedValue());
        program->SetMainFunction(thread_, mainFunc.GetTaggedValue());
        program->SetMethodsData(methods_);
        program->SetNumberMethods(numMethods_);
        // link program
        constpool->Set(thread_, constpoolIndex_, program.GetTaggedValue());
    }

    DefineClassInConstPool(constpool);
    return *program;
}

void PandaFileTranslator::FixOpcode(uint8_t *pc) const
{
    auto opcode = static_cast<BytecodeInstruction::Opcode>(*pc);

    switch (opcode) {
        case BytecodeInstruction::Opcode::MOV_V4_V4:
            *pc = static_cast<uint8_t>(EcmaOpcode::MOV_V4_V4);
            break;
        case BytecodeInstruction::Opcode::MOV_DYN_V8_V8:
            *pc = static_cast<uint8_t>(EcmaOpcode::MOV_DYN_V8_V8);
            break;
        case BytecodeInstruction::Opcode::MOV_DYN_V16_V16:
            *pc = static_cast<uint8_t>(EcmaOpcode::MOV_DYN_V16_V16);
            break;
        case BytecodeInstruction::Opcode::LDA_STR_ID32:
            *pc = static_cast<uint8_t>(EcmaOpcode::LDA_STR_ID32);
            break;
        case BytecodeInstruction::Opcode::JMP_IMM8:
            *pc = static_cast<uint8_t>(EcmaOpcode::JMP_IMM8);
            break;
        case BytecodeInstruction::Opcode::JMP_IMM16:
            *pc = static_cast<uint8_t>(EcmaOpcode::JMP_IMM16);
            break;
        case BytecodeInstruction::Opcode::JMP_IMM32:
            *pc = static_cast<uint8_t>(EcmaOpcode::JMP_IMM32);
            break;
        case BytecodeInstruction::Opcode::JEQZ_IMM8:
            *pc = static_cast<uint8_t>(EcmaOpcode::JEQZ_IMM8);
            break;
        case BytecodeInstruction::Opcode::JEQZ_IMM16:
            *pc = static_cast<uint8_t>(EcmaOpcode::JEQZ_IMM16);
            break;
        case BytecodeInstruction::Opcode::JNEZ_IMM8:
            *pc = static_cast<uint8_t>(EcmaOpcode::JNEZ_IMM8);
            break;
        case BytecodeInstruction::Opcode::JNEZ_IMM16:
            *pc = static_cast<uint8_t>(EcmaOpcode::JNEZ_IMM16);
            break;
        case BytecodeInstruction::Opcode::LDA_DYN_V8:
            *pc = static_cast<uint8_t>(EcmaOpcode::LDA_DYN_V8);
            break;
        case BytecodeInstruction::Opcode::STA_DYN_V8:
            *pc = static_cast<uint8_t>(EcmaOpcode::STA_DYN_V8);
            break;
        case BytecodeInstruction::Opcode::LDAI_DYN_IMM32:
            *pc = static_cast<uint8_t>(EcmaOpcode::LDAI_DYN_IMM32);
            break;
        case BytecodeInstruction::Opcode::FLDAI_DYN_IMM64:
            *pc = static_cast<uint8_t>(EcmaOpcode::FLDAI_DYN_IMM64);
            break;
        case BytecodeInstruction::Opcode::RETURN_DYN:
            *pc = static_cast<uint8_t>(EcmaOpcode::RETURN_DYN);
            break;
        default:
            if (*pc != static_cast<uint8_t>(BytecodeInstruction::Opcode::ECMA_LDNAN_PREF_NONE)) {
                LOG_ECMA(FATAL) << "Is not an Ecma Opcode opcode: " << static_cast<uint16_t>(opcode);
                UNREACHABLE();
            }
            *pc = *(pc + 1);
            *(pc + 1) = 0xFF;
            break;
    }
}

// reuse prefix 8bits to store slotid
void PandaFileTranslator::UpdateICOffset(JSMethod *method, uint8_t *pc) const
{
    uint8_t offset = method->GetSlotSize();
    if (UNLIKELY(offset == JSMethod::MAX_SLOT_SIZE)) {
        return;
    }

    auto opcode = static_cast<EcmaOpcode>(*pc);
    switch (opcode) {
        case EcmaOpcode::TRYLDGLOBALBYNAME_PREF_ID32:
        case EcmaOpcode::TRYSTGLOBALBYNAME_PREF_ID32:
        case EcmaOpcode::LDGLOBALVAR_PREF_ID32:
        case EcmaOpcode::STGLOBALVAR_PREF_ID32:
        case EcmaOpcode::ADD2DYN_PREF_V8:
        case EcmaOpcode::SUB2DYN_PREF_V8:
        case EcmaOpcode::MUL2DYN_PREF_V8:
        case EcmaOpcode::DIV2DYN_PREF_V8:
        case EcmaOpcode::MOD2DYN_PREF_V8:
        case EcmaOpcode::SHL2DYN_PREF_V8:
        case EcmaOpcode::SHR2DYN_PREF_V8:
        case EcmaOpcode::ASHR2DYN_PREF_V8:
        case EcmaOpcode::AND2DYN_PREF_V8:
        case EcmaOpcode::OR2DYN_PREF_V8:
        case EcmaOpcode::XOR2DYN_PREF_V8:
        case EcmaOpcode::EQDYN_PREF_V8:
        case EcmaOpcode::NOTEQDYN_PREF_V8:
        case EcmaOpcode::LESSDYN_PREF_V8:
        case EcmaOpcode::LESSEQDYN_PREF_V8:
        case EcmaOpcode::GREATERDYN_PREF_V8:
        case EcmaOpcode::GREATEREQDYN_PREF_V8:
            method->UpdateSlotSize(1);
            break;
        case EcmaOpcode::LDOBJBYVALUE_PREF_V8_V8:
        case EcmaOpcode::STOBJBYVALUE_PREF_V8_V8:
        case EcmaOpcode::STOWNBYVALUE_PREF_V8_V8:
        case EcmaOpcode::LDOBJBYNAME_PREF_ID32_V8:
        case EcmaOpcode::STOBJBYNAME_PREF_ID32_V8:
        case EcmaOpcode::STOWNBYNAME_PREF_ID32_V8:
        case EcmaOpcode::LDOBJBYINDEX_PREF_V8_IMM32:
        case EcmaOpcode::STOBJBYINDEX_PREF_V8_IMM32:
        case EcmaOpcode::STOWNBYINDEX_PREF_V8_IMM32:
        case EcmaOpcode::LDSUPERBYVALUE_PREF_V8_V8:
        case EcmaOpcode::STSUPERBYVALUE_PREF_V8_V8:
        case EcmaOpcode::LDSUPERBYNAME_PREF_ID32_V8:
        case EcmaOpcode::STSUPERBYNAME_PREF_ID32_V8:
        case EcmaOpcode::LDMODVARBYNAME_PREF_ID32_V8:
        case EcmaOpcode::STMODULEVAR_PREF_ID32:
            method->UpdateSlotSize(2);
            break;
        default:
            return;
    }

    *(pc + 1) = offset;
}

void PandaFileTranslator::FixInstructionId32(const BytecodeInstruction &inst, [[maybe_unused]] uint32_t index,
                                             uint32_t fixOrder) const
{
    // NOLINTNEXTLINE(hicpp-use-auto)
    auto pc = const_cast<uint8_t *>(inst.GetAddress());
    switch (inst.GetFormat()) {
        case BytecodeInstruction::Format::ID32: {
            uint8_t size = sizeof(uint32_t);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(pc + FixInstructionIndex::FIX_ONE, size, &index, size) != EOK) {
                LOG_ECMA(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            break;
        }
        case BytecodeInstruction::Format::PREF_ID16_IMM16_V8: {
            uint16_t u16Index = index;
            uint8_t size = sizeof(uint16_t);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(pc + FixInstructionIndex::FIX_TWO, size, &u16Index, size) != EOK) {
                LOG_ECMA(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            break;
        }
        case BytecodeInstruction::Format::PREF_ID32:
        case BytecodeInstruction::Format::PREF_ID32_V8:
        case BytecodeInstruction::Format::PREF_ID32_IMM8: {
            uint8_t size = sizeof(uint32_t);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(pc + FixInstructionIndex::FIX_TWO, size, &index, size) != EOK) {
                LOG_ECMA(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            break;
        }
        case BytecodeInstruction::Format::PREF_IMM16: {
            ASSERT(static_cast<uint16_t>(index) == index);
            uint16_t u16Index = index;
            uint8_t size = sizeof(uint16_t);
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(pc + FixInstructionIndex::FIX_TWO, size, &u16Index, size) != EOK) {
                LOG_ECMA(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            break;
        }
        case BytecodeInstruction::Format::PREF_ID16_IMM16_IMM16_V8_V8: {
            // Usually, we fix one part of instruction one time. But as for instruction DefineClassWithBuffer,
            // which use both method id and literal buffer id.Using fixOrder indicates fix Location.
            if (fixOrder == 0) {
                uint8_t size = sizeof(uint16_t);
                ASSERT(static_cast<uint16_t>(index) == index);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                if (memcpy_s(pc + FixInstructionIndex::FIX_TWO, size, &index, size) != EOK) {
                    LOG_ECMA(FATAL) << "memcpy_s failed";
                    UNREACHABLE();
                }
                break;
            }
            if (fixOrder == 1) {
                ASSERT(static_cast<uint16_t>(index) == index);
                uint16_t u16Index = index;
                uint8_t size = sizeof(uint16_t);
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                if (memcpy_s(pc + FixInstructionIndex::FIX_FOUR, size, &u16Index, size) != EOK) {
                    LOG_ECMA(FATAL) << "memcpy_s failed";
                    UNREACHABLE();
                }
                break;
            }
            break;
        }
        default:
            UNREACHABLE();
    }
}

void PandaFileTranslator::TranslateBytecode(uint32_t insSz, const uint8_t *insArr, const panda_file::File &pf,
                                            const JSMethod *method, std::vector<BytecodeTranslationInfo> &infoList)
{
    auto bcIns = BytecodeInstruction(insArr);
    auto bcInsLast = bcIns.JumpTo(insSz);
    infoList.push_back(BytecodeTranslationInfo{{}, &pf, method});
    auto &pcArray = infoList.back().pcArray;

    while (bcIns.GetAddress() != bcInsLast.GetAddress()) {
        if (bcIns.HasFlag(BytecodeInstruction::Flags::STRING_ID) &&
            BytecodeInstruction::HasId(bcIns.GetFormat(), 0)) {
            auto index = GetOrInsertConstantPool(ConstPoolType::STRING, bcIns.GetId().AsFileId().GetOffset());
            FixInstructionId32(bcIns, index);
        } else {
            BytecodeInstruction::Opcode opcode = static_cast<BytecodeInstruction::Opcode>(bcIns.GetOpcode());
            switch (opcode) {
                uint32_t index;
                uint32_t methodId;
                case BytecodeInstruction::Opcode::ECMA_DEFINEFUNCDYN_PREF_ID16_IMM16_V8:
                    methodId = pf.ResolveMethodIndex(method->GetFileId(), bcIns.GetId().AsIndex()).GetOffset();
                    index = GetOrInsertConstantPool(ConstPoolType::BASE_FUNCTION, methodId);
                    FixInstructionId32(bcIns, index);
                    break;
                case BytecodeInstruction::Opcode::ECMA_DEFINENCFUNCDYN_PREF_ID16_IMM16_V8:
                    methodId = pf.ResolveMethodIndex(method->GetFileId(), bcIns.GetId().AsIndex()).GetOffset();
                    index = GetOrInsertConstantPool(ConstPoolType::NC_FUNCTION, methodId);
                    FixInstructionId32(bcIns, index);
                    break;
                case BytecodeInstruction::Opcode::ECMA_DEFINEGENERATORFUNC_PREF_ID16_IMM16_V8:
                    methodId = pf.ResolveMethodIndex(method->GetFileId(), bcIns.GetId().AsIndex()).GetOffset();
                    index = GetOrInsertConstantPool(ConstPoolType::GENERATOR_FUNCTION, methodId);
                    FixInstructionId32(bcIns, index);
                    break;
                case BytecodeInstruction::Opcode::ECMA_DEFINEASYNCFUNC_PREF_ID16_IMM16_V8:
                    methodId = pf.ResolveMethodIndex(method->GetFileId(), bcIns.GetId().AsIndex()).GetOffset();
                    index = GetOrInsertConstantPool(ConstPoolType::ASYNC_FUNCTION, methodId);
                    FixInstructionId32(bcIns, index);
                    break;
                case BytecodeInstruction::Opcode::ECMA_DEFINEMETHOD_PREF_ID16_IMM16_V8:
                    methodId = pf.ResolveMethodIndex(method->GetFileId(), bcIns.GetId().AsIndex()).GetOffset();
                    index = GetOrInsertConstantPool(ConstPoolType::METHOD, methodId);
                    FixInstructionId32(bcIns, index);
                    break;
                case BytecodeInstruction::Opcode::ECMA_CREATEOBJECTWITHBUFFER_PREF_IMM16:
                case BytecodeInstruction::Opcode::ECMA_CREATEOBJECTHAVINGMETHOD_PREF_IMM16:
                    index = GetOrInsertConstantPool(ConstPoolType::OBJECT_LITERAL,
                                                    bcIns.GetImm<BytecodeInstruction::Format::PREF_IMM16>());
                    FixInstructionId32(bcIns, index);
                    break;
                case BytecodeInstruction::Opcode::ECMA_CREATEARRAYWITHBUFFER_PREF_IMM16:
                    index = GetOrInsertConstantPool(ConstPoolType::ARRAY_LITERAL,
                                                    bcIns.GetImm<BytecodeInstruction::Format::PREF_IMM16>());
                    FixInstructionId32(bcIns, index);
                    break;
                case BytecodeInstruction::Opcode::ECMA_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8:
                    methodId = pf.ResolveMethodIndex(method->GetFileId(), bcIns.GetId().AsIndex()).GetOffset();
                    index = GetOrInsertConstantPool(ConstPoolType::CLASS_FUNCTION, methodId);
                    FixInstructionId32(bcIns, index);
                    index = GetOrInsertConstantPool(ConstPoolType::CLASS_LITERAL,
                                                    bcIns.GetImm<BytecodeInstruction::Format::
                                                    PREF_ID16_IMM16_IMM16_V8_V8>());
                    FixInstructionId32(bcIns, index, 1);
                    break;
                default:
                    break;
            }
        }
        // NOLINTNEXTLINE(hicpp-use-auto)
        auto pc = const_cast<uint8_t *>(bcIns.GetAddress());
        bcIns = bcIns.GetNext();
        FixOpcode(pc);
        UpdateICOffset(const_cast<JSMethod *>(method), pc);
        if (ecmaVm_->GetJSOptions().IsEnableTsAot()) {
            pcArray.emplace_back(pc);
        }
    }
    if (ecmaVm_->GetJSOptions().IsEnableTsAot()) {
        pcArray.emplace_back(const_cast<uint8_t *>(bcInsLast.GetAddress()));
    }
}

uint32_t PandaFileTranslator::GetOrInsertConstantPool(ConstPoolType type, uint32_t offset)
{
    auto it = constpoolMap_.find(offset);
    if (it != constpoolMap_.cend()) {
        ConstPoolValue value(it->second);
        return value.GetConstpoolIndex();
    }
    ASSERT(constpoolIndex_ != UINT32_MAX);
    uint32_t index = constpoolIndex_++;
    ConstPoolValue value(type, index);
    constpoolMap_.insert({offset, value.GetValue()});
    return index;
}

JSHandle<JSFunction> PandaFileTranslator::DefineMethodInLiteral(JSThread *thread, uint32_t methodId, FunctionKind kind,
                                                                uint16_t length) const
{
    auto method = const_cast<JSMethod *>(FindMethods(methodId));
    ASSERT(method != nullptr);

    JSHandle<GlobalEnv> env = thread_->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSHClass> functionClass;
    if (kind == FunctionKind::NORMAL_FUNCTION) {
        functionClass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
    } else {
        functionClass = JSHandle<JSHClass>::Cast(env->GetGeneratorFunctionClass());
    }
    JSHandle<JSFunction> jsFunc = factory_->NewJSFunctionByDynClass(method, functionClass, kind);

    if (kind == FunctionKind::GENERATOR_FUNCTION) {
        JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
        JSHandle<JSObject> initialGeneratorFuncPrototype =
            factory_->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
        JSObject::SetPrototype(thread_, initialGeneratorFuncPrototype, env->GetGeneratorPrototype());
        jsFunc->SetProtoOrDynClass(thread_, initialGeneratorFuncPrototype);
    }
    jsFunc->SetPropertyInlinedProps(thread, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, JSTaggedValue(length));
    return jsFunc;
}

void PandaFileTranslator::DefineClassInConstPool(const JSHandle<ConstantPool> &constpool) const
{
    uint32_t length = constpool->GetLength();
    uint32_t index = 0;
    while (index < length - 1) {
        JSTaggedValue value = constpool->Get(index);
        if (!value.IsClassInfoExtractor()) {
            index++;
            continue;
        }

        // Here, using a law: when inserting ctor in index of constantpool, the index + 1 location will be inserted by
        // corresponding class literal. Because translator fixes ECMA_DEFINECLASSWITHBUFFER two consecutive times.
        JSTaggedValue nextValue = constpool->Get(index + 1);
        ASSERT(nextValue.IsTaggedArray());

        JSHandle<ClassInfoExtractor> extractor(thread_, value);
        JSHandle<TaggedArray> literal(thread_, nextValue);
        ClassInfoExtractor::BuildClassInfoExtractorFromLiteral(thread_, extractor, literal);
        JSHandle<JSFunction> cls = ClassHelper::DefineClassTemplate(thread_, extractor, constpool);
        constpool->Set(thread_, index, cls);
        index += 2;  // 2: pair of extractor and literal
    }
}
}  // namespace panda::ecmascript
