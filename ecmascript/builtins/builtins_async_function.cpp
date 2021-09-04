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

#include "ecmascript/builtins/builtins_async_function.h"
#include "ecmascript/ecma_macros.h"

namespace panda::ecmascript::builtins {
// ecma2017 25.5.1.1 AsyncFunction (p1, p2, ... , pn, body)
JSTaggedValue BuiltinsAsyncFunction::AsyncFunctionConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    // not support
    THROW_TYPE_ERROR_AND_RETURN(argv->GetThread(), "Not support eval. Forbidden using new AsyncFunction().",
                                JSTaggedValue::Exception());
}
}  // namespace panda::ecmascript::builtins
