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

#include "ecmascript/builtins/builtins_intl.h"

#include "ecmascript/js_array.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript::builtins {
// 8.2.1 Intl.getCanonicalLocales ( locales )
JSTaggedValue BuiltinsIntl::GetCanonicalLocales(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope scope(thread);

    // 1.Let ll be ? CanonicalizeLocaleList(locales).
    JSHandle<JSTaggedValue> locales = GetCallArg(argv, 0);
    JSHandle<TaggedArray> elements = JSLocale::CanonicalizeLocaleList(thread, locales);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    // 2.Return CreateArrayFromList(ll).
    JSHandle<JSArray> result = JSArray::CreateArrayFromList(thread, elements);
    return result.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins