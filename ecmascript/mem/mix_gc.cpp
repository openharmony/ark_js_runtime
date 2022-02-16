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

#include "ecmascript/mem/mix_gc.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/mem_manager.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/object_xray-inl.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/parallel_evacuation.h"
#include "ecmascript/mem/parallel_marker-inl.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
MixGC::MixGC(Heap *heap) : heap_(heap), workList_(heap->GetWorkList()) {}

void MixGC::RunPhases()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "MixGC::RunPhases");
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), MixGC_RunPhases);
    ClockScope clockScope;

    concurrentMark_ = heap_->CheckConcurrentMark();

    ECMA_GC_LOG() << "concurrentMark_" << concurrentMark_;
    InitializePhase();
    MarkingPhase();
    SweepPhases();
    EvacuaPhases();
    FinishPhase();
    heap_->GetEcmaVM()->GetEcmaGCStats()->StatisticMixGC(concurrentMark_, clockScope.GetPauseTime(), freeSize_);
    ECMA_GC_LOG() << "MixGC::RunPhases " << clockScope.TotalSpentTime();
}

void MixGC::InitializePhase()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "MixGC::InitializePhase");
    if (!concurrentMark_) {
        LOG(INFO, RUNTIME) << "Concurrent mark failure";
        heap_->Prepare();
        if (heap_->IsFullMark()) {
            if (heap_->GetSweeper()->IsOldSpaceSweeped()) {
                const_cast<OldSpace *>(heap_->GetOldSpace())->SelectCSet();
            }
            heap_->GetSweeper()->SetOldSpaceSweeped(false);
            heap_->EnumerateNonNewSpaceRegions([this](Region *current) {
                current->ResetAliveObject();
            });
        }
        workList_->Initialize(TriggerGCType::OLD_GC, ParallelGCTaskPhase::OLD_HANDLE_GLOBAL_POOL_TASK);

        freeSize_ = 0;
        hugeSpaceFreeSize_ = 0;
        oldSpaceCommitSize_ = heap_->GetOldSpace()->GetCommittedSize();
        nonMoveSpaceCommitSize_ = heap_->GetNonMovableSpace()->GetCommittedSize();
    }
}

void MixGC::FinishPhase()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "MixGC::FinishPhase");
    if (concurrentMark_) {
        auto marker = heap_->GetConcurrentMarker();
        marker->Reset(false);
    } else {
        size_t aliveSize = 0;
        workList_->Finish(aliveSize);
    }
}

void MixGC::MarkingPhase()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "MixGC::MarkingPhase");
    if (concurrentMark_) {
        heap_->GetConcurrentMarker()->ReMarking();
        return;
    }
    heap_->GetNonMovableMarker()->MarkRoots(0);
    if (heap_->IsFullMark()) {
        heap_->GetNonMovableMarker()->ProcessMarkStack(0);
    } else {
        heap_->GetNonMovableMarker()->ProcessOldToNew(0);
    }
    heap_->WaitRunningTaskFinished();
}

void MixGC::SweepPhases()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "MixGC::SweepPhases");
    if (heap_->IsFullMark()) {
        heap_->GetSweeper()->SweepPhases();
    }
}

void MixGC::EvacuaPhases()
{
    ECMA_BYTRACE_NAME(BYTRACE_TAG_ARK, "MixGC::EvacuaPhases");
    heap_->GetEvacuation()->Evacuate();
}
}  // namespace panda::ecmascript