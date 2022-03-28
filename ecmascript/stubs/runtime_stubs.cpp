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

#include "runtime_stubs-inl.h"
#include "ecmascript/accessor_data.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/frames.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/ic_runtime.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/ic/properties_cache.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/interpreter/interpreter_assembly.h"
#include "ecmascript/js_api_arraylist.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/layout_info.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/message_string.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/runtime_api.h"
#include "ecmascript/tagged_dictionary.h"
#include "libpandabase/utils/string_helpers.h"
#include "ecmascript/ts_types/ts_loader.h"

namespace panda::ecmascript {
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#define DEF_RUNTIME_STUBS(name) \
JSTaggedType RuntimeStubs::name(uintptr_t argGlue, uint32_t argc, uintptr_t argv) \

#define RUNTIME_STUBS_HEADER(name)                        \
    auto thread = JSThread::GlueToJSThread(argGlue);      \
    RUNTIME_TRACE(thread, name);                          \
    [[maybe_unused]] EcmaHandleScope handleScope(thread)  \

#define CONVERT_ARG_TAGGED_TYPE_CHECKED(name, index) \
    ASSERT((index) < argc);                          \
    JSTaggedType name = *(reinterpret_cast<JSTaggedType *>(argv) + (index))

#define CONVERT_ARG_TAGGED_CHECKED(name, index) \
    ASSERT((index) < argc);                     \
    JSTaggedValue name = JSTaggedValue(*(reinterpret_cast<JSTaggedType *>(argv) + (index)))

#define CONVERT_ARG_HANDLE_CHECKED(type, name, index) \
    ASSERT((index) < argc);                           \
    JSHandle<type> name(&(reinterpret_cast<JSTaggedType *>(argv)[index]))

#define CONVERT_ARG_PTR_CHECKED(type, name, index) \
    ASSERT((index) < argc);                        \
    type name = reinterpret_cast<type>(*(reinterpret_cast<JSTaggedType *>(argv) + (index)))

DEF_RUNTIME_STUBS(AddElementInternal)
{
    RUNTIME_STUBS_HEADER(AddElementInternal);
    CONVERT_ARG_HANDLE_CHECKED(JSObject, receiver, 0);
    CONVERT_ARG_TAGGED_CHECKED(argIndex, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 2);
    CONVERT_ARG_TAGGED_CHECKED(argAttr, 3);
    auto attr = static_cast<PropertyAttributes>(argAttr.GetInt());
    auto result = JSObject::AddElementInternal(thread, receiver, argIndex.GetInt(), value, attr);
    return JSTaggedValue(result).GetRawData();
}

DEF_RUNTIME_STUBS(CallSetter)
{
    RUNTIME_STUBS_HEADER(CallSetter);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(argSetter, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, receiver, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 2);
    CONVERT_ARG_TAGGED_CHECKED(argMayThrow, 3);
    auto setter = AccessorData::Cast((reinterpret_cast<TaggedObject *>(argSetter)));
    auto result = JSObject::CallSetter(thread, *setter, receiver, value, argMayThrow.IsTrue());
    return JSTaggedValue(result).GetRawData();
}

DEF_RUNTIME_STUBS(CallSetter2)
{
    RUNTIME_STUBS_HEADER(CallSetter2);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, objHandle, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, valueHandle, 1);
    CONVERT_ARG_TAGGED_CHECKED(argAccessor, 2);
    auto accessor = AccessorData::Cast(argAccessor.GetTaggedObject());
    bool success = JSObject::CallSetter(thread, *accessor, objHandle, valueHandle, true);
    return success ? JSTaggedValue::Undefined().GetRawData() : JSTaggedValue::Exception().GetRawData();
}

DEF_RUNTIME_STUBS(CallGetter2)
{
    RUNTIME_STUBS_HEADER(CallGetter2);
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

DEF_RUNTIME_STUBS(JSProxySetProperty)
{
    RUNTIME_STUBS_HEADER(JSProxySetProperty);
    CONVERT_ARG_HANDLE_CHECKED(JSProxy, proxy, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, index, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 2);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, receiver, 3);
    CONVERT_ARG_TAGGED_CHECKED(argMayThrow, 4);
    auto result = JSProxy::SetProperty(thread, proxy, index, value, receiver, argMayThrow.IsTrue());
    return JSTaggedValue(result).GetRawData();
}

DEF_RUNTIME_STUBS(GetHash32)
{
    CONVERT_ARG_TAGGED_CHECKED(argKey, 0);
    CONVERT_ARG_TAGGED_CHECKED(len, 1);
    int key = argKey.GetInt();
    auto pkey = reinterpret_cast<uint8_t *>(&key);
    uint32_t result = panda::GetHash32(pkey, len.GetInt());
    return JSTaggedValue(static_cast<uint64_t>(result)).GetRawData();
}

DEF_RUNTIME_STUBS(CallGetter)
{
    RUNTIME_STUBS_HEADER(CallGetter);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(argGetter, 0);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(argReceiver, 1);

    auto accessor = AccessorData::Cast(reinterpret_cast<TaggedObject *>(argGetter));
    JSHandle<JSTaggedValue> objHandle(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(argReceiver)));
    return JSObject::CallGetter(thread, accessor, objHandle).GetRawData();
}

DEF_RUNTIME_STUBS(CallInternalGetter)
{
    RUNTIME_STUBS_HEADER(CallInternalGetter);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(argAccessor, 0);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(argReceiver, 1);

    auto accessor = AccessorData::Cast(reinterpret_cast<TaggedObject *>(argAccessor));
    JSHandle<JSObject> objHandle(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(argReceiver)));
    return accessor->CallInternalGet(thread, objHandle).GetRawData();
}

DEF_RUNTIME_STUBS(FindElementWithCache)
{
    RUNTIME_STUBS_HEADER(FindElementWithCache);
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

DEF_RUNTIME_STUBS(StringGetHashCode)
{
    CONVERT_ARG_TAGGED_TYPE_CHECKED(ecmaString, 0);
    auto string = reinterpret_cast<EcmaString *>(ecmaString);
    uint32_t result = string->GetHashcode();
    return JSTaggedValue(static_cast<uint64_t>(result)).GetRawData();
}

void RuntimeStubs::PrintHeapReginInfo(uintptr_t argGlue)
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

DEF_RUNTIME_STUBS(GetTaggedArrayPtrTest)
{
    RUNTIME_STUBS_HEADER(GetTaggedArrayPtrTest);
    // this case static static JSHandle<TaggedArray> arr don't free in first call
    // second call trigger gc.
    // don't call EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    CONVERT_ARG_TAGGED_TYPE_CHECKED(array, 0);
    bool allocated = false;
    if (array == JSTaggedValue::VALUE_UNDEFINED) {
        // 2 : means construct 2 elements size taggedArray
        JSHandle<TaggedArray> arr = factory->NewTaggedArray(2);
        arr->Set(thread, 0, JSTaggedValue(3.5)); // 3.5: first element
        arr->Set(thread, 1, JSTaggedValue(4.5)); // 4.5: second element
        array = arr.GetTaggedValue().GetRawData();
        allocated = true;
    }
    JSHandle<TaggedArray> arr1(thread, JSTaggedValue(array));
#ifndef NDEBUG
    PrintHeapReginInfo(argGlue);
#endif
    if (!allocated) {
        thread->GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);
    }
    LOG_ECMA(INFO) << " arr->GetData() " << std::hex << "  " << arr1->GetData();
    return arr1.GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(FloatMod)
{
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    double result = std::fmod(left.GetDouble(), right.GetDouble());
    return JSTaggedValue(result).GetRawData();
}

DEF_RUNTIME_STUBS(NewInternalString)
{
    RUNTIME_STUBS_HEADER(NewInternalString);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, keyHandle, 0);
    return JSTaggedValue(thread->GetEcmaVM()->GetFactory()->InternString(keyHandle)).GetRawData();
}

DEF_RUNTIME_STUBS(NewTaggedArray)
{
    RUNTIME_STUBS_HEADER(NewTaggedArray);
    CONVERT_ARG_TAGGED_CHECKED(length, 0);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewTaggedArray(length.GetInt()).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(CopyArray)
{
    RUNTIME_STUBS_HEADER(CopyArray);
    CONVERT_ARG_HANDLE_CHECKED(TaggedArray, array, 0);
    CONVERT_ARG_TAGGED_CHECKED(length, 1);
    CONVERT_ARG_TAGGED_CHECKED(capacity, 2);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->CopyArray(array, length.GetInt(), capacity.GetInt()).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(NameDictPutIfAbsent)
{
    RUNTIME_STUBS_HEADER(NameDictPutIfAbsent);
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

DEF_RUNTIME_STUBS(PropertiesSetValue)
{
    RUNTIME_STUBS_HEADER(PropertiesSetValue);
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

DEF_RUNTIME_STUBS(TaggedArraySetValue)
{
    RUNTIME_STUBS_HEADER(TaggedArraySetValue);
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

DEF_RUNTIME_STUBS(NewEcmaDynClass)
{
    RUNTIME_STUBS_HEADER(NewEcmaDynClass);
    CONVERT_ARG_TAGGED_CHECKED(size, 0);
    CONVERT_ARG_TAGGED_CHECKED(type, 1);
    CONVERT_ARG_TAGGED_CHECKED(inlinedProps, 2);
    return (thread->GetEcmaVM()->GetFactory()->NewEcmaDynClass(
        size.GetInt(), JSType(type.GetInt()), inlinedProps.GetInt())).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(UpdateLayOutAndAddTransition)
{
    RUNTIME_STUBS_HEADER(UpdateLayOutAndAddTransition);
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

void RuntimeStubs::DebugPrint(int fmtMessageId, ...)
{
    std::string format = MessageString::GetMessageString(fmtMessageId);
    va_list args;
    va_start(args, fmtMessageId);
    std::string result = panda::helpers::string::Vformat(format.c_str(), args);
    std::cerr << result << std::endl;
    va_end(args);
}

void RuntimeStubs::FatalPrint(int fmtMessageId, ...)
{
    std::string format = MessageString::GetMessageString(fmtMessageId);
    va_list args;
    va_start(args, fmtMessageId);
    std::string result = panda::helpers::string::Vformat(format.c_str(), args);
    std::cerr << result << std::endl;
    va_end(args);
    UNREACHABLE();
}

DEF_RUNTIME_STUBS(NoticeThroughChainAndRefreshUser)
{
    RUNTIME_STUBS_HEADER(NoticeThroughChainAndRefreshUser);
    CONVERT_ARG_HANDLE_CHECKED(JSHClass, oldHClassHandle, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSHClass, newHClassHandle, 1);

    JSHClass::NoticeThroughChain(thread, oldHClassHandle);
    JSHClass::RefreshUsers(thread, oldHClassHandle, newHClassHandle);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(IncDyn)
{
    RUNTIME_STUBS_HEADER(IncDyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 0);
    return RuntimeIncDyn(thread, value).GetRawData();
}

DEF_RUNTIME_STUBS(DecDyn)
{
    RUNTIME_STUBS_HEADER(DecDyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 0);
    return RuntimeDecDyn(thread, value).GetRawData();
}

DEF_RUNTIME_STUBS(ExpDyn)
{
    RUNTIME_STUBS_HEADER(ExpDyn);
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
    // Slow path
    JSTaggedValue res = RuntimeExpDyn(thread, JSHandle<JSTaggedValue>(thread, baseValue),
                                      JSHandle<JSTaggedValue>(thread, exponentValue));
    return res.GetRawData();
}

DEF_RUNTIME_STUBS(IsInDyn)
{
    RUNTIME_STUBS_HEADER(IsInDyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, prop, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, obj, 1);
    return RuntimeIsInDyn(thread, prop, obj).GetRawData();
}

DEF_RUNTIME_STUBS(InstanceOfDyn)
{
    RUNTIME_STUBS_HEADER(InstanceOfDyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, obj, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, target, 1);
    return RuntimeInstanceofDyn(thread, obj, target).GetRawData();
}

DEF_RUNTIME_STUBS(FastStrictNotEqual)
{
    RUNTIME_STUBS_HEADER(FastStrictNotEqual);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    bool result = FastRuntimeStub::FastStrictEqual(left, right);
    if (result) {
        return JSTaggedValue::False().GetRawData();
    }
    return JSTaggedValue::True().GetRawData();
}

DEF_RUNTIME_STUBS(FastStrictEqual)
{
    RUNTIME_STUBS_HEADER(FastStrictEqual);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    bool result = FastRuntimeStub::FastStrictEqual(left, right);
    if (result) {
        return JSTaggedValue::True().GetRawData();
    }
    return JSTaggedValue::False().GetRawData();
}

DEF_RUNTIME_STUBS(CreateGeneratorObj)
{
    RUNTIME_STUBS_HEADER(CreateGeneratorObj);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, genFunc, 0);
    return RuntimeCreateGeneratorObj(thread, genFunc).GetRawData();
}

DEF_RUNTIME_STUBS(GetTemplateObject)
{
    RUNTIME_STUBS_HEADER(GetTemplateObject);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, literal, 0);
    return RuntimeGetTemplateObject(thread, literal).GetRawData();
}

DEF_RUNTIME_STUBS(GetNextPropName)
{
    RUNTIME_STUBS_HEADER(GetNextPropName);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, iter, 0);
    return RuntimeGetNextPropName(thread, iter).GetRawData();
}

DEF_RUNTIME_STUBS(IterNext)
{
    RUNTIME_STUBS_HEADER(IterNext);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, iter, 0);
    return RuntimeIterNext(thread, iter).GetRawData();
}

DEF_RUNTIME_STUBS(CloseIterator)
{
    RUNTIME_STUBS_HEADER(CloseIterator);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, iter, 0);
    return RuntimeCloseIterator(thread, iter).GetRawData();
}

DEF_RUNTIME_STUBS(CopyModule)
{
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(SuperCallSpread)
{
    RUNTIME_STUBS_HEADER(SuperCallSpread);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, func, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, array, 1);
    auto sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    JSTaggedValue function = EcmaInterpreter::GetNewTarget(sp);
    return RuntimeSuperCallSpread(thread, func, JSHandle<JSTaggedValue>(thread, function), array).GetRawData();
}

DEF_RUNTIME_STUBS(DelObjProp)
{
    RUNTIME_STUBS_HEADER(DelObjProp);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, obj, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, prop, 1);
    return RuntimeDelObjProp(thread, obj, prop).GetRawData();
}

DEF_RUNTIME_STUBS(NewObjSpreadDyn)
{
    RUNTIME_STUBS_HEADER(NewObjSpreadDyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, func, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, newTarget, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, array, 2);
    return RuntimeNewObjSpreadDyn(thread, func, newTarget, array).GetRawData();
}

DEF_RUNTIME_STUBS(CreateIterResultObj)
{
    RUNTIME_STUBS_HEADER(CreateIterResultObj);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 0);
    CONVERT_ARG_TAGGED_CHECKED(flag, 1);
    return RuntimeCreateIterResultObj(thread, value, flag).GetRawData();
}

DEF_RUNTIME_STUBS(AsyncFunctionAwaitUncaught)
{
    RUNTIME_STUBS_HEADER(AsyncFunctionAwaitUncaught);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, asyncFuncObj, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 1);
    return RuntimeAsyncFunctionAwaitUncaught(thread, asyncFuncObj, value).GetRawData();
}

DEF_RUNTIME_STUBS(AsyncFunctionResolveOrReject)
{
    RUNTIME_STUBS_HEADER(AsyncFunctionResolveOrReject);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, asyncFuncObj, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 1);
    CONVERT_ARG_TAGGED_CHECKED(is_resolve, 2);
    return RuntimeAsyncFunctionResolveOrReject(thread, asyncFuncObj, value, is_resolve.IsTrue()).GetRawData();
}

DEF_RUNTIME_STUBS(CopyDataProperties)
{
    RUNTIME_STUBS_HEADER(CopyDataProperties);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, dst, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, src, 1);
    return RuntimeCopyDataProperties(thread, dst, src).GetRawData();
}

DEF_RUNTIME_STUBS(StArraySpread)
{
    RUNTIME_STUBS_HEADER(StArraySpread);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, dst, 0);
    CONVERT_ARG_TAGGED_CHECKED(index, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, src, 2);
    return RuntimeStArraySpread(thread, dst, index, src).GetRawData();
}

DEF_RUNTIME_STUBS(GetIteratorNext)
{
    RUNTIME_STUBS_HEADER(GetIteratorNext);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, obj, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, method, 1);
    return RuntimeGetIteratorNext(thread, obj, method).GetRawData();
}

DEF_RUNTIME_STUBS(SetObjectWithProto)
{
    RUNTIME_STUBS_HEADER(SetObjectWithProto);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, proto, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSObject, obj, 1);
    return RuntimeSetObjectWithProto(thread, proto, obj).GetRawData();
}

DEF_RUNTIME_STUBS(LoadICByValue)
{
    RUNTIME_STUBS_HEADER(LoadICByValue);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, profileTypeInfo, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, receiver, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, propKey, 2);
    CONVERT_ARG_TAGGED_CHECKED(slotId, 3);

    if (profileTypeInfo->IsUndefined()) {
        return RuntimeLdObjByValue(thread, receiver, propKey, false, JSTaggedValue::Undefined()).GetRawData();
    }
    LoadICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileTypeInfo), slotId.GetInt(), ICKind::LoadIC);
    return icRuntime.LoadMiss(receiver, propKey).GetRawData();
}

DEF_RUNTIME_STUBS(StoreICByValue)
{
    RUNTIME_STUBS_HEADER(StoreICByValue);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, profileTypeInfo, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, receiver, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, propKey, 2);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 3);
    CONVERT_ARG_TAGGED_CHECKED(slotId, 4);

    if (profileTypeInfo->IsUndefined()) {
        return RuntimeStObjByValue(thread, receiver, propKey, value).GetRawData();
    }
    StoreICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileTypeInfo), slotId.GetInt(),
                             ICKind::StoreIC);
    return icRuntime.StoreMiss(receiver, propKey, value).GetRawData();
}

DEF_RUNTIME_STUBS(StOwnByValue)
{
    RUNTIME_STUBS_HEADER(StOwnByValue);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, obj, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, key, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 2);

    return RuntimeStOwnByValue(thread, obj, key, value).GetRawData();
}

DEF_RUNTIME_STUBS(LdSuperByValue)
{
    RUNTIME_STUBS_HEADER(LdSuperByValue);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, obj, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, key, 1);
    auto sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    JSTaggedValue thisFunc = EcmaInterpreter::GetThisFunction(sp);
    return RuntimeLdSuperByValue(thread, obj, key, thisFunc).GetRawData();
}

DEF_RUNTIME_STUBS(StSuperByValue)
{
    RUNTIME_STUBS_HEADER(StSuperByValue);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, obj, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, key, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 2);
    auto sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    JSTaggedValue thisFunc = EcmaInterpreter::GetThisFunction(sp);
    return RuntimeStSuperByValue(thread, obj, key, value, thisFunc).GetRawData();
}

DEF_RUNTIME_STUBS(LdObjByIndex)
{
    RUNTIME_STUBS_HEADER(LdObjByIndex);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, obj, 0);
    CONVERT_ARG_TAGGED_CHECKED(idx, 1);
    CONVERT_ARG_TAGGED_CHECKED(callGetter, 2);
    CONVERT_ARG_TAGGED_CHECKED(receiver, 3);
    return RuntimeLdObjByIndex(thread, obj, idx.GetInt(), callGetter.IsTrue(), receiver).GetRawData();
}

DEF_RUNTIME_STUBS(StObjByIndex)
{
    RUNTIME_STUBS_HEADER(StObjByIndex);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, obj, 0);
    CONVERT_ARG_TAGGED_CHECKED(idx, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 2);
    return RuntimeStObjByIndex(thread, obj, idx.GetInt(), value).GetRawData();
}

DEF_RUNTIME_STUBS(StOwnByIndex)
{
    RUNTIME_STUBS_HEADER(StOwnByIndex);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, obj, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, idx, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 2);
    return RuntimeStOwnByIndex(thread, obj, idx, value).GetRawData();
}

DEF_RUNTIME_STUBS(StGlobalRecord)
{
    RUNTIME_STUBS_HEADER(StGlobalRecord);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, prop, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 1);
    CONVERT_ARG_TAGGED_CHECKED(isConst, 2);
    return RuntimeStGlobalRecord(thread, prop, value, isConst.IsTrue()).GetRawData();
}

DEF_RUNTIME_STUBS(NegDyn)
{
    RUNTIME_STUBS_HEADER(NegDyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 0);
    return RuntimeNegDyn(thread, value).GetRawData();
}

DEF_RUNTIME_STUBS(NotDyn)
{
    RUNTIME_STUBS_HEADER(NotDyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 0);
    return RuntimeNotDyn(thread, value).GetRawData();
}

DEF_RUNTIME_STUBS(Shl2Dyn)
{
    RUNTIME_STUBS_HEADER(Shl2Dyn);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    
    auto res = SlowRuntimeStub::Shl2Dyn(thread, left, right);
    return JSTaggedValue(res).GetRawData();
}

DEF_RUNTIME_STUBS(Shr2Dyn)
{
    RUNTIME_STUBS_HEADER(Shr2Dyn);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    
    auto res = SlowRuntimeStub::Shr2Dyn(thread, left, right);
    return JSTaggedValue(res).GetRawData();
}

DEF_RUNTIME_STUBS(Ashr2Dyn)
{
    RUNTIME_STUBS_HEADER(Ashr2Dyn);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    
    auto res = SlowRuntimeStub::Ashr2Dyn(thread, left, right);
    return JSTaggedValue(res).GetRawData();
}

DEF_RUNTIME_STUBS(And2Dyn)
{
    RUNTIME_STUBS_HEADER(And2Dyn);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    
    auto res = SlowRuntimeStub::And2Dyn(thread, left, right);
    return JSTaggedValue(res).GetRawData();
}

DEF_RUNTIME_STUBS(Xor2Dyn)
{
    RUNTIME_STUBS_HEADER(Xor2Dyn);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);

    auto res = SlowRuntimeStub::Xor2Dyn(thread, left, right);
    return JSTaggedValue(res).GetRawData();
}

DEF_RUNTIME_STUBS(Or2Dyn)
{
    RUNTIME_STUBS_HEADER(Or2Dyn);
    CONVERT_ARG_TAGGED_CHECKED(left, 0);
    CONVERT_ARG_TAGGED_CHECKED(right, 1);
    
    auto res = SlowRuntimeStub::Or2Dyn(thread, left, right);
    return JSTaggedValue(res).GetRawData();
}

DEF_RUNTIME_STUBS(ResolveClass)
{
    RUNTIME_STUBS_HEADER(ResolveClass);
    CONVERT_ARG_HANDLE_CHECKED(JSFunction, ctor, 0);
    CONVERT_ARG_HANDLE_CHECKED(TaggedArray, literal, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, base, 2);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, lexenv, 3);
    CONVERT_ARG_HANDLE_CHECKED(ConstantPool, constpool, 4);
    return RuntimeResolveClass(thread, ctor, literal, base, lexenv, constpool).GetRawData();
}

DEF_RUNTIME_STUBS(CloneClassFromTemplate)
{
    RUNTIME_STUBS_HEADER(CloneClassFromTemplate);
    CONVERT_ARG_HANDLE_CHECKED(JSFunction, ctor, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, base, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, lexenv, 2);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, constpool, 3);
    return RuntimeCloneClassFromTemplate(thread, ctor, base, lexenv, constpool).GetRawData();
}

DEF_RUNTIME_STUBS(SetClassConstructorLength)
{
    RUNTIME_STUBS_HEADER(SetClassConstructorLength);
    CONVERT_ARG_TAGGED_CHECKED(ctor, 0);
    CONVERT_ARG_TAGGED_CHECKED(length, 1);
    return RuntimeSetClassConstructorLength(thread, ctor, length).GetRawData();
}


DEF_RUNTIME_STUBS(UpdateHotnessCounter)
{
    RUNTIME_STUBS_HEADER(UpdateHotnessCounter);
    InterpretedFrame *state = GET_FRAME(const_cast<JSTaggedType *>(thread->GetCurrentSPFrame()));
    thread->CheckSafepoint();
    auto thisFunc = JSFunction::Cast(state->function.GetTaggedObject());
    if (thisFunc->GetProfileTypeInfo() == JSTaggedValue::Undefined()) {
        auto method = thisFunc->GetCallTarget();
        auto res = RuntimeNotifyInlineCache(thread, JSHandle<JSFunction>(thread, thisFunc), method);
        return res.GetRawData();
    }
    return thisFunc->GetProfileTypeInfo().GetRawData();
}

DEF_RUNTIME_STUBS(LoadICByName)
{
    RUNTIME_STUBS_HEADER(LoadICByName);
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

DEF_RUNTIME_STUBS(StoreICByName)
{
    RUNTIME_STUBS_HEADER(StoreICByName);
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

DEF_RUNTIME_STUBS(SetFunctionNameNoPrefix)
{
    RUNTIME_STUBS_HEADER(SetFunctionNameNoPrefix);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(argFunc, 0);
    CONVERT_ARG_TAGGED_CHECKED(argName, 1);
    JSFunction::SetFunctionNameNoPrefix(thread, reinterpret_cast<JSFunction *>(argFunc), argName);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(StOwnByValueWithNameSet)
{
    RUNTIME_STUBS_HEADER(StOwnByValueWithNameSet);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, obj, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, prop, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 2);
    return RuntimeStOwnByValueWithNameSet(thread, obj, prop, value).GetRawData();
}

DEF_RUNTIME_STUBS(StOwnByName)
{
    RUNTIME_STUBS_HEADER(StOwnByName);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, obj, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, prop, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 2);
    return RuntimeStOwnByName(thread, obj, prop, value).GetRawData();
}

DEF_RUNTIME_STUBS(StOwnByNameWithNameSet)
{
    RUNTIME_STUBS_HEADER(StOwnByNameWithNameSet);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, obj, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, prop, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 2);
    return RuntimeStOwnByValueWithNameSet(thread, obj, prop, value).GetRawData();
}

DEF_RUNTIME_STUBS(SuspendGenerator)
{
    RUNTIME_STUBS_HEADER(SuspendGenerator);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, obj, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 1);
    return RuntimeSuspendGenerator(thread, obj, value).GetRawData();
}

DEF_RUNTIME_STUBS(UpFrame)
{
    RUNTIME_STUBS_HEADER(UpFrame);
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

DEF_RUNTIME_STUBS(GetModuleNamespace)
{
    RUNTIME_STUBS_HEADER(GetModuleNamespace);
    CONVERT_ARG_TAGGED_CHECKED(localName, 0);
    return RuntimeGetModuleNamespace(thread, localName).GetRawData();
}

DEF_RUNTIME_STUBS(StModuleVar)
{
    RUNTIME_STUBS_HEADER(StModuleVar);
    CONVERT_ARG_TAGGED_CHECKED(key, 0);
    CONVERT_ARG_TAGGED_CHECKED(value, 1);
    RuntimeStModuleVar(thread, key, value);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(LdModuleVar)
{
    RUNTIME_STUBS_HEADER(LdModuleVar);
    CONVERT_ARG_TAGGED_CHECKED(key, 0);
    CONVERT_ARG_TAGGED_CHECKED(taggedFlag, 1);
    bool innerFlag = taggedFlag.GetInt() != 0;
    return RuntimeLdModuleVar(thread, key, innerFlag).GetRawData();
}

DEF_RUNTIME_STUBS(GetPropIterator)
{
    RUNTIME_STUBS_HEADER(GetPropIterator);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 0);
    return RuntimeGetPropIterator(thread, value).GetRawData();
}

DEF_RUNTIME_STUBS(AsyncFunctionEnter)
{
    RUNTIME_STUBS_HEADER(AsyncFunctionEnter);
    return RuntimeAsyncFunctionEnter(thread).GetRawData();
}

DEF_RUNTIME_STUBS(GetIterator)
{
    RUNTIME_STUBS_HEADER(GetIterator);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, obj, 0);
    return RuntimeGetIterator(thread, obj).GetRawData();
}

DEF_RUNTIME_STUBS(ThrowDyn)
{
    RUNTIME_STUBS_HEADER(ThrowDyn);
    CONVERT_ARG_TAGGED_CHECKED(value, 0);
    RuntimeThrowDyn(thread, value);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowThrowNotExists)
{
    RUNTIME_STUBS_HEADER(ThrowThrowNotExists);
    RuntimeThrowThrowNotExists(thread);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowPatternNonCoercible)
{
    RUNTIME_STUBS_HEADER(ThrowPatternNonCoercible);
    RuntimeThrowPatternNonCoercible(thread);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowDeleteSuperProperty)
{
    RUNTIME_STUBS_HEADER(ThrowDeleteSuperProperty);
    RuntimeThrowDeleteSuperProperty(thread);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowUndefinedIfHole)
{
    RUNTIME_STUBS_HEADER(ThrowUndefinedIfHole);
    CONVERT_ARG_HANDLE_CHECKED(EcmaString, obj, 0);
    RuntimeThrowUndefinedIfHole(thread, obj);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowIfNotObject)
{
    RUNTIME_STUBS_HEADER(ThrowIfNotObject);
    RuntimeThrowIfNotObject(thread);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowConstAssignment)
{
    RUNTIME_STUBS_HEADER(ThrowConstAssignment);
    CONVERT_ARG_HANDLE_CHECKED(EcmaString, value, 0);
    RuntimeThrowConstAssignment(thread, value);
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(ThrowTypeError)
{
    RUNTIME_STUBS_HEADER(ThrowTypeError);
    CONVERT_ARG_TAGGED_CHECKED(argMessageStringId, 0);
    std::string message = MessageString::GetMessageString(argMessageStringId.GetInt());
    ObjectFactory *factory = JSThread::Cast(thread)->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR, message.c_str());
    THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error.GetTaggedValue(), JSTaggedValue::Hole().GetRawData());
}

DEF_RUNTIME_STUBS(LdGlobalRecord)
{
    RUNTIME_STUBS_HEADER(LdGlobalRecord);
    CONVERT_ARG_TAGGED_CHECKED(key, 0);
    return RuntimeLdGlobalRecord(thread, key).GetRawData();
}

DEF_RUNTIME_STUBS(GetGlobalOwnProperty)
{
    RUNTIME_STUBS_HEADER(GetGlobalOwnProperty);
    CONVERT_ARG_TAGGED_CHECKED(key, 0);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    JSTaggedValue globalObj = globalEnv->GetGlobalObject();
    return FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, key).GetRawData();
}

DEF_RUNTIME_STUBS(TryLdGlobalByName)
{
    RUNTIME_STUBS_HEADER(TryLdGlobalByName);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, prop, 0);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    JSTaggedValue globalObj = globalEnv->GetGlobalObject();
    return RuntimeTryLdGlobalByName(thread, globalObj, prop).GetRawData();
}

DEF_RUNTIME_STUBS(LoadMiss)
{
    RUNTIME_STUBS_HEADER(LoadMiss);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(profileTypeInfo, 0);
    CONVERT_ARG_TAGGED_CHECKED(receiver, 1);
    CONVERT_ARG_TAGGED_CHECKED(key, 2);
    CONVERT_ARG_TAGGED_CHECKED(slotId, 3);
    CONVERT_ARG_TAGGED_CHECKED(kind, 4);
    return ICRuntimeStub::LoadMiss(thread, reinterpret_cast<ProfileTypeInfo *>(profileTypeInfo), receiver, key,
        slotId.GetInt(), static_cast<ICKind>(kind.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(StoreMiss)
{
    RUNTIME_STUBS_HEADER(StoreMiss);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(profileTypeInfo, 0);
    CONVERT_ARG_TAGGED_CHECKED(receiver, 1);
    CONVERT_ARG_TAGGED_CHECKED(key, 2);
    CONVERT_ARG_TAGGED_CHECKED(value, 3);
    CONVERT_ARG_TAGGED_CHECKED(slotId, 4);
    CONVERT_ARG_TAGGED_CHECKED(kind, 5);
    return ICRuntimeStub::StoreMiss(thread, reinterpret_cast<ProfileTypeInfo *>(profileTypeInfo), receiver, key, value,
        slotId.GetInt(), static_cast<ICKind>(kind.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(TryUpdateGlobalRecord)
{
    RUNTIME_STUBS_HEADER(TryUpdateGlobalRecord);
    CONVERT_ARG_TAGGED_CHECKED(prop, 0);
    CONVERT_ARG_TAGGED_CHECKED(value, 1);
    return RuntimeTryUpdateGlobalRecord(thread, prop, value).GetRawData();
}

DEF_RUNTIME_STUBS(ThrowReferenceError)
{
    RUNTIME_STUBS_HEADER(ThrowReferenceError);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, prop, 0);
    return RuntimeThrowReferenceError(thread, prop, " is not defined").GetRawData();
}

DEF_RUNTIME_STUBS(LdGlobalVar)
{
    RUNTIME_STUBS_HEADER(LdGlobalVar);
    CONVERT_ARG_TAGGED_CHECKED(global, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, prop, 1);
    return RuntimeLdGlobalVar(thread, global, prop).GetRawData();
}

DEF_RUNTIME_STUBS(StGlobalVar)
{
    RUNTIME_STUBS_HEADER(StGlobalVar);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, prop, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 1);
    return RuntimeStGlobalVar(thread, prop, value).GetRawData();
}

DEF_RUNTIME_STUBS(ToNumber)
{
    RUNTIME_STUBS_HEADER(ToNumber);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, value, 0);
    return RuntimeToNumber(thread, value).GetRawData();
}

DEF_RUNTIME_STUBS(ToBoolean)
{
    RUNTIME_STUBS_HEADER(ToBoolean);
    CONVERT_ARG_TAGGED_CHECKED(value, 0);
    bool result = value.ToBoolean();
    return JSTaggedValue(result).GetRawData();
}

DEF_RUNTIME_STUBS(EqDyn)
{
    RUNTIME_STUBS_HEADER(EqDyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, left, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, right, 1);
    return RuntimeEqDyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(NotEqDyn)
{
    RUNTIME_STUBS_HEADER(NotEqDyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, left, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, right, 1);
    return RuntimeNotEqDyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(LessDyn)
{
    RUNTIME_STUBS_HEADER(LessDyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, left, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, right, 1);
    return RuntimeLessDyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(LessEqDyn)
{
    RUNTIME_STUBS_HEADER(LessEqDyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, left, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, right, 1);
    return RuntimeLessEqDyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(GreaterDyn)
{
    RUNTIME_STUBS_HEADER(GreaterDyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, left, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, right, 1);
    return RuntimeGreaterDyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(GreaterEqDyn)
{
    RUNTIME_STUBS_HEADER(GreaterEqDyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, left, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, right, 1);
    return RuntimeGreaterEqDyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(Add2Dyn)
{
    RUNTIME_STUBS_HEADER(Add2Dyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, left, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, right, 1);
    return RuntimeAdd2Dyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(Sub2Dyn)
{
    RUNTIME_STUBS_HEADER(Sub2Dyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, left, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, right, 1);
    return RuntimeSub2Dyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(Mul2Dyn)
{
    RUNTIME_STUBS_HEADER(Mul2Dyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, left, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, right, 1);
    return RuntimeMul2Dyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(Div2Dyn)
{
    RUNTIME_STUBS_HEADER(Div2Dyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, left, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, right, 1);
    return RuntimeDiv2Dyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(Mod2Dyn)
{
    RUNTIME_STUBS_HEADER(Mod2Dyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, left, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, right, 1);
    return RuntimeMod2Dyn(thread, left, right).GetRawData();
}

DEF_RUNTIME_STUBS(GetLexicalEnv)
{
    RUNTIME_STUBS_HEADER(GetLexicalEnv);
    return thread->GetCurrentLexenv().GetRawData();
}

DEF_RUNTIME_STUBS(LoadValueFromConstantStringTable)
{
    RUNTIME_STUBS_HEADER(LoadValueFromConstantStringTable);
    CONVERT_ARG_TAGGED_CHECKED(id, 0);
    auto tsLoader = thread->GetEcmaVM()->GetTSLoader();
    return tsLoader->GetStringById(id.GetInt()).GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(CallArg0Dyn)
{
    RUNTIME_STUBS_HEADER(CallArg0Dyn);
    CONVERT_ARG_TAGGED_CHECKED(func, 0);
    uint32_t actualNumArgs = EcmaInterpreter::ActualNumArgsOfCall::CALLARG0;
    bool callThis = false;
    std::vector<JSTaggedType> actualArgs;
    return RuntimeNativeCall(thread, func, callThis, actualNumArgs, actualArgs);
}

DEF_RUNTIME_STUBS(CallArg1Dyn)
{
    RUNTIME_STUBS_HEADER(CallArg1Dyn);
    CONVERT_ARG_TAGGED_CHECKED(func, 0);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(arg0, 1);
    uint32_t actualNumArgs = EcmaInterpreter::ActualNumArgsOfCall::CALLARG1;
    bool callThis = false;
    std::vector<JSTaggedType> actualArgs;
    actualArgs.emplace_back(arg0);
    return RuntimeNativeCall(thread, func, callThis, actualNumArgs, actualArgs);
}

DEF_RUNTIME_STUBS(CallArgs2Dyn)
{
    RUNTIME_STUBS_HEADER(CallArgs2Dyn);
    CONVERT_ARG_TAGGED_CHECKED(func, 0);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(arg0, 1);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(arg1, 2);
    uint32_t actualNumArgs = EcmaInterpreter::ActualNumArgsOfCall::CALLARGS2;
    bool callThis = false;
    std::vector<JSTaggedType> actualArgs;
    actualArgs.emplace_back(arg0);
    actualArgs.emplace_back(arg1);
    return RuntimeNativeCall(thread, func, callThis, actualNumArgs, actualArgs);
}

DEF_RUNTIME_STUBS(CallArgs3Dyn)
{
    RUNTIME_STUBS_HEADER(CallArgs3Dyn);
    CONVERT_ARG_TAGGED_CHECKED(func, 0);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(arg0, 1);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(arg1, 2);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(arg2, 3);
    uint32_t actualNumArgs = EcmaInterpreter::ActualNumArgsOfCall::CALLARGS3;
    bool callThis = false;
    std::vector<JSTaggedType> actualArgs;
    actualArgs.emplace_back(arg0);
    actualArgs.emplace_back(arg1);
    actualArgs.emplace_back(arg2);
    return RuntimeNativeCall(thread, func, callThis, actualNumArgs, actualArgs);
}

DEF_RUNTIME_STUBS(CallIThisRangeDyn)
{
    RUNTIME_STUBS_HEADER(CallIThisRangeDyn);
    CONVERT_ARG_TAGGED_CHECKED(func, 0);
    uint32_t actualNumArgs = argc - 2; // 2 : skip func and this
    std::vector<JSTaggedType> actualArgs;
    for (size_t i = 1; i < argc; i++) {
        JSTaggedType arg = reinterpret_cast<JSTaggedType *>(argv)[i];
        actualArgs.emplace_back(arg);
    }
    bool callThis  = true;
    return RuntimeNativeCall(thread, func, callThis, actualNumArgs, actualArgs);
}

DEF_RUNTIME_STUBS(CallIRangeDyn)
{
    RUNTIME_STUBS_HEADER(CallIRangeDyn);
    CONVERT_ARG_TAGGED_CHECKED(func, 0);
    std::vector<JSTaggedType> actualArgs;
    uint32_t actualNumArgs = argc - 1; // 1 : skip func
    for (size_t i = 1; i < argc; i++) {
        JSTaggedType arg = reinterpret_cast<JSTaggedType *>(argv)[i];
        actualArgs.emplace_back(arg);
    }
    bool callThis  = false;
    return RuntimeNativeCall(thread, func, callThis, actualNumArgs, actualArgs);
}

DEF_RUNTIME_STUBS(JumpToCInterpreter)
{
    RUNTIME_STUBS_HEADER(JumpToCInterpreter);
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
}

DEF_RUNTIME_STUBS(CreateEmptyObject)
{
    RUNTIME_STUBS_HEADER(CreateEmptyObject);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    return RuntimeCreateEmptyObject(thread, factory, globalEnv).GetRawData();
}

DEF_RUNTIME_STUBS(CreateEmptyArray)
{
    RUNTIME_STUBS_HEADER(CreateEmptyArray);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    return RuntimeCreateEmptyArray(thread, factory, globalEnv).GetRawData();
}

DEF_RUNTIME_STUBS(GetSymbolFunction)
{
    RUNTIME_STUBS_HEADER(GetSymbolFunction);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    return globalEnv->GetSymbolFunction().GetTaggedValue().GetRawData();
}

DEF_RUNTIME_STUBS(GetUnmapedArgs)
{
    RUNTIME_STUBS_HEADER(GetUnmapedArgs);
    auto sp = const_cast<JSTaggedType*>(thread->GetCurrentSPFrame());
    uint32_t startIdx = 0;
    uint32_t actualNumArgs = EcmaInterpreter::GetNumArgs(sp, 0, startIdx);
    return RuntimeGetUnmapedArgs(thread, sp, actualNumArgs, startIdx).GetRawData();
}

DEF_RUNTIME_STUBS(CopyRestArgs)
{
    RUNTIME_STUBS_HEADER(CopyRestArgs);
    CONVERT_ARG_TAGGED_CHECKED(restIdx, 0);
    auto sp = const_cast<JSTaggedType*>(thread->GetCurrentSPFrame());
    uint32_t startIdx = 0;
    uint32_t restNumArgs = EcmaInterpreter::GetNumArgs(sp, restIdx.GetInt(), startIdx);
    return RuntimeCopyRestArgs(thread, sp, restNumArgs, startIdx).GetRawData();
}

DEF_RUNTIME_STUBS(CreateArrayWithBuffer)
{
    RUNTIME_STUBS_HEADER(CreateArrayWithBuffer);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, argArray, 0);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    return RuntimeCreateArrayWithBuffer(thread, factory, argArray).GetRawData();
}

DEF_RUNTIME_STUBS(CreateObjectWithBuffer)
{
    RUNTIME_STUBS_HEADER(CreateObjectWithBuffer);
    CONVERT_ARG_HANDLE_CHECKED(JSObject, argObj, 0);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    return RuntimeCreateObjectWithBuffer(thread, factory, argObj).GetRawData();
}

DEF_RUNTIME_STUBS(NewLexicalEnvDyn)
{
    RUNTIME_STUBS_HEADER(NewLexicalEnvDyn);
    CONVERT_ARG_TAGGED_CHECKED(numVars, 0);
    return RuntimeNewLexicalEnvDyn(thread, static_cast<uint16_t>(numVars.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(NewObjDynRange)
{
    RUNTIME_STUBS_HEADER(NewObjDynRange);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, func, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, newTarget, 1);
    CONVERT_ARG_TAGGED_CHECKED(firstArgIdx, 2);
    CONVERT_ARG_TAGGED_CHECKED(length, 3);
    return RuntimeNewObjDynRange(thread, func, newTarget, static_cast<uint16_t>(firstArgIdx.GetInt()),
                                 static_cast<uint16_t>(length.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(DefinefuncDyn)
{
    RUNTIME_STUBS_HEADER(DefinefuncDyn);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(func, 0);
    return RuntimeDefinefuncDyn(thread, reinterpret_cast<JSFunction*>(func)).GetRawData();
}

DEF_RUNTIME_STUBS(CreateRegExpWithLiteral)
{
    RUNTIME_STUBS_HEADER(CreateRegExpWithLiteral);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, pattern, 0);
    CONVERT_ARG_TAGGED_CHECKED(flags, 1);
    return RuntimeCreateRegExpWithLiteral(thread, pattern, static_cast<uint8_t>(flags.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(ThrowIfSuperNotCorrectCall)
{
    RUNTIME_STUBS_HEADER(ThrowIfSuperNotCorrectCall);
    CONVERT_ARG_TAGGED_CHECKED(index, 0);
    CONVERT_ARG_TAGGED_CHECKED(thisValue, 1);
    return RuntimeThrowIfSuperNotCorrectCall(thread, static_cast<uint16_t>(index.GetInt()), thisValue).GetRawData();
}

DEF_RUNTIME_STUBS(CreateObjectHavingMethod)
{
    RUNTIME_STUBS_HEADER(CreateObjectHavingMethod);
    CONVERT_ARG_HANDLE_CHECKED(JSObject, literal, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, env, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, constpool, 2);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    return RuntimeCreateObjectHavingMethod(thread, factory, literal, env, constpool).GetRawData();
}

DEF_RUNTIME_STUBS(CreateObjectWithExcludedKeys)
{
    RUNTIME_STUBS_HEADER(CreateObjectWithExcludedKeys);
    CONVERT_ARG_TAGGED_CHECKED(numKeys, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, objVal, 1);
    CONVERT_ARG_TAGGED_CHECKED(firstArgRegIdx, 2);
    return RuntimeCreateObjectWithExcludedKeys(thread, static_cast<uint16_t>(numKeys.GetInt()), objVal,
        static_cast<uint16_t>(firstArgRegIdx.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(DefineNCFuncDyn)
{
    RUNTIME_STUBS_HEADER(DefineNCFuncDyn);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(func, 0);
    return RuntimeDefineNCFuncDyn(thread, reinterpret_cast<JSFunction*>(func)).GetRawData();
}

DEF_RUNTIME_STUBS(DefineGeneratorFunc)
{
    RUNTIME_STUBS_HEADER(DefineGeneratorFunc);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(func, 0);
    return RuntimeDefineGeneratorFunc(thread, reinterpret_cast<JSFunction*>(func)).GetRawData();
}

DEF_RUNTIME_STUBS(DefineAsyncFunc)
{
    RUNTIME_STUBS_HEADER(DefineAsyncFunc);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(func, 0);
    return RuntimeDefineAsyncFunc(thread, reinterpret_cast<JSFunction*>(func)).GetRawData();
}

DEF_RUNTIME_STUBS(DefineMethod)
{
    RUNTIME_STUBS_HEADER(DefineMethod);
    CONVERT_ARG_TAGGED_TYPE_CHECKED(func, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, homeObject, 1);
    return RuntimeDefineMethod(thread, reinterpret_cast<JSFunction*>(func), homeObject).GetRawData();
}

DEF_RUNTIME_STUBS(CallSpreadDyn)
{
    RUNTIME_STUBS_HEADER(CallSpreadDyn);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, func, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, obj, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, array, 2);
    return RuntimeCallSpreadDyn(thread, func, obj, array).GetRawData();
}

DEF_RUNTIME_STUBS(DefineGetterSetterByValue)
{
    RUNTIME_STUBS_HEADER(DefineGetterSetterByValue);
    CONVERT_ARG_HANDLE_CHECKED(JSObject, obj, 0);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, prop, 1);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, getter, 2);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, setter, 3);
    CONVERT_ARG_TAGGED_CHECKED(flag, 4);
    bool bFlag = flag.ToBoolean();
    return RuntimeDefineGetterSetterByValue(thread, obj, prop, getter, setter, bFlag).GetRawData();
}

DEF_RUNTIME_STUBS(SuperCall)
{
    RUNTIME_STUBS_HEADER(SuperCall);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, func, 0);
    CONVERT_ARG_TAGGED_CHECKED(firstVRegIdx, 1);
    CONVERT_ARG_TAGGED_CHECKED(length, 2);
    auto sp = const_cast<JSTaggedType*>(thread->GetCurrentSPFrame());
    JSTaggedValue newTarget = EcmaInterpreter::GetNewTarget(sp);
    return RuntimeSuperCall(thread, func, JSHandle<JSTaggedValue>(thread, newTarget),
        static_cast<uint16_t>(firstVRegIdx.GetInt()),
        static_cast<uint16_t>(length.GetInt())).GetRawData();
}

DEF_RUNTIME_STUBS(SetNotCallableException)
{
    RUNTIME_STUBS_HEADER(SetNotCallableException);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR, "is not callable");
    thread->SetException(error.GetTaggedValue());
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(SetCallConstructorException)
{
    RUNTIME_STUBS_HEADER(SetCallConstructorException);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR,
                                                   "class constructor cannot called without 'new'");
    thread->SetException(error.GetTaggedValue());
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(SetStackOverflowException)
{
    RUNTIME_STUBS_HEADER(SetStackOverflowException);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(base::ErrorType::RANGE_ERROR, "Stack overflow!");
    if (LIKELY(!thread->HasPendingException())) {
        thread->SetException(error.GetTaggedValue());
    }
    return JSTaggedValue::Hole().GetRawData();
}

DEF_RUNTIME_STUBS(CallNative)
{
    RUNTIME_STUBS_HEADER(CallNative);
    CONVERT_ARG_TAGGED_CHECKED(numArgs, 0);
    CONVERT_ARG_PTR_CHECKED(JSTaggedValue *, sp, 1);
    CONVERT_ARG_PTR_CHECKED(JSMethod *, method, 2);

    EcmaRuntimeCallInfo ecmaRuntimeCallInfo(thread, numArgs.GetInt(), sp);
    JSTaggedValue retValue = reinterpret_cast<EcmaEntrypoint>(
        const_cast<void *>(method->GetNativePointer()))(&ecmaRuntimeCallInfo);
    return retValue.GetRawData();
}

DEF_RUNTIME_STUBS(LdBigInt)
{
    RUNTIME_STUBS_HEADER(LdBigInt);
    CONVERT_ARG_HANDLE_CHECKED(JSTaggedValue, numberBigInt, 0);
    return RuntimeLdBigInt(thread, numberBigInt).GetRawData();
}

DEF_RUNTIME_STUBS(NewLexicalEnvWithNameDyn)
{
    RUNTIME_STUBS_HEADER(NewLexicalEnvWithNameDyn);
    CONVERT_ARG_TAGGED_CHECKED(numVars, 0);
    CONVERT_ARG_TAGGED_CHECKED(scopeId, 1);
    return RuntimeNewLexicalEnvWithNameDyn(thread,
        static_cast<uint16_t>(numVars.GetInt()),
        static_cast<uint16_t>(scopeId.GetInt())).GetRawData();
}

int32_t RuntimeStubs::DoubleToInt(double x)
{
    return base::NumberHelper::DoubleToInt(x, base::INT32_BITS);
}

void RuntimeStubs::InsertOldToNewRememberedSet([[maybe_unused]]uintptr_t argGlue, Region* region, uintptr_t addr)
{
    return region->InsertOldToNewRememberedSet(addr);
}

void RuntimeStubs::MarkingBarrier([[maybe_unused]]uintptr_t argGlue, uintptr_t slotAddr,
    Region *objectRegion, TaggedObject *value,
    Region *valueRegion)
{
    if (!valueRegion->IsMarking()) {
        return;
    }
    ::panda::ecmascript::RuntimeApi::MarkObject(slotAddr, objectRegion, value, valueRegion);
}

void RuntimeStubs::Initialize(JSThread *thread)
{
#define DEF_RUNTIME_STUB(name, counter) kungfu::RuntimeStubCSigns::ID_##name
#define INITIAL_RUNTIME_FUNCTIONS(name, count) \
    thread->RegisterRTInterface(DEF_RUNTIME_STUB(name, count), reinterpret_cast<uintptr_t>(name));
    RUNTIME_STUB_LIST(INITIAL_RUNTIME_FUNCTIONS)
#undef INITIAL_RUNTIME_FUNCTIONS
#undef DEF_RUNTIME_STUB
}

#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}  // namespace panda::ecmascript
