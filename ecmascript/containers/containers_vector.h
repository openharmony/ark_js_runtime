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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_VECTOR_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_VECTOR_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

namespace panda::ecmascript::containers {
class ContainersVector : public base::BuiltinsBase {
public:
    static JSTaggedValue VectorConstructor(EcmaRuntimeCallInfo *argv);

    static JSTaggedValue Add(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Insert(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue SetLength(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetCapacity(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue IncreaseCapacityTo(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Get(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIndexOf(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIndexFrom(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue IsEmpty(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetLastElement(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetLastIndexOf(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetLastIndexFrom(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Remove(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue RemoveByIndex(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue RemoveByRange(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Set(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue SubVector(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetSize(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ReplaceAllElements(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue TrimToCurrentLength(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Clear(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Clone(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Has(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetFirstElement(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue CopyToArray(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ConvertToArray(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Sort(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIteratorObj(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::containers
#endif  // ECMASCRIPT_CONTAINERS_CONTAINERS_VECTOR_H