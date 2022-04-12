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

#ifndef ECMASCRIPT_INTERPRETER_FAST_RUNTIME_STUB_INL_H
#define ECMASCRIPT_INTERPRETER_FAST_RUNTIME_STUB_INL_H

#include "ecmascript/interpreter/fast_runtime_stub.h"

#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_api_arraylist.h"
#include "ecmascript/js_api_deque.h"
#include "ecmascript/js_api_plain_array.h"
#include "ecmascript/js_api_queue.h"
#include "ecmascript/js_api_stack.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/tagged_dictionary.h"

namespace panda::ecmascript {
JSTaggedValue FastRuntimeStub::FastAdd(JSTaggedValue left, JSTaggedValue right)
{
    if (left.IsNumber() && right.IsNumber()) {
        return JSTaggedValue(left.GetNumber() + right.GetNumber());
    }

    return JSTaggedValue::Hole();
}

JSTaggedValue FastRuntimeStub::FastSub(JSTaggedValue left, JSTaggedValue right)
{
    if (left.IsNumber() && right.IsNumber()) {
        return JSTaggedValue(left.GetNumber() - right.GetNumber());
    }

    return JSTaggedValue::Hole();
}

JSTaggedValue FastRuntimeStub::FastMul(JSTaggedValue left, JSTaggedValue right)
{
    if (left.IsNumber() && right.IsNumber()) {
        return JSTaggedValue(left.GetNumber() * right.GetNumber());
    }

    return JSTaggedValue::Hole();
}

JSTaggedValue FastRuntimeStub::FastDiv(JSTaggedValue left, JSTaggedValue right)
{
    if (left.IsNumber() && right.IsNumber()) {
        double dLeft = left.IsInt() ? left.GetInt() : left.GetDouble();
        double dRight = right.IsInt() ? right.GetInt() : right.GetDouble();
        if (UNLIKELY(dRight == 0.0)) {
            if (dLeft == 0.0 || std::isnan(dLeft)) {
                return JSTaggedValue(base::NAN_VALUE);
            }
            uint64_t flagBit = ((bit_cast<uint64_t>(dLeft)) ^ (bit_cast<uint64_t>(dRight))) & base::DOUBLE_SIGN_MASK;
            return JSTaggedValue(bit_cast<double>(flagBit ^ (bit_cast<uint64_t>(base::POSITIVE_INFINITY))));
        }
        return JSTaggedValue(dLeft / dRight);
    }
    return JSTaggedValue::Hole();
}

JSTaggedValue FastRuntimeStub::FastMod(JSTaggedValue left, JSTaggedValue right)
{
    if (right.IsInt() && left.IsInt()) {
        int iRight = right.GetInt();
        int iLeft = left.GetInt();
        if (iRight > 0 && iLeft > 0) {
            return JSTaggedValue(iLeft % iRight);
        }
    }
    if (left.IsNumber() && right.IsNumber()) {
        double dLeft = left.IsInt() ? left.GetInt() : left.GetDouble();
        double dRight = right.IsInt() ? right.GetInt() : right.GetDouble();
        if (dRight == 0.0 || std::isnan(dRight) || std::isnan(dLeft) || std::isinf(dLeft)) {
            return JSTaggedValue(base::NAN_VALUE);
        }
        if (dLeft == 0.0 || std::isinf(dRight)) {
            return JSTaggedValue(dLeft);
        }
        return JSTaggedValue(std::fmod(dLeft, dRight));
    }
    return JSTaggedValue::Hole();
}

JSTaggedValue FastRuntimeStub::FastEqual(JSTaggedValue left, JSTaggedValue right)
{
    if (left == right) {
        if (UNLIKELY(left.IsDouble())) {
            return JSTaggedValue(!std::isnan(left.GetDouble()));
        }
        return JSTaggedValue::True();
    }
    if (left.IsNumber()) {
        if (left.IsInt() && right.IsInt()) {
            return JSTaggedValue::False();
        }
    }
    if (right.IsUndefinedOrNull()) {
        if (left.IsHeapObject()) {
            return JSTaggedValue::False();
        }
        if (left.IsUndefinedOrNull()) {
            return JSTaggedValue::True();
        }
    }
    if (left.IsBoolean()) {
        if (right.IsSpecial()) {
            return JSTaggedValue::False();
        }
    }
    if (left.IsBigInt() && right.IsBigInt()) {
        return JSTaggedValue(BigInt::Equal(left, right));
    }
    return JSTaggedValue::Hole();
}

bool FastRuntimeStub::FastStrictEqual(JSTaggedValue left, JSTaggedValue right)
{
    if (left.IsNumber()) {
        if (right.IsNumber()) {
            double dLeft = left.IsInt() ? left.GetInt() : left.GetDouble();
            double dRight = right.IsInt() ? right.GetInt() : right.GetDouble();
            return JSTaggedValue::StrictNumberEquals(dLeft, dRight);
        }
        return false;
    }
    if (right.IsNumber()) {
        return false;
    }
    if (left == right) {
        return true;
    }
    if (left.IsString() && right.IsString()) {
        return EcmaString::StringsAreEqual(static_cast<EcmaString *>(left.GetTaggedObject()),
                                           static_cast<EcmaString *>(right.GetTaggedObject()));
    }
    if (left.IsBigInt()) {
        if (right.IsBigInt()) {
            return BigInt::Equal(left, right);
        }
        return false;
    }
    if (right.IsBigInt()) {
        return false;
    }
    return false;
}

bool FastRuntimeStub::IsSpecialIndexedObj(JSType jsType)
{
    return jsType > JSType::JS_ARRAY;
}

bool FastRuntimeStub::IsSpecialReceiverObj(JSType jsType)
{
    return jsType > JSType::JS_PRIMITIVE_REF;
}

bool FastRuntimeStub::IsSpecialContainer(JSType jsType)
{
    return jsType >= JSType::JS_API_ARRAY_LIST && jsType <= JSType::JS_API_QUEUE;
}

int32_t FastRuntimeStub::TryToElementsIndex(JSTaggedValue key)
{
    if (LIKELY(key.IsInt())) {
        return key.GetInt();
    }
    if (key.IsString()) {
        uint32_t index = 0;
        if (JSTaggedValue::StringToElementIndex(key, &index)) {
            return static_cast<int32_t>(index);
        }
    } else if (key.IsDouble()) {
        double number = key.GetDouble();
        auto integer = static_cast<int32_t>(number);
        if (number == integer) {
            return integer;
        }
    }
    return -1;
}

JSTaggedValue FastRuntimeStub::CallGetter(JSThread *thread, JSTaggedValue receiver, JSTaggedValue holder,
                                          JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, CallGetter);
    // Accessor
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    AccessorData *accessor = AccessorData::Cast(value.GetTaggedObject());
    if (UNLIKELY(accessor->IsInternal())) {
        JSHandle<JSObject> objHandle(thread, holder);
        return accessor->CallInternalGet(thread, objHandle);
    }
    JSHandle<JSTaggedValue> objHandle(thread, receiver);
    return JSObject::CallGetter(thread, accessor, objHandle);
}

JSTaggedValue FastRuntimeStub::CallSetter(JSThread *thread, JSTaggedValue receiver, JSTaggedValue value,
                                          JSTaggedValue accessorValue)
{
    INTERPRETER_TRACE(thread, CallSetter);
    // Accessor
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> objHandle(thread, receiver);
    JSHandle<JSTaggedValue> valueHandle(thread, value);

    auto accessor = AccessorData::Cast(accessorValue.GetTaggedObject());
    bool success = JSObject::CallSetter(thread, *accessor, objHandle, valueHandle, true);
    return success ? JSTaggedValue::Undefined() : JSTaggedValue::Exception();
}

bool FastRuntimeStub::ShouldCallSetter(JSTaggedValue receiver, JSTaggedValue holder, JSTaggedValue accessorValue,
                                       PropertyAttributes attr)
{
    if (!AccessorData::Cast(accessorValue.GetTaggedObject())->IsInternal()) {
        return true;
    }
    if (receiver != holder) {
        return false;
    }
    return attr.IsWritable();
}

PropertyAttributes FastRuntimeStub::AddPropertyByName(JSThread *thread, JSHandle<JSObject> objHandle,
                                                      JSHandle<JSTaggedValue> keyHandle,
                                                      JSHandle<JSTaggedValue> valueHandle,
                                                      PropertyAttributes attr)
{
    INTERPRETER_TRACE(thread, AddPropertyByName);

    if (objHandle->IsJSArray() && keyHandle.GetTaggedValue() == thread->GlobalConstants()->GetConstructorString()) {
        objHandle->GetJSHClass()->SetHasConstructor(true);
    }
    int32_t nextInlinedPropsIndex = objHandle->GetJSHClass()->GetNextInlinedPropsIndex();
    if (nextInlinedPropsIndex >= 0) {
        objHandle->SetPropertyInlinedProps(thread, nextInlinedPropsIndex, valueHandle.GetTaggedValue());
        attr.SetOffset(nextInlinedPropsIndex);
        attr.SetIsInlinedProps(true);
        JSHClass::AddProperty(thread, objHandle, keyHandle, attr);
        return attr;
    }

    JSMutableHandle<TaggedArray> array(thread, objHandle->GetProperties());
    uint32_t length = array->GetLength();
    if (length == 0) {
        length = JSObject::MIN_PROPERTIES_LENGTH;
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        array.Update(factory->NewTaggedArray(length).GetTaggedValue());
        objHandle->SetProperties(thread, array.GetTaggedValue());
    }

    if (!array->IsDictionaryMode()) {
        attr.SetIsInlinedProps(false);

        uint32_t nonInlinedProps = objHandle->GetJSHClass()->GetNextNonInlinedPropsIndex();
        ASSERT(length >= nonInlinedProps);
        // if array is full, grow array or change to dictionary mode
        if (length == nonInlinedProps) {
            if (UNLIKELY(length == JSHClass::MAX_CAPACITY_OF_OUT_OBJECTS)) {
                // change to dictionary and add one.
                JSHandle<NameDictionary> dict(JSObject::TransitionToDictionary(thread, objHandle));
                JSHandle<NameDictionary> newDict =
                    NameDictionary::PutIfAbsent(thread, dict, keyHandle, valueHandle, attr);
                objHandle->SetProperties(thread, newDict);
                // index is not essential when fastMode is false;
                return attr;
            }
            // Grow properties array size
            uint32_t capacity = JSObject::ComputePropertyCapacity(length);
            ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
            array.Update(factory->CopyArray(array, length, capacity).GetTaggedValue());
            objHandle->SetProperties(thread, array.GetTaggedValue());
        }

        attr.SetOffset(nonInlinedProps + objHandle->GetJSHClass()->GetInlinedProperties());
        JSHClass::AddProperty(thread, objHandle, keyHandle, attr);
        array->Set(thread, nonInlinedProps, valueHandle.GetTaggedValue());
    } else {
        JSHandle<NameDictionary> dictHandle(array);
        JSHandle<NameDictionary> newDict =
            NameDictionary::PutIfAbsent(thread, dictHandle, keyHandle, valueHandle, attr);
        objHandle->SetProperties(thread, newDict);
    }
    return attr;
}

JSTaggedValue FastRuntimeStub::AddPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                                  JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, AddPropertyByIndex);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    if (UNLIKELY(!JSObject::Cast(receiver)->IsExtensible())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot add property in prevent extensions ", JSTaggedValue::Exception());
    }

    bool success = JSObject::AddElementInternal(thread, JSHandle<JSObject>(thread, receiver), index,
                                                JSHandle<JSTaggedValue>(thread, value), PropertyAttributes::Default());
    return success ? JSTaggedValue::Undefined() : JSTaggedValue::Exception();
}

template<bool UseOwn>
JSTaggedValue FastRuntimeStub::GetPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index)
{
    INTERPRETER_TRACE(thread, GetPropertyByIndex);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSTaggedValue holder = receiver;
    do {
        auto *hclass = holder.GetTaggedObject()->GetClass();
        JSType jsType = hclass->GetObjectType();
        if (IsSpecialIndexedObj(jsType)) {
            if (IsSpecialContainer(jsType)) {
                return GetContainerProperty(thread, holder, index, jsType);
            }
            return JSTaggedValue::Hole();
        }
        TaggedArray *elements = TaggedArray::Cast(JSObject::Cast(holder)->GetElements().GetTaggedObject());

        if (!hclass->IsDictionaryElement()) {
            ASSERT(!elements->IsDictionaryMode());
            if (index < elements->GetLength()) {
                JSTaggedValue value = elements->Get(index);
                if (!value.IsHole()) {
                    return value;
                }
            } else {
                return JSTaggedValue::Hole();
            }
        } else {
            NumberDictionary *dict = NumberDictionary::Cast(elements);
            int entry = dict->FindEntry(JSTaggedValue(static_cast<int>(index)));
            if (entry != -1) {
                auto attr = dict->GetAttributes(entry);
                auto value = dict->GetValue(entry);
                if (UNLIKELY(attr.IsAccessor())) {
                    return CallGetter(thread, receiver, holder, value);
                }
                ASSERT(!value.IsAccessor());
                return value;
            }
        }
        if (UseOwn) {
            break;
        }
        holder = JSObject::Cast(holder)->GetJSHClass()->GetPrototype();
    } while (holder.IsHeapObject());

    // not found
    return JSTaggedValue::Undefined();
}

template<bool UseOwn>
JSTaggedValue FastRuntimeStub::GetPropertyByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key)
{
    INTERPRETER_TRACE(thread, GetPropertyByValue);
    if (UNLIKELY(!key.IsNumber() && !key.IsStringOrSymbol())) {
        return JSTaggedValue::Hole();
    }
    // fast path
    auto index = TryToElementsIndex(key);
    if (LIKELY(index >= 0)) {
        return GetPropertyByIndex<UseOwn>(thread, receiver, index);
    }
    if (!key.IsNumber()) {
        if (key.IsString() && !EcmaString::Cast(key.GetTaggedObject())->IsInternString()) {
            // update string stable
            [[maybe_unused]] EcmaHandleScope handleScope(thread);
            JSHandle<JSTaggedValue> receiverHandler(thread, receiver);
            key = JSTaggedValue(thread->GetEcmaVM()->GetFactory()->InternString(JSHandle<JSTaggedValue>(thread, key)));
            // Maybe moved by GC
            receiver = receiverHandler.GetTaggedValue();
        }
        return FastRuntimeStub::GetPropertyByName<UseOwn>(thread, receiver, key);
    }
    return JSTaggedValue::Hole();
}

template<bool UseOwn>
JSTaggedValue FastRuntimeStub::GetPropertyByName(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key)
{
    INTERPRETER_TRACE(thread, GetPropertyByName);
    // no gc when return hole
    ASSERT(key.IsStringOrSymbol());
    JSTaggedValue holder = receiver;
    do {
        auto *hclass = holder.GetTaggedObject()->GetClass();
        JSType jsType = hclass->GetObjectType();
        if (IsSpecialIndexedObj(jsType)) {
            return JSTaggedValue::Hole();
        }

        if (LIKELY(!hclass->IsDictionaryMode())) {
            ASSERT(!TaggedArray::Cast(JSObject::Cast(holder)->GetProperties().GetTaggedObject())->IsDictionaryMode());

            LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
            int propsNumber = hclass->NumberOfProps();
            int entry = layoutInfo->FindElementWithCache(thread, hclass, key, propsNumber);
            if (entry != -1) {
                PropertyAttributes attr(layoutInfo->GetAttr(entry));
                ASSERT(static_cast<int>(attr.GetOffset()) == entry);
                auto value = JSObject::Cast(holder)->GetProperty(hclass, attr);
                if (UNLIKELY(attr.IsAccessor())) {
                    return CallGetter(thread, receiver, holder, value);
                }
                ASSERT(!value.IsAccessor());
                return value;
            }
        } else {
            TaggedArray *array = TaggedArray::Cast(JSObject::Cast(holder)->GetProperties().GetTaggedObject());
            ASSERT(array->IsDictionaryMode());
            NameDictionary *dict = NameDictionary::Cast(array);
            int entry = dict->FindEntry(key);
            if (entry != -1) {
                auto value = dict->GetValue(entry);
                auto attr = dict->GetAttributes(entry);
                if (UNLIKELY(attr.IsAccessor())) {
                    return CallGetter(thread, receiver, holder, value);
                }
                ASSERT(!value.IsAccessor());
                return value;
            }
        }
        if (UseOwn) {
            break;
        }
        holder = hclass->GetPrototype();
    } while (holder.IsHeapObject());
    // not found
    return JSTaggedValue::Undefined();
}

template<bool UseOwn>
JSTaggedValue FastRuntimeStub::SetPropertyByName(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                                 JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, SetPropertyByName);
    // property
    JSTaggedValue holder = receiver;
    do {
        auto *hclass = holder.GetTaggedObject()->GetClass();
        JSType jsType = hclass->GetObjectType();
        if (IsSpecialIndexedObj(jsType)) {
            if (IsSpecialContainer(jsType)) {
                THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot set property on Container", JSTaggedValue::Exception());
            }
            return JSTaggedValue::Hole();
        }
        // UpdateRepresentation
        if (LIKELY(!hclass->IsDictionaryMode())) {
            ASSERT(!TaggedArray::Cast(JSObject::Cast(holder)->GetProperties().GetTaggedObject())->IsDictionaryMode());

            LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());

            int propsNumber = hclass->NumberOfProps();
            int entry = layoutInfo->FindElementWithCache(thread, hclass, key, propsNumber);
            if (entry != -1) {
                PropertyAttributes attr(layoutInfo->GetAttr(entry));
                ASSERT(static_cast<int>(attr.GetOffset()) == entry);
                if (UNLIKELY(attr.IsAccessor())) {
                    auto accessor = JSObject::Cast(holder)->GetProperty(hclass, attr);
                    if (ShouldCallSetter(receiver, holder, accessor, attr)) {
                        return CallSetter(thread, receiver, value, accessor);
                    }
                }
                if (UNLIKELY(!attr.IsWritable())) {
                    [[maybe_unused]] EcmaHandleScope handleScope(thread);
                    THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot set readonly property", JSTaggedValue::Exception());
                }
                if (UNLIKELY(holder != receiver)) {
                    break;
                }
                JSObject::Cast(holder)->SetProperty(thread, hclass, attr, value);
                return JSTaggedValue::Undefined();
            }
        } else {
            TaggedArray *properties = TaggedArray::Cast(JSObject::Cast(holder)->GetProperties().GetTaggedObject());
            ASSERT(properties->IsDictionaryMode());
            NameDictionary *dict = NameDictionary::Cast(properties);
            int entry = dict->FindEntry(key);
            if (entry != -1) {
                auto attr = dict->GetAttributes(entry);
                if (UNLIKELY(attr.IsAccessor())) {
                    auto accessor = dict->GetValue(entry);
                    if (ShouldCallSetter(receiver, holder, accessor, attr)) {
                        return CallSetter(thread, receiver, value, accessor);
                    }
                }
                if (UNLIKELY(!attr.IsWritable())) {
                    [[maybe_unused]] EcmaHandleScope handleScope(thread);
                    THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot set readonly property", JSTaggedValue::Exception());
                }
                if (UNLIKELY(holder != receiver)) {
                    break;
                }
                dict->UpdateValue(thread, entry, value);
                return JSTaggedValue::Undefined();
            }
        }
        if (UseOwn) {
            break;
        }
        holder = hclass->GetPrototype();
    } while (holder.IsHeapObject());

    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSObject> objHandle(thread, receiver);
    JSHandle<JSTaggedValue> keyHandle(thread, key);
    JSHandle<JSTaggedValue> valueHandle(thread, value);

    if (UNLIKELY(!JSObject::Cast(receiver)->IsExtensible())) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot add property in prevent extensions ", JSTaggedValue::Exception());
    }
    AddPropertyByName(thread, objHandle, keyHandle, valueHandle, PropertyAttributes::Default());
    return JSTaggedValue::Undefined();
}

template<bool UseOwn>
JSTaggedValue FastRuntimeStub::SetPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                                  JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, SetPropertyByIndex);
    JSTaggedValue holder = receiver;
    do {
        auto *hclass = holder.GetTaggedObject()->GetClass();
        JSType jsType = hclass->GetObjectType();
        if (IsSpecialIndexedObj(jsType)) {
            if (IsSpecialContainer(jsType)) {
                return SetContainerProperty(thread, holder, index, value, jsType);
            }
            return JSTaggedValue::Hole();
        }
        TaggedArray *elements = TaggedArray::Cast(JSObject::Cast(holder)->GetElements().GetTaggedObject());
        if (!hclass->IsDictionaryElement()) {
            ASSERT(!elements->IsDictionaryMode());
            if (UNLIKELY(holder != receiver)) {
                break;
            }
            if (index < elements->GetLength()) {
                if (!elements->Get(index).IsHole()) {
                    elements->Set(thread, index, value);
                    return JSTaggedValue::Undefined();
                }
            }
        } else {
            return JSTaggedValue::Hole();
        }
        if (UseOwn) {
            break;
        }
        holder = JSObject::Cast(holder)->GetJSHClass()->GetPrototype();
    } while (holder.IsHeapObject());

    return AddPropertyByIndex(thread, receiver, index, value);
}

template<bool UseOwn>
JSTaggedValue FastRuntimeStub::SetPropertyByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                                  JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, SetPropertyByValue);
    if (UNLIKELY(!key.IsNumber() && !key.IsStringOrSymbol())) {
        return JSTaggedValue::Hole();
    }
    // fast path
    auto index = TryToElementsIndex(key);
    if (LIKELY(index >= 0)) {
        return SetPropertyByIndex<UseOwn>(thread, receiver, index, value);
    }
    if (!key.IsNumber()) {
        if (key.IsString() && !EcmaString::Cast(key.GetTaggedObject())->IsInternString()) {
            // update string stable
            [[maybe_unused]] EcmaHandleScope handleScope(thread);
            JSHandle<JSTaggedValue> receiverHandler(thread, receiver);
            JSHandle<JSTaggedValue> valueHandler(thread, value);
            key = JSTaggedValue(thread->GetEcmaVM()->GetFactory()->InternString(JSHandle<JSTaggedValue>(thread, key)));
            // Maybe moved by GC
            receiver = receiverHandler.GetTaggedValue();
            value = valueHandler.GetTaggedValue();
        }
        return FastRuntimeStub::SetPropertyByName<UseOwn>(thread, receiver, key, value);
    }
    return JSTaggedValue::Hole();
}

JSTaggedValue FastRuntimeStub::GetGlobalOwnProperty(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key)
{
    JSObject *obj = JSObject::Cast(receiver);
    TaggedArray *properties = TaggedArray::Cast(obj->GetProperties().GetTaggedObject());
    GlobalDictionary *dict = GlobalDictionary::Cast(properties);
    int entry = dict->FindEntry(key);
    if (entry != -1) {
        auto value = dict->GetValue(entry);
        if (UNLIKELY(value.IsAccessor())) {
            return CallGetter(thread, receiver, receiver, value);
        }
        ASSERT(!value.IsAccessor());
        return value;
    }
    return JSTaggedValue::Hole();
}

JSTaggedValue FastRuntimeStub::FastTypeOf(JSThread *thread, JSTaggedValue obj)
{
    INTERPRETER_TRACE(thread, FastTypeOf);
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    switch (obj.GetRawData()) {
        case JSTaggedValue::VALUE_TRUE:
        case JSTaggedValue::VALUE_FALSE:
            return globalConst->GetBooleanString();
        case JSTaggedValue::VALUE_NULL:
            return globalConst->GetObjectString();
        case JSTaggedValue::VALUE_UNDEFINED:
            return globalConst->GetUndefinedString();
        default:
            if (obj.IsHeapObject()) {
                if (obj.IsString()) {
                    return globalConst->GetStringString();
                }
                if (obj.IsSymbol()) {
                    return globalConst->GetSymbolString();
                }
                if (obj.IsCallable()) {
                    return globalConst->GetFunctionString();
                }
                if (obj.IsBigInt()) {
                    return globalConst->GetBigIntString();
                }
                return globalConst->GetObjectString();
            }
            if (obj.IsNumber()) {
                return globalConst->GetNumberString();
            }
    }
    return globalConst->GetUndefinedString();
}

bool FastRuntimeStub::FastSetPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                             JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, FastSetPropertyByIndex);
#ifdef ECMASCRIPT_ENABLE_STUB_AOT1
    auto stubAddr = thread->GetFastStubEntry(CommonStubCSigns::SetPropertyByIndex);
    typedef JSTaggedValue (*PFSetPropertyByIndex)(uintptr_t, JSTaggedValue, uint32_t, JSTaggedValue);
    auto setPropertyByIndex = reinterpret_cast<PFSetPropertyByIndex>(stubAddr);
    JSTaggedValue result = setPropertyByIndex(thread->GetGlueAddr(), receiver, index, value);
#else
    JSTaggedValue result = FastRuntimeStub::SetPropertyByIndex(thread, receiver, index, value);
#endif
    if (!result.IsHole()) {
        return result != JSTaggedValue::Exception();
    }
    return JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>(thread, receiver), index,
                                      JSHandle<JSTaggedValue>(thread, value), true);
}

bool FastRuntimeStub::FastSetPropertyByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                             JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, FastSetPropertyByValue);
    JSTaggedValue result = FastRuntimeStub::SetPropertyByValue(thread, receiver, key, value);
    if (!result.IsHole()) {
        return result != JSTaggedValue::Exception();
    }
    return JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>(thread, receiver),
                                      JSHandle<JSTaggedValue>(thread, key), JSHandle<JSTaggedValue>(thread, value),
                                      true);
}

// must not use for interpreter
JSTaggedValue FastRuntimeStub::FastGetPropertyByName(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key)
{
    INTERPRETER_TRACE(thread, FastGetPropertyByName);
    ASSERT(key.IsStringOrSymbol());
    if (key.IsString() && !EcmaString::Cast(key.GetTaggedObject())->IsInternString()) {
        JSHandle<JSTaggedValue> receiverHandler(thread, receiver);
        key = JSTaggedValue(thread->GetEcmaVM()->GetFactory()->InternString(JSHandle<JSTaggedValue>(thread, key)));
        // Maybe moved by GC
        receiver = receiverHandler.GetTaggedValue();
    }
#ifdef ECMASCRIPT_ENABLE_STUB_AOT1
    auto stubAddr = thread->GetFastStubEntry(CommonStubCSigns::GetPropertyByName);
    typedef JSTaggedValue (*PFGetPropertyByName)(uintptr_t, JSTaggedValue, JSTaggedValue);
    auto getPropertyByNamePtr = reinterpret_cast<PFGetPropertyByName>(stubAddr);
    JSTaggedValue result = getPropertyByNamePtr(thread->GetGlueAddr(), receiver, key);
#else
    JSTaggedValue result = FastRuntimeStub::GetPropertyByName(thread, receiver, key);
#endif
    if (result.IsHole()) {
        return JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>(thread, receiver),
                                          JSHandle<JSTaggedValue>(thread, key))
            .GetValue()
            .GetTaggedValue();
    }
    return result;
}

JSTaggedValue FastRuntimeStub::FastGetPropertyByValue(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key)
{
    INTERPRETER_TRACE(thread, FastGetPropertyByValue);
#ifdef ECMASCRIPT_ENABLE_STUB_AOT1
    auto stubAddr = thread->GetFastStubEntry(CommonStubCSigns::GetPropertyByValue);
    typedef JSTaggedValue (*PFGetPropertyByValue)(uintptr_t, JSTaggedValue, JSTaggedValue);
    auto getPropertyByValuePtr = reinterpret_cast<PFGetPropertyByValue>(stubAddr);
    JSTaggedValue result = getPropertyByValuePtr(thread->GetGlueAddr(), receiver, key);
#else
    JSTaggedValue result = FastRuntimeStub::GetPropertyByValue(thread, receiver, key);
#endif
    if (result.IsHole()) {
        return JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>(thread, receiver),
                                          JSHandle<JSTaggedValue>(thread, key))
            .GetValue()
            .GetTaggedValue();
    }
    return result;
}

template<bool UseHole>  // UseHole is only for Array::Sort() which requires Hole order
JSTaggedValue FastRuntimeStub::FastGetPropertyByIndex(JSThread *thread, JSTaggedValue receiver, uint32_t index)
{
    INTERPRETER_TRACE(thread, FastGetPropertyByIndex);
#ifdef ECMASCRIPT_ENABLE_STUB_AOT1
    auto stubAddr = thread->GetFastStubEntry(CommonStubCSigns::GetPropertyByIndex);
    typedef JSTaggedValue (*PFGetPropertyByIndex)(uintptr_t, JSTaggedValue, uint32_t);
    auto getPropertyByIndex = reinterpret_cast<PFGetPropertyByIndex>(stubAddr);
    JSTaggedValue result = getPropertyByIndex(thread->GetGlueAddr(), receiver, index);
#else
    JSTaggedValue result = FastRuntimeStub::GetPropertyByIndex(thread, receiver, index);
#endif
    if (result.IsHole() && !UseHole) {
        return JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>(thread, receiver), index)
            .GetValue()
            .GetTaggedValue();
    }
    return result;
}

JSTaggedValue FastRuntimeStub::NewLexicalEnvDyn(JSThread *thread, ObjectFactory *factory, uint16_t numVars)
{
    INTERPRETER_TRACE(thread, NewLexicalEnvDyn);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    LexicalEnv *newEnv = factory->InlineNewLexicalEnv(numVars);
    if (UNLIKELY(newEnv == nullptr)) {
        return JSTaggedValue::Hole();
    }
    JSTaggedValue currentLexenv = thread->GetCurrentLexenv();
    newEnv->SetParentEnv(thread, currentLexenv);
    newEnv->SetScopeInfo(thread, JSTaggedValue::Hole());
    return JSTaggedValue(newEnv);
}

// Those interface below is discarded
bool FastRuntimeStub::IsSpecialIndexedObjForGet(JSTaggedValue obj)
{
    JSType jsType = obj.GetTaggedObject()->GetClass()->GetObjectType();
    return jsType > JSType::JS_ARRAY && jsType <= JSType::JS_PRIMITIVE_REF;
}

bool FastRuntimeStub::IsSpecialIndexedObjForSet(JSTaggedValue obj)
{
    JSType jsType = obj.GetTaggedObject()->GetClass()->GetObjectType();
    return jsType >= JSType::JS_ARRAY && jsType <= JSType::JS_PRIMITIVE_REF;
}

JSTaggedValue FastRuntimeStub::GetElement(JSTaggedValue receiver, uint32_t index)
{
    JSTaggedValue holder = receiver;
    while (true) {
        JSTaggedValue val = FindOwnElement(JSObject::Cast(holder), index);
        if (!val.IsHole()) {
            return val;
        }

        holder = JSObject::Cast(holder)->GetJSHClass()->GetPrototype();
        if (!holder.IsHeapObject()) {
            return JSTaggedValue::Undefined();
        }
    }
}

JSTaggedValue FastRuntimeStub::GetElementWithArray(JSTaggedValue receiver, uint32_t index)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue holder = receiver;
    while (true) {
        JSTaggedValue val = FindOwnElement(JSObject::Cast(holder), index);
        if (!val.IsHole()) {
            return val;
        }

        holder = JSObject::Cast(holder)->GetJSHClass()->GetPrototype();
        if (!holder.IsHeapObject()) {
            return val;
        }
    }
}

bool FastRuntimeStub::SetElement(JSThread *thread, JSTaggedValue receiver, uint32_t index, JSTaggedValue value,
                                 bool mayThrow)
{
    INTERPRETER_TRACE(thread, SetElement);
    JSTaggedValue holder = receiver;
    bool onPrototype = false;

    while (true) {
        PropertyAttributes attr;
        uint32_t indexOrEntry = 0;
        TaggedArray *elements = TaggedArray::Cast(JSObject::Cast(holder)->GetElements().GetHeapObject());
        bool isDict = elements->IsDictionaryMode();
        JSTaggedValue val = FindOwnElement(elements, index, isDict, &attr, &indexOrEntry);
        if (!val.IsHole()) {
            if (UNLIKELY(onPrototype)) {
                if (UNLIKELY(!JSObject::Cast(receiver)->IsExtensible())) {
                    if (mayThrow) {
                        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot add property in prevent extensions ", false);
                    }
                    return false;
                }

                return JSObject::AddElementInternal(thread, JSHandle<JSObject>(thread, receiver), index,
                                                    JSHandle<JSTaggedValue>(thread, value),
                                                    PropertyAttributes::Default());
            }
            if (!attr.IsAccessor()) {
                if (attr.IsWritable()) {
                    elements = TaggedArray::Cast(JSObject::Cast(receiver)->GetElements().GetHeapObject());
                    if (!isDict) {
                        elements->Set(thread, indexOrEntry, value);
                        JSObject::Cast(receiver)->GetJSHClass()->UpdateRepresentation(value);
                        return true;
                    }
                    NumberDictionary::Cast(elements)->UpdateValueAndAttributes(thread, indexOrEntry, value, attr);
                    return true;
                }

                if (mayThrow) {
                    THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot set readonly property", false);
                }
                return false;
            }

            // Accessor
            [[maybe_unused]] EcmaHandleScope handleScope(thread);
            JSHandle<JSTaggedValue> objHandle(thread, receiver);
            JSHandle<JSTaggedValue> valueHandle(thread, value);
            AccessorData *access = AccessorData::Cast(val.GetHeapObject());
            return JSObject::CallSetter(thread, *access, objHandle, valueHandle, mayThrow);
        }

        holder = JSObject::Cast(holder)->GetJSHClass()->GetPrototype();
        if (!holder.IsHeapObject()) {
            if (UNLIKELY(!JSObject::Cast(receiver)->IsExtensible())) {
                if (mayThrow) {
                    THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot add property in prevent extensions ", false);
                }
                return false;
            }

            return JSObject::AddElementInternal(thread, JSHandle<JSObject>(thread, receiver), index,
                                                JSHandle<JSTaggedValue>(thread, value), PropertyAttributes::Default());
        }
        if (holder.IsJSProxy()) {
            return JSProxy::SetProperty(
                thread, JSHandle<JSProxy>(thread, holder), JSHandle<JSTaggedValue>(thread, JSTaggedValue(index)),
                JSHandle<JSTaggedValue>(thread, value), JSHandle<JSTaggedValue>(thread, receiver), mayThrow);
        }
        onPrototype = true;
    }
}

bool FastRuntimeStub::SetPropertyByName(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                        JSTaggedValue value, bool mayThrow)
{
    INTERPRETER_TRACE(thread, SetPropertyByName);
    // property
    JSTaggedValue holder = receiver;
    bool onPrototype = false;

    while (true) {
        TaggedArray *properties = TaggedArray::Cast(JSObject::Cast(holder)->GetProperties().GetHeapObject());
        PropertyAttributes attr;
        uint32_t indexOrEntry = 0;
        JSTaggedValue val = FindOwnProperty(thread, JSObject::Cast(holder), properties, key, &attr, &indexOrEntry);
        if (!val.IsHole()) {
            if (!attr.IsAccessor()) {
                if (UNLIKELY(onPrototype)) {
                    if (UNLIKELY(!JSObject::Cast(receiver)->IsExtensible() || !attr.IsWritable())) {
                        if (mayThrow) {
                            THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot add property in prevent extensions ", false);
                        }
                        return false;
                    }
                    [[maybe_unused]] EcmaHandleScope handleScope(thread);
                    ObjectOperator::FastAdd(thread, receiver, key, JSHandle<JSTaggedValue>(thread, value),
                                            PropertyAttributes::Default());

                    return true;
                }

                if (attr.IsWritable()) {
                    properties = TaggedArray::Cast(JSObject::Cast(receiver)->GetProperties().GetHeapObject());
                    if (!properties->IsDictionaryMode()) {
                        Representation representation =
                            PropertyAttributes::UpdateRepresentation(attr.GetRepresentation(), value);
                        if (attr.GetRepresentation() != representation) {
                            attr.SetRepresentation(representation);
                        }

                        JSObject::Cast(receiver)->GetJSHClass()->UpdatePropertyMetaData(thread, key, attr);
                        if (UNLIKELY(val.IsInternalAccessor())) {
                            [[maybe_unused]] EcmaHandleScope handleScope(thread);
                            AccessorData::Cast(val.GetHeapObject())
                                ->CallInternalSet(thread, JSHandle<JSObject>(thread, receiver),
                                                  JSHandle<JSTaggedValue>(thread, value), mayThrow);
                            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
                            return true;
                        }

                        if (attr.IsInlinedProps()) {
                            JSObject::Cast(receiver)->SetPropertyInlinedProps(thread, indexOrEntry, value);
                        } else {
                            properties->Set(thread, indexOrEntry, value);
                        }
                        return true;
                    }

                    if (receiver.IsJSGlobalObject()) {
                        [[maybe_unused]] EcmaHandleScope handleScope(thread);
                        JSHandle<GlobalDictionary> dictHandle(thread, properties);
                        // globalobj have no internal accessor
                        GlobalDictionary::InvalidatePropertyBox(thread, dictHandle, indexOrEntry, attr);
                        return true;
                    }

                    if (UNLIKELY(val.IsInternalAccessor())) {
                        [[maybe_unused]] EcmaHandleScope handleScope(thread);
                        AccessorData::Cast(val.GetHeapObject())
                            ->CallInternalSet(thread, JSHandle<JSObject>(thread, receiver),
                                              JSHandle<JSTaggedValue>(thread, value), mayThrow);
                        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
                        return true;
                    }

                    NameDictionary::Cast(properties)->UpdateValueAndAttributes(thread, indexOrEntry, value, attr);
                    return true;
                }

                if (mayThrow) {
                    THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot set readonly property", false);
                }
                return false;
            }

            // Accessor
            [[maybe_unused]] EcmaHandleScope handleScope(thread);
            JSHandle<JSTaggedValue> objHandle(thread, receiver);
            JSHandle<JSTaggedValue> valueHandle(thread, value);
            AccessorData *access = AccessorData::Cast(val.GetHeapObject());
            return JSObject::CallSetter(thread, *access, objHandle, valueHandle, mayThrow);
        }

        if (holder.IsTypedArray()) {
            [[maybe_unused]] EcmaHandleScope handleScope(thread);
            return JSTypedArray::SetProperty(thread, JSHandle<JSTaggedValue>(thread, holder),
                                             JSTypedArray::ToPropKey(thread, JSHandle<JSTaggedValue>(thread, key)),
                                             JSHandle<JSTaggedValue>(thread, value),
                                             JSHandle<JSTaggedValue>(thread, receiver), mayThrow);
        }

        holder = JSObject::Cast(holder)->GetJSHClass()->GetPrototype();
        if (!holder.IsHeapObject()) {
            if (UNLIKELY(!JSObject::Cast(receiver)->IsExtensible())) {
                if (mayThrow) {
                    THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot add property in prevent extensions ", false);
                }
                return false;
            }
            [[maybe_unused]] EcmaHandleScope handleScope(thread);
            ObjectOperator::FastAdd(thread, receiver, key, JSHandle<JSTaggedValue>(thread, value),
                                    PropertyAttributes::Default());

            return true;
        }
        if (holder.IsJSProxy()) {
            [[maybe_unused]] EcmaHandleScope handleScope(thread);
            return JSProxy::SetProperty(thread, JSHandle<JSProxy>(thread, holder), JSHandle<JSTaggedValue>(thread, key),
                                        JSHandle<JSTaggedValue>(thread, value),
                                        JSHandle<JSTaggedValue>(thread, receiver), mayThrow);
        }
        onPrototype = true;
    }
}

bool FastRuntimeStub::SetGlobalOwnProperty(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                           JSTaggedValue value, bool mayThrow)
{
    INTERPRETER_TRACE(thread, SetGlobalOwnProperty);
    uint32_t index = 0;
    if (JSTaggedValue::ToElementIndex(key, &index)) {
        return SetElement(thread, receiver, index, value, mayThrow);
    }

    JSObject *obj = JSObject::Cast(receiver);
    GlobalDictionary *dict = GlobalDictionary::Cast(obj->GetProperties().GetTaggedObject());
    PropertyAttributes attr = PropertyAttributes::Default();
    if (UNLIKELY(dict->GetLength() == 0)) {
        JSHandle<JSTaggedValue> keyHandle(thread, key);
        JSHandle<JSTaggedValue> valHandle(thread, value);
        JSHandle<JSObject> objHandle(thread, obj);
        JSHandle<GlobalDictionary> dictHandle(GlobalDictionary::Create(thread));

        // Add PropertyBox to global dictionary
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<PropertyBox> boxHandle = factory->NewPropertyBox(valHandle);
        boxHandle->SetValue(thread, valHandle.GetTaggedValue());
        PropertyBoxType boxType = valHandle->IsUndefined() ? PropertyBoxType::UNDEFINED : PropertyBoxType::CONSTANT;
        attr.SetBoxType(boxType);

        JSHandle<GlobalDictionary> properties =
            GlobalDictionary::PutIfAbsent(thread, dictHandle, keyHandle, JSHandle<JSTaggedValue>(boxHandle), attr);
        objHandle->SetProperties(thread, properties);
        return true;
    }

    int entry = dict->FindEntry(key);
    if (entry != -1) {
        attr = dict->GetAttributes(entry);
        JSTaggedValue val = dict->GetValue(entry);
        if (!attr.IsAccessor()) {
            if (attr.IsWritable()) {
                // globalobj have no internal accessor
                JSHandle<GlobalDictionary> dictHandle(thread, dict);
                GlobalDictionary::InvalidatePropertyBox(thread, dictHandle, entry, attr);
                return true;
            }
        }

        // Accessor
        JSTaggedValue setter = AccessorData::Cast(val.GetHeapObject())->GetSetter();
        if (setter.IsUndefined()) {
            if (mayThrow) {
                THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot set property when setter is undefined", false);
            }
            return false;
        }

        JSHandle<JSTaggedValue> objHandle(thread, receiver);
        JSHandle<JSTaggedValue> setFunc(thread, setter);
        JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
        EcmaRuntimeCallInfo info = EcmaInterpreter::NewRuntimeCallInfo(thread, setFunc, objHandle, undefined, 1);
        info.SetCallArg(value);
        JSFunction::Call(&info);
        // 10. ReturnIfAbrupt(setterResult).
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
        return true;
    }

    JSHandle<JSTaggedValue> keyHandle(thread, key);
    JSHandle<JSTaggedValue> valHandle(thread, value);
    JSHandle<JSObject> objHandle(thread, obj);
    JSHandle<GlobalDictionary> dictHandle(thread, dict);

    // Add PropertyBox to global dictionary
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<PropertyBox> boxHandle = factory->NewPropertyBox(keyHandle);
    boxHandle->SetValue(thread, valHandle.GetTaggedValue());
    PropertyBoxType boxType = valHandle->IsUndefined() ? PropertyBoxType::UNDEFINED : PropertyBoxType::CONSTANT;
    attr.SetBoxType(boxType);

    JSHandle<GlobalDictionary> properties =
        GlobalDictionary::PutIfAbsent(thread, dictHandle, keyHandle, JSHandle<JSTaggedValue>(boxHandle), attr);
    objHandle->SetProperties(thread, properties);
    return true;
}

// set property that is not accessor and is writable
void FastRuntimeStub::SetOwnPropertyByName(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key,
                                           JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, SetOwnPropertyByName);
    TaggedArray *properties = TaggedArray::Cast(JSObject::Cast(receiver)->GetProperties().GetHeapObject());
    PropertyAttributes attr;
    uint32_t indexOrEntry;
    JSTaggedValue val = FindOwnProperty(thread, JSObject::Cast(receiver), properties, key, &attr, &indexOrEntry);
    if (!val.IsHole()) {
        ASSERT(!attr.IsAccessor() && attr.IsWritable());
        if (!properties->IsDictionaryMode()) {
            Representation representation = PropertyAttributes::UpdateRepresentation(attr.GetRepresentation(), value);
            if (attr.GetRepresentation() != representation) {
                attr.SetRepresentation(representation);
            }

            JSObject::Cast(receiver)->GetJSHClass()->UpdatePropertyMetaData(thread, key, attr);

            if (attr.IsInlinedProps()) {
                JSObject::Cast(receiver)->SetPropertyInlinedProps(thread, indexOrEntry, value);
            } else {
                properties->Set(thread, indexOrEntry, value);
            }
            return;
        }

        NameDictionary::Cast(properties)->UpdateValueAndAttributes(thread, indexOrEntry, value, attr);
        return;
    }
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ObjectOperator::FastAdd(thread, receiver, key, JSHandle<JSTaggedValue>(thread, value),
                            PropertyAttributes::Default());
}

// set element that is not accessor and is writable
bool FastRuntimeStub::SetOwnElement(JSThread *thread, JSTaggedValue receiver, uint32_t index, JSTaggedValue value)
{
    INTERPRETER_TRACE(thread, SetOwnElement);
    PropertyAttributes attr;
    uint32_t indexOrEntry;
    TaggedArray *elements = TaggedArray::Cast(JSObject::Cast(receiver)->GetElements().GetHeapObject());
    bool isDict = elements->IsDictionaryMode();
    [[maybe_unused]] JSTaggedValue val = FindOwnElement(elements, index, isDict, &attr, &indexOrEntry);
    if (!val.IsHole()) {
        ASSERT(!attr.IsAccessor() && attr.IsWritable());
        if (!isDict) {
            elements->Set(thread, indexOrEntry, value);
            JSObject::Cast(receiver)->GetJSHClass()->UpdateRepresentation(value);
            return true;
        }
        NumberDictionary::Cast(elements)->UpdateValueAndAttributes(thread, indexOrEntry, value, attr);
        return true;
    }

    return JSObject::AddElementInternal(thread, JSHandle<JSObject>(thread, receiver), index,
                                        JSHandle<JSTaggedValue>(thread, value), PropertyAttributes::Default());
}

bool FastRuntimeStub::FastSetProperty(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key, JSTaggedValue value,
                                      bool mayThrow)
{
    INTERPRETER_TRACE(thread, FastSetProperty);
    if (receiver.IsJSObject() && !receiver.IsTypedArray() && (key.IsStringOrSymbol())) {
        uint32_t index = 0;
        if (UNLIKELY(JSTaggedValue::ToElementIndex(key, &index))) {
            if (!FastRuntimeStub::IsSpecialIndexedObjForSet(receiver)) {
                return FastRuntimeStub::SetElement(thread, receiver, index, value, true);
            }
            return JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>(thread, receiver),
                                              JSHandle<JSTaggedValue>(thread, key),
                                              JSHandle<JSTaggedValue>(thread, value), mayThrow);
        }
        if (key.IsString()) {
            key = JSTaggedValue(thread->GetEcmaVM()->GetFactory()->InternString(JSHandle<JSTaggedValue>(thread, key)));
        }
        return FastRuntimeStub::SetPropertyByName(thread, receiver, key, value, mayThrow);
    }
    return JSTaggedValue::SetProperty(thread, JSHandle<JSTaggedValue>(thread, receiver),
                                      JSHandle<JSTaggedValue>(thread, key), JSHandle<JSTaggedValue>(thread, value),
                                      mayThrow);
}

JSTaggedValue FastRuntimeStub::FastGetProperty(JSThread *thread, JSTaggedValue receiver, JSTaggedValue key)
{
    INTERPRETER_TRACE(thread, FastGetProperty);
    JSTaggedValue result;
    if (receiver.IsJSObject() && !receiver.IsTypedArray() && (key.IsStringOrSymbol())) {
        uint32_t index = 0;
        if (UNLIKELY(JSTaggedValue::ToElementIndex(key, &index))) {
            if (FastRuntimeStub::IsSpecialIndexedObjForSet(receiver)) {
                result = JSTaggedValue::Hole();
            } else {
                result = FastRuntimeStub::GetElement(receiver, index);
            }
        } else {
            if (key.IsString()) {
                key = JSTaggedValue(
                    thread->GetEcmaVM()->GetFactory()->InternString(JSHandle<JSTaggedValue>(thread, key)));
            }
            result = FastRuntimeStub::GetPropertyByName(thread, receiver, key);
        }
    }
    if (!result.IsHole()) {
        if (UNLIKELY(result.IsAccessor())) {
            return JSObject::CallGetter(thread, AccessorData::Cast(result.GetHeapObject()),
                                        JSHandle<JSTaggedValue>(thread, receiver));
        }
        return result;
    }
    return JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>(thread, receiver),
                                      JSHandle<JSTaggedValue>(thread, key))
        .GetValue()
        .GetTaggedValue();
}

JSTaggedValue FastRuntimeStub::FindOwnProperty(JSThread *thread, JSObject *obj, TaggedArray *properties,
                                               JSTaggedValue key, PropertyAttributes *attr, uint32_t *indexOrEntry)
{
    INTERPRETER_TRACE(thread, FindOwnProperty);
    if (!properties->IsDictionaryMode()) {
        JSHClass *cls = obj->GetJSHClass();
        JSTaggedValue attrs = cls->GetLayout();
        if (!attrs.IsNull()) {
            LayoutInfo *layoutInfo = LayoutInfo::Cast(attrs.GetHeapObject());
            int propNumber = cls->NumberOfProps();
            int entry = layoutInfo->FindElementWithCache(thread, cls, key, propNumber);
            if (entry != -1) {
                *attr = layoutInfo->GetAttr(entry);
                ASSERT(entry == static_cast<int>(attr->GetOffset()));
                *indexOrEntry = entry;
                if (attr->IsInlinedProps()) {
                    return obj->GetPropertyInlinedProps(entry);
                }
                *indexOrEntry -= cls->GetInlinedProperties();
                return properties->Get(*indexOrEntry);
            }
        }
        return JSTaggedValue::Hole();  // properties == empty properties will return here.
    }

    if (obj->IsJSGlobalObject()) {
        GlobalDictionary *dict = GlobalDictionary::Cast(properties);
        int entry = dict->FindEntry(key);
        if (entry != -1) {
            *indexOrEntry = entry;
            *attr = dict->GetAttributes(entry);
            return dict->GetValue(entry);
        }
        return JSTaggedValue::Hole();
    }

    NameDictionary *dict = NameDictionary::Cast(properties);
    int entry = dict->FindEntry(key);
    if (entry != -1) {
        *indexOrEntry = entry;
        *attr = dict->GetAttributes(entry);
        return dict->GetValue(entry);
    }

    return JSTaggedValue::Hole();
}

JSTaggedValue FastRuntimeStub::FindOwnElement(TaggedArray *elements, uint32_t index, bool isDict,
                                              PropertyAttributes *attr, uint32_t *indexOrEntry)
{
    if (!isDict) {
        if (elements->GetLength() <= index) {
            return JSTaggedValue::Hole();
        }

        JSTaggedValue value = elements->Get(index);
        if (!value.IsHole()) {
            *attr = PropertyAttributes::Default();
            *indexOrEntry = index;
            return value;
        }
    } else {
        NumberDictionary *dict = NumberDictionary::Cast(elements);
        int entry = dict->FindEntry(JSTaggedValue(static_cast<int>(index)));
        if (entry != -1) {
            *indexOrEntry = entry;
            *attr = dict->GetAttributes(entry);
            return dict->GetValue(entry);
        }
    }
    return JSTaggedValue::Hole();
}

JSTaggedValue FastRuntimeStub::FindOwnProperty(JSThread *thread, JSObject *obj, JSTaggedValue key)
{
    INTERPRETER_TRACE(thread, FindOwnProperty);
    TaggedArray *array = TaggedArray::Cast(obj->GetProperties().GetHeapObject());
    if (!array->IsDictionaryMode()) {
        JSHClass *cls = obj->GetJSHClass();
        JSTaggedValue attrs = cls->GetLayout();
        if (!attrs.IsNull()) {
            LayoutInfo *layoutInfo = LayoutInfo::Cast(attrs.GetHeapObject());
            int propsNumber = cls->NumberOfProps();
            int entry = layoutInfo->FindElementWithCache(thread, cls, key, propsNumber);
            if (entry != -1) {
                PropertyAttributes attr(layoutInfo->GetAttr(entry));
                ASSERT(static_cast<int>(attr.GetOffset()) == entry);
                return attr.IsInlinedProps() ? obj->GetPropertyInlinedProps(entry)
                                             : array->Get(entry - cls->GetInlinedProperties());
            }
        }
        return JSTaggedValue::Hole();  // array == empty array will return here.
    }

    if (obj->IsJSGlobalObject()) {
        GlobalDictionary *dict = GlobalDictionary::Cast(array);
        int entry = dict->FindEntry(key);
        if (entry != -1) {
            return dict->GetValue(entry);
        }
        return JSTaggedValue::Hole();
    }

    NameDictionary *dict = NameDictionary::Cast(array);
    int entry = dict->FindEntry(key);
    if (entry != -1) {
        return dict->GetValue(entry);
    }

    return JSTaggedValue::Hole();
}

JSTaggedValue FastRuntimeStub::FindOwnElement(JSObject *obj, uint32_t index)
{
    TaggedArray *elements = TaggedArray::Cast(JSObject::Cast(obj)->GetElements().GetHeapObject());

    if (!elements->IsDictionaryMode()) {
        if (elements->GetLength() <= index) {
            return JSTaggedValue::Hole();
        }

        JSTaggedValue value = elements->Get(index);
        if (!value.IsHole()) {
            return value;
        }
    } else {
        NumberDictionary *dict = NumberDictionary::Cast(elements);
        int entry = dict->FindEntry(JSTaggedValue(static_cast<int>(index)));
        if (entry != -1) {
            return dict->GetValue(entry);
        }
    }
    return JSTaggedValue::Hole();
}

JSTaggedValue FastRuntimeStub::HasOwnProperty(JSThread *thread, JSObject *obj, JSTaggedValue key)
{
    INTERPRETER_TRACE(thread, HasOwnProperty);
    uint32_t index = 0;
    if (UNLIKELY(JSTaggedValue::ToElementIndex(key, &index))) {
        return FastRuntimeStub::FindOwnElement(obj, index);
    }

    return FastRuntimeStub::FindOwnProperty(thread, obj, key);
}

JSTaggedValue FastRuntimeStub::GetContainerProperty(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                                    JSType jsType)
{
    JSTaggedValue res = JSTaggedValue::Undefined();
    switch (jsType) {
        case JSType::JS_API_ARRAY_LIST:
            res = JSAPIArrayList::Cast(receiver.GetTaggedObject())->Get(thread, index);
            break;
        case JSType::JS_API_QUEUE:
            res = JSAPIQueue::Cast(receiver.GetTaggedObject())->Get(thread, index);
            break;
        case JSType::JS_API_PLAIN_ARRAY:
            res = JSAPIPlainArray::Cast(receiver.GetTaggedObject())->Get(JSTaggedValue(index));
            break;
        case JSType::JS_API_DEQUE:
            res = JSAPIDeque::Cast(receiver.GetTaggedObject())->Get(index);
            break;
        case JSType::JS_API_STACK:
            res = JSAPIStack::Cast(receiver.GetTaggedObject())->Get(index);
            break;
        default:
            break;
    }
    return res;
}

JSTaggedValue FastRuntimeStub::SetContainerProperty(JSThread *thread, JSTaggedValue receiver, uint32_t index,
                                                    JSTaggedValue value, JSType jsType)
{
    JSTaggedValue res = JSTaggedValue::Undefined();
    switch (jsType) {
        case JSType::JS_API_ARRAY_LIST:
            res = JSAPIArrayList::Cast(receiver.GetTaggedObject())->Set(thread, index, value);
            break;
        case JSType::JS_API_QUEUE:
            res = JSAPIQueue::Cast(receiver.GetTaggedObject())->Set(thread, index, value);
            break;
        case JSType::JS_API_PLAIN_ARRAY:
            res = JSAPIPlainArray::Set(thread, JSHandle<JSAPIPlainArray> (thread, receiver), index, value);
            break;
        case JSType::JS_API_DEQUE:
            res = JSAPIDeque::Cast(receiver.GetTaggedObject())->Set(thread, index, value);
            break;
        case JSType::JS_API_STACK:
            res = JSAPIStack::Cast(receiver.GetTaggedObject())->Set(thread, index, value);
            break;
        default:
            break;
    }
    return res;
}

JSTaggedValue FastRuntimeStub::NewThisObject(JSThread *thread, JSTaggedValue ctor, JSTaggedValue newTarget,
                                             InterpretedFrame *state)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSFunction> ctorHandle(thread, ctor);
    JSHandle<JSTaggedValue> newTargetHandle(thread, newTarget);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(ctorHandle, newTargetHandle);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());

    state->function = ctorHandle.GetTaggedValue();
    state->constpool = ctorHandle->GetConstantPool();
    state->profileTypeInfo = ctorHandle->GetProfileTypeInfo();
    state->env = ctorHandle->GetLexicalEnv();

    return obj.GetTaggedValue();
}

JSTaggedValue FastRuntimeStub::NewThisObject(JSThread *thread, JSTaggedValue ctor, JSTaggedValue newTarget,
                                             AsmInterpretedFrame *state)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSFunction> ctorHandle(thread, ctor);
    JSHandle<JSTaggedValue> newTargetHandle(thread, newTarget);
    JSHandle<JSObject> obj = factory->NewJSObjectByConstructor(ctorHandle, newTargetHandle);
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception());

    state->function = ctorHandle.GetTaggedValue();
    state->env = ctorHandle->GetLexicalEnv();

    return obj.GetTaggedValue();
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_INTERPRETER_FAST_RUNTIME_STUB_INL_H
