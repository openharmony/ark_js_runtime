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

#include "containers_arraylist.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_arraylist.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersArrayList::ArrayListConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "new target can't be undefined", JSTaggedValue::Exception());
    }
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);

    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    return obj.GetTaggedValue();
}

JSTaggedValue ContainersArrayList::Add(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Add);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSArrayList()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSArrayList", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    JSArrayList::Add(thread, JSHandle<JSArrayList>::Cast(self), value);

    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::False());
    return JSTaggedValue::True();
}

JSTaggedValue ContainersArrayList::Iterator(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), ArrayList, Iterator);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    return JSTaggedValue::Undefined();
}
}  // namespace panda::ecmascript::containers
