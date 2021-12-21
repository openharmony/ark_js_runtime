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

#ifndef ECMASCRIPT_COMPILER_JS_THREAD_OFFSET_TABLE_H
#define ECMASCRIPT_COMPILER_JS_THREAD_OFFSET_TABLE_H

#include <array>
#include "ecmascript/js_thread.h"

// this class only use in host compiling
namespace kungfu {
using JSThread = panda::ecmascript::JSThread;
class OffsetTable {
public:
    static void CreateInstance(const char* triple);
    static void Destroy();
    static OffsetTable *GetInstance()
    {
        ASSERT(instance_);
        return instance_;
    }

    static uint32_t GetOffset(JSThread::GlueID id)
    {
        ASSERT(instance_);
        return instance_->offsetTable_[static_cast<size_t>(id)];
    }

private:
    explicit OffsetTable(const char* triple);
    ~OffsetTable() = default;

    static OffsetTable *instance_;
    std::array<uint32_t, static_cast<size_t>(JSThread::GlueID::NUMBER_OF_GLUE)> offsetTable_ {};

    NO_COPY_SEMANTIC(OffsetTable);
    NO_MOVE_SEMANTIC(OffsetTable);
};
}  // namespace kungfu
#endif  // ECMASCRIPT_COMPILER_JS_THREAD_OFFSET_TABLE_H
