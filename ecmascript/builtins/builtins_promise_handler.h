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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_PROMISE_HANDLER_H
#define ECMASCRIPT_BUILTINS_BUILTINS_PROMISE_HANDLER_H

#include "ecmascript/base/builtins_base.h"

namespace panda::ecmascript::builtins {
class BuiltinsPromiseHandler : public base::BuiltinsBase {
public:
    // es6 26.6.1.3.1 Promise Resolve Functions
    static JSTaggedValue Resolve(EcmaRuntimeCallInfo *argv);

    // es6 26.6.1.3.2 Promise Reject Functions
    static JSTaggedValue Reject(EcmaRuntimeCallInfo *argv);

    // es6 26.6.1.5.1 GetCapabilitiesExecutor Functions
    static JSTaggedValue Executor(EcmaRuntimeCallInfo *argv);

    // es2017 25.5.5.4 AsyncFunction Awaited Fulfilled
    static JSTaggedValue AsyncAwaitFulfilled(EcmaRuntimeCallInfo *argv);

    // es2017 25.5.5.5 AsyncFunction Awaited Rejected
    static JSTaggedValue AsyncAwaitRejected(EcmaRuntimeCallInfo *argv);

    // es6 25.4.4.1.2 Promise.all Resolve Element Functions
    static JSTaggedValue ResolveElementFunction(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue ThenFinally(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue CatchFinally(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue valueThunkFunction(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue throwerFunction(EcmaRuntimeCallInfo *argv);

    static JSHandle<JSTaggedValue> PromiseResolve(JSThread *thread, const JSHandle<JSTaggedValue> &constructor,
                                                  const JSHandle<JSTaggedValue> &xValue);

    static JSTaggedValue AllSettledResolveElementFunction(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue AllSettledRejectElementFunction(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue AnyRejectElementFunction(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_PROMISE_HANDLER_H
