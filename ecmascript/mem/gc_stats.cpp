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
void GCStats::PrintStatisticResult(bool isForce)
{
    LOG(ERROR, RUNTIME) << "/******************* GCStats statistic: *******************/";
    PrintSemiStatisticResult(isForce);
    PrintMixStatisticResult(isForce);
    PrintCompressStatisticResult(isForce);
    PrintHeapStatisticResult(isForce);
}

void GCStats::PrintSemiStatisticResult(bool isForce)
{
    if ((isForce && semiGCCount_ != 0) || (!isForce && semiGCCount_ != lastSemiGCCount_)) {
        lastSemiGCCount_ = semiGCCount_;
        LOG(ERROR, RUNTIME) << " SemiCollector statistic: total semi gc count " << semiGCCount_;
        LOG(ERROR, RUNTIME) << " MIN pause time: " << PrintTimeMilliseconds(semiGCMinPause_) << "ms"
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
}

void GCStats::PrintMixStatisticResult(bool isForce)
{
    if ((isForce && mixGCCount_ != 0) || (!isForce && lastOldGCCount_ != mixGCCount_)) {
        lastOldGCCount_ = mixGCCount_;
        LOG(ERROR, RUNTIME) << " MixCollector with non-concurrent mark statistic: total old gc count " << mixGCCount_;
        LOG(ERROR, RUNTIME) << " Pause time statistic:: MIN pause time: " << PrintTimeMilliseconds(mixGCMinPause_)
                            << "ms"
                            << " MAX pause time: " << PrintTimeMilliseconds(mixGCMAXPause_) << "ms"
                            << " total pause time: " << PrintTimeMilliseconds(mixGCTotalPause_) << "ms"
                            << " average pause time: " << PrintTimeMilliseconds(mixGCTotalPause_ / mixGCCount_) << "ms";
        if (!isForce) {
            PrintHeapStatisticResult(true);
        }
    }

    if ((isForce && mixConcurrentMarkGCCount_ != 0) ||
            (!isForce && lastOldConcurrentMarkGCCount_ != mixConcurrentMarkGCCount_)) {
        lastOldConcurrentMarkGCCount_ = mixConcurrentMarkGCCount_;
        LOG(ERROR, RUNTIME) << " MixCollector with concurrent mark statistic: total old gc count "
                            << mixConcurrentMarkGCCount_;
        LOG(ERROR, RUNTIME) << " Pause time statistic:: Current GC pause time: "
                            << PrintTimeMilliseconds(mixConcurrentMarkGCPauseTime_) << "ms"
                            << " Concurrent mark pause time: " << PrintTimeMilliseconds(mixConcurrentMarkMarkPause_)
                            << "ms"
                            << " Concurrent mark wait time: " << PrintTimeMilliseconds(mixConcurrentMarkWaitPause_)
                            << "ms"
                            << " Remark pause time: " << PrintTimeMilliseconds(mixConcurrentMarkRemarkPause_) << "ms"
                            << " Evacuate pause time: " << PrintTimeMilliseconds(mixConcurrentMarkEvacuatePause_)
                            << "ms"
                            << " MIN pause time: " << PrintTimeMilliseconds(mixConcurrentMarkGCMinPause_) << "ms"
                            << " MAX pause time: " << PrintTimeMilliseconds(mixConcurrentMarkGCMAXPause_) << "ms"
                            << " total pause time: " << PrintTimeMilliseconds(mixConcurrentMarkGCTotalPause_) << "ms"
                            << " average pause time: "
                            << PrintTimeMilliseconds(mixConcurrentMarkGCTotalPause_ / mixConcurrentMarkGCCount_)
                            << "ms";
        if (!isForce) {
            PrintHeapStatisticResult(true);
        }
    }
}

void GCStats::PrintCompressStatisticResult(bool isForce)
{
    if ((isForce && compressGCCount_ != 0) || (!isForce && compressGCCount_ != lastCompressGCCount_)) {
        lastCompressGCCount_ = compressGCCount_;
        LOG(ERROR, RUNTIME) << " compressCollector statistic: total compress gc count " << compressGCCount_;
        LOG(ERROR, RUNTIME)
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
}

void GCStats::PrintHeapStatisticResult(bool isForce)
{
    if (isForce && heap_ != nullptr) {
        RegionFactory *regionFactory = const_cast<RegionFactory *>(heap_->GetRegionFactory());
        LOG(ERROR, RUNTIME) << "/******************* Memory statistic: *******************/";
        LOG(ERROR, RUNTIME) << " Anno memory usage size:" << sizeToMB(regionFactory->GetAnnoMemoryUsage()) << "MB"
                            << " anno memory max usage size:" << sizeToMB(regionFactory->GetMaxAnnoMemoryUsage())
                            << "MB"
                            << " native memory usage size:" << sizeToMB(regionFactory->GetNativeMemoryUsage())
                            << "MB"
                            << " native memory max usage size:" << sizeToMB(regionFactory->GetMaxNativeMemoryUsage())
                            << "MB";
        LOG(ERROR, RUNTIME) << " Semi space commit size" << sizeToMB(heap_->GetNewSpace()->GetCommittedSize()) << "MB"
                            << " semi space heap object size: " << sizeToMB(heap_->GetNewSpace()->GetHeapObjectSize())
                            << "MB"
                            << " old space commit size: "
                            << sizeToMB(heap_->GetOldSpace()->GetCommittedSize()) << "MB"
                            << " old space heap object size: " << sizeToMB(heap_->GetOldSpace()->GetHeapObjectSize())
                            << "MB"
                            << " non move space commit size: "
                            << sizeToMB(heap_->GetNonMovableSpace()->GetCommittedSize()) << "MB"
                            << " huge object space commit size: "
                            << sizeToMB(heap_->GetHugeObjectSpace()->GetCommittedSize()) << "MB";
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

void GCStats::StatisticMixCollector(bool concurrentMark, Duration time, size_t freeSize)
{
    auto timeToMillion = TimeToMicroseconds(time);
    if (concurrentMark) {
        timeToMillion += mixConcurrentMarkMarkPause_;
        timeToMillion += mixConcurrentMarkWaitPause_;
        if (mixConcurrentMarkGCCount_ == 0) {
            mixConcurrentMarkGCMinPause_ = timeToMillion;
            mixConcurrentMarkGCMAXPause_ = timeToMillion;
        } else {
            mixConcurrentMarkGCMinPause_ = std::min(mixConcurrentMarkGCMinPause_, timeToMillion);
            mixConcurrentMarkGCMAXPause_ = std::max(mixConcurrentMarkGCMAXPause_, timeToMillion);
        }
        mixConcurrentMarkGCPauseTime_ = timeToMillion;
        mixConcurrentMarkGCTotalPause_ += timeToMillion;
        mixOldSpaceConcurrentMarkFreeSize_ = freeSize;
        mixConcurrentMarkGCCount_++;
    } else {
        if (mixGCCount_ == 0) {
            mixGCMinPause_ = timeToMillion;
            mixGCMAXPause_ = timeToMillion;
        } else {
            mixGCMinPause_ = std::min(mixGCMinPause_, timeToMillion);
            mixGCMAXPause_ = std::max(mixGCMAXPause_, timeToMillion);
        }
        mixGCTotalPause_ += timeToMillion;
        mixOldSpaceFreeSize_ = freeSize;
        mixGCCount_++;
    }
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

void GCStats::StatisticConcurrentMark(Duration time)
{
    mixConcurrentMarkMarkPause_ = TimeToMicroseconds(time);
}

void GCStats::StatisticConcurrentMarkWait(Duration time)
{
    mixConcurrentMarkWaitPause_ = TimeToMicroseconds(time);
}

void GCStats::StatisticConcurrentEvacuate(Duration time)
{
    mixConcurrentMarkEvacuatePause_ = TimeToMicroseconds(time);
}

void GCStats::StatisticConcurrentRemark(Duration time)
{
    mixConcurrentMarkRemarkPause_ = TimeToMicroseconds(time);
}
}  // namespace panda::ecmascript
