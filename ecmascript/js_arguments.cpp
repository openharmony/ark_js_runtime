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
    // 4.Let allowed be OrdinaryDefineOwnProperty(args, P, Desc).
    bool allowed = JSObject::OrdinaryDefineOwnProperty(thread, JSHandle<JSObject>(args), key, desc);

    // 5.ReturnIfAbrupt(allowed).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, allowed);

    // 8.Return true.
    return true;
}

OperationResult JSArguments::GetProperty(JSThread *thread, const JSHandle<JSArguments> &args,
                                         const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &receiver)
{
    // 5.If the value of isMapped is false, then
    //   a.Return the result of calling the default ordinary object [[Get]] internal method (9.1.8)
    //   on args passing P and Receiver as the arguments.
    return JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(args), key, receiver);
}

bool JSArguments::SetProperty(JSThread *thread, const JSHandle<JSArguments> &args, const JSHandle<JSTaggedValue> &key,
                              const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &receiver)
{
    // 5.Return the result of calling the default ordinary object [[Set]] internal method (9.1.9)
    // on args passing P, V and Receiver as the arguments.
    return JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(args), key, value, receiver);
}

bool JSArguments::DeleteProperty(JSThread *thread, const JSHandle<JSArguments> &args,
                                 const JSHandle<JSTaggedValue> &key)
{
    // 4.Let result be the result of calling the default [[Delete]] internal method for ordinary objects (9.1.10)
    // on the arguments object passing P as the argument.
    bool result = JSTaggedValue::DeleteProperty(thread, JSHandle<JSTaggedValue>(args), key);

    // 5.ReturnIfAbrupt(result).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, result);

    // 7.Return result.
    return result;
}
}  // namespace panda::ecmascript
