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

#ifndef ECMASCRIPT_MEM_WORK_MANAGER_H
#define ECMASCRIPT_MEM_WORK_MANAGER_H

#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/taskpool/taskpool.h"

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
    UNDEFINED_TASK,
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

    bool PushObject(uintptr_t obj)
    {
        return stack_->PushBackChecked(obj);
    }

    bool PopObject(uintptr_t *obj)
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

class GlobalWorkStack {
public:
    GlobalWorkStack() : top_(nullptr) {}
    ~GlobalWorkStack() = default;

    NO_COPY_SEMANTIC(GlobalWorkStack);
    NO_MOVE_SEMANTIC(GlobalWorkStack);

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
    WorkNode *inNode_ {nullptr};
    WorkNode *outNode_ {nullptr};
    ProcessQueue *weakQueue_ {nullptr};
    std::vector<SlotNeedUpdate> pendingUpdateSlots_;
    TlabAllocator *allocator_ {nullptr};
    size_t aliveSize_ = 0;
    size_t promotedSize_ = 0;
};

class WorkManager final {
public:
    WorkManager() = delete;
    explicit WorkManager(Heap *heap, uint32_t threadNum);
    ~WorkManager();

    void Initialize(TriggerGCType gcType, ParallelGCTaskPhase taskPhase);
    void Finish(size_t &aliveSize);
    void Finish(size_t &aliveSize, size_t &promotedSize);

    bool Push(uint32_t threadId, TaggedObject *object);
    bool Push(uint32_t threadId, TaggedObject *object, Region *region);
    bool Pop(uint32_t threadId, TaggedObject **object);

    bool PopWorkNodeFromGlobal(uint32_t threadId);
    void PushWorkNodeToGlobal(uint32_t threadId, bool postTask = true);

    inline void PushWeakReference(uint32_t threadId, JSTaggedType *weak)
    {
        works_[threadId].weakQueue_->PushBack(weak);
    }

    inline void IncreaseAliveSize(uint32_t threadId, size_t size)
    {
        works_[threadId].aliveSize_ += size;
    }

    inline void IncreasePromotedSize(uint32_t threadId, size_t size)
    {
        works_[threadId].promotedSize_ += size;
    }

    inline ProcessQueue *GetWeakReferenceQueue(uint32_t threadId) const
    {
        return works_[threadId].weakQueue_;
    }

    inline TlabAllocator *GetTlabAllocator(uint32_t threadId) const
    {
        return works_[threadId].allocator_;
    }

    inline void PushSlotNeedUpdate(uint32_t threadId, SlotNeedUpdate slot)
    {
        works_[threadId].pendingUpdateSlots_.emplace_back(slot);
    }

    inline bool GetSlotNeedUpdate(uint32_t threadId, SlotNeedUpdate *slot)
    {
        std::vector<SlotNeedUpdate> &pendingUpdateSlots = works_[threadId].pendingUpdateSlots_;
        if (pendingUpdateSlots.empty()) {
            return false;
        }
        *slot = pendingUpdateSlots.back();
        pendingUpdateSlots.pop_back();
        return true;
    }

private:
    NO_COPY_SEMANTIC(WorkManager);
    NO_MOVE_SEMANTIC(WorkManager);

    WorkNode *AllocateWorkNode();

    Heap *heap_;
    uint32_t threadNum_;
    WorkNodeHolder works_[MAX_TASKPOOL_THREAD_NUM + 1];
    ContinuousStack<JSTaggedType> *continuousQueue_[MAX_TASKPOOL_THREAD_NUM + 1];
    GlobalWorkStack workStack_;
    uintptr_t markSpace_;
    uintptr_t spaceTop_;
    uintptr_t markSpaceEnd_;
    std::vector<uintptr_t> unuseSpace_;
    os::memory::Mutex mtx_;
    ParallelGCTaskPhase parallelGCTaskPhase_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_WORK_MANAGER_H
