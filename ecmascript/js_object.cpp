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

#include "accessor_data.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_thread.h"
#include "global_dictionary-inl.h"
#include "js_array.h"
#include "js_for_in_iterator.h"
#include "js_hclass.h"
#include "js_iterator.h"
#include "object_factory.h"
#include "property_attributes.h"
#include "tagged_array-inl.h"

namespace panda::ecmascript {
PropertyAttributes::PropertyAttributes(const PropertyDescriptor &desc)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (desc.HasWritable()) {
        SetWritable(desc.IsWritable());
    }

    if (desc.HasEnumerable()) {
        SetEnumerable(desc.IsEnumerable());
    }

    if (desc.HasConfigurable()) {
        SetConfigurable(desc.IsConfigurable());
    }

    if (desc.IsAccessorDescriptor()) {
        SetIsAccessor(true);
    }
    // internal accessor
    if (desc.HasValue() && desc.GetValue()->IsAccessor()) {
        SetIsAccessor(true);
    }
}

JSMethod *ECMAObject::GetCallTarget() const
{
    const TaggedObject *obj = this;
    ASSERT(JSTaggedValue(obj).IsJSFunctionBase() || JSTaggedValue(obj).IsJSProxy());
    if (JSTaggedValue(obj).IsJSFunctionBase()) {
        return JSFunctionBase::ConstCast(obj)->GetMethod();
    }
    return JSProxy::ConstCast(obj)->GetMethod();
}

JSHandle<TaggedArray> JSObject::GrowElementsCapacity(const JSThread *thread, const JSHandle<JSObject> &obj,
                                                     uint32_t capacity)
{
    uint32_t newCapacity = ComputeElementCapacity(capacity);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> oldElements(thread, obj->GetElements());
    uint32_t oldLength = oldElements->GetLength();
    JSHandle<TaggedArray> newElements = factory->CopyArray(oldElements, oldLength, newCapacity);

    obj->SetElements(thread, newElements);
    return newElements;
}

bool JSObject::IsRegExp(JSThread *thread, const JSHandle<JSTaggedValue> &argument)
{
    if (!argument->IsECMAObject()) {
        return false;
    }
    JSHandle<JSTaggedValue> matchSymbol = thread->GetEcmaVM()->GetGlobalEnv()->GetMatchSymbol();
    JSHandle<JSTaggedValue> isRegexp = JSObject::GetProperty(thread, argument, matchSymbol).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (!isRegexp->IsUndefined()) {
        return isRegexp->ToBoolean();
    }
    JSHandle<JSObject> argumentObj = JSHandle<JSObject>::Cast(argument);
    return argumentObj->IsJSRegExp();
}

JSHandle<NameDictionary> JSObject::TransitionToDictionary(const JSThread *thread, const JSHandle<JSObject> &receiver)
{
    JSHandle<TaggedArray> array(thread, receiver->GetProperties());
    JSHandle<JSHClass> jshclass(thread, receiver->GetJSHClass());
    ASSERT(!jshclass->IsDictionaryMode());
    uint32_t propNumber = jshclass->NumberOfProps();

    ASSERT(propNumber >= 0);
    ASSERT(!jshclass->GetLayout().IsNull());
    JSHandle<LayoutInfo> layoutInfoHandle(thread, jshclass->GetLayout());
    ASSERT(layoutInfoHandle->GetLength() != 0);
    JSMutableHandle<NameDictionary> dict(
        thread, NameDictionary::Create(thread, NameDictionary::ComputeHashTableSize(propNumber)));
    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    uint32_t numberInlinedProps = jshclass->GetInlinedProperties();
    for (uint32_t i = 0; i < propNumber; i++) {
        JSTaggedValue key = layoutInfoHandle->GetKey(i);
        PropertyAttributes attr = layoutInfoHandle->GetAttr(i);
        ASSERT(i == attr.GetOffset());
        JSTaggedValue value;

        if (i < numberInlinedProps) {
            value = receiver->GetPropertyInlinedProps(i);
        } else {
            value = array->Get(i - numberInlinedProps);
        }

        attr.SetBoxType(PropertyBoxType::UNDEFINED);
        valueHandle.Update(value);
        keyHandle.Update(key);
        JSHandle<NameDictionary> newDict = NameDictionary::PutIfAbsent(thread, dict, keyHandle, valueHandle, attr);
        dict.Update(newDict);
    }

    receiver->SetProperties(thread, dict);
    // change HClass
    JSHClass::TransitionToDictionary(thread, receiver);

    // trim in-obj properties space
    if (numberInlinedProps > 0) {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        uint32_t newSize = receiver->GetClass()->GetObjectSize();
        size_t trimBytes = numberInlinedProps * JSTaggedValue::TaggedTypeSize();
        factory->FillFreeObject(ToUintPtr(*receiver) + newSize, trimBytes, RemoveSlots::YES);
    }

    return dict;
}

void JSObject::ElementsToDictionary(const JSThread *thread, JSHandle<JSObject> obj)
{
    JSHandle<TaggedArray> elements(thread, obj->GetElements());
    ASSERT(!obj->GetJSHClass()->IsDictionaryElement());
    int length = elements->GetLength();
    JSMutableHandle<NumberDictionary> dict(thread, NumberDictionary::Create(thread));
    auto attr = PropertyAttributes(PropertyAttributes::GetDefaultAttributes());
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue ::Undefined());
    for (int i = 0; i < length; i++) {
        JSTaggedValue value = elements->Get(i);
        if (value.IsHole()) {
            continue;
        }
        key.Update(JSTaggedValue(i));
        valueHandle.Update(value);
        JSHandle<NumberDictionary> newDict = NumberDictionary::PutIfAbsent(thread, dict, key, valueHandle, attr);
        dict.Update(newDict);
    }
    obj->SetElements(thread, dict);

    JSHClass::TransitionElementsToDictionary(thread, obj);
}

bool JSObject::IsArrayLengthWritable(JSThread *thread, const JSHandle<JSObject> &receiver)
{
    auto *hclass = receiver->GetJSHClass();
    if (!hclass->IsDictionaryMode()) {
        LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
        PropertyAttributes attr(layoutInfo->GetAttr(JSArray::LENGTH_INLINE_PROPERTY_INDEX));
        return attr.IsWritable();
    }
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    ObjectOperator op(thread, receiver, lengthKey, OperatorType::OWN);
    return op.GetAttr().IsWritable();
}

bool JSObject::AddElementInternal(JSThread *thread, const JSHandle<JSObject> &receiver, uint32_t index,
                                  const JSHandle<JSTaggedValue> &value, PropertyAttributes attr)
{
    bool isDictionary = receiver->GetJSHClass()->IsDictionaryElement();
    if (receiver->IsJSArray()) {
        DISALLOW_GARBAGE_COLLECTION;
        JSArray *arr = JSArray::Cast(*receiver);
        uint32_t oldLength = arr->GetArrayLength();
        if (index >= oldLength) {
            if (!IsArrayLengthWritable(thread, receiver)) {
                return false;
            }
            arr->SetArrayLength(thread, index + 1);
        }
    }
    thread->NotifyStableArrayElementsGuardians(receiver);

    TaggedArray *elements = TaggedArray::Cast(receiver->GetElements().GetTaggedObject());
    if (isDictionary) {
        ASSERT(elements->IsDictionaryMode());
        JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(static_cast<int32_t>(index)));
        JSHandle<NumberDictionary> newDict =
            NumberDictionary::Put(thread, JSHandle<NumberDictionary>(thread, elements), keyHandle, value, attr);
        receiver->SetElements(thread, newDict);
        return true;
    }

    uint32_t capacity = elements->GetLength();
    if (index >= capacity || !attr.IsDefaultAttributes()) {
        if (ShouldTransToDict(capacity, index) || !attr.IsDefaultAttributes()) {
            JSObject::ElementsToDictionary(thread, receiver);
            JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(static_cast<int32_t>(index)));
            JSHandle<NumberDictionary> dict(thread, receiver->GetElements());
            JSHandle<NumberDictionary> newKey = NumberDictionary::Put(thread, dict, keyHandle, value, attr);
            receiver->SetElements(thread, newKey);
            return true;
        }
        elements = *JSObject::GrowElementsCapacity(thread, receiver, index + 1);
    }
    elements->Set(thread, index, value);
    receiver->GetJSHClass()->UpdateRepresentation(value.GetTaggedValue());
    return true;
}

void JSObject::DeletePropertyInternal(JSThread *thread, const JSHandle<JSObject> &obj,
                                      const JSHandle<JSTaggedValue> &key, uint32_t index)
{
    JSHandle<TaggedArray> array(thread, obj->GetProperties());

    if (obj->IsJSGlobalObject()) {
        JSHandle<GlobalDictionary> dictHandle(thread, obj->GetProperties());
        JSHandle<GlobalDictionary> newDict = GlobalDictionary::Remove(thread, dictHandle, index);
        obj->SetProperties(thread, newDict);
        return;
    }

    if (!array->IsDictionaryMode()) {
        JSHandle<NameDictionary> dictHandle(TransitionToDictionary(thread, obj));
        int entry = dictHandle->FindEntry(key.GetTaggedValue());
        ASSERT(entry != -1);
        JSHandle<NameDictionary> newDict = NameDictionary::Remove(thread, dictHandle, entry);
        obj->SetProperties(thread, newDict);
        return;
    }

    JSHandle<NameDictionary> dictHandle(array);
    JSHandle<NameDictionary> newDict = NameDictionary::Remove(thread, dictHandle, index);
    obj->SetProperties(thread, newDict);
}

void JSObject::GetAllKeys(const JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                          const JSHandle<TaggedArray> &keyArray)

{
    TaggedArray *array = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
    if (!array->IsDictionaryMode()) {
        int end = obj->GetJSHClass()->NumberOfProps();
        if (end > 0) {
            LayoutInfo::Cast(obj->GetJSHClass()->GetLayout().GetTaggedObject())
                ->GetAllKeys(thread, end, offset, *keyArray);
        }
        return;
    }

    if (obj->IsJSGlobalObject()) {
        GlobalDictionary *dict = GlobalDictionary::Cast(array);
        return dict->GetAllKeys(thread, offset, *keyArray);
    }

    NameDictionary *dict = NameDictionary::Cast(obj->GetProperties().GetTaggedObject());
    dict->GetAllKeys(thread, offset, *keyArray);
}

// For Serialization use. Does not support JSGlobalObject
void JSObject::GetAllKeys(const JSThread *thread, const JSHandle<JSObject> &obj,
                          std::vector<JSTaggedValue> &keyVector)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT_PRINT(!obj->IsJSGlobalObject(), "Do not support get key of JSGlobal Object");
    TaggedArray *array = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
    if (!array->IsDictionaryMode()) {
        int end = obj->GetJSHClass()->NumberOfProps();
        if (end > 0) {
            LayoutInfo::Cast(obj->GetJSHClass()->GetLayout().GetTaggedObject())
                ->GetAllKeys(thread, end, keyVector);
        }
    } else {
        NameDictionary *dict = NameDictionary::Cast(obj->GetProperties().GetTaggedObject());
        dict->GetAllKeysIntoVector(keyVector);
    }
}

JSHandle<TaggedArray> JSObject::GetAllEnumKeys(const JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                                               uint32_t numOfKeys, uint32_t *keys)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    if (obj->IsJSGlobalObject()) {
        JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(numOfKeys);
        GlobalDictionary *dict = GlobalDictionary::Cast(obj->GetProperties().GetTaggedObject());
        dict->GetEnumAllKeys(thread, offset, *keyArray, keys);
        return keyArray;
    }

    TaggedArray *array = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
    if (!array->IsDictionaryMode()) {
        JSHClass *jsHclass = obj->GetJSHClass();
        JSTaggedValue enumCache = jsHclass->GetEnumCache();
        if (!enumCache.IsNull()) {
            auto keyArray = JSHandle<TaggedArray>(thread, enumCache);
            *keys = keyArray->GetLength();
            return keyArray;
        }
        JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(numOfKeys);
        int end = jsHclass->NumberOfProps();
        if (end > 0) {
            LayoutInfo::Cast(jsHclass->GetLayout().GetTaggedObject())
                ->GetAllEnumKeys(thread, end, offset, *keyArray, keys);
            if (*keys == keyArray->GetLength()) {
                jsHclass->SetEnumCache(thread, keyArray.GetTaggedValue());
            }
        }
        return keyArray;
    }

    JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(numOfKeys);
    NameDictionary *dict = NameDictionary::Cast(obj->GetProperties().GetTaggedObject());
    dict->GetAllEnumKeys(thread, offset, *keyArray, keys);
    return keyArray;
}

void JSObject::GetAllElementKeys(JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                                 const JSHandle<TaggedArray> &keyArray)
{
    uint32_t elementIndex = 0;

    if (obj->IsJSPrimitiveRef() && JSPrimitiveRef::Cast(*obj)->IsString()) {
        elementIndex = JSPrimitiveRef::Cast(*obj)->GetStringLength() + offset;
        for (uint32_t i = offset; i < elementIndex; ++i) {
            auto key = base::NumberHelper::NumberToString(thread, JSTaggedValue(i));
            keyArray->Set(thread, i, key);
        }
    }

    JSHandle<TaggedArray> elements(thread, obj->GetElements());
    if (!elements->IsDictionaryMode()) {
        uint32_t elementsLen = elements->GetLength();
        for (uint32_t i = 0, j = elementIndex; i < elementsLen; ++i) {
            if (!elements->Get(i).IsHole()) {
                auto key = base::NumberHelper::NumberToString(thread, JSTaggedValue(i));
                keyArray->Set(thread, j++, key);
            }
        }
    } else {
        NumberDictionary::GetAllKeys(thread, JSHandle<NumberDictionary>(elements), elementIndex, keyArray);
    }
}

void JSObject::GetALLElementKeysIntoVector(const JSThread *thread, const JSHandle<JSObject> &obj,
                                           std::vector<JSTaggedValue> &keyVector)
{
    JSHandle<TaggedArray> elements(thread, obj->GetElements());
    if (!elements->IsDictionaryMode()) {
        uint32_t elementsLen = elements->GetLength();
        for (uint32_t i = 0; i < elementsLen; ++i) {
            if (!elements->Get(i).IsHole()) {
                keyVector.emplace_back(JSTaggedValue(i));
            }
        }
    } else {
        JSHandle<NumberDictionary> dict = JSHandle<NumberDictionary>::Cast(elements);
        dict->GetAllKeysIntoVector(keyVector);
    }
}

JSHandle<TaggedArray> JSObject::GetEnumElementKeys(JSThread *thread, const JSHandle<JSObject> &obj, int offset,
                                                   uint32_t numOfElements, uint32_t *keys)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> elementArray = factory->NewTaggedArray(numOfElements);
    uint32_t elementIndex = 0;
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());

    if (obj->IsJSPrimitiveRef() && JSPrimitiveRef::Cast(*obj)->IsString()) {
        elementIndex = JSPrimitiveRef::Cast(*obj)->GetStringLength();
        *keys += elementIndex;
        elementIndex += offset;
        for (uint32_t i = offset; i < elementIndex; ++i) {
            keyHandle.Update(JSTaggedValue(i));
            auto key = JSTaggedValue::ToString(thread, keyHandle);
            elementArray->Set(thread, i, key);
        }
    }

    JSHandle<TaggedArray> arr(thread, obj->GetElements());
    if (!arr->IsDictionaryMode()) {
        uint32_t elementsLen = arr->GetLength();
        uint32_t preElementIndex = elementIndex;
        for (uint32_t i = 0; i < elementsLen; ++i) {
            if (!arr->Get(i).IsHole()) {
                keyHandle.Update(JSTaggedValue(i));
                auto key = JSTaggedValue::ToString(thread, keyHandle);
                elementArray->Set(thread, elementIndex++, key);
            }
        }
        *keys += (elementIndex - preElementIndex);
    } else {
        NumberDictionary::GetAllEnumKeys(thread, JSHandle<NumberDictionary>(arr), elementIndex, elementArray, keys);
    }
    return elementArray;
}

uint32_t JSObject::GetNumberOfKeys()
{
    DISALLOW_GARBAGE_COLLECTION;
    TaggedArray *array = TaggedArray::Cast(GetProperties().GetTaggedObject());

    if (!array->IsDictionaryMode()) {
        return GetJSHClass()->NumberOfProps();
    }

    return NameDictionary::Cast(array)->EntriesCount();
}

bool JSObject::GlobalSetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &key,
                                 const JSHandle<JSTaggedValue> &value, bool mayThrow)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    ObjectOperator op(thread, key);
    if (!op.IsFound()) {
        PropertyAttributes attr = PropertyAttributes::Default(true, true, false);
        op.SetAttr(attr);
    }
    return SetProperty(&op, value, mayThrow);
}

uint32_t JSObject::GetNumberOfElements()
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t numOfElements = 0;
    if (IsJSPrimitiveRef() && JSPrimitiveRef::Cast(this)->IsString()) {
        numOfElements = JSPrimitiveRef::Cast(this)->GetStringLength();
    }

    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    if (!elements->IsDictionaryMode()) {
        uint32_t elementsLen = elements->GetLength();
        for (uint32_t i = 0; i < elementsLen; ++i) {
            if (!elements->Get(i).IsHole()) {
                numOfElements++;
            }
        }
    } else {
        numOfElements += NumberDictionary::Cast(elements)->EntriesCount();
    }

    return numOfElements;
}

// 9.1.9 [[Set]] ( P, V, Receiver)
bool JSObject::SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                           const JSHandle<JSTaggedValue> &value, const JSHandle<JSTaggedValue> &receiver, bool mayThrow)
{
    ASSERT_PRINT(!(obj->IsUndefined() || obj->IsNull() || obj->IsHole()), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    // 2 ~ 4 findProperty in Receiver, Obj and its parents
    ObjectOperator op(thread, obj, receiver, key);
    return SetProperty(&op, value, mayThrow);
}

bool JSObject::SetProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                           const JSHandle<JSTaggedValue> &value, bool mayThrow)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid JSObject");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    ObjectOperator op(thread, obj, key);
    return SetProperty(&op, value, mayThrow);
}

bool JSObject::SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                           const JSHandle<JSTaggedValue> &value, bool mayThrow)
{
    ASSERT_PRINT(!(obj->IsUndefined() || obj->IsNull() || obj->IsHole()), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    // 2 ~ 4 findProperty in Receiver, Obj and its parents
    ObjectOperator op(thread, obj, key);
    return SetProperty(&op, value, mayThrow);
}

bool JSObject::SetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t index,
                           const JSHandle<JSTaggedValue> &value, bool mayThrow)
{
    ASSERT_PRINT(!(obj->IsUndefined() || obj->IsNull() || obj->IsHole()), "Obj is not a valid object");

    ObjectOperator op(thread, obj, index);
    return SetProperty(&op, value, mayThrow);
}

bool JSObject::SetProperty(ObjectOperator *op, const JSHandle<JSTaggedValue> &value, bool mayThrow)
{
    JSThread *thread = op->GetThread();

    JSHandle<JSTaggedValue> receiver = op->GetReceiver();
    JSHandle<JSTaggedValue> holder = op->GetHolder();
    if (holder->IsJSProxy()) {
        if (op->IsElement()) {
            JSHandle<JSTaggedValue> key(thread, JSTaggedValue(op->GetElementIndex()));
            return JSProxy::SetProperty(thread, JSHandle<JSProxy>::Cast(holder), key, value, receiver, mayThrow);
        }
        return JSProxy::SetProperty(thread, JSHandle<JSProxy>::Cast(holder), op->GetKey(), value, receiver, mayThrow);
    }

    // When op is not found and is not set extra attributes
    if (!op->IsFound() && op->IsPrimitiveAttr()) {
        op->SetAsDefaultAttr();
    }

    bool isInternalAccessor = false;
    if (op->IsAccessorDescriptor()) {
        JSTaggedValue ret = ShouldGetValueFromBox(op);
        isInternalAccessor = AccessorData::Cast(ret.GetTaggedObject())->IsInternal();
    }

    // 5. If IsDataDescriptor(ownDesc) is true, then
    if (!op->IsAccessorDescriptor() || isInternalAccessor) {
        if (!op->IsWritable()) {
            if (mayThrow) {
                THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot assign to read only property", false);
            }
            return false;
        }

        if (!receiver->IsECMAObject()) {
            if (mayThrow) {
                THROW_TYPE_ERROR_AND_RETURN(thread, "Receiver is not a JSObject", false);
            }
            return false;
        }

        if (receiver->IsJSProxy()) {
            JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
            if (op->IsElement()) {
                key.Update(JSTaggedValue(op->GetElementIndex()));
            } else {
                key.Update(op->GetKey().GetTaggedValue());
            }

            PropertyDescriptor existDesc(thread);
            JSProxy::GetOwnProperty(thread, JSHandle<JSProxy>::Cast(receiver), key, existDesc);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            if (!existDesc.IsEmpty()) {
                if (existDesc.IsAccessorDescriptor()) {
                    return false;
                }

                if (!existDesc.IsWritable()) {
                    return false;
                }

                PropertyDescriptor valueDesc(thread, value);
                return JSProxy::DefineOwnProperty(thread, JSHandle<JSProxy>::Cast(receiver), key, valueDesc);
            }
            return CreateDataProperty(thread, JSHandle<JSObject>(receiver), key, value);
        }

        // 5e. If existingDescriptor is not undefined, then
        bool hasReceiver = false;
        if (op->HasReceiver()) {
            op->ReLookupPropertyInReceiver();
            hasReceiver = true;
        }
        bool isSuccess = true;
        if (op->IsFound() && !op->IsOnPrototype()) {
            // i. If IsAccessorDescriptor(existingDescriptor) is true, return false.
            if (op->IsAccessorDescriptor() && !isInternalAccessor) {
                return false;
            }

            // ii. If existingDescriptor.[[Writable]] is false, return false.
            if (!op->IsWritable()) {
                if (mayThrow) {
                    THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot assign to read only property", false);
                }
                return false;
            }
            isSuccess = op->UpdateDataValue(JSHandle<JSObject>(receiver), value, isInternalAccessor, mayThrow);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, isSuccess);
        } else {
            // 5f. Else if Receiver does not currently have a property P, Return CreateDataProperty(Receiver, P, V).
            if (!receiver->IsExtensible(thread)) {
                if (mayThrow) {
                    THROW_TYPE_ERROR_AND_RETURN(thread, "receiver is not Extensible", false);
                }
                return false;
            }
            if (LIKELY(!hasReceiver)) {
                return op->AddProperty(JSHandle<JSObject>(receiver), value, op->GetAttr());
            } else {
                PropertyAttributes attr;
                attr.SetDefaultAttributes();
                return op->AddProperty(JSHandle<JSObject>(receiver), value, attr);
            }
        }
        return isSuccess;
    }
    // 6. Assert: IsAccessorDescriptor(ownDesc) is true.
    ASSERT(op->IsAccessorDescriptor());
    // 8. If setter is undefined, return false.
    JSTaggedValue ret = ShouldGetValueFromBox(op);
    AccessorData *accessor = AccessorData::Cast(ret.GetTaggedObject());
    return CallSetter(thread, *accessor, receiver, value, mayThrow);
}

bool JSObject::CallSetter(JSThread *thread, const AccessorData &accessor, const JSHandle<JSTaggedValue> &receiver,
                          const JSHandle<JSTaggedValue> &value, bool mayThrow)
{
    if (UNLIKELY(accessor.IsInternal())) {
        return accessor.CallInternalSet(thread, JSHandle<JSObject>::Cast(receiver), value, mayThrow);
    }
    JSTaggedValue setter = accessor.GetSetter();
    // 8. If setter is undefined, return false.
    if (setter.IsUndefined()) {
        if (mayThrow) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot set property when setter is undefined", false);
        }
        return false;
    }

    JSHandle<JSTaggedValue> func(thread, setter);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, receiver, undefined, 1);
    info.SetCallArg(value.GetTaggedValue());
    JSFunction::Call(&info);

    // 10. ReturnIfAbrupt(setterResult).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    return true;
}

JSTaggedValue JSObject::CallGetter(JSThread *thread, const AccessorData *accessor,
                                   const JSHandle<JSTaggedValue> &receiver)
{
    JSTaggedValue getter = accessor->GetGetter();
    // 7. If getter is undefined, return undefined.
    if (getter.IsUndefined()) {
        return JSTaggedValue::Undefined();
    }

    JSHandle<JSTaggedValue> func(thread, getter);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, func, receiver, undefined, 0);
    JSTaggedValue res = JSFunction::Call(&info);
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
    return res;
}

// 9.1.8 [[Get]] (P, Receiver)
OperationResult JSObject::GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                      const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &receiver)
{
    ASSERT_PRINT(!(obj->IsUndefined() || obj->IsNull() || obj->IsHole()), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    ObjectOperator op(thread, obj, receiver, key);
    return OperationResult(thread, GetProperty(thread, &op), PropertyMetaData(op.IsFound()));
}

OperationResult JSObject::GetProperty(JSThread *thread, const JSHandle<JSObject> &obj,
                                      const JSHandle<JSTaggedValue> &key)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid JSObject");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    ObjectOperator op(thread, obj, key);
    return OperationResult(thread, GetProperty(thread, &op), PropertyMetaData(op.IsFound()));
}

OperationResult JSObject::GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                      const JSHandle<JSTaggedValue> &key)
{
    ASSERT_PRINT(!(obj->IsUndefined() || obj->IsNull() || obj->IsHole()), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    ObjectOperator op(thread, obj, key);
    return OperationResult(thread, GetProperty(thread, &op), PropertyMetaData(op.IsFound()));
}

OperationResult JSObject::GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t index)
{
    ASSERT_PRINT(!(obj->IsUndefined() || obj->IsNull() || obj->IsHole()), "Obj is not a valid object");

    ObjectOperator op(thread, obj, index);
    return OperationResult(thread, GetProperty(thread, &op), PropertyMetaData(op.IsFound()));
}

OperationResult JSObject::GetPropertyFromGlobal(JSThread *thread, const JSHandle<JSTaggedValue> &key)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    ObjectOperator op(thread, key);
    return OperationResult(thread, GetProperty(thread, &op), PropertyMetaData(op.IsFound()));
}

JSTaggedValue JSObject::GetProperty(JSThread *thread, ObjectOperator *op)
{
    JSHandle<JSTaggedValue> receiver = op->GetReceiver();
    JSHandle<JSTaggedValue> holder = op->GetHolder();
    if (holder->IsJSProxy()) {
        if (op->IsElement()) {
            JSHandle<JSTaggedValue> key(thread, JSTaggedValue(op->GetElementIndex()));
            return JSProxy::GetProperty(thread, JSHandle<JSProxy>::Cast(holder), op->GetKey(), receiver)
                .GetValue()
                .GetTaggedValue();
        }
        return JSProxy::GetProperty(thread, JSHandle<JSProxy>::Cast(holder), op->GetKey(), receiver)
            .GetValue()
            .GetTaggedValue();
    }

    // 4. If desc is undefined, then
    if (!op->IsFound()) {
        // 4c. If obj and parent is null, return undefined.
        return JSTaggedValue::Undefined();
    }
    // 5. If IsDataDescriptor(desc) is true, return desc.[[Value]]
    JSTaggedValue ret = ShouldGetValueFromBox(op);
    if (!op->IsAccessorDescriptor()) {
        return ret;
    }

    // 6. Otherwise, IsAccessorDescriptor(desc) must be true so, let getter be desc.[[Get]].
    AccessorData *accessor = AccessorData::Cast(ret.GetTaggedObject());
    // 8. Return Call(getter, Receiver).
    if (UNLIKELY(accessor->IsInternal())) {
        return accessor->CallInternalGet(thread, JSHandle<JSObject>::Cast(holder));
    }
    return CallGetter(thread, accessor, receiver);
}

bool JSObject::DeleteProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key)
{
    // 1. Assert: IsPropertyKey(P) is true.
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    // 2. Let desc be O.[[GetOwnProperty]](P).

    ObjectOperator op(thread, JSHandle<JSTaggedValue>(obj), key, OperatorType::OWN);

    // 4. If desc is undefined, return true.
    if (!op.IsFound()) {
        return true;
    }
    // 5. If desc.[[Configurable]] is true, then
    // a. Remove the own property with name P from O.
    // b. Return true.
    // 6. Return false.
    if (op.IsConfigurable()) {
        op.DeletePropertyInHolder();
        return true;
    }
    return false;
}

bool JSObject::GetOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                              PropertyDescriptor &desc)
{
    return OrdinaryGetOwnProperty(thread, obj, key, desc);
}

bool JSObject::GlobalGetOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    ObjectOperator op(thread, key, OperatorType::OWN);

    if (!op.IsFound()) {
        return false;
    }

    op.ToPropertyDescriptor(desc);

    if (desc.HasValue()) {
        PropertyBox *cell = PropertyBox::Cast(desc.GetValue().GetTaggedValue().GetTaggedObject());
        JSHandle<JSTaggedValue> valueHandle(thread, cell->GetValue());
        desc.SetValue(valueHandle);
    }
    ASSERT(!desc.GetValue()->IsInternalAccessor());
    return true;
}

bool JSObject::OrdinaryGetOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj,
                                      const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    ObjectOperator op(thread, JSHandle<JSTaggedValue>(obj), key, OperatorType::OWN);

    if (!op.IsFound()) {
        return false;
    }

    op.ToPropertyDescriptor(desc);

    if (desc.HasValue() && obj->IsJSGlobalObject()) {
        PropertyBox *cell = PropertyBox::Cast(desc.GetValue().GetTaggedValue().GetTaggedObject());
        JSHandle<JSTaggedValue> valueHandle(thread, cell->GetValue());
        desc.SetValue(valueHandle);
    }

    return true;
}

bool JSObject::DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                                 const PropertyDescriptor &desc)
{
    return OrdinaryDefineOwnProperty(thread, obj, key, desc);
}

bool JSObject::DefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                 const PropertyDescriptor &desc)
{
    return OrdinaryDefineOwnProperty(thread, obj, index, desc);
}

// 9.1.6.1 OrdinaryDefineOwnProperty (O, P, Desc)
bool JSObject::OrdinaryDefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj,
                                         const JSHandle<JSTaggedValue> &key, const PropertyDescriptor &desc)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    // 1. Let current be O.[[GetOwnProperty]](P).
    JSHandle<JSTaggedValue> objValue(obj);
    ObjectOperator op(thread, objValue, key, OperatorType::OWN);

    bool extensible = obj->IsExtensible();
    PropertyDescriptor current(thread);
    op.ToPropertyDescriptor(current);
    // 4. Return ValidateAndApplyPropertyDescriptor(O, P, extensible, Desc, current).
    return ValidateAndApplyPropertyDescriptor(&op, extensible, desc, current);
}

bool JSObject::OrdinaryDefineOwnProperty(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                         const PropertyDescriptor &desc)
{
    JSHandle<JSTaggedValue> objValue(obj);
    ObjectOperator op(thread, objValue, index, OperatorType::OWN);

    bool extensible = obj->IsExtensible();
    PropertyDescriptor current(thread);
    op.ToPropertyDescriptor(current);
    return ValidateAndApplyPropertyDescriptor(&op, extensible, desc, current);
}

// 9.1.6.3 ValidateAndApplyPropertyDescriptor (O, P, extensible, Desc, current)
bool JSObject::ValidateAndApplyPropertyDescriptor(ObjectOperator *op, bool extensible, const PropertyDescriptor &desc,
                                                  const PropertyDescriptor &current)
{
    // 2. If current is undefined, then
    if (current.IsEmpty()) {
        // 2a. If extensible is false, return false.
        if (!extensible) {
            return false;
        }
        if (!op->HasHolder()) {
            return true;
        }

        // 2c. If IsGenericDescriptor(Desc) or IsDataDescriptor(Desc) is true, then
        PropertyAttributes attr(desc);
        bool success = false;
        if (!desc.IsAccessorDescriptor()) {
            success = op->AddPropertyInHolder(desc.GetValue(), attr);
        } else {  // is AccessorDescriptor
            // may GC in NewAccessorData, so we need to handle getter and setter.
            JSThread *thread = op->GetThread();
            JSHandle<AccessorData> accessor = thread->GetEcmaVM()->GetFactory()->NewAccessorData();
            if (desc.HasGetter()) {
                accessor->SetGetter(thread, desc.GetGetter());
            }

            if (desc.HasSetter()) {
                accessor->SetSetter(thread, desc.GetSetter());
            }
            success = op->AddPropertyInHolder(JSHandle<JSTaggedValue>::Cast(accessor), attr);
        }

        return success;
    }

    // 3. Return true, if every field in Desc is absent
    // 4. Return true, if every field in Desc also occurs in current and the value of every field in Desc is the
    // same value as the corresponding field in current when compared using the SameValue algorithm.
    if ((!desc.HasEnumerable() || desc.IsEnumerable() == current.IsEnumerable()) &&
        (!desc.HasConfigurable() || desc.IsConfigurable() == current.IsConfigurable()) &&
        (!desc.HasValue() || JSTaggedValue::SameValue(current.GetValue(), desc.GetValue())) &&
        (!desc.HasWritable() || (current.IsWritable() == desc.IsWritable())) &&
        (!desc.HasGetter() ||
         (current.HasGetter() && JSTaggedValue::SameValue(current.GetGetter(), desc.GetGetter()))) &&
        (!desc.HasSetter() ||
         (current.HasSetter() && JSTaggedValue::SameValue(current.GetSetter(), desc.GetSetter())))) {
        return true;
    }

    // 5. If the [[Configurable]] field of current is false, then
    if (!current.IsConfigurable()) {
        // 5a. Return false, if the [[Configurable]] field of Desc is true.
        if (desc.HasConfigurable() && desc.IsConfigurable()) {
            return false;
        }
        // b. Return false, if the [[Enumerable]] field of Desc is present and the [[Enumerable]] fields of current
        // and Desc are the Boolean negation of each other.
        if (desc.HasEnumerable() && (desc.IsEnumerable() != current.IsEnumerable())) {
            return false;
        }
    }

    // 6. If IsGenericDescriptor(Desc) is true, no further validation is required.
    if (desc.IsGenericDescriptor()) {
        // 7. Else if IsDataDescriptor(current) and IsDataDescriptor(Desc) have different results, then
    } else if (current.IsDataDescriptor() != desc.IsDataDescriptor()) {
        // 7a. Return false, if the [[Configurable]] field of current is false.
        if (!current.IsConfigurable()) {
            return false;
        }
        // 7b. If IsDataDescriptor(current) is true, then
        if (current.IsDataDescriptor()) {
            // 7bi. If O is not undefined, convert the property named P of object O from a data property to an
            // accessor property. Preserve the existing values of the converted property’s [[Configurable]] and
            // [[Enumerable]] attributes and set the rest of the property’s attributes to their default values.
        } else {
            // 7ci.  If O is not undefined, convert the property named P of object O from an accessor property to a
            // data property. Preserve the existing values of the converted property’s [[Configurable]] and
            // [[Enumerable]] attributes and set the rest of the property’s attributes to their default values.
        }
        // 8. Else if IsDataDescriptor(current) and IsDataDescriptor(Desc) are both true, then
    } else if (current.IsDataDescriptor() && desc.IsDataDescriptor()) {
        // 8a. If the [[Configurable]] field of current is false, then
        if (!current.IsConfigurable()) {
            // 8a i. Return false, if the [[Writable]] field of current is false and the [[Writable]] field of Desc
            // is true.
            if (!current.IsWritable() && desc.HasWritable() && desc.IsWritable()) {
                return false;
            }
            // 8a ii. If the [[Writable]] field of current is false, then
            if (!current.IsWritable()) {
                if (desc.HasValue() && !JSTaggedValue::SameValue(current.GetValue(), desc.GetValue())) {
                    return false;
                }
            }
        }
        // 8b. Else the [[Configurable]] field of current is true, so any change is acceptable.
    } else {  // 9. Else IsAccessorDescriptor(current) and IsAccessorDescriptor(Desc) are both true,
        // 9a. If the [[Configurable]] field of current is false, then
        if (!current.IsConfigurable()) {
            // i. Return false, if the [[Set]] field of Desc is present and SameValue(Desc.[[Set]], current.[[Set]])
            // is false.
            if (desc.HasSetter() && !JSTaggedValue::SameValue(current.GetSetter(), desc.GetSetter())) {
                return false;
            }
            // ii. Return false, if the [[Get]] field of Desc is present and SameValue(Desc.[[Get]],
            // current.[[Get]]) is false.
            if (desc.HasGetter() && !JSTaggedValue::SameValue(current.GetGetter(), desc.GetGetter())) {
                return false;
            }
        }
    }

    if (op->HasHolder()) {
        // 10. If O is not undefined, then
        // a. For each field of Desc that is present, set the corresponding attribute of the property named P of object
        // O to the value of the field.
        return op->WriteDataPropertyInHolder(desc);
    }
    return true;
}

// 9.1.6.2 IsCompatiblePropertyDescriptor (Extensible, Desc, Current)
bool JSObject::IsCompatiblePropertyDescriptor(bool extensible, const PropertyDescriptor &desc,
                                              const PropertyDescriptor &current)
{
    // 1. Return ValidateAndApplyPropertyDescriptor(undefined, undefined, Extensible, Desc, Current).
    ObjectOperator op;
    return ValidateAndApplyPropertyDescriptor(&op, extensible, desc, current);
}

JSTaggedValue JSObject::GetPrototype(const JSHandle<JSObject> &obj)
{
    JSHClass *hclass = obj->GetJSHClass();
    return hclass->GetPrototype();
}

bool JSObject::SetPrototype(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &proto)
{
    ASSERT_PRINT(proto->IsECMAObject() || proto->IsNull(), "proto must be object or null");
    JSTaggedValue current = JSObject::GetPrototype(obj);
    if (current == proto.GetTaggedValue()) {
        return true;
    }
    if (!obj->IsExtensible()) {
        return false;
    }
    bool done = false;
    JSMutableHandle<JSTaggedValue> tempProtoHandle(thread, proto.GetTaggedValue());
    while (!done) {
        if (tempProtoHandle->IsNull() || !tempProtoHandle->IsECMAObject()) {
            done = true;
        } else if (JSTaggedValue::SameValue(tempProtoHandle.GetTaggedValue(), obj.GetTaggedValue())) {
            return false;
        } else {
            if (tempProtoHandle->IsJSProxy()) {
                break;
            }
            tempProtoHandle.Update(JSTaggedValue::GetPrototype(thread, JSHandle<JSTaggedValue>(tempProtoHandle)));
        }
    }
    // map transition
    JSHandle<JSHClass> dynclass(thread, obj->GetJSHClass());
    JSHandle<JSHClass> newDynclass = JSHClass::TransitionProto(thread, dynclass, proto);
    JSHClass::NotifyHclassChanged(thread, dynclass, newDynclass);
    obj->SetClass(newDynclass);
    thread->NotifyStableArrayElementsGuardians(obj);
    return true;
}

bool JSObject::HasProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key)
{
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    JSHandle<JSTaggedValue> objValue(obj);
    ObjectOperator op(thread, objValue, key);

    JSHandle<JSTaggedValue> holder = op.GetHolder();
    if (holder->IsJSProxy()) {
        return JSProxy::HasProperty(thread, JSHandle<JSProxy>::Cast(holder), key);
    }

    return op.IsFound();
}

bool JSObject::HasProperty(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index)
{
    JSHandle<JSTaggedValue> objValue(obj);
    ObjectOperator op(thread, objValue, index);

    JSHandle<JSTaggedValue> holder = op.GetHolder();
    if (holder->IsJSProxy()) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(index));
        return JSProxy::HasProperty(thread, JSHandle<JSProxy>::Cast(holder), key);
    }

    return op.IsFound();
}

bool JSObject::PreventExtensions(JSThread *thread, const JSHandle<JSObject> &obj)
{
    if (obj->IsExtensible()) {
        JSHandle<JSHClass> jshclass(thread, obj->GetJSHClass());
        JSHandle<JSHClass> newHclass = JSHClass::TransitionExtension(thread, jshclass);
        obj->SetClass(newHclass);
    }

    return true;
}

// 9.1.12 [[OwnPropertyKeys]] ( )
JSHandle<TaggedArray> JSObject::GetOwnPropertyKeys(JSThread *thread, const JSHandle<JSObject> &obj)
{
    [[maybe_unused]] uint32_t elementIndex = 0;
    uint32_t numOfElements = obj->GetNumberOfElements();
    uint32_t keyLen = numOfElements + obj->GetNumberOfKeys();

    JSHandle<TaggedArray> keyArray = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(keyLen);

    if (numOfElements > 0) {
        GetAllElementKeys(thread, obj, 0, keyArray);
    }
    GetAllKeys(thread, obj, static_cast<int32_t>(numOfElements), keyArray);
    return keyArray;
}

JSHandle<JSObject> JSObject::ObjectCreate(JSThread *thread, const JSHandle<JSObject> &proto)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetObjectFunction();
    JSHandle<JSObject> objHandle =
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSObject, thread);
    SetPrototype(thread, objHandle, JSHandle<JSTaggedValue>(proto));
    return objHandle;
}

// 7.3.4 CreateDataProperty (O, P, V)
bool JSObject::CreateDataProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                                  const JSHandle<JSTaggedValue> &value)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    auto result = FastRuntimeStub::SetPropertyByValue<true>(thread, obj.GetTaggedValue(), key.GetTaggedValue(),
                                                            value.GetTaggedValue());
    if (!result.IsHole()) {
        return result != JSTaggedValue::Exception();
    }
    PropertyDescriptor desc(thread, value, true, true, true);
    return JSTaggedValue::DefineOwnProperty(thread, JSHandle<JSTaggedValue>::Cast(obj), key, desc);
}

bool JSObject::CreateDataProperty(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                  const JSHandle<JSTaggedValue> &value)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");
    auto result =
        FastRuntimeStub::SetPropertyByIndex<true>(thread, obj.GetTaggedValue(), index, value.GetTaggedValue());
    if (!result.IsHole()) {
        return result != JSTaggedValue::Exception();
    }
    PropertyDescriptor desc(thread, value, true, true, true);
    return DefineOwnProperty(thread, obj, index, desc);
}

// 7.3.5 CreateMethodProperty (O, P, V)
bool JSObject::CreateDataPropertyOrThrow(JSThread *thread, const JSHandle<JSObject> &obj,
                                         const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    bool success = CreateDataProperty(thread, obj, key, value);
    if (!success) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "failed to create data property", success);
    }
    return success;
}

bool JSObject::CreateDataPropertyOrThrow(JSThread *thread, const JSHandle<JSObject> &obj, uint32_t index,
                                         const JSHandle<JSTaggedValue> &value)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");

    bool success = CreateDataProperty(thread, obj, index, value);
    if (!success) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "failed to create data property", success);
    }
    return success;
}
// 7.3.6 CreateDataPropertyOrThrow (O, P, V)
bool JSObject::CreateMethodProperty(JSThread *thread, const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &key,
                                    const JSHandle<JSTaggedValue> &value)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");

    PropertyDescriptor desc(thread, value, true, false, true);
    return DefineOwnProperty(thread, obj, key, desc);
}

// 7.3.9 GetMethod (O, P)
JSHandle<JSTaggedValue> JSObject::GetMethod(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                            const JSHandle<JSTaggedValue> &key)
{
    JSTaggedValue func = FastRuntimeStub::FastGetProperty(thread, obj.GetTaggedValue(), key.GetTaggedValue());
    if (func.IsUndefined() || func.IsNull()) {
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined());
    }

    JSHandle<JSTaggedValue> result(thread, func);
    if (!result->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "obj is not Callable", result);
    }
    return result;
}

// 7.3.14 SetIntegrityLevel (O, level)
bool JSObject::SetIntegrityLevel(JSThread *thread, const JSHandle<JSObject> &obj, IntegrityLevel level)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");
    ASSERT_PRINT((level == IntegrityLevel::SEALED || level == IntegrityLevel::FROZEN),
                 "level is not a valid IntegrityLevel");

    bool status = PreventExtensions(thread, obj);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (!status) {
        return false;
    }

    JSHandle<TaggedArray> jshandleKeys = GetOwnPropertyKeys(thread, obj);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    PropertyDescriptor descNoConf(thread);
    descNoConf.SetConfigurable(false);
    PropertyDescriptor descNoConfWrite(thread);
    descNoConfWrite.SetWritable(false);
    descNoConfWrite.SetConfigurable(false);

    if (level == IntegrityLevel::SEALED) {
        uint32_t length = jshandleKeys->GetLength();
        if (length == 0) {
            return true;
        }
        auto key = jshandleKeys->Get(0);
        JSMutableHandle<JSTaggedValue> handleKey(thread, key);
        for (uint32_t i = 0; i < length; i++) {
            auto taggedKey = JSTaggedValue(jshandleKeys->Get(i));
            handleKey.Update(taggedKey);
            [[maybe_unused]] bool success =
                JSTaggedValue::DefinePropertyOrThrow(thread, JSHandle<JSTaggedValue>(obj), handleKey, descNoConf);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        }
    } else {
        uint32_t length = jshandleKeys->GetLength();
        if (length == 0) {
            return true;
        }
        auto key = jshandleKeys->Get(0);
        JSMutableHandle<JSTaggedValue> handleKey(thread, key);
        for (uint32_t i = 0; i < length; i++) {
            auto taggedKey = JSTaggedValue(jshandleKeys->Get(i));
            handleKey.Update(taggedKey);
            PropertyDescriptor currentDesc(thread);
            bool curDescStatus = GetOwnProperty(thread, obj, handleKey, currentDesc);
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            if (curDescStatus) {
                PropertyDescriptor desc = currentDesc.IsAccessorDescriptor() ? descNoConf : descNoConfWrite;
                [[maybe_unused]] bool success =
                    JSTaggedValue::DefinePropertyOrThrow(thread, JSHandle<JSTaggedValue>(obj), handleKey, desc);
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
            }
        }
    }
    return true;
}

// 7.3.15 TestIntegrityLevel (O, level)
bool JSObject::TestIntegrityLevel(JSThread *thread, const JSHandle<JSObject> &obj, IntegrityLevel level)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");
    ASSERT_PRINT((level == IntegrityLevel::SEALED || level == IntegrityLevel::FROZEN),
                 "level is not a valid IntegrityLevel");

    bool status = obj->IsExtensible();
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    if (status) {
        return false;
    }

    JSHandle<TaggedArray> jshandleKeys = GetOwnPropertyKeys(thread, obj);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    uint32_t length = jshandleKeys->GetLength();
    if (length == 0) {
        return true;
    }
    auto key = jshandleKeys->Get(0);
    JSMutableHandle<JSTaggedValue> handleKey(thread, key);
    for (uint32_t i = 0; i < length; i++) {
        auto taggedKey = JSTaggedValue(jshandleKeys->Get(i));
        handleKey.Update(taggedKey);
        PropertyDescriptor currentDesc(thread);
        bool curDescStatus = GetOwnProperty(thread, obj, handleKey, currentDesc);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        if (curDescStatus) {
            if (currentDesc.IsConfigurable()) {
                return false;
            }
            if (level == IntegrityLevel::FROZEN && currentDesc.IsDataDescriptor() && currentDesc.IsWritable()) {
                return false;
            }
        }
    }
    return true;
}

// 7.3.21 EnumerableOwnNames (O)
JSHandle<TaggedArray> JSObject::EnumerableOwnNames(JSThread *thread, const JSHandle<JSObject> &obj)
{
    ASSERT_PRINT(obj->IsECMAObject(), "obj is not object");
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> keys;
    JSHandle<JSTaggedValue> tagObj(obj);
    uint32_t copyLength = 0;
    // fast mode
    if (tagObj->IsJSObject() && !tagObj->IsTypedArray() && !tagObj->IsModuleNamespace()) {
        uint32_t numOfKeys = obj->GetNumberOfKeys();
        uint32_t numOfElements = obj->GetNumberOfElements();
        JSHandle<TaggedArray> elementArray;
        if (numOfElements > 0) {
            elementArray = JSObject::GetEnumElementKeys(thread, obj, 0, numOfElements, &copyLength);
        }

        JSHandle<TaggedArray> keyArray;
        if (numOfKeys > 0) {
            keyArray = JSObject::GetAllEnumKeys(thread, obj, 0, numOfKeys, &copyLength);
        }

        if (numOfKeys != 0 && numOfElements != 0) {
            keys = TaggedArray::AppendSkipHole(thread, elementArray, keyArray, copyLength);
        } else if (numOfKeys != 0) {
            keys = factory->CopyArray(keyArray, copyLength, copyLength);
        } else if (numOfElements != 0) {
            keys = factory->CopyArray(elementArray, copyLength, copyLength);
        } else {
            keys = factory->EmptyArray();
        }
        return keys;
    }

    keys = JSTaggedValue::GetOwnPropertyKeys(thread, tagObj);
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
    uint32_t length = keys->GetLength();

    JSHandle<TaggedArray> names = factory->NewTaggedArray(length);
    for (uint32_t i = 0; i < length; i++) {
        JSTaggedValue key(keys->Get(i));
        if (key.IsString()) {
            PropertyDescriptor desc(thread);
            bool status = JSTaggedValue::GetOwnProperty(thread, JSHandle<JSTaggedValue>(obj),
                                                        JSHandle<JSTaggedValue>(thread, key), desc);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);

            if (status && desc.IsEnumerable()) {
                names->Set(thread, copyLength, key);
                copyLength++;
            }
        }
    }

    return factory->CopyArray(names, length, copyLength);
}

JSHandle<TaggedArray> JSObject::EnumerableOwnPropertyNames(JSThread *thread, const JSHandle<JSObject> &obj,
                                                           PropertyKind kind)
{
    // 1. Assert: Type(O) is Object.
    ASSERT_PRINT(obj->IsECMAObject(), "obj is not object");

    // 2. Let ownKeys be ? O.[[OwnPropertyKeys]]().
    JSHandle<TaggedArray> ownKeys = JSTaggedValue::GetOwnPropertyKeys(thread, JSHandle<JSTaggedValue>(obj));
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);

    // 3. Let properties be a new empty List.
    uint32_t length = ownKeys->GetLength();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> properties = factory->NewTaggedArray(length);

    // 4. For each element key of ownKeys, do
    // a. If Type(key) is String, then
    //     i. Let desc be ? O.[[GetOwnProperty]](key).
    //     ii. If desc is not undefined and desc.[[Enumerable]] is true, then
    //         1. If kind is key, append key to properties.
    //         2. Else,
    //            a. Let value be ? Get(O, key).
    //            b. If kind is value, append value to properties.
    //            c. Else,
    //               i. Assert: kind is key+value.
    //               ii. Let entry be ! CreateArrayFromList(« key, value »).
    //               iii. Append entry to properties.
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    uint32_t index = 0;
    for (uint32_t i = 0; i < length; i++) {
        key.Update(ownKeys->Get(thread, i));
        if (key->IsString()) {
            PropertyDescriptor desc(thread);
            bool status = GetOwnProperty(thread, obj, JSHandle<JSTaggedValue>(key), desc);
            RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
            if (status && desc.IsEnumerable()) {
                if (kind == PropertyKind::KEY) {
                    properties->Set(thread, index++, key);
                } else {
                    OperationResult result =
                        JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(obj), key);
                    RETURN_HANDLE_IF_ABRUPT_COMPLETION(TaggedArray, thread);
                    JSHandle<JSTaggedValue> value = result.GetValue();
                    if (kind == PropertyKind::VALUE) {
                        properties->Set(thread, index++, value);
                    } else {
                        ASSERT_PRINT(kind == PropertyKind::KEY_VALUE, "kind is invalid");
                        JSHandle<TaggedArray> keyValue = factory->NewTaggedArray(2);  // 2: key-value pair
                        keyValue->Set(thread, 0, key.GetTaggedValue());
                        keyValue->Set(thread, 1, value.GetTaggedValue());
                        JSHandle<JSArray> entry = JSArray::CreateArrayFromList(thread, keyValue);
                        properties->Set(thread, index++, entry.GetTaggedValue());
                    }
                }
            }
        }
    }
    // 5. Return properties.
    return properties;
}

JSHandle<GlobalEnv> JSObject::GetFunctionRealm(JSThread *thread, const JSHandle<JSTaggedValue> &object)
{
    // 1. Assert: obj is a callable object.
    ASSERT(object->IsCallable());
    // 2. If obj has a [[Realm]] internal slot, then return obj’s [[Realm]] internal slot.
    // 3. If obj is a Bound Function exotic object, then
    if (object->IsBoundFunction()) {
        // a. Let target be obj’s [[BoundTargetFunction]] internal slot.
        JSHandle<JSTaggedValue> target(thread, JSHandle<JSBoundFunction>(object)->GetBoundTarget());
        // b. Return GetFunctionRealm(target).
        return GetFunctionRealm(thread, target);
    }
    // 4. If obj is a Proxy exotic object, then
    if (object->IsJSProxy()) {
        // a. If the value of the [[ProxyHandler]] internal slot of obj is null, throw a TypeError exception.
        if (JSHandle<JSProxy>(object)->GetHandler().IsNull()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "JSObject::GetFunctionRealm: handler is null",
                                        JSHandle<GlobalEnv>(thread, JSTaggedValue::Exception()));
        }
        // b. Let proxyTarget be the value of obj’s [[ProxyTarget]] internal slot.
        JSHandle<JSTaggedValue> proxyTarget(thread, JSHandle<JSProxy>(object)->GetTarget());
        return GetFunctionRealm(thread, proxyTarget);
    }
    JSTaggedValue maybeGlobalEnv = JSHandle<JSFunction>(object)->GetLexicalEnv();
    if (maybeGlobalEnv.IsUndefined()) {
        return thread->GetEcmaVM()->GetGlobalEnv();
    }
    while (!maybeGlobalEnv.IsJSGlobalEnv()) {
        maybeGlobalEnv = LexicalEnv::Cast(maybeGlobalEnv.GetTaggedObject())->GetParentEnv();
    }
    return JSHandle<GlobalEnv>(thread, maybeGlobalEnv);
}

bool JSObject::InstanceOf(JSThread *thread, const JSHandle<JSTaggedValue> &object,
                          const JSHandle<JSTaggedValue> &target)
{
    // 1. If Type(target) is not Object, throw a TypeError exception.
    if (!target->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "InstanceOf error when type of target is not Object", false);
    }

    EcmaVM *vm = thread->GetEcmaVM();
    // 2. Let instOfHandler be GetMethod(target, @@hasInstance).
    JSHandle<JSTaggedValue> instOfHandler = GetMethod(thread, target, vm->GetGlobalEnv()->GetHasInstanceSymbol());

    // 3. ReturnIfAbrupt(instOfHandler).
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);

    // 4. If instOfHandler is not undefined, then
    if (!instOfHandler->IsUndefined()) {
        // a. Return ! ToBoolean(? Call(instOfHandler, target, «object»)).
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo info =
            EcmaInterpreter::NewRuntimeCallInfo(thread, instOfHandler, target, undefined, 1);
        info.SetCallArg(object.GetTaggedValue());
        JSTaggedValue tagged = JSFunction::Call(&info);
        return tagged.ToBoolean();
    }

    // 5. If IsCallable(target) is false, throw a TypeError exception.
    if (!target->IsCallable()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "InstanceOf error when target is not Callable", false);
    }

    // 6. Return ? OrdinaryHasInstance(target, object).
    return JSFunction::OrdinaryHasInstance(thread, target, object);
}

// ecma6.0 6.2.4.4
JSHandle<JSTaggedValue> JSObject::FromPropertyDescriptor(JSThread *thread, const PropertyDescriptor &desc)
{
    // 1. If Desc is undefined, return undefined
    if (desc.IsEmpty()) {
        return JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined());
    }

    // 2. Let obj be ObjectCreate(%ObjectPrototype%).
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFunc = env->GetObjectFunction();
    JSHandle<JSObject> objHandle =
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(objFunc), objFunc);

    auto globalConst = thread->GlobalConstants();
    // 4. If Desc has a [[Value]] field, then Perform CreateDataProperty(obj, "value", Desc.[[Value]]).
    if (desc.HasValue()) {
        JSHandle<JSTaggedValue> valueStr = globalConst->GetHandledValueString();
        bool success = CreateDataProperty(thread, objHandle, valueStr, desc.GetValue());
        RASSERT_PRINT(success, "CreateDataProperty must be success");
    }
    // 5. If Desc has a [[Writable]] field, then Perform CreateDataProperty(obj, "writable", Desc.[[Writable]]).
    if (desc.HasWritable()) {
        JSHandle<JSTaggedValue> writableStr = globalConst->GetHandledWritableString();
        JSHandle<JSTaggedValue> writable(thread, JSTaggedValue(desc.IsWritable()));
        [[maybe_unused]] bool success = CreateDataProperty(thread, objHandle, writableStr, writable);
        ASSERT_PRINT(success, "CreateDataProperty must be success");
    }
    // 6. If Desc has a [[Get]] field, then Perform CreateDataProperty(obj, "get", Desc.[[Get]]).
    if (desc.HasGetter()) {
        JSHandle<JSTaggedValue> getStr = globalConst->GetHandledGetString();
        bool success = CreateDataProperty(thread, objHandle, getStr, desc.GetGetter());
        RASSERT_PRINT(success, "CreateDataProperty must be success");
    }
    // 7. If Desc has a [[Set]] field, then Perform CreateDataProperty(obj, "set", Desc.[[Set]])
    if (desc.HasSetter()) {
        JSHandle<JSTaggedValue> setStr = globalConst->GetHandledSetString();
        bool success = CreateDataProperty(thread, objHandle, setStr, desc.GetSetter());
        RASSERT_PRINT(success, "CreateDataProperty must be success");
    }
    // 8. If Desc has an [[Enumerable]] field, then Perform CreateDataProperty(obj, "enumerable",
    // Desc.[[Enumerable]]).
    if (desc.HasEnumerable()) {
        JSHandle<JSTaggedValue> enumerableStr = globalConst->GetHandledEnumerableString();
        JSHandle<JSTaggedValue> enumerable(thread, JSTaggedValue(desc.IsEnumerable()));
        [[maybe_unused]] bool success = CreateDataProperty(thread, objHandle, enumerableStr, enumerable);
        ASSERT_PRINT(success, "CreateDataProperty must be success");
    }
    // 9. If Desc has a [[Configurable]] field, then Perform CreateDataProperty(obj , "configurable",
    // Desc.[[Configurable]]).
    if (desc.HasConfigurable()) {
        JSHandle<JSTaggedValue> configurableStr = globalConst->GetHandledConfigurableString();
        JSHandle<JSTaggedValue> configurable(thread, JSTaggedValue(desc.IsConfigurable()));
        [[maybe_unused]] bool success = CreateDataProperty(thread, objHandle, configurableStr, configurable);
        ASSERT_PRINT(success, "CreateDataProperty must be success");
    }
    return JSHandle<JSTaggedValue>(objHandle);
}

bool JSObject::ToPropertyDescriptorFast(JSThread *thread, const JSHandle<JSTaggedValue> &obj, PropertyDescriptor &desc)
{
    auto *hclass = obj->GetTaggedObject()->GetClass();
    JSType jsType = hclass->GetObjectType();
    if (jsType != JSType::JS_OBJECT) {
        return false;
    }
    if (hclass->IsDictionaryMode()) {
        return false;
    }
    auto env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    if (hclass->GetPrototype() != env->GetObjectFunctionPrototype().GetTaggedValue()) {
        return false;
    }
    if (JSObject::Cast(hclass->GetPrototype().GetTaggedObject())->GetClass() !=
        env->GetObjectFunctionPrototypeClass().GetObject<JSHClass>()) {
        return false;
    }
    LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
    int propsNumber = hclass->NumberOfProps();
    for (int i = 0; i < propsNumber; i++) {
        auto attr = layoutInfo->GetAttr(i);
        if (attr.IsAccessor()) {
            return false;
        }
        auto key = layoutInfo->GetKey(i);
        auto value = JSObject::Cast(obj->GetTaggedObject())->GetProperty(hclass, attr);
        if (key == globalConst->GetEnumerableString()) {
            bool enumerable = value.ToBoolean();
            desc.SetEnumerable(enumerable);
        } else if (key == globalConst->GetConfigurableString()) {
            bool configurable = value.ToBoolean();
            desc.SetConfigurable(configurable);
        } else if (key == globalConst->GetValueString()) {
            auto handleValue = JSHandle<JSTaggedValue>(thread, value);
            desc.SetValue(handleValue);
        } else if (key == globalConst->GetWritableString()) {
            bool writable = value.ToBoolean();
            desc.SetWritable(writable);
        } else if (key == globalConst->GetGetString()) {
            if (!value.IsCallable()) {
                return false;
            }
            auto getter = JSHandle<JSTaggedValue>(thread, value);
            desc.SetGetter(getter);
        } else if (key == globalConst->GetSetString()) {
            if (!value.IsCallable()) {
                return false;
            }
            auto setter = JSHandle<JSTaggedValue>(thread, value);
            desc.SetSetter(setter);
        }
    }

    if (desc.IsAccessorDescriptor()) {
        // 22a. If either desc.[[Value]] or desc.[[Writable]] is present, throw a TypeError exception.
        if (desc.HasValue() || desc.HasWritable()) {
            THROW_TYPE_ERROR_AND_RETURN(thread, "either Value or Writable is present", true);
        }
    }
    return true;
}

// ecma6.0 6.2.4.5 ToPropertyDescriptor ( Obj )
void JSObject::ToPropertyDescriptor(JSThread *thread, const JSHandle<JSTaggedValue> &obj, PropertyDescriptor &desc)
{
    if (!obj->IsECMAObject()) {
        // 2. If Type(Obj) is not Object, throw a TypeError exception.
        THROW_TYPE_ERROR(thread, "ToPropertyDescriptor error obj is not Object");
    }

    if (ToPropertyDescriptorFast(thread, obj, desc)) {
        return;
    }
    auto globalConst = thread->GlobalConstants();
    // 3. Let desc be a new Property Descriptor that initially has no fields.
    // 4. Let hasEnumerable be HasProperty(Obj, "enumerable")
    JSHandle<JSTaggedValue> enumerableStr = globalConst->GetHandledEnumerableString();
    {
        ObjectOperator op(thread, obj.GetTaggedValue(), enumerableStr.GetTaggedValue());
        if (op.IsFound()) {
            auto value = op.FastGetValue();
            bool enumerable = value->IsException() ? false : value->ToBoolean();
            desc.SetEnumerable(enumerable);
        }
    }
    // 7. Let hasConfigurable be HasProperty(Obj, "configurable").
    JSHandle<JSTaggedValue> configurableStr = globalConst->GetHandledConfigurableString();
    {
        ObjectOperator op(thread, obj.GetTaggedValue(), configurableStr.GetTaggedValue());
        if (op.IsFound()) {
            auto value = op.FastGetValue();
            bool conf = value->IsException() ? false : value->ToBoolean();
            desc.SetConfigurable(conf);
        }
    }
    // 10. Let hasValue be HasProperty(Obj, "value").
    JSHandle<JSTaggedValue> valueStr = globalConst->GetHandledValueString();
    {
        ObjectOperator op(thread, obj.GetTaggedValue(), valueStr.GetTaggedValue());
        if (op.IsFound()) {
            JSHandle<JSTaggedValue> prop = op.FastGetValue();
            desc.SetValue(prop);
        }
    }
    // 13. Let hasWritable be HasProperty(Obj, "writable").
    JSHandle<JSTaggedValue> writableStr = globalConst->GetHandledWritableString();
    {
        ObjectOperator op(thread, obj.GetTaggedValue(), writableStr.GetTaggedValue());
        if (op.IsFound()) {
            auto value = op.FastGetValue();
            bool writable = value->IsException() ? false : value->ToBoolean();
            desc.SetWritable(writable);
        }
    }
    // 16. Let hasGet be HasProperty(Obj, "get").
    JSHandle<JSTaggedValue> getStr = globalConst->GetHandledGetString();
    {
        ObjectOperator op(thread, obj.GetTaggedValue(), getStr.GetTaggedValue());
        if (op.IsFound()) {
            JSHandle<JSTaggedValue> getter = op.FastGetValue();
            if (!getter->IsCallable() && !getter->IsUndefined()) {
                THROW_TYPE_ERROR(thread, "getter not callable or undefined");
            }
            desc.SetGetter(getter);
        }
    }

    // 19. Let hasSet be HasProperty(Obj, "set").
    JSHandle<JSTaggedValue> setStr = globalConst->GetHandledSetString();
    {
        ObjectOperator op(thread, obj.GetTaggedValue(), setStr.GetTaggedValue());
        if (op.IsFound()) {
            JSHandle<JSTaggedValue> setter = op.FastGetValue();
            if (!setter->IsCallable() && !setter->IsUndefined()) {
                THROW_TYPE_ERROR(thread, "setter not callable or undefined");
            }
            desc.SetSetter(setter);
        }
    }

    // 22. If either desc.[[Get]] or desc.[[Set]] is present, then
    if (desc.IsAccessorDescriptor()) {
        // 22a. If either desc.[[Value]] or desc.[[Writable]] is present, throw a TypeError exception.
        if (desc.HasValue() || desc.HasWritable()) {
            THROW_TYPE_ERROR(thread, "either desc.[[Value]] or desc.[[Writable]] is present");
        }
    }
    // 23. Return desc.
}

JSHandle<JSTaggedValue> JSObject::SpeciesConstructor(JSThread *thread, const JSHandle<JSObject> &obj,
                                                     const JSHandle<JSTaggedValue> &defaultConstructort)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    // Assert: Type(O) is Object.
    ASSERT_PRINT(obj->IsECMAObject(), "obj must be js object");

    // Let C be Get(O, "constructor").
    JSHandle<JSTaggedValue> contructorKey = globalConst->GetHandledConstructorString();
    JSHandle<JSTaggedValue> objConstructor(GetProperty(thread, JSHandle<JSTaggedValue>(obj), contructorKey).GetValue());
    // ReturnIfAbrupt(C).
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    // If C is undefined, return defaultConstructor.
    if (objConstructor->IsUndefined()) {
        return defaultConstructort;
    }
    // If Type(C) is not Object, throw a TypeError exception.
    if (!objConstructor->IsECMAObject()) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Constructor is not Object",
                                    JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
    }
    // Let S be Get(C, @@species).
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    JSHandle<JSTaggedValue> speciesConstructor(GetProperty(thread, objConstructor, speciesSymbol).GetValue());
    // ReturnIfAbrupt(S).
    RETURN_HANDLE_IF_ABRUPT_COMPLETION(JSTaggedValue, thread);
    // If S is either undefined or null, return defaultConstructor.
    if (speciesConstructor->IsUndefined() || speciesConstructor->IsNull()) {
        return defaultConstructort;
    }
    // If IsConstructor(S) is true, return S.
    if (speciesConstructor->IsConstructor()) {
        return speciesConstructor;
    }
    // Throw a TypeError exception.
    THROW_TYPE_ERROR_AND_RETURN(thread, "Is not Constructor",
                                JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception()));
    return JSHandle<JSTaggedValue>(thread, JSTaggedValue::Exception());
}

// 6.2.4.6 CompletePropertyDescriptor ( Desc )
void PropertyDescriptor::CompletePropertyDescriptor(const JSThread *thread, PropertyDescriptor &desc)
{
    // 1. ReturnIfAbrupt(Desc).
    // 2. Assert: Desc is a Property Descriptor
    // 3. Let like be Record{[[Value]]: undefined, [[Writable]]: false, [[Get]]: undefined, [[Set]]: undefined,
    // [[Enumerable]]: false, [[Configurable]]: false}.
    // 4. If either IsGenericDescriptor(Desc) or IsDataDescriptor(Desc) is true, then
    if (!desc.IsAccessorDescriptor()) {
        // a. If Desc does not have a [[Value]] field, set Desc.[[Value]] to like.[[Value]].
        // b. If Desc does not have a [[Writable]] field, set Desc.[[Writable]] to like.[[Writable]].
        if (!desc.HasValue()) {
            desc.SetValue(JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
        }
        if (!desc.HasWritable()) {
            desc.SetWritable(false);
        }
    } else {
        // a. If Desc does not have a [[Get]] field, set Desc.[[Get]] to like.[[Get]].
        // b. If Desc does not have a [[Set]] field, set Desc.[[Set]] to like.[[Set]].
        // Default value of Get and Set is undefined.
    }
    // 6. If Desc does not have an [[Enumerable]] field, set Desc.[[Enumerable]] to like.[[Enumerable]].
    // 7. If Desc does not have a [[Configurable]] field, set Desc.[[Configurable]] to like.[[Configurable]].
    if (!desc.HasEnumerable()) {
        desc.SetEnumerable(false);
    }
    if (!desc.HasConfigurable()) {
        desc.SetConfigurable(false);
    }
}

// 13.7.5.15 EnumerateObjectProperties ( O )
JSHandle<JSForInIterator> JSObject::EnumerateObjectProperties(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    // 1. Return an Iterator object (25.1.1.2) whose next method iterates over all the String-valued keys of
    // enumerable properties of O. The Iterator object must inherit from %IteratorPrototype% (25.1.2). The
    // mechanics and order of enumerating the properties is not specified but must conform to the rules specified
    // below.
    JSHandle<JSTaggedValue> object;
    if (obj->IsString()) {
        object = JSHandle<JSTaggedValue>::Cast(JSPrimitiveRef::StringCreate(thread, obj));
    } else {
        object = JSTaggedValue::ToPrototypeOrObj(thread, obj);
    }

    return thread->GetEcmaVM()->GetFactory()->NewJSForinIterator(object);
}

void JSObject::DefinePropertyByLiteral(JSThread *thread, const JSHandle<JSObject> &obj,
                                       const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &value,
                                       bool useForClass)
{
    ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    PropertyAttributes attr = useForClass ? PropertyAttributes::Default(true, false, true)
                                          : PropertyAttributes::Default();

    if (value->IsAccessorData()) {
        attr.SetIsAccessor(true);
    }

    uint32_t index = 0;
    if (UNLIKELY(JSTaggedValue::ToElementIndex(key.GetTaggedValue(), &index))) {
        AddElementInternal(thread, obj, index, value, attr);
        return;
    }
    UNREACHABLE();
}

void JSObject::DefineSetter(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                            const JSHandle<JSTaggedValue> &value)
{
    ASSERT_PRINT(!(obj->IsUndefined() || obj->IsNull() || obj->IsHole()), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    ObjectOperator op(thread, obj, key, OperatorType::OWN);
    ASSERT(op.IsFound());
    op.DefineSetter(value);
}

void JSObject::DefineGetter(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                            const JSHandle<JSTaggedValue> &value)
{
    ASSERT_PRINT(!(obj->IsUndefined() || obj->IsNull() || obj->IsHole()), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    ObjectOperator op(thread, obj, key, OperatorType::OWN);
    ASSERT(op.IsFound());
    op.DefineGetter(value);
}

JSHandle<JSObject> JSObject::CreateObjectFromProperties(const JSThread *thread, const JSHandle<TaggedArray> &properties)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    size_t length = properties->GetLength();
    uint32_t propsLen = 0;
    for (size_t i = 0; i < length; i += 2) {  // 2: skip a pair of key and value
        if (properties->Get(i).IsHole()) {
            break;
        }
        propsLen++;
    }
    if (propsLen <= PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES) {
        JSHandle<JSObject> obj = factory->GetObjectLiteralByHClass(properties, propsLen);
        ASSERT_PRINT(obj->IsECMAObject(), "Obj is not a valid object");
        for (size_t i = 0; i < propsLen; i++) {
            // 2: literal contains a pair of key-value
            obj->SetPropertyInlinedProps(thread, i, properties->Get(i * 2 + 1));
        }
        return obj;
    } else {
        JSHandle<JSObject> obj = factory->NewEmptyJSObject();
        JSHClass::TransitionToDictionary(thread, obj);

        JSMutableHandle<NameDictionary> dict(
            thread, NameDictionary::Create(thread, NameDictionary::ComputeHashTableSize(propsLen)));
        JSMutableHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
        for (size_t i = 0; i < propsLen; i++) {
            PropertyAttributes attr = PropertyAttributes::Default();
            // 2: literal contains a pair of key-value
            valueHandle.Update(properties->Get(i * 2 + 1));
            // 2: literal contains a pair of key-value
            keyHandle.Update(properties->Get(i * 2));
            JSHandle<NameDictionary> newDict = NameDictionary::PutIfAbsent(thread, dict, keyHandle, valueHandle, attr);
            dict.Update(newDict);
        }
        obj->SetProperties(thread, dict);
        return obj;
    }
}

void JSObject::AddAccessor(JSThread *thread, const JSHandle<JSTaggedValue> &obj, const JSHandle<JSTaggedValue> &key,
                           const JSHandle<AccessorData> &value, PropertyAttributes attr)
{
    ASSERT_PRINT(!(obj->IsUndefined() || obj->IsNull() || obj->IsHole()), "Obj is not a valid object");
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    ASSERT_PRINT(attr.IsAccessor(), "Attr is not AccessorData");
    ObjectOperator op(thread, obj, key, OperatorType::OWN);
    ASSERT(!op.IsFound());
    op.AddProperty(JSHandle<JSObject>::Cast(obj), JSHandle<JSTaggedValue>(value), attr);
}

bool JSObject::UpdatePropertyInDictionary(const JSThread *thread, JSTaggedValue key, JSTaggedValue value)
{
    [[maybe_unused]] DisallowGarbageCollection noGc;
    NameDictionary *dict = NameDictionary::Cast(GetProperties().GetTaggedObject());
    int entry = dict->FindEntry(key);
    if (entry == -1) {
        return false;
    }
    dict->UpdateValue(thread, entry, value);
    return true;
}

void ECMAObject::SetHash(int32_t hash)
{
    JSTaggedType hashField = Barriers::GetDynValue<JSTaggedType>(this, HASH_OFFSET);
    JSTaggedValue value(hashField);
    if (value.IsHeapObject()) {
        JSThread *thread = this->GetJSThread();
        ASSERT(value.IsTaggedArray());
        TaggedArray *array = TaggedArray::Cast(value.GetHeapObject());
        array->Set(thread, 0, JSTaggedValue(hash));
    } else {
        Barriers::SetDynPrimitive<JSTaggedType>(this, HASH_OFFSET, JSTaggedValue(hash).GetRawData());
    }
}

int32_t ECMAObject::GetHash() const
{
    JSTaggedType hashField = Barriers::GetDynValue<JSTaggedType>(this, HASH_OFFSET);
    JSTaggedValue value(hashField);
    if (value.IsHeapObject()) {
        TaggedArray *array = TaggedArray::Cast(value.GetHeapObject());
        return array->Get(0).GetInt();
    }
    JSThread *thread = this->GetJSThread();
    JSHandle<JSTaggedValue> valueHandle(thread, value);
    return JSTaggedValue::ToInt32(thread, valueHandle);
}

void *ECMAObject::GetNativePointerField(int32_t index) const
{
    JSTaggedType hashField = Barriers::GetDynValue<JSTaggedType>(this, HASH_OFFSET);
    JSTaggedValue value(hashField);
    if (value.IsHeapObject()) {
        JSThread *thread = this->GetJSThread();
        JSHandle<TaggedArray> array(thread, value);
        if (static_cast<int32_t>(array->GetLength()) > index + 1) {
            JSHandle<JSNativePointer> pointer(thread, array->Get(index + 1));
            return pointer->GetExternalPointer();
        }
    }
    return nullptr;
}

void ECMAObject::SetNativePointerField(int32_t index, void *nativePointer,
    const DeleteEntryPoint &callBack, void *data)
{
    JSTaggedType hashField = Barriers::GetDynValue<JSTaggedType>(this, HASH_OFFSET);
    JSTaggedValue value(hashField);
    if (value.IsHeapObject()) {
        JSThread *thread = this->GetJSThread();
        JSHandle<TaggedArray> array(thread, value);
        if (static_cast<int32_t>(array->GetLength()) > index + 1) {
            EcmaVM *vm = thread->GetEcmaVM();
            JSHandle<JSTaggedValue> current = JSHandle<JSTaggedValue>(thread, array->Get(thread, index + 1));
            if (!current->IsHole() && nativePointer == nullptr) {
                // Try to remove native pointer if exists.
                vm->RemoveFromNativePointerList(*JSHandle<JSNativePointer>(current));
                array->Set(thread, index + 1, JSTaggedValue::Hole());
            } else {
                JSHandle<JSNativePointer> pointer = vm->GetFactory()->NewJSNativePointer(
                    nativePointer, callBack, data);
                array->Set(thread, index + 1, pointer.GetTaggedValue());
            }
        }
    }
}

int32_t ECMAObject::GetNativePointerFieldCount() const
{
    int32_t len = 0;
    JSTaggedType hashField = Barriers::GetDynValue<JSTaggedType>(this, HASH_OFFSET);
    JSTaggedValue value(hashField);
    if (value.IsHeapObject()) {
        TaggedArray *array = TaggedArray::Cast(value.GetHeapObject());
        len = array->GetLength() - 1;
    }
    return len;
}

void ECMAObject::SetNativePointerFieldCount(int32_t count)
{
    JSTaggedType hashField = Barriers::GetDynValue<JSTaggedType>(this, HASH_OFFSET);
    JSTaggedValue value(hashField);
    if (!value.IsHeapObject()) {
        JSThread *thread = this->GetJSThread();
        JSHandle<ECMAObject> obj(thread, this);
        JSHandle<TaggedArray> newArray = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(count + 1);
        newArray->Set(thread, 0, value);
        Barriers::SetDynObject<true>(thread, *obj, HASH_OFFSET, newArray.GetTaggedValue().GetRawData());
    }
}
}  // namespace panda::ecmascript
