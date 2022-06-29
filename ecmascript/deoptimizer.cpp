/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd.
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
#include "deoptimizer.h"
#include "ecmascript/compiler/assembler/assembler.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/frames.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/stubs/runtime_stubs-inl.h"
namespace panda::ecmascript {
void Deoptimizer::CollectVregs(kungfu::DeoptBundleVec deoptBundle)
{
    vregs_.clear();
    for (size_t i = 0; i < deoptBundle.size(); i += 2) { // 2: skip id and value
        kungfu::DeoptBundle vregId = deoptBundle.at(i);
        kungfu::DeoptBundle vregValue = deoptBundle.at(i + 1);
        kungfu::OffsetType id;
        JSTaggedType value;
        if (std::holds_alternative<kungfu::OffsetType>(vregId)) {
            id = std::get<kungfu::OffsetType>(vregId);
        } else {
            UNREACHABLE();
        }
        if (std::holds_alternative<kungfu::DwarfRegAndOffsetType>(vregValue)) {
            auto regAndOffset = std::get<kungfu::DwarfRegAndOffsetType>(vregValue);
            kungfu::DwarfRegType dwarfReg = regAndOffset.first;
            kungfu::OffsetType offset = regAndOffset.second;
            ASSERT (dwarfReg == GCStackMapRegisters::FP || dwarfReg == GCStackMapRegisters::SP);
            uintptr_t addr;
            if (dwarfReg == GCStackMapRegisters::SP) {
                addr = context_.callsiteSp + offset;
            } else {
                addr = context_.callsiteFp + offset;
            }
            value = *(reinterpret_cast<JSTaggedType *>(addr));
        } else if (std::holds_alternative<kungfu::LargeInt>(vregValue)) {
            value = JSTaggedType(static_cast<int64_t>(std::get<kungfu::LargeInt>(vregValue)));
        } else {
            ASSERT(std::holds_alternative<kungfu::OffsetType>(vregValue));
            value = JSTaggedType(static_cast<int64_t>(std::get<kungfu::OffsetType>(vregValue)));
        }
        if (id != static_cast<kungfu::OffsetType>(SpecVregIndex::PC_INDEX)) {
            vregs_[id] = JSHandle<JSTaggedValue>(thread_, JSTaggedValue(value));
        } else {
            pc_ = static_cast<uint32_t>(value);
        }
    }
}

kungfu::DeoptBundleVec Deoptimizer::CollectDeoptBundleVec()
{
    kungfu::DeoptBundleVec deoptBundle;
    FrameIterator it(lastLeaveFrame_, thread_);
    for (; !it.Done() && deoptBundle.empty(); it.Advance()) {
        FrameType type = it.GetFrameType();
        switch (type) {
            case FrameType::OPTIMIZED_JS_FUNCTION_FRAME: {
                auto frame = it.GetFrame<OptimizedJSFunctionFrame>();
                auto info = frame->GetDeoptBundleInfo(it);
                if (info.has_value()) {
                    deoptBundle = info.value();
                }
                context_.callsiteSp = it.GetCallSiteSp();
                context_.callsiteFp = reinterpret_cast<uintptr_t>(it.GetSp());
                uintptr_t *preFrameSp = frame->GetPreFrameSp(it);
                uint64_t argc = *(reinterpret_cast<uint64_t *>(preFrameSp));
                JSTaggedType *argv = frame->GetArgv(reinterpret_cast<uintptr_t *>(preFrameSp));
                if (argc > 0) {
                    ASSERT(argc > FIXED_NUM_ARGS);
                    callTarget_.Update(JSHandle<JSTaggedValue>(thread_, JSTaggedValue(argv[0])));
                }
                break;
            }
            case FrameType::ASM_INTERPRETER_FRAME:
            case FrameType::INTERPRETER_CONSTRUCTOR_FRAME:
            case FrameType::INTERPRETER_FRAME:
            case FrameType::INTERPRETER_FAST_NEW_FRAME:
            case FrameType::LEAVE_FRAME:
            case FrameType::LEAVE_FRAME_WITH_ARGV:
            case FrameType::BUILTIN_FRAME_WITH_ARGV:
            case FrameType::BUILTIN_ENTRY_FRAME:
            case FrameType::BUILTIN_FRAME:
            case FrameType::INTERPRETER_ENTRY_FRAME:
            case FrameType::OPTIMIZED_FRAME:
            case FrameType::OPTIMIZED_JS_FUNCTION_ARGS_CONFIG_FRAME:
            case FrameType::OPTIMIZED_ENTRY_FRAME:
            case FrameType::ASM_INTERPRETER_ENTRY_FRAME:
            case FrameType::ASM_INTERPRETER_BRIDGE_FRAME: {
                break;
            }
            default: {
                LOG_ECMA(FATAL) << "frame type error!";
                UNREACHABLE();
            }
        }
    }
    ASSERT(!it.Done());
    preFrame_ = it.GetSp();
    return deoptBundle;
}

JSTaggedValue Deoptimizer::DeoptFromOptimizedEntry()
{
    ASSERT(thread_ != nullptr);
    ObjectFactory *factory = thread_->GetEcmaVM()->GetFactory();
    JSHandle<GeneratorContext> context = factory->NewGeneratorContext();

    auto fun = callTarget_.GetTaggedValue();
    uint32_t nregs = JSFunction::Cast(fun.GetTaggedObject())->GetMethod()->GetNumVregs();
    JSHandle<TaggedArray> regsArray = factory->NewTaggedArray(nregs);
    for (uint32_t i = 0; i < nregs; i++) {
        JSHandle<JSTaggedValue> value = JSHandle<JSTaggedValue>(thread_, JSTaggedValue::Undefined());
        if (vregs_.find(static_cast<kungfu::OffsetType>(i)) != vregs_.end()) {
            value = vregs_.at(static_cast<kungfu::OffsetType>(i));
        }
        regsArray->Set(thread_, i, value.GetTaggedValue());
    }
    context->SetRegsArray(thread_, regsArray.GetTaggedValue());
    context->SetMethod(thread_, callTarget_.GetTaggedValue());
    auto acc = vregs_.at(static_cast<kungfu::OffsetType>(SpecVregIndex::ACC_INDEX));
    context->SetAcc(thread_, acc.GetTaggedValue()); // acc
    context->SetLexicalEnv(thread_, JSTaggedValue::Undefined());
    context->SetNRegs(nregs);
    context->SetBCOffset(pc_);
    thread_->SetDeoptContext(context.GetTaggedValue());
    return JSTaggedValue::Hole();
}

void Deoptimizer::ConstructASMInterpretFrame()
{
    FrameIterator it(preFrame_, thread_);
    FrameType type = it.GetFrameType();
    if (type == FrameType::OPTIMIZED_ENTRY_FRAME) {
        DeoptFromOptimizedEntry();
    } else {
        UNREACHABLE();
    }
}
}  // namespace panda::ecmascript