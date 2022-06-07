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

#ifndef ECMASCRIPT_HPROF_HEAP_TRACKER_H
#define ECMASCRIPT_HPROF_HEAP_TRACKER_H

#include <atomic>
#include <cstdint>
#include <thread>

#include "libpandabase/macros.h"
#include "ecmascript/tooling/interface/stream.h"
#include "ecmascript/mem/tagged_object.h"

namespace panda::ecmascript {
class HeapSnapshot;

class HeapTrackerSample {
public:
    explicit HeapTrackerSample(HeapSnapshot *snapshot, double timeInterval, Stream *stream)
        : timeInterval_(timeInterval), snapshot_(snapshot), stream_(stream)
    {
    }

    ~HeapTrackerSample()
    {
        isInterrupt_ = true;
    }

    void Start()
    {
        isInterrupt_ = false;
        thread_ = std::thread(&HeapTrackerSample::Run, this);
    }

    void Stop()
    {
        isInterrupt_ = true;
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    void Run();

    NO_COPY_SEMANTIC(HeapTrackerSample);
    NO_MOVE_SEMANTIC(HeapTrackerSample);

private:
    std::thread thread_;
    std::atomic_bool isInterrupt_ = true;
    double timeInterval_ = 0;
    HeapSnapshot *snapshot_;
    Stream *stream_ {nullptr};
};

class HeapTracker {
public:
    HeapTracker(HeapSnapshot *snapshot, double timeInterval, Stream *stream)
        : snapshot_(snapshot), sample_(snapshot, timeInterval, stream) {}
    ~HeapTracker() = default;

    void StartTracing()
    {
        sample_.Start();
    }

    void StopTracing()
    {
        sample_.Stop();
    }

    void AllocationEvent(TaggedObject* address);
    void MoveEvent(uintptr_t address, TaggedObject* forward_address);

    NO_COPY_SEMANTIC(HeapTracker);
    NO_MOVE_SEMANTIC(HeapTracker);

private:
    HeapSnapshot *snapshot_;
    HeapTrackerSample sample_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_HPROF_HEAP_TRACKER_H
