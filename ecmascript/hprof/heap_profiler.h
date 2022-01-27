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

#ifndef ECMASCRIPT_HPROF_HEAP_PROFILER_H
#define ECMASCRIPT_HPROF_HEAP_PROFILER_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/hprof/heap_profiler_interface.h"
#include "ecmascript/hprof/heap_snapshot_json_serializer.h"
#include "ecmascript/hprof/heap_tracker.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/heap.h"
#include "os/mem.h"

namespace panda::ecmascript {
class HeapSnapShot;
class HeapProfiler : public HeapProfilerInterface {
public:
    NO_MOVE_SEMANTIC(HeapProfiler);
    NO_COPY_SEMANTIC(HeapProfiler);
    explicit HeapProfiler(const Heap *heap) : heap_(heap)
    {
        jsonSerializer_ = const_cast<RegionFactory *>(heap->GetRegionFactory())->New<HeapSnapShotJSONSerializer>();
        if (UNLIKELY(jsonSerializer_ == nullptr)) {
            LOG_ECMA(FATAL) << "alloc snapshot json serializer failed";
            UNREACHABLE();
        }
    }
    ~HeapProfiler() override;

    enum class SampleType { ONE_SHOT, REAL_TIME };
    /**
     * dump the specific snapshot in target format
     */
    bool DumpHeapSnapShot(JSThread *thread, DumpFormat dumpFormat, const std::string &path, bool isVmMode = true);
    void AddSnapShot(HeapSnapShot *snapshot);

    bool StartHeapTracking(JSThread *thread, double timeInterval, bool isVmMode = true) override;
    bool StopHeapTracking(JSThread *thread, DumpFormat dumpFormat, const std::string &filePath) override;

private:
    /**
     * tigger full gc to make sure no unreachable objects in heap
     */
    bool ForceFullGC(JSThread *thread);

    /**
     * make a new heap snapshot and put it into a container eg, vector
     */
    HeapSnapShot *MakeHeapSnapShot(JSThread *thread, SampleType sampleType, bool isVmMode = true);
    std::pair<bool, CString> FilePathValid(const std::string &filePath);
    std::string GenDumpFileName(DumpFormat dumpFormat);
    CString GetTimeStamp();
    void ClearSnapShot();

    const size_t MAX_NUM_HPROF = 5;  // ~10MB
    CVector<HeapSnapShot *> hprofs_;
    HeapSnapShotJSONSerializer *jsonSerializer_{nullptr};
    std::unique_ptr<HeapTracker> heapTracker_;
    const Heap *heap_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_HPROF_HEAP_PROFILER_H
