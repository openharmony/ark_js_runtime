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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_PROMISE_H
#define ECMASCRIPT_BUILTINS_BUILTINS_PROMISE_H

#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::builtins {
class BuiltinsPromise : public base::BuiltinsBase {
public:
    // 25.4.3.1 Promise ( executor )
    static JSTaggedValue PromiseConstructor(EcmaRuntimeCallInfo *argv);
    // 25.4.4.1 Promise.all ( iterable )
    static JSTaggedValue All(EcmaRuntimeCallInfo *argv);
    // 25.4.4.3 Promise.race ( iterable )
    static JSTaggedValue Race(EcmaRuntimeCallInfo *argv);
    // 25.4.4.4 Promise.reject ( r )
    static JSTaggedValue Reject(EcmaRuntimeCallInfo *argv);
    // 25.4.4.5 Promise.resolve ( x )
    static JSTaggedValue Resolve(EcmaRuntimeCallInfo *argv);
    // 25.4.4.6 get Promise [ @@species ]
    static JSTaggedValue GetSpecies(EcmaRuntimeCallInfo *argv);
    // 25.4.5.1 Promise.prototype.catch ( onRejected )
    static JSTaggedValue Catch(EcmaRuntimeCallInfo *argv);
    // 25.4.5.3 Promise.prototype.then ( onFulfilled , onRejected )
    static JSTaggedValue Then(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue PerformPromiseThen(JSThread *thread, const JSHandle<JSPromise> &promise,
                                            const JSHandle<JSTaggedValue> &onFulfilled,
                                            const JSHandle<JSTaggedValue> &onRejected,
                                            const JSHandle<PromiseCapability> &capability);

private:
    static JSHandle<CompletionRecord> PerformPromiseAll(JSThread *thread,
                                                        const JSHandle<PromiseIteratorRecord> &itRecord,
                                                        const JSHandle<JSTaggedValue> &ctor,
                                                        const JSHandle<PromiseCapability> &capa);

    static JSHandle<CompletionRecord> PerformPromiseRace(JSThread *thread,
                                                         const JSHandle<PromiseIteratorRecord> &iteratorRecord,
                                                         const JSHandle<PromiseCapability> &capability,
                                                         const JSHandle<JSTaggedValue> &constructor);
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_PROMISE_H