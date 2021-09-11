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
        globalNodes_ = chunk->New<CVector<std::array<Node, GLOBAL_BLOCK_SIZE> *>>();
        weakGlobalNodes_ = chunk->New<CVector<std::array<Node, GLOBAL_BLOCK_SIZE> *>>();
    };

    ~EcmaGlobalStorage()
    {
        for (auto block : *globalNodes_) {
            chunk_->Delete(block);
        }
        chunk_->Delete(globalNodes_);

        for (auto block : *weakGlobalNodes_) {
            chunk_->Delete(block);
        }
        chunk_->Delete(weakGlobalNodes_);
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

        uintptr_t GetObjectAddress() const
        {
            return reinterpret_cast<uintptr_t>(&obj_);
        }

    private:
        JSTaggedType obj_;
        Node *next_;
    };

    inline uintptr_t NewGlobalHandle(JSTaggedType value);
    inline void DisposeGlobalHandle(uintptr_t addr);
    inline uintptr_t SetWeak(uintptr_t addr);
    inline bool IsWeak(uintptr_t addr) const;

    inline CVector<std::array<Node, GLOBAL_BLOCK_SIZE> *> *GetNodes() const
    {
        return globalNodes_;
    }

    inline CVector<std::array<Node, GLOBAL_BLOCK_SIZE> *> *GetWeakNodes() const
    {
        return weakGlobalNodes_;
    }

    inline int32_t GetCount() const
    {
        return count_;
    }

    inline int32_t GetWeakCount() const
    {
        return weakCount_;
    }

private:
    NO_COPY_SEMANTIC(EcmaGlobalStorage);
    NO_MOVE_SEMANTIC(EcmaGlobalStorage);

    inline uintptr_t NewGlobalHandleImplement(CVector<std::array<Node, GLOBAL_BLOCK_SIZE> *> *storage, Node *freeList,
                                              int32_t &count, JSTaggedType value);

    Chunk *chunk_ {nullptr};
    CVector<std::array<Node, GLOBAL_BLOCK_SIZE> *> *globalNodes_ {nullptr};
    int32_t count_ {GLOBAL_BLOCK_SIZE};
    Node *freeList_ {nullptr};
    CVector<std::array<Node, GLOBAL_BLOCK_SIZE> *> *weakGlobalNodes_ {nullptr};
    int32_t weakCount_ {GLOBAL_BLOCK_SIZE};
    Node *weakFreeList_ {nullptr};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_ECMA_GLOABL_STORAGE_H
