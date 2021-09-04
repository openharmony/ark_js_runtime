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

#ifndef PANDA_RUNTIME_ECMASCRIPT_TAGGED_OBJECT_HEADER_H
#define PANDA_RUNTIME_ECMASCRIPT_TAGGED_OBJECT_HEADER_H

#include "ecmascript/mem/mark_word.h"
#include "include/object_header.h"

namespace panda::ecmascript {
class JSHClass;
template <typename T>
class JSHandle;

class TaggedObject : public ObjectHeader {
public:
    static TaggedObject *Cast(ObjectHeader *header)
    {
        return static_cast<TaggedObject *>(header);
    }

    void SetClass(JSHandle<JSHClass> hclass);

    void SynchronizedSetClass(JSHClass *hclass);
    JSHClass *SynchronizedGetClass() const;
    void SetClass(JSHClass *hclass);
    JSHClass *GetClass() const;

    size_t GetObjectSize();

    // Size of object header
    static constexpr int ObjectHeaderSize()
    {
        return sizeof(TaggedObject);
    }
};
static_assert(TaggedObject::ObjectHeaderSize() == sizeof(MarkWordType));
}  //  namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_TAGGED_OBJECT_HEADER_H
