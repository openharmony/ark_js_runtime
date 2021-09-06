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

#ifndef PANDA_RUNTIME_ECMASCRIPT_MEM_GC_STATS_H
#define PANDA_RUNTIME_ECMASCRIPT_MEM_GC_STATS_H

#include "time.h"
#include "chrono"
#include "libpandabase/utils/logger.h"

namespace panda::ecmascript {
class GCStats {
    using Duration = std::chrono::duration<uint64_t, std::nano>;

public:
    GCStats() = default;
    ~GCStats() = default;

    void PrintStatisticResult();

    void StatisticSemiCollector(Duration time, size_t aliveSize, size_t promoteSize, size_t commitSize);
    void StatisticOldCollector(Duration time, size_t freeSize, size_t oldSpaceCommitSize,
                               size_t nonMoveSpaceCommitSize);
    void StatisticCompressCollector(Duration time, size_t youngAndOldAliveSize, size_t youngCommitSize,
                                    size_t oldCommitSize, size_t nonMoveSpaceFreeSize, size_t nonMoveSpaceCommitSize);

private:
    size_t TimeToMicroseconds(Duration time)
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(time).count();
    }

    float PrintTimeMilliseconds(uint64_t time)
    {
        return (float)time / MILLION_TIME;
    }

    float sizeToMB(size_t size)
    {
        return (float)size / MB;
    }

    size_t semiGCCount_ = 0;
    size_t semiGCMinPause_ = 0;
    size_t semiGCMAXPause_ = 0;
    size_t semiGCTotalPause_ = 0;
    size_t semiTotalAliveSize_ = 0;
    size_t semiTotalCommitSize_ = 0;
    size_t semiTotalPromoteSize_ = 0;

    size_t oldGCCount_ = 0;
    size_t oldGCMinPause_ = 0;
    size_t oldGCMAXPause_ = 0;
    size_t oldGCTotalPause_ = 0;
    size_t oldTotalFreeSize_ = 0;
    size_t oldSpaceTotalCommitSize_ = 0;
    size_t oldNonMoveTotalCommitSize_ = 0;

    size_t compressGCCount_ = 0;
    size_t compressGCMinPause_ = 0;
    size_t compressGCMaxPause_ = 0;
    size_t compressGCTotalPause_ = 0;
    size_t compressYoungAndOldAliveSize_ = 0;
    size_t compressYoungCommitSize_ = 0;
    size_t compressOldCommitSize_ = 0;
    size_t compressNonMoveTotalFreeSize_ = 0;
    size_t compressNonMoveTotalCommitSize_ = 0;

    static constexpr uint32_t MILLION_TIME = 1000;
    static constexpr uint32_t MB = 1 * 1024 * 1024;

    NO_COPY_SEMANTIC(GCStats);
    NO_MOVE_SEMANTIC(GCStats);
};
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_MEM_GC_STATS_H