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
#include "ecmascript/tooling/interface/stream.h"
#include "ecmascript/tooling/interface/progress.h"

namespace panda::ecmascript {
enum class DumpFormat { JSON, BINARY, OTHER };

class HeapProfilerInterface {
public:
    static HeapProfilerInterface *GetInstance(const EcmaVM *vm);
    static void Destroy(const EcmaVM *vm);

    HeapProfilerInterface() = default;
    virtual ~HeapProfilerInterface() = default;

    virtual bool DumpHeapSnapshot(DumpFormat dumpFormat, Stream *stream, Progress *progress = nullptr,
                                  bool isVmMode = true, bool isPrivate = false) = 0;

    virtual bool StartHeapTracking(double timeInterval, bool isVmMode = true,
                                   Stream *stream = nullptr, bool traceAllocation = false) = 0;
    virtual bool StopHeapTracking(Stream *stream, Progress *progress = nullptr) = 0;

    NO_MOVE_SEMANTIC(HeapProfilerInterface);
    NO_COPY_SEMANTIC(HeapProfilerInterface);

private:
    static HeapProfilerInterface *heapProfile_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_HPROF_HEAP_PROFILER_INTERFACE_H
