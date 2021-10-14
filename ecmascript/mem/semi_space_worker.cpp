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

#include "ecmascript/mem/semi_space_worker.h"

#include "ecmascript/mem/area.h"
#include "ecmascript/mem/compress_collector.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/old_space_collector.h"
#include "ecmascript/mem/region_factory.h"
#include "ecmascript/mem/tlab_allocator-inl.h"

namespace panda::ecmascript {
void Worker::Finish(size_t &aliveSize)
{
    for (uint32_t i = 0; i < threadNum_; i++) {
        WorkNodeHolder &holder = workList_[i];
        holder.weakQueue_->FinishMarking(continuousQueue_[i]);
        delete holder.weakQueue_;
        holder.weakQueue_ = nullptr;
        delete holder.allocator_;
        holder.allocator_ = nullptr;
        holder.waitUpdate_.clear();
        aliveSize += holder.aliveSize_;
    }

    while (!unuseSpace_.empty()) {
        const_cast<RegionFactory *>(heap_->GetRegionFactory())->FreeBuffer(reinterpret_cast<void *>(
            unuseSpace_.back()));
        unuseSpace_.pop_back();
    }
}

void Worker::Finish(size_t &aliveSize, size_t &promoteSize)
{
    Finish(aliveSize);
    for (uint32_t i = 0; i < threadNum_; i++) {
        WorkNodeHolder &holder = workList_[i];
        promoteSize += holder.aliveSize_;
    }
}

bool Worker::Push(uint32_t threadId, TaggedObject *object)
{
    WorkNode *&pushNode = workList_[threadId].pushNode_;
    if (!pushNode->Push(ToUintPtr(object))) {
        PushWorkNodeToGlobal(threadId);
        return pushNode->Push(ToUintPtr(object));
    }
    return true;
}

bool Worker::Pop(uint32_t threadId, TaggedObject **object)
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

bool Worker::PopWorkNodeFromGlobal(uint32_t threadId)
{
    return globalWork_.Pop(&workList_[threadId].popNode_);
}

WorkNode *Worker::AllocalWorkNode()
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

Worker::Worker(Heap *heap, uint32_t threadNum)
    : heap_(heap), threadNum_(threadNum), markSpace_(0), spaceTop_(0), markSpaceEnd_(0)
{
    for (uint32_t i = 0; i < threadNum_; i++) {
        continuousQueue_[i] = new ProcessQueue(heap);
    }
    markSpace_ = ToUintPtr(const_cast<RegionFactory *>(heap_->GetRegionFactory())->AllocateBuffer(SPACE_SIZE));
}

Worker::~Worker()
{
    for (uint32_t i = 0; i < threadNum_; i++) {
        continuousQueue_[i]->Destroy();
        delete continuousQueue_[i];
        continuousQueue_[i] = nullptr;
    }
    const_cast<RegionFactory *>(heap_->GetRegionFactory())->FreeBuffer(reinterpret_cast<void *>(markSpace_));
}

SemiSpaceWorker::~SemiSpaceWorker() = default;

CompressGCWorker::~CompressGCWorker() = default;

void SemiSpaceWorker::PushWorkNodeToGlobal(uint32_t threadId)
{
    WorkNode *&pushNode = workList_[threadId].pushNode_;
    if (!pushNode->IsEmpty()) {
        globalWork_.Push(pushNode);
        pushNode = AllocalWorkNode();

        auto pool = heap_->GetThreadPool();
        if (pool->GetTaskCount() < pool->GetThreadNum() - 1) {
            pool->Submit(std::bind(&SemiSpaceCollector::ParallelHandleGlobalPool, heap_->GetSemiSpaceCollector(),
                                   std::placeholders::_1));
        }
    }
}

void SemiSpaceWorker::Initialize()
{
    spaceTop_ = markSpace_;
    markSpaceEnd_ = markSpace_ + SPACE_SIZE;
    for (uint32_t i = 0; i < threadNum_; i++) {
        WorkNodeHolder &holder = workList_[i];
        holder.pushNode_ = AllocalWorkNode();
        holder.popNode_ = AllocalWorkNode();
        holder.weakQueue_ = new ProcessQueue();
        holder.weakQueue_->BeginMarking(heap_, continuousQueue_[i]);
        holder.allocator_ = new TlabAllocator(heap_, TriggerGCType::SEMI_GC);
        holder.aliveSize_ = 0;
        holder.promoteSize_ = 0;
    }
}

void CompressGCWorker::PushWorkNodeToGlobal(uint32_t threadId)
{
    WorkNode *&pushNode = workList_[threadId].pushNode_;
    if (!pushNode->IsEmpty()) {
        globalWork_.Push(pushNode);
        pushNode = AllocalWorkNode();

        auto pool = heap_->GetThreadPool();
        if (pool->GetTaskCount() < pool->GetThreadNum() - 1) {
            pool->Submit(
                std::bind(&CompressCollector::ProcessMarkStack, heap_->GetCompressCollector(), std::placeholders::_1));
        }
    }
}

void CompressGCWorker::Initialize()
{
    spaceTop_ = markSpace_;
    markSpaceEnd_ = markSpace_ + SPACE_SIZE;
    for (uint32_t i = 0; i < threadNum_; i++) {
        WorkNodeHolder &holder = workList_[i];
        holder.pushNode_ = AllocalWorkNode();
        holder.popNode_ = AllocalWorkNode();
        holder.weakQueue_ = new ProcessQueue();
        holder.weakQueue_->BeginMarking(heap_, continuousQueue_[i]);
        holder.allocator_ = new TlabAllocator(heap_, TriggerGCType::COMPRESS_FULL_GC);
        holder.aliveSize_ = 0;
    }
}

void OldGCWorker::Initialize()
{
    spaceTop_ = markSpace_;
    markSpaceEnd_ = markSpace_ + SPACE_SIZE;
    for (uint32_t i = 0; i < threadNum_; i++) {
        WorkNodeHolder &holder = workList_[i];
        holder.pushNode_ = AllocalWorkNode();
        holder.popNode_ = AllocalWorkNode();
        holder.weakQueue_ = new ProcessQueue();
        holder.weakQueue_->BeginMarking(heap_, continuousQueue_[i]);
    }
}

void OldGCWorker::PushWorkNodeToGlobal(uint32_t threadId)
{
    WorkNode *&pushNode = workList_[threadId].pushNode_;
    if (!pushNode->IsEmpty()) {
        globalWork_.Push(pushNode);
        pushNode = AllocalWorkNode();

        auto pool = heap_->GetThreadPool();
        if (pool->GetTaskCount() < pool->GetThreadNum() - 1) {
            pool->Submit(std::bind(&OldSpaceCollector::ProcessMarkStack, heap_->GetOldSpaceCollector(),
                std::placeholders::_1));
        }
    }
}
}  // namespace panda::ecmascript
