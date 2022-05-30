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
inline EcmaGlobalStorage::NodeList *EcmaGlobalStorage::NodeList::NodeToNodeList(
    EcmaGlobalStorage::Node *node)
{
    uintptr_t ptr = ToUintPtr(node) - node->GetIndex() * sizeof(EcmaGlobalStorage::Node);
    return reinterpret_cast<NodeList *>(ptr);
}

EcmaGlobalStorage::Node *EcmaGlobalStorage::NodeList::NewNode(JSTaggedType value)
{
    if (IsFull()) {
        return nullptr;
    }
    Node *node = &nodeList_[index_++];
    node->SetPrev(nullptr);
    node->SetNext(usedList_);
    node->SetObject(value);
    node->SetFree(false);

    if (usedList_ != nullptr) {
        usedList_->SetPrev(node);
    }
    usedList_ = node;
    return node;
}

void EcmaGlobalStorage::NodeList::FreeNode(EcmaGlobalStorage::Node *node)
{
    if (node->GetPrev() != nullptr) {
        node->GetPrev()->SetNext(node->GetNext());
    }
    if (node->GetNext() != nullptr) {
        node->GetNext()->SetPrev(node->GetPrev());
    }
    if (node == usedList_) {
        usedList_ = node->GetNext();
    }
    node->SetPrev(nullptr);
    node->SetNext(freeList_);
    node->SetObject(JSTaggedValue::Undefined().GetRawData());
    node->SetFree(true);

    if (freeList_ != nullptr) {
        freeList_->SetPrev(node);
    }
    freeList_ = node;
}

EcmaGlobalStorage::Node *EcmaGlobalStorage::NodeList::GetFreeNode(JSTaggedType value)
{
    Node *node = freeList_;
    if (node != nullptr) {
        freeList_ = node->GetNext();

        node->SetPrev(nullptr);
        node->SetNext(usedList_);
        node->SetObject(value);
        node->SetFree(false);

        if (usedList_ != nullptr) {
            usedList_->SetPrev(node);
        }
        usedList_ = node;
    }
    return node;
}

void EcmaGlobalStorage::NodeList::LinkTo(NodeList *prev)
{
    next_ = nullptr;
    prev_ = prev;
    prev_->next_ = this;
}

void EcmaGlobalStorage::NodeList::RemoveList()
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

uintptr_t EcmaGlobalStorage::NewGlobalHandleImplement(NodeList **storage, NodeList **freeList,
                                                      bool isWeak, JSTaggedType value)
{
    if (!(*storage)->IsFull()) {
        // alloc new block
        Node *node = (*storage)->NewNode(value);
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
    auto block = chunk_->New<NodeList>(isWeak);
    block->LinkTo(*storage);
    *storage = block;

    // use node in block finally
    Node *node = (*storage)->NewNode(value);
    ASSERT(node != nullptr);
    return node->GetObjectAddress();
}

inline uintptr_t EcmaGlobalStorage::NewGlobalHandle(JSTaggedType value)
{
    return NewGlobalHandleImplement(&lastGlobalNodes_, &freeListNodes_, false, value);
}

inline void EcmaGlobalStorage::DisposeGlobalHandle(uintptr_t nodeAddr)
{
    Node *node = reinterpret_cast<Node *>(nodeAddr);
    if (node->IsFree()) {
        return;
    }
    NodeList *list = NodeList::NodeToNodeList(node);
    list->FreeNode(node);

    // If NodeList has no usage node, then delete NodeList
    NodeList **freeList = nullptr;
    NodeList **top = nullptr;
    NodeList **last = nullptr;
    if (list->IsWeak()) {
        freeList = &weakFreeListNodes_;
        top = &topWeakGlobalNodes_;
        last = &lastWeakGlobalNodes_;
    } else {
        freeList = &freeListNodes_;
        top = &topGlobalNodes_;
        last = &lastGlobalNodes_;
    }
    if (!list->HasUsagedNode() && (*top != *last)) {
        list->RemoveList();
        if (*freeList == list) {
            *freeList = list->GetNext();
        }
        if (*top == list) {
            *top = list->GetNext();
        }
        if (*last == list) {
            *last = list->GetPrev();
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

inline uintptr_t EcmaGlobalStorage::SetWeak(uintptr_t nodeAddr)
{
    auto value = reinterpret_cast<Node *>(nodeAddr)->GetObject();
    DisposeGlobalHandle(nodeAddr);
    return NewGlobalHandleImplement(&lastWeakGlobalNodes_, &weakFreeListNodes_, true, value);
}

inline uintptr_t EcmaGlobalStorage::ClearWeak(uintptr_t nodeAddr)
{
    auto value = reinterpret_cast<Node *>(nodeAddr)->GetObject();
    DisposeGlobalHandle(nodeAddr);
    return NewGlobalHandleImplement(&lastGlobalNodes_, &freeListNodes_, false, value);
}

inline bool EcmaGlobalStorage::IsWeak(uintptr_t addr) const
{
    Node *node = reinterpret_cast<Node *>(addr);
    NodeList *list = NodeList::NodeToNodeList(node);
    return list->IsWeak();
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_ECMA_GLOABL_STORAGE_INL_H
