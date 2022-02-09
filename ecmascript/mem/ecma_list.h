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

#ifndef ECMASCRIPT_MEM_ECMALIST_H
#define ECMASCRIPT_MEM_ECMALIST_H

#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {
//  Invoking std::list will cause cross invoking, which is time-consuming.
//  Therefore, we implement ecma list inside the vm.

template<class T>
class EcmaList {
public:
    EcmaList() : first_(nullptr), last_(nullptr) {}

    explicit EcmaList(T *node) : first_(node), last_(node)
    {
        node->LinkPrev(nullptr);
        node->LinkNext(nullptr);
    }
    ~EcmaList() = default;
    NO_COPY_SEMANTIC(EcmaList);
    NO_MOVE_SEMANTIC(EcmaList);
    void AddNode(T *node)
    {
        ASSERT(node != nullptr);
        if (LIKELY(first_ != nullptr)) {
            T *lastNext = last_->GetNext();
            node->LinkNext(lastNext);
            node->LinkPrev(last_);
            last_->LinkNext(node);
            if (lastNext) {
                lastNext->LinkPrev(node);
            } else {
                last_ = node;
            }
        } else {
            node->LinkPrev(nullptr);
            node->LinkNext(nullptr);
            first_ = last_ = node;
        }
        length_++;
    }

    void AddNodeToFront(T *node)
    {
        ASSERT(node != nullptr);
        if (LIKELY(last_ != nullptr)) {
            node->LinkNext(first_);
            node->LinkPrev(first_->GetPrev());
            first_->LinkPrev(node);
            first_ = node;
        } else {
            node->LinkPrev(nullptr);
            node->LinkNext(nullptr);
            first_ = last_ = node;
        }
        length_++;
    }

    T *PopBack()
    {
        T *node = last_;
        RemoveNode(last_);
        return node;
    }

    void RemoveNode(T *node)
    {
        ASSERT(HasNode(node));
        if (last_ == node) {
            // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
            last_ = node->GetPrev();
        }
        if (first_ == node) {
            // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
            first_ = node->GetNext();
        }
        // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
        T *next = node->GetNext();
        // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
        T *prev = node->GetPrev();
        if (next != nullptr) {
            next->LinkPrev(prev);
        }
        if (prev != nullptr) {
            prev->LinkNext(next);
        }
        // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
        node->LinkPrev(nullptr);
        // NOLINTNEXTLINE(clang-analyzer-core.CallAndMessage)
        node->LinkNext(nullptr);
        length_--;
    }

    bool HasNode(T *node)
    {
        T *it = first_;
        while (it != nullptr) {
            if (it == node) {
                return true;
            }
            it = it->GetNext();
        }
        return false;
    }

    T *GetFirst() const
    {
        return first_;
    }

    T *GetLast() const
    {
        return last_;
    }

    bool IsEmpty() const
    {
        return last_ == nullptr;
    }

    void Clear()
    {
        first_ = last_ = nullptr;
        length_ = 0;
    }

    uint32_t GetLength() const
    {
        return length_;
    }

private:
    T *first_;
    T *last_;
    uint32_t length_{0};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_ECMALIST_H
