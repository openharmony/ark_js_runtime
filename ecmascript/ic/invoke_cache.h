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

#ifndef ECMASCRIPT_IC_INVOKE_CACHE_H_
#define ECMASCRIPT_IC_INVOKE_CACHE_H_

#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
class InvokeCache {
public:
    static bool SetMonoConstuctCacheSlot(JSThread *thread, ProfileTypeInfo *profileTypeInfo, uint32_t slotId,
                                         JSTaggedValue newTarget, JSTaggedValue initialHClass);

    static bool SetPolyConstuctCacheSlot(JSThread *thread, ProfileTypeInfo *profileTypeInfo, uint32_t slotId,
                                         uint8_t length, JSTaggedValue newTargetArray,
                                         JSTaggedValue initialHClassArray);

    static JSTaggedValue CheckPolyInvokeCache(JSTaggedValue cachedArray, JSTaggedValue func);

    static JSTaggedValue Construct(JSThread *thread, JSTaggedValue firstValue, JSTaggedValue secondValue,
                                   JSTaggedValue ctor, JSTaggedValue newTarget, uint16_t firstArgIdx,
                                   uint16_t length);

    static bool SetMonoInlineCallCacheSlot(JSThread *thread, ProfileTypeInfo *profileTypeInfo, uint32_t slotId,
                                           JSTaggedValue callee);

    static bool SetPolyInlineCallCacheSlot(JSThread *thread, ProfileTypeInfo *profileTypeInfo, uint32_t slotId,
                                           uint8_t length, JSTaggedValue calleeArray);

    static bool DecideCanBeInlined(JSMethod *method);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_IC_INVOKE_CACHE_H_
