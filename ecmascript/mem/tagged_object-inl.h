/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_TAGGED_OBJECT_HEADER_INL_H
#define ECMASCRIPT_TAGGED_OBJECT_HEADER_INL_H

#include "ecmascript/mem/tagged_object.h"

#include <atomic>
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "heap.h"

namespace panda::ecmascript {
inline void TaggedObject::SetClassWithoutBarrier(JSHClass *hclass)
{
    class_ = reinterpret_cast<MarkWordType>(hclass);
}

inline void TaggedObject::SetClass(JSHClass *hclass)
{
    Barriers::SetDynObject<true>(GetJSThread(), this, 0, JSTaggedValue(hclass).GetRawData());
}

inline void TaggedObject::SetClass(JSHandle<JSHClass> hclass)
{
    SetClass(*hclass);
}

inline JSHClass *TaggedObject::GetClass() const
{
    return reinterpret_cast<JSHClass *>(class_);
}

inline void TaggedObject::SynchronizedSetClass(JSHClass *hclass)
{
    reinterpret_cast<std::atomic<MarkWordType> *>(this)->store(reinterpret_cast<MarkWordType>(hclass),
                                                               std::memory_order_release);
}

inline JSHClass *TaggedObject::SynchronizedGetClass() const
{
    return reinterpret_cast<JSHClass *>(
        reinterpret_cast<std::atomic<MarkWordType> *>(ToUintPtr(this))->load(std::memory_order_acquire));
}

inline JSThread *TaggedObject::GetJSThread() const
{
    Region *region = Region::ObjectAddressToRange(const_cast<TaggedObject *>(this));
    ASSERT(region != nullptr);
    return region->GetJSThread();
}
}  //  namespace panda::ecmascript

#endif  // ECMASCRIPT_TAGGED_OBJECT_HEADER_INL_H
