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

#ifndef ECMASCRIPT_MEM_PARALLEL_WORK_HELPER_H
#define ECMASCRIPT_MEM_PARALLEL_WORK_HELPER_H

#include "ecmascript/mem/mark_stack-inl.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/platform/platform.h"

namespace panda::ecmascript {
using SlotNeedUpdate = std::pair<TaggedObject *, ObjectSlot>;

static constexpr uint32_t MARKSTACK_MAX_SIZE = 100;
static constexpr uint32_t STACK_AREA_SIZE = sizeof(uintptr_t) * MARKSTACK_MAX_SIZE;
static constexpr uint32_t SPACE_SIZE = 8 * 1024;

class Heap;
class Stack;
class SemiSpaceCollector;
class TlabAllocator;
class Region;

enum ParallelGCTaskPhase {
    SEMI_HANDLE_THREAD_ROOTS_TASK,
    SEMI_HANDLE_SNAPSHOT_TASK,
    SEMI_HANDLE_GLOBAL_POOL_TASK,
    OLD_HANDLE_GLOBAL_POOL_TASK,
    COMPRESS_HANDLE_GLOBAL_POOL_TASK,
    CONCURRENT_HANDLE_GLOBAL_POOL_TASK,
    CONCURRENT_HANDLE_OLD_TO_NEW_TASK,
    TASK_LAST  // Count of different Task phase
};

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
    WorkNode *top_ {nullptr};
    os::memory::Mutex mtx_;
};

struct WorkNodeHolder {
    WorkNode *pushNode_ {nullptr};
    WorkNode *popNode_ {nullptr};
    ProcessQueue *weakQueue_ {nullptr};
    std::vector<SlotNeedUpdate> waitUpdate_;
    TlabAllocator *allocator_ {nullptr};
    size_t aliveSize_ = 0;
    size_t promoteSize_ = 0;
};

class WorkerHelper final {
public:
    WorkerHelper() = delete;
    explicit WorkerHelper(Heap *heap, uint32_t threadNum);
    ~WorkerHelper();

    void Initialize(TriggerGCType gcType, ParallelGCTaskPhase parallelTask);
    void Finish(size_t &aliveSize);
    void Finish(size_t &aliveSize, size_t &promoteSize);

    bool Push(uint32_t threadId, TaggedObject *object);
    bool Push(uint32_t threadId, TaggedObject *object, Region *region);
    bool Pop(uint32_t threadId, TaggedObject **object);

    bool PopWorkNodeFromGlobal(uint32_t threadId);
    void PushWorkNodeToGlobal(uint32_t threadId, bool postTask = true);

    inline void PushWeakReference(uint32_t threadId, JSTaggedType *weak)
    {
        workList_[threadId].weakQueue_->PushBack(weak);
    }

    inline void AddAliveSize(uint32_t threadId, size_t size)
    {
        workList_[threadId].aliveSize_ += size;
    }

    inline void AddPromoteSize(uint32_t threadId, size_t size)
    {
        workList_[threadId].promoteSize_ += size;
    }

    inline ProcessQueue *GetWeakReferenceQueue(uint32_t threadId) const
    {
        return workList_[threadId].weakQueue_;
    }

    inline TlabAllocator *GetTlabAllocator(uint32_t threadId) const
    {
        return workList_[threadId].allocator_;
    }

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

private:
    NO_COPY_SEMANTIC(WorkerHelper);
    NO_MOVE_SEMANTIC(WorkerHelper);

    WorkNode *AllocalWorkNode();

    Heap *heap_;
    uint32_t threadNum_;
    WorkNodeHolder workList_[MAX_PLATFORM_THREAD_NUM + 1];
    ContinuousStack<JSTaggedType> *continuousQueue_[MAX_PLATFORM_THREAD_NUM + 1];
    GlobalWorkList globalWork_;
    uintptr_t markSpace_;
    uintptr_t spaceTop_;
    uintptr_t markSpaceEnd_;
    std::vector<uintptr_t> unuseSpace_;
    os::memory::Mutex mtx_;
    ParallelGCTaskPhase parallelTask_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_PARALLEL_WORK_HELPER_H
