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

#include "ecmascript/ic/invoke_cache.h"
#include "ecmascript/interpreter/interpreter-inl.h"

namespace panda::ecmascript {
// Build the infrastructure and wait for TS to invoke.
bool InvokeCache::SetMonoConstuctCacheSlot(JSThread *thread, ProfileTypeInfo *profileTypeInfo, uint32_t slotId,
                                           JSTaggedValue newTarget, JSTaggedValue initialHClass)
{
    // only cache class constructor
    if (UNLIKELY(!newTarget.IsClassConstructor())) {
        return false;
    }

    profileTypeInfo->Set(thread, slotId, newTarget);
    profileTypeInfo->Set(thread, slotId + 1, initialHClass);

    return true;
}

bool InvokeCache::SetPolyConstuctCacheSlot(JSThread *thread, ProfileTypeInfo *profileTypeInfo, uint32_t slotId,
                                           uint8_t length, JSTaggedValue newTargetArray,
                                           JSTaggedValue initialHClassArray)
{
    ASSERT(length <= 4 && newTargetArray.IsTaggedArray() && initialHClassArray.IsTaggedArray());

    JSHandle<TaggedArray> profileTypeInfoArr(thread, profileTypeInfo);
    JSHandle<TaggedArray> newTargetArr(thread, newTargetArray);
    JSHandle<TaggedArray> initialHClassArr(thread, initialHClassArray);

    auto factory = thread->GetEcmaVM()->GetFactory();
    constexpr uint8_t step = 2;
    JSHandle<TaggedArray> newArray = factory->NewTaggedArray(length * step);  // 2: newTarget and hclass

    for (uint8_t index = 0; index < length; ++index) {
        ASSERT(newTargetArr->Get(index).IsClassConstructor());

        newArray->Set(thread, index * step, newTargetArr->Get(index));
        newArray->Set(thread, index * step + 1, initialHClassArr->Get(index));
    }

    profileTypeInfoArr->Set(thread, slotId, newArray);
    profileTypeInfoArr->Set(thread, slotId + 1, JSTaggedValue::Hole());

    return true;
}

JSTaggedValue InvokeCache::CheckPolyInvokeCache(JSTaggedValue cachedArray, JSTaggedValue func)
{
    ASSERT(cachedArray.IsTaggedArray());
    TaggedArray *array = TaggedArray::Cast(cachedArray.GetHeapObject());
    array_size_t length = array->GetLength();
    for (array_size_t index = 0; index < length; index += 2) {  // 2: means one ic, two slot
        auto result = array->Get(index);
        if (JSFunction::Cast(result.GetTaggedObject())->GetMethod() ==
            JSFunction::Cast(func.GetTaggedObject())->GetMethod()) {
            return array->Get(index + 1);
        }
    }

    return JSTaggedValue::Hole();
}

JSTaggedValue InvokeCache::Construct(JSThread *thread, JSTaggedValue firstValue, JSTaggedValue secondValue,
                                     JSTaggedValue ctor, JSTaggedValue newTarget, uint16_t firstArgIdx, uint16_t length)
{
    // ic miss
    if (UNLIKELY(!firstValue.IsHeapObject())) {
        return JSTaggedValue::Hole();
    }

    // gc protection
    JSHandle<JSFunction> constructor(thread, ctor);
    JSHandle<JSFunction> newTgt(thread, newTarget);

    JSHandle<JSHClass> instanceHClass;
    // monomorphic
    if (LIKELY(firstValue.IsJSFunction() &&
        newTgt->GetMethod() == JSFunction::Cast(firstValue.GetTaggedObject())->GetMethod())) {
        instanceHClass = JSHandle<JSHClass>(thread, JSHClass::Cast(secondValue.GetTaggedObject()));
    } else {
        // polymorphic
        ASSERT(firstValue.IsTaggedArray());
        JSTaggedValue polyCache = CheckPolyInvokeCache(firstValue, newTarget);
        if (UNLIKELY(polyCache.IsHole())) {
            return JSTaggedValue::Hole();
        }
        instanceHClass = JSHandle<JSHClass>(thread, JSHClass::Cast(polyCache.GetTaggedObject()));
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewJSObject(instanceHClass);

    CVector<JSTaggedType> values;
    values.reserve(length);

    InterpretedFrameHandler frameHandler(thread);
    CallParams params;
    params.callTarget = ECMAObject::Cast(*constructor);
    params.newTarget = newTgt.GetTaggedType();
    params.thisArg = obj.GetTaggedType();
    params.argc = length;
    for (array_size_t i = 0; i < length; ++i) {
        JSTaggedValue value = frameHandler.GetVRegValue(firstArgIdx + i);
        values.emplace_back(value.GetRawData());
    }
    params.argv = values.data();

    EcmaInterpreter::Execute(thread, params);

    return obj.GetTaggedValue();
}

// just identify simple callee case which can be inlined, the implement of inline need wait TS AOT
bool InvokeCache::SetMonoInlineCallCacheSlot(JSThread *thread, ProfileTypeInfo *profileTypeInfo, uint32_t slotId,
                                             JSTaggedValue callee)
{
    ASSERT(callee.IsJSFunction());
    JSMethod *calleeMethod = JSFunction::Cast(callee.GetTaggedObject())->GetMethod();
    if (DecideCanBeInlined(calleeMethod)) {
        profileTypeInfo->Set(thread, slotId, callee);
        return true;
    }

    profileTypeInfo->Set(thread, slotId, JSTaggedValue::Hole());
    return false;
}

bool InvokeCache::SetPolyInlineCallCacheSlot(JSThread *thread, ProfileTypeInfo *profileTypeInfo, uint32_t slotId,
                                             uint8_t length, JSTaggedValue calleeArray)
{
    ASSERT(calleeArray.IsTaggedArray() && length >= 2 && length <= 4);
    JSHandle<TaggedArray> calleeArr(thread, calleeArray);
    ASSERT(calleeArr->GetLength() == length);
    JSHandle<TaggedArray> profileTypeInfoArr(thread, profileTypeInfo);

    auto factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> newArray = factory->NewTaggedArray(length);

    for (uint8_t index = 0; index < length; ++index) {
        JSTaggedValue calleeElement = calleeArr->Get(index);
        JSMethod *calleeMethod = JSFunction::Cast(calleeElement.GetTaggedObject())->GetMethod();
        if (DecideCanBeInlined(calleeMethod)) {
            newArray->Set(thread, index, calleeElement);
        } else {
            newArray->Set(thread, index, JSTaggedValue::Hole());
        }
    }

    profileTypeInfoArr->Set(thread, slotId, newArray);
    return true;
}

bool InvokeCache::DecideCanBeInlined(JSMethod *method)
{
    constexpr uint32_t MAX_INLINED_BYTECODE_SIZE = 128;
    uint32_t bcSize = method->GetBytecodeArraySize();
    return (bcSize > 0 && bcSize < MAX_INLINED_BYTECODE_SIZE);  // 0 is invalid
}
}  // namespace panda::ecmascript
