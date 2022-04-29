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

#ifndef ECMASCRIPT_MEM_CONCURRENT_MARKER_H
#define ECMASCRIPT_MEM_CONCURRENT_MARKER_H

#include <array>
#include <atomic>

#include "ecmascript/mem/space.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/mem/work_manager.h"
#include "ecmascript/taskpool/task.h"
#include "os/mutex.h"

namespace panda::ecmascript {
class Heap;

class ConcurrentMarker {
public:
    explicit ConcurrentMarker(Heap *heap);
    ~ConcurrentMarker() = default;

    void ConcurrentMarking();
    void FinishPhase();
    void ReMarking();

    void HandleMarkFinished();  // call in vm thread.
    void WaitConcurrentMarkingFinished();  // call in main thread
    void Reset(bool isRevertCSet = true);

    double GetDuration() const
    {
        return duration_;
    }

    double GetHeapObjectSize() const
    {
        return heapObjectSize_;
    }

private:
    NO_COPY_SEMANTIC(ConcurrentMarker);
    NO_MOVE_SEMANTIC(ConcurrentMarker);

    class MarkerTask : public Task {
    public:
        explicit MarkerTask(Heap *heap) : heap_(heap) {}
        ~MarkerTask() override = default;
        bool Run(uint32_t threadId) override;

    private:
        NO_COPY_SEMANTIC(MarkerTask);
        NO_MOVE_SEMANTIC(MarkerTask);

        Heap *heap_ {nullptr};
    };

    void SetDuration(double duration)
    {
        duration_ = duration;
    }

    void InitializeMarking();
    void MarkingFinished(float spendTime);

    Heap *heap_ {nullptr};
    EcmaVM *vm_ {nullptr};
    JSThread *thread_ {nullptr};

    // obtain from heap
    WorkManager *workManager_ {nullptr};
    size_t heapObjectSize_ {0};
    double duration_ {0.0};

    bool notifyMarkingFinished_ {false};             // notify js-thread that marking is finished and need sweep
    bool vmThreadWaitMarkingFinished_ {false};   // jsMainThread waiting for concurrentGC FINISHED
    os::memory::Mutex waitMarkingFinishedMutex_;
    os::memory::ConditionVariable waitMarkingFinishedCV_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_CONCURRENT_MARKER_H
