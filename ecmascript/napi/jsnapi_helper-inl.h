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

#ifndef PANDA_RUNTIME_ECMASCRIPT_NAPI_JSNAPI_HELPER_INL_H
#define PANDA_RUNTIME_ECMASCRIPT_NAPI_JSNAPI_HELPER_INL_H

#include "ecmascript/js_tagged_value.h"
#include "jsnapi_helper.h"
#include "libpandabase/macros.h"

namespace panda {
template <typename T>
Local<T> JSNApiHelper::ToLocal(ecmascript::JSHandle<ecmascript::JSTaggedValue> from)
{
    return Local<T>(from.GetAddress());
}

ecmascript::JSTaggedValue JSNApiHelper::ToJSTaggedValue(JSValueRef *from)
{
    ASSERT(from != nullptr);
    return *reinterpret_cast<ecmascript::JSTaggedValue *>(from);
}

ecmascript::JSHandle<ecmascript::JSTaggedValue> JSNApiHelper::ToJSHandle(Local<JSValueRef> from)
{
    ASSERT(!from.IsEmpty());
    return ecmascript::JSHandle<ecmascript::JSTaggedValue>(reinterpret_cast<uintptr_t>(*from));
}

ecmascript::JSHandle<ecmascript::JSTaggedValue> JSNApiHelper::ToJSHandle(JSValueRef *from)
{
    ASSERT(from != nullptr);
    return ecmascript::JSHandle<ecmascript::JSTaggedValue>(reinterpret_cast<uintptr_t>(from));
}
}  // namespace panda
#endif  // PANDA_RUNTIME_ECMASCRIPT_NAPI_JSNAPI_HELPER_INL_H