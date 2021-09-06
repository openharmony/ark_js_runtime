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

#ifndef PANDA_RUNTIME_ECMASCRIPT_MEM_SEMI_SPACE_WORKER_H
#define PANDA_RUNTIME_ECMASCRIPT_MEM_SEMI_SPACE_WORKER_H

#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/slots.h"

namespace panda::ecmascript {
using SlotNeedUpdate = std::pair<TaggedObject *, ObjectSlot>;

static constexpr uint32_t MARKSTACK_MAX_SIZE = 100;
static constexpr uint32_t THREAD_NUM_FOR_YOUNG_GC = 6;
static constexpr uint32_t STACK_AREA_SIZE = sizeof(uintptr_t *) * MARKSTACK_MAX_SIZE;

static constexpr uint32_t SPACE_SIZE = 8 * 1024;

class Heap;
class Stack;
class SemiSpaceCollector;
class TlabAllocator;

class WorkNode {
public:
    explicit WorkNode(Stack *stack) : next_(nullptr), stack_(stack) {}
    ~WorkNode()
    {
        delete stack_;
        stack_ = nullptr;
    }

    NO_COPY_SEMANTIC(WorkNode);
    NO_MOVE_SEMANTIC(WorkNode);

    bool Push(uintptr_t obj)
    {
        return stack_->PushBackChecked(obj);
    }

    bool Pop(uintptr_t *obj)
    {
        if (IsEmpty()) {
            return false;
        }
        auto object = ToVoidPtr(*obj);
        if (object != nullptr) {
            delete reinterpret_cast<WorkNode *>(object);
        }
        *obj = stack_->PopBackUnchecked();
        return true;
    }

    bool IsEmpty() const
    {
        return stack_->IsEmpty();
    }

    WorkNode *Next() const
    {
        return next_;
    }

    void SetNext(WorkNode *node)
    {
        next_ = node;
    }

private:
    WorkNode *next_;
    Stack *stack_;
};

class GlobalWorkList {
public:
    GlobalWorkList() : top_(nullptr) {}
    ~GlobalWorkList() = default;

    NO_COPY_SEMANTIC(GlobalWorkList);
    NO_MOVE_SEMANTIC(GlobalWorkList);

    void Push(WorkNode *node)
    {
        if (node == nullptr) {
            return;
        }
        os::memory::LockHolder lock(mtx_);
        node->SetNext(top_);
        top_ = node;
    }

    bool Pop(WorkNode **node)
    {
        os::memory::LockHolder lock(mtx_);
        if (top_ == nullptr) {
            return false;
        }
        *node = top_;
        top_ = top_->Next();
        return true;
    }

private:
    WorkNode *top_;
    os::memory::Mutex mtx_;
};

struct WorkNodeHolder {
    WorkNode *pushNode_{nullptr};
    WorkNode *popNode_{nullptr};
    ProcessQueue *weakQueue_{nullptr};
    std::vector<SlotNeedUpdate> waitUpdate_;
    TlabAllocator *allocator_{nullptr};
    size_t aliveSize_ = 0;
    size_t promoteSize_ = 0;
};

class Worker {
public:
    Worker() = delete;
    explicit Worker(Heap *heap, uint32_t threadNum);

    virtual ~Worker() = 0;
    virtual void PushWorkNodeToGlobal(uint32_t threadId) = 0;
    virtual void Initialize() = 0;

    void Finish(size_t &aliveSize);
    void Finish(size_t &aliveSize, size_t &promoteSize);

    bool Push(uint32_t threadId, TaggedObject *object);
    bool Pop(uint32_t threadId, TaggedObject **object);

    bool PopWorkNodeFromGlobal(uint32_t threadId);

    void PushWeakReference(uint32_t threadId, JSTaggedType *weak)
    {
        workList_[threadId].weakQueue_->PushBack(weak);
    }

    void AddAliveSize(uint32_t threadId, size_t size)
    {
        workList_[threadId].aliveSize_ += size;
    }

    void AddPromoteSize(uint32_t threadId, size_t size)
    {
        workList_[threadId].promoteSize_ += size;
    }

    ProcessQueue *GetWeakReferenceQueue(uint32_t threadId) const
    {
        return workList_[threadId].weakQueue_;
    }

    TlabAllocator *GetTlabAllocator(uint32_t threadId) const
    {
        return workList_[threadId].allocator_;
    }

    NO_COPY_SEMANTIC(Worker);
    NO_MOVE_SEMANTIC(Worker);

protected:
    WorkNode *AllocalWorkNode();

    Heap *heap_;          // NOLINT(misc-non-private-member-variables-in-classes)
    uint32_t threadNum_;  // NOLINT(misc-non-private-member-variables-in-classes)
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes, modernize-avoid-c-arrays)
    WorkNodeHolder workList_[THREAD_NUM_FOR_YOUNG_GC];
    // NOLINTNEXTLINE(misc-non-private-member-variables-in-classes, modernize-avoid-c-arrays)
    ContinuousStack<JSTaggedType> *continuousQueue_[THREAD_NUM_FOR_YOUNG_GC];
    GlobalWorkList globalWork_;  // NOLINT(misc-non-private-member-variables-in-classes)

    uintptr_t markSpace_;     // NOLINT(misc-non-private-member-variables-in-classes)
    uintptr_t spaceTop_;      // NOLINT(misc-non-private-member-variables-in-classes)
    uintptr_t markSpaceEnd_;  // NOLINT(misc-non-private-member-variables-in-classes)

private:
    std::vector<uintptr_t> unuseSpace_;
    os::memory::Mutex mtx_;
};

class SemiSpaceWorker : public Worker {
public:
    SemiSpaceWorker() = delete;
    explicit SemiSpaceWorker(Heap *heap, uint32_t threadNum) : Worker(heap, threadNum) {}

    ~SemiSpaceWorker() override;
    void PushWorkNodeToGlobal(uint32_t threadId) override;
    void Initialize() override;

    inline void PushWaitUpdateSlot(uint32_t threadId, SlotNeedUpdate slot)
    {
        workList_[threadId].waitUpdate_.emplace_back(slot);
    }

    inline bool GetSlotNeedUpdate(uint32_t threadId, SlotNeedUpdate *slot)
    {
        std::vector<SlotNeedUpdate> &waitUpdate = workList_[threadId].waitUpdate_;
        if (waitUpdate.empty()) {
            return false;
        }
        *slot = waitUpdate.back();
        waitUpdate.pop_back();
        return true;
    }

    NO_COPY_SEMANTIC(SemiSpaceWorker);
    NO_MOVE_SEMANTIC(SemiSpaceWorker);
};

class CompressGCWorker : public Worker {
public:
    CompressGCWorker() = delete;
    explicit CompressGCWorker(Heap *heap, uint32_t threadNum) : Worker(heap, threadNum) {}

    ~CompressGCWorker() override;
    void PushWorkNodeToGlobal(uint32_t threadId) override;
    void Initialize() override;

    NO_COPY_SEMANTIC(CompressGCWorker);
    NO_MOVE_SEMANTIC(CompressGCWorker);
};
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_MEM_SEMI_SPACE_WORKER_H
