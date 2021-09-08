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

#ifndef PANDA_TOOLING_JS_EXTRACTOR_H
#define PANDA_TOOLING_JS_EXTRACTOR_H

#include "ecmascript/js_method.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/c_containers.h"
#include "libpandafile/debug_info_extractor.h"
#include "libpandabase/macros.h"
#include "include/tooling/debug_interface.h"

namespace panda::tooling::ecmascript {
using panda::ecmascript::CList;
using panda::ecmascript::EcmaVM;
using panda::ecmascript::JSMethod;
using panda::panda_file::DebugInfoExtractor;
using panda::panda_file::File;

class PtJSExtractor : public DebugInfoExtractor {
public:
    class SingleStepper {
    public:
        enum class Type { INTO, OVER, OUT };
        SingleStepper(const EcmaVM *ecmaVm, JSMethod *method, CList<PtStepRange> stepRanges, Type type)
            : ecmaVm_(ecmaVm),
              method_(method),
              stepRanges_(std::move(stepRanges)),
              stackDepth_(GetStackDepth()),
              type_(type)
        {
        }
        virtual ~SingleStepper() = default;
        NO_COPY_SEMANTIC(SingleStepper);
        NO_MOVE_SEMANTIC(SingleStepper);

        bool StepComplete(uint32_t bcOffset) const;

    private:
        uint32_t GetStackDepth() const;
        bool InStepRange(uint32_t pc) const;

        const EcmaVM *ecmaVm_;
        JSMethod *method_;
        CList<PtStepRange> stepRanges_;
        uint32_t stackDepth_;
        Type type_;
    };

    explicit PtJSExtractor(const File *pf) : DebugInfoExtractor(pf) {}
    virtual ~PtJSExtractor() = default;

    template<class Callback>
    bool MatchWithLine(const Callback &cb, int32_t line)
    {
        auto methods = GetMethodIdList();
        for (const auto &method : methods) {
            auto table = GetLineNumberTable(method);
            for (const auto &pair : table) {
                if (static_cast<int32_t>(pair.line) == line) {
                    return cb(method, pair.offset);
                }
            }
        }
        return false;
    }

    template<class Callback>
    bool MatchWithOffset(const Callback &cb, File::EntityId methodId, uint32_t offset)
    {
        auto table = GetLineNumberTable(methodId);
        panda_file::LineTableEntry prePair{0, 0};
        for (const auto &pair : table) {
            if (offset < pair.offset) {
                return cb(static_cast<int32_t>(prePair.line));
            }
            if (offset == pair.offset) {
                return cb(static_cast<int32_t>(pair.line));
            }
            prePair = pair;
        }
        return cb(static_cast<int32_t>(table.back().line));
    }

    std::unique_ptr<SingleStepper> GetStepIntoStepper(const EcmaVM *ecmaVm);
    std::unique_ptr<SingleStepper> GetStepOverStepper(const EcmaVM *ecmaVm);
    std::unique_ptr<SingleStepper> GetStepOutStepper(const EcmaVM *ecmaVm);

private:
    NO_COPY_SEMANTIC(PtJSExtractor);
    NO_MOVE_SEMANTIC(PtJSExtractor);
    CList<PtStepRange> GetStepRanges(File::EntityId methodId, uint32_t offset);
    std::unique_ptr<SingleStepper> GetStepper(const EcmaVM *ecmaVm, SingleStepper::Type type);
};
}  // namespace panda::tooling::ecmascript
#endif
