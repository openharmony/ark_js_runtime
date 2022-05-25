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

#include "ecmascript/builtins/builtins_proxy.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_function.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript::builtins {
// 26.2.1.1 Proxy( [ value ] )
JSTaggedValue BuiltinsProxy::ProxyConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Proxy, Constructor);
    [[maybe_unused]] EcmaHandleScope handleScope(argv->GetThread());

    // 1.If NewTarget is undefined, throw a TypeError exception.
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "ProxyConstructor: NewTarget is undefined",
                                    JSTaggedValue::Exception());
    }

    // 2.Return ProxyCreate(target, handler).
    JSHandle<JSProxy> proxy = JSProxy::ProxyCreate(argv->GetThread(), GetCallArg(argv, 0), GetCallArg(argv, 1));
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(argv->GetThread());
    return proxy.GetTaggedValue();
}

// 26.2.2.1 Proxy.revocable ( target, handler )
JSTaggedValue BuiltinsProxy::Revocable([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Proxy, Revocable);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    // 1.Let p be ProxyCreate(target, handler).
    JSHandle<JSProxy> proxy = JSProxy::ProxyCreate(thread, GetCallArg(argv, 0), GetCallArg(argv, 1));

    // 2.ReturnIfAbrupt(p).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 3 ~ 4 new revoker function and set the [[RevocableProxy]] internal slot
    JSHandle<JSProxyRevocFunction> revoker = thread->GetEcmaVM()->GetFactory()->NewJSProxyRevocFunction(proxy);

    // 5.Let result be ObjectCreate(%ObjectPrototype%).
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = env->GetObjectFunctionPrototype();
    JSHandle<JSObject> result = thread->GetEcmaVM()->GetFactory()->OrdinaryNewJSObjectCreate(proto);

    // 6.Perform CreateDataProperty(result, "proxy", p).
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> proxyKey = globalConst->GetHandledProxyString();
    JSObject::CreateDataProperty(thread, result, proxyKey, JSHandle<JSTaggedValue>(proxy));

    // 7.Perform CreateDataProperty(result, "revoke", revoker).
    JSHandle<JSTaggedValue> revokeKey = globalConst->GetHandledRevokeString();
    JSObject::CreateDataProperty(thread, result, revokeKey, JSHandle<JSTaggedValue>(revoker));

    // 8.Return result.
    return result.GetTaggedValue();
}

// A Proxy revocation function to invalidate a specific Proxy object
JSTaggedValue BuiltinsProxy::InvalidateProxyFunction(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Proxy, InvalidateProxyFunction);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSObject> revoke_obj(GetThis(argv));
    JSHandle<JSTaggedValue> revokeKey = thread->GlobalConstants()->GetHandledRevokeString();

    PropertyDescriptor desc(thread);
    JSObject::GetOwnProperty(thread, revoke_obj, revokeKey, desc);
    JSProxyRevocFunction::ProxyRevocFunctions(thread, JSHandle<JSProxyRevocFunction>(desc.GetValue()));
    return JSTaggedValue::Undefined();
}
}  // namespace panda::ecmascript::builtins
