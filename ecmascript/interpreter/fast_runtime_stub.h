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

#ifndef ECMASCRIPT_INTERPRETER_FAST_RUNTIME_STUB_H
#define ECMASCRIPT_INTERPRETER_FAST_RUNTIME_STUB_H

#include <memory>
#include "ecmascript/frames.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
class GlobalEnv;
class PropertyAttributes;

class FastRuntimeStub {
public:
    static inline JSTaggedValue FastMul(JSTaggedValue left, JSTaggedValue right);
    static inline JSTaggedValue FastDiv(JSTaggedValue left, JSTaggedValue right);
    static inline JSTaggedValue FastMod(JSTaggedValue left, JSTaggedValue right);
    static inline JSTaggedValue FastEqual(JSTaggedValue left, JSTaggedValue right);
    static inline JSTaggedValue FastTypeOf(JSThread *thread, JSTaggedValue obj);
    static inline bool FastStrictEqual(JSTaggedValue left, JSTaggedValue right);
    static inline JSTaggedValue NewLexicalEnvDyn(JSThread *thread, ObjectFactory *factory, uint16_t numVars);
    static inline JSTaggedValue GetGlobalOwnProperty(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key);
    template<bool UseOwn = false>
    static inline JSTaggedValue GetPropertyByName(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key);
    template<bool UseOwn = false>
    static inline JSTaggedValue GetPropertyByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key);
    template<bool UseOwn = false>
    static inline JSTaggedValue GetPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index);
    template<bool UseOwn = false>
    static inline JSTaggedValue SetPropertyByName(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                                  JSTaggedValue value);
    template<bool UseOwn = false>
    static inline JSTaggedValue SetPropertyByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                                   JSTaggedValue value);
    template<bool UseOwn = false>
    static inline JSTaggedValue SetPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                                   JSTaggedValue value);

    static inline bool FastSetPropertyByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                              JSTaggedValue value);
    static inline bool FastSetPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                              JSTaggedValue value);
    static inline JSTaggedValue FastGetPropertyByName(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key);
    static inline JSTaggedValue FastGetPropertyByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key);
    template<bool UseHole = false>
    static inline JSTaggedValue FastGetPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index);
    static inline PropertyAttributes AddPropertyByName(JSThread *thread, JSHandle<JSObject> objHandle,
                                                       JSHandle<JSTaggedValue> keyHandle,
                                                       JSHandle<JSTaggedValue> valueHandle,
                                                       PropertyAttributes attr);

    static inline JSTaggedValue NewThisObject(JSThread *thread, JSTaggedValue ctor, JSTaggedValue newTarget,
                                              InterpretedFrame* state);

private:
    friend class ICRuntimeStub;
    static inline bool IsSpecialIndexedObj(JSType jsType);
    static inline bool IsSpecialReceiverObj(JSType jsType);
    static inline bool IsFastTypeArray(JSType jsType);
    static inline int32_t TryToElementsIndex(JSTaggedValue key);
    static inline JSTaggedValue CallGetter(JSThread *thread, JSTaggedValue receiver, JSTaggedValue holder,
                                           JSTaggedValue value);
    static inline JSTaggedValue CallSetter(JSThread *thread, JSTaggedValue receiver, JSTaggedValue value,
                                           JSTaggedValue accessorValue);
    static inline bool ShouldCallSetter(JSTaggedValue receiver, JSTaggedValue holder, JSTaggedValue accessorValue,
                                        PropertyAttributes attr);
    static inline JSTaggedValue AddPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                                   JSTaggedValue value);

    // non ECMA standard jsapi container
    static inline bool IsSpecialContainer(JSType jsType);
    static inline JSTaggedValue GetContainerProperty(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                                     JSType jsType);
    static inline JSTaggedValue SetContainerProperty(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                                     JSTaggedValue value, JSType jsType);
    static inline bool TryStringOrSymbolToIndex(JSTaggedValue key, uint32_t *output);
    static inline JSTaggedValue FastGetTypeArrayProperty(JSThread *thread, JSTaggedValue receiver, JSTaggedValue holder,
                                                         JSTaggedValue key, JSType jsType);
    static inline JSTaggedValue FastSetTypeArrayProperty(JSThread *thread, JSTaggedValue receiver, JSTaggedValue holder,
                                                         JSTaggedValue key, JSTaggedValue value, JSType jsType);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_INTERPRETER_OBJECT_OPERATOR_INL_H
