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

#include "ecmascript/js_string_iterator.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
JSHandle<JSStringIterator> JSStringIterator::CreateStringIterator(const JSThread *thread,
                                                                  const JSHandle<EcmaString> &string)
{
    // 1. Assert: Type(string) is String.
    // 2. Let iterator be ObjectCreate(%StringIteratorPrototype%, [[IteratedString]], [[StringIteratorNextIndex]] ?.)
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> strIterCtor = env->GetStringIterator();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSStringIterator> iterator = JSHandle<JSStringIterator>::Cast(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(strIterCtor), strIterCtor));
    // 3. Set iterator’s [[IteratedString]] internal slot to string.
    // 4. Set iterator’s [[StringIteratorNextIndex]] internal slot to 0.
    iterator->SetIteratedString(thread, string);
    iterator->SetStringIteratorNextIndex(thread, JSTaggedValue(0));
    return iterator;
}
}  // namespace panda::ecmascript
