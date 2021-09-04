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

#include "ecmascript/mem/free_object_list.h"

#include "ecmascript/free_object.h"
#include "ecmascript/mem/free_object_list-inl.h"
#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {
FreeObjectList::FreeObjectList()
{
    kinds_ = Span<FreeObjectKind *>(new FreeObjectKind *[NUMBER_OF_KINDS](), NUMBER_OF_KINDS);
    noneEmptyKindBitMap_ = 0;
}

FreeObjectList::~FreeObjectList()
{
    for (auto it : kinds_) {
        delete it;
    }
    delete[] kinds_.data();
    noneEmptyKindBitMap_ = 0;
}

FreeObject *FreeObjectList::Allocator(size_t size)
{
    if (noneEmptyKindBitMap_ == 0) {
        return nullptr;
    }
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    // find from suitable
    KindType type = SelectKindType(size);
    if (type == FreeObjectKind::INVALID_KIND_TYPE) {
        return nullptr;
    }

    KindType lastType = type - 1;
    for (type = CalcNextNoneEmptyIndex(type); type > lastType && type < NUMBER_OF_KINDS;
         type = CalcNextNoneEmptyIndex(type + 1)) {
        lastType = type;
        FreeObjectKind *top = kinds_[type];
        if (top == nullptr || top->Available() < size) {
            continue;
        }
        FreeObject *current = nullptr;
        if (type <= SMALL_KIND_MAX_INDEX) {
            current = top->SearchSmallFreeObject(size);
        } else {
            current = top->SearchLargeFreeObject(size);
        }
        if (top->Empty()) {
            RemoveKind(top);
        }
        if (current != nullptr) {
            size_t currentSize = current->Available();
            available_ -= currentSize;
            if (currentSize >= size) {
                return current;
            }
        }
    }
    return nullptr;
}

void FreeObjectList::Free(uintptr_t start, size_t size)
{
    if (start == 0 || size == 0) {
        return;
    }
    size = AlignUp(size, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
    KindType type = SelectKindType(size);
    if (type == FreeObjectKind::INVALID_KIND_TYPE) {
        return;
    }

    auto kind = kinds_[type];
    if (kind == nullptr) {
        kind = new FreeObjectKind(type, start, size);
        if (!AddKind(kind)) {
            delete kind;
            return;
        }
    } else {
        kind->Free(start, size);
    }
    available_ += size;
}

void FreeObjectList::Rebuild()
{
    for (auto kind : kinds_) {
        if (kind != nullptr) {
            kind->Rebuild();
            RemoveKind(kind);
        }
    }
    available_ = 0;
    noneEmptyKindBitMap_ = 0;
}

size_t FreeObjectList::GetFreeObjectSize() const
{
    return available_;
}

bool FreeObjectList::AddKind(FreeObjectKind *kind)
{
    if (kind == nullptr || kind->Empty()) {
        return false;
    }
    KindType type = kind->kindType_;
    FreeObjectKind *top = kinds_[type];
    if (kind == top) {
        return true;
    }
    if (top != nullptr) {
        top->prev_ = kind;
    }
    kind->next_ = top;
    kinds_[type] = kind;
    SetNoneEmptyBit(type);
    return true;
}

void FreeObjectList::RemoveKind(FreeObjectKind *kind)
{
    if (kind == nullptr) {
        return;
    }
    KindType type = kind->kindType_;
    FreeObjectKind *top = kinds_[type];
    if (top == kind) {
        kinds_[type] = top->next_;
    }
    if (kind->prev_ != nullptr) {
        kind->prev_->next_ = kind->next_;
    }
    if (kind->next_ != nullptr) {
        kind->next_->prev_ = kind->prev_;
    }
    kind->prev_ = nullptr;
    kind->next_ = nullptr;
    if (kinds_[type] == nullptr) {
        ClearNoneEmptyBit(type);
    }
    delete kind;
}
}  // namespace panda::ecmascript
