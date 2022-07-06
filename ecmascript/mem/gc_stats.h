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

#ifndef ECMASCRIPT_MEM_GC_STATS_H
#define ECMASCRIPT_MEM_GC_STATS_H

#include <chrono>
#include <string>
#include "time.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
class Heap;
class GCStats {
    using Duration = std::chrono::duration<uint64_t, std::nano>;

public:
    explicit GCStats(const Heap *heap) : heap_(heap) {}
    explicit GCStats(const Heap *heap, size_t longPuaseTime) : heap_(heap),
        longPauseTime_(longPuaseTime) {}
    ~GCStats() = default;

    void PrintStatisticResult(bool force = false);
    void PrintHeapStatisticResult(bool force = true);

    void StatisticSTWYoungGC(Duration time, size_t aliveSize, size_t promotedSize, size_t commitSize);
    void StatisticPartialGC(bool concurrentMark, Duration time, size_t freeSize);
    void StatisticFullGC(Duration time, size_t youngAndOldAliveSize, size_t youngCommitSize,
                         size_t oldCommitSize, size_t nonMoveSpaceFreeSize, size_t nonMoveSpaceCommitSize);
    void StatisticConcurrentMark(Duration time);
    void StatisticConcurrentMarkWait(Duration time);
    void StatisticConcurrentRemark(Duration time);
    void StatisticConcurrentEvacuate(Duration time);

    void CheckIfLongTimePause();
private:
    void PrintSemiStatisticResult(bool force);
    void PrintPartialStatisticResult(bool force);
    void PrintCompressStatisticResult(bool force);

    size_t TimeToMicroseconds(Duration time)
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(time).count();
    }

    float PrintTimeMilliseconds(uint64_t ms)
    {
        return (float)ms / THOUSAND;
    }

    float sizeToMB(size_t size)
    {
        return (float)size / MB;
    }

    size_t lastSemiGCCount_ = 0;
    size_t semiGCCount_ = 0;
    size_t semiGCMinPause_ = 0;
    size_t semiGCMaxPause_ = 0;
    size_t semiGCTotalPause_ = 0;
    size_t semiTotalAliveSize_ = 0;
    size_t semiTotalCommitSize_ = 0;
    size_t semiTotalPromoteSize_ = 0;

    size_t lastOldGCCount_ = 0;
    size_t partialGCCount_ = 0;
    size_t partialGCMinPause_ = 0;
    size_t partialGCMaxPause_ = 0;
    size_t partialGCTotalPause_ = 0;
    size_t partialOldSpaceFreeSize_ = 0;

    size_t lastOldConcurrentMarkGCCount_ = 0;
    size_t partialConcurrentMarkGCPauseTime_ = 0;
    size_t partialConcurrentMarkMarkPause_ = 0;
    size_t partialConcurrentMarkWaitPause_ = 0;
    size_t partialConcurrentMarkRemarkPause_ = 0;
    size_t partialConcurrentMarkEvacuatePause_ = 0;
    size_t partialConcurrentMarkGCCount_ = 0;
    size_t partialConcurrentMarkGCMinPause_ = 0;
    size_t partialConcurrentMarkGCMaxPause_ = 0;
    size_t partialConcurrentMarkGCTotalPause_ = 0;
    size_t partialOldSpaceConcurrentMarkFreeSize_ = 0;

    size_t lastFullGCCount_ = 0;
    size_t fullGCCount_ = 0;
    size_t fullGCMinPause_ = 0;
    size_t fullGCMaxPause_ = 0;
    size_t fullGCTotalPause_ = 0;
    size_t compressYoungAndOldAliveSize_ = 0;
    size_t compressYoungCommitSize_ = 0;
    size_t compressOldCommitSize_ = 0;
    size_t compressNonMoveTotalFreeSize_ = 0;
    size_t compressNonMoveTotalCommitSize_ = 0;

    const Heap *heap_;
    std::string currentGcType_ = "";
    size_t currentPauseTime_ = 0;
    size_t longPauseTime_ = 0;

    static constexpr uint32_t THOUSAND = 1000;
    static constexpr uint32_t MB = 1 * 1024 * 1024;

    NO_COPY_SEMANTIC(GCStats);
    NO_MOVE_SEMANTIC(GCStats);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_GC_STATS_H
