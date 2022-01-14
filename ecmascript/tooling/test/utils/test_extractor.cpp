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
std::pair<EntityId, uint32_t> TestExtractor::GetBreakpointAddress(const SourceLocation &source_location)
{
    auto pos = source_location.path.find_last_of("/\\");
    auto name = source_location.path;

    if (pos != CString::npos) {
        name = name.substr(pos + 1);
    }

    std::vector<panda_file::File::EntityId> methods = GetMethodIdList();
    for (const auto &method : methods) {
        auto srcName = CString(GetSourceFile(method));
        auto pos_sf = srcName.find_last_of("/\\");
        if (pos_sf != CString::npos) {
            srcName = srcName.substr(pos_sf + 1);
        }
        if (srcName == name) {
            const panda_file::LineNumberTable &lineTable = GetLineNumberTable(method);
            if (lineTable.empty()) {
                continue;
            }

            std::optional<size_t> offset = GetOffsetByTableLineNumber(lineTable, source_location.line);
            if (offset == std::nullopt) {
                continue;
            }
            return {method, offset.value()};
        }
    }
    return {EntityId(), 0};
}

CList<PtStepRange> TestExtractor::GetStepRanges(EntityId method_id, uint32_t current_offset)
{
    const panda_file::LineNumberTable &lineTable = GetLineNumberTable(method_id);
    if (lineTable.empty()) {
        return {};
    }

    std::optional<size_t> line = GetLineNumberByTableOffset(lineTable, current_offset);
    if (line == std::nullopt) {
        return {};
    }

    CList<PtStepRange> res;
    for (auto it = lineTable.begin(); it != lineTable.end(); ++it) {
        if (it->line == line) {
            size_t idx = it - lineTable.begin();
            if (it + 1 != lineTable.end()) {
                res.push_back({lineTable[idx].offset, lineTable[idx + 1].offset});
            } else {
                res.push_back({lineTable[idx].offset, std::numeric_limits<uint32_t>::max()});
            }
        }
    }
    return res;
}

std::vector<panda_file::LocalVariableInfo> TestExtractor::GetLocalVariableInfo(EntityId method_id, size_t offset)
{
    const std::vector<panda_file::LocalVariableInfo> &variables = GetLocalVariableTable(method_id);
    std::vector<panda_file::LocalVariableInfo> result;

    for (const auto &variable : variables) {
        if (variable.start_offset <= offset && offset <= variable.end_offset) {
            result.push_back(variable);
        }
    }
    return result;
}

std::optional<size_t> TestExtractor::GetLineNumberByTableOffset(const panda_file::LineNumberTable &table,
                                                                uint32_t offset)
{
    for (const auto &value : table) {
        if (value.offset == offset) {
            return value.line;
        }
    }
    return std::nullopt;
}

std::optional<uint32_t> TestExtractor::GetOffsetByTableLineNumber(const panda_file::LineNumberTable &table, size_t line)
{
    for (const auto &value : table) {
        if (value.line == line) {
            return value.offset;
        }
    }
    return std::nullopt;
}

SourceLocation TestExtractor::GetSourceLocation(EntityId method_id, uint32_t bytecode_offset)
{
    const panda_file::LineNumberTable &lineTable = GetLineNumberTable(method_id);
    if (lineTable.empty()) {
        return SourceLocation();
    }

    std::optional<size_t> line = GetLineNumberByTableOffset(lineTable, bytecode_offset);
    if (line == std::nullopt) {
        return SourceLocation();
    }

    return SourceLocation{GetSourceFile(method_id), line.value()};
}
}  // namespace  panda::tooling::ecmascript::test
