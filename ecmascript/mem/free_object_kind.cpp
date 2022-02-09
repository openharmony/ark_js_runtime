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

#include "ecmascript/mem/free_object_kind.h"

#include "ecmascript/free_object.h"
#include "ecmascript/mem/free_object_list.h"

namespace panda::ecmascript {
void FreeObjectKind::Free(uintptr_t begin, size_t size)
{
    auto freeObject = FreeObject::Cast(begin);
    ASSERT(freeObject->IsFreeObject());
    freeObject->SetNext(freeObject_);
    freeObject_ = freeObject;
    available_ += size;
}

void FreeObjectKind::Rebuild()
{
    freeObject_ = nullptr;
    available_ = 0;
    isAdded_ = false;
    next_ = nullptr;
    prev_ = nullptr;
}

FreeObject *FreeObjectKind::ObtainSmallFreeObject(size_t size)
{
    FreeObject *curFreeObject = nullptr;
    if (freeObject_ != nullptr && freeObject_->Available() >= size) {
        curFreeObject = freeObject_;
        freeObject_ = freeObject_->GetNext();
        curFreeObject->SetNext(nullptr);
        available_ -= curFreeObject->Available();
    }
    return curFreeObject;
}

FreeObject *FreeObjectKind::ObtainLargeFreeObject(size_t size)
{
    FreeObject *prevFreeObject = freeObject_;
    FreeObject *curFreeObject = freeObject_;
    while (curFreeObject != nullptr) {
        if (curFreeObject->Available() >= size) {
            if (curFreeObject == freeObject_) {
                freeObject_ = curFreeObject->GetNext();
            } else {
                prevFreeObject->SetNext(curFreeObject->GetNext());
            }
            curFreeObject->SetNext(nullptr);
            available_ -= curFreeObject->Available();
            return curFreeObject;
        }
        prevFreeObject = curFreeObject;
        curFreeObject = curFreeObject->GetNext();
    }
    return nullptr;
}

FreeObject *FreeObjectKind::LookupSmallFreeObject(size_t size)
{
    if (freeObject_ != nullptr && freeObject_->Available() >= size) {
        return freeObject_;
    }
    return nullptr;
}

FreeObject *FreeObjectKind::LookupLargeFreeObject(size_t size)
{
    if (available_ < size) {
        return nullptr;
    }
    FreeObject *curFreeObject = freeObject_;
    while (curFreeObject != nullptr) {
        if (curFreeObject->Available() >= size) {
            return curFreeObject;
        }
        curFreeObject = curFreeObject->GetNext();
    }
    return nullptr;
}
}  // namespace panda::ecmascript
