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

#ifndef ECMASCRIPT_JSARGUMENTS_H
#define ECMASCRIPT_JSARGUMENTS_H

#include "ecmascript/js_object.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript {
class JSThread;

class JSArguments : public JSObject {
public:
    static constexpr int LENGTH_OF_INLINE_PROPERTIES = 4;
    static constexpr int LENGTH_INLINE_PROPERTY_INDEX = 0;
    static constexpr int ITERATOR_INLINE_PROPERTY_INDEX = 1;
    static constexpr int CALLER_INLINE_PROPERTY_INDEX = 2;
    static constexpr int CALLEE_INLINE_PROPERTY_INDEX = 3;

    CAST_NO_CHECK(JSArguments);

    // 9.4.4.1 [[GetOwnProperty]] (P)
    static bool GetOwnProperty(JSThread *thread, const JSHandle<JSArguments> &args, const JSHandle<JSTaggedValue> &key,
                               PropertyDescriptor &desc);
    // 9.4.4.2 [[DefineOwnProperty]] (P, Desc)
    static bool DefineOwnProperty(JSThread *thread, const JSHandle<JSArguments> &args,
                                  const JSHandle<JSTaggedValue> &key, const PropertyDescriptor &desc);
    // 9.4.4.3 [[Get]] (P, Receiver)
    static inline OperationResult GetProperty(JSThread *thread, const JSHandle<JSObject> &obj,
                                              const JSHandle<JSTaggedValue> &key)
    {
        return GetProperty(thread, JSHandle<JSArguments>::Cast(obj), key, JSHandle<JSTaggedValue>::Cast(obj));
    }
    static OperationResult GetProperty(JSThread *thread, const JSHandle<JSArguments> &args,
                                       const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &receiver);
    // 9.4.4.4 [[Set]] ( P, V, Receiver)
    static inline bool SetProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                                   const JSHandle<JSTaggedValue> &value)
    {
        return SetProperty(thread, JSHandle<JSArguments>::Cast(obj), key, value, JSHandle<JSTaggedValue>::Cast(obj));
    }
    static bool SetProperty(JSThread *thread, const JSHandle<JSArguments> &args, const JSHandle<JSTaggedValue> &key,
                            const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &receiver);
    // 9.4.4.5 [[Delete]] (P)
    static bool DeleteProperty(JSThread *thread, const JSHandle<JSArguments> &args, const JSHandle<JSTaggedValue> &key);
    // 9.4.4.6 CreateUnmappedArgumentsObject(argumentsList)
    // 9.4.4.7 CreateMappedArgumentsObject ( func, formals, argumentsList, env )
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JSARGUMENTS_H
