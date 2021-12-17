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

#include "ecmascript/mem/mix_space_collector.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/ecma_heap_manager.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/heap_roots-inl.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/parallel_evacuation.h"
#include "ecmascript/mem/parallel_marker-inl.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/vmstat/runtime_stat.h"

namespace panda::ecmascript {
MixSpaceCollector::MixSpaceCollector(Heap *heap) : heap_(heap), workList_(heap->GetWorkList()) {}

void MixSpaceCollector::RunPhases()
{
    [[maybe_unused]] ecmascript::JSThread *thread = heap_->GetEcmaVM()->GetJSThread();
    INTERPRETER_TRACE(thread, MixSpaceCollector_RunPhases);
    ClockScope clockScope;

    concurrentMark_ = heap_->CheckConcurrentMark(thread);
    // SelectCSet
    InitializePhase();
    MarkingPhase();
    SweepPhases();
    EvacuaPhases();
    FinishPhase();
    heap_->GetEcmaVM()->GetEcmaGCStats()->StatisticOldCollector(
        clockScope.GetPauseTime(), freeSize_, oldSpaceCommitSize_, nonMoveSpaceCommitSize_);
    ECMA_GC_LOG() << "MixSpaceCollector::RunPhases " << clockScope.TotalSpentTime();
}

void MixSpaceCollector::InitializePhase()
{
    if (!concurrentMark_) {
        heap_->Prepare();
        heap_->EnumerateRegions([](Region *current) {
            // ensure mark bitmap
            auto bitmap = current->GetMarkBitmap();
            if (bitmap == nullptr) {
                current->GetOrCreateMarkBitmap();
            } else {
                bitmap->ClearAllBits();
            }
            auto rememberset = current->GetCrossRegionRememberedSet();
            if (rememberset != nullptr) {
                rememberset->ClearAllBits();
            }
            current->SetMarking(false);
            current->ResetAvailable();
        });
        workList_->Initialize(TriggerGCType::OLD_GC, ParallelGCTaskPhase::OLD_HANDLE_GLOBAL_POOL_TASK);

        freeSize_ = 0;
        hugeSpaceFreeSize_ = 0;
        oldSpaceCommitSize_ = heap_->GetOldSpace()->GetCommittedSize();
        nonMoveSpaceCommitSize_ = heap_->GetNonMovableSpace()->GetCommittedSize();
    }
}

void MixSpaceCollector::FinishPhase()
{
    if (concurrentMark_) {
        auto marker = heap_->GetConcurrentMarker();
        marker->Reset(false);
    } else {
        size_t aliveSize = 0;
        workList_->Finish(aliveSize);
        heap_->EnumerateRegions([this](Region *current) {
            current->ResetAvailable();
            current->ClearFlag(RegionFlags::IS_IN_PROMOTE_SET);
        });
    }
}

void MixSpaceCollector::MarkingPhase()
{
    if (concurrentMark_) {
        [[maybe_unused]] ClockScope scope;
        heap_->GetConcurrentMarker()->ReMarking();
        return;
    }
    trace::ScopedTrace scoped_trace("MixSpaceCollector::MarkingPhase");
    heap_->GetNonMovableMarker()->MarkRoots(0);
    if (heap_->IsOnlyMarkSemi()) {
        heap_->GetNonMovableMarker()->ProcessOldToNew(0);
    } else {
        heap_->GetNonMovableMarker()->ProcessMarkStack(0);
    }
    heap_->WaitRunningTaskFinished();
}

void MixSpaceCollector::SweepPhases()
{
    trace::ScopedTrace scoped_trace("MixSpaceCollector::SweepPhases");
    if (!heap_->IsOnlyMarkSemi()) {
        heap_->GetSweeper()->SweepPhases();
    }
}

void MixSpaceCollector::EvacuaPhases()
{
    heap_->GetEvacuation()->Evacuate();
}
}  // namespace panda::ecmascript
