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

#ifndef PANDA_RUNTIME_ECMASCRIPT_JS_ITERATOR_H
#define PANDA_RUNTIME_ECMASCRIPT_JS_ITERATOR_H

#include "ecmascript/js_tagged_value.h"
#include "tagged_array-inl.h"

namespace panda::ecmascript {
enum class IterationKind { KEY = 0, VALUE, KEY_AND_VALUE };
class JSIterator final {
public:
    static JSTaggedValue IteratorCloseAndReturn(JSThread *thread, const JSHandle<JSTaggedValue> &iter,
                                                const JSHandle<JSTaggedValue> &status);
    // 7.4.1
    static JSHandle<JSTaggedValue> GetIterator(JSThread *thread, const JSHandle<JSTaggedValue> &obj);

    static JSHandle<JSTaggedValue> GetIterator(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                               const JSHandle<JSTaggedValue> &method);
    // 7.4.2
    static JSHandle<JSObject> IteratorNext(JSThread *thread, const JSHandle<JSTaggedValue> &iter,
                                           const JSHandle<JSTaggedValue> &value);

    static JSHandle<JSObject> IteratorNext(JSThread *thread, const JSHandle<JSTaggedValue> &iter);
    // 7.4.3
    static bool IteratorComplete(JSThread *thread, const JSHandle<JSTaggedValue> &iterResult);
    // 7.4.4
    static JSHandle<JSTaggedValue> IteratorValue(JSThread *thread, const JSHandle<JSTaggedValue> &iterResult);
    // 7.4.5
    static JSHandle<JSTaggedValue> IteratorStep(JSThread *thread, const JSHandle<JSTaggedValue> &iter);
    // 7.4.6
    static JSHandle<JSTaggedValue> IteratorClose(JSThread *thread, const JSHandle<JSTaggedValue> &iter,
                                                 const JSHandle<JSTaggedValue> &completion);
    // 7.4.7
    static JSHandle<JSObject> CreateIterResultObject(JSThread *thread, const JSHandle<JSTaggedValue> &value, bool done);
};
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_JS_ITERATOR_H
