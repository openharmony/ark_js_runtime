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

#ifndef ECMASCRIPT_ECMA_GLOABL_STORAGE_H
#define ECMASCRIPT_ECMA_GLOABL_STORAGE_H

#include "ecmascript/js_tagged_value.h"

#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/chunk.h"

namespace panda::ecmascript {
class EcmaGlobalStorage {
public:
    static const int32_t GLOBAL_BLOCK_SIZE = 256;

    explicit EcmaGlobalStorage(Chunk *chunk) : chunk_(chunk)
    {
        ASSERT(chunk != nullptr);
        topGlobalNodes_ = lastGlobalNodes_ = chunk_->New<NodeList>(false);
        topWeakGlobalNodes_ = lastWeakGlobalNodes_ = chunk_->New<NodeList>(true);
    }

    ~EcmaGlobalStorage()
    {
        NodeList *next = topGlobalNodes_;
        NodeList *current = nullptr;
        while (next != nullptr) {
            current = next;
            next = current->GetNext();
            chunk_->Delete(current);
        }

        next = topWeakGlobalNodes_;
        while (next != nullptr) {
            current = next;
            next = current->GetNext();
            chunk_->Delete(current);
        }
    }

    class Node {
    public:
        JSTaggedType GetObject() const
        {
            return obj_;
        }

        void SetObject(JSTaggedType obj)
        {
            obj_ = obj;
        }

        Node *GetNext() const
        {
            return next_;
        }

        void SetNext(Node *node)
        {
            next_ = node;
        }

        Node *GetPrev() const
        {
            return prev_;
        }

        void SetPrev(Node *node)
        {
            prev_ = node;
        }

        int32_t GetIndex()
        {
            return index_;
        }

        void SetIndex(int32_t index)
        {
            index_ = index;
        }

        void SetFree(bool free)
        {
            isFree_ = free;
        }

        bool IsFree() const
        {
            return isFree_;
        }

        uintptr_t GetObjectAddress() const
        {
            return reinterpret_cast<uintptr_t>(&obj_);
        }

    private:
        JSTaggedType obj_;
        Node *next_ {nullptr};
        Node *prev_ {nullptr};
        int32_t index_ {-1};
        bool isFree_ {false};
    };

    class NodeList {
    public:
        explicit NodeList(bool isWeak) : isWeak_(isWeak)
        {
            for (int i = 0; i < GLOBAL_BLOCK_SIZE; i++) {
                nodeList_[i].SetIndex(i);
            }
        }
        ~NodeList() = default;

        inline static NodeList *NodeToNodeList(Node *node);

        inline Node *NewNode(JSTaggedType value);
        inline Node *GetFreeNode(JSTaggedType value);
        inline void FreeNode(Node *node);

        inline void LinkTo(NodeList *prev);
        inline void RemoveList();

        inline bool IsFull()
        {
            return index_ >= GLOBAL_BLOCK_SIZE;
        }

        inline bool IsWeak()
        {
            return isWeak_;
        }

        inline bool HasFreeNode()
        {
            return freeList_ != nullptr;
        }

        inline bool HasUsagedNode()
        {
            return !IsFull() || usedList_ != nullptr;
        }

        inline void SetNext(NodeList *next)
        {
            next_ = next;
        }
        inline NodeList *GetNext() const
        {
            return next_;
        }

        inline void SetPrev(NodeList *prev)
        {
            prev_ = prev;
        }
        inline NodeList *GetPrev() const
        {
            return prev_;
        }

        inline void SetFreeNext(NodeList *next)
        {
            freeNext_ = next;
        }
        inline NodeList *GetFreeNext() const
        {
            return freeNext_;
        }

        inline void SetFreePrev(NodeList *prev)
        {
            freePrev_ = prev;
        }
        inline NodeList *GetFreePrev() const
        {
            return freePrev_;
        }

        template<class Callback>
        inline void IterateUsageGlobal(Callback callback)
        {
            Node *next = usedList_;
            Node *current = nullptr;
            while (next != nullptr) {
                current = next;
                next = current->GetNext();
                ASSERT(current != next);
                callback(current);
            }
        }

    private:
        Node nodeList_[GLOBAL_BLOCK_SIZE];  // all
        Node *freeList_ {nullptr};  // dispose node
        Node *usedList_ {nullptr};  // usage node
        int32_t index_ {0};
        bool isWeak_ {false};
        NodeList *next_ {nullptr};
        NodeList *prev_ {nullptr};
        NodeList *freeNext_ {nullptr};
        NodeList *freePrev_ {nullptr};
    };

    inline uintptr_t NewGlobalHandle(JSTaggedType value);
    inline void DisposeGlobalHandle(uintptr_t addr);
    inline uintptr_t SetWeak(uintptr_t addr);
    inline uintptr_t ClearWeak(uintptr_t addr);
    inline bool IsWeak(uintptr_t addr) const;

    template<class Callback>
    void IterateUsageGlobal(Callback callback)
    {
        NodeList *next = topGlobalNodes_;
        NodeList *current = nullptr;
        while (next != nullptr) {
            current = next;
            next = current->GetNext();
            ASSERT(current != next);
            current->IterateUsageGlobal(callback);
        }
    }

    template<class Callback>
    void IterateWeakUsageGlobal(Callback callback)
    {
        NodeList *next = topWeakGlobalNodes_;
        NodeList *current = nullptr;
        while (next != nullptr) {
            current = next;
            next = current->GetNext();
            ASSERT(current != next);
            current->IterateUsageGlobal(callback);
        }
    }

private:
    NO_COPY_SEMANTIC(EcmaGlobalStorage);
    NO_MOVE_SEMANTIC(EcmaGlobalStorage);

    inline uintptr_t NewGlobalHandleImplement(NodeList **storage, NodeList **freeList, bool isWeak, JSTaggedType value);

    Chunk *chunk_ {nullptr};
    NodeList *topGlobalNodes_ {nullptr};
    NodeList *lastGlobalNodes_ {nullptr};
    NodeList *freeListNodes_ {nullptr};

    NodeList *topWeakGlobalNodes_ {nullptr};
    NodeList *lastWeakGlobalNodes_ {nullptr};
    NodeList *weakFreeListNodes_ {nullptr};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_ECMA_GLOABL_STORAGE_H
