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

#include "ecmascript/tooling/test/utils/test_extractor.h"

#include "libpandabase/utils/leb128.h"
#include "libpandabase/utils/utf.h"
#include "libpandafile/class_data_accessor-inl.h"
#include "libpandafile/code_data_accessor-inl.h"
#include "libpandafile/debug_data_accessor-inl.h"
#include "libpandafile/helpers.h"
#include "libpandafile/method_data_accessor-inl.h"
#include "libpandafile/proto_data_accessor-inl.h"

namespace panda::tooling::ecmascript::test {
std::pair<EntityId, uint32_t> TestExtractor::GetBreakpointAddress(const SourceLocation &sourceLocation)
{
    EntityId retId = EntityId();
    uint32_t retOffset = 0;
    auto callbackFunc = [&retId, &retOffset](File::EntityId id, uint32_t offset) -> bool {
        retId = id;
        retOffset = offset;
        return true;
    };
    MatchWithLocation(callbackFunc, sourceLocation.line, sourceLocation.column);
    return {retId, retOffset};
}

SourceLocation TestExtractor::GetSourceLocation(EntityId methodId, uint32_t bytecodeOffset)
{
    SourceLocation location {GetSourceFile(methodId), 0, 0};
    auto callbackFunc = [&location](int32_t line, int32_t column) -> bool {
        location.line = line;
        location.column = column;
        return true;
    };
    MatchWithOffset(callbackFunc, methodId, bytecodeOffset);

    return location;
}
}  // namespace  panda::tooling::ecmascript::test
