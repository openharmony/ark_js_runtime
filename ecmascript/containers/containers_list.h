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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_LIST_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_LIST_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"

namespace panda::ecmascript::containers {
class ContainersList : public base::BuiltinsBase {
public:
    static JSTaggedValue ListConstructor(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Add(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetFirst(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetLast(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Insert(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Clear(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue RemoveByIndex(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Remove(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Has(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue IsEmpty(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Get(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIndexOf(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetLastIndexOf(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Set(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ForEach(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ReplaceAllElements(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetIteratorObj(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Equal(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Sort(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue ConvertToArray(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetSubList(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue Length(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::containers
#endif  // ECMASCRIPT_CONTAINERS_CONTAINERS_LIST_H
