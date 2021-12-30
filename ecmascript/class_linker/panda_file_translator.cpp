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
#include <tuple>
#include <numeric>

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
#include "libpandafile/code_data_accessor.h"
#include "libpandafile/code_data_accessor-inl.h"
#include "ecmascript/interpreter/interpreter-inl.h"

namespace panda::ecmascript {
PandaFileTranslator::PandaFileTranslator(EcmaVM *vm)
    : ecmaVm_(vm), factory_(vm->GetFactory()), thread_(vm->GetJSThread())
{
}

JSHandle<Program> PandaFileTranslator::TranslatePandaFile(EcmaVM *vm, const panda_file::File &pf,
                                                          const CString &methodName)
{
    PandaFileTranslator translator(vm);
    translator.TranslateClasses(pf, methodName);
    auto result = translator.GenerateProgram(pf);
    return JSHandle<Program>(translator.thread_, result);
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

void PandaFileTranslator::TranslateClasses(const panda_file::File &pf, const CString &methodName)
{
    RegionFactory *factory = ecmaVm_->GetRegionFactory();
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

    auto methodsData = factory->AllocateBuffer(sizeof(JSMethod) * numMethods);
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
        cda.EnumerateMethods([this, &sd, &methods, &methodIdx, &pf](panda_file::MethodDataAccessor &mda) {
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
                this->TranslateBytecode(codeSize, insns, pf, method);
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
                                            const JSMethod *method)
{
    auto bcIns = BytecodeInstruction(insArr);
    auto bcInsLast = bcIns.JumpTo(insSz);

#ifdef ECMASCRIPT_ENABLE_TS_AOT
    auto preIns = const_cast<uint8_t *>(BytecodeInstruction(insArr).GetAddress());
    std::map<uint8_t *, uint8_t*> byteCodeCurPrePc;
    std::vector<std::tuple<uint8_t *, int32_t, std::vector<uint8_t *>>> bytecodeBlockInfo;
    auto startPc = const_cast<uint8_t *>(bcIns.GetAddress());
    bytecodeBlockInfo.emplace_back(std::make_tuple(startPc, 1, std::vector<uint8_t *>(1, startPc)));
#endif

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
#ifdef ECMASCRIPT_ENABLE_TS_AOT
        // mark bytecode need split offset
        byteCodeCurPrePc.insert(std::pair<uint8_t *, uint8_t *>(pc, preIns));
        preIns = pc;
        CollectBytecodeBlockInfo(pc, bytecodeBlockInfo);
    }
    byteCodeCurPrePc.insert(std::pair<uint8_t *, uint8_t *>(const_cast<uint8_t *>(bcInsLast.GetAddress()), preIns));

    // for (const auto &[key, value] : byteCodeCurPrePc) {
    //     std::cout << "cur pc" << static_cast<void *>(key) << " pre pc: " << static_cast<void *>(value) << std::endl;
    // }

    // collect try catch block info
    auto expectionInfo = CollectTryCatchBlockInfo(pf, method, byteCodeCurPrePc, bytecodeBlockInfo);

    // Complete bytecode blcok Infomation
    CompleteBytecodeBlockInfo(byteCodeCurPrePc, bytecodeBlockInfo);

    // Building the basic block diagram of bytecode
    BuildBasicBlocks(method, expectionInfo, bytecodeBlockInfo, byteCodeCurPrePc);
#else
    }
#endif
}

void PandaFileTranslator::CollectBytecodeBlockInfo(uint8_t* pc,
    std::vector<std::tuple<uint8_t *, int32_t, std::vector<uint8_t *>>> &bytecodeBlockInfo)
{
    auto opcode = static_cast<EcmaOpcode>(*pc);
    switch (opcode) {
        case EcmaOpcode::JMP_IMM8: {
            int8_t offset = READ_INST_8_0();
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + offset);
            // current basic block end
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc, 2, temp));
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc + 2, 1, std::vector<uint8_t *>(1, pc + 2)));
            // jump basic block start
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc + offset, 1, std::vector<uint8_t *>(1, pc + offset)));
        }
            break;
        case EcmaOpcode::JMP_IMM16: {
            int16_t offset = READ_INST_16_0();
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + offset);
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc, 2, temp));
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc + 3, 1, std::vector<uint8_t *>(1, pc + 3)));
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc + offset, 1, std::vector<uint8_t *>(1, pc + offset)));
        }
            break;
        case EcmaOpcode::JMP_IMM32: {
            int32_t offset = READ_INST_32_0();
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + offset);
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc, 2, temp));
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc + 5, 1, std::vector<uint8_t *>(1, pc + 5)));
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc + offset, 1, std::vector<uint8_t *>(1, pc + offset)));
        }
            break;
        case EcmaOpcode::JEQZ_IMM8: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + 2);   // first successor
            int8_t offset = READ_INST_8_0();
            temp.emplace_back(pc + offset);  // second successor
            // condition branch current basic block end
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc, 2, temp));
            // first branch basic block start
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc + 2, 1, std::vector<uint8_t *>(1, pc + 2)));
            // second branch basic block start
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc + offset, 1, std::vector<uint8_t *>(1, pc + offset)));
        }
            break;
        case EcmaOpcode::JEQZ_IMM16: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + 3);   // first successor
            int16_t offset = READ_INST_16_0();
            temp.emplace_back(pc + offset);  // second successor
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc, 2, temp)); // end
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc + 3, 1, std::vector<uint8_t *>(1, pc + 3)));
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc + offset, 1, std::vector<uint8_t *>(1, pc + offset)));
        }
            break;
        case EcmaOpcode::JNEZ_IMM8: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + 2);   // first successor
            int8_t offset = READ_INST_8_0();
            temp.emplace_back(pc + offset);  // second successor
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc, 2, temp));
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc + 2, 1, std::vector<uint8_t *>(1, pc + 2)));
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc + offset, 1, std::vector<uint8_t *>(1, pc + offset)));
        }
            break;
        case EcmaOpcode::JNEZ_IMM16: {
            std::vector<uint8_t *> temp;
            temp.emplace_back(pc + 3);   // first successor
            int8_t offset = READ_INST_16_0();
            temp.emplace_back(pc + offset);  // second successor
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc, 2, temp));
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc + 3, 1, std::vector<uint8_t *>(1, pc + 3)));
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc + offset, 1, std::vector<uint8_t *>(1, pc + offset)));
        }
            break;
        case EcmaOpcode::RETURN_DYN:
        case EcmaOpcode::RETURNUNDEFINED_PREF: {
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc, 2, std::vector<uint8_t *>(1, pc)));
            break;
        }
        case EcmaOpcode::THROWDYN_PREF: {
            bytecodeBlockInfo.emplace_back(std::make_tuple(pc, 2, std::vector<uint8_t *>(1, pc)));
        }
            break;
        default:
            break;
    }
}

std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> PandaFileTranslator::CollectTryCatchBlockInfo(
    const panda_file::File &file, const JSMethod *method,
    std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc,
    std::vector<std::tuple<uint8_t *, int32_t, std::vector<uint8_t *>>> &bytecodeBlockInfo)
{
    // try contains many catch
    std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> byteCodeException;
    panda_file::MethodDataAccessor mda(file, method->GetFileId());
    panda_file::CodeDataAccessor cda(file, mda.GetCodeId().value());
    cda.EnumerateTryBlocks([method, &byteCodeCurPrePc, &bytecodeBlockInfo, &byteCodeException](
        panda_file::CodeDataAccessor::TryBlock &try_block) {
        auto tryStartoffset = try_block.GetStartPc();
        auto tryEndoffset = try_block.GetStartPc() + try_block.GetLength();
        auto tryStartPc = const_cast<uint8_t *>(method->GetBytecodeArray() + tryStartoffset);
        auto tryEndPc = const_cast<uint8_t *>(method->GetBytecodeArray() + tryEndoffset);
        byteCodeException[std::make_pair(tryStartPc, tryEndPc)] = {};
        uint32_t pcOffset = panda_file::INVALID_OFFSET;
        try_block.EnumerateCatchBlocks([&](panda_file::CodeDataAccessor::CatchBlock &catch_block) {
            pcOffset = catch_block.GetHandlerPc();
            auto catchBlockPc = const_cast<uint8_t *>(method->GetBytecodeArray() + pcOffset);
            // try block associate catch block
            byteCodeException[std::make_pair(tryStartPc, tryEndPc)].emplace_back(catchBlockPc);
            return true;
        });
        // Check whether the previous block of the try block exists.
        // If yes, add the current block; otherwise, create a new block.
        bool flag = false;
        for (size_t i = 0; i < bytecodeBlockInfo.size(); i++) {
            if (std::get<1>(bytecodeBlockInfo[i]) == 1) {
                continue;
            }
            if (std::get<0>(bytecodeBlockInfo[i]) == byteCodeCurPrePc[tryStartPc]) {
                auto opcode = static_cast<EcmaOpcode>(*(std::get<0>(bytecodeBlockInfo[i])));
                switch (opcode) {
                    case EcmaOpcode::JMP_IMM8:
                    case EcmaOpcode::JMP_IMM16:
                    case EcmaOpcode::JMP_IMM32:
                    case EcmaOpcode::RETURN_DYN:
                    case EcmaOpcode::RETURNUNDEFINED_PREF:
                    case EcmaOpcode::THROWDYN_PREF: {
                        flag = true;
                        break;
                    }
                    default: {
                        std::get<2>(bytecodeBlockInfo[i]).emplace_back(tryStartPc);
                        flag = true;
                        break;
                    }
                }
                break;
            }
        }
        if (!flag) {
            bytecodeBlockInfo.emplace_back(byteCodeCurPrePc[tryStartPc], 2, std::vector<uint8_t *>(1, tryStartPc)); // pre block
        }
        bytecodeBlockInfo.emplace_back(tryStartPc, 1, std::vector<uint8_t *>(1, tryStartPc)); // try block
        flag = false;
        for (size_t i = 0; i < bytecodeBlockInfo.size(); i++) {
            if (std::get<1>(bytecodeBlockInfo[i]) == 1) {
                continue;
            }
            if (std::get<0>(bytecodeBlockInfo[i]) == byteCodeCurPrePc[tryEndPc]) {
                auto &succs = std::get<2>(bytecodeBlockInfo[i]);
                auto iter = std::find(succs.begin(), succs.end(), std::get<0>(bytecodeBlockInfo[i]));
                if (iter == succs.end()) {
                    auto opcode = static_cast<EcmaOpcode>(*(std::get<0>(bytecodeBlockInfo[i])));
                    switch (opcode) {
                        case EcmaOpcode::JMP_IMM8:
                        case EcmaOpcode::JMP_IMM16:
                        case EcmaOpcode::JMP_IMM32:
                        case EcmaOpcode::RETURN_DYN:
                        case EcmaOpcode::RETURNUNDEFINED_PREF:
                        case EcmaOpcode::THROWDYN_PREF: {
                            break;
                        }
                        default: {
                            succs.emplace_back(tryEndPc);
                            break;
                        }
                    }
                }
                flag = true;
                break;
            }
        }
        if (!flag) {
            bytecodeBlockInfo.emplace_back(byteCodeCurPrePc[tryEndPc], 2, std::vector<uint8_t *>(1, tryEndPc));
        }
        bytecodeBlockInfo.emplace_back(tryEndPc, 1, std::vector<uint8_t *>(1, tryEndPc));  // next block
        return true;
    });
    return byteCodeException;
}

void PandaFileTranslator::CompleteBytecodeBlockInfo(std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc,
    std::vector<std::tuple<uint8_t *, int32_t, std::vector<uint8_t *>>> &bytecodeBlockInfo)
{
    PrintCollectBlockInfo(bytecodeBlockInfo);
    // sort bytecodeBlockInfo
    Sort(bytecodeBlockInfo);
    PrintCollectBlockInfo(bytecodeBlockInfo);

    // Deduplicate
    auto deduplicateIndex = std::unique(bytecodeBlockInfo.begin(), bytecodeBlockInfo.end());
    bytecodeBlockInfo.erase(deduplicateIndex, bytecodeBlockInfo.end());

    // Supplementary block information
    std::vector<uint8_t *> endBlockPc;
    std::vector<uint8_t *> startBlockPc;
    for (size_t i = 0; i < bytecodeBlockInfo.size() -1; i++) {
        if (std::get<1>(bytecodeBlockInfo[i]) == std::get<1>(bytecodeBlockInfo[i + 1]) &&
            std::get<1>(bytecodeBlockInfo[i]) == 1) {
            auto prePc = byteCodeCurPrePc[std::get<0>(bytecodeBlockInfo[i + 1])];
            endBlockPc.emplace_back(prePc); // Previous instruction of current instruction
            endBlockPc.emplace_back(std::get<0>(bytecodeBlockInfo[i + 1])); // current instruction
            continue;
        }
        if (std::get<1>(bytecodeBlockInfo[i]) == std::get<1>(bytecodeBlockInfo[i + 1]) &&
            std::get<1>(bytecodeBlockInfo[i]) == 2) {
            auto tempPc = std::get<0>(bytecodeBlockInfo[i]);
            auto findItem = std::find_if(byteCodeCurPrePc.begin(), byteCodeCurPrePc.end(),
                [tempPc](const std::map<uint8_t *, uint8_t *>::value_type item){
                return item.second == tempPc;
            });
            if (findItem != byteCodeCurPrePc.end())
            {
                startBlockPc.emplace_back((*findItem).first);
            }
        }
    }

    // Supplementary end block info
    for(auto iter = endBlockPc.begin(); iter != endBlockPc.end(); iter+=2) {
        bytecodeBlockInfo.emplace_back(std::make_tuple(*iter, 2, std::vector<uint8_t *>(1, *(iter+1))));
    }
    // Supplementary start block info
    for(auto iter = startBlockPc.begin(); iter != startBlockPc.end(); iter++) {
        bytecodeBlockInfo.emplace_back(std::make_tuple(*iter, 1, std::vector<uint8_t *>(1, *iter)));
    }

    // Deduplicate successor
    for (size_t i = 0; i < bytecodeBlockInfo.size(); i++) {
        if (std::get<1>(bytecodeBlockInfo[i]) == 2) {
            std::set<uint8_t *> tempSet(std::get<2>(bytecodeBlockInfo[i]).begin(), std::get<2>(bytecodeBlockInfo[i]).end());
            std::get<2>(bytecodeBlockInfo[i]).assign(tempSet.begin(), tempSet.end());
        }
    }

    Sort(bytecodeBlockInfo);

    // handling jumps to an empty block
    auto endPc = std::get<0>(bytecodeBlockInfo[bytecodeBlockInfo.size() - 1]);
    std::cout << __FUNCTION__ << " , " << __LINE__ << " " << static_cast<void *>(endPc) << std::endl;
    auto iter = --byteCodeCurPrePc.end();
    if (endPc == iter->first) {
        bytecodeBlockInfo.emplace_back(std::make_tuple(endPc, 2, std::vector<uint8_t *>(1, endPc)));
    }
    // Deduplicate
    deduplicateIndex = std::unique(bytecodeBlockInfo.begin(), bytecodeBlockInfo.end());
    bytecodeBlockInfo.erase(deduplicateIndex, bytecodeBlockInfo.end());
    PrintCollectBlockInfo(bytecodeBlockInfo);
}

void PandaFileTranslator::BuildBasicBlocks(const JSMethod *method,
    std::map<std::pair<uint8_t *, uint8_t *>, std::vector<uint8_t *>> &exception,
    std::vector<std::tuple<uint8_t *, int32_t, std::vector<uint8_t *>>> &bytecodeBlockInfo,
    std::map<uint8_t *, uint8_t*> &byteCodeCurPrePc)
{

    std::map<uint8_t *, ByteCodeBasicBlock *> map1; // [start, bb]
    std::map<uint8_t *, ByteCodeBasicBlock *> map2; // [end, bb]
    ByteCodeGraph byteCodeGraph;
    auto &blocks = byteCodeGraph.graph;
    byteCodeGraph.method = method;
    blocks.resize(bytecodeBlockInfo.size() / 2);
    // build basic block
    int blockId = 0;
    int index = 0;
    for (size_t i = 0; i < bytecodeBlockInfo.size() - 1; i+=2) {
        auto startPc = std::get<0>(bytecodeBlockInfo[i]);
        auto endPc = std::get<0>(bytecodeBlockInfo[i + 1]);
        auto block = &blocks[index++];
        block->id = blockId++;
        block->start = startPc;
        block->end = endPc;
        block->preds = {};
        block->succs = {};
        map1[startPc] = block;
        map2[endPc] = block;
    }

    // add block associate
    for (size_t i = 0; i < bytecodeBlockInfo.size(); i++) {
        if (std::get<1>(bytecodeBlockInfo[i]) == 1)
        {
            continue;
        }
        auto curPc = std::get<0>(bytecodeBlockInfo[i]);
        auto successors = std::get<2>(bytecodeBlockInfo[i]);
        for (size_t j = 0; j < successors.size(); j++) {
            if (successors[j] == curPc) {
                continue;
            }
            auto curBlock = map2[curPc];
            auto succsBlock = map1[successors[j]];
            curBlock->succs.emplace_back(succsBlock);
            succsBlock->preds.emplace_back(curBlock);
        }
    }

    // try catch block associate
    for (size_t i = 0; i < blocks.size(); i++) {
        auto pc = blocks[i].start;
        auto it = exception.begin();
        for(; it != exception.end(); it++){
            if (pc < it->first.first || pc >= it->first.second) { // try block interval
               continue;
            }
            auto catchs = exception[it->first]; // catchs start pc
            for (size_t j = i + 1; j < blocks.size(); j++) {
                if (std::find(catchs.begin(), catchs.end(), blocks[j].start) != catchs.end()) {
                    blocks[i].catchs.insert(blocks[i].catchs.begin(), &blocks[j]);
                    blocks[i].succs.emplace_back(&blocks[j]);
                    blocks[j].preds.emplace_back(&blocks[i]);
                }
            }
        }
    }

    PrintGraph(blocks);
    methodsGraphs_[method] = blocks;
    graphs_.emplace_back(byteCodeGraph);
    std::cout << "===========================build basic block end=====================================" << std::endl;
    PrintGraph(byteCodeGraph.graph);
    ComputeDominatorTree(byteCodeGraph);
}

void PandaFileTranslator::ComputeDominatorTree(ByteCodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    // Construct graph backward order
    std::map<size_t, size_t> dfsTimestamp; // (basicblock id, dfs order)
    size_t timestamp = 0;
    std::deque<size_t> pendingList;
    std::vector<size_t> visited(graph.size(), 0);
    auto basicBlockId = graph[0].id;
    pendingList.push_back(basicBlockId);
    while (!pendingList.empty()) {
        auto &curBlockId = pendingList.back();
        std::cout << "*********************: " << curBlockId << std::endl;
        pendingList.pop_back();
        dfsTimestamp[curBlockId] = timestamp++;
        for(auto &succBlock : graph[curBlockId].succs) {
            if (visited[succBlock->id] == 0) {
                visited[succBlock->id] = 1;
                pendingList.push_back(succBlock->id);
            }
        }
    }

    DeadCodeRemove(dfsTimestamp, byteCodeGraph);

    // print cfg order
    for(auto iter : dfsTimestamp) {
        std::cout << "BB_" << iter.first << " depth is : " << iter.second << std::endl;
    }

    std::vector<size_t> immDom(graph.size()); // immediate dominator
    std::vector<std::vector<size_t>> dom(graph.size()); // dominators set
    dom[0] = {0};
    for (size_t i = 1; i < dom.size(); i++) {
        dom[i].resize(dom.size());
        std::iota(dom[i].begin(), dom[i].end(), 0);
    }
    bool changed = true;
    while (changed) {
        changed = false;
        for(size_t i = 1; i < dom.size(); i++) {
            if (graph[i].isDead) {
                continue;
            }
            auto &curDom = dom[i];
            size_t curDomSize = curDom.size();
            curDom.resize(dom.size());
            std::iota(curDom.begin(), curDom.end(), 0);
            std::cout << "current block is " << i << std::endl;
            // traverse the predecessor nodes of the current node, Computing Dominators
            for (auto &preBlock : graph[i].preds){
                std::vector<size_t> tmp(curDom.size());
                auto preDom = dom[preBlock->id];
                std::cout << "pre block id: " << preBlock->id << " doms: ";
                for(size_t j = 0; j < preDom.size(); j++) {
                    std::cout << preDom[j] << " , ";
                }
                std::cout << std::endl;
                auto it = std::set_intersection(
                    curDom.begin(), curDom.end(), preDom.begin(), preDom.end(), tmp.begin());
                tmp.resize(it - tmp.begin());
                std::cout<< "intersection size is: " << tmp.size() << std::endl;
                curDom = tmp;
            }
            auto it = std::find(curDom.begin(), curDom.end(), i);
            if (it == curDom.end()) {
                curDom.push_back(i);
                std::sort(curDom.begin(), curDom.end());
            }

            if (dom[i].size() != curDomSize) {
                changed = true;
            }
        }
    }

    // print dominators set
    for (size_t i = 0; i < dom.size(); i++) {
        std::cout << "block " << i << " dominator blocks has: ";
        for (auto j : dom[i]) {
            std::cout << j << " , ";
        }
        std::cout << std::endl;
    }

    // compute immediate dominator
    immDom[0] = dom[0].front();
    for(size_t i = 1; i < dom.size(); i++) {
        if (graph[i].isDead) {
            continue;
        }
        auto it = std::remove(dom[i].begin(), dom[i].end(), i);
        dom[i].resize(it - dom[i].begin());
        immDom[i] = *std::max_element(dom[i].begin(), dom[i].end(),
            [graph, dfsTimestamp](size_t lhs, size_t rhs) -> bool {
                return dfsTimestamp.at(graph[lhs].id) < dfsTimestamp.at(graph[rhs].id);
        });
    }

    // print immediate dominator
    for(size_t i = 0; i < immDom.size(); i++) {
        std::cout << i << " immediate dominator: " << immDom[i] << std::endl;
    }
    PrintGraph(graph);
    BuildImmediateDominator(immDom, byteCodeGraph);
}

void PandaFileTranslator::BuildImmediateDominator(std::vector<size_t> &immDom, ByteCodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    std::map<size_t, ByteCodeBasicBlock *> map;
    for(size_t i = 0; i < graph.size(); i++) {
        map[graph.at(i).id] = &graph.at(i);
    }

    graph[0].iDominator = &graph[0];
    for(size_t i = 1; i < immDom.size(); i++) {
        auto dominatedBlock = map.at(i);
        if (dominatedBlock->isDead) {
            continue;
        }
        auto immDomBlock = map.at(immDom[i]);
        dominatedBlock->iDominator = immDomBlock;
    }

    for(auto block : graph) {
        if (block.isDead) {
            continue;
        }
        std::cout << "current block " << block.id << " immediate dominator block id: " << block.iDominator->id << std::endl;
    }

    for (auto &block : graph) {
        if (block.isDead) {
            continue;
        }
        if (block.iDominator->id != block.id) {
            block.iDominator->immDomBlocks.emplace_back(&block);
        }
    }

    for (auto &block : graph) {
        if (block.isDead) {
            continue;
        }
        std::cout << "block "<< block.id << " dominat block has: ";
        for (size_t i = 0; i < block.immDomBlocks.size(); i++) {
            std::cout << block.immDomBlocks[i]->id << ",";
        }
        std::cout << std::endl;
    }
    PrintGraph(graph);
    ComputeDomFrontiers(immDom, byteCodeGraph);
    InsertPhi(byteCodeGraph);
    BuildCircuit(byteCodeGraph);
}

void PandaFileTranslator::ComputeDomFrontiers(std::vector<size_t> &immDom, ByteCodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
     std::map<size_t, ByteCodeBasicBlock *> map;
    for(size_t i = 0; i < graph.size(); i++) {
        map[graph.at(i).id] = &graph.at(i);
    }
    std::vector<std::set<ByteCodeBasicBlock *>> domFrontiers(immDom.size());
    for (auto &bb : graph) {
        if (bb.isDead) {
            continue;
        }
        if (bb.preds.size() < 2) {
            continue;
        }
        for (size_t i = 0; i < bb.preds.size(); i++) {
            auto runner = bb.preds[i];
            while(runner->id != immDom[bb.id]) {
                domFrontiers[runner->id].insert(&bb);
                runner = map.at(immDom[runner->id]);
            }
        }
    }

    for (size_t i = 0; i < domFrontiers.size(); i++) {
        for (auto iter = domFrontiers[i].begin(); iter != domFrontiers[i].end(); iter++) {
            graph[i].domFrontiers.emplace_back(*iter);
        }
    }

    for (size_t i = 0; i < domFrontiers.size(); i++) {
        std::cout << "basic block " << i << " dominate Frontiers is: ";
        for (auto iter = domFrontiers[i].begin(); iter != domFrontiers[i].end(); iter++) {
            std::cout << (*iter)->id << " , ";
        }
        std::cout << std::endl;
    }
}

void PandaFileTranslator::DeadCodeRemove(const std::map<size_t, size_t> &dfsTimestamp, ByteCodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    for (auto &block : graph) {
        std::vector<ByteCodeBasicBlock *> newPreds;
        for (auto &bb : block.preds) {
            if (dfsTimestamp.count(bb->id)) {
                newPreds.emplace_back(bb);
            }
        }
        block.preds = newPreds;
    }

    for (auto &block : graph) {
        block.isDead = !dfsTimestamp.count(block.id);
        std::cout << block.isDead << std::endl;
        if (block.isDead) {
            block.succs.clear();
        }
    }
    // PrintGraph(graph);
}

ByteCodeInfo PandaFileTranslator::GetByteCodeInfo(uint8_t *pc)
{
    ByteCodeInfo info;
    auto opcode = static_cast<EcmaOpcode>(*pc);
    info.opcode = opcode;
    switch (opcode) {
        case EcmaOpcode::MOV_V4_V4: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t vdst = READ_INST_4_0();
            uint16_t vsrc = READ_INST_4_1();
            info.vregIn.emplace_back(vsrc);
            info.vregOut.emplace_back(vdst);
            info.offset = 2;
            break;
        }
        case EcmaOpcode::MOV_DYN_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t vdst = READ_INST_8_0();
            uint16_t vsrc = READ_INST_8_1();
            info.vregIn.emplace_back(vsrc);
            info.vregOut.emplace_back(vdst);
            info.offset = 3;
            break;
        }
        case EcmaOpcode::MOV_DYN_V16_V16: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t vdst = READ_INST_16_0();
            uint16_t vsrc = READ_INST_16_2();
            info.vregIn.emplace_back(vsrc);
            info.vregOut.emplace_back(vdst);
            info.offset = 5;
            break;
        }
        case EcmaOpcode::LDA_STR_ID32: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::JMP_IMM8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::JMP_IMM16: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::JMP_IMM32: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::JEQZ_IMM8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::JEQZ_IMM16: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::JNEZ_IMM8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::JNEZ_IMM16: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::LDA_DYN_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t vsrc = READ_INST_8_0();
            info.vregIn.emplace_back(vsrc);
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::STA_DYN_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t vdst = READ_INST_8_0();
            info.vregOut.emplace_back(vdst);
            info.accIn = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDAI_DYN_IMM32: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::FLDAI_DYN_IMM64: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 9;
            break;
        }
        case EcmaOpcode::CALLARG0DYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t funcReg = READ_INST_8_1();
            info.vregIn.emplace_back(funcReg);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::CALLARG1DYN_PREF_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t funcReg = READ_INST_8_1();
            uint32_t reg = READ_INST_8_2();
            info.vregIn.emplace_back(funcReg);
            info.vregIn.emplace_back(reg);
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::CALLARGS2DYN_PREF_V8_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t funcReg = READ_INST_8_1();
            uint32_t reg = READ_INST_8_3();
            info.vregIn.emplace_back(funcReg);
            info.vregIn.emplace_back(reg);
            info.accOut = true;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::CALLARGS3DYN_PREF_V8_V8_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t funcReg = READ_INST_8_1();
            uint32_t reg = READ_INST_8_4();
            info.vregIn.emplace_back(funcReg);
            info.vregIn.emplace_back(reg);
            info.accOut = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::CALLITHISRANGEDYN_PREF_IMM16_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t funcReg = READ_INST_8_3();
            uint32_t actualNumArgs = READ_INST_16_1() - 1;
            size_t copyArgs = actualNumArgs +  NUM_MANDATORY_JSFUNC_ARGS - 2;
            info.vregIn.emplace_back(funcReg);
            for (size_t i = 1; i <= copyArgs; i++) {
                info.vregIn.emplace_back(funcReg + i);
            }
            info.accOut = true;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::CALLSPREADDYN_PREF_V8_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            uint16_t v2 = READ_INST_8_3();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.vregIn.emplace_back(v2);
            info.accOut = true;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::CALLIRANGEDYN_PREF_IMM16_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t funcReg = READ_INST_8_3();
            uint32_t actualNumArgs = READ_INST_16_1();
            size_t copyArgs = actualNumArgs +  NUM_MANDATORY_JSFUNC_ARGS - 2;
            info.vregIn.emplace_back(funcReg);
            for (size_t i = 1; i <= copyArgs; i++) {
                info.vregIn.emplace_back(funcReg + i);
            }
            info.accOut = true;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::RETURN_DYN:{
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.accOut = true;
            info.offset = 1;
            break;
        }
        case EcmaOpcode::RETURNUNDEFINED_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true; // todo(mingliang): this is hack and should be fixed
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDNAN_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDINFINITY_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDGLOBALTHIS_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDUNDEFINED_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDNULL_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDSYMBOL_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDGLOBAL_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDTRUE_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDFALSE_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::LDLEXENVDYN_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::GETUNMAPPEDARGS_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONENTER_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::TONUMBER_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::NEGDYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::NOTDYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::INCDYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::DECDYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::THROWDYN_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::TYPEOFDYN_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::GETPROPITERATOR_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::RESUMEGENERATOR_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t vs = READ_INST_8_1();
            info.vregIn.emplace_back(vs);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::GETRESUMEMODE_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t vs = READ_INST_8_1();
            info.vregIn.emplace_back(vs);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::GETITERATOR_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::THROWCONSTASSIGNMENT_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.offset = 3;
            break;
        }
        case EcmaOpcode::THROWTHROWNOTEXISTS_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::THROWPATTERNNONCOERCIBLE_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::THROWIFNOTOBJECT_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.offset = 3;
            break;
        }
        case EcmaOpcode::ITERNEXT_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::CLOSEITERATOR_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::ADD2DYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::SUB2DYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::MUL2DYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::DIV2DYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::MOD2DYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::EQDYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::NOTEQDYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::LESSDYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::LESSEQDYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::GREATERDYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::GREATEREQDYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t vs = READ_INST_8_1();
            info.vregIn.emplace_back(vs);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::SHL2DYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::SHR2DYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::ASHR2DYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::AND2DYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::OR2DYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::XOR2DYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::DELOBJPROP_PREF_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::DEFINEFUNCDYN_PREF_ID16_IMM16_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::DEFINENCFUNCDYN_PREF_ID16_IMM16_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::DEFINEMETHOD_PREF_ID16_IMM16_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::NEWOBJDYNRANGE_PREF_IMM16_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t firstArgRegIdx = READ_INST_8_3();
            info.vregIn.emplace_back(firstArgRegIdx);
            info.vregIn.emplace_back(firstArgRegIdx + 1);
            info.accOut = true;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::EXPDYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::ISINDYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::INSTANCEOFDYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::STRICTNOTEQDYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::STRICTEQDYN_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::LDLEXVARDYN_PREF_IMM16_IMM16: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::LDLEXVARDYN_PREF_IMM8_IMM8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::LDLEXVARDYN_PREF_IMM4_IMM4: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::STLEXVARDYN_PREF_IMM16_IMM16_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.offset = 7;
            break;
        }
        case EcmaOpcode::STLEXVARDYN_PREF_IMM8_IMM8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.offset = 5;
            break;
        }
        case EcmaOpcode::STLEXVARDYN_PREF_IMM4_IMM4_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.offset = 4;
            break;
        }
        case EcmaOpcode::NEWLEXENVDYN_PREF_IMM16: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.offset = 4;
            break;
        }
        case EcmaOpcode::POPLEXENVDYN_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::CREATEITERRESULTOBJ_PREF_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::SUSPENDGENERATOR_PREF_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            uint16_t v2 = READ_INST_8_3();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v2);
            info.accOut = true;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::ASYNCFUNCTIONREJECT_PREF_V8_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            uint16_t v2 = READ_INST_8_3();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v2);
            info.offset = 5;
            break;
        }
        case EcmaOpcode::NEWOBJSPREADDYN_PREF_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::THROWUNDEFINEDIFHOLE_PREF_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.offset = 4;
            break;
        }
        case EcmaOpcode::STOWNBYNAME_PREF_ID32_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::CREATEEMPTYARRAY_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::CREATEEMPTYOBJECT_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::CREATEOBJECTWITHBUFFER_PREF_IMM16: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::SETOBJECTWITHPROTO_PREF_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.offset = 4;
            break;
        }
        case EcmaOpcode::CREATEARRAYWITHBUFFER_PREF_IMM16: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::IMPORTMODULE_PREF_ID32: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::STMODULEVAR_PREF_ID32: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::COPYMODULE_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.offset = 3;
            break;
        }
        case EcmaOpcode::LDMODVARBYNAME_PREF_ID32_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::CREATEREGEXPWITHLITERAL_PREF_ID32_IMM8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::GETTEMPLATEOBJECT_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::GETNEXTPROPNAME_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::COPYDATAPROPERTIES_PREF_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::STOWNBYINDEX_PREF_V8_IMM32: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::STOWNBYVALUE_PREF_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_3();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::DEFINEGENERATORFUNC_PREF_ID16_IMM16_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::DEFINEASYNCFUNC_PREF_ID16_IMM16_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::LDHOLE_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::COPYRESTARGS_PREF_IMM16: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::DEFINEGETTERSETTERBYVALUE_PREF_V8_V8_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            uint16_t v2 = READ_INST_8_3();
            uint16_t v3 = READ_INST_8_4();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.vregIn.emplace_back(v2);
            info.vregIn.emplace_back(v3);
            info.accIn = true;
            info.accOut = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::LDOBJBYINDEX_PREF_V8_IMM32: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::STOBJBYINDEX_PREF_V8_IMM32: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::LDOBJBYVALUE_PREF_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::STOBJBYVALUE_PREF_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::LDSUPERBYVALUE_PREF_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::STSUPERBYVALUE_PREF_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::TRYLDGLOBALBYNAME_PREF_ID32: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::TRYSTGLOBALBYNAME_PREF_ID32: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::STCONSTTOGLOBALRECORD_PREF_ID32: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::STLETTOGLOBALRECORD_PREF_ID32: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::STCLASSTOGLOBALRECORD_PREF_ID32: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::STOWNBYVALUEWITHNAMESET_PREF_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t v0 = READ_INST_8_1();
            uint32_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::STOWNBYNAMEWITHNAMESET_PREF_ID32_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::LDGLOBALVAR_PREF_ID32: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::LDOBJBYNAME_PREF_ID32_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::STOBJBYNAME_PREF_ID32_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::LDSUPERBYNAME_PREF_ID32_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::STSUPERBYNAME_PREF_ID32_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint32_t v0 = READ_INST_8_5();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.offset = 7;
            break;
        }
        case EcmaOpcode::STGLOBALVAR_PREF_ID32: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.offset = 6;
            break;
        }
        case EcmaOpcode::CREATEGENERATOROBJ_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::STARRAYSPREAD_PREF_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accIn = true;
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::GETITERATORNEXT_PREF_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_7();
            uint16_t v1 = READ_INST_8_8();
            info.vregIn.emplace_back(v0);
            info.vregIn.emplace_back(v1);
            info.accOut = true;
            info.offset = 10;
            break;
        }
        case EcmaOpcode::LDFUNCTION_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::SUPERCALL_PREF_IMM16_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.accOut = true;
            info.offset = 5;
            break;
        }
        case EcmaOpcode::SUPERCALLSPREAD_PREF_V8: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            uint16_t v0 = READ_INST_8_1();
            info.vregIn.emplace_back(v0);
            info.accIn = true;
            info.accOut = true;
            info.offset = 3;
            break;
        }
        case EcmaOpcode::CREATEOBJECTHAVINGMETHOD_PREF_IMM16: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.accOut = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::THROWIFSUPERNOTCORRECTCALL_PREF_IMM16: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.offset = 4;
            break;
        }
        case EcmaOpcode::LDHOMEOBJECT_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::THROWDELETESUPERPROPERTY_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::DEBUGGER_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::ISTRUE_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        case EcmaOpcode::ISFALSE_PREF: {
            std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
            info.accIn = true;
            info.accOut = true;
            info.offset = 2;
            break;
        }
        default: {
            std::cout << opcode << std::endl;
            abort();
            break;
        }
    }
    return info;
}

void PandaFileTranslator::InsertPhi(ByteCodeGraph &byteCodeGraph)
{
    auto &graph = byteCodeGraph.graph;
    std::map<uint16_t, std::set<size_t>> defsitesInfo; // <vreg, bbs>
    for(auto &bb : graph) {
        if (bb.isDead) {
            continue;
        }
        auto pc = bb.start;
        std::cout << "current block start pc: " << static_cast<const void *>(pc) << " end pc: " << static_cast<const void *>(bb.end) << std::endl;
        while (pc <= bb.end) {
            auto bytecodeInfo = GetByteCodeInfo(pc);
            pc = pc + bytecodeInfo.offset; // next inst start pc
            std::cout << "current block next pc: " << static_cast<const void *>(pc) << std::endl;
            for (const auto& vreg: bytecodeInfo.vregOut) {
                defsitesInfo[vreg].insert(bb.id);
            }
        }
    }
    std::cout << __FUNCTION__ << " : " << __LINE__ << " defsites size(): " << defsitesInfo.size() << std::endl;
    for (const auto& [variable, defsites] : defsitesInfo) {
        std::cout << "variable: " << variable << " locate block have: ";
        for (auto id : defsites) {
            std::cout << id << " , ";
        }
        std::cout << std::endl;
    }

    for (const auto& [variable, defsites] : defsitesInfo) {
        std::queue<uint16_t> workList;
        for (auto blockId : defsites) {
            workList.push(blockId);
        }
        while (!workList.empty()) {
            auto currentId = workList.front();
            workList.pop();
            for (auto &block : graph[currentId].domFrontiers) {
                if (!block->phi.count(variable)) {
                    block->phi.insert(variable);
                    if (!defsitesInfo[variable].count(block->id)) {
                        workList.push(block->id);
                    }
                }
            }
        }
    }
    std::cout << __FUNCTION__ << " : " << __LINE__ << std::endl;
    PrintGraph(graph);
}

bool PandaFileTranslator::IsJump(EcmaOpcode opcode) {
    switch (opcode) {
        case EcmaOpcode::JMP_IMM8:
        case EcmaOpcode::JMP_IMM16:
        case EcmaOpcode::JMP_IMM32:
        case EcmaOpcode::JEQZ_IMM8:
        case EcmaOpcode::JEQZ_IMM16:
        case EcmaOpcode::JNEZ_IMM8:
        case EcmaOpcode::JNEZ_IMM16:
            return true;
        default:
            return false;
    }
}

bool PandaFileTranslator::IsCondJump(EcmaOpcode opcode) {
    switch (opcode) {
        case EcmaOpcode::JEQZ_IMM8:
        case EcmaOpcode::JEQZ_IMM16:
        case EcmaOpcode::JNEZ_IMM8:
        case EcmaOpcode::JNEZ_IMM16:
            return true;
        default:
            return false;
    }
}

bool PandaFileTranslator::IsMov(EcmaOpcode opcode) {
    switch (opcode) {
        case EcmaOpcode::MOV_V4_V4:
        case EcmaOpcode::MOV_DYN_V8_V8:
        case EcmaOpcode::MOV_DYN_V16_V16:
        case EcmaOpcode::LDA_DYN_V8:
        case EcmaOpcode::STA_DYN_V8:
            return true;
        default:
            return false;
    }
}

bool PandaFileTranslator::IsReturn(EcmaOpcode opcode) {
    switch (opcode) {
        case EcmaOpcode::RETURN_DYN:
        case EcmaOpcode::RETURNUNDEFINED_PREF:
            return true;
        default:
            return false;
    }
}

bool PandaFileTranslator::IsGeneral(EcmaOpcode opcode) {
    return !IsMov(opcode) && !IsJump(opcode) && !IsReturn(opcode);
}

void PandaFileTranslator::BuildCircuit(ByteCodeGraph &byteCodeGraph) {
    auto &graph = byteCodeGraph.graph;
    std::map<kungfu::GateRef, std::pair<size_t, uint8_t *>> gateToByteCode;
    std::map<uint8_t *, kungfu::GateRef> byteCodeToGate;
    kungfu::Circuit circuit;
    // workaround, remove this when the problem in preds succs trys catchs is fixed.
    for (auto &bb : graph) {
        if (bb.isDead) {
            continue;
        }
        bb.preds.clear();
        bb.trys.clear();
        std::vector<ByteCodeBasicBlock *> newSuccs;
        for (const auto &succ : bb.succs) {
            if (std::count(bb.catchs.begin(), bb.catchs.end(), succ)) {
                continue;
            }
            newSuccs.push_back(succ);
        }
        bb.succs = newSuccs;
    }
    for (auto &bb : graph) {
        if (bb.isDead) {
            continue;
        }
        for (auto &succ : bb.succs) {
            succ->preds.push_back(&bb);
        }
        for (auto &catchBlock : bb.catchs) {
            catchBlock->trys.push_back(&bb);
        }
    }
    for (auto &bb : graph) {
        if (bb.isDead) {
            continue;
        }
        std::cout << "------------------------" << std::endl;
        std::cout << "block: " << bb.id << std::endl;
        std::cout << "preds: ";
        for (auto pred : bb.preds) {
            std::cout << pred->id << " , ";
        }
        std::cout << std::endl;
        std::cout << "succs: ";
        for (auto succ : bb.succs) {
            std::cout << succ->id << " , ";
        }
        std::cout << std::endl;
        std::cout << "catchs: ";
        for (auto catchBlock : bb.catchs) {
            std::cout << catchBlock->id << " , ";
        }
        std::cout << std::endl;
        std::cout << "trys: ";
        for (auto tryBlock : bb.trys) {
            std::cout << tryBlock->id << " , ";
        }
        std::cout << std::endl;
    }
    // create arg gates
    const size_t numArgs = byteCodeGraph.method->GetNumArgs();
    const size_t offsetArgs = byteCodeGraph.method->GetNumVregs();
    std::vector<kungfu::GateRef> argGates(numArgs);
    for (size_t argIdx = 0; argIdx < numArgs; argIdx++) {
        argGates.at(argIdx) = circuit.NewGate(kungfu::OpCode(kungfu::OpCode::INT64_ARG), argIdx,
                                              {kungfu::Circuit::GetCircuitRoot(
                                                      kungfu::OpCode(kungfu::OpCode::ARG_LIST))},
                                              kungfu::TypeCode::NOTYPE);
    }
    // get number of state predicates of each block
    for (auto &bb : graph) {
        if (bb.isDead) {
            continue;
        }
        bb.numStatePred = 0;
    }
    for (auto &bb : graph) {
        if (bb.isDead) {
            continue;
        }
        auto pc = bb.start;
        while (pc <= bb.end) {
            auto bytecodeInfo = GetByteCodeInfo(pc);
            pc = pc + bytecodeInfo.offset; // next inst start pc
            if (IsGeneral(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                if (!bb.catchs.empty()) {
                    bb.catchs.at(0)->numStatePred++;
                }
            }
        }
        for (auto &succ : bb.succs) {
            succ->numStatePred++;
        }
    }
    // build entry of each block
    for (auto &bb : graph) {
        if (bb.isDead) {
            continue;
        }
        if (bb.numStatePred == 0) {
            bb.stateStart = kungfu::Circuit::GetCircuitRoot(kungfu::OpCode(kungfu::OpCode::STATE_ENTRY));
            bb.dependStart = kungfu::Circuit::GetCircuitRoot(kungfu::OpCode(kungfu::OpCode::DEPEND_ENTRY));
        } else if (bb.numStatePred == 1) {
            bb.stateStart = circuit.NewGate(kungfu::OpCode(kungfu::OpCode::ORDINARY_BLOCK), 0,
                                            {kungfu::Circuit::NullGate()}, kungfu::TypeCode::NOTYPE);
            bb.dependStart = circuit.NewGate(kungfu::OpCode(kungfu::OpCode::DEPEND_RELAY), 0,
                                             {bb.stateStart, kungfu::Circuit::NullGate()}, kungfu::TypeCode::NOTYPE);
        } else {
            bb.stateStart = circuit.NewGate(kungfu::OpCode(kungfu::OpCode::MERGE), bb.numStatePred,
                                            std::vector<kungfu::GateRef>(bb.numStatePred, kungfu::Circuit::NullGate()),
                                            kungfu::TypeCode::NOTYPE);
            bb.dependStart = circuit.NewGate(kungfu::OpCode(kungfu::OpCode::DEPEND_SELECTOR), bb.numStatePred,
                                             std::vector<kungfu::GateRef>(bb.numStatePred + 1, kungfu::Circuit::NullGate()),
                                             kungfu::TypeCode::NOTYPE);
            circuit.NewIn(bb.dependStart, 0, bb.stateStart);
        }
    }
    // build states sub-circuit of each block
    for (auto &bb : graph) {
        if (bb.isDead) {
            continue;
        }
        auto stateCur = bb.stateStart;
        auto dependCur = bb.dependStart;
        ASSERT(stateCur != kungfu::Circuit::NullGate());
        ASSERT(dependCur != kungfu::Circuit::NullGate());
        auto pc = bb.start;
        while (pc <= bb.end) {
            auto pcPrev = pc;
            auto bytecodeInfo = GetByteCodeInfo(pc);
            pc = pc + bytecodeInfo.offset; // next inst start pc
            size_t numValueInputs = (bytecodeInfo.accIn ? 1 : 0) + bytecodeInfo.vregIn.size();
            if (IsGeneral(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                auto gate = circuit.NewGate(kungfu::OpCode(kungfu::OpCode::JS_BYTECODE), numValueInputs,
                                            std::vector<kungfu::GateRef>(
                                                    2 + numValueInputs, kungfu::Circuit::NullGate()),
                                            kungfu::TypeCode::NOTYPE);
                circuit.NewIn(gate, 0, stateCur);
                circuit.NewIn(gate, 1, dependCur);
                auto ifSuccess = circuit.NewGate(kungfu::OpCode(kungfu::OpCode::IF_SUCCESS), 0,
                                                 {gate},
                                                 kungfu::TypeCode::NOTYPE);
                auto ifException = circuit.NewGate(kungfu::OpCode(kungfu::OpCode::IF_EXCEPTION), 0,
                                                   {gate},
                                                   kungfu::TypeCode::NOTYPE);
                if (!bb.catchs.empty()) {
                    auto bbNext = bb.catchs.at(0);
                    circuit.NewIn(bbNext->stateStart, bbNext->cntStatePred, ifException);
                    circuit.NewIn(bbNext->dependStart, bbNext->cntStatePred + 1, gate);
                    bbNext->cntStatePred++;
                    bbNext->realPreds.push_back({bb.id, pcPrev, true});
                    ASSERT(bbNext->cntStatePred <= bbNext->numStatePred);
                } else {
                    circuit.NewGate(kungfu::OpCode(kungfu::OpCode::THROW), 0,
                                    {ifException, gate, gate,
                                     kungfu::Circuit::GetCircuitRoot(kungfu::OpCode(kungfu::OpCode::THROW_LIST))},
                                    kungfu::TypeCode::NOTYPE);
                }
                stateCur = ifSuccess;
                dependCur = gate;
                gateToByteCode[gate] = {bb.id, pcPrev};
                if (pcPrev == bb.end) {
                    auto bbNext = &graph.at(bb.id + 1);
                    circuit.NewIn(bbNext->stateStart, bbNext->cntStatePred, stateCur);
                    circuit.NewIn(bbNext->dependStart, bbNext->cntStatePred + 1, dependCur);
                    bbNext->cntStatePred++;
                    bbNext->realPreds.push_back({bb.id, pcPrev, false});
                    ASSERT(bbNext->cntStatePred <= bbNext->numStatePred);
                }
            } else if (IsJump(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                if (IsCondJump(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                    auto gate = circuit.NewGate(kungfu::OpCode(kungfu::OpCode::JS_BYTECODE), numValueInputs,
                                                std::vector<kungfu::GateRef>(
                                                        2 + numValueInputs, kungfu::Circuit::NullGate()),
                                                kungfu::TypeCode::NOTYPE);
                    circuit.NewIn(gate, 0, stateCur);
                    circuit.NewIn(gate, 1, dependCur);
                    auto ifTrue = circuit.NewGate(kungfu::OpCode(kungfu::OpCode::IF_TRUE), 0,
                                                  {gate},
                                                  kungfu::TypeCode::NOTYPE);
                    auto ifFalse = circuit.NewGate(kungfu::OpCode(kungfu::OpCode::IF_FALSE), 0,
                                                   {gate},
                                                   kungfu::TypeCode::NOTYPE);
                    ASSERT(bb.succs.size() == 2);
                    int bitSet = 0;
                    for (auto &bbNext: bb.succs) {
                        if (bbNext->id == bb.id + 1) {
                            circuit.NewIn(bbNext->stateStart, bbNext->cntStatePred, ifFalse);
                            circuit.NewIn(bbNext->dependStart, bbNext->cntStatePred + 1, gate);
                            bbNext->cntStatePred++;
                            bbNext->realPreds.push_back({bb.id, pcPrev, false});
                            ASSERT(bbNext->cntStatePred <= bbNext->numStatePred);
                            bitSet |= 1;
                        } else {
                            circuit.NewIn(bbNext->stateStart, bbNext->cntStatePred, ifTrue);
                            circuit.NewIn(bbNext->dependStart, bbNext->cntStatePred + 1, gate);
                            bbNext->cntStatePred++;
                            bbNext->realPreds.push_back({bb.id, pcPrev, false});
                            ASSERT(bbNext->cntStatePred <= bbNext->numStatePred);
                            bitSet |= 2;
                        }
                    }
                    ASSERT(bitSet == 3);
                    gateToByteCode[gate] = {bb.id, pcPrev};
                    break;
                } else {
                    ASSERT(bb.succs.size() == 1);
                    auto bbNext = bb.succs.at(0);
                    circuit.NewIn(bbNext->stateStart, bbNext->cntStatePred, stateCur);
                    circuit.NewIn(bbNext->dependStart, bbNext->cntStatePred + 1, dependCur);
                    bbNext->cntStatePred++;
                    bbNext->realPreds.push_back({bb.id, pcPrev, false});
                    ASSERT(bbNext->cntStatePred <= bbNext->numStatePred);
                    break;
                }
            } else if (IsReturn(static_cast<EcmaOpcode>(bytecodeInfo.opcode))) {
                ASSERT(bb.succs.empty());
                auto gate = circuit.NewGate(kungfu::OpCode(kungfu::OpCode::RETURN), 0,
                                            {stateCur, dependCur, kungfu::Circuit::NullGate(),
                                             kungfu::Circuit::GetCircuitRoot(
                                                     kungfu::OpCode(kungfu::OpCode::RETURN_LIST))},
                                            kungfu::TypeCode::NOTYPE);
                gateToByteCode[gate] = {bb.id, pcPrev};
                break;
            } else {
                // todo(mingliang): handle explicit throw
                ASSERT(IsMov(static_cast<EcmaOpcode>(bytecodeInfo.opcode)));
                if (pcPrev == bb.end) {
                    auto bbNext = &graph.at(bb.id + 1);
                    circuit.NewIn(bbNext->stateStart, bbNext->cntStatePred, stateCur);
                    circuit.NewIn(bbNext->dependStart, bbNext->cntStatePred + 1, dependCur);
                    bbNext->cntStatePred++;
                    bbNext->realPreds.push_back({bb.id, pcPrev, false});
                    ASSERT(bbNext->cntStatePred <= bbNext->numStatePred);
                }
            }
        }
    }
    // set all value inputs
    for (auto &bb : graph) {
        if (bb.isDead) {
            continue;
        }
        ASSERT(bb.cntStatePred == bb.numStatePred);
    }
    for (auto &bb : graph) {
        if (bb.isDead) {
            continue;
        }
        bb.phiAcc = (bb.numStatePred > 1) || (!bb.trys.empty());
    }
    for (auto &bb : graph) {
        if (bb.isDead) {
            continue;
        }
        auto pc = bb.start;
        std::cout << "BB_" << bb.id << ": " << std::endl;
        while (pc <= bb.end) {
            auto curInfo = GetByteCodeInfo(pc);
            std::cout << "Inst_" << static_cast<int>(curInfo.opcode) << ": ";
            std::cout << "In=[";
            if (curInfo.accIn) {
                std::cout << "acc" << ",";
            }
            for (const auto &in : curInfo.vregIn) {
                std::cout << in << ",";
            }
            std::cout << "] Out=[";
            if (curInfo.accOut) {
                std::cout << "acc" << ",";
            }
            for (const auto &out : curInfo.vregOut) {
                std::cout << out << ",";
            }
            std::cout << "]";
            std::cout << std::endl;
            pc += curInfo.offset;
        }
    }
    for (const auto &[key, value] : gateToByteCode) {
        byteCodeToGate[value.second] = key;
    }
    for (auto gate : circuit.GetAllGates()) {
        // circuit.Print(gate);
        auto numInsArray = circuit.GetOpCode(gate).GetOpCodeNumInsArray(circuit.GetBitField(gate));
        auto it = gateToByteCode.find(gate);
        if (it == gateToByteCode.end()) {
            continue;
        }
        const auto &[id, pc] = it->second;
        auto bytecodeInfo = GetByteCodeInfo(pc);
        size_t numValueInputs = (bytecodeInfo.accIn ? 1 : 0) + bytecodeInfo.vregIn.size();
        size_t numValueOutputs = (bytecodeInfo.accOut ? 1 : 0) + bytecodeInfo.vregOut.size();
        ASSERT(numValueInputs == numInsArray[2]);
        ASSERT(numValueOutputs <= 1);
        std::function<kungfu::GateRef(size_t, const uint8_t *, uint16_t, bool)> defSiteOfReg =
                [&](size_t bbId,
                    const uint8_t *end,
                    uint16_t reg,
                    bool acc) -> kungfu::GateRef {
            auto ans = kungfu::Circuit::NullGate();
            auto &bb = graph.at(bbId);
            std::vector<uint8_t *> instList;
            {
                auto pcIter = bb.start;
                while (pcIter <= end) {
                    instList.push_back(pcIter);
                    auto curInfo = GetByteCodeInfo(pcIter);
                    pcIter += curInfo.offset;
                }
            }
            std::reverse(instList.begin(), instList.end());
            for (auto pcIter: instList) {
                auto curInfo = GetByteCodeInfo(pcIter);
                if (acc) {
                    if (curInfo.accOut) {
                        if (IsMov(static_cast<EcmaOpcode>(curInfo.opcode))) {
                            acc = curInfo.accIn;
                            if (!curInfo.vregIn.empty()) {
                                ASSERT(!acc);
                                ASSERT(curInfo.vregIn.size() == 1);
                                reg = curInfo.vregIn.at(0);
                            }
                        } else {
                            ans = byteCodeToGate.at(pcIter);
                            break;
                        }
                    }
                } else {
                    if (!curInfo.vregOut.empty() && curInfo.vregOut.at(0) == reg) {
                        if (IsMov(static_cast<EcmaOpcode>(curInfo.opcode))) {
                            acc = curInfo.accIn;
                            if (!curInfo.vregIn.empty()) {
                                ASSERT(!acc);
                                ASSERT(curInfo.vregIn.size() == 1);
                                reg = curInfo.vregIn.at(0);
                            }
                        } else {
                            ans = byteCodeToGate.at(pcIter);
                            break;
                        }
                    }
                }
            }
            if (ans == kungfu::Circuit::NullGate() && !acc && bb.phi.count(reg)) {
                if (!bb.valueSelector.count(reg)) {
                    auto gate = circuit.NewGate(kungfu::OpCode(kungfu::OpCode::VALUE_SELECTOR_INT64),
                                                bb.numStatePred,
                                                std::vector<kungfu::GateRef>(
                                                        1 + bb.numStatePred, kungfu::Circuit::NullGate()),
                                                kungfu::TypeCode::NOTYPE);
                    bb.valueSelector[reg] = gate;
                    circuit.NewIn(gate, 0, bb.stateStart);
                    for (size_t i = 0; i < bb.numStatePred; ++i) {
                        auto &[predId, predPc, isException] = bb.realPreds.at(i);
                        circuit.NewIn(gate, i + 1, defSiteOfReg(predId, predPc, reg, acc));
                    }
                }
                ans = bb.valueSelector.at(reg);
            }
            if (ans == kungfu::Circuit::NullGate() && acc && bb.phiAcc) {
                if (bb.valueSelectorAcc == kungfu::Circuit::NullGate()) {
                    auto gate = circuit.NewGate(kungfu::OpCode(kungfu::OpCode::VALUE_SELECTOR_INT64),
                                                bb.numStatePred,
                                                std::vector<kungfu::GateRef>(
                                                        1 + bb.numStatePred, kungfu::Circuit::NullGate()),
                                                kungfu::TypeCode::NOTYPE);
                    bb.valueSelectorAcc = gate;
                    circuit.NewIn(gate, 0, bb.stateStart);
                    bool hasException = false;
                    bool hasNonException = false;
                    for (size_t i = 0; i < bb.numStatePred; ++i) {
                        auto &[predId, predPc, isException] = bb.realPreds.at(i);
                        if (isException) {
                            hasException = true;
                        } else {
                            hasNonException = true;
                        }
                        if (isException) {
                            auto ifExceptionGate = circuit.GetIn(bb.stateStart, i);
                            ASSERT(circuit.GetOpCode(ifExceptionGate) == kungfu::OpCode::IF_EXCEPTION);
                            circuit.NewIn(gate, i + 1, circuit.GetIn(ifExceptionGate, 0));
                        } else {
                            circuit.NewIn(gate, i + 1, defSiteOfReg(predId, predPc, reg, acc));
                        }
                    }
                    // catch block should have only exception entries
                    // normal block should have only normal entries
                    ASSERT(!hasException || !hasNonException);
                }
                ans = bb.valueSelectorAcc;
            }
            if (ans == kungfu::Circuit::NullGate() && bbId == 0) {
                ASSERT(!acc && reg >= offsetArgs && reg < offsetArgs + argGates.size());
                return argGates.at(reg - offsetArgs);
            }
            if (ans == kungfu::Circuit::NullGate()) {
                return defSiteOfReg(bb.iDominator->id, bb.iDominator->end, reg, acc);
            } else {
                return ans;
            }
        };
        for (size_t valueIdx = 0; valueIdx < numInsArray[2]; valueIdx++) {
            auto inIdx = valueIdx + numInsArray[0] + numInsArray[1];
            ASSERT(circuit.IsInGateNull(gate, inIdx));
            if (valueIdx < bytecodeInfo.vregIn.size()) {
                circuit.NewIn(gate, inIdx, defSiteOfReg(id, pc - 1, bytecodeInfo.vregIn.at(valueIdx), false));
            } else {
                circuit.NewIn(gate, inIdx, defSiteOfReg(id, pc - 1, 0, true));
            }
        }
    }
    circuit.PrintAllGates();
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
