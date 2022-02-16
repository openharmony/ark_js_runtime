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

#include "ecmascript/mem/parallel_work_helper.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/area.h"
#include "ecmascript/mem/full_gc.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mix_gc.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/region_factory.h"
#include "ecmascript/mem/tlab_allocator-inl.h"

namespace panda::ecmascript {
WorkerHelper::WorkerHelper(Heap *heap, uint32_t threadNum)
    : heap_(heap), threadNum_(threadNum), markSpace_(0), spaceTop_(0), markSpaceEnd_(0)
{
    for (uint32_t i = 0; i < threadNum_; i++) {
        continuousQueue_[i] = new ProcessQueue(heap);
    }
    markSpace_ = ToUintPtr(const_cast<RegionFactory *>(heap_->GetRegionFactory())->AllocateBuffer(SPACE_SIZE));
}

WorkerHelper::~WorkerHelper()
{
    for (uint32_t i = 0; i < threadNum_; i++) {
        continuousQueue_[i]->Destroy();
        delete continuousQueue_[i];
        continuousQueue_[i] = nullptr;
    }
    const_cast<RegionFactory *>(heap_->GetRegionFactory())->FreeBuffer(reinterpret_cast<void *>(markSpace_));
}

bool WorkerHelper::Push(uint32_t threadId, TaggedObject *object)
{
    WorkNode *&pushNode = workList_[threadId].pushNode_;
    if (!pushNode->Push(ToUintPtr(object))) {
        PushWorkNodeToGlobal(threadId);
        return pushNode->Push(ToUintPtr(object));
    }
    return true;
}

bool WorkerHelper::Push(uint32_t threadId, TaggedObject *object, Region *region)
{
    if (Push(threadId, object)) {
        auto klass = object->GetClass();
        auto size = klass->SizeFromJSHClass(object);
        region->IncrementAliveObjectSafe(size);
        return true;
    }
    return false;
}

void WorkerHelper::PushWorkNodeToGlobal(uint32_t threadId, bool postTask)
{
    WorkNode *&pushNode = workList_[threadId].pushNode_;
    if (!pushNode->IsEmpty()) {
        globalWork_.Push(pushNode);
        pushNode = AllocalWorkNode();
        if (postTask && heap_->IsParallelGCEnabled() && heap_->CheckCanDistributeTask()) {
            heap_->PostParallelGCTask(parallelTask_);
        }
    }
}

bool WorkerHelper::Pop(uint32_t threadId, TaggedObject **object)
{
    WorkNode *&popNode = workList_[threadId].popNode_;
    WorkNode *&pushNode = workList_[threadId].pushNode_;
    if (!popNode->Pop(reinterpret_cast<uintptr_t *>(object))) {
        if (!pushNode->IsEmpty()) {
            WorkNode *tmp = popNode;
            popNode = pushNode;
            pushNode = tmp;
        } else if (!PopWorkNodeFromGlobal(threadId)) {
            return false;
        }
        return popNode->Pop(reinterpret_cast<uintptr_t *>(object));
    }
    return true;
}

bool WorkerHelper::PopWorkNodeFromGlobal(uint32_t threadId)
{
    return globalWork_.Pop(&workList_[threadId].popNode_);
}

void WorkerHelper::Finish(size_t &aliveSize)
{
    for (uint32_t i = 0; i < threadNum_; i++) {
        WorkNodeHolder &holder = workList_[i];
        holder.weakQueue_->FinishMarking(continuousQueue_[i]);
        delete holder.weakQueue_;
        holder.weakQueue_ = nullptr;
        if (holder.allocator_ != nullptr) {
            holder.allocator_->Finalize();
            delete holder.allocator_;
            holder.allocator_ = nullptr;
        }
        holder.waitUpdate_.clear();
        aliveSize += holder.aliveSize_;
    }

    while (!unuseSpace_.empty()) {
        const_cast<RegionFactory *>(heap_->GetRegionFactory())->FreeBuffer(reinterpret_cast<void *>(
            unuseSpace_.back()));
        unuseSpace_.pop_back();
    }
}

void WorkerHelper::Finish(size_t &aliveSize, size_t &promoteSize)
{
    Finish(aliveSize);
    for (uint32_t i = 0; i < threadNum_; i++) {
        WorkNodeHolder &holder = workList_[i];
        promoteSize += holder.aliveSize_;
    }
}

void WorkerHelper::Initialize(TriggerGCType gcType, ParallelGCTaskPhase parallelTask)
{
    parallelTask_ = parallelTask;
    spaceTop_ = markSpace_;
    markSpaceEnd_ = markSpace_ + SPACE_SIZE;
    for (uint32_t i = 0; i < threadNum_; i++) {
        WorkNodeHolder &holder = workList_[i];
        holder.pushNode_ = AllocalWorkNode();
        holder.popNode_ = AllocalWorkNode();
        holder.weakQueue_ = new ProcessQueue();
        holder.weakQueue_->BeginMarking(heap_, continuousQueue_[i]);
        holder.aliveSize_ = 0;
        holder.promoteSize_ = 0;
        if (gcType != TriggerGCType::OLD_GC) {
            holder.allocator_ = new TlabAllocator(heap_);
        }
    }
}

WorkNode *WorkerHelper::AllocalWorkNode()
{
    size_t totalSize = sizeof(WorkNode) + sizeof(Stack) + STACK_AREA_SIZE;
    // CAS
    volatile auto atomicField = reinterpret_cast<volatile std::atomic<uintptr_t> *>(&spaceTop_);
    bool result = false;
    uintptr_t begin = 0;
    do {
        begin = atomicField->load(std::memory_order_acquire);
        if (begin + totalSize >= markSpaceEnd_) {
            os::memory::LockHolder lock(mtx_);
            begin = atomicField->load(std::memory_order_acquire);
            if (begin + totalSize >= markSpaceEnd_) {
                unuseSpace_.emplace_back(markSpace_);
                markSpace_ =
                    ToUintPtr(const_cast<RegionFactory *>(heap_->GetRegionFactory())->AllocateBuffer(SPACE_SIZE));
                spaceTop_ = markSpace_;
                markSpaceEnd_ = markSpace_ + SPACE_SIZE;
                begin = spaceTop_;
            }
        }
        result = std::atomic_compare_exchange_strong_explicit(atomicField, &begin, begin + totalSize,
                                                              std::memory_order_release, std::memory_order_relaxed);
    } while (!result);
    Stack *stack = reinterpret_cast<Stack *>(begin + sizeof(WorkNode));
    stack->ResetBegin(begin + sizeof(WorkNode) + sizeof(Stack), begin + totalSize);
    WorkNode *work = reinterpret_cast<WorkNode *>(begin);
    return new (work) WorkNode(stack);
}
}  // namespace panda::ecmascript
