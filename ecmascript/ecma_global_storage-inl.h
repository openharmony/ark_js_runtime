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
#ifndef ECMASCRIPT_ECMA_GLOABL_STORAGE_INL_H
#define ECMASCRIPT_ECMA_GLOABL_STORAGE_INL_H

#include "ecmascript/ecma_global_storage.h"
#include "ecmascript/mem/chunk.h"

namespace panda::ecmascript {
template<typename T>
inline EcmaGlobalStorage::NodeList<T> *EcmaGlobalStorage::NodeList<T>::NodeToNodeList(T *node)
{
    uintptr_t ptr = ToUintPtr(node) - node->GetIndex() * sizeof(T);
    return reinterpret_cast<NodeList<T> *>(ptr);
}

template<typename T>
T *EcmaGlobalStorage::NodeList<T>::NewNode(JSTaggedType value)
{
    if (IsFull()) {
        return nullptr;
    }
    T *node = &nodeList_[index_++];
    node->SetPrev(nullptr);
    node->SetNext(usedList_);
    node->SetObject(value);
    node->SetUsing(true);
    if (usedList_ != nullptr) {
        usedList_->SetPrev(node);
    }
    usedList_ = node;
    return node;
}

template<typename T>
void EcmaGlobalStorage::NodeList<T>::FreeNode(T *node)
{
    if (node->GetPrev() != nullptr) {
        node->GetPrev()->SetNext(node->GetNext());
    }
    if (node->GetNext() != nullptr) {
        node->GetNext()->SetPrev(node->GetPrev());
    }
    if (node == usedList_) {
        usedList_ = reinterpret_cast<T *>(node->GetNext());
    }
    node->SetPrev(nullptr);
    node->SetNext(freeList_);
    node->SetObject(JSTaggedValue::Undefined().GetRawData());
    node->SetUsing(false);
    if (node->IsWeak()) {
        reinterpret_cast<WeakNode *>(node)->SetReference(nullptr);
        reinterpret_cast<WeakNode *>(node)->SetCallback(nullptr);
    }
    if (freeList_ != nullptr) {
        freeList_->SetPrev(node);
    }
    freeList_ = node;
}

template<typename T>
T *EcmaGlobalStorage::NodeList<T>::GetFreeNode(JSTaggedType value)
{
    T *node = freeList_;
    if (node != nullptr) {
        freeList_ = reinterpret_cast<T *>(node->GetNext());

        node->SetPrev(nullptr);
        node->SetNext(usedList_);
        node->SetObject(value);
        node->SetUsing(true);
        if (usedList_ != nullptr) {
            usedList_->SetPrev(node);
        }
        usedList_ = node;
    }
    return node;
}

template<typename T>
void EcmaGlobalStorage::NodeList<T>::LinkTo(NodeList<T> *prev)
{
    next_ = nullptr;
    prev_ = prev;
    prev_->next_ = this;
}

template<typename T>
void EcmaGlobalStorage::NodeList<T>::RemoveList()
{
    if (next_ != nullptr) {
        next_->SetPrev(prev_);
    }
    if (prev_ != nullptr) {
        prev_->SetNext(next_);
    }
    if (freeNext_ != nullptr) {
        freeNext_->SetFreePrev(freePrev_);
    }
    if (freePrev_ != nullptr) {
        freePrev_->SetFreeNext(freeNext_);
    }
}

template<typename T>
uintptr_t EcmaGlobalStorage::NewGlobalHandleImplement(NodeList<T> **storage, NodeList<T> **freeList, JSTaggedType value)
{
#if ECMASCRIPT_ENABLE_NEW_HANDLE_CHECK
    thread_->CheckJSTaggedType(value);
#endif
    if (!(*storage)->IsFull()) {
        // alloc new block
        T *node = (*storage)->NewNode(value);
        ASSERT(node != nullptr);
        return node->GetObjectAddress();
    }
    if (*freeList != nullptr) {
        // use free_list node
        Node *node = (*freeList)->GetFreeNode(value);
        ASSERT(node != nullptr);
        if (!(*freeList)->HasFreeNode()) {
            auto next = (*freeList)->GetFreeNext();
            (*freeList)->SetFreeNext(nullptr);
            (*freeList)->SetFreePrev(nullptr);
            if (next != nullptr) {
                next->SetFreePrev(nullptr);
            }
            *freeList = next;
        }
        return node->GetObjectAddress();
    }
    auto block = chunk_->New<NodeList<T>>();
    block->LinkTo(*storage);
    *storage = block;

    // use node in block finally
    T *node = (*storage)->NewNode(value);
    ASSERT(node != nullptr);
    return node->GetObjectAddress();
}

inline uintptr_t EcmaGlobalStorage::NewGlobalHandle(JSTaggedType value)
{
    uintptr_t ret = NewGlobalHandleImplement(&lastGlobalNodes_, &freeListNodes_, value);
    return ret;
}

template<typename T>
inline void EcmaGlobalStorage::DisposeGlobalHandle(T *node, NodeList<T> **freeList, NodeList<T> **topNodes,
                                                   NodeList<T> **lastNodes)
{
    NodeList<T> *list = NodeList<T>::NodeToNodeList(node);
    list->FreeNode(node);

    // If NodeList has no usage node, then delete NodeList
    if (!list->HasUsagedNode() && (*topNodes != *lastNodes)) {
        list->RemoveList();
        if (*freeList == list) {
            *freeList = list->GetNext();
        }
        if (*topNodes == list) {
            *topNodes = list->GetNext();
        }
        if (*lastNodes == list) {
            *lastNodes = list->GetPrev();
        }
        chunk_->Delete(list);
    } else {
        // Add to freeList
        if (list != *freeList && list->GetFreeNext() == nullptr && list->GetFreePrev() == nullptr) {
            list->SetFreeNext(*freeList);
            if (*freeList != nullptr) {
                (*freeList)->SetFreePrev(list);
            }
            *freeList = list;
        }
    }
}

inline void EcmaGlobalStorage::DisposeGlobalHandle(uintptr_t nodeAddr)
{
    Node *node = reinterpret_cast<Node *>(nodeAddr);
    if (!node->IsUsing()) {
        return;
    }
    if (node->IsWeak()) {
        DisposeGlobalHandle(reinterpret_cast<WeakNode *>(node), &weakFreeListNodes_, &topWeakGlobalNodes_,
                            &lastWeakGlobalNodes_);
    } else {
        DisposeGlobalHandle(node, &freeListNodes_, &topGlobalNodes_, &lastGlobalNodes_);
    }
}

inline uintptr_t EcmaGlobalStorage::SetWeak(uintptr_t nodeAddr, void *ref, WeakClearCallback callback)
{
    auto value = reinterpret_cast<Node *>(nodeAddr)->GetObject();
    DisposeGlobalHandle(nodeAddr);
    uintptr_t addr = NewGlobalHandleImplement<WeakNode>(&lastWeakGlobalNodes_, &weakFreeListNodes_, value);
    WeakNode *node = reinterpret_cast<WeakNode *>(addr);
    node->SetReference(ref);
    node->SetCallback(callback);
    return addr;
}

inline uintptr_t EcmaGlobalStorage::ClearWeak(uintptr_t nodeAddr)
{
    auto value = reinterpret_cast<Node *>(nodeAddr)->GetObject();
    DisposeGlobalHandle(nodeAddr);
    uintptr_t ret = NewGlobalHandleImplement<Node>(&lastGlobalNodes_, &freeListNodes_, value);
    return ret;
}

inline bool EcmaGlobalStorage::IsWeak(uintptr_t addr) const
{
    Node *node = reinterpret_cast<Node *>(addr);
    return node->IsWeak();
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_ECMA_GLOABL_STORAGE_INL_H
