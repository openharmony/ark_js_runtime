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

#ifndef ECMASCRIPT_GENERATOR_HELPER_H
#define ECMASCRIPT_GENERATOR_HELPER_H

#include <array>

#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_handle.h"

namespace panda::ecmascript {
class GeneratorHelper {
public:
    static JSHandle<JSObject> Next(JSThread *thread, const JSHandle<GeneratorContext> &genContext, JSTaggedValue value);

    static JSHandle<JSObject> Return(JSThread *thread, const JSHandle<GeneratorContext> &genContext,
                                     JSTaggedValue value);

    static JSHandle<JSObject> Throw(JSThread *thread, const JSHandle<GeneratorContext> &genContext,
                                    JSTaggedValue value);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_GENERATOR_HELPER_H
