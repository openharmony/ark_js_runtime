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

#ifndef ECMASCRIPT_JSARRAYBUFFER_H
#define ECMASCRIPT_JSARRAYBUFFER_H

#include "ecmascript/js_object.h"

namespace panda::ecmascript {
class JSArrayBuffer final : public JSObject {
public:
    static JSArrayBuffer *Cast(ObjectHeader *object)
    {
        return static_cast<JSArrayBuffer *>(object);
    }
    // 6.2.6.2
    static void CopyDataBlockBytes(JSTaggedValue toBlock, JSTaggedValue fromBlock, int32_t fromIndex, int32_t count);

    void Attach(JSThread *thread, JSTaggedValue arrayBufferByteLength, JSTaggedValue arrayBufferData);
    void Detach(JSThread *thread);

    bool IsDetach()
    {
        JSTaggedValue arrayBufferData = GetArrayBufferData();
        return arrayBufferData == JSTaggedValue::Null();
    }

    static constexpr size_t ARRAY_BUFFER_BYTE_LENGTH_OFFSET = JSObject::SIZE;
    ACCESSORS(ArrayBufferByteLength, ARRAY_BUFFER_BYTE_LENGTH_OFFSET, ARRAY_BUFFER_DATA_OFFSET)
    ACCESSORS(ArrayBufferData, ARRAY_BUFFER_DATA_OFFSET, ARRAY_BUFFER_SHARED_OFFSET)
    ACCESSORS(Shared, ARRAY_BUFFER_SHARED_OFFSET, SIZE)

    DECL_DUMP()
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, ARRAY_BUFFER_BYTE_LENGTH_OFFSET, SIZE)
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSARRAYBUFFER_H