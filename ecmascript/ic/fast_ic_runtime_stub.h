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

#ifndef PANDA_RUNTIME_ECMASCRIPT_FAST_IC_RUNTIME_STUB_H
#define PANDA_RUNTIME_ECMASCRIPT_FAST_IC_RUNTIME_STUB_H

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/property_attributes.h"

namespace panda::ecmascript {
class ICRuntimeStub {
public:
    static inline JSTaggedValue LoadGlobalICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                   JSTaggedValue globalValue, JSTaggedValue key, uint32_t slotId);
    static inline JSTaggedValue StoreGlobalICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                                    JSTaggedValue globalValue, JSTaggedValue key,
                                                    JSTaggedValue value, uint32_t slotId);
    static inline JSTaggedValue LoadICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                             JSTaggedValue receiver, JSTaggedValue key, uint32_t slotId);
    static inline JSTaggedValue StoreICByName(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                              JSTaggedValue receiver, JSTaggedValue key,
                                              JSTaggedValue value, uint32_t slotId);
    static inline JSTaggedValue CheckPolyHClass(JSTaggedValue cachedValue, JSHClass* hclass);
    static inline JSTaggedValue LoadICWithHandler(JSThread *thread, JSTaggedValue receiver, JSTaggedValue holder,
                                                  JSTaggedValue handler);
    static inline JSTaggedValue StoreICWithHandler(JSThread *thread, JSTaggedValue receiver, JSTaggedValue holder,
                                                   JSTaggedValue value, JSTaggedValue handler);
    static inline void StoreWithTransition(JSThread *thread, JSObject *receiver, JSTaggedValue value,
                                           JSTaggedValue handler);
    static inline JSTaggedValue StorePrototype(JSThread *thread, JSTaggedValue receiver,
                                               JSTaggedValue value, JSTaggedValue handler);
    static inline JSTaggedValue LoadFromField(JSObject *receiver, uint32_t handlerInfo);
    static inline void StoreField(JSThread *thread, JSObject *receiver, JSTaggedValue value, uint32_t handler);
    static inline JSTaggedValue LoadGlobal(JSTaggedValue handler);
    static inline JSTaggedValue StoreGlobal(JSThread *thread, JSTaggedValue value, JSTaggedValue handler);
    static inline JSTaggedValue LoadPrototype(JSThread *thread, JSTaggedValue receiver, JSTaggedValue handler);

    static inline JSTaggedValue LoadICByValue(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                              JSTaggedValue receiver, JSTaggedValue key, uint32_t slotId);
    static inline JSTaggedValue StoreICByValue(JSThread *thread, ProfileTypeInfo *profileTypeInfo,
                                               JSTaggedValue receiver, JSTaggedValue key, JSTaggedValue value,
                                               uint32_t slotId);
    static inline JSTaggedValue LoadElement(JSObject *receiver, JSTaggedValue key);
    static inline JSTaggedValue StoreElement(JSThread *thread, JSObject *receiver, JSTaggedValue key,
                                             JSTaggedValue value, uint32_t handlerInfo);
    static inline int32_t TryToElementsIndex(JSTaggedValue key);
};
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_FAST_IC_RUNTIME_STUB_H
