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

#ifndef PANDA_RUNTIME_ECMASCRIPT_JS_STABLE_ARRAY_H
#define PANDA_RUNTIME_ECMASCRIPT_JS_STABLE_ARRAY_H

#include <limits>
#include "ecmascript/js_array.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
class JSStableArray {
public:
    enum SeparatorFlag : int { MINUS_ONE = -1, MINUS_TWO = -2 };
    static constexpr int FLAG_OF_SEPARATOR = 3;
    static JSTaggedValue Push(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Pop(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Join(JSHandle<JSArray> receiver, EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_JS_STABLE_ARRAY_H