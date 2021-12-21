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

#include "include/coretypes/native_pointer.h"

namespace panda::ecmascript {
using DeleteEntryPoint = void (*)(void *, void *);

class JSNativePointer : public coretypes::NativePointer {
public:
    static JSNativePointer *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsJSNativePointer());
        return reinterpret_cast<JSNativePointer *>(object);
    }

    inline void ResetExternalPointer(void *externalPointer)
    {
        ClearExternalPointer();
        SetExternalPointer(externalPointer);
    }

    inline void ClearExternalPointer()
    {
        if (GetExternalPointer() == nullptr) {
            return;
        }
        if (deleter_ != nullptr) {
            deleter_(GetExternalPointer(), data_);
        }
        SetExternalPointer(nullptr);
    }

    inline void SetDeleter(DeleteEntryPoint deleter)
    {
        deleter_ = deleter;
    }

    inline void SetData(void *data)
    {
        data_ = data;
    }

    inline const void *GetData() const
    {
        return data_;
    }

    inline void Destroy()
    {
        ClearExternalPointer();
        SetDeleter(nullptr);
        SetData(nullptr);
    }

private:
    DeleteEntryPoint deleter_{nullptr};
    alignas(sizeof(uint64_t)) void *data_ {nullptr};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSNATIVEPOINTER_H
