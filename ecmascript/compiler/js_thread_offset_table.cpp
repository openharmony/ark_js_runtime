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

#include "ecmascript/compiler/js_thread_offset_table.h"
#include "ecmascript/compiler/triple.h"

namespace kungfu {
using namespace panda::ecmascript;
OffsetTable *OffsetTable::instance_ = nullptr;
OffsetTable::OffsetTable(const char* triple)
{
    if (triple == TripleConst::GetLLVMArm32Triple()) {
        offsetTable_ = {
            GLUE_EXCEPTION_OFFSET_32, GLUE_GLOBAL_CONSTANTS_OFFSET_32, GLUE_PROPERTIES_CACHE_OFFSET_32,
            GLUE_GLOBAL_STORAGE_OFFSET_32, GLUE_CURRENT_FRAME_OFFSET_32, GLUE_LAST_IFRAME_OFFSET_32,
            GLUE_RUNTIME_FUNCTIONS_OFFSET_32, GLUE_FASTSTUB_ENTRIES_OFFSET_32
        };
    } else if (triple == TripleConst::GetLLVMArm64Triple() || triple == TripleConst::GetLLVMAmd64Triple()) {
        offsetTable_ = {
            GLUE_EXCEPTION_OFFSET_64, GLUE_GLOBAL_CONSTANTS_OFFSET_64, GLUE_PROPERTIES_CACHE_OFFSET_64,
            GLUE_GLOBAL_STORAGE_OFFSET_64, GLUE_CURRENT_FRAME_OFFSET_64, GLUE_LAST_IFRAME_OFFSET_64,
            GLUE_RUNTIME_FUNCTIONS_OFFSET_64, GLUE_FASTSTUB_ENTRIES_OFFSET_64
        };
    } else {
        UNREACHABLE();
    }
}
// static
void OffsetTable::CreateInstance(const char* triple)
{
    instance_ = new OffsetTable(triple);
}
// static
void OffsetTable::Destroy()
{
    if (instance_) {
        delete instance_;
        instance_ = nullptr;
    }
}
}  // namespace kungfu
