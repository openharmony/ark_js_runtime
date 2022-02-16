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

#ifndef ECMASCRIPT_MEM_MEM_CONTROLLER_H
#define ECMASCRIPT_MEM_MEM_CONTROLLER_H

#include <chrono>

#include "ecmascript/base/gc_ring_buffer.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {
constexpr static int MILLISECONDS_PER_SECOND = 1000;

using BytesAndDuration = std::pair<uint64_t, double>;

inline BytesAndDuration MakeBytesAndDuration(uint64_t bytes, double duration)
{
    return std::make_pair(bytes, duration);
}

class MemController {
public:
    explicit MemController(Heap* heap);
    MemController() = default;
    ~MemController() = default;
    NO_COPY_SEMANTIC(MemController);
    NO_MOVE_SEMANTIC(MemController);

    static double GetSystemTimeInMs()
    {
        double currentTime =
            std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        return currentTime * MILLISECOND_PER_SECOND;
    }

    double CalculateAllocLimit(size_t currentSize, size_t minSize, size_t maxSize, size_t newSpaceCapacity,
                               double factor) const;

    double CalculateGrowingFactor(double gcSpeed, double mutatorSpeed);

    void StartCalculationBeforeGC();
    void StopCalculationAfterGC(TriggerGCType gcType);

    void RecordAfterConcurrentMark(const bool isFull, const ConcurrentMarker *marker);

    double CalculateMarkCompactSpeedPerMS();
    double GetCurrentOldSpaceAllocationThroughtputPerMS(double timeMs = THROUGHPUT_TIME_FRAME_MS) const;
    double GetNewSpaceAllocationThroughtPerMS() const;
    double GetOldSpaceAllocationThroughtPerMS() const;
    double GetNewSpaceConcurrentMarkSpeedPerMS() const;
    double GetFullSpaceConcurrentMarkSpeedPerMS() const;

    double GetAllocTimeMs() const
    {
        return allocTimeMs_;
    }

    size_t GetOldSpaceAllocAccumulatorSize() const
    {
        return oldSpaceAllocAccumulatorSize_;
    }

    size_t GetNonMovableSpaceAllocAccumulatorSize() const
    {
        return nonMovableSpaceAllocAccumulatorSize_;
    }

    size_t GetCodeSpaceAllocAccumulatorSize() const
    {
        return codeSpaceAllocAccumulatorSize_;
    }

    double GetAllocDurationSinceGc() const
    {
        return allocDurationSinceGc_;
    }

    size_t GetNewSpaceAllocSizeSinceGC() const
    {
        return newSpaceAllocSizeSinceGC_;
    }

    size_t GetOldSpaceAllocSizeSinceGC() const
    {
        return oldSpaceAllocSizeSinceGC_;
    }

    size_t GetNonMovableSpaceAllocSizeSinceGC() const
    {
        return nonMovableSpaceAllocSizeSinceGC_;
    }

    size_t GetCodeSpaceAllocSizeSinceGC() const
    {
        return codeSpaceAllocSizeSinceGC_;
    }

    size_t GetHugeObjectAllocSizeSinceGC() const
    {
        return hugeObjectAllocSizeSinceGC_;
    }

private:
    static constexpr int LENGTH = 10;
    static double CalculateAverageSpeed(const base::GCRingBuffer<BytesAndDuration, LENGTH> &buffer);
    static double CalculateAverageSpeed(const base::GCRingBuffer<BytesAndDuration, LENGTH> &buffer,
                                        const BytesAndDuration &initial, const double timeMs);

    Heap* heap_;

    double gcStartTime_ {0.0};
    double gcEndTime_ {0.0};

    // Time and allocation accumulators.
    double allocTimeMs_ {0.0};
    size_t oldSpaceAllocAccumulatorSize_ {0};
    size_t nonMovableSpaceAllocAccumulatorSize_ {0};
    size_t codeSpaceAllocAccumulatorSize_ {0};

    // Duration and allocation size in last gc.
    double allocDurationSinceGc_ {0.0};
    size_t newSpaceAllocSizeSinceGC_ {0};
    size_t oldSpaceAllocSizeSinceGC_ {0};
    size_t nonMovableSpaceAllocSizeSinceGC_ {0};
    size_t codeSpaceAllocSizeSinceGC_ {0};
    size_t hugeObjectAllocSizeSinceGC_{0};

    int startCounter_ {0};
    double markCompactSpeedCache_ {0.0};

    base::GCRingBuffer<BytesAndDuration, LENGTH> recordedMarkCompacts_;
    base::GCRingBuffer<BytesAndDuration, LENGTH> recordedNewSpaceAllocations_;
    base::GCRingBuffer<BytesAndDuration, LENGTH> recordedOldSpaceAllocations_;
    base::GCRingBuffer<BytesAndDuration, LENGTH> recordedNonmovableSpaceAllocations_;
    base::GCRingBuffer<BytesAndDuration, LENGTH> recordedCodeSpaceAllocations_;

    base::GCRingBuffer<BytesAndDuration, LENGTH> recordedConcurrentMarks_;
    base::GCRingBuffer<BytesAndDuration, LENGTH> recordedSemiConcurrentMarks_;

    static constexpr double THROUGHPUT_TIME_FRAME_MS = 5000;
    static constexpr int MILLISECOND_PER_SECOND = 1000;
};

MemController *CreateMemController(Heap *heap, std::string_view gcTriggerType);
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_MEM_CONTROLLER_H
