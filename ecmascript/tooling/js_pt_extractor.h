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

#ifndef ECMASCRIPT_TOOLING_JS_PT_EXTRACTOR_H
#define ECMASCRIPT_TOOLING_JS_PT_EXTRACTOR_H

#include "ecmascript/js_method.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/debug_info_extractor.h"
#include "ecmascript/mem/c_containers.h"
#include "libpandabase/macros.h"
#include "include/tooling/debug_interface.h"

namespace panda::tooling::ecmascript {
using panda::ecmascript::CList;
using panda::ecmascript::DebugInfoExtractor;
using panda::ecmascript::EcmaVM;
using panda::ecmascript::JSMethod;
using panda::panda_file::File;

class JSPtExtractor : public DebugInfoExtractor {
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

    explicit JSPtExtractor(const File *pf) : DebugInfoExtractor(pf) {}
    virtual ~JSPtExtractor() = default;

    template<class Callback>
    bool MatchWithLocation(const Callback &cb, size_t line, size_t column)
    {
        auto methods = GetMethodIdList();
        for (const auto &method : methods) {
            auto lineTable = GetLineNumberTable(method);
            auto columnTable = GetColumnNumberTable(method);
            for (uint32_t i = 0; i < lineTable.size(); i++) {
                if (lineTable[i].line != line) {
                    continue;
                }
                uint32_t currentOffset = lineTable[i].offset;
                uint32_t nextOffset = ((i == lineTable.size() - 1) ? UINT32_MAX : lineTable[i + 1].offset);
                for (const auto &pair : columnTable) {
                    if (pair.column == column && pair.offset >= currentOffset && pair.offset < nextOffset) {
                        return cb(method, pair.offset);
                    }
                }
                return cb(method, currentOffset);
            }
        }
        return false;
    }

    template<class Callback>
    bool MatchWithOffset(const Callback &cb, File::EntityId methodId, uint32_t offset)
    {
        auto lineTable = GetLineNumberTable(methodId);
        auto columnTable = GetColumnNumberTable(methodId);
        size_t line = 0;
        size_t column = 0;

        for (const auto &pair : lineTable) {
            if (offset < pair.offset) {
                break;
            } else if (offset == pair.offset) {
                line = pair.line;
                break;
            }
            line = pair.line;
        }

        for (const auto &pair : columnTable) {
            if (offset < pair.offset) {
                break;
            } else if (offset == pair.offset) {
                column = pair.column;
                break;
            }
            column = pair.column;
        }
        return cb(line, column);
    }

    std::unique_ptr<SingleStepper> GetStepIntoStepper(const EcmaVM *ecmaVm);
    std::unique_ptr<SingleStepper> GetStepOverStepper(const EcmaVM *ecmaVm);
    std::unique_ptr<SingleStepper> GetStepOutStepper(const EcmaVM *ecmaVm);

private:
    NO_COPY_SEMANTIC(JSPtExtractor);
    NO_MOVE_SEMANTIC(JSPtExtractor);
    CList<PtStepRange> GetStepRanges(File::EntityId methodId, uint32_t offset);
    std::unique_ptr<SingleStepper> GetStepper(const EcmaVM *ecmaVm, SingleStepper::Type type);
};
}  // namespace panda::tooling::ecmascript
#endif  // ECMASCRIPT_TOOLING_JS_PT_EXTRACTOR_H
