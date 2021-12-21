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

#include "builtins_weak_set.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/js_invoker.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/linked_hash_table-inl.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsWeakSet::WeakSetConstructor(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), WeakSet, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1.If NewTarget is undefined, throw a TypeError exception
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        // throw type error
        THROW_TYPE_ERROR_AND_RETURN(thread, "new target can't be undefined", JSTaggedValue::Exception());
    }
    // 2.Let weakset be OrdinaryCreateFromConstructor(NewTarget, "%WeakSetPrototype%", «‍[[WeakSetData]]» ).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    // 3.returnIfAbrupt()
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSWeakSet> weakSet = JSHandle<JSWeakSet>::Cast(obj);
    // 3.ReturnIfAbrupt(weakSet).
    // 4.WeakSet set’s [[WeakSetData]] internal slot to a new empty List.
    JSHandle<LinkedHashSet> linkedSet = LinkedHashSet::Create(thread);
    weakSet->SetLinkedSet(thread, linkedSet);

    // add data into weakset from iterable
    // 5.If iterable is not present, let iterable be undefined.
    // 6.If iterable is either undefined or null, let iter be undefined.
    JSHandle<JSTaggedValue> iterable(GetCallArg(argv, 0));
    // 8.If iter is undefined, return weakset
    if (iterable->IsUndefined() || iterable->IsNull()) {
        return weakSet.GetTaggedValue();
    }
    // Let adder be Get(weakset, "add").
    JSHandle<JSTaggedValue> adderKey(factory->NewFromCanBeCompressString("add"));
    JSHandle<JSTaggedValue> weakSetHandle(weakSet);
    JSHandle<JSTaggedValue> adder = JSObject::GetProperty(thread, weakSetHandle, adderKey).GetValue();
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
    JSMutableHandle<JSTaggedValue> status(thread, JSTaggedValue::Undefined());
    while (!next->IsFalse()) {
        // ReturnIfAbrupt(next).
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, next.GetTaggedValue());
        // Let nextValue be IteratorValue(next).
        JSHandle<JSTaggedValue> nextValue(JSIterator::IteratorValue(thread, next));
        // ReturnIfAbrupt(nextValue).
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, nextValue.GetTaggedValue());
        InternalCallParams *arguments = thread->GetInternalCallParams();
        arguments->MakeArgv(nextValue);
        if (nextValue->IsArray(thread)) {
            auto prop = JSObject::GetProperty(thread, nextValue, valueIndex).GetValue();
            arguments->MakeArgv(prop);
        }
        JSTaggedValue ret = JSFunction::Call(thread, adder, JSHandle<JSTaggedValue>(weakSet), 1, arguments->GetArgv());

        // Let status be Call(adder, weakset, «nextValue.[[value]]»).
        status.Update(ret);
        // If status is an abrupt completion, return IteratorClose(iter, status).
        if (thread->HasPendingException()) {
            return JSIterator::IteratorCloseAndReturn(thread, iter, status);
        }
        // Let next be IteratorStep(iter).
        next = JSIterator::IteratorStep(thread, iter);
    }
    return weakSet.GetTaggedValue();
}

JSTaggedValue BuiltinsWeakSet::Add(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), WeakSet, Add);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    // 2.If Type(S) is not Object, throw a TypeError exception.
    // 3.If S does not have a [[WeakSetData]] internal slot, throw a TypeError exception.
    if (!self->IsJSWeakSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSWeakSet", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> value(GetCallArg(argv, 0));
    if (!value->IsHeapObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "value is not an object", JSTaggedValue::Exception());
    }
    if (value->IsSymbol() || value->IsString()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "value is Symblol or String", JSTaggedValue::Exception());
    }

    JSHandle<JSWeakSet> weakSet(thread, JSWeakSet::Cast(*JSTaggedValue::ToObject(thread, self)));

    JSWeakSet::Add(thread, weakSet, value);
    return weakSet.GetTaggedValue();
}

JSTaggedValue BuiltinsWeakSet::Delete(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), WeakSet, Delete);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    // 2.If Type(S) is not Object, throw a TypeError exception.
    // 3.If S does not have a [[WeakSetData]] internal slot, throw a TypeError exception.
    if (!self->IsJSWeakSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSWeakSet", JSTaggedValue::Exception());
    }

    JSHandle<JSWeakSet> weakSet(thread, JSWeakSet::Cast(*JSTaggedValue::ToObject(thread, self)));
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    if (!value->IsHeapObject()) {
        GetTaggedBoolean(false);
    }
    return GetTaggedBoolean(JSWeakSet::Delete(thread, weakSet, value));
}

JSTaggedValue BuiltinsWeakSet::Has(EcmaRuntimeCallInfo *argv)
{
    ASSERT(argv);
    BUILTINS_API_TRACE(argv->GetThread(), WeakSet, Has);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    // 2.If Type(S) is not Object, throw a TypeError exception.
    // 3.If S does not have a [[SetData]] internal slot, throw a TypeError exception.
    if (!self->IsJSWeakSet()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSWeakSet", JSTaggedValue::Exception());
    }
    JSWeakSet *jsWeakSet = JSWeakSet::Cast(*JSTaggedValue::ToObject(thread, self));
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
    if (!value->IsHeapObject()) {
        GetTaggedBoolean(false);
    }
    return GetTaggedBoolean(jsWeakSet->Has(value.GetTaggedValue()));
}
}  // namespace panda::ecmascript::builtins
