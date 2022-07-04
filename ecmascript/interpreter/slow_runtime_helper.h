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

#ifndef ECMASCRIPT_INTERPRETER_SLOW_RUNTIME_HELPER_H
#define ECMASCRIPT_INTERPRETER_SLOW_RUNTIME_HELPER_H

#include "ecmascript/js_function.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
class SlowRuntimeHelper {
public:
    static JSTaggedValue NewObject(EcmaRuntimeCallInfo *info);

    static JSTaggedValue CallBoundFunction(EcmaRuntimeCallInfo *info);

    static void SaveFrameToContext(JSThread *thread, JSHandle<GeneratorContext> context);

    static JSTaggedValue Construct(JSThread *thread, JSHandle<JSTaggedValue> ctor, JSHandle<JSTaggedValue> newTarget,
                                   JSHandle<JSTaggedValue> preArgs, uint32_t argsCount, uint32_t baseArgLocation);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_INTERPRETER_SLOW_RUNTIME_HELPER_H
