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

#include "ecmascript/builtins/builtins_boolean.h"

#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_primitive_ref.h"

namespace panda::ecmascript::builtins {
// ecma 19.3.1.1 Boolean(value)
JSTaggedValue BuiltinsBoolean::BooleanConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Boolean, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let b be ToBoolean(value).
    bool boolValue = GetCallArg(argv, 0)->ToBoolean();
    // 2. If NewTarget is undefined, return b.
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        return GetTaggedBoolean(boolValue);
    }
    // 3. Let O be OrdinaryCreateFromConstructor(NewTarget, "%BooleanPrototype%", [[BooleanData]] ).
    // 5. Set the value of O's [[BooleanData]] internal slot to b.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> ctor = JSHandle<JSFunction>(GetConstructor(argv));
    JSHandle<JSObject> result = factory->NewJSObjectByConstructor(ctor, newTarget);
    JSTaggedValue objValue = boolValue ? JSTaggedValue::True() : JSTaggedValue::False();
    JSPrimitiveRef::Cast(*result)->SetValue(thread, objValue);
    // 4. ReturnIfAbrupt(O).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // 6. Return O.
    return result.GetTaggedValue();
}

// ecma 19.3.3 abstract operation thisBooleanValue(value)
JSTaggedValue BuiltinsBoolean::ThisBooleanValue(JSThread *thread, JSTaggedValue value)
{
    BUILTINS_API_TRACE(thread, Boolean, ThisBooleanValue);
    // 1. If Type(value) is Boolean, return value
    if (value.IsBoolean()) {
        return value == JSTaggedValue::True() ? GetTaggedBoolean(true) : GetTaggedBoolean(false);
    }
    // 2. If Type(value) is Object and value has a [[BooleanData]] internal slot, then
    if (value.IsJSPrimitiveRef()) {
        JSTaggedValue primitive = JSPrimitiveRef::Cast(value.GetTaggedObject())->GetValue();
        // a. Assert: value's [[BooleanData]] internal slot is a Boolean value.
        if (primitive.IsBoolean()) {
            // b. Return the value of value's [[BooleanData]] internal slot.
            return primitive == JSTaggedValue::True() ? GetTaggedBoolean(true) : GetTaggedBoolean(false);
        }
    }
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 3. Throw a TypeError exception.
    THROW_TYPE_ERROR_AND_RETURN(thread, "the type can not convert to BooleanValue", JSTaggedValue::Exception());
}

// ecma 19.3.3.2 Boolean.prototype.toString ()
JSTaggedValue BuiltinsBoolean::BooleanPrototypeToString(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Let b be thisBooleanValue(this value).
    JSTaggedValue thisValueToBoolean = BuiltinsBoolean::ThisBooleanValue(thread, GetThis(argv).GetTaggedValue());
    // 2. ReturnIfAbrupt(b)
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3. If b is true, return "true"; else return "false".
    return thisValueToBoolean.IsTrue() ? GetTaggedString(thread, "true") : GetTaggedString(thread, "false");
}

// ecma 19.3.3.3 Boolean.prototype.valueOf ()
JSTaggedValue BuiltinsBoolean::BooleanPrototypeValueOf(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    // 1. Return thisBooleanValue(this value).
    return BuiltinsBoolean::ThisBooleanValue(argv->GetThread(), GetThis(argv).GetTaggedValue());
}
}  // namespace panda::ecmascript::builtins
