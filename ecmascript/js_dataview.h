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

#ifndef ECMASCRIPT_JSDATAVIEW_H
#define ECMASCRIPT_JSDATAVIEW_H

#include "ecmascript/js_object.h"

namespace panda::ecmascript {
enum class DataViewType : uint8_t { FLOAT32 = 0, FLOAT64, INT8, INT16, INT32, UINT8, UINT16, UINT32, UINT8_CLAMPED };
class JSDataView : public JSObject {
public:
    static JSDataView *Cast(ObjectHeader *object)
    {
        return static_cast<JSDataView *>(object);
    }
    static int32_t GetElementSize(DataViewType type);

    static constexpr size_t DATA_VIEW_OFFSET = JSObject::SIZE;
    ACCESSORS(DataView, DATA_VIEW_OFFSET, VIEW_ARRAY_BUFFER_OFFSET);
    ACCESSORS(ViewedArrayBuffer, VIEW_ARRAY_BUFFER_OFFSET, BYTE_LENGTH_OFFSET);
    ACCESSORS(ByteLength, BYTE_LENGTH_OFFSET, BYTE_OFFSET_OFFSET);
    ACCESSORS(ByteOffset, BYTE_OFFSET_OFFSET, SIZE);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, DATA_VIEW_OFFSET, SIZE)

    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSDATAVIEW_H
