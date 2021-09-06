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

#ifndef PANDA_RUNTIME_ECMASCRIPT_JSPRIMITIVEREF_H
#define PANDA_RUNTIME_ECMASCRIPT_JSPRIMITIVEREF_H

#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
enum class PrimitiveType : uint8_t {
    PRIMITIVE_BOOLEAN = 0,
    PRIMITIVE_NUMBER,
    PRIMITIVE_STRING,
    PRIMITIVE_SYMBOL,
};

class JSPrimitiveRef : public JSObject {
public:
    static JSPrimitiveRef *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsJSPrimitiveRef());
        return static_cast<JSPrimitiveRef *>(object);
    }

    JSPrimitiveRef() = delete;

    bool IsNumber() const
    {
        return GetValue().IsNumber();
    }

    bool IsBoolean() const
    {
        return GetValue().IsBoolean();
    }

    bool IsString() const
    {
        return GetValue().IsString();
    }

    bool IsSymbol() const
    {
        return GetValue().IsSymbol();
    }

    uint32_t GetStringLength() const
    {
        ASSERT(IsString());
        return EcmaString::Cast(GetValue().GetTaggedObject())->GetLength();
    }

    // ES6 9.4.3 String Exotic Objects
    // ES6 9.4.3.4 StringCreate( value, prototype)// proto always be %StringPrototype%
    static JSHandle<JSPrimitiveRef> StringCreate(JSThread *thread, const JSHandle<JSTaggedValue> &value);
    static bool StringGetIndexProperty(const JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                       PropertyDescriptor *desc);

    static constexpr size_t VALUE_OFFSET = JSObject::SIZE;
    ACCESSORS(Value, VALUE_OFFSET, SIZE);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, VALUE_OFFSET, SIZE)

    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_JSPRIMITIVEREF_H
