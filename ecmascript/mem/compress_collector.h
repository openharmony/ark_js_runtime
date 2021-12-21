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

#ifndef ECMASCRIPT_MEM_COMPRESS_COLLECTOR_H
#define ECMASCRIPT_MEM_COMPRESS_COLLECTOR_H

#include "ecmascript/mem/parallel_work_helper.h"
#include "ecmascript/mem/semi_space_collector.h"

namespace panda {
namespace ecmascript {
class Heap;
class JSHClass;

class CompressCollector : public GarbageCollector {
public:
    explicit CompressCollector(Heap *heap);
    ~CompressCollector() override = default;

    NO_COPY_SEMANTIC(CompressCollector);
    NO_MOVE_SEMANTIC(CompressCollector);

    void RunPhases();

private:
    void InitializePhase();
    void MarkingPhase();
    void SweepPhases();
    void FinishPhase();

    Heap *heap_;
    size_t youngAndOldAliveSize_ = 0;
    size_t nonMoveSpaceFreeSize_ = 0;
    size_t youngSpaceCommitSize_ = 0;
    size_t oldSpaceCommitSize_ = 0;
    size_t nonMoveSpaceCommitSize_ = 0;

    // obtain from heap
    WorkerHelper *workList_ {nullptr};

    friend class WorkerHelper;
    friend class Heap;
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_MEM_COMPRESS_COLLECTOR_H
