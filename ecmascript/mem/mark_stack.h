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

#ifndef PANDA_RUNTIME_ECMASCRIPT_MEM_MARK_STACK_H
#define PANDA_RUNTIME_ECMASCRIPT_MEM_MARK_STACK_H

#include "ecmascript/mem/ecma_list.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/mem/area.h"
#include "ecmascript/mem/region_factory.h"
#include "ecmascript/js_tagged_value.h"

namespace panda {
namespace ecmascript {
class Stack {
public:
    Stack() = default;
    virtual ~Stack() = default;
    NO_COPY_SEMANTIC(Stack);
    NO_MOVE_SEMANTIC(Stack);
    uintptr_t GetBegin() const
    {
        return begin_;
    }

    uintptr_t PopBackChecked()
    {
        if (UNLIKELY(top_ <= reinterpret_cast<uintptr_t *>(begin_))) {
            return 0;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return *--top_;
    }

    void PushBackUnchecked(uintptr_t obj)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *top_++ = obj;
    }

    uintptr_t PopBackUnchecked()
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return *--top_;
    }

    bool PushBackChecked(uintptr_t obj)
    {
        if (UNLIKELY(top_ >= end_)) {
            return false;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        *top_++ = obj;
        return true;
    }

    bool IsEmpty() const
    {
        return top_ == reinterpret_cast<uintptr_t *>(begin_);
    }

    void ResetBegin(uintptr_t begin, uintptr_t end)
    {
        begin_ = begin;
        top_ = reinterpret_cast<uintptr_t *>(begin);
        end_ = reinterpret_cast<uintptr_t *>(end);
    }

    void ResetTop(uintptr_t begin, uintptr_t end)
    {
        begin_ = begin;
        top_ = end_ = reinterpret_cast<uintptr_t *>(end);
    }

private:
    template <class T>
    friend class ContinuousStack;
    friend class WorkNode;
    uintptr_t begin_{0};
    uintptr_t *end_{nullptr};
    uintptr_t *top_{nullptr};
};

template <class T>
class ContinuousStack : public Stack {
public:
    ContinuousStack() = default;
    explicit ContinuousStack(Heap *heap) : heap_(heap) {}
    ~ContinuousStack() override = default;
    NO_COPY_SEMANTIC(ContinuousStack);
    NO_MOVE_SEMANTIC(ContinuousStack);

    inline void BeginMarking(Heap *heap, ContinuousStack<T> *other);
    inline void FinishMarking(ContinuousStack<T> *other);

    T *PopBack()
    {
        if (UNLIKELY(top_ <= reinterpret_cast<uintptr_t *>(begin_))) {
            if (!areaList_.IsEmpty()) {
                unusedList_.AddNode(currentArea_);
                Area *last = areaList_.PopBack();
                currentArea_ = last;
                ResetTop(currentArea_->GetBegin(), currentArea_->GetEnd());
            } else {
                return nullptr;
            }
        }
        return reinterpret_cast<T *>(PopBackUnchecked());
    }

    void PushBack(T *obj)
    {
        if (UNLIKELY(top_ >= end_)) {
            Extend();
        }
        PushBackUnchecked(ToUintPtr(obj));
    }

    inline void TearDown();

private:
    inline void Extend();

    Heap *heap_{nullptr};
    Area *currentArea_{nullptr};
    EcmaList<Area> areaList_{};
    EcmaList<Area> unusedList_{};
};

using MarkStack = ContinuousStack<TaggedObject>;
using ProcessQueue = ContinuousStack<JSTaggedType>;
}  // namespace ecmascript
}  // namespace panda

#endif  // PANDA_RUNTIME_ECMASCRIPT_MEM_MARK_STACK_H
