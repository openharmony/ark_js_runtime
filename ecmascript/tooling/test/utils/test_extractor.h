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

#ifndef ECMASCRIPT_TOOLING_TEST_UTILS_TEST_EXTRACTOR_H
#define ECMASCRIPT_TOOLING_TEST_UTILS_TEST_EXTRACTOR_H

#include "ecmascript/tooling/js_pt_extractor.h"

namespace panda::tooling::ecmascript::test {
using EntityId = panda_file::File::EntityId;
using panda::ecmascript::CString;

// NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
struct SourceLocation {
    CString path;  // NOLINT(misc-non-private-member-variables-in-classes)
    int32_t line;          // NOLINT(misc-non-private-member-variables-in-classes)
    int32_t column;

    bool operator==(const SourceLocation &other) const
    {
        return path == other.path && line == other.line && column == other.column;
    }

    bool IsValid() const
    {
        return !path.empty();
    }
};

class TestExtractor : public JSPtExtractor {
public:
    explicit TestExtractor(const JSPandaFile *pandaFile) : JSPtExtractor(pandaFile) {}
    ~TestExtractor() = default;

    std::pair<EntityId, uint32_t> GetBreakpointAddress(const SourceLocation &sourceLocation);

    SourceLocation GetSourceLocation(EntityId methodId, uint32_t bytecodeOffset);
};
}  // namespace panda::tooling::ecmascript::test

#endif  // ECMASCRIPT_TOOLING_TEST_UTILS_TEST_EXTRACTOR_H
