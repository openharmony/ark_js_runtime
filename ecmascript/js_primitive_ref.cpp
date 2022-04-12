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

#include "js_primitive_ref.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/mem/assert_scope.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
// ES6 9.4.3.4 StringCreate( value, prototype)
JSHandle<JSPrimitiveRef> JSPrimitiveRef::StringCreate(JSThread *thread, const JSHandle<JSTaggedValue> &value)
{
    // 1. ReturnIfAbrupt(prototype).
    // 2. Assert: Type(value) is String.
    ASSERT(value->IsString());
    // 3. Let S be a newly created String exotic object.
    // 4. Set the [[StringData]] internal slot of S to value.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> str(factory->NewJSString(value));
    // 11. Let length be the number of code unit elements in value.
    // 12. Let status be DefinePropertyOrThrow(S, "length", PropertyDescriptor{[[Value]]: length, [[Writable]]:
    // false, [[Enumerable]]: false, [[Configurable]]: false }).
    JSHandle<JSTaggedValue> lengthStr = thread->GlobalConstants()->GetHandledLengthString();

    uint32_t length = EcmaString::Cast(value->GetTaggedObject())->GetLength();
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(length)), false, false, false);
    [[maybe_unused]] bool status = JSTaggedValue::DefinePropertyOrThrow(thread, str, lengthStr, desc);
    // 13. Assert: status is not an abrupt completion.
    // 14. Return S.
    ASSERT(status);
    return JSHandle<JSPrimitiveRef>(str);
}

bool JSPrimitiveRef::StringGetIndexProperty(const JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                            PropertyDescriptor *desc)
{
    uint16_t tmpChar;
    {
        DISALLOW_GARBAGE_COLLECTION;
        EcmaString *str = EcmaString::Cast(JSPrimitiveRef::Cast(*obj)->GetValue().GetTaggedObject());
        if (str->GetLength() <= index) {
            return false;
        }
        // 10. Let resultStr be a String value of length 1, containing one code unit from str, specifically the code
        // unit at index index
        tmpChar = str->At(index);
    }
    JSHandle<JSTaggedValue> value(thread->GetEcmaVM()->GetFactory()->NewFromUtf16(&tmpChar, 1));
    // 11. Return a PropertyDescriptor{ [[Value]]: resultStr, [[Enumerable]]: true, [[Writable]]: false,
    // [[Configurable]]: false }.
    desc->SetValue(value);
    desc->SetEnumerable(true);
    desc->SetWritable(false);
    desc->SetConfigurable(false);
    return true;
}
// ES6 9.4.3.3 [[OwnPropertyKeys]] ( )
}  // namespace panda::ecmascript
