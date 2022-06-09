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

#ifndef ECMASCRIPT_ECMA_GLOBAL_STORAGE_H
#define ECMASCRIPT_ECMA_GLOBAL_STORAGE_H

#include "ecmascript/js_tagged_value.h"

#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/chunk.h"

namespace panda::ecmascript {
class EcmaGlobalStorage {
public:
    static const int32_t GLOBAL_BLOCK_SIZE = 256;
    using WeakClearCallback = void (*)(void *);

    explicit EcmaGlobalStorage(JSThread *thread, Chunk *chunk) : thread_(thread), chunk_(chunk)
    {
        ASSERT(chunk != nullptr);
        topGlobalNodes_ = lastGlobalNodes_ = chunk_->New<NodeList<Node>>();
        topWeakGlobalNodes_ = lastWeakGlobalNodes_ = chunk_->New<NodeList<WeakNode>>();
    }

    ~EcmaGlobalStorage()
    {
        auto *next = topGlobalNodes_;
        NodeList<Node> *current = nullptr;
        while (next != nullptr) {
            current = next;
            next = current->GetNext();
            current->IterateUsageGlobal([] (Node *node) {
                node->SetUsing(false);
                node->SetObject(JSTaggedValue::Undefined().GetRawData());
            });
            chunk_->Delete(current);
        }

        auto *weakNext = topWeakGlobalNodes_;
        NodeList<WeakNode> *weakCurrent = nullptr;
        while (weakNext != nullptr) {
            weakCurrent = weakNext;
            weakNext = weakCurrent->GetNext();
            weakCurrent->IterateUsageGlobal([] (Node *node) {
                node->SetUsing(false);
                node->SetObject(JSTaggedValue::Undefined().GetRawData());
                reinterpret_cast<WeakNode *>(node)->CallWeakCallback();
            });
            chunk_->Delete(weakCurrent);
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

        void SetUsing(bool free)
        {
            isUsing_ = free;
        }

        void SetWeak(bool isWeak)
        {
            isWeak_ = isWeak;
        }

        bool IsUsing() const
        {
            return isUsing_;
        }

        bool IsWeak() const
        {
            return isWeak_;
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
        bool isUsing_ {false};
        bool isWeak_ {false};
    };

    class WeakNode : public Node {
    public:
        void SetReference(void *ref)
        {
            reference_ = ref;
        }

        void SetCallback(WeakClearCallback callback)
        {
            callback_ = callback;
        }

        void CallWeakCallback()
        {
            if (callback_ != nullptr) {
                callback_(reference_);
            }
        }
    private:
        void *reference_ {nullptr};
        WeakClearCallback callback_ {nullptr};
    };

    template<typename T>
    class NodeList {
    public:
        explicit NodeList()
        {
            bool isWeak = std::is_same<T, EcmaGlobalStorage::WeakNode>::value;
            for (int i = 0; i < GLOBAL_BLOCK_SIZE; i++) {
                nodeList_[i].SetIndex(i);
                nodeList_[i].SetWeak(isWeak);
            }
        }
        ~NodeList() = default;

        inline static NodeList<T> *NodeToNodeList(T *node);

        inline T *NewNode(JSTaggedType value);
        inline T *GetFreeNode(JSTaggedType value);

        inline void FreeNode(T *node);

        inline void LinkTo(NodeList<T> *prev);
        inline void RemoveList();

        inline bool IsFull()
        {
            return index_ >= GLOBAL_BLOCK_SIZE;
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
        inline NodeList<T> *GetNext() const
        {
            return next_;
        }

        inline void SetPrev(NodeList<T> *prev)
        {
            prev_ = prev;
        }
        inline NodeList<T> *GetPrev() const
        {
            return prev_;
        }

        inline void SetFreeNext(NodeList<T> *next)
        {
            freeNext_ = next;
        }
        inline NodeList<T> *GetFreeNext() const
        {
            return freeNext_;
        }

        inline void SetFreePrev(NodeList<T> *prev)
        {
            freePrev_ = prev;
        }
        inline NodeList<T> *GetFreePrev() const
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
        T nodeList_[GLOBAL_BLOCK_SIZE];  // all
        T *freeList_ {nullptr};  // dispose node
        T *usedList_ {nullptr};  // usage node
        int32_t index_ {0};
        NodeList<T> *next_ {nullptr};
        NodeList<T> *prev_ {nullptr};
        NodeList<T> *freeNext_ {nullptr};
        NodeList<T> *freePrev_ {nullptr};
    };

    inline uintptr_t NewGlobalHandle(JSTaggedType value);
    inline void DisposeGlobalHandle(uintptr_t addr);
    inline uintptr_t SetWeak(uintptr_t addr, void *ref = nullptr, WeakClearCallback callback = nullptr);
    inline uintptr_t ClearWeak(uintptr_t addr);
    inline bool IsWeak(uintptr_t addr) const;

    template<class Callback>
    void IterateUsageGlobal(Callback callback)
    {
        NodeList<Node> *next = topGlobalNodes_;
        NodeList<Node> *current = nullptr;
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
        NodeList<WeakNode> *next = topWeakGlobalNodes_;
        NodeList<WeakNode> *current = nullptr;
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

    template<typename T>
    inline void DisposeGlobalHandle(T *node, NodeList<T> **freeLis, NodeList<T> **topNodes,
                                    NodeList<T> **lastNodes);
    template<typename T>
    inline uintptr_t NewGlobalHandleImplement(NodeList<T> **storage, NodeList<T> **freeList, JSTaggedType value);

    [[maybe_unused]] JSThread *thread_ {nullptr};
    Chunk *chunk_ {nullptr};
    NodeList<Node> *topGlobalNodes_ {nullptr};
    NodeList<Node> *lastGlobalNodes_ {nullptr};
    NodeList<Node> *freeListNodes_ {nullptr};

    NodeList<WeakNode> *topWeakGlobalNodes_ {nullptr};
    NodeList<WeakNode> *lastWeakGlobalNodes_ {nullptr};
    NodeList<WeakNode> *weakFreeListNodes_ {nullptr};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_ECMA_GLOBAL_STORAGE_H
