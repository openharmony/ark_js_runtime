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
inline uintptr_t EcmaGlobalStorage::NewGlobalHandleImplement(CVector<std::array<Node, GLOBAL_BLOCK_SIZE> *> *storage,
                                                             Node *freeList, int32_t &count, JSTaggedType value)
{
    if (count == GLOBAL_BLOCK_SIZE && freeList == nullptr) {
        // alloc new block
        auto block = chunk_->New<std::array<Node, GLOBAL_BLOCK_SIZE>>();
        storage->push_back(block);
        count = count - GLOBAL_BLOCK_SIZE;
    }

    // use node in block first
    if (count != GLOBAL_BLOCK_SIZE) {
        storage->back()->at(count).SetNext(nullptr);
        storage->back()->at(count).SetObject(value);
        return storage->back()->at(count++).GetObjectAddress();
    }

    // use free_list node
    Node *node = freeList;
    freeList = freeList->GetNext();
    node->SetNext(nullptr);
    node->SetObject(value);
    return node->GetObjectAddress();
}

inline uintptr_t EcmaGlobalStorage::NewGlobalHandle(JSTaggedType value)
{
    return NewGlobalHandleImplement(globalNodes_, freeList_, count_, value);
}

inline void EcmaGlobalStorage::DisposeGlobalHandle(uintptr_t nodeAddr)
{
    Node *node = reinterpret_cast<Node *>(nodeAddr);
    node->SetObject(JSTaggedValue::Undefined().GetRawData());
    auto freeList = freeList_;
    if (IsWeak(nodeAddr)) {
        freeList = weakFreeList_;
    }
    if (freeList != nullptr) {
        node->SetNext(freeList->GetNext());
        freeList->SetNext(node);
    } else {
        freeList = node;
    }
}

inline uintptr_t EcmaGlobalStorage::SetWeak(uintptr_t nodeAddr)
{
    auto value = reinterpret_cast<Node *>(nodeAddr)->GetObject();
    DisposeGlobalHandle(nodeAddr);
    return NewGlobalHandleImplement(weakGlobalNodes_, weakFreeList_, weakCount_, value);
}

inline bool EcmaGlobalStorage::IsWeak(uintptr_t nodeAddr) const
{
    if (weakGlobalNodes_->empty()) {
        return false;
    }
    size_t pageSize = sizeof(Node) * GLOBAL_BLOCK_SIZE;
    for (size_t i = 0; i < weakGlobalNodes_->size(); i++) {
        auto block = weakGlobalNodes_->at(i);
        if (nodeAddr >= ToUintPtr(block) && nodeAddr < ToUintPtr(block) + pageSize) {
            return true;
        }
    }
    return false;
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_ECMA_GLOABL_STORAGE_INL_H
