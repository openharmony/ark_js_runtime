/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_LIGHTWEIGHTMAP_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_LIGHTWEIGHTMAP_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

namespace panda::ecmascript::containers {
class ContainersLightWeightMap : public base::BuiltinsBase {
public:
    static JSTaggedValue LightWeightMapConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Length(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue HasAll(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue HasKey(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue HasValue(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue IncreaseCapacityTo(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Entries(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Get(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIndexOfKey(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIndexOfValue(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue IsEmpty(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetKeyAt(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Keys(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue SetAll(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Set(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Remove(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue RemoveAt(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Clear(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue SetValueAt(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetValueAt(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Values(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIteratorObj(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::containers
#endif  // ECMASCRIPT_CONTAINERS_CONTAINERS_LIGHTWEIGHTMAP_H
