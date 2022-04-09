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

#include "ecmascript/js_arguments.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
bool JSArguments::GetOwnProperty(JSThread *thread, const JSHandle<JSArguments> &args,
                                 const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc)
{
    // 1 ~ 3 Let desc be OrdinaryGetOwnProperty(args, P).
    JSObject::OrdinaryGetOwnProperty(thread, JSHandle<JSObject>(args), key, desc);
    if (desc.IsEmpty()) {
        return true;
    }

    // 4.Let map be the value of the [[ParameterMap]] internal slot of the arguments object.
    JSHandle<JSTaggedValue> map(thread, args->GetParameterMap());

    // 5.Let isMapped be HasOwnProperty(map, P).
    bool isMapped = JSTaggedValue::HasOwnProperty(thread, map, key);

    // 6.Assert: isMapped is never an abrupt completion.
    ASSERT(!thread->HasPendingException());

    // 7.If the value of isMapped is true, then
    //   a.Set desc.[[Value]] to Get(map, P).
    if (isMapped) {
        auto prop = JSObject::GetProperty(thread, map, key).GetValue();
        desc.SetValue(prop);
    }

    // 8.If IsDataDescriptor(desc) is true and P is "caller" and desc.[[Value]] is a strict mode Function object,
    //   throw a TypeError exception.
    JSHandle<EcmaString> caller = thread->GetEcmaVM()->GetFactory()->NewFromASCII("caller");
    if (desc.IsDataDescriptor() && JSTaggedValue::SameValue(key.GetTaggedValue(), caller.GetTaggedValue()) &&
        desc.GetValue()->IsJSFunction()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Arguments GetOwnProperty: type error", false);
    }
    // 9.Return desc.
    return true;
}

bool JSArguments::DefineOwnProperty(JSThread *thread, const JSHandle<JSArguments> &args,
                                    const JSHandle<JSTaggedValue> &key, const PropertyDescriptor &desc)
{
    // 1 ~ 2 Let args be the arguments object and get map.
    JSHandle<JSTaggedValue> map(thread, args->GetParameterMap());

    // 3.Let isMapped be HasOwnProperty(map, P).
    bool isMapped = JSTaggedValue::HasOwnProperty(thread, map, key);

    // 4.Let allowed be OrdinaryDefineOwnProperty(args, P, Desc).
    bool allowed = JSObject::OrdinaryDefineOwnProperty(thread, JSHandle<JSObject>(args), key, desc);

    // 5.ReturnIfAbrupt(allowed).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, allowed);

    // 6.If allowed is false, return false.
    if (!allowed) {
        return false;
    }

    // 7.If the value of isMapped is true, then
    //   a.If IsAccessorDescriptor(Desc) is true, then
    //     i.Call map.[[Delete]](P).
    //   b.Else
    //     i.If Desc.[[Value]] is present, then
    //        1.Let setStatus be Set(map, P, Desc.[[Value]], false).
    //        2.Assert: setStatus is true because formal parameters mapped by argument objects are always writable.
    //     ii.If Desc.[[Writable]] is present and its value is false, then
    //        1.Call map.[[Delete]](P).
    if (isMapped) {
        if (desc.IsAccessorDescriptor()) {
            JSTaggedValue::DeleteProperty(thread, map, key);
        } else {
            if (desc.HasValue()) {
                [[maybe_unused]] bool setStatus = JSTaggedValue::SetProperty(thread, map, key, desc.GetValue(), false);
                ASSERT(setStatus == true);
            }
            if (desc.HasWritable() && !desc.IsWritable()) {
                JSTaggedValue::DeleteProperty(thread, map, key);
            }
        }
    }

    // 8.Return true.
    return true;
}

OperationResult JSArguments::GetProperty(JSThread *thread, const JSHandle<JSArguments> &args,
                                         const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &receiver)
{
    // 1 ~ 2 Let args be the arguments object and get map.
    JSHandle<JSTaggedValue> map(thread, args->GetParameterMap());

    // 3.Let isMapped be HasOwnProperty(map, P).
    bool isMapped = JSTaggedValue::HasOwnProperty(thread, map, key);

    // 4.Assert: isMapped is not an abrupt completion.
    ASSERT(!thread->HasPendingException());

    // 5.If the value of isMapped is false, then
    //   a.Return the result of calling the default ordinary object [[Get]] internal method (9.1.8)
    //   on args passing P and Receiver as the arguments.
    if (!isMapped) {
        return JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(args), key, receiver);
    }

    // 6.Else map contains a formal parameter mapping for P,
    //   a.Return Get(map, P).
    return JSTaggedValue::GetProperty(thread, map, key);
}

bool JSArguments::SetProperty(JSThread *thread, const JSHandle<JSArguments> &args, const JSHandle<JSTaggedValue> &key,
                              const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &receiver)
{
    // 1.Let args be the arguments object.
    JSHandle<JSTaggedValue> map(thread, args->GetParameterMap());

    // 2.If SameValue(args, Receiver) is false, then
    //   a.Let isMapped be false.
    bool isMapped = false;
    if (JSTaggedValue::SameValue(args.GetTaggedValue(), receiver.GetTaggedValue())) {
        // 3.Else,
        //   a.Let map be the value of the [[ParameterMap]] internal slot of the arguments object.
        //   b.Let isMapped be HasOwnProperty(map, P).
        //   c.Assert: isMapped is not an abrupt completion.
        isMapped = JSTaggedValue::HasOwnProperty(thread, map, key);
        ASSERT(!thread->HasPendingException());
    }

    // 4.If isMapped is true, then
    //   a.Let setStatus be Set(map, P, V, false).
    //   b.Assert: setStatus is true because formal parameters mapped by argument objects are always writable.
    if (isMapped) {
        [[maybe_unused]] bool setStatus = JSTaggedValue::SetProperty(thread, map, key, value);
        ASSERT(setStatus == true);
    }

    // 5.Return the result of calling the default ordinary object [[Set]] internal method (9.1.9)
    // on args passing P, V and Receiver as the arguments.
    return JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(args), key, value, receiver);
}

bool JSArguments::DeleteProperty(JSThread *thread, const JSHandle<JSArguments> &args,
                                 const JSHandle<JSTaggedValue> &key)
{
    // 1.Let map be the value of the [[ParameterMap]] internal slot of the arguments object.
    JSHandle<JSTaggedValue> map(thread, args->GetParameterMap());

    // 2.Let isMapped be HasOwnProperty(map, P).
    bool isMapped = JSTaggedValue::HasOwnProperty(thread, map, key);

    // 3.Assert: isMapped is not an abrupt completion.
    ASSERT(!thread->HasPendingException());

    // 4.Let result be the result of calling the default [[Delete]] internal method for ordinary objects (9.1.10)
    // on the arguments object passing P as the argument.
    bool result = JSTaggedValue::DeleteProperty(thread, JSHandle<JSTaggedValue>(args), key);

    // 5.ReturnIfAbrupt(result).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, result);

    // 6.If result is true and the value of isMapped is true, then
    //   a.Call map.[[Delete]](P).
    if (result && isMapped) {
        JSTaggedValue::DeleteProperty(thread, map, key);
    }

    // 7.Return result.
    return result;
}
}  // namespace panda::ecmascript
