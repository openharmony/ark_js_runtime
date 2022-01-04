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
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
class GlobalEnv;
class PropertyAttributes;

class FastRuntimeStub {
public:
    /* -------------- Common API Begin, Don't change those interface!!! ----------------- */
    static inline JSTaggedValue FastAdd(JSTaggedValue left, JSTaggedValue right);
    static inline JSTaggedValue FastSub(JSTaggedValue left, JSTaggedValue right);
    static inline JSTaggedValue FastMul(JSTaggedValue left, JSTaggedValue right);
    static inline JSTaggedValue FastDiv(JSTaggedValue left, JSTaggedValue right);
    static inline JSTaggedValue FastMod(JSTaggedValue left, JSTaggedValue right);
    static inline JSTaggedValue FastEqual(JSTaggedValue left, JSTaggedValue right);
    static inline JSTaggedValue FastTypeOf(JSThread *thread, JSTaggedValue obj);
    static inline bool FastStrictEqual(JSTaggedValue left, JSTaggedValue right);
    static inline JSTaggedValue NewLexicalEnvDyn(JSThread *thread, ObjectFactory *factory, uint16_t numVars);
    static inline JSTaggedValue GetGlobalOwnProperty(JSTaggedValue receiver, JSTaggedValue key, bool *found);
    /* -------------- Special API For Multi-Language VM Begin ----------------- */
    static inline bool IsSpecialIndexedObjForGet(JSTaggedValue obj);
    static inline bool IsSpecialIndexedObjForSet(JSTaggedValue obj);
    static inline JSTaggedValue GetElement(JSTaggedValue receiver, uint32_t index);
    static inline JSTaggedValue GetElementWithArray(JSTaggedValue receiver, uint32_t index);
    static inline bool SetElement(JSThread *thread, JSTaggedValue receiver, uint32_t index, JSTaggedValue value,
                                  bool mayThrow);
    static inline bool SetPropertyByName(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                         JSTaggedValue value, bool mayThrow);
    static inline bool SetGlobalOwnProperty(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                            JSTaggedValue value, bool mayThrow);

    // set property that is not accessor and is writable
    static inline void SetOwnPropertyByName(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                            JSTaggedValue value);
    // set element that is not accessor and is writable
    static inline bool SetOwnElement(JSThread *thread, JSTaggedValue receiver, uint32_t index, JSTaggedValue value);
    static inline bool FastSetProperty(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key, JSTaggedValue value,
                                       bool mayThrow);
    static inline JSTaggedValue FastGetProperty(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key);
    static inline JSTaggedValue FindOwnProperty(JSThread *thread, JSObject *obj, TaggedArray *properties,
                                                JSTaggedValue key, PropertyAttributes *attr, uint32_t *indexOrEntry);
    static inline JSTaggedValue FindOwnElement(TaggedArray *elements, uint32_t index, bool isDict,
                                               PropertyAttributes *attr, uint32_t *indexOrEntry);
    static inline JSTaggedValue FindOwnProperty(JSThread *thread, JSObject *obj, JSTaggedValue key);

    static inline JSTaggedValue FindOwnElement(JSObject *obj, uint32_t index);

    static inline JSTaggedValue HasOwnProperty(JSThread *thread, JSObject *obj, JSTaggedValue key);
    /* -------------- Special API For Multi-Language VM End ----------------- */
    /* -------------- Common API End, Don't change those interface!!! ----------------- */

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

private:
    friend class ICRuntimeStub;
    static inline bool IsSpecialIndexedObj(JSType jsType);
    static inline bool IsSpecialReceiverObj(JSType jsType);
    static inline bool IsSpecialContainer(JSType jsType);
    static inline int32_t TryToElementsIndex(JSTaggedValue key);
    static inline JSTaggedValue CallGetter(JSThread *thread, JSTaggedValue receiver, JSTaggedValue holder,
                                           JSTaggedValue value);
    static inline JSTaggedValue CallSetter(JSThread *thread, JSTaggedValue receiver, JSTaggedValue value,
                                           JSTaggedValue accessorValue);
    static inline bool ShouldCallSetter(JSTaggedValue receiver, JSTaggedValue holder, JSTaggedValue accessorValue,
                                        PropertyAttributes attr);
    static inline JSTaggedValue AddPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                                   JSTaggedValue value);
    static inline JSTaggedValue GetContainerProperty(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                                     JSType jsType);
    static inline JSTaggedValue SetContainerProperty(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                                     JSTaggedValue value, JSType jsType);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_INTERPRETER_OBJECT_OPERATOR_INL_H
