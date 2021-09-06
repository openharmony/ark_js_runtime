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

#ifndef PANDA_RUNTIME_ECMASCRIPT_IC_ACCESSOR_H
#define PANDA_RUNTIME_ECMASCRIPT_IC_ACCESSOR_H

#include "ecmascript/accessor_data.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
class ICAccessor {
public:
    static JSTaggedValue LoadIC(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key, uint16_t slotId);

    static JSTaggedValue LoadGlobalIC(JSThread *thread, JSTaggedValue key, uint16_t slotId);

    static JSTaggedValue LoadGlobalICByName(JSThread *thread, uint32_t stringId, uint16_t slotId);

    static JSTaggedValue LoadICByName(JSThread *thread, JSTaggedValue receiver, uint32_t stringId, uint16_t slotId);
    // dispatch
    static JSTaggedValue LoadWithHandler(JSThread *thread, JSObject *obj, JSTaggedValue receiver,
                                         JSTaggedValue handler);

    static JSTaggedValue LoadGlobalWithHandler(JSThread *thread, JSTaggedValue receiver, JSTaggedValue handler);

    static JSTaggedValue LoadProperty(JSThread *thread, JSObject *obj, JSTaggedValue receiver, JSTaggedValue handler);

    static JSTaggedValue LoadGlobal(JSThread *thread, JSTaggedValue receiver, JSTaggedValue handler);

    static JSTaggedValue LoadPrototype(JSThread *thread, JSObject *obj, JSTaggedValue handler);

    static JSTaggedValue LoadGlobalPrototype(JSThread *thread, JSTaggedValue obj, JSTaggedValue handler);

    static JSTaggedValue LoadNonExistent(JSTaggedValue handler);

    static bool StoreIC(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key, JSTaggedValue value,
                        uint16_t slotId);

    static bool StoreGlobalICByName(JSThread *thread, uint32_t stringId, JSTaggedValue value, uint16_t slotId);

    static bool StoreGlobalICByValue(JSThread *thread, JSTaggedValue key, JSTaggedValue value, uint16_t slotId);

    static bool StoreICByName(JSThread *thread, JSTaggedValue receiver, uint32_t stringId, JSTaggedValue value,
                              uint16_t slotId);

    static bool StoreWithHandler(JSThread *thread, JSObject *obj, JSTaggedValue key, JSTaggedValue value,
                                 JSTaggedValue handler, uint16_t slotId);

    static bool StoreWithHandlerByName(JSThread *thread, JSObject *obj, uint32_t stringId, JSTaggedValue value,
                                       JSTaggedValue handler, uint16_t slotId);

    static bool StoreGlobalWithHandlerByName(JSThread *thread, JSTaggedValue obj, uint32_t stringId,
                                             JSTaggedValue value, JSTaggedValue handler, uint16_t slotId);

    static bool StoreGlobalWithHandlerByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue key,
                                              JSTaggedValue value, JSTaggedValue handler, uint16_t slotId);

    static void StoreProperty(JSThread *thread, JSObject *obj, JSTaggedValue value, uint32_t handler);

    static void StoreField(const JSThread *thread, JSObject *obj, JSTaggedValue value, uint32_t handler);

    static bool StoreGlobal(JSThread *thread, JSObject *obj, JSTaggedValue key, JSTaggedValue value,
                            JSTaggedValue handler, uint16_t slotId);

    static bool StoreGlobalByName(JSThread *thread, JSObject *obj, uint32_t stringId, JSTaggedValue value,
                                  JSTaggedValue handler, uint16_t slotId);

    static bool StoreGlobalByValue(JSThread *thread, JSObject *obj, JSTaggedValue key, JSTaggedValue value,
                                   JSTaggedValue handler, uint16_t slotId);

    static void StoreWithTransition(const JSThread *thread, JSObject *obj, JSTaggedValue value, JSTaggedValue handler);

    static bool StorePrototype(JSThread *thread, JSObject *obj, JSTaggedValue key, JSTaggedValue value,
                               JSTaggedValue handler, uint16_t slotId);

    static bool StorePrototypeByName(JSThread *thread, JSObject *obj, uint32_t stringId, JSTaggedValue value,
                                     JSTaggedValue handler, uint16_t slotId);

    static bool StoreGlobalPrototypeByName(JSThread *thread, JSTaggedValue obj, uint32_t stringId, JSTaggedValue value,
                                           JSTaggedValue handler, uint16_t slotId);

    static inline bool StoreGlobalPrototypeByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue key,
                                                   JSTaggedValue value, JSTaggedValue handler, uint16_t slotId);

    static AccessorData *GetAccessor(JSThread *thread, JSObject *obj, uint32_t handler);

    static AccessorData *GetInternelAccessor(JSObject *obj, uint32_t handler);

    static JSTaggedValue LoadMiss(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                                  uint16_t slotId);

    static JSTaggedValue LoadGlobalMiss(JSThread *thread, const JSHandle<JSTaggedValue> &key, uint16_t slotId);

    static JSTaggedValue LoadGlobalMissByName(JSThread *thread, const JSHandle<JSTaggedValue> &key, uint16_t slotId);

    static bool StoreMiss(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                          const JSHandle<JSTaggedValue> &value, uint16_t slotId);

    static JSTaggedValue LoadMissByName(JSThread *thread, const JSHandle<JSObject> &obj,
                                        const JSHandle<JSTaggedValue> &key, uint16_t slotId);

    static bool StoreMissByName(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                                const JSHandle<JSTaggedValue> &value, uint16_t slotId);
    static bool StoreGlobalMissByName(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                      const JSHandle<JSTaggedValue> &value, uint16_t slotId);

    static bool StoreGlobalMissByValue(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                       const JSHandle<JSTaggedValue> &value, uint16_t slotId);
};
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_IC_ACCESSOR_H
