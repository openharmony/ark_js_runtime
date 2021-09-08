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

#include "ecmascript/builtins/builtins_promise_job.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_tagged_value.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsPromiseJob::PromiseReactionJob(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), PromiseJob, Reaction);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    // 1. Assert: reaction is a PromiseReaction Record.
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    ASSERT(value->IsPromiseReaction());
    JSHandle<PromiseReaction> reaction = JSHandle<PromiseReaction>::Cast(value);
    JSHandle<JSTaggedValue> argument = GetCallArg(argv, 1);

    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // 2. Let promiseCapability be reaction.[[Capabilities]].
    JSHandle<PromiseCapability> capability(thread, reaction->GetPromiseCapability());
    // 3. Let handler be reaction.[[Handler]].
    JSHandle<JSTaggedValue> handler(thread, reaction->GetHandler());
    JSHandle<JSTaggedValue> thisValue = globalConst->GetHandledUndefined();
    JSMutableHandle<JSTaggedValue> call(thread, capability->GetResolve());
    InternalCallParams *result = thread->GetInternalCallParams();
    result->MakeArgv(argument);
    if (handler->IsString()) {
        // 4. If handler is "Identity", let handlerResult be NormalCompletion(argument).
        // 5. Else if handler is "Thrower", let handlerResult be Completion{[[type]]: throw, [[value]]: argument,
        // [[target]]: empty}.

        if (EcmaString::StringsAreEqual(handler.GetObject<EcmaString>(),
                                        globalConst->GetHandledThrowerString().GetObject<EcmaString>())) {
            call.Update(capability->GetReject());
        }
    } else {
        // 6. Else, let handlerResult be Call(handler, undefined, «argument»).
        JSTaggedValue taggedValue = JSFunction::Call(thread, handler, thisValue, 1, result->GetArgv());
        result->MakeArgv(taggedValue);
        // 7. If handlerResult is an abrupt completion, then
        // a. Let status be Call(promiseCapability.[[Reject]], undefined, «handlerResult.[[value]]»).
        // b. NextJob Completion(status).
        if (thread->HasPendingException()) {
            JSHandle<JSTaggedValue> throwValue = JSPromise::IfThrowGetThrowValue(thread);
            thread->ClearException();
            result->MakeArgv(throwValue);
            call.Update(capability->GetReject());
        }
    }
    // 8. Let status be Call(promiseCapability.[[Resolve]], undefined, «handlerResult.[[value]]»).
    return JSFunction::Call(thread, call, thisValue, 1, result->GetArgv());
}

JSTaggedValue BuiltinsPromiseJob::PromiseResolveThenableJob(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), PromiseJob, ResolveThenableJob);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> promise = GetCallArg(argv, 0);
    ASSERT(promise->IsJSPromise());
    // 1. Let resolvingFunctions be CreateResolvingFunctions(promiseToResolve).
    JSHandle<ResolvingFunctionsRecord> resolvingFunctions =
        JSPromise::CreateResolvingFunctions(thread, JSHandle<JSPromise>::Cast(promise));
    JSHandle<JSTaggedValue> thenable = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> then = GetCallArg(argv, BuiltinsBase::ArgsPosition::THIRD);

    // 2. Let thenCallResult be Call(then, thenable, «resolvingFunctions.[[Resolve]], resolvingFunctions.[[Reject]]»).
    InternalCallParams *arguments = thread->GetInternalCallParams();
    arguments->MakeArgv(resolvingFunctions->GetResolveFunction(), resolvingFunctions->GetRejectFunction());
    JSTaggedValue result = JSFunction::Call(thread, then, thenable, 2, arguments->GetArgv());  // 2: two args
    JSHandle<JSTaggedValue> thenResult(thread, result);
    // 3. If thenCallResult is an abrupt completion,
    // a. Let status be Call(resolvingFunctions.[[Reject]], undefined, «thenCallResult.[[value]]»).
    // b. NextJob Completion(status).
    if (thread->HasPendingException()) {
        thenResult = JSPromise::IfThrowGetThrowValue(thread);
        thread->ClearException();
        JSHandle<JSTaggedValue> reject(thread, resolvingFunctions->GetRejectFunction());
        JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();
        arguments->MakeArgv(thenResult);
        return JSFunction::Call(thread, reject, undefined, 1, arguments->GetArgv());
    }
    // 4. NextJob Completion(thenCallResult).
    return result;
}
}  // namespace panda::ecmascript::builtins
