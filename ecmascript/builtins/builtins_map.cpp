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

#include "builtins_map.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/internal_call_params.h"
#include "ecmascript/js_invoker.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::builtins {
JSTaggedValue BuiltinsMap::MapConstructor(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Map, Constructor);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // 1.If NewTarget is undefined, throw a TypeError exception
    JSHandle<JSTaggedValue> newTarget = GetNewTarget(argv);
    if (newTarget->IsUndefined()) {
        // throw type error
        THROW_TYPE_ERROR_AND_RETURN(thread, "new target can't be undefined", JSTaggedValue::Exception());
    }
    // 2.Let Map be OrdinaryCreateFromConstructor(NewTarget, "%MapPrototype%", «‍[[MapData]]» ).
    JSHandle<JSTaggedValue> constructor = GetConstructor(argv);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), newTarget);
    // 3.returnIfAbrupt()
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    JSHandle<JSMap> map = JSHandle<JSMap>::Cast(obj);

    // 4.Set map’s [[MapData]] internal slot to a new empty List.
    JSTaggedValue linkedMap = LinkedHashMap::Create(thread);
    map->SetLinkedMap(thread, linkedMap);
    // add data into set from iterable
    // 5.If iterable is not present, let iterable be undefined.
    // 6.If iterable is either undefined or null, let iter be undefined.
    JSHandle<JSTaggedValue> iterable = GetCallArg(argv, 0);
    // 8.If iter is undefined, return set
    if (iterable->IsUndefined() || iterable->IsNull()) {
        return map.GetTaggedValue();
    }
    if (!iterable->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "iterable is not object", JSTaggedValue::Exception());
    }
    // Let adder be Get(map, "set").
    JSHandle<JSTaggedValue> adderKey(factory->NewFromCanBeCompressString("set"));
    JSHandle<JSTaggedValue> adder = JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(map), adderKey).GetValue();
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
    JSHandle<JSTaggedValue> keyIndex(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> valueIndex(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> next = JSIterator::IteratorStep(thread, iter);
    JSMutableHandle<JSTaggedValue> status(thread, JSTaggedValue::Undefined());
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, next.GetTaggedValue());
    while (!next->IsFalse()) {
        // ReturnIfAbrupt(next).
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, next.GetTaggedValue());
        // Let nextValue be IteratorValue(next).
        JSHandle<JSTaggedValue> nextValue(JSIterator::IteratorValue(thread, next));
        // ReturnIfAbrupt(nextValue).
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, next.GetTaggedValue());

        // If Type(nextItem) is not Object
        if (!nextValue->IsECMAObject()) {
            JSHandle<JSObject> typeError = factory->GetJSError(ErrorType::TYPE_ERROR, "nextItem is not Object");
            JSHandle<JSTaggedValue> record(
                factory->NewCompletionRecord(CompletionRecord::THROW, JSHandle<JSTaggedValue>(typeError)));
            JSTaggedValue ret = JSIterator::IteratorClose(thread, iter, record).GetTaggedValue();
            if (!thread->HasPendingException()) {
                THROW_NEW_ERROR_AND_RETURN_VALUE(thread, typeError.GetTaggedValue(), ret);
            };
            return ret;
        }
        // Let k be Get(nextItem, "0").
        JSHandle<JSTaggedValue> key = JSObject::GetProperty(thread, nextValue, keyIndex).GetValue();
        // If k is an abrupt completion, return IteratorClose(iter, k).
        if (thread->HasPendingException()) {
            return JSIterator::IteratorCloseAndReturn(thread, iter, key);
        }
        // Let v be Get(nextItem, "1").
        JSHandle<JSTaggedValue> value = JSObject::GetProperty(thread, nextValue, valueIndex).GetValue();
        // If v is an abrupt completion, return IteratorClose(iter, v).
        if (thread->HasPendingException()) {
            return JSIterator::IteratorCloseAndReturn(thread, iter, value);
        }
        // Let status be Call(adder, map, «nextValue.[[value]]»).
        InternalCallParams *arguments = thread->GetInternalCallParams();
        arguments->MakeArgv(key, value);
        JSTaggedValue ret = JSFunction::Call(thread,
            adder, JSHandle<JSTaggedValue>(map), 2, arguments->GetArgv());  // 2: key and value pair

        status.Update(ret);
        // If status is an abrupt completion, return IteratorClose(iter, status).
        if (thread->HasPendingException()) {
            return JSIterator::IteratorCloseAndReturn(thread, iter, status);
        }
        // Let next be IteratorStep(iter).
        next = JSIterator::IteratorStep(thread, iter);
    }
    return map.GetTaggedValue();
}

JSTaggedValue BuiltinsMap::Set(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Map, Set);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);

    // 2.If Type(S) is not Object, throw a TypeError exception.
    // 3.If S does not have a [[MapData]] internal slot, throw a TypeError exception.
    if (!self->IsJSMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSMap", JSTaggedValue::Exception());
    }

    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> value = GetCallArg(argv, 1);

    JSHandle<JSMap> map(self);
    JSMap::Set(thread, map, key, value);
    return map.GetTaggedValue();
}

JSTaggedValue BuiltinsMap::Clear(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Map, Clear);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> self = GetThis(argv);

    // 2.If Type(S) is not Object, throw a TypeError exception.
    // 3.If S does not have a [[MapData]] internal slot, throw a TypeError exception.
    if (!self->IsJSMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSMap", JSTaggedValue::Exception());
    }
    JSHandle<JSMap> map(self);
    JSMap::Clear(thread, map);
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsMap::Delete(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Map, Delete);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    // 2.If Type(S) is not Object, throw a TypeError exception.
    // 3.If S does not have a [[MapData]] internal slot, throw a TypeError exception.
    if (!self->IsJSMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSMap", JSTaggedValue::Exception());
    }

    JSHandle<JSMap> map(self);
    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
    bool flag = JSMap::Delete(thread, map, key);
    return GetTaggedBoolean(flag);
}

JSTaggedValue BuiltinsMap::Has(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Map, Has);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self(GetThis(argv));
    // 2.If Type(S) is not Object, throw a TypeError exception.
    // 3.If S does not have a [[MapData]] internal slot, throw a TypeError exception.
    if (!self->IsJSMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSMap", JSTaggedValue::Exception());
    }
    JSMap *jsMap = JSMap::Cast(*JSTaggedValue::ToObject(thread, self));
    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
    bool flag = jsMap->Has(key.GetTaggedValue());
    return GetTaggedBoolean(flag);
}

JSTaggedValue BuiltinsMap::Get(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Map, Get);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self(GetThis(argv));
    // 2.If Type(S) is not Object, throw a TypeError exception.
    // 3.If S does not have a [[MapData]] internal slot, throw a TypeError exception.
    if (!self->IsJSMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSMap", JSTaggedValue::Exception());
    }
    JSMap *jsMap = JSMap::Cast(*JSTaggedValue::ToObject(thread, self));
    JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
    JSTaggedValue value = jsMap->Get(key.GetTaggedValue());
    return value;
}

JSTaggedValue BuiltinsMap::ForEach([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> self = GetThis(argv);
    // 2.If Type(S) is not Object, throw a TypeError exception.
    // 3.If S does not have a [[MapData]] internal slot, throw a TypeError exception.
    if (!self->IsJSMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSMap", JSTaggedValue::Exception());
    }
    JSHandle<JSMap> map(thread, JSMap::Cast(*JSTaggedValue::ToObject(thread, self)));

    // 4.If IsCallable(callbackfn) is false, throw a TypeError exception.
    JSHandle<JSTaggedValue> func(GetCallArg(argv, 0));
    if (!func->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not Callable", JSTaggedValue::Exception());
    }
    // 5.If thisArg was supplied, let T be thisArg; else let T be undefined.
    JSHandle<JSTaggedValue> thisArg = GetCallArg(argv, 1);

    // composed arguments
    JSHandle<JSTaggedValue> iter(factory->NewJSMapIterator(map, IterationKind::KEY_AND_VALUE));
    JSHandle<JSTaggedValue> keyIndex(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> valueIndex(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> result = JSIterator::IteratorStep(thread, iter);
    InternalCallParams *arguments = thread->GetInternalCallParams();
    while (!result->IsFalse()) {
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, result.GetTaggedValue());
        JSHandle<JSTaggedValue> iterValue(JSIterator::IteratorValue(thread, result));
        JSHandle<JSTaggedValue> key = JSObject::GetProperty(thread, iterValue, keyIndex).GetValue();
        JSHandle<JSTaggedValue> value = JSObject::GetProperty(thread, iterValue, valueIndex).GetValue();
        // Let funcResult be Call(callbackfn, T, «e, e, S»).
        arguments->MakeArgv(value, key, JSHandle<JSTaggedValue>(map));
        JSTaggedValue ret = JSFunction::Call(thread, func, thisArg, 3, arguments->GetArgv());  // 3: three args
        // returnIfAbrupt
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, ret);
        result = JSIterator::IteratorStep(thread, iter);
    }

    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsMap::Species([[maybe_unused]] EcmaRuntimeCallInfo *argv)
{
    return GetThis(argv).GetTaggedValue();
}

JSTaggedValue BuiltinsMap::GetSize(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Map, GetSize);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self(GetThis(argv));
    // 2.If Type(S) is not Object, throw a TypeError exception.
    // 3.If S does not have a [[MapData]] internal slot, throw a TypeError exception.
    if (!self->IsJSMap()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not JSMap", JSTaggedValue::Exception());
    }
    JSMap *jsMap = JSMap::Cast(*JSTaggedValue::ToObject(thread, self));
    int count = jsMap->GetSize();
    return JSTaggedValue(count);
}

JSTaggedValue BuiltinsMap::Entries(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Map, Entries);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter = JSMapIterator::CreateMapIterator(thread, self, IterationKind::KEY_AND_VALUE);
    return iter.GetTaggedValue();
}

JSTaggedValue BuiltinsMap::Keys(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Map, Keys);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter = JSMapIterator::CreateMapIterator(thread, self, IterationKind::KEY);
    return iter.GetTaggedValue();
}

JSTaggedValue BuiltinsMap::Values(EcmaRuntimeCallInfo *argv)
{
    BUILTINS_API_TRACE(argv->GetThread(), Map, Values);
    JSThread *thread = argv->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> self = GetThis(argv);
    JSHandle<JSTaggedValue> iter = JSMapIterator::CreateMapIterator(thread, self, IterationKind::VALUE);
    return iter.GetTaggedValue();
}
}  // namespace panda::ecmascript::builtins
