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
// Taken from panda-tools/panda-debugger/debugger
#ifndef PANDA_RUNTIME_DEBUG_TEST_DEBUG_INFO_EXTRACTOR_H
#define PANDA_RUNTIME_DEBUG_TEST_DEBUG_INFO_EXTRACTOR_H

#include "ecmascript/mem/c_string.h"
#include "ecmascript/tooling/pt_js_extractor.h"

namespace panda::tooling::ecmascript::test {
using EntityId = panda_file::File::EntityId;
using panda::ecmascript::CString;
using panda::tooling::ecmascript::PtJSExtractor;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct SourceLocation {
    CString path;  // NOLINT(misc-non-private-member-variables-in-classes)
    size_t line;          // NOLINT(misc-non-private-member-variables-in-classes)

    bool operator==(const SourceLocation &other) const
    {
        return path == other.path && line == other.line;
    }

    bool IsValid() const
    {
        return !path.empty();
    }
};

class TestExtractor : public PtJSExtractor {
public:
    explicit TestExtractor(const panda_file::File *pandaFileData) : PtJSExtractor(pandaFileData) {}
    ~TestExtractor() = default;

    std::pair<EntityId, uint32_t> GetBreakpointAddress(const SourceLocation &sourceLocation);

    CList<PtStepRange> GetStepRanges(EntityId methodId, uint32_t currentOffset);

    std::vector<panda_file::LocalVariableInfo> GetLocalVariableInfo(EntityId methodId, size_t offset);

    SourceLocation GetSourceLocation(EntityId methodId, uint32_t bytecodeOffset);

    std::optional<size_t> GetLineNumberByTableOffset(const panda_file::LineNumberTable &table, uint32_t offset);
    std::optional<uint32_t> GetOffsetByTableLineNumber(const panda_file::LineNumberTable &table, size_t line);
};
}  // namespace panda::tooling::ecmascript::test

#endif  // PANDA_RUNTIME_DEBUG_TEST_DEBUG_INFO_EXTRACTOR_H
