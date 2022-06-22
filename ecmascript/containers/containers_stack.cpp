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

#include "containers_stack.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_api_stack.h"
#include "ecmascript/js_api_stack_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript::containers {
JSTaggedValue ContainersStack::StackConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Stack, Constructor);
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

    JSHandle<JSAPIStack> stack = JSHandle<JSAPIStack>::Cast(obj);
    stack->SetTop(-1);

    return obj.GetTaggedValue();
}

JSTaggedValue ContainersStack::IsEmpty(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Stack, IsEmpty);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIStack()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIStack", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIStack> stack = JSHandle<JSAPIStack>::Cast(self);
    bool judge = stack->Empty();

    return GetTaggedBoolean(judge);
}

JSTaggedValue ContainersStack::Push(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Stack, Push);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIStack()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIStack", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSTaggedValue jsValue = JSAPIStack::Push(thread, JSHandle<JSAPIStack>::Cast(self), value);
    return jsValue;
}

JSTaggedValue ContainersStack::Peek(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Stack, Peek);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIStack()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIStack", JSTaggedValue::Exception());
    }

    JSHandle<JSAPIStack> stack = JSHandle<JSAPIStack>::Cast(self);
    JSTaggedValue jsValue = stack->Peek();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return jsValue;
}

JSTaggedValue ContainersStack::Locate(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Stack, Locate);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIStack()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIStack", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    JSHandle<JSAPIStack> stack = JSHandle<JSAPIStack>::Cast(self);
    int num = stack->Search(value);
    return JSTaggedValue(num);
}

JSTaggedValue ContainersStack::Pop(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Stack, Pop);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIStack()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIStack", JSTaggedValue::Exception());
    }

    JSHandle<JSAPIStack> stack = JSHandle<JSAPIStack>::Cast(self);
    JSTaggedValue jsValue = stack->Pop();
    return jsValue;
}

JSTaggedValue ContainersStack::ForEach(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Stack, ForEach);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> thisHandle = GetThis(argv);
    JSHandle<JSAPIStack> stack = JSHandle<JSAPIStack>::Cast(GetThis(argv));
    if (!thisHandle->IsJSAPIStack()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIStack", JSTaggedValue::Exception());
    }
    
    int len = stack->GetSize();
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);

    JSHandle<JSTaggedValue> callbackFnHandle = GetCallArg(argv, 0);
    if (!callbackFnHandle->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "the callbackfun is not callable.", JSTaggedValue::Exception());
    }
    
    JSHandle<JSTaggedValue> thisArgHandle = GetCallArg(argv, 1);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    int k = 0;
    while (k < len + 1) {
        JSTaggedValue kValue = stack->Get(k);
        EcmaRuntimeCallInfo info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, callbackFnHandle, thisArgHandle, undefined, 3); // 3:three args
        info.SetCallArg(kValue, JSTaggedValue(k), thisHandle.GetTaggedValue());
        JSTaggedValue funcResult = JSFunction::Call(&info);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, funcResult);
        k++;
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue ContainersStack::Iterator(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Stack, Iterator);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> self = GetThis(argv);
    if (!self->IsJSAPIStack()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIStack", JSTaggedValue::Exception());
    }
    JSHandle<JSAPIStackIterator> iter(factory->NewJSAPIStackIterator(JSHandle<JSAPIStack>::Cast(self)));
    return iter.GetTaggedValue();
}

JSTaggedValue ContainersStack::GetLength(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv != nullptr);
    BUILTINS_API_TRACE(argv->GetThread(), Stack, GetLength);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> thisHandle = GetThis(argv); // JSAPIStack
    JSHandle<JSTaggedValue> self = GetThis(argv);

    if (!self->IsJSAPIStack()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSAPIStack", JSTaggedValue::Exception());
    }

    int len = (JSHandle<JSAPIStack>::Cast(thisHandle))->GetSize();
    return JSTaggedValue(len + 1);
}
}  // namespace panda::ecmascript::containers
