/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/mem/work_manager.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/area.h"
#include "ecmascript/mem/full_gc.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/heap_region_allocator.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/mem/region.h"
#include "ecmascript/mem/tlab_allocator-inl.h"

namespace panda::ecmascript {
WorkManager::WorkManager(Heap *heap, uint32_t threadNum)
    : heap_(heap), threadNum_(threadNum), continuousQueue_ { nullptr }, workSpace_(0), spaceStart_(0), spaceEnd_(0),
      parallelGCTaskPhase_(UNDEFINED_TASK)
{
    for (uint32_t i = 0; i < threadNum_; i++) {
        continuousQueue_[i] = new ProcessQueue(heap);
    }
    workSpace_ =
        ToUintPtr(heap_->GetNativeAreaAllocator()->AllocateBuffer(SPACE_SIZE));
}

WorkManager::~WorkManager()
{
    Finish();
    for (uint32_t i = 0; i < threadNum_; i++) {
        continuousQueue_[i]->Destroy();
        delete continuousQueue_[i];
        continuousQueue_[i] = nullptr;
    }

    heap_->GetNativeAreaAllocator()->FreeBuffer(
        reinterpret_cast<void *>(workSpace_));
}

bool WorkManager::Push(uint32_t threadId, TaggedObject *object)
{
    WorkNode *&inNode = works_[threadId].inNode_;
    if (!inNode->PushObject(ToUintPtr(object))) {
        PushWorkNodeToGlobal(threadId);
        return inNode->PushObject(ToUintPtr(object));
    }
    return true;
}

bool WorkManager::Push(uint32_t threadId, TaggedObject *object, Region *region)
{
    if (Push(threadId, object)) {
        auto klass = object->GetClass();
        auto size = klass->SizeFromJSHClass(object);
        region->IncreaseAliveObjectSafe(size);
        return true;
    }
    return false;
}

void WorkManager::PushWorkNodeToGlobal(uint32_t threadId, bool postTask)
{
    WorkNode *&inNode = works_[threadId].inNode_;
    if (!inNode->IsEmpty()) {
        workStack_.Push(inNode);
        inNode = AllocateWorkNode();
        if (postTask && heap_->IsParallelGCEnabled() && heap_->CheckCanDistributeTask()) {
            heap_->PostParallelGCTask(parallelGCTaskPhase_);
        }
    }
}

bool WorkManager::Pop(uint32_t threadId, TaggedObject **object)
{
    WorkNode *&outNode = works_[threadId].outNode_;
    WorkNode *&inNode = works_[threadId].inNode_;
    if (!outNode->PopObject(reinterpret_cast<uintptr_t *>(object))) {
        if (!inNode->IsEmpty()) {
            WorkNode *tmp = outNode;
            outNode = inNode;
            inNode = tmp;
        } else if (!PopWorkNodeFromGlobal(threadId)) {
            return false;
        }
        return outNode->PopObject(reinterpret_cast<uintptr_t *>(object));
    }
    return true;
}

bool WorkManager::PopWorkNodeFromGlobal(uint32_t threadId)
{
    return workStack_.Pop(&works_[threadId].outNode_);
}

size_t WorkManager::Finish()
{
    size_t aliveSize = 0;
    for (uint32_t i = 0; i < threadNum_; i++) {
        WorkNodeHolder &holder = works_[i];
        if (holder.weakQueue_ != nullptr) {
            holder.weakQueue_->FinishMarking(continuousQueue_[i]);
            delete holder.weakQueue_;
            holder.weakQueue_ = nullptr;
        }
        if (holder.allocator_ != nullptr) {
            holder.allocator_->Finalize();
            delete holder.allocator_;
            holder.allocator_ = nullptr;
        }
        holder.pendingUpdateSlots_.clear();
        aliveSize += holder.aliveSize_;
    }

    while (!agedSpaces_.empty()) {
        heap_->GetNativeAreaAllocator()->FreeBuffer(reinterpret_cast<void *>(
            agedSpaces_.back()));
        agedSpaces_.pop_back();
    }
    return aliveSize;
}

void WorkManager::Finish(size_t &aliveSize, size_t &promotedSize)
{
    aliveSize = Finish();
    for (uint32_t i = 0; i < threadNum_; i++) {
        WorkNodeHolder &holder = works_[i];
        promotedSize += holder.promotedSize_;
    }
}

void WorkManager::Initialize(TriggerGCType gcType, ParallelGCTaskPhase taskPhase)
{
    parallelGCTaskPhase_ = taskPhase;
    spaceStart_ = workSpace_;
    spaceEnd_ = workSpace_ + SPACE_SIZE;
    for (uint32_t i = 0; i < threadNum_; i++) {
        WorkNodeHolder &holder = works_[i];
        holder.inNode_ = AllocateWorkNode();
        holder.outNode_ = AllocateWorkNode();
        holder.weakQueue_ = new ProcessQueue();
        holder.weakQueue_->BeginMarking(heap_, continuousQueue_[i]);
        holder.aliveSize_ = 0;
        holder.promotedSize_ = 0;
        if (gcType != TriggerGCType::OLD_GC) {
            holder.allocator_ = new TlabAllocator(heap_);
        }
    }
}

WorkNode *WorkManager::AllocateWorkNode()
{
    size_t totalSize = sizeof(WorkNode) + sizeof(Stack) + STACK_AREA_SIZE;
    ASSERT(totalSize < SPACE_SIZE);

    // CAS
    volatile auto atomicField = reinterpret_cast<volatile std::atomic<uintptr_t> *>(&spaceStart_);
    bool result = false;
    uintptr_t begin = 0;
    do {
        begin = atomicField->load(std::memory_order_acquire);
        if (begin + totalSize >= spaceEnd_) {
            os::memory::LockHolder lock(mtx_);
            begin = atomicField->load(std::memory_order_acquire);
            if (begin + totalSize >= spaceEnd_) {
                agedSpaces_.emplace_back(workSpace_);
                workSpace_ = ToUintPtr(
                    heap_->GetNativeAreaAllocator()->AllocateBuffer(SPACE_SIZE));
                spaceStart_ = workSpace_;
                spaceEnd_ = workSpace_ + SPACE_SIZE;
                begin = spaceStart_;
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
