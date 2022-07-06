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

#include "ecmascript/mem/mem_controller.h"

#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/parallel_evacuator.h"

namespace panda::ecmascript {
MemController::MemController(Heap *heap) : heap_(heap), allocTimeMs_(GetSystemTimeInMs())
{
    minAllocLimitGrowingStep_ = heap->GetEcmaVM()->GetEcmaParamConfiguration().GetMinAllocLimitGrowingStep();
}

double MemController::CalculateAllocLimit(size_t currentSize, size_t minSize, size_t maxSize, size_t newSpaceCapacity,
                                          double factor) const
{
    const uint64_t limit = std::max(static_cast<uint64_t>(currentSize * factor),
                                    static_cast<uint64_t>(currentSize) + minAllocLimitGrowingStep_) +
                           newSpaceCapacity;

    const uint64_t limitAboveMinSize = std::max<uint64_t>(limit, minSize);
    const uint64_t halfToMaxSize = (static_cast<uint64_t>(currentSize) + maxSize) / 2;
    auto result = static_cast<size_t>(std::min(limitAboveMinSize, halfToMaxSize));
    result = static_cast<size_t>(std::min(result, maxSize));
    return result;
}

double MemController::CalculateGrowingFactor(double gcSpeed, double mutatorSpeed)
{
    double maxGrowingFactor = 4.0;
    double halfMaxGrowingFactor = 2.0;
    double minGrowingFactor = 1.3;
    double minimumFactor = 1.1;
    switch (heap_->GetMemGrowingType()) {
        case MemGrowingType::HIGH_THROUGHPUT:
            break;
        case MemGrowingType::CONSERVATIVE:
            minGrowingFactor = minimumFactor;
            maxGrowingFactor = halfMaxGrowingFactor;
            break;
        case MemGrowingType::PRESSURE:
            return minimumFactor;
        default:
            break;
    }

    static constexpr double targetMutatorUtilization = 0.97;
    if (gcSpeed == 0 || mutatorSpeed == 0) {
        return maxGrowingFactor;
    }

    const double speedRatio = gcSpeed / mutatorSpeed;

    const double a = speedRatio * (1 - targetMutatorUtilization);
    const double b = speedRatio * (1 - targetMutatorUtilization) -  targetMutatorUtilization;

    double factor = (a < b * maxGrowingFactor) ? a / b : maxGrowingFactor;
    factor = std::min(maxGrowingFactor, factor);
    factor = std::max(factor, minGrowingFactor);
    OPTIONAL_LOG(heap_->GetEcmaVM(), ERROR) << "CalculateGrowingFactor gcSpeed"
        << gcSpeed << " mutatorSpeed" << mutatorSpeed << " factor" << factor;
    return factor;
}

void MemController::StartCalculationBeforeGC()
{
    startCounter_++;
    if (startCounter_ != 1) {
        return;
    }

    // It's unnecessary to calculate newSpaceAllocAccumulatedSize. newSpaceAllocBytesSinceGC can be calculated directly.
    auto newSpace = heap_->GetNewSpace();
    size_t newSpaceAllocBytesSinceGC = newSpace->GetAllocatedSizeSinceGC(newSpace->GetTop());
    size_t hugeObjectAllocSizeSinceGC = heap_->GetHugeObjectSpace()->GetHeapObjectSize() - hugeObjectAllocSizeSinceGC_;
    size_t oldSpaceAllocAccumulatedSize = heap_->GetOldSpace()->GetTotalAllocatedSize();
    size_t nonMovableSpaceAllocAccumulatedSize = heap_->GetNonMovableSpace()->GetTotalAllocatedSize();
    size_t codeSpaceAllocAccumulatedSize = heap_->GetMachineCodeSpace()->GetTotalAllocatedSize();
    double currentTimeInMs = GetSystemTimeInMs();
    gcStartTime_ = currentTimeInMs;
    size_t oldSpaceAllocSize = oldSpaceAllocAccumulatedSize - oldSpaceAllocAccumulatedSize_;
    size_t nonMovableSpaceAllocSize = nonMovableSpaceAllocAccumulatedSize - nonMovableSpaceAllocAccumulatedSize_;
    size_t codeSpaceAllocSize = codeSpaceAllocAccumulatedSize - codeSpaceAllocAccumulatedSize_;

    double duration = currentTimeInMs - allocTimeMs_;
    allocTimeMs_ = currentTimeInMs;
    oldSpaceAllocAccumulatedSize_ = oldSpaceAllocAccumulatedSize;
    nonMovableSpaceAllocAccumulatedSize_ = nonMovableSpaceAllocAccumulatedSize;
    codeSpaceAllocAccumulatedSize_ = codeSpaceAllocAccumulatedSize;

    allocDurationSinceGc_ += duration;
    newSpaceAllocSizeSinceGC_ += newSpaceAllocBytesSinceGC;
    oldSpaceAllocSizeSinceGC_ += oldSpaceAllocSize;
    oldSpaceAllocSizeSinceGC_ += hugeObjectAllocSizeSinceGC;
    nonMovableSpaceAllocSizeSinceGC_ += nonMovableSpaceAllocSize;
    codeSpaceAllocSizeSinceGC_ += codeSpaceAllocSize;
}

void MemController::StopCalculationAfterGC(TriggerGCType gcType)
{
    startCounter_--;
    if (startCounter_ != 0) {
        return;
    }

    gcEndTime_ = GetSystemTimeInMs();
    allocTimeMs_ = gcEndTime_;
    if (allocDurationSinceGc_ > 0) {
        oldSpaceAllocSizeSinceGC_ += heap_->GetEvacuator()->GetPromotedSize();
        recordedNewSpaceAllocations_.Push(MakeBytesAndDuration(newSpaceAllocSizeSinceGC_, allocDurationSinceGc_));
        recordedOldSpaceAllocations_.Push(MakeBytesAndDuration(oldSpaceAllocSizeSinceGC_, allocDurationSinceGc_));
        recordedNonmovableSpaceAllocations_.Push(
            MakeBytesAndDuration(nonMovableSpaceAllocSizeSinceGC_, allocDurationSinceGc_));
        recordedCodeSpaceAllocations_.Push(MakeBytesAndDuration(codeSpaceAllocSizeSinceGC_, allocDurationSinceGc_));
    }
    allocDurationSinceGc_ = 0.0;
    newSpaceAllocSizeSinceGC_ = 0;
    oldSpaceAllocSizeSinceGC_ = 0;
    nonMovableSpaceAllocSizeSinceGC_ = 0;
    codeSpaceAllocSizeSinceGC_ = 0;

    hugeObjectAllocSizeSinceGC_ = heap_->GetHugeObjectSpace()->GetHeapObjectSize();

    double duration = gcEndTime_ - gcStartTime_;
    switch (gcType) {
        case TriggerGCType::YOUNG_GC:
        case TriggerGCType::OLD_GC: {
            if (heap_->IsFullMark()) {
                if (heap_->GetConcurrentMarker()->IsEnabled()) {
                    duration += heap_->GetConcurrentMarker()->GetDuration();
                }
                recordedMarkCompacts_.Push(MakeBytesAndDuration(heap_->GetHeapObjectSize(), duration));
            }
            break;
        }
        case TriggerGCType::FULL_GC: {
            recordedMarkCompacts_.Push(MakeBytesAndDuration(heap_->GetHeapObjectSize(), duration));
            break;
        }
        default:
            break;
    }
}

void MemController::RecordAfterConcurrentMark(const bool isFull, const ConcurrentMarker *marker)
{
    double duration = marker->GetDuration();
    if (isFull) {
        recordedConcurrentMarks_.Push(MakeBytesAndDuration(marker->GetHeapObjectSize(), duration));
    } else {
        recordedSemiConcurrentMarks_.Push(MakeBytesAndDuration(marker->GetHeapObjectSize(), duration));
    }
}

double MemController::CalculateMarkCompactSpeedPerMS()
{
    markCompactSpeedCache_ = CalculateAverageSpeed(recordedMarkCompacts_);
    if (markCompactSpeedCache_ > 0) {
        return markCompactSpeedCache_;
    }
    return 0;
}

double MemController::CalculateAverageSpeed(const base::GCRingBuffer<BytesAndDuration, LENGTH> &buffer,
                                            const BytesAndDuration &initial, const double timeMs)
{
    BytesAndDuration sum = buffer.Sum(
        [timeMs](BytesAndDuration a, BytesAndDuration b) {
            if (timeMs != 0 && a.second >= timeMs) {
                return a;
            }
            return std::make_pair(a.first + b.first, a.second + b.second);
        },
        initial);
    uint64_t bytes = sum.first;
    double durations = sum.second;
    if (durations == 0.0) {
        return 0;
    }
    double speed = bytes / durations;
    const int maxSpeed = 1024 * 1024 * 1024;
    const int minSpeed = 1;
    if (speed >= maxSpeed) {
        return maxSpeed;
    }
    if (speed <= minSpeed) {
        return minSpeed;
    }
    return speed;
}

double MemController::CalculateAverageSpeed(const base::GCRingBuffer<BytesAndDuration, LENGTH> &buffer)
{
    return CalculateAverageSpeed(buffer, MakeBytesAndDuration(0, 0), 0);
}

double MemController::GetCurrentOldSpaceAllocationThroughputPerMS(double timeMs) const
{
    size_t allocatedSize = oldSpaceAllocSizeSinceGC_;
    double duration = allocDurationSinceGc_;
    return CalculateAverageSpeed(recordedOldSpaceAllocations_,
                                 MakeBytesAndDuration(allocatedSize, duration), timeMs);
}

double MemController::GetNewSpaceAllocationThroughputPerMS() const
{
    return CalculateAverageSpeed(recordedNewSpaceAllocations_);
}

double MemController::GetNewSpaceConcurrentMarkSpeedPerMS() const
{
    return CalculateAverageSpeed(recordedSemiConcurrentMarks_);
}

double MemController::GetOldSpaceAllocationThroughputPerMS() const
{
    return CalculateAverageSpeed(recordedOldSpaceAllocations_);
}

double MemController::GetFullSpaceConcurrentMarkSpeedPerMS() const
{
    return CalculateAverageSpeed(recordedConcurrentMarks_);
}
}  // namespace panda::ecmascript
