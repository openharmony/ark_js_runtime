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

#ifndef ECMASCRIPT_HPROF_HEAP_PROFILER_INTERFACE_H
#define ECMASCRIPT_HPROF_HEAP_PROFILER_INTERFACE_H

#include "ecmascript/mem/c_string.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
enum class DumpFormat { JSON, BINARY, OTHER };

class HeapProfilerInterface {
public:
    static void DumpHeapSnapShot(JSThread *thread, DumpFormat dumpFormat,
                                 const CString &filePath, bool isVmMode = true);

    static HeapProfilerInterface *CreateHeapProfiler(JSThread *thread);
    static void Destroy(JSThread *thread, HeapProfilerInterface *heapProfiler);

    HeapProfilerInterface() = default;
    virtual ~HeapProfilerInterface() = default;

    virtual bool StartHeapTracking(JSThread *thread, double timeInterval, bool isVmMode = true) = 0;
    virtual bool StopHeapTracking(JSThread *thread, DumpFormat dumpFormat, const CString &filePath) = 0;

    NO_MOVE_SEMANTIC(HeapProfilerInterface);
    NO_COPY_SEMANTIC(HeapProfilerInterface);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_HPROF_HEAP_PROFILER_INTERFACE_H
