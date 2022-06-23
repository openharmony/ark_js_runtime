/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_ECMA_PARAM_CONFIGURATION_H
#define ECMASCRIPT_ECMA_PARAM_CONFIGURATION_H

#include "libpandabase/mem/mem.h"

namespace panda::ecmascript {
static constexpr size_t DEFAULT_HEAP_SIZE = 256_MB;                 // Recommended range: 128-256MB
static constexpr size_t DEFAULT_WORKER_HEAP_SIZE = 64_MB;           // Recommended range: 64_MB

class EcmaParamConfiguration {
public:
    EcmaParamConfiguration(bool isWorker, size_t poolSize)
    {
        if (isWorker) {
            maxHeapSize_ = DEFAULT_WORKER_HEAP_SIZE;
        } else {
            if (poolSize >= DEFAULT_HEAP_SIZE) {
                maxHeapSize_ = DEFAULT_HEAP_SIZE;
            } else {
                maxHeapSize_ = poolSize; // pool is too small, no memory left for worker
            }
        }
        Initialize();
    }

    void Initialize()
    {
        if (maxHeapSize_ < LOW_MEMORY) {
            UNREACHABLE();
        }
        if (maxHeapSize_ < MEDIUM_MEMORY) { // 64_MB ~ 128_MB
            minSemiSpaceSize_ = 2_MB;
            maxSemiSpaceSize_ = 4_MB;
            defaultReadOnlySpaceSize_ = 256_KB;
            defaultNonMovableSpaceSize_ = 2_MB;
            defaultSnapshotSpaceSize_ = 512_KB;
            defaultMachineCodeSpaceSize_ = 2_MB;
            semiSpaceTriggerConcurrentMark_ = 1_MB;
            semiSpaceOvershootSize_ = 2_MB;
            minAllocLimitGrowingStep_ = 2_MB;
            minGrowingStep_ = 4_MB;
            maxStackSize_ = 512_KB;
        } else if (maxHeapSize_ < HIGH_MEMORY) { // 128_MB ~ 256_MB
            minSemiSpaceSize_ = 2_MB;
            maxSemiSpaceSize_ = 8_MB;
            defaultReadOnlySpaceSize_ = 256_KB;
            defaultNonMovableSpaceSize_ = 4_MB;
            defaultSnapshotSpaceSize_ = 512_KB;
            defaultMachineCodeSpaceSize_ = 2_MB;
            semiSpaceTriggerConcurrentMark_ = 1.5_MB;
            semiSpaceOvershootSize_ = 2_MB;
            minAllocLimitGrowingStep_ = 4_MB;
            minGrowingStep_ = 8_MB;
            maxStackSize_ = 512_KB;
        }  else { // 256_MB
            minSemiSpaceSize_ = 2_MB;
            maxSemiSpaceSize_ = 16_MB;
            defaultReadOnlySpaceSize_ = 256_KB;
            defaultNonMovableSpaceSize_ = 4_MB;
            defaultSnapshotSpaceSize_ = 4_MB;
            defaultMachineCodeSpaceSize_ = 8_MB;
            semiSpaceTriggerConcurrentMark_ = 1.5_MB;
            semiSpaceOvershootSize_ = 2_MB;
            minAllocLimitGrowingStep_ = 8_MB;
            minGrowingStep_ = 16_MB;
            maxStackSize_ = 512_KB;
        }
        size_t half = 2;
        defaultHugeObjectSpaceSize_ = maxHeapSize_ / half;
    }

    size_t GetMaxHeapSize() const
    {
        return maxHeapSize_;
    }

    size_t GetMinSemiSpaceSize() const
    {
        return minSemiSpaceSize_;
    }

    size_t GetMaxSemiSpaceSize() const
    {
        return maxSemiSpaceSize_;
    }

    size_t GetDefaultReadOnlySpaceSize() const
    {
        return defaultReadOnlySpaceSize_;
    }
        
    size_t GetDefaultNonMovableSpaceSize() const
    {
        return defaultNonMovableSpaceSize_;
    }

    size_t GetDefaultSnapshotSpaceSize() const
    {
        return defaultSnapshotSpaceSize_;
    }

    size_t GetDefaultMachineCodeSpaceSize() const
    {
        return defaultMachineCodeSpaceSize_;
    }

    size_t GetSemiSpaceTriggerConcurrentMark() const
    {
        return semiSpaceTriggerConcurrentMark_;
    }

    size_t GetSemiSpaceOvershootSize() const
    {
        return semiSpaceOvershootSize_;
    }

    size_t GetDefaultHugeObjectSpaceSize() const
    {
        return defaultHugeObjectSpaceSize_;
    }

    size_t GetMinAllocLimitGrowingStep() const
    {
        return minAllocLimitGrowingStep_;
    }

    size_t GetMinGrowingStep() const
    {
        return minGrowingStep_;
    }

    uint32_t GetMaxStackSize() const
    {
        return maxStackSize_;
    }

private:
    static constexpr size_t LOW_MEMORY = 64_MB;
    static constexpr size_t MEDIUM_MEMORY = 128_MB;
    static constexpr size_t HIGH_MEMORY = 256_MB;

    size_t maxHeapSize_ {0};
    size_t minSemiSpaceSize_ {0};
    size_t maxSemiSpaceSize_ {0};
    size_t defaultReadOnlySpaceSize_ {0};
    size_t defaultNonMovableSpaceSize_ {0};
    size_t defaultSnapshotSpaceSize_ {0};
    size_t defaultMachineCodeSpaceSize_ {0};
    size_t defaultHugeObjectSpaceSize_ {0};
    size_t semiSpaceTriggerConcurrentMark_ {0};
    size_t semiSpaceOvershootSize_ {0};
    size_t minAllocLimitGrowingStep_ {0};
    size_t minGrowingStep_ {0};
    uint32_t maxStackSize_ {0};
};
} // namespace panda::ecmascript

#endif // ECMASCRIPT_ECMA_PARAM_CONFIGURATION_H
