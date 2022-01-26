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

#include "ecmascript/tooling/interface/debugger_api.h"
#include "ecmascript/tooling/pt_js_extractor.h"

namespace panda::tooling::ecmascript {
using panda::ecmascript::InterpretedFrameHandler;
using panda::ecmascript::JSTaggedType;
uint32_t PtJSExtractor::SingleStepper::GetStackDepth() const
{
    return DebuggerApi::GetStackDepth(ecmaVm_);
}

bool PtJSExtractor::SingleStepper::InStepRange(uint32_t pc) const
{
    for (const auto &range : stepRanges_) {
        if (pc >= range.start_bc_offset && pc < range.end_bc_offset) {
            return true;
        }
    }
    return false;
}

bool PtJSExtractor::SingleStepper::StepComplete(uint32_t bcOffset) const
{
    JSMethod *method = DebuggerApi::GetMethod(ecmaVm_);
    uint32_t stackDepth = GetStackDepth();

    switch (type_) {
        case Type::INTO: {
            if (method_ == method && InStepRange(bcOffset)) {
                return false;
            }
            break;
        }
        case Type::OVER: {
            if (stackDepth_ < stackDepth) {
                return false;
            }
            if (stackDepth_ == stackDepth && InStepRange(bcOffset)) {
                return false;
            }
            break;
        }
        case SingleStepper::Type::OUT: {
            if (stackDepth_ <= stackDepth) {
                return false;
            }
            break;
        }
        default: {
            return false;
        }
    }

    return true;
}

std::unique_ptr<PtJSExtractor::SingleStepper> PtJSExtractor::GetStepIntoStepper(const EcmaVM *ecmaVm)
{
    return GetStepper(ecmaVm, SingleStepper::Type::INTO);
}

std::unique_ptr<PtJSExtractor::SingleStepper> PtJSExtractor::GetStepOverStepper(const EcmaVM *ecmaVm)
{
    return GetStepper(ecmaVm, SingleStepper::Type::OVER);
}

std::unique_ptr<PtJSExtractor::SingleStepper> PtJSExtractor::GetStepOutStepper(const EcmaVM *ecmaVm)
{
    return GetStepper(ecmaVm, SingleStepper::Type::OUT);
}

CList<PtStepRange> PtJSExtractor::GetStepRanges(File::EntityId methodId, uint32_t offset)
{
    CList<PtStepRange> ranges {};
    auto table = GetLineNumberTable(methodId);
    auto callbackFunc = [table, &ranges](size_t line, [[maybe_unused]] size_t column) -> bool {
        for (auto it = table.begin(); it != table.end(); ++it) {
            auto next = it + 1;
            if (it->line == line) {
                PtStepRange range {it->offset, next != table.end() ? next->offset : UINT32_MAX};
                ranges.push_back(range);
            }
        }
        return true;
    };
    MatchWithOffset(callbackFunc, methodId, offset);

    return ranges;
}

std::unique_ptr<PtJSExtractor::SingleStepper> PtJSExtractor::GetStepper(const EcmaVM *ecmaVm, SingleStepper::Type type)
{
    JSMethod *method = DebuggerApi::GetMethod(ecmaVm);
    ASSERT(method != nullptr);

    if (type == SingleStepper::Type::OUT) {
        return std::make_unique<SingleStepper>(ecmaVm, method, CList<PtStepRange> {}, type);
    }

    CList<PtStepRange> ranges = GetStepRanges(method->GetFileId(), DebuggerApi::GetBytecodeOffset(ecmaVm));
    return std::make_unique<SingleStepper>(ecmaVm, method, std::move(ranges), type);
}
}  // namespace panda::tooling::ecmascript
