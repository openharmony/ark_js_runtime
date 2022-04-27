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

#ifndef ECMASCRIPT_MEM_FULL_GC_H
#define ECMASCRIPT_MEM_FULL_GC_H

#include "ecmascript/mem/garbage_collector.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/work_manager.h"

namespace panda {
namespace ecmascript {
class FullGC : public GarbageCollector {
public:
    explicit FullGC(Heap *heap);
    ~FullGC() override = default;

    NO_COPY_SEMANTIC(FullGC);
    NO_MOVE_SEMANTIC(FullGC);

    void RunPhases() override;

protected:
    void Initialize() override;
    void Mark() override;
    void Sweep() override;
    void Finish() override;

private:
    Heap *heap_;
    size_t youngAndOldAliveSize_ = 0;
    size_t nonMoveSpaceFreeSize_ = 0;
    size_t youngSpaceCommitSize_ = 0;
    size_t oldSpaceCommitSize_ = 0;
    size_t nonMoveSpaceCommitSize_ = 0;

    // obtain from heap
    WorkManager *workManager_ {nullptr};

    friend class WorkManager;
    friend class Heap;
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_MEM_FULL_GC_H
