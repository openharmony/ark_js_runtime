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

#include "runtime_trampolines.h"
#include "ecmascript/accessor_data.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/frames.h"
#include "ecmascript/ic/ic_runtime.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/ic/properties_cache.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/interpreter/interpreter_assembly.h"
#include "ecmascript/js_api_arraylist.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/layout_info.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/message_string.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/runtime_api.h"
#include "ecmascript/tagged_dictionary.h"
#include "libpandabase/utils/string_helpers.h"

namespace panda::ecmascript {
#define DEF_RUNTIME_TRAMPOLINES(name) \
JSTaggedType RuntimeTrampolines::name(uintptr_t argGlue, uint32_t argc, uintptr_t argv) \

#define RUNTIME_TRAMPOLINES_HEADER(name)                  \
    auto thread = JSThread::GlueToJSThread(argGlue);      \
    [[maybe_unused]] EcmaHandleScope handleScope(thread)  \

#define CONVERT_ARG_TAGGED_TYPE_CHECKED(name, index) \
    ASSERT((index) < argc);                          \
    JSTaggedType name = *(reinterpret_cast<JSTaggedType *>(argv) + (index))

#define CONVERT_ARG_TAGGED_CHECKED(name, index) \
    ASSERT((index) < argc);                     \
    JSTaggedValue name = JSTaggedValue(*(reinterpret_cast<JSTaggedType *>(argv) + (index)))

#define CONVERT_ARG_HANDLE_CHECKED(type, name, index) \
    ASSERT((index) < argc);                           \
    JSHandle<type> name(thread, JSTaggedValue(*(reinterpret_cast<JSTaggedType *>(argv) + (index))))

#define CONVERT_ARG_PTR_CHECKED(type, name, index) \
    ASSERT((index) < argc);                        \
    type name = reinterpret_cast<type>(*(reinterpret_cast<JSTaggedType *>(argv) + (index)))

DEF_RUNTIME_TRAMPOLINES(AddElementInternal)
{
    RUNTIME_TRAMPOLINES_HEADER(AddElementInternal);
    CONVERT_ARG_HANDLE_CHECKED(JSObject, receiver, 0);
    CONVERT_ARG_TAGGED_CHECKED(argIndex, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 2);
    CONVERT_ARG_TAGGED_CHECKED(argAttr, 3);
    auto attr = static_cast<PropertyAttributes>(argAttr.GetInt());
    auto result = JSObject::AddElementInternal(thread, receiver, argIndex.GetInt(), value, attr);
    return JSTaggedValue(result).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(CallSetter)
{
    RUNTIME_TRAMPOLINES_HEADER(CallSetter);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(argSetter, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, receiver, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 2);
    CONVERT_ARG_TAGGED_CHECKED(argMayThrow, 3);
    auto setter = AccessorData::Cast((reinterpret_cast<TaggedObject *>(argSetter)));
    auto result = JSObject::CallSetter(thread, *setter, receiver, value, argMayThrow.IsTrue());
    return JSTaggedValue(result).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(CallSetter2)
{
    RUNTIME_TRAMPOLINES_HEADER(CallSetter2);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, objHandle, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, valueHandle, 1);
    CONVERT_ARG_TAGGED_CHECKED(argAccessor, 2);
    auto accessor = AccessorData::Cast(argAccessor.GetTaggedObject());
    bool success = JSObject::CallSetter(thread, *accessor, objHandle, valueHandle, true);
    return success ? JSTaggedValue::Undefined().GetRawData() : JSTaggedValue::Exception().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(CallGetter2)
{
    RUNTIME_TRAMPOLINES_HEADER(CallGetter2);
    CONVERT_ARG_TAGGED_CHECKED(argReceiver, 0);
    CONVERT_ARG_TAGGED_CHECKED(argHolder, 1);
    CONVERT_ARG_TAGGED_CHECKED(argAccessor, 2);
    AccessorData *accessor = AccessorData::Cast(argAccessor.GetTaggedObject());
    if (UNLIKELY(accessor->IsInternal())) {
        JSHandle<JSObject> objHandle(thread, argHolder);
        return accessor->CallInternalGet(thread, objHandle).GetRawData();
    }
    JSHandle<JSTaggedValue> objHandle(thread, argReceiver);
    return JSObject::CallGetter(thread, accessor, objHandle).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(JSProxySetProperty)
{
    RUNTIME_TRAMPOLINES_HEADER(JSProxySetProperty);
    CONVERT_ARG_HANDLE_CHECKED(JSProxy, proxy, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, index, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 2);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, receiver, 3);
    CONVERT_ARG_TAGGED_CHECKED(argMayThrow, 4);
    auto result = JSProxy::SetProperty(thread, proxy, index, value, receiver, argMayThrow.IsTrue());
    return JSTaggedValue(result).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(GetHash32)
{
    CONVERT_ARG_TAGGED_CHECKED(argKey, 0);
    CONVERT_ARG_TAGGED_CHECKED(len, 1);
    int key = argKey.GetInt();
    auto pkey = reinterpret_cast<uint8_t *>(&key);
    uint32_t result = panda::GetHash32(pkey, len.GetInt());
    return JSTaggedValue(static_cast<uint64_t>(result)).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(CallGetter)
{
    RUNTIME_TRAMPOLINES_HEADER(CallGetter);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(argGetter, 0);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(argReceiver, 1);

    auto accessor = AccessorData::Cast(reinterpret_cast<TaggedObject *>(argGetter));
    JSHandle<JSTaggedValue> objHandle(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(argReceiver)));
    return JSObject::CallGetter(thread, accessor, objHandle).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(CallInternalGetter)
{
    RUNTIME_TRAMPOLINES_HEADER(CallInternalGetter);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(argAccessor, 0);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(argReceiver, 1);

    auto accessor = AccessorData::Cast(reinterpret_cast<TaggedObject *>(argAccessor));
    JSHandle<JSObject> objHandle(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(argReceiver)));
    return accessor->CallInternalGet(thread, objHandle).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(FindElementWithCache)
{
    RUNTIME_TRAMPOLINES_HEADER(FindElementWithCache);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(hClass, 0);
    CONVERT_ARG_TAGGED_CHECKED(key, 1);
    CONVERT_ARG_TAGGED_CHECKED(num, 2);

    auto cls  = reinterpret_cast<JSHClass *>(hClass);
    auto layoutInfo = LayoutInfo::Cast(cls->GetLayout().GetTaggedObject());
    PropertiesCache *cache = thread->GetPropertiesCache();
    int index = cache->Get(cls, key);
    if (index == PropertiesCache::NOT_FOUND) {
        index = layoutInfo->BinarySearch(key, num.GetInt());
        cache->Set(cls, key, index);
    }
    return JSTaggedValue(index).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(StringGetHashCode)
{
    CONVERT_ARG_TAGGED_TYPE_CHECKED(ecmaString, 0);
    auto string = reinterpret_cast<EcmaString *>(ecmaString);
    uint32_t result = string->GetHashcode();
    return JSTaggedValue(static_cast<uint64_t>(result)).GetRawData();
}

void RuntimeTrampolines::PrintHeapReginInfo(uintptr_t argGlue)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    thread->GetEcmaVM()->GetHeap()->GetNewSpace()->EnumerateRegions([](Region *current) {
        LOG_ECMA(INFO) << "semispace region: " << current << std::endl;
    });
    thread->GetEcmaVM()->GetHeap()->GetOldSpace()->EnumerateRegions([](Region *current) {
        LOG_ECMA(INFO) << "GetOldSpace region: " << current << std::endl;
    });
    thread->GetEcmaVM()->GetHeap()->GetNonMovableSpace()->EnumerateRegions([](Region *current) {
        LOG_ECMA(INFO) << "GetNonMovableSpace region: " << current << std::endl;
    });
    thread->GetEcmaVM()->GetHeap()->GetMachineCodeSpace()->EnumerateRegions([](Region *current) {
        LOG_ECMA(INFO) << "GetMachineCodeSpace region: " << current << std::endl;
    });
}

DEF_RUNTIME_TRAMPOLINES(GetTaggedArrayPtrTest)
{
    RUNTIME_TRAMPOLINES_HEADER(GetTaggedArrayPtrTest);
    // this case static static JSHandle<TaggedArray> arr don't free in first call
    // second call trigger gc.
    // don't call EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    static int i = 0;
    static JSHandle<TaggedArray> arr = factory->NewTaggedArray(2);
    if (i == 0) {
        arr->Set(thread, 0, JSTaggedValue(3.5)); // 3.5: first element
        arr->Set(thread, 1, JSTaggedValue(4.5)); // 4.5: second element
    }
#ifndef NDEBUG
    PrintHeapReginInfo(argGlue);
#endif
    if (i != 0) {
        thread->GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);
    }
    LOG_ECMA(INFO) << " arr->GetData() " << std::hex << "  " << arr->GetData();
    i++;
    return arr.GetTaggedValue().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(FloatMod)
{
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    double result = std::fmod(left.GetDouble(), right.GetDouble());
    return JSTaggedValue(result).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(NewInternalString)
{
    RUNTIME_TRAMPOLINES_HEADER(NewInternalString);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, keyHandle, 0);
    return JSTaggedValue(thread->GetEcmaVM()->GetFactory()->InternString(keyHandle)).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(NewTaggedArray)
{
    RUNTIME_TRAMPOLINES_HEADER(NewTaggedArray);
    CONVERT_ARG_TAGGED_CHECKED(length, 0);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewTaggedArray(length.GetInt()).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(CopyArray)
{
    RUNTIME_TRAMPOLINES_HEADER(CopyArray);
    CONVERT_ARG_HANDLE_CHECKED(TaggedArray, array, 0);
    CONVERT_ARG_TAGGED_CHECKED(length, 1);
    CONVERT_ARG_TAGGED_CHECKED(capacity, 2);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->CopyArray(array, length.GetInt(), capacity.GetInt()).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(NameDictPutIfAbsent)
{
    RUNTIME_TRAMPOLINES_HEADER(NameDictPutIfAbsent);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(receiver, 0);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(array, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, keyHandle, 2);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, valueHandle, 3);
    CONVERT_ARG_TAGGED_CHECKED(attr, 4);
    CONVERT_ARG_TAGGED_CHECKED(needTransToDict, 5);

    PropertyAttributes propAttr(attr.GetInt());
    if (needTransToDict.IsTrue()) {
        JSHandle<JSObject> objHandle(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(receiver)));
        JSHandle<NameDictionary> dictHandle(JSObject::TransitionToDictionary(thread, objHandle));
        return NameDictionary::
            PutIfAbsent(thread, dictHandle, keyHandle, valueHandle, propAttr).GetTaggedValue().GetRawData();
    } else {
        JSHandle<NameDictionary> dictHandle(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(array)));
        return NameDictionary::
            PutIfAbsent(thread, dictHandle, keyHandle, valueHandle, propAttr).GetTaggedValue().GetRawData();
    }
}

DEF_RUNTIME_TRAMPOLINES(PropertiesSetValue)
{
    RUNTIME_TRAMPOLINES_HEADER(PropertiesSetValue);
    CONVERT_ARG_HANDLE_CHECKED(JSObject, objHandle, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, valueHandle, 1);
    CONVERT_ARG_HANDLE_CHECKED(TaggedArray, arrayHandle, 2);
    CONVERT_ARG_TAGGED_CHECKED(taggedCapacity, 3);
    CONVERT_ARG_TAGGED_CHECKED(taggedIndex, 4);
    uint32_t capacity = taggedCapacity.GetInt();
    uint32_t index = taggedIndex.GetInt();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> properties;
    if (capacity == 0) {
        properties = factory->NewTaggedArray(JSObject::MIN_PROPERTIES_LENGTH);
    } else {
        properties = factory->CopyArray(arrayHandle, capacity, JSObject::ComputePropertyCapacity(capacity));
    }
    properties->Set(thread, index, valueHandle);
    objHandle->SetProperties(thread, properties);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(TaggedArraySetValue)
{
    RUNTIME_TRAMPOLINES_HEADER(TaggedArraySetValue);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(argReceiver, 0);
    CONVERT_ARG_TAGGED_CHECKED(value, 1);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(argElement, 2);
    CONVERT_ARG_TAGGED_CHECKED(taggedElementIndex, 3);
    CONVERT_ARG_TAGGED_CHECKED(taggedCapacity, 4);

    uint32_t elementIndex = taggedElementIndex.GetInt();
    uint32_t capacity = taggedCapacity.GetInt();
    auto elements = reinterpret_cast<TaggedArray *>(argElement);
    if (elementIndex >= capacity) {
        if (JSObject::ShouldTransToDict(capacity, elementIndex)) {
            return JSTaggedValue::Hole().GetRawData();
        }
        JSHandle<JSObject> receiverHandle(thread, reinterpret_cast<JSObject *>(argReceiver));
        JSHandle<JSTaggedValue> valueHandle(thread, value);
        elements = *JSObject::GrowElementsCapacity(thread, receiverHandle,
                                                   JSObject::ComputeElementCapacity(elementIndex + 1));
        receiverHandle->SetElements(thread, JSTaggedValue(elements));
        elements->Set(thread, elementIndex, valueHandle);
        return JSTaggedValue::Undefined().GetRawData();
    }
    elements->Set(thread, elementIndex, value);
    return JSTaggedValue::Undefined().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(NewEcmaDynClass)
{
    RUNTIME_TRAMPOLINES_HEADER(NewEcmaDynClass);
    CONVERT_ARG_TAGGED_CHECKED(size, 0);
    CONVERT_ARG_TAGGED_CHECKED(type, 1);
    CONVERT_ARG_TAGGED_CHECKED(inlinedProps, 2);
    return (thread->GetEcmaVM()->GetFactory()->NewEcmaDynClass(
        size.GetInt(), JSType(type.GetInt()), inlinedProps.GetInt())).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(UpdateLayOutAndAddTransition)
{
    RUNTIME_TRAMPOLINES_HEADER(UpdateLayOutAndAddTransition);
    CONVERT_ARG_HANDLE_CHECKED(JSHClass, oldHClassHandle, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSHClass, newHClassHandle, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, keyHandle, 2);
    CONVERT_ARG_TAGGED_CHECKED(attr, 3);

    auto factory = thread->GetEcmaVM()->GetFactory();
    PropertyAttributes attrValue(attr.GetInt());
    int offset = attrValue.GetOffset();
    newHClassHandle->IncNumberOfProps();

    {
        JSMutableHandle<LayoutInfo> layoutInfoHandle(thread, newHClassHandle->GetLayout());

        if (layoutInfoHandle->NumberOfElements() != offset) {
            layoutInfoHandle.Update(factory->CopyAndReSort(layoutInfoHandle, offset, offset + 1));
            newHClassHandle->SetLayout(thread, layoutInfoHandle);
        } else if (layoutInfoHandle->GetPropertiesCapacity() <= offset) {  // need to Grow
            layoutInfoHandle.Update(
                factory->ExtendLayoutInfo(layoutInfoHandle, LayoutInfo::ComputeGrowCapacity(offset)));
            newHClassHandle->SetLayout(thread, layoutInfoHandle);
        }
        layoutInfoHandle->AddKey(thread, offset, keyHandle.GetTaggedValue(), attrValue);
    }

    // 5. Add newDynclass to old dynclass's transitions.
    JSHClass::AddTransitions(thread, oldHClassHandle, newHClassHandle, keyHandle, attrValue);
    return JSTaggedValue::Hole().GetRawData();
}

void RuntimeTrampolines::DebugPrint(int fmtMessageId, ...)
{
    std::string format = MessageString::GetMessageString(fmtMessageId);
    va_list args;
    va_start(args, fmtMessageId);
    std::string result = panda::helpers::string::Vformat(format.c_str(), args);
    std::cerr << result << std::endl;
    va_end(args);
}

DEF_RUNTIME_TRAMPOLINES(NoticeThroughChainAndRefreshUser)
{
    RUNTIME_TRAMPOLINES_HEADER(NoticeThroughChainAndRefreshUser);
    CONVERT_ARG_HANDLE_CHECKED(JSHClass, oldHClassHandle, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSHClass, newHClassHandle, 1);

    JSHClass::NoticeThroughChain(thread, oldHClassHandle);
    JSHClass::RefreshUsers(thread, oldHClassHandle, newHClassHandle);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(IncDyn)
{
    RUNTIME_TRAMPOLINES_HEADER(IncDyn);
    CONVERT_ARG_TAGGED_CHECKED(value, 0);
    return SlowRuntimeStub::IncDyn(thread, value).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(DecDyn)
{
    RUNTIME_TRAMPOLINES_HEADER(DecDyn);
    CONVERT_ARG_TAGGED_CHECKED(value, 0);
    return SlowRuntimeStub::DecDyn(thread, value).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ExpDyn)
{
    RUNTIME_TRAMPOLINES_HEADER(ExpDyn);
    CONVERT_ARG_TAGGED_CHECKED(baseValue, 0);
    CONVERT_ARG_TAGGED_CHECKED(exponentValue, 1);

    if (baseValue.IsNumber() && exponentValue.IsNumber()) {
        // fast path
        double doubleBase = baseValue.IsInt() ? baseValue.GetInt() : baseValue.GetDouble();
        double doubleExponent = exponentValue.IsInt() ? exponentValue.GetInt() : exponentValue.GetDouble();
        if (std::abs(doubleBase) == 1 && std::isinf(doubleExponent)) {
            return JSTaggedValue(base::NAN_VALUE).GetRawData();
        }
        if ((doubleBase == 0 &&
            ((bit_cast<uint64_t>(doubleBase)) & base::DOUBLE_SIGN_MASK) == base::DOUBLE_SIGN_MASK) &&
            std::isfinite(doubleExponent) && base::NumberHelper::TruncateDouble(doubleExponent) == doubleExponent &&
            base::NumberHelper::TruncateDouble(doubleExponent / 2) + base::HALF ==  // 2 : half
            (doubleExponent / 2)) {  // 2 : half
            if (doubleExponent > 0) {
                return JSTaggedValue(-0.0).GetRawData();
            }
            if (doubleExponent < 0) {
                return JSTaggedValue(-base::POSITIVE_INFINITY).GetRawData();
            }
        }
        return JSTaggedValue(std::pow(doubleBase, doubleExponent)).GetRawData();
    }
    // slow path
    return SlowRuntimeStub::ExpDyn(thread, baseValue, exponentValue).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(IsInDyn)
{
    RUNTIME_TRAMPOLINES_HEADER(IsInDyn);
    CONVERT_ARG_TAGGED_CHECKED(prop, 0);
    CONVERT_ARG_TAGGED_CHECKED(obj, 1);
    return SlowRuntimeStub::IsInDyn(thread, prop, obj).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(InstanceOfDyn)
{
    RUNTIME_TRAMPOLINES_HEADER(InstanceOfDyn);
    CONVERT_ARG_TAGGED_CHECKED(obj, 0);
    CONVERT_ARG_TAGGED_CHECKED(target, 1);
    return SlowRuntimeStub::InstanceofDyn(thread, obj, target).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(FastStrictNotEqual)
{
    RUNTIME_TRAMPOLINES_HEADER(FastStrictNotEqual);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    bool result = FastRuntimeStub::FastStrictEqual(left, right);
    if (result) {
        return JSTaggedValue::False().GetRawData();
    }
    return JSTaggedValue::True().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(FastStrictEqual)
{
    RUNTIME_TRAMPOLINES_HEADER(FastStrictEqual);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    bool result = FastRuntimeStub::FastStrictEqual(left, right);
    if (result) {
        return JSTaggedValue::True().GetRawData();
    }
    return JSTaggedValue::False().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(CreateGeneratorObj)
{
    RUNTIME_TRAMPOLINES_HEADER(CreateGeneratorObj);
    CONVERT_ARG_TAGGED_CHECKED(genFunc, 0);
    return SlowRuntimeStub::CreateGeneratorObj(thread, genFunc).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(GetTemplateObject)
{
    RUNTIME_TRAMPOLINES_HEADER(GetTemplateObject);
    CONVERT_ARG_TAGGED_CHECKED(literal, 0);
    return SlowRuntimeStub::GetTemplateObject(thread, literal).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(GetNextPropName)
{
    RUNTIME_TRAMPOLINES_HEADER(GetNextPropName);
    CONVERT_ARG_TAGGED_CHECKED(iter, 0);
    return SlowRuntimeStub::GetNextPropName(thread, iter).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(IterNext)
{
    RUNTIME_TRAMPOLINES_HEADER(IterNext);
    CONVERT_ARG_TAGGED_CHECKED(iter, 0);
    return SlowRuntimeStub::IterNext(thread, iter).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(CloseIterator)
{
    RUNTIME_TRAMPOLINES_HEADER(CloseIterator);
    CONVERT_ARG_TAGGED_CHECKED(iter, 0);
    return SlowRuntimeStub::CloseIterator(thread, iter).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(CopyModule)
{
    RUNTIME_TRAMPOLINES_HEADER(CopyModule);
    CONVERT_ARG_TAGGED_CHECKED(srcModule, 0);
    SlowRuntimeStub::CopyModule(thread, srcModule);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(SuperCallSpread)
{
    RUNTIME_TRAMPOLINES_HEADER(SuperCallSpread);
    CONVERT_ARG_TAGGED_CHECKED(func, 0);
    CONVERT_ARG_TAGGED_CHECKED(array, 1);
    auto sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    JSTaggedValue function = EcmaInterpreter::GetNewTarget(sp);
    return SlowRuntimeStub::SuperCallSpread(thread, func, function, array).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(DelObjProp)
{
    RUNTIME_TRAMPOLINES_HEADER(DelObjProp);
    CONVERT_ARG_TAGGED_CHECKED(obj, 0);
    CONVERT_ARG_TAGGED_CHECKED(prop, 1);
    return SlowRuntimeStub::DelObjProp(thread, obj, prop).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(NewObjSpreadDyn)
{
    RUNTIME_TRAMPOLINES_HEADER(NewObjSpreadDyn);
    CONVERT_ARG_TAGGED_CHECKED(func, 0);
    CONVERT_ARG_TAGGED_CHECKED(newTarget, 1);
    CONVERT_ARG_TAGGED_CHECKED(array, 2);
    return SlowRuntimeStub::NewObjSpreadDyn(thread, func, newTarget, array).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(CreateIterResultObj)
{
    RUNTIME_TRAMPOLINES_HEADER(CreateIterResultObj);
    CONVERT_ARG_TAGGED_CHECKED(value, 0);
    CONVERT_ARG_TAGGED_CHECKED(flag, 1);
    return SlowRuntimeStub::CreateIterResultObj(thread, value, flag).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(AsyncFunctionAwaitUncaught)
{
    RUNTIME_TRAMPOLINES_HEADER(AsyncFunctionAwaitUncaught);
    CONVERT_ARG_TAGGED_CHECKED(asyncFuncObj, 0);
    CONVERT_ARG_TAGGED_CHECKED(value, 1);
    return SlowRuntimeStub::AsyncFunctionAwaitUncaught(thread, asyncFuncObj, value).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(AsyncFunctionResolveOrReject)
{
    RUNTIME_TRAMPOLINES_HEADER(AsyncFunctionResolveOrReject);
    CONVERT_ARG_TAGGED_CHECKED(asyncFuncObj, 0);
    CONVERT_ARG_TAGGED_CHECKED(value, 1);
    CONVERT_ARG_TAGGED_CHECKED(is_resolve, 2);
    return SlowRuntimeStub::AsyncFunctionResolveOrReject(thread,
        asyncFuncObj, value, is_resolve.IsTrue()).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(CopyDataProperties)
{
    RUNTIME_TRAMPOLINES_HEADER(CopyDataProperties);
    CONVERT_ARG_TAGGED_CHECKED(dst, 0);
    CONVERT_ARG_TAGGED_CHECKED(src, 1);
    return SlowRuntimeStub::CopyDataProperties(thread, dst, src).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(StArraySpread)
{
    RUNTIME_TRAMPOLINES_HEADER(StArraySpread);
    CONVERT_ARG_TAGGED_CHECKED(dst, 0);
    CONVERT_ARG_TAGGED_CHECKED(index, 1);
    CONVERT_ARG_TAGGED_CHECKED(src, 2);
    return SlowRuntimeStub::StArraySpread(thread, dst, index, src).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(GetIteratorNext)
{
    RUNTIME_TRAMPOLINES_HEADER(GetIteratorNext);
    CONVERT_ARG_TAGGED_CHECKED(obj, 0);
    CONVERT_ARG_TAGGED_CHECKED(method, 1);
    return SlowRuntimeStub::GetIteratorNext(thread, obj, method).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(SetObjectWithProto)
{
    RUNTIME_TRAMPOLINES_HEADER(SetObjectWithProto);
    CONVERT_ARG_TAGGED_CHECKED(proto, 0);
    CONVERT_ARG_TAGGED_CHECKED(obj, 1);
    return SlowRuntimeStub::SetObjectWithProto(thread, proto, obj).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(LoadICByValue)
{
    RUNTIME_TRAMPOLINES_HEADER(LoadICByValue);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(profileTypeInfo, 0);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(receiver, 1);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(propKey, 2);
    CONVERT_ARG_TAGGED_CHECKED(slotId, 3);

    auto profileHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(profileTypeInfo));
    if (profileHandle->IsUndefined()) {
        return SlowRuntimeStub::LdObjByValue(thread, JSTaggedValue(receiver), JSTaggedValue(propKey),
            false, JSTaggedValue::Undefined()).GetRawData();
    }
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(receiver));
    auto keyHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(propKey));
    LoadICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileHandle), slotId.GetInt(), ICKind::LoadIC);
    return icRuntime.LoadMiss(receiverHandle, keyHandle).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(StoreICByValue)
{
    RUNTIME_TRAMPOLINES_HEADER(StoreICByValue);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(profileTypeInfo, 0);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(receiver, 1);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(propKey, 2);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(value, 3);
    CONVERT_ARG_TAGGED_CHECKED(slotId, 4);

    auto profileHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(profileTypeInfo));
    if (profileHandle->IsUndefined()) {
        return SlowRuntimeStub::StObjByValue(thread,
            JSTaggedValue(receiver), JSTaggedValue(propKey), JSTaggedValue(value)).GetRawData();
    }
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(receiver));
    auto keyHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(propKey));
    auto valueHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(value));
    StoreICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileHandle), slotId.GetInt(), ICKind::StoreIC);
    return icRuntime.StoreMiss(receiverHandle, keyHandle, valueHandle).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(StOwnByValue)
{
    RUNTIME_TRAMPOLINES_HEADER(StOwnByValue);
    CONVERT_ARG_TAGGED_CHECKED(obj, 0);
    CONVERT_ARG_TAGGED_CHECKED(key, 1);
    CONVERT_ARG_TAGGED_CHECKED(value, 2);

    return SlowRuntimeStub::StOwnByValue(thread, obj, key, value).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(LdSuperByValue)
{
    RUNTIME_TRAMPOLINES_HEADER(LdSuperByValue);
    CONVERT_ARG_TAGGED_CHECKED(obj, 0);
    CONVERT_ARG_TAGGED_CHECKED(key, 1);
    auto sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    JSTaggedValue thisFunc = EcmaInterpreter::GetThisFunction(sp);
    return SlowRuntimeStub::LdSuperByValue(thread, obj, key, thisFunc).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(StSuperByValue)
{
    RUNTIME_TRAMPOLINES_HEADER(StSuperByValue);
    CONVERT_ARG_TAGGED_CHECKED(obj, 0);
    CONVERT_ARG_TAGGED_CHECKED(key, 1);
    CONVERT_ARG_TAGGED_CHECKED(value, 2);
    auto sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    JSTaggedValue thisFunc = EcmaInterpreter::GetThisFunction(sp);
    return SlowRuntimeStub::StSuperByValue(thread, obj, key, value, thisFunc).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(LdObjByIndex)
{
    RUNTIME_TRAMPOLINES_HEADER(LdObjByIndex);
    CONVERT_ARG_TAGGED_CHECKED(obj, 0);
    CONVERT_ARG_TAGGED_CHECKED(idx, 1);
    CONVERT_ARG_TAGGED_CHECKED(callGetter, 2);
    CONVERT_ARG_TAGGED_CHECKED(receiver, 3);
    return SlowRuntimeStub::LdObjByIndex(thread, obj, idx.GetInt(), callGetter.IsTrue(), receiver).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(StObjByIndex)
{
    RUNTIME_TRAMPOLINES_HEADER(StObjByIndex);
    CONVERT_ARG_TAGGED_CHECKED(obj, 0);
    CONVERT_ARG_TAGGED_CHECKED(idx, 1);
    CONVERT_ARG_TAGGED_CHECKED(value, 2);
    return SlowRuntimeStub::StObjByIndex(thread, obj, idx.GetInt(), value).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(StOwnByIndex)
{
    RUNTIME_TRAMPOLINES_HEADER(StOwnByIndex);
    CONVERT_ARG_TAGGED_CHECKED(obj, 0);
    CONVERT_ARG_TAGGED_CHECKED(idx, 1);
    CONVERT_ARG_TAGGED_CHECKED(value, 2);
    return SlowRuntimeStub::StOwnByIndex(thread, obj, idx.GetInt(), value).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(StGlobalRecord)
{
    RUNTIME_TRAMPOLINES_HEADER(StGlobalRecord);
    CONVERT_ARG_TAGGED_CHECKED(prop, 0);
    CONVERT_ARG_TAGGED_CHECKED(value, 1);
    CONVERT_ARG_TAGGED_CHECKED(isConst, 2);
    return SlowRuntimeStub::StGlobalRecord(thread, prop, value, isConst.IsTrue()).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(NegDyn)
{
    RUNTIME_TRAMPOLINES_HEADER(NegDyn);
    CONVERT_ARG_TAGGED_CHECKED(value, 0);
    return SlowRuntimeStub::NegDyn(thread, value).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(NotDyn)
{
    RUNTIME_TRAMPOLINES_HEADER(NotDyn);
    CONVERT_ARG_TAGGED_CHECKED(value, 0);
    return SlowRuntimeStub::NotDyn(thread, value).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ChangeUintAndIntShrToJSTaggedValue)
{
    RUNTIME_TRAMPOLINES_HEADER(ChangeUintAndIntShrToJSTaggedValue);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, leftHandle, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, rightHandle, 1);

    int32_t leftInt32 = JSTaggedValue::ToInt32(thread, leftHandle);
    if (thread->HasPendingException()) {
        return JSTaggedValue::Exception().GetRawData();
    }
    int32_t rightInt32 = JSTaggedValue::ToUint32(thread, rightHandle);
    if (thread->HasPendingException()) {
        return JSTaggedValue::Exception().GetRawData();
    }

    uint32_t shift = static_cast<uint32_t>(rightInt32) & 0x1f;
    auto ret = static_cast<int32_t>(leftInt32 >> shift);
    return JSTaggedValue(ret).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ChangeTwoInt32AndToJSTaggedValue)
{
    RUNTIME_TRAMPOLINES_HEADER(ChangeTwoInt32AndToJSTaggedValue);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, leftHandle, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, rightHandle, 1);

    int32_t leftInt = JSTaggedValue::ToInt32(thread, leftHandle);
    if (thread->HasPendingException()) {
        return JSTaggedValue::Exception().GetRawData();
    }
    int32_t rightInt = JSTaggedValue::ToInt32(thread, rightHandle);
    if (thread->HasPendingException()) {
        return JSTaggedValue::Exception().GetRawData();
    }

    auto ret = static_cast<uint32_t>(leftInt) & static_cast<uint32_t>(rightInt);
    return JSTaggedValue(static_cast<uint32_t>(ret)).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ChangeTwoInt32OrToJSTaggedValue)
{
    RUNTIME_TRAMPOLINES_HEADER(ChangeTwoInt32OrToJSTaggedValue);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, leftHandle, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, rightHandle, 1);

    int32_t leftInt = JSTaggedValue::ToInt32(thread, leftHandle);
    if (thread->HasPendingException()) {
        return JSTaggedValue::Exception().GetRawData();
    }
    int32_t rightInt = JSTaggedValue::ToInt32(thread, rightHandle);
    if (thread->HasPendingException()) {
        return JSTaggedValue::Exception().GetRawData();
    }

    auto ret = static_cast<uint32_t>(leftInt) | static_cast<uint32_t>(rightInt);
    return JSTaggedValue(static_cast<uint32_t>(ret)).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ChangeTwoInt32XorToJSTaggedValue)
{
    RUNTIME_TRAMPOLINES_HEADER(ChangeTwoInt32XorToJSTaggedValue);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, leftHandle, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, rightHandle, 1);

    int32_t leftInt = JSTaggedValue::ToInt32(thread, leftHandle);
    if (thread->HasPendingException()) {
        return JSTaggedValue::Exception().GetRawData();
    }
    int32_t rightInt = JSTaggedValue::ToInt32(thread, rightHandle);
    if (thread->HasPendingException()) {
        return JSTaggedValue::Exception().GetRawData();
    }

    auto ret = static_cast<uint32_t>(leftInt) ^ static_cast<uint32_t>(rightInt);
    return JSTaggedValue(static_cast<uint32_t>(ret)).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ChangeTwoUint32AndToJSTaggedValue)
{
    RUNTIME_TRAMPOLINES_HEADER(ChangeTwoUint32AndToJSTaggedValue);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, leftHandle, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, rightHandle, 1);

    int32_t leftInt = JSTaggedValue::ToUint32(thread, leftHandle);
    if (thread->HasPendingException()) {
        return JSTaggedValue::Exception().GetRawData();
    }
    int32_t rightInt = JSTaggedValue::ToUint32(thread, rightHandle);
    if (thread->HasPendingException()) {
        return JSTaggedValue::Exception().GetRawData();
    }

    auto ret = static_cast<uint32_t>(leftInt) & static_cast<uint32_t>(rightInt);
    return JSTaggedValue(ret).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ChangeUintAndIntShlToJSTaggedValue)
{
    RUNTIME_TRAMPOLINES_HEADER(ChangeUintAndIntShlToJSTaggedValue);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, leftHandle, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, rightHandle, 1);

    int32_t leftInt32 = JSTaggedValue::ToInt32(thread, leftHandle);
    if (thread->HasPendingException()) {
        return JSTaggedValue::Exception().GetRawData();
    }
    int32_t rightInt32 = JSTaggedValue::ToUint32(thread, rightHandle);
    if (thread->HasPendingException()) {
        return JSTaggedValue::Exception().GetRawData();
    }

    uint32_t shift =
        static_cast<uint32_t>(rightInt32) & 0x1f;  // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
    using unsigned_type = std::make_unsigned_t<int32_t>;
    auto ret =
        static_cast<int32_t>(static_cast<unsigned_type>(leftInt32) << shift);  // NOLINT(hicpp-signed-bitwise)
    return JSTaggedValue(ret).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ResolveClass)
{
    RUNTIME_TRAMPOLINES_HEADER(ResolveClass);
    CONVERT_ARG_TAGGED_CHECKED(ctor, 0);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(literal, 1);
    CONVERT_ARG_TAGGED_CHECKED(base, 2);
    CONVERT_ARG_TAGGED_CHECKED(lexenv, 3);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(constpool, 4);
    return SlowRuntimeStub::ResolveClass(thread, ctor, reinterpret_cast<TaggedArray *>(literal), base, lexenv,
        reinterpret_cast<ConstantPool *>(constpool)).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(CloneClassFromTemplate)
{
    RUNTIME_TRAMPOLINES_HEADER(CloneClassFromTemplate);
    CONVERT_ARG_TAGGED_CHECKED(ctor, 0);
    CONVERT_ARG_TAGGED_CHECKED(base, 1);
    CONVERT_ARG_TAGGED_CHECKED(lexenv, 2);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(constpool, 3);
    return SlowRuntimeStub::CloneClassFromTemplate(thread, ctor, base, lexenv,
        reinterpret_cast<ConstantPool *>(constpool)).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(SetClassConstructorLength)
{
    RUNTIME_TRAMPOLINES_HEADER(SetClassConstructorLength);
    CONVERT_ARG_TAGGED_CHECKED(ctor, 0);
    CONVERT_ARG_TAGGED_CHECKED(length, 1);
    return SlowRuntimeStub::SetClassConstructorLength(thread, ctor, length).GetRawData();
}


DEF_RUNTIME_TRAMPOLINES(UpdateHotnessCounter)
{
    RUNTIME_TRAMPOLINES_HEADER(UpdateHotnessCounter);
    InterpretedFrame *state = GET_FRAME(const_cast<JSTaggedType *>(thread->GetCurrentSPFrame()));
    thread->CheckSafepoint();
    if (state->profileTypeInfo == JSTaggedValue::Undefined()) {
        auto thisFunc = state->function;
        auto method = ECMAObject::Cast(thisFunc.GetTaggedObject())->GetCallTarget();
        auto res = SlowRuntimeStub::NotifyInlineCache(
            thread, JSFunction::Cast(thisFunc.GetHeapObject()), method);
            state->profileTypeInfo = res;
    }
    return state->profileTypeInfo.GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(LoadICByName)
{
    RUNTIME_TRAMPOLINES_HEADER(LoadICByName);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, profileHandle, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, receiverHandle, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, keyHandle, 2);
    CONVERT_ARG_TAGGED_CHECKED(slotId, 3);

    if (profileHandle->IsUndefined()) {
        auto res = JSTaggedValue::GetProperty(thread, receiverHandle, keyHandle).GetValue().GetTaggedValue();
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
        return res.GetRawData();
    }
    LoadICRuntime icRuntime(
        thread, JSHandle<ProfileTypeInfo>::Cast(profileHandle), slotId.GetInt(), ICKind::NamedLoadIC);
    return icRuntime.LoadMiss(receiverHandle, keyHandle).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(StoreICByName)
{
    RUNTIME_TRAMPOLINES_HEADER(StoreICByName);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, profileHandle, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, receiverHandle, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, keyHandle, 2);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, valueHandle, 3);
    CONVERT_ARG_TAGGED_CHECKED(slotId, 4);

    if (profileHandle->IsUndefined()) {
        JSTaggedValue::SetProperty(thread, receiverHandle, keyHandle, valueHandle, true);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
        return JSTaggedValue::True().GetRawData();
    }
    StoreICRuntime icRuntime(
        thread, JSHandle<ProfileTypeInfo>::Cast(profileHandle), slotId.GetInt(), ICKind::NamedStoreIC);
    return icRuntime.StoreMiss(receiverHandle, keyHandle, valueHandle).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(SetFunctionNameNoPrefix)
{
    RUNTIME_TRAMPOLINES_HEADER(SetFunctionNameNoPrefix);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(argFunc, 0);
    CONVERT_ARG_TAGGED_CHECKED(argName, 1);
    JSFunction::SetFunctionNameNoPrefix(thread, reinterpret_cast<JSFunction *>(argFunc), argName);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(StOwnByValueWithNameSet)
{
    RUNTIME_TRAMPOLINES_HEADER(StOwnByValueWithNameSet);
    CONVERT_ARG_TAGGED_CHECKED(obj, 0);
    CONVERT_ARG_TAGGED_CHECKED(prop, 1);
    CONVERT_ARG_TAGGED_CHECKED(value, 2);
    return SlowRuntimeStub::StOwnByValueWithNameSet(thread, obj, prop, value).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(StOwnByName)
{
    RUNTIME_TRAMPOLINES_HEADER(StOwnByName);
    CONVERT_ARG_TAGGED_CHECKED(obj, 0);
    CONVERT_ARG_TAGGED_CHECKED(prop, 1);
    CONVERT_ARG_TAGGED_CHECKED(value, 2);
    return SlowRuntimeStub::StOwnByName(thread, obj, prop, value).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(StOwnByNameWithNameSet)
{
    RUNTIME_TRAMPOLINES_HEADER(StOwnByNameWithNameSet);
    CONVERT_ARG_TAGGED_CHECKED(obj, 0);
    CONVERT_ARG_TAGGED_CHECKED(prop, 1);
    CONVERT_ARG_TAGGED_CHECKED(value, 2);
    return SlowRuntimeStub::StOwnByValueWithNameSet(thread, obj, prop, value).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(SuspendGenerator)
{
    RUNTIME_TRAMPOLINES_HEADER(SuspendGenerator);
    CONVERT_ARG_TAGGED_CHECKED(obj, 0);
    CONVERT_ARG_TAGGED_CHECKED(value, 1);
    return SlowRuntimeStub::SuspendGenerator(thread, obj, value).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(UpFrame)
{
    RUNTIME_TRAMPOLINES_HEADER(UpFrame);
    auto sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    InterpretedFrameHandler frameHandler(sp);
    uint32_t pcOffset = panda_file::INVALID_OFFSET;
    for (; frameHandler.HasFrame(); frameHandler.PrevInterpretedFrame()) {
        if (frameHandler.IsBreakFrame()) {
            return JSTaggedValue(static_cast<uint64_t>(0)).GetRawData();
        }
        auto method = frameHandler.GetMethod();
        pcOffset = EcmaInterpreter::FindCatchBlock(method, frameHandler.GetBytecodeOffset());
        if (pcOffset != panda_file::INVALID_OFFSET) {
            thread->SetCurrentSPFrame(frameHandler.GetSp());
            uintptr_t pc = reinterpret_cast<uintptr_t>(method->GetBytecodeArray() + pcOffset);
            return JSTaggedValue(static_cast<uint64_t>(pc)).GetRawData();
        }
    }
    return JSTaggedValue(static_cast<uint64_t>(0)).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ImportModule)
{
    RUNTIME_TRAMPOLINES_HEADER(ImportModule);
    CONVERT_ARG_TAGGED_CHECKED(moduleName, 0);
    return SlowRuntimeStub::ImportModule(thread, moduleName).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(StModuleVar)
{
    RUNTIME_TRAMPOLINES_HEADER(StModuleVar);
    CONVERT_ARG_TAGGED_CHECKED(exportName, 0);
    CONVERT_ARG_TAGGED_CHECKED(exportObj, 1);
    SlowRuntimeStub::StModuleVar(thread, exportName, exportObj);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(LdModvarByName)
{
    RUNTIME_TRAMPOLINES_HEADER(LdModvarByName);
    CONVERT_ARG_TAGGED_CHECKED(moduleObj, 0);
    CONVERT_ARG_TAGGED_CHECKED(itemName, 1);
    return SlowRuntimeStub::LdModvarByName(thread, moduleObj, itemName).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(GetPropIterator)
{
    RUNTIME_TRAMPOLINES_HEADER(GetPropIterator);
    CONVERT_ARG_TAGGED_CHECKED(value, 0);
    return SlowRuntimeStub::GetPropIterator(thread, value).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(AsyncFunctionEnter)
{
    RUNTIME_TRAMPOLINES_HEADER(AsyncFunctionEnter);
    return SlowRuntimeStub::AsyncFunctionEnter(thread).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(GetIterator)
{
    RUNTIME_TRAMPOLINES_HEADER(GetIterator);
    CONVERT_ARG_TAGGED_CHECKED(obj, 0);
    return SlowRuntimeStub::GetIterator(thread, obj).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ThrowDyn)
{
    RUNTIME_TRAMPOLINES_HEADER(ThrowDyn);
    CONVERT_ARG_TAGGED_CHECKED(value, 0);
    SlowRuntimeStub::ThrowDyn(thread, value);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ThrowThrowNotExists)
{
    RUNTIME_TRAMPOLINES_HEADER(ThrowThrowNotExists);
    SlowRuntimeStub::ThrowThrowNotExists(thread);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ThrowPatternNonCoercible)
{
    RUNTIME_TRAMPOLINES_HEADER(ThrowPatternNonCoercible);
    SlowRuntimeStub::ThrowPatternNonCoercible(thread);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ThrowDeleteSuperProperty)
{
    RUNTIME_TRAMPOLINES_HEADER(ThrowDeleteSuperProperty);
    SlowRuntimeStub::ThrowDeleteSuperProperty(thread);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ThrowUndefinedIfHole)
{
    RUNTIME_TRAMPOLINES_HEADER(ThrowUndefinedIfHole);
    CONVERT_ARG_TAGGED_CHECKED(obj, 0);
    SlowRuntimeStub::ThrowUndefinedIfHole(thread, obj);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ThrowIfNotObject)
{
    RUNTIME_TRAMPOLINES_HEADER(ThrowIfNotObject);
    SlowRuntimeStub::ThrowIfNotObject(thread);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ThrowConstAssignment)
{
    RUNTIME_TRAMPOLINES_HEADER(ThrowConstAssignment);
    CONVERT_ARG_TAGGED_CHECKED(value, 0);
    SlowRuntimeStub::ThrowConstAssignment(thread, value);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ThrowTypeError)
{
    RUNTIME_TRAMPOLINES_HEADER(ThrowTypeError);
    CONVERT_ARG_TAGGED_CHECKED(argMessageStringId, 0);
    std::string message = MessageString::GetMessageString(argMessageStringId.GetInt());
    ObjectFactory *factory = JSThread::Cast(thread)->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR, message.c_str());
    THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error.GetTaggedValue(), JSTaggedValue::Hole().GetRawData());
}

DEF_RUNTIME_TRAMPOLINES(LdGlobalRecord)
{
    RUNTIME_TRAMPOLINES_HEADER(LdGlobalRecord);
    CONVERT_ARG_TAGGED_CHECKED(key, 0);
    return SlowRuntimeStub::LdGlobalRecord(thread, key).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(GetGlobalOwnProperty)
{
    RUNTIME_TRAMPOLINES_HEADER(GetGlobalOwnProperty);
    CONVERT_ARG_TAGGED_CHECKED(key, 0);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    JSTaggedValue globalObj = globalEnv->GetGlobalObject();
    return FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, key).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(TryLdGlobalByName)
{
    RUNTIME_TRAMPOLINES_HEADER(TryLdGlobalByName);
    CONVERT_ARG_TAGGED_CHECKED(prop, 0);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    JSTaggedValue globalObj = globalEnv->GetGlobalObject();
    return SlowRuntimeStub::TryLdGlobalByName(thread, globalObj, prop).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(LoadMiss)
{
    RUNTIME_TRAMPOLINES_HEADER(LoadMiss);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(profileTypeInfo, 0);
    CONVERT_ARG_TAGGED_CHECKED(receiver, 1);
    CONVERT_ARG_TAGGED_CHECKED(key, 2);
    CONVERT_ARG_TAGGED_CHECKED(slotId, 3);
    CONVERT_ARG_TAGGED_CHECKED(kind, 4);
    return ICRuntimeStub::LoadMiss(thread, reinterpret_cast<ProfileTypeInfo *>(profileTypeInfo), receiver, key,
        slotId.GetInt(), static_cast<ICKind>(kind.GetInt())).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(StoreMiss)
{
    RUNTIME_TRAMPOLINES_HEADER(StoreMiss);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(profileTypeInfo, 0);
    CONVERT_ARG_TAGGED_CHECKED(receiver, 1);
    CONVERT_ARG_TAGGED_CHECKED(key, 2);
    CONVERT_ARG_TAGGED_CHECKED(value, 3);
    CONVERT_ARG_TAGGED_CHECKED(slotId, 4);
    CONVERT_ARG_TAGGED_CHECKED(kind, 5);
    return ICRuntimeStub::StoreMiss(thread, reinterpret_cast<ProfileTypeInfo *>(profileTypeInfo), receiver, key, value,
        slotId.GetInt(), static_cast<ICKind>(kind.GetInt())).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(TryUpdateGlobalRecord)
{
    RUNTIME_TRAMPOLINES_HEADER(TryUpdateGlobalRecord);
    CONVERT_ARG_TAGGED_CHECKED(prop, 0);
    CONVERT_ARG_TAGGED_CHECKED(value, 1);
    return SlowRuntimeStub::TryUpdateGlobalRecord(thread, prop, value).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ThrowReferenceError)
{
    RUNTIME_TRAMPOLINES_HEADER(ThrowReferenceError);
    CONVERT_ARG_TAGGED_CHECKED(prop, 0);
    return SlowRuntimeStub::ThrowReferenceError(thread, prop, " is not defined").GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(LdGlobalVar)
{
    RUNTIME_TRAMPOLINES_HEADER(LdGlobalVar);
    CONVERT_ARG_TAGGED_CHECKED(global, 0);
    CONVERT_ARG_TAGGED_CHECKED(prop, 1);
    return SlowRuntimeStub::LdGlobalVar(thread, global, prop).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(StGlobalVar)
{
    RUNTIME_TRAMPOLINES_HEADER(StGlobalVar);
    CONVERT_ARG_TAGGED_CHECKED(prop, 0);
    CONVERT_ARG_TAGGED_CHECKED(value, 1);
    return SlowRuntimeStub::StGlobalVar(thread, prop, value).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ToNumber)
{
    RUNTIME_TRAMPOLINES_HEADER(ToNumber);
    CONVERT_ARG_TAGGED_CHECKED(value, 0);
    return SlowRuntimeStub::ToNumber(thread, value).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(ToBoolean)
{
    RUNTIME_TRAMPOLINES_HEADER(ToBoolean);
    CONVERT_ARG_TAGGED_CHECKED(value, 0);
    bool result = value.ToBoolean();
    return JSTaggedValue(result).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(EqDyn)
{
    RUNTIME_TRAMPOLINES_HEADER(EqDyn);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    return SlowRuntimeStub::EqDyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(NotEqDyn)
{
    RUNTIME_TRAMPOLINES_HEADER(NotEqDyn);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    return SlowRuntimeStub::NotEqDyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(LessDyn)
{
    RUNTIME_TRAMPOLINES_HEADER(LessDyn);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    return SlowRuntimeStub::LessDyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(LessEqDyn)
{
    RUNTIME_TRAMPOLINES_HEADER(LessEqDyn);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    return SlowRuntimeStub::LessEqDyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(GreaterDyn)
{
    RUNTIME_TRAMPOLINES_HEADER(GreaterDyn);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    return SlowRuntimeStub::GreaterDyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(GreaterEqDyn)
{
    RUNTIME_TRAMPOLINES_HEADER(GreaterEqDyn);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    return SlowRuntimeStub::GreaterEqDyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(Add2Dyn)
{
    RUNTIME_TRAMPOLINES_HEADER(Add2Dyn);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    return SlowRuntimeStub::Add2Dyn(thread, ecmaVm, left, right).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(Sub2Dyn)
{
    RUNTIME_TRAMPOLINES_HEADER(Sub2Dyn);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    return SlowRuntimeStub::Sub2Dyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(Mul2Dyn)
{
    RUNTIME_TRAMPOLINES_HEADER(Mul2Dyn);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    return SlowRuntimeStub::Mul2Dyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(Div2Dyn)
{
    RUNTIME_TRAMPOLINES_HEADER(Div2Dyn);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    return SlowRuntimeStub::Div2Dyn(thread, JSTaggedValue(left), JSTaggedValue(right)).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(Mod2Dyn)
{
    RUNTIME_TRAMPOLINES_HEADER(Mod2Dyn);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    return SlowRuntimeStub::Mod2Dyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_TRAMPOLINES(JumpToCInterpreter)
{
#if ECMASCRIPT_COMPILE_INTERPRETER_ASM
    RUNTIME_TRAMPOLINES_HEADER(JumpToCInterpreter);
    CONVERT_ARG_TAGGED_CHECKED(constpool, 0);
    CONVERT_ARG_TAGGED_CHECKED(profileTypeInfo, 1);
    CONVERT_ARG_TAGGED_CHECKED(acc, 2);
    CONVERT_ARG_TAGGED_CHECKED(hotnessCounter, 3);

    auto sp = const_cast<JSTaggedType*>(thread->GetCurrentSPFrame());
    const uint8_t* currentPc = reinterpret_cast<const uint8_t*>(GET_FRAME(sp)->pc);

    uint8_t opcode = currentPc[0];
    asmDispatchTable[opcode](thread, currentPc, sp, constpool, profileTypeInfo, acc, hotnessCounter.GetInt());
    sp = const_cast<JSTaggedType*>(thread->GetCurrentSPFrame());
    InterpretedFrame *frame = GET_FRAME(sp);
    uintptr_t framePc = reinterpret_cast<uintptr_t>(frame->pc);
    return JSTaggedValue(static_cast<uint64_t>(framePc)).GetRawData();
#else
    return 0;
#endif
}

void RuntimeTrampolines::InsertOldToNewRememberedSet([[maybe_unused]]uintptr_t argGlue, Region* region, uintptr_t addr)
{
    return region->InsertOldToNewRememberedSet(addr);
}

void RuntimeTrampolines::MarkingBarrier([[maybe_unused]]uintptr_t argGlue, uintptr_t slotAddr,
    Region *objectRegion, TaggedObject *value,
    Region *valueRegion)
{
    if (!valueRegion->IsMarking()) {
        return;
    }
    ::panda::ecmascript::RuntimeApi::MarkObject(slotAddr, objectRegion, value, valueRegion);
}
}  // namespace panda::ecmascript
