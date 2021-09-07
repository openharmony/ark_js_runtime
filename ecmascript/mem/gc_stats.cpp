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

#include "ecmascript/mem/gc_stats.h"

#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {
void GCStats::PrintStatisticResult()
{
    LOG(DEBUG, RUNTIME) << "GCStats statistic: ";
    if (semiGCCount_ != 0) {
        LOG(DEBUG, RUNTIME) << " SemiCollector statistic: total semi gc count " << semiGCCount_;
        LOG(DEBUG, RUNTIME) << " MIN pause time: " << PrintTimeMilliseconds(semiGCMinPause_) << "ms"
                            << " MAX pause time: " << PrintTimeMilliseconds(semiGCMAXPause_) << "ms"
                            << " total pause time: " << PrintTimeMilliseconds(semiGCTotalPause_) << "ms"
                            << " average pause time: " << PrintTimeMilliseconds(semiGCTotalPause_ / semiGCCount_)
                            << "ms"
                            << " tatal alive size: " << sizeToMB(semiTotalAliveSize_) << "MB"
                            << " average alive size: " << sizeToMB(semiTotalAliveSize_ / semiGCCount_) << "MB"
                            << " tatal commit size: " << sizeToMB(semiTotalCommitSize_) << "MB"
                            << " average commit size: " << sizeToMB(semiTotalCommitSize_ / semiGCCount_) << "MB"
                            << " semi aliveRate: " << double(semiTotalAliveSize_) / semiTotalCommitSize_
                            << " total promote size: " << sizeToMB(semiTotalPromoteSize_) << "MB"
                            << " average promote size: " << sizeToMB(semiTotalPromoteSize_ / semiGCCount_) << "MB";
    }

    if (oldGCCount_ != 0) {
        LOG(DEBUG, RUNTIME) << " oldCollector statistic: total old gc count " << oldGCCount_;
        LOG(DEBUG, RUNTIME) << " MIN pause time: " << PrintTimeMilliseconds(oldGCMinPause_) << "ms"
                            << " MAX pause time: " << PrintTimeMilliseconds(oldGCMAXPause_) << "ms"
                            << " total pause time: " << PrintTimeMilliseconds(oldGCTotalPause_) << "ms"
                            << " average pause time: " << PrintTimeMilliseconds(oldGCTotalPause_ / oldGCCount_) << "ms"
                            << " total free size: " << sizeToMB(oldTotalFreeSize_) << "MB"
                            << " average free size: " << sizeToMB(oldTotalFreeSize_ / oldGCCount_) << "MB"
                            << " old space total commit size: " << sizeToMB(oldSpaceTotalCommitSize_) << "MB"
                            << " old space average commit size: " << sizeToMB(oldSpaceTotalCommitSize_ / oldGCCount_)
                            << "MB"
                            << " non move space total commit size: " << sizeToMB(oldNonMoveTotalCommitSize_) << "MB"
                            << " non move space average commit size: "
                            << sizeToMB(oldNonMoveTotalCommitSize_ / oldGCCount_) << "MB"
                            << " old free rate: "
                            << float(oldTotalFreeSize_) / (oldSpaceTotalCommitSize_ + oldNonMoveTotalCommitSize_);
    }

    if (compressGCCount_ != 0) {
        LOG(DEBUG, RUNTIME) << " compressCollector statistic: total compress gc count " << compressGCCount_;
        LOG(DEBUG, RUNTIME)
            << " MIN pause time: " << PrintTimeMilliseconds(compressGCMinPause_) << "ms"
            << " MAX pause time: " << PrintTimeMilliseconds(compressGCMaxPause_) << "ms"
            << " total pause time: " << PrintTimeMilliseconds(compressGCTotalPause_) << "ms"
            << " average pause time: " << PrintTimeMilliseconds(compressGCTotalPause_ / compressGCCount_) << "ms"
            << " young and old total alive size: " << sizeToMB(compressYoungAndOldAliveSize_) << "MB"
            << " young and old average alive size: " << sizeToMB(compressYoungAndOldAliveSize_ / compressGCCount_)
            << "MB"
            << " young total commit size: " << sizeToMB(compressYoungCommitSize_) << "MB"
            << " old total commit size: " << sizeToMB(compressOldCommitSize_) << "MB"
            << " young and old average commit size: "
            << sizeToMB((compressYoungCommitSize_ + compressOldCommitSize_) / compressGCCount_) << "MB"
            << " young and old free rate: "
            << 1 - float(compressYoungAndOldAliveSize_) / (compressYoungCommitSize_ + compressOldCommitSize_)
            << " non move total free size: " << sizeToMB(compressNonMoveTotalFreeSize_) << "MB"
            << " non move total commit size: " << sizeToMB(compressNonMoveTotalCommitSize_) << "MB"
            << " non move free rate: " << float(compressNonMoveTotalFreeSize_) / compressNonMoveTotalCommitSize_;
    }

    if (heap_ != nullptr) {
        RegionFactory *regionFactory = const_cast<RegionFactory *>(heap_->GetRegionFactory());
        LOG(DEBUG, RUNTIME) << "pool statistic:: "
                            << "anno memory usage size:" << regionFactory->GetAnnoMemoryUsage()
                            << "anno memory max usage size:" << regionFactory->GetMaxAnnoMemoryUsage()
                            << "native memory usage size:" << regionFactory->GetNativeMemoryUsage()
                            << "native memory max usage size:" << regionFactory->GetMaxNativeMemoryUsage();
    }
}

void GCStats::StatisticSemiCollector(Duration time, size_t aliveSize, size_t promoteSize, size_t commitSize)
{
    auto timeToMillion = TimeToMicroseconds(time);
    if (semiGCCount_ == 0) {
        semiGCMinPause_ = timeToMillion;
        semiGCMAXPause_ = timeToMillion;
    } else {
        semiGCMinPause_ = std::min(semiGCMinPause_, timeToMillion);
        semiGCMAXPause_ = std::max(semiGCMAXPause_, timeToMillion);
    }
    semiGCTotalPause_ += timeToMillion;
    semiTotalAliveSize_ += aliveSize;
    semiTotalCommitSize_ += commitSize;
    semiTotalPromoteSize_ += promoteSize;
    semiGCCount_++;
}

void GCStats::StatisticOldCollector(Duration time, size_t freeSize, size_t oldSpaceCommitSize,
                                    size_t nonMoveSpaceCommitSize)
{
    auto timeToMillion = TimeToMicroseconds(time);
    if (oldGCCount_ == 0) {
        oldGCMinPause_ = timeToMillion;
        oldGCMAXPause_ = timeToMillion;
    } else {
        oldGCMinPause_ = std::min(oldGCMinPause_, timeToMillion);
        oldGCMAXPause_ = std::max(oldGCMAXPause_, timeToMillion);
    }
    oldGCTotalPause_ += timeToMillion;
    oldTotalFreeSize_ += freeSize;
    oldSpaceTotalCommitSize_ += oldSpaceCommitSize;
    oldNonMoveTotalCommitSize_ += nonMoveSpaceCommitSize;
    oldGCCount_++;
}

void GCStats::StatisticCompressCollector(Duration time, size_t youngAndOldAliveSize, size_t youngCommitSize,
                                         size_t oldCommitSize, size_t nonMoveSpaceFreeSize,
                                         size_t nonMoveSpaceCommitSize)
{
    auto timeToMillion = TimeToMicroseconds(time);
    if (compressGCCount_ == 0) {
        compressGCMinPause_ = timeToMillion;
        compressGCMaxPause_ = timeToMillion;
    } else {
        compressGCMinPause_ = std::min(compressGCMinPause_, timeToMillion);
        compressGCMaxPause_ = std::max(compressGCMaxPause_, timeToMillion);
    }
    compressGCTotalPause_ += timeToMillion;
    compressYoungAndOldAliveSize_ += youngAndOldAliveSize;
    compressYoungCommitSize_ += youngCommitSize;
    compressOldCommitSize_ += oldCommitSize;
    compressNonMoveTotalFreeSize_ += nonMoveSpaceFreeSize;
    compressNonMoveTotalCommitSize_ += nonMoveSpaceCommitSize;
    compressGCCount_++;
}
}  // namespace panda::ecmascript
