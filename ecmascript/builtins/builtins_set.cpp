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

#include "builtins_set.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/js_invoker.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/linked_hash_table-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsSet::SetConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Set, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1.If NewTarget is undefined, throw a TypeError exception
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        // throw type error
        THROW_TYPE_ERROR_AND_RETURN(thread, "new target can't be undefined", JSTaggedValue::Exception());
    }
    // 2.Let set be OrdinaryCreateFromConstructor(NewTarget, "%SetPrototype%", «‍[[SetData]]» ).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    // 3.returnIfAbrupt()
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSSet> set = JSHandle<JSSet>::Cast(obj);
    // 3.ReturnIfAbrupt(set).
    // 4.Set set’s [[SetData]] internal slot to a new empty List.
    JSTaggedValue linkedSet = LinkedHashSet::Create(thread);
    set->SetLinkedSet(thread, linkedSet);

    // add data into set from iterable
    // 5.If iterable is not present, let iterable be undefined.
    // 6.If iterable is either undefined or null, let iter be undefined.
    JSHandle<JSTaggedValue> iterable(GetCallArg(argv, 0));
    // 8.If iter is undefined, return set
    if (iterable->IsUndefined() || iterable->IsNull()) {
        return set.GetTaggedValue();
    }
    // Let adder be Get(set, "add").
    JSHandle<JSTaggedValue> adderKey(factory->NewFromCanBeCompressString("add"));
    JSHandle<JSTaggedValue> setHandle(set);
    JSHandle<JSTaggedValue> adder = JSObject::GetProperty(thread, setHandle, adderKey).GetValue();
    // ReturnIfAbrupt(adder).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, adder.GetTaggedValue());
    // If IsCallable(adder) is false, throw a TypeError exception
    if (!adder->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "adder is not callable", adder.GetTaggedValue());
    }
    // Let iter be GetIterator(iterable).
    JSHandle<JSTaggedValue> iter(JSIterator::GetIterator(thread, iterable));
    // ReturnIfAbrupt(iter).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, iter.GetTaggedValue());
    // values in iterator_result may be a JSArray, values[0] = key values[1]=value, used valueIndex to get value from
    // jsarray
    JSHandle<JSTaggedValue> valueIndex(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> next = JSIterator::IteratorStep(thread, iter);
    InternalCallParams *arguments = thread->GetInternalCallParams();
    while (!next->IsFalse()) {
        // ReturnIfAbrupt(next).
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, next.GetTaggedValue());
        // Let nextValue be IteratorValue(next).
        JSHandle<JSTaggedValue> nextValue(JSIterator::IteratorValue(thread, next));
        // ReturnIfAbrupt(nextValue).
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, nextValue.GetTaggedValue());
        arguments->MakeArgv(nextValue);
        if (nextValue->IsArray(thread)) {
            auto prop = JSObject::GetProperty(thread, nextValue, valueIndex).GetValue();
            arguments->MakeArgv(prop);
        }
        JSTaggedValue ret = JSFunction::Call(thread, adder, JSHandle<JSTaggedValue>(set), 1, arguments->GetArgv());
        // Let status be Call(adder, set, «nextValue.[[value]]»).
        JSHandle<JSTaggedValue> status(thread, ret);

        if (thread->HasPendingException()) {
            return JSIterator::IteratorCloseAndReturn(thread, iter, status);
        }
        // Let next be IteratorStep(iter).
        next = JSIterator::IteratorStep(thread, iter);
    }
    return set.GetTaggedValue();
}

JSTaggedValue BuiltinsSet::Add(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Set, Add);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    // 2.If Type(S) is not Object, throw a TypeError exception.
    // 3.If S does not have a [[SetData]] internal slot, throw a TypeError exception.
    if (!self->IsJSSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSSet", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    JSHandle<JSSet> set(thread, JSSet::Cast(*JSTaggedValue::ToObject(thread, self)));

    JSSet::Add(thread, set, value);
    return set.GetTaggedValue();
}

JSTaggedValue BuiltinsSet::Clear(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Set, Clear);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    // 2.If Type(S) is not Object, throw a TypeError exception.
    // 3.If S does not have a [[SetData]] internal slot, throw a TypeError exception.
    if (!self->IsJSSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSSet", JSTaggedValue::Exception());
    }
    JSHandle<JSSet> set(thread, JSSet::Cast(*JSTaggedValue::ToObject(thread, self)));
    JSSet::Clear(thread, set);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsSet::Delete(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Set, Delete);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    // 2.If Type(S) is not Object, throw a TypeError exception.
    // 3.If S does not have a [[SetData]] internal slot, throw a TypeError exception.
    if (!self->IsJSSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSSet", JSTaggedValue::Exception());
    }

    JSHandle<JSSet> set(thread, JSSet::Cast(*JSTaggedValue::ToObject(thread, self)));
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    bool flag = JSSet::Delete(thread, set, value);
    return GetTaggedBoolean(flag);
}

JSTaggedValue BuiltinsSet::Has(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Set, Has);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    // 2.If Type(S) is not Object, throw a TypeError exception.
    // 3.If S does not have a [[SetData]] internal slot, throw a TypeError exception.
    if (!self->IsJSSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSSet", JSTaggedValue::Exception());
    }
    JSSet *jsSet = JSSet::Cast(*JSTaggedValue::ToObject(thread, self));
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    bool flag = jsSet->Has(value.GetTaggedValue());
    return GetTaggedBoolean(flag);
}

JSTaggedValue BuiltinsSet::ForEach([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> self = GetThis(argv);
    // 2.If Type(S) is not Object, throw a TypeError exception.
    // 3.If S does not have a [[SetData]] internal slot, throw a TypeError exception.
    if (!self->IsJSSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSSet", JSTaggedValue::Exception());
    }
    JSHandle<JSSet> set(thread, JSSet::Cast(*JSTaggedValue::ToObject(thread, self)));

    // 4.If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> func(GetCallArg(argv, 0));
    if (!func->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "callbackfn is not callable", JSTaggedValue::Exception());
    }

    // 5.If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArg = GetCallArg(argv, 1);

    // composed arguments
    JSHandle<JSTaggedValue> iter(factory->NewJSSetIterator(set, IterationKind::KEY));
    JSHandle<JSTaggedValue> result = JSIterator::IteratorStep(thread, iter);
    InternalCallParams *arguments = thread->GetInternalCallParams();
    while (!result->IsFalse()) {
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, result.GetTaggedValue());
        JSHandle<JSTaggedValue> value = JSIterator::IteratorValue(thread, result);
        // Let funcResult be Call(callbackfn, T, «e, e, S»).
        arguments->MakeArgv(value, value, JSHandle<JSTaggedValue>(set));
        JSTaggedValue ret = JSFunction::Call(thread, func, thisArg, 3, arguments->GetArgv());  // 3: three args
        // returnIfAbrupt
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ret);
        result = JSIterator::IteratorStep(thread, iter);
    }
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsSet::Species([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    return GetThis(argv).GetTaggedValue();
}

JSTaggedValue BuiltinsSet::GetSize(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Set, GetSize);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self(GetThis(argv));
    // 2.If Type(S) is not Object, throw a TypeError exception.
    // 3.If S does not have a [[SetData]] internal slot, throw a TypeError exception.
    if (!self->IsJSSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSSet", JSTaggedValue::Exception());
    }
    JSSet *jsSet = JSSet::Cast(*JSTaggedValue::ToObject(thread, self));
    int count = jsSet->GetSize();
    return JSTaggedValue(count);
}

JSTaggedValue BuiltinsSet::Entries(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Set, Entries);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter = JSSetIterator::CreateSetIterator(thread, self, IterationKind::KEY_AND_VALUE);
    return iter.GetTaggedValue();
}

JSTaggedValue BuiltinsSet::Values(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), Set, Values);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter = JSSetIterator::CreateSetIterator(thread, self, IterationKind::VALUE);
    return iter.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins
