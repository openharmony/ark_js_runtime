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

#include "ecmascript/js_array.h"
#include "ecmascript/accessor_data.h"
#include "ecmascript/base/array_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory.h"
#include "interpreter/fast_runtime_stub-inl.h"

namespace panda::ecmascript {
JSTaggedValue JSArray::LengthGetter([[maybe_unused]] JSThread *thread, const JSHandle<JSObject> &self)
{
    return JSArray::Cast(*self)->GetLength();
}

bool JSArray::LengthSetter(JSThread *thread, const JSHandle<JSObject> &self, const JSHandle<JSTaggedValue> &value,
                           bool mayThrow)
{
    uint32_t newLen = 0;
    if (!JSTaggedValue::ToArrayLength(thread, value, &newLen) && mayThrow) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "array length must less than 2^32 - 1", false);
    }

    if (!IsArrayLengthWritable(thread, self)) {
        if (mayThrow) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot assign to read only property", false);
        }
        return false;
    }

    uint32_t oldLen = JSArray::Cast(*self)->GetArrayLength();
    JSArray::SetCapacity(thread, self, oldLen, newLen);
    return true;
}

JSHandle<JSTaggedValue> JSArray::ArrayCreate(JSThread *thread, JSTaggedNumber length)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> arrayFunction = env->GetArrayFunction();
    return JSArray::ArrayCreate(thread, length, arrayFunction);
}

// 9.4.2.2 ArrayCreate(length, proto)
JSHandle<JSTaggedValue> JSArray::ArrayCreate(JSThread *thread, JSTaggedNumber length,
                                             const JSHandle<JSTaggedValue> &newTarget)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // Assert: length is an integer Number ≥ 0.
    ASSERT_PRINT(length.IsInteger() && length.GetNumber() >= 0, "length must be positive integer");
    // 2. If length is −0, let length be +0.
    double arrayLength = JSTaggedValue::ToInteger(thread, JSHandle<JSTaggedValue>(thread, length)).GetDouble();
    if (arrayLength > MAX_ARRAY_INDEX) {
        JSHandle<JSTaggedValue> exception(thread, JSTaggedValue::Exception());
        THROW_RANGE_ERROR_AND_RETURN(thread, "array length must less than 2^32 - 1", exception);
    }
    uint32_t normalArrayLength = length.ToUint32();

    // 8. Set the [[Prototype]] internal slot of A to proto.
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> arrayFunc = env->GetArrayFunction();
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(arrayFunc), newTarget);
    // 9. Set the [[Extensible]] internal slot of A to true.
    obj->GetJSHClass()->SetExtensible(true);

    // 10. Perform OrdinaryDefineOwnProperty(A, "length", PropertyDescriptor{[[Value]]: length, [[Writable]]:
    // true, [[Enumerable]]: false, [[Configurable]]: false}).
    JSArray::Cast(*obj)->SetArrayLength(thread, normalArrayLength);

    return JSHandle<JSTaggedValue>(obj);
}

// 9.4.2.3 ArraySpeciesCreate(originalArray, length)
JSTaggedValue JSArray::ArraySpeciesCreate(JSThread *thread, const JSHandle<JSObject> &originalArray,
                                          JSTaggedNumber length)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // Assert: length is an integer Number ≥ 0.
    ASSERT_PRINT(length.IsInteger() && length.GetNumber() >= 0, "length must be positive integer");
    // If length is −0, let length be +0.
    double arrayLength = JSTaggedValue::ToInteger(thread, JSHandle<JSTaggedValue>(thread, length)).GetDouble();
    if (arrayLength == -0) {
        arrayLength = +0;
    }
    // Let C be undefined.
    // Let isArray be IsArray(originalArray).
    JSHandle<JSTaggedValue> originalValue(originalArray);
    bool isArray = originalValue->IsArray(thread);
    // ReturnIfAbrupt(isArray).
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    // If isArray is true, then
    JSHandle<JSTaggedValue> constructor(thread, JSTaggedValue::Undefined());
    if (isArray) {
        // Let C be Get(originalArray, "constructor").
        auto *hclass = originalArray->GetJSHClass();
        if (hclass->IsJSArray() && !hclass->HasConstructor()) {
            return JSArray::ArrayCreate(thread, length).GetTaggedValue();
        }
        JSHandle<JSTaggedValue> constructorKey = globalConst->GetHandledConstructorString();
        constructor = JSTaggedValue::GetProperty(thread, originalValue, constructorKey).GetValue();
        // ReturnIfAbrupt(C).
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
        // If IsConstructor(C) is true, then
        if (constructor->IsConstructor()) {
            // Let thisRealm be the running execution context’s Realm.
            // Let realmC be GetFunctionRealm(C).
            JSHandle<GlobalEnv> realmC = JSObject::GetFunctionRealm(thread, constructor);
            // ReturnIfAbrupt(realmC).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // If thisRealm and realmC are not the same Realm Record, then
            if (*realmC != *env) {
                JSTaggedValue realmArrayConstructor = realmC->GetArrayFunction().GetTaggedValue();
                // If SameValue(C, realmC.[[intrinsics]].[[%Array%]]) is true, let C be undefined.
                if (JSTaggedValue::SameValue(constructor.GetTaggedValue(), realmArrayConstructor)) {
                    return JSArray::ArrayCreate(thread, length).GetTaggedValue();
                }
            }
        }

        // If Type(C) is Object, then
        if (constructor->IsECMAObject()) {
            // Let C be Get(C, @@species).
            JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
            constructor = JSTaggedValue::GetProperty(thread, constructor, speciesSymbol).GetValue();
            // ReturnIfAbrupt(C).
            RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
            // If C is null, let C be undefined.
            if (constructor->IsNull()) {
                return JSArray::ArrayCreate(thread, length).GetTaggedValue();
            }
        }
    }

    // If C is undefined, return ArrayCreate(length).
    if (constructor->IsUndefined()) {
        return JSArray::ArrayCreate(thread, length).GetTaggedValue();
    }
    // If IsConstructor(C) is false, throw a TypeError exception.
    if (!constructor->IsConstructor()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Not a constructor", JSTaggedValue::Exception());
    }
    // Return Construct(C, «length»).
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info =
        EcmaInterpreter::NewRuntimeCallInfo(thread, constructor, undefined, undefined, 1);
    info.SetCallArg(JSTaggedValue(arrayLength));
    JSTaggedValue result = JSFunction::Construct(&info);

    // NOTEIf originalArray was created using the standard built-in Array constructor for
    // a Realm that is not the Realm of the running execution context, then a new Array is
    // created using the Realm of the running execution context. This maintains compatibility
    // with Web browsers that have historically had that behaviour for the Array.prototype methods
    // that now are defined using ArraySpeciesCreate.
    return result;
}

void JSArray::SetCapacity(JSThread *thread, const JSHandle<JSObject> &array, uint32_t oldLen, uint32_t newLen)
{
    TaggedArray *element = TaggedArray::Cast(array->GetElements().GetTaggedObject());

    if (element->IsDictionaryMode()) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        int32_t numOfElements = array->GetNumberOfElements();
        uint32_t newNumOfElements = newLen;
        if (newLen < oldLen && numOfElements != 0) {
            JSHandle<NumberDictionary> dictHandle(thread, element);
            JSHandle<TaggedArray> newArr = factory->NewTaggedArray(numOfElements);
            GetAllElementKeys(thread, array, 0, newArr);
            for (uint32_t i = numOfElements - 1; i >= newLen; i--) {
                JSTaggedValue value = newArr->Get(i);
                uint32_t output = 0;
                JSTaggedValue::StringToElementIndex(value, &output);
                JSTaggedValue key(static_cast<int>(output));
                int entry = dictHandle->FindEntry(key);
                uint32_t attr = dictHandle->GetAttributes(entry).GetValue();
                PropertyAttributes propAttr(attr);
                if (propAttr.IsConfigurable()) {
                    JSHandle<NumberDictionary> newDict = NumberDictionary::Remove(thread, dictHandle, entry);
                    array->SetElements(thread, newDict);
                    if (i == 0) {
                        newNumOfElements = i;
                        break;
                    }
                } else {
                    newNumOfElements = i + 1;
                    break;
                }
            }
        }
        JSArray::Cast(*array)->SetArrayLength(thread, newNumOfElements);
        return;
    }
    uint32_t capacity = element->GetLength();
    if (newLen <= capacity) {
        // judge if need to cut down the array size, else fill the unused tail with holes
        array->FillElementsWithHoles(thread, newLen, oldLen < capacity ? oldLen : capacity);
    }
    if (JSObject::ShouldTransToDict(oldLen, newLen)) {
        JSObject::ElementsToDictionary(thread, array);
    } else if (newLen > capacity) {
        JSObject::GrowElementsCapacity(thread, array, newLen);
    }
    JSArray::Cast(*array)->SetArrayLength(thread, newLen);
}

bool JSArray::ArraySetLength(JSThread *thread, const JSHandle<JSObject> &array, const PropertyDescriptor &desc)
{
    JSHandle<JSTaggedValue> lengthKeyHandle(thread->GlobalConstants()->GetHandledLengthString());

    // 1. If the [[Value]] field of Desc is absent, then
    if (!desc.HasValue()) {
        // 1a. Return OrdinaryDefineOwnProperty(A, "length", Desc).
        return JSObject::OrdinaryDefineOwnProperty(thread, array, lengthKeyHandle, desc);
    }
    // 2. Let newLenDesc be a copy of Desc.
    // (Actual copying is not necessary.)
    PropertyDescriptor newLenDesc = desc;
    // 3. - 7. Convert Desc.[[Value]] to newLen.
    uint32_t newLen = 0;
    if (!JSTaggedValue::ToArrayLength(thread, desc.GetValue(), &newLen)) {
        THROW_RANGE_ERROR_AND_RETURN(thread, "array length must less than 2^32 - 1", false);
    }
    // 8. Set newLenDesc.[[Value]] to newLen.
    // (Done below, if needed.)
    // 9. Let oldLenDesc be OrdinaryGetOwnProperty(A, "length").
    PropertyDescriptor oldLenDesc(thread);
    [[maybe_unused]] bool success = GetOwnProperty(thread, array, lengthKeyHandle, oldLenDesc);
    // 10. (Assert)
    ASSERT(success);

    // 11. Let oldLen be oldLenDesc.[[Value]].
    uint32_t oldLen = 0;
    JSTaggedValue::ToArrayLength(thread, oldLenDesc.GetValue(), &oldLen);
    // 12. If newLen >= oldLen, then
    if (newLen >= oldLen) {
        // 8. Set newLenDesc.[[Value]] to newLen.
        // 12a. Return OrdinaryDefineOwnProperty(A, "length", newLenDesc).
        newLenDesc.SetValue(JSHandle<JSTaggedValue>(thread, JSTaggedValue(newLen)));
        return JSObject::OrdinaryDefineOwnProperty(thread, array, lengthKeyHandle, newLenDesc);
    }
    // 13. If oldLenDesc.[[Writable]] is false, return false.
    if (!oldLenDesc.IsWritable() ||
        // Also handle the {configurable: true} case since we later use
        // JSArray::SetLength instead of OrdinaryDefineOwnProperty to change
        // the length, and it doesn't have access to the descriptor anymore.
        newLenDesc.IsConfigurable()) {
        return false;
    }
    // 14. If newLenDesc.[[Writable]] is absent or has the value true,
    // let newWritable be true.
    bool newWritable = false;
    if (!newLenDesc.HasWritable() || newLenDesc.IsWritable()) {
        newWritable = true;
    }
    // 15. Else,
    // 15a. Need to defer setting the [[Writable]] attribute to false in case
    //      any elements cannot be deleted.
    // 15b. Let newWritable be false. (It's initialized as "false" anyway.)
    // 15c. Set newLenDesc.[[Writable]] to true.
    // (Not needed.)

    // Most of steps 16 through 19 is implemented by JSArray::SetCapacity.
    JSArray::SetCapacity(thread, array, oldLen, newLen);
    // Steps 19d-ii, 20.
    if (!newWritable) {
        PropertyDescriptor readonly(thread);
        readonly.SetWritable(false);
        success = JSObject::DefineOwnProperty(thread, array, lengthKeyHandle, readonly);
        ASSERT_PRINT(success, "DefineOwnProperty of length must be success here!");
    }

    // Steps 19d-v, 21. Return false if there were non-deletable elements.
    uint32_t arrayLength = JSArray::Cast(*array)->GetArrayLength();
    return arrayLength == newLen;
}

bool JSArray::PropertyKeyToArrayIndex(JSThread *thread, const JSHandle<JSTaggedValue> &key, uint32_t *output)
{
    return JSTaggedValue::ToArrayLength(thread, key, output) && *output <= JSArray::MAX_ARRAY_INDEX;
}

// 9.4.2.1 [[DefineOwnProperty]] ( P, Desc)
bool JSArray::DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &array, const JSHandle<JSTaggedValue> &key,
                                const PropertyDescriptor &desc)
{
    // 1. Assert: IsPropertyKey(P) is true.
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key!");
    // 2. If P is "length", then
    if (IsLengthString(thread, key)) {
        // a. Return ArraySetLength(A, Desc).
        return ArraySetLength(thread, array, desc);
    }
    // 3. Else if P is an array index, then
    // already do in step 4.
    // 4. Return OrdinaryDefineOwnProperty(A, P, Desc).
    bool success = JSObject::OrdinaryDefineOwnProperty(thread, array, key, desc);
    if (success) {
        JSTaggedValue constructorKey = thread->GlobalConstants()->GetConstructorString();
        if (key.GetTaggedValue() == constructorKey) {
            array->GetJSHClass()->SetHasConstructor(true);
            return true;
        }
    }
    return success;
}

bool JSArray::DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &array, uint32_t index,
                                const PropertyDescriptor &desc)
{
    return JSObject::OrdinaryDefineOwnProperty(thread, array, index, desc);
}

bool JSArray::IsLengthString(JSThread *thread, const JSHandle<JSTaggedValue> &key)
{
    return key.GetTaggedValue() == thread->GlobalConstants()->GetLengthString();
}

// ecma6 7.3 Operations on Objects
JSHandle<JSArray> JSArray::CreateArrayFromList(JSThread *thread, const JSHandle<TaggedArray> &elements)
{
    // Assert: elements is a List whose elements are all ECMAScript language values.
    // 2. Let array be ArrayCreate(0) (see 9.4.2.2).
    uint32_t length = elements->GetLength();

    // 4. For each element e of elements
    auto env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> arrayFunc = env->GetArrayFunction();
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(arrayFunc), arrayFunc);
    obj->GetJSHClass()->SetExtensible(true);
    JSArray::Cast(*obj)->SetArrayLength(thread, length);

    obj->SetElements(thread, elements);

    return JSHandle<JSArray>(obj);
}

JSHandle<JSTaggedValue> JSArray::FastGetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                        uint32_t index)
{
    auto result = FastRuntimeStub::FastGetPropertyByIndex(thread, obj.GetTaggedValue(), index);
    return JSHandle<JSTaggedValue>(thread, result);
}

JSHandle<JSTaggedValue> JSArray::FastGetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                        const JSHandle<JSTaggedValue> &key)
{
    auto result = FastRuntimeStub::FastGetPropertyByValue(thread, obj.GetTaggedValue(), key.GetTaggedValue());
    return JSHandle<JSTaggedValue>(thread, result);
}

bool JSArray::FastSetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t index,
                                     const JSHandle<JSTaggedValue> &value)
{
    return FastRuntimeStub::FastSetPropertyByIndex(thread, obj.GetTaggedValue(), index, value.GetTaggedValue());
}

bool JSArray::FastSetPropertyByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                     const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
{
    return FastRuntimeStub::FastSetPropertyByValue(thread, obj.GetTaggedValue(), key.GetTaggedValue(),
                                                   value.GetTaggedValue());
}

void JSArray::Sort(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &fn)
{
    if (!fn->IsUndefined() && !fn->IsCallable()) {
        THROW_TYPE_ERROR(thread, "Callable is false");
    }

    // 2. Let len be ToLength(Get(obj, "length")).
    double len = base::ArrayHelper::GetArrayLength(thread, JSHandle<JSTaggedValue>(obj));
    // 3. ReturnIfAbrupt(len).
    RETURN_IF_ABRUPT_COMPLETION(thread);

    JSMutableHandle<JSTaggedValue> presentValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> middleValue(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> previousValue(thread, JSTaggedValue::Undefined());
    for (int i = 1; i < len; i++) {
        int beginIndex = 0;
        int endIndex = i;
        presentValue.Update(FastRuntimeStub::FastGetPropertyByIndex<true>(thread, obj.GetTaggedValue(), i));
        RETURN_IF_ABRUPT_COMPLETION(thread);
        while (beginIndex < endIndex) {
            int middleIndex = (beginIndex + endIndex) / 2; // 2 : half
            middleValue.Update(
                FastRuntimeStub::FastGetPropertyByIndex<true>(thread, obj.GetTaggedValue(), middleIndex));
            RETURN_IF_ABRUPT_COMPLETION(thread);
            int32_t compareResult = base::ArrayHelper::SortCompare(thread, fn, middleValue, presentValue);
            RETURN_IF_ABRUPT_COMPLETION(thread);
            if (compareResult > 0) {
                endIndex = middleIndex;
            } else {
                beginIndex = middleIndex + 1;
            }
        }

        if (endIndex >= 0 && endIndex < i) {
            for (int j = i; j > endIndex; j--) {
                previousValue.Update(
                    FastRuntimeStub::FastGetPropertyByIndex<true>(thread, obj.GetTaggedValue(), j - 1));
                RETURN_IF_ABRUPT_COMPLETION(thread);
                FastRuntimeStub::FastSetPropertyByIndex(thread, obj.GetTaggedValue(), j,
                                                        previousValue.GetTaggedValue());
                RETURN_IF_ABRUPT_COMPLETION(thread);
            }
            FastRuntimeStub::FastSetPropertyByIndex(thread, obj.GetTaggedValue(), endIndex,
                                                    presentValue.GetTaggedValue());
            RETURN_IF_ABRUPT_COMPLETION(thread);
        }
    }
}

bool JSArray::IncludeInSortedValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                   const JSHandle<JSTaggedValue> &value)
{
    ASSERT(obj->IsJSArray());
    JSHandle<JSArray> arrayObj = JSHandle<JSArray>::Cast(obj);
    int32_t length = arrayObj->GetArrayLength();
    if (length == 0) {
        return false;
    }
    int32_t left = 0;
    int32_t right = length - 1;
    while (left <= right) {
        int32_t middle = (left + right) / 2;
        JSHandle<JSTaggedValue> vv = JSArray::FastGetPropertyByValue(thread, obj, middle);
        ComparisonResult res = JSTaggedValue::Compare(thread, vv, value);
        if (res == ComparisonResult::EQUAL) {
            return true;
        } else if (res == ComparisonResult::LESS) {
            left = middle + 1;
        } else {
            right = middle - 1;
        }
    }
    return false;
}

JSHandle<TaggedArray> JSArray::ToTaggedArray(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    ASSERT(obj->IsJSArray());
    JSHandle<JSArray> arrayObj = JSHandle<JSArray>::Cast(obj);
    int32_t length = arrayObj->GetArrayLength();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(length);
    for (int32_t idx = 0; idx < length; idx++) {
        JSHandle<JSTaggedValue> vv = JSArray::FastGetPropertyByValue(thread, obj, idx);
        taggedArray->Set(thread, idx, vv);
    }
    return taggedArray;
}
}  // namespace panda::ecmascript
