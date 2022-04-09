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

#include "ecmascript/global_env.h"
#include "ecmascript/builtins/builtins_promise_handler.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/object_factory.h"
#include "js_array.h"
#include "js_realm.h"

namespace panda::ecmascript {
JSHandle<JSTaggedValue> GlobalEnv::GetSymbol(JSThread *thread, const JSHandle<JSTaggedValue> &string)
{
    JSHandle<JSTaggedValue> symbolFunction(GetSymbolFunction());
    return JSObject::GetProperty(thread, symbolFunction, string).GetValue();
}

JSHandle<JSTaggedValue> GlobalEnv::GetStringPrototypeFunctionByName(JSThread *thread, const char *name)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> stringFuncPrototype(thread,
        JSObject::GetPrototype(JSHandle<JSObject>(GetStringFunction())));
    JSHandle<JSTaggedValue> nameKey(factory->NewFromUtf8(name));
    return JSObject::GetProperty(thread, stringFuncPrototype, nameKey).GetValue();
}

JSHandle<JSTaggedValue> GlobalEnv::GetStringFunctionByName(JSThread *thread, const char *name)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> stringFuncObj = GetStringFunction();
    JSHandle<JSTaggedValue> nameKey(factory->NewFromUtf8(name));
    return JSObject::GetProperty(thread, stringFuncObj, nameKey).GetValue();
}
}  // namespace panda::ecmascript
