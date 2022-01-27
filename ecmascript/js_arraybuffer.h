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
    CAST_NO_CHECK(JSArrayBuffer);

    // 6.2.6.2
    static void CopyDataBlockBytes(JSTaggedValue toBlock, JSTaggedValue fromBlock, int32_t fromIndex, int32_t count);

    void Attach(JSThread *thread, uint32_t arrayBufferByteLength, JSTaggedValue arrayBufferData);
    void Detach(JSThread *thread);

    bool IsDetach()
    {
        JSTaggedValue arrayBufferData = GetArrayBufferData();
        return arrayBufferData == JSTaggedValue::Null();
    }

    static constexpr size_t DATA_OFFSET = JSObject::SIZE;
    ACCESSORS(ArrayBufferData, DATA_OFFSET, BYTE_LENGTH_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(ArrayBufferByteLength, uint32_t, BYTE_LENGTH_OFFSET, BIT_FIELD_OFFSET)
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t SHARED_BITS = 2;
    FIRST_BIT_FIELD(BitField, Shared, bool, SHARED_BITS)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, DATA_OFFSET, BYTE_LENGTH_OFFSET)
    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSARRAYBUFFER_H