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

#ifndef ECMASCRIPT_JSNATIVEPOINTER_H
#define ECMASCRIPT_JSNATIVEPOINTER_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/mem/tagged_object.h"

namespace panda::ecmascript {
using DeleteEntryPoint = void (*)(void *, void *);

// Used for the requirement of ACE that wants to associated a registered C++ resource with a JSObject.
class JSNativePointer : public TaggedObject {
public:
    static JSNativePointer *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsJSNativePointer());
        return reinterpret_cast<JSNativePointer *>(object);
    }

    inline void ResetExternalPointer(void *externalPointer)
    {
        DeleteExternalPointer();
        SetExternalPointer(externalPointer);
    }

    inline void Destroy()
    {
        DeleteExternalPointer();
        SetExternalPointer(nullptr);
        SetDeleter(nullptr);
        SetData(nullptr);
    }

    static constexpr size_t POINTER_OFFSET = TaggedObjectSize();
    ACCESSORS_NATIVE_FIELD(ExternalPointer, void, POINTER_OFFSET, DELETER_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(Deleter, DeleteEntryPoint, DELETER_OFFSET, DATA_OFFSET)
    ACCESSORS_NATIVE_FIELD(Data, void, DATA_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

private:
    inline void DeleteExternalPointer()
    {
        void *externalPointer = GetExternalPointer();
        DeleteEntryPoint deleter = GetDeleter();
        if (deleter != nullptr) {
            deleter(externalPointer, GetData());
        }
    }
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JSNATIVEPOINTER_H
