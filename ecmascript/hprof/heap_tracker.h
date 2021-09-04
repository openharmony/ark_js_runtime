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

#ifndef PANDA_RUNTIME_ECMASCRIPT_HPROF_HEAP_TRACKER_H
#define PANDA_RUNTIME_ECMASCRIPT_HPROF_HEAP_TRACKER_H

#include <atomic>
#include <cstdint>
#include <thread>

#include "libpandabase/macros.h"

namespace panda::ecmascript {
class HeapSnapShot;

class HeapTrackerSample {
public:
    explicit HeapTrackerSample(HeapSnapShot *snapShot, double timeInterval)
        : timeInterval_(timeInterval), snapShot_(snapShot)
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
    HeapSnapShot *snapShot_;
};

class HeapTracker {
public:
    HeapTracker(HeapSnapShot *snapShot, double timeInterval) : snapShot_(snapShot), sample_(snapShot, timeInterval) {}
    ~HeapTracker() = default;

    void StartTracing()
    {
        sample_.Start();
    }

    void StopTracing()
    {
        sample_.Stop();
    }

    void AllocationEvent(uintptr_t address);
    void MoveEvent(uintptr_t address, uintptr_t forward_address);

    NO_COPY_SEMANTIC(HeapTracker);
    NO_MOVE_SEMANTIC(HeapTracker);

private:
    HeapSnapShot *snapShot_;
    HeapTrackerSample sample_;
};
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_HPROF_HEAP_TRACKER_H
