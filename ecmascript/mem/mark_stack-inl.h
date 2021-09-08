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

#ifndef ECMASCRIPT_MEM_MARK_STACK_INL_H
#define ECMASCRIPT_MEM_MARK_STACK_INL_H

#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/heap.h"

namespace panda::ecmascript {
template<class T>
void ContinuousStack<T>::BeginMarking(Heap *heap, ContinuousStack<T> *other)
{
    heap_ = heap;
    currentArea_ = other->currentArea_;
    if (currentArea_ == nullptr) {
        currentArea_ = const_cast<RegionFactory *>(heap_->GetRegionFactory())->AllocateArea(DEFAULT_MARK_STACK_SIZE);
    }
    ResetBegin(currentArea_->GetBegin(), currentArea_->GetEnd());
}

template<class T>
void ContinuousStack<T>::FinishMarking(ContinuousStack<T> *other)
{
    other->currentArea_ = currentArea_;

    while (!unusedList_.IsEmpty()) {
        Area *node = unusedList_.PopBack();
        const_cast<RegionFactory *>(heap_->GetRegionFactory())->FreeArea(node);
    }
}

template<class T>
void ContinuousStack<T>::Extend()
{
    auto area = const_cast<RegionFactory *>(heap_->GetRegionFactory())->AllocateArea(DEFAULT_MARK_STACK_SIZE);
    areaList_.AddNode(currentArea_);
    currentArea_ = area;
    ResetBegin(currentArea_->GetBegin(), currentArea_->GetEnd());
}

template<class T>
void ContinuousStack<T>::Destroy()
{
    if (currentArea_ != nullptr) {
        const_cast<RegionFactory *>(heap_->GetRegionFactory())->FreeArea(currentArea_);
        currentArea_ = nullptr;
    }
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_MARK_STACK_INL_H
