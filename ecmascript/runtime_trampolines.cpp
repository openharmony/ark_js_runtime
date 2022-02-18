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
bool RuntimeTrampolines::AddElementInternal(uintptr_t argGlue, JSTaggedType argReceiver, uint32_t argIndex,
                                            JSTaggedType argValue, uint32_t argAttr)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSObject> receiver(thread, JSTaggedValue(argReceiver));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(argValue));
    auto attr = static_cast<PropertyAttributes>(argAttr);
    return JSObject::AddElementInternal(thread, receiver, argIndex, value, attr);
}

bool RuntimeTrampolines::CallSetter(uintptr_t argGlue, JSTaggedType argSetter, JSTaggedType argReceiver,
                                    JSTaggedType argValue, bool argMayThrow)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> receiver(thread, JSTaggedValue(argReceiver));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(argValue));
    auto setter = AccessorData::Cast((reinterpret_cast<TaggedObject *>(argSetter)));
    return JSObject::CallSetter(thread, *setter, receiver, value, argMayThrow);
}

JSTaggedType RuntimeTrampolines::CallSetter2(uintptr_t argGlue, JSTaggedType argReceiver, JSTaggedType argValue,
                                             JSTaggedType argAccessor)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> objHandle(thread, JSTaggedValue(argReceiver));
    JSHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue(argValue));

    auto accessor = AccessorData::Cast(JSTaggedValue(argAccessor).GetTaggedObject());
    bool success = JSObject::CallSetter(thread, *accessor, objHandle, valueHandle, true);
    return success ? JSTaggedValue::Undefined().GetRawData() : JSTaggedValue::Exception().GetRawData();
}

JSTaggedType RuntimeTrampolines::CallGetter2(uintptr_t argGlue, JSTaggedType argReceiver, JSTaggedType argHolder,
                                             JSTaggedType argAccessor)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    AccessorData *accessor = AccessorData::Cast(JSTaggedValue(argAccessor).GetTaggedObject());
    if (UNLIKELY(accessor->IsInternal())) {
        JSHandle<JSObject> objHandle(thread, JSTaggedValue(argHolder));
        return accessor->CallInternalGet(thread, objHandle).GetRawData();
    }
    JSHandle<JSTaggedValue> objHandle(thread, JSTaggedValue(argReceiver));
    return JSObject::CallGetter(thread, accessor, objHandle).GetRawData();
}

void RuntimeTrampolines::ThrowTypeError(uintptr_t argGlue, int argMessageStringId)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    std::string message = MessageString::GetMessageString(argMessageStringId);
    ObjectFactory *factory = JSThread::Cast(thread)->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR, message.c_str());
    THROW_NEW_ERROR_AND_RETURN(thread, error.GetTaggedValue());
}

bool RuntimeTrampolines::JSProxySetProperty(uintptr_t argGlue, JSTaggedType argProxy, JSTaggedType argKey,
                                            JSTaggedType argValue, JSTaggedType argReceiver, bool argMayThrow)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSProxy> proxy(thread, JSTaggedValue(argProxy));
    JSHandle<JSTaggedValue> index(thread, JSTaggedValue(argKey));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(argValue));
    JSHandle<JSTaggedValue> receiver(thread, JSTaggedValue(argReceiver));

    return JSProxy::SetProperty(thread, proxy, index, value, receiver, argMayThrow);
}

uint32_t RuntimeTrampolines::GetHash32(uintptr_t key, uint32_t len)
{
    auto pkey = reinterpret_cast<uint8_t *>(key);
    return panda::GetHash32(pkey, static_cast<size_t>(len));
}

JSTaggedType RuntimeTrampolines::CallGetter(uintptr_t argGlue, JSTaggedType argGetter, JSTaggedType argReceiver)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto accessor = AccessorData::Cast(reinterpret_cast<TaggedObject *>(argGetter));
    JSHandle<JSTaggedValue> objHandle(thread, JSTaggedValue(argReceiver));
    return JSObject::CallGetter(thread, accessor, objHandle).GetRawData();
}

JSTaggedType RuntimeTrampolines::CallInternalGetter(uintptr_t argGlue, JSTaggedType argAccessor,
                                                    JSTaggedType argReceiver)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto accessor = AccessorData::Cast(reinterpret_cast<TaggedObject *>(argAccessor));
    JSHandle<JSObject> objHandle(thread, JSTaggedValue(argReceiver));
    return accessor->CallInternalGet(thread, objHandle).GetRawData();
}

int32_t RuntimeTrampolines::FindElementWithCache(uintptr_t argGlue, JSTaggedType hClass, JSTaggedType key,
                                                 int32_t num)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    auto cls  = reinterpret_cast<JSHClass *>(hClass);
    auto layoutInfo = LayoutInfo::Cast(cls->GetLayout().GetTaggedObject());
    PropertiesCache *cache = thread->GetPropertiesCache();
    int index = cache->Get(cls, JSTaggedValue(key));
    if (index == PropertiesCache::NOT_FOUND) {
        index = layoutInfo->BinarySearch(JSTaggedValue(key), num);
        cache->Set(cls, JSTaggedValue(key), index);
    }
    return index;
}

uint32_t RuntimeTrampolines::StringGetHashCode(JSTaggedType ecmaString)
{
    auto string = reinterpret_cast<EcmaString *>(ecmaString);
    return string->GetHashcode();
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

JSTaggedType RuntimeTrampolines::GetTaggedArrayPtrTest(uintptr_t argGlue)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    // this case static static JSHandle<TaggedArray> arr don't free in first call
    // second call trigger gc.
    // don't call EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    static int i = 0;
    static JSHandle<TaggedArray> arr = factory->NewTaggedArray(2);
    if (i == 0) {
        arr->Set(thread, 0, JSTaggedValue(3)); // 3: first element
        arr->Set(thread, 1, JSTaggedValue(4)); // 4: second element
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

JSTaggedType RuntimeTrampolines::Execute(uintptr_t argGlue, JSTaggedType argFunc, JSTaggedType thisArg,
                                         uint32_t argc, uintptr_t argArgv)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    auto func = reinterpret_cast<ECMAObject *>(argFunc);
    auto argv = reinterpret_cast<const TaggedType *>(argArgv);
    CallParams params;
    params.callTarget = func;
    params.newTarget = JSTaggedValue::VALUE_UNDEFINED;
    params.thisArg = thisArg;
    params.argc = argc;
    params.argv = argv;

    return EcmaInterpreter::Execute(thread, params).GetRawData();
}

double RuntimeTrampolines::FloatMod(double left, double right)
{
    return std::fmod(left, right);
}

JSTaggedType RuntimeTrampolines::NewInternalString(uintptr_t argGlue, JSTaggedType argKey)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(argKey));
    return JSTaggedValue(thread->GetEcmaVM()->GetFactory()->InternString(keyHandle)).GetRawData();
}

JSTaggedType RuntimeTrampolines::NewTaggedArray(uintptr_t argGlue, uint32_t length)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    return factory->NewTaggedArray(length).GetTaggedValue().GetRawData();
}

JSTaggedType RuntimeTrampolines::CopyArray(uintptr_t argGlue, JSTaggedType argArray, uint32_t length, uint32_t capacity)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(thread, JSTaggedValue(argArray));
    return factory->CopyArray(array, length, capacity).GetTaggedValue().GetRawData();
}

JSTaggedType RuntimeTrampolines::NameDictPutIfAbsent(uintptr_t argGlue, JSTaggedType receiver, JSTaggedType array,
    JSTaggedType key, JSTaggedType value, uint32_t attr, bool needTransToDict)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(key));
    JSHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue(value));
    PropertyAttributes propAttr(attr);
    if (needTransToDict) {
        JSHandle<JSObject> objHandle(thread, JSTaggedValue(receiver));
        JSHandle<NameDictionary> dictHandle(JSObject::TransitionToDictionary(thread, objHandle));
        return NameDictionary::
            PutIfAbsent(thread, dictHandle, keyHandle, valueHandle, propAttr).GetTaggedValue().GetRawData();
    } else {
        JSHandle<NameDictionary> dictHandle(thread, JSTaggedValue(array));
        return NameDictionary::
            PutIfAbsent(thread, dictHandle, keyHandle, valueHandle, propAttr).GetTaggedValue().GetRawData();
    }
}

void RuntimeTrampolines::PropertiesSetValue(uintptr_t argGlue, JSTaggedType argReceiver, JSTaggedType argValue,
                                            JSTaggedType argArray, uint32_t capacity, uint32_t index)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<TaggedArray> properties;
    JSHandle<JSObject> objHandle(thread, reinterpret_cast<JSObject *>(argReceiver));
    JSHandle<JSTaggedValue> valueHandle(thread, JSTaggedValue(argValue));
    if (capacity == 0) {
        capacity = JSObject::MIN_PROPERTIES_LENGTH;
        properties = factory->NewTaggedArray(capacity);
    } else {
        auto arrayHandle = JSHandle<TaggedArray>(thread, reinterpret_cast<TaggedArray *>(argArray));
        properties = factory->CopyArray(arrayHandle, capacity,
                                        JSObject::ComputePropertyCapacity(capacity));
    }
    properties->Set(thread, index, valueHandle);
    objHandle->SetProperties(thread, properties);
}

JSTaggedType RuntimeTrampolines::TaggedArraySetValue(uintptr_t argGlue, JSTaggedType argReceiver,
                                                     JSTaggedType argValue, JSTaggedType argElement,
                                                     uint32_t elementIndex, uint32_t capacity)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    auto elements = reinterpret_cast<TaggedArray *>(argElement);
    JSTaggedValue value(argValue);
    if (elementIndex >= capacity) {
        if (JSObject::ShouldTransToDict(capacity, elementIndex)) {
            return JSTaggedValue::Hole().GetRawData();
        }
        [[maybe_unused]] EcmaHandleScope handleScope(thread);
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

JSTaggedType RuntimeTrampolines::NewEcmaDynClass(uintptr_t argGlue, uint32_t size, uint32_t type, uint32_t inlinedProps)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return (thread->GetEcmaVM()->GetFactory()->NewEcmaDynClass(size, JSType(type), inlinedProps)).
        GetTaggedValue().GetRawData();
}

void RuntimeTrampolines::UpdateLayOutAndAddTransition(uintptr_t argGlue, JSTaggedType oldHClass,
                                                      JSTaggedType newHClass, JSTaggedType key, uint32_t attr)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    auto factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSHClass> oldHClassHandle(thread, reinterpret_cast<JSHClass *>(oldHClass));
    JSHandle<JSHClass> newHClassHandle(thread, reinterpret_cast<JSHClass *>(newHClass));
    JSHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue(key));
    PropertyAttributes attrValue(attr);
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
    return;
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

void RuntimeTrampolines::NoticeThroughChainAndRefreshUser(uintptr_t argGlue, JSTaggedType argoldHClass,
                                                          JSTaggedType argnewHClass)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSHClass> oldHClassHandle(thread, reinterpret_cast<JSHClass *>(argoldHClass));
    JSHandle<JSHClass> newHClassHandle(thread, reinterpret_cast<JSHClass *>(argnewHClass));

    JSHClass::NoticeThroughChain(thread, oldHClassHandle);
    JSHClass::RefreshUsers(thread, oldHClassHandle, newHClassHandle);
}

uintptr_t RuntimeTrampolines::JumpToCInterpreter(uintptr_t argGlue, uintptr_t pc, uintptr_t sp,
    JSTaggedType constpool, JSTaggedType profileTypeInfo, JSTaggedType acc, int32_t hotnessCounter)
{
    return 0;
}

JSTaggedType RuntimeTrampolines::IncDyn(uintptr_t argGlue, JSTaggedType value)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::IncDyn(thread, JSTaggedValue(value)).GetRawData();
}

JSTaggedType RuntimeTrampolines::DecDyn(uintptr_t argGlue, JSTaggedType value)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::DecDyn(thread, JSTaggedValue(value)).GetRawData();
}

JSTaggedType RuntimeTrampolines::ExpDyn(uintptr_t argGlue, JSTaggedType base, JSTaggedType exponent)
{
    JSTaggedValue baseValue(base);
    JSTaggedValue exponentValue(exponent);
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
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::ExpDyn(thread, baseValue, exponentValue).GetRawData();
}

JSTaggedType RuntimeTrampolines::IsInDyn(uintptr_t argGlue, JSTaggedType prop, JSTaggedType obj)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::IsInDyn(thread, JSTaggedValue(prop), JSTaggedValue(obj)).GetRawData();
}

JSTaggedType RuntimeTrampolines::InstanceOfDyn(uintptr_t argGlue, JSTaggedType obj, JSTaggedType target)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::InstanceofDyn(thread, JSTaggedValue(obj), JSTaggedValue(target)).GetRawData();
}

JSTaggedType RuntimeTrampolines::FastStrictNotEqual(JSTaggedType left, JSTaggedType right)
{
    bool result = FastRuntimeStub::FastStrictEqual(JSTaggedValue(left), JSTaggedValue(right));
    if (result) {
        return JSTaggedValue::False().GetRawData();
    }
    return JSTaggedValue::True().GetRawData();
}

JSTaggedType RuntimeTrampolines::FastStrictEqual(JSTaggedType left, JSTaggedType right)
{
    bool result = FastRuntimeStub::FastStrictEqual(JSTaggedValue(left), JSTaggedValue(right));
    if (result) {
        return JSTaggedValue::True().GetRawData();
    }
    return JSTaggedValue::False().GetRawData();
}

JSTaggedType RuntimeTrampolines::CreateGeneratorObj(uintptr_t argGlue, JSTaggedType genFunc)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::CreateGeneratorObj(thread, JSTaggedValue(genFunc)).GetRawData();
}

void RuntimeTrampolines::ThrowConstAssignment(uintptr_t argGlue, JSTaggedType value)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    SlowRuntimeStub::ThrowConstAssignment(thread, JSTaggedValue(value));
}

JSTaggedType RuntimeTrampolines::GetTemplateObject(uintptr_t argGlue, JSTaggedType literal)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::GetTemplateObject(thread, JSTaggedValue(literal)).GetRawData();
}

JSTaggedType RuntimeTrampolines::GetNextPropName(uintptr_t argGlue, JSTaggedType iter)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::GetNextPropName(thread, JSTaggedValue(iter)).GetRawData();
}

void RuntimeTrampolines::ThrowIfNotObject(uintptr_t argGlue)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    SlowRuntimeStub::ThrowIfNotObject(thread);
}

JSTaggedType RuntimeTrampolines::IterNext(uintptr_t argGlue, JSTaggedType iter)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::IterNext(thread, JSTaggedValue(iter)).GetRawData();
}

JSTaggedType RuntimeTrampolines::CloseIterator(uintptr_t argGlue, JSTaggedType iter)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::CloseIterator(thread, JSTaggedValue(iter)).GetRawData();
}

void RuntimeTrampolines::CopyModule(uintptr_t argGlue, JSTaggedType srcModule)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    SlowRuntimeStub::CopyModule(thread, JSTaggedValue(srcModule));
}

JSTaggedType RuntimeTrampolines::SuperCallSpread(uintptr_t argGlue,
                                                 JSTaggedType func, uintptr_t sp, JSTaggedType array)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    JSTaggedValue function = EcmaInterpreter::GetNewTarget(reinterpret_cast<JSTaggedType *>(sp));
    return SlowRuntimeStub::SuperCallSpread(thread, JSTaggedValue(func), function, JSTaggedValue(array)).GetRawData();
}

JSTaggedType RuntimeTrampolines::DelObjProp(uintptr_t argGlue, JSTaggedType obj, JSTaggedType prop)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::DelObjProp(thread, JSTaggedValue(obj), JSTaggedValue(prop)).GetRawData();
}

JSTaggedType RuntimeTrampolines::NewObjSpreadDyn(uintptr_t argGlue,
                                                 JSTaggedType func, JSTaggedType newTarget, JSTaggedType array)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::NewObjSpreadDyn(thread,
        JSTaggedValue(func), JSTaggedValue(newTarget), JSTaggedValue(array)).GetRawData();
}

JSTaggedType RuntimeTrampolines::CreateIterResultObj(uintptr_t argGlue, JSTaggedType value, JSTaggedType flag)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::CreateIterResultObj(thread, JSTaggedValue(value), JSTaggedValue(flag)).GetRawData();
}

JSTaggedType RuntimeTrampolines::AsyncFunctionAwaitUncaught(uintptr_t argGlue,
                                                            JSTaggedType asyncFuncObj, JSTaggedType value)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::AsyncFunctionAwaitUncaught(thread,
        JSTaggedValue(asyncFuncObj), JSTaggedValue(value)).GetRawData();
}

void RuntimeTrampolines::ThrowUndefinedIfHole(uintptr_t argGlue, JSTaggedType obj)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    SlowRuntimeStub::ThrowUndefinedIfHole(thread, JSTaggedValue(obj));
}

JSTaggedType RuntimeTrampolines::CopyDataProperties(uintptr_t argGlue, JSTaggedType dst, JSTaggedType src)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::CopyDataProperties(thread, JSTaggedValue(dst), JSTaggedValue(src)).GetRawData();
}

JSTaggedType RuntimeTrampolines::StArraySpread(uintptr_t argGlue,
                                               JSTaggedType dst, JSTaggedType index, JSTaggedType src)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::StArraySpread(thread,
        JSTaggedValue(dst), JSTaggedValue(index), JSTaggedValue(src)).GetRawData();
}

JSTaggedType RuntimeTrampolines::GetIteratorNext(uintptr_t argGlue, JSTaggedType obj, JSTaggedType method)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::GetIteratorNext(thread, JSTaggedValue(obj), JSTaggedValue(method)).GetRawData();
}

JSTaggedType RuntimeTrampolines::SetObjectWithProto(uintptr_t argGlue, JSTaggedType proto, JSTaggedType obj)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::SetObjectWithProto(thread, JSTaggedValue(proto), JSTaggedValue(obj)).GetRawData();
}

JSTaggedType RuntimeTrampolines::LoadICByValue(uintptr_t argGlue, JSTaggedType profileTypeInfo,
                                               JSTaggedType receiver, JSTaggedType propKey, int32_t slotId)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    INTERPRETER_TRACE(thread, LoadICByValue);

    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto profileHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(profileTypeInfo));
    if (profileHandle->IsUndefined()) {
        return SlowRuntimeStub::LdObjByValue(thread, JSTaggedValue(receiver), JSTaggedValue(propKey),
            false, JSTaggedValue::Undefined()).GetRawData();
    }
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(receiver));
    auto keyHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(propKey));
    LoadICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileHandle), slotId, ICKind::LoadIC);
    return icRuntime.LoadMiss(receiverHandle, keyHandle).GetRawData();
}

JSTaggedType RuntimeTrampolines::StoreICByValue(uintptr_t argGlue, JSTaggedType profileTypeInfo,
                                                JSTaggedType receiver, JSTaggedType propKey, JSTaggedType value,
                                                int32_t slotId)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    INTERPRETER_TRACE(thread, StoreICByValue);

    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto profileHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(profileTypeInfo));
    if (profileHandle->IsUndefined()) {
        return SlowRuntimeStub::StObjByValue(thread,
            JSTaggedValue(receiver), JSTaggedValue(propKey), JSTaggedValue(value)).GetRawData();
    }
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(receiver));
    auto keyHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(propKey));
    auto valueHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(value));
    StoreICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileHandle), slotId, ICKind::StoreIC);
    return icRuntime.StoreMiss(receiverHandle, keyHandle, valueHandle).GetRawData();
}

JSTaggedType RuntimeTrampolines::StOwnByValue(uintptr_t argGlue,
                                              JSTaggedType obj, JSTaggedType key, JSTaggedType value)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::StOwnByValue(thread,
        JSTaggedValue(obj), JSTaggedValue(key), JSTaggedValue(value)).GetRawData();
}

JSTaggedType RuntimeTrampolines::LdSuperByValue(uintptr_t argGlue, JSTaggedType obj, JSTaggedType key, uintptr_t sp)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    JSTaggedValue thisFunc = EcmaInterpreter::GetThisFunction(reinterpret_cast<JSTaggedType *>(sp));
    return SlowRuntimeStub::LdSuperByValue(thread, JSTaggedValue(obj), JSTaggedValue(key), thisFunc).GetRawData();
}

JSTaggedType RuntimeTrampolines::StSuperByValue(uintptr_t argGlue,
                                                JSTaggedType obj, JSTaggedType key, JSTaggedType value, uintptr_t sp)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    JSTaggedValue thisFunc = EcmaInterpreter::GetThisFunction(reinterpret_cast<JSTaggedType *>(sp));
    return SlowRuntimeStub::StSuperByValue(thread,
        JSTaggedValue(obj), JSTaggedValue(key), JSTaggedValue(value), thisFunc).GetRawData();
}

JSTaggedType RuntimeTrampolines::LdObjByIndex(uintptr_t argGlue, JSTaggedType obj, uint32_t idx,
                                              bool callGetter, JSTaggedType receiver)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::LdObjByIndex(thread, JSTaggedValue(obj), idx,
                                         callGetter, JSTaggedValue(receiver)).GetRawData();
}

JSTaggedType RuntimeTrampolines::StObjByIndex(uintptr_t argGlue, JSTaggedType obj, uint32_t idx, JSTaggedType value)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::StObjByIndex(thread, JSTaggedValue(obj), idx, JSTaggedValue(value)).GetRawData();
}

JSTaggedType RuntimeTrampolines::StOwnByIndex(uintptr_t argGlue, JSTaggedType obj, uint32_t idx, JSTaggedType value)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::StOwnByIndex(thread, JSTaggedValue(obj), idx, JSTaggedValue(value)).GetRawData();
}

JSTaggedType RuntimeTrampolines::StGlobalRecord(uintptr_t argGlue, JSTaggedType prop, JSTaggedType value, bool isConst)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::StGlobalRecord(thread, JSTaggedValue(prop), JSTaggedValue(value), isConst).GetRawData();
}

JSTaggedType RuntimeTrampolines::NegDyn(uintptr_t argGlue, JSTaggedType value)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::NegDyn(thread, JSTaggedValue(value)).GetRawData();
}

JSTaggedType RuntimeTrampolines::NotDyn(uintptr_t argGlue, JSTaggedType value)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::NotDyn(thread, JSTaggedValue(value)).GetRawData();
}

JSTaggedType RuntimeTrampolines::ChangeUintAndIntShrToJSTaggedValue(uintptr_t argGlue, JSTaggedType leftInt,
                                                                    JSTaggedType rightUint)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> leftHandle(thread, JSTaggedValue(leftInt));
    JSHandle<JSTaggedValue> rightHandle(thread, JSTaggedValue(rightUint));
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

JSTaggedType RuntimeTrampolines::ChangeTwoInt32AndToJSTaggedValue(uintptr_t argGlue, JSTaggedType left,
                                                                  JSTaggedType right)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> leftHandle(thread, JSTaggedValue(left));
    JSHandle<JSTaggedValue> rightHandle(thread, JSTaggedValue(right));
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

JSTaggedType RuntimeTrampolines::ChangeTwoInt32OrToJSTaggedValue(uintptr_t argGlue, JSTaggedType left,
                                                                 JSTaggedType right)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> leftHandle(thread, JSTaggedValue(left));
    JSHandle<JSTaggedValue> rightHandle(thread, JSTaggedValue(right));
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

JSTaggedType RuntimeTrampolines::ChangeTwoInt32XorToJSTaggedValue(uintptr_t argGlue, JSTaggedType left,
                                                                  JSTaggedType right)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> leftHandle(thread, JSTaggedValue(left));
    JSHandle<JSTaggedValue> rightHandle(thread, JSTaggedValue(right));
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

JSTaggedType RuntimeTrampolines::ChangeTwoUint32AndToJSTaggedValue(uintptr_t argGlue, JSTaggedType left,
                                                                   JSTaggedType right)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> leftHandle(thread, JSTaggedValue(left));
    JSHandle<JSTaggedValue> rightHandle(thread, JSTaggedValue(right));
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

JSTaggedType RuntimeTrampolines::ChangeUintAndIntShlToJSTaggedValue(uintptr_t argGlue, JSTaggedType leftInt,
                                                                    JSTaggedType rightUint)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> leftHandle(thread, JSTaggedValue(leftInt));
    JSHandle<JSTaggedValue> rightHandle(thread, JSTaggedValue(rightUint));
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

JSTaggedType RuntimeTrampolines::ResolveClass(uintptr_t argGlue, JSTaggedType ctor, JSTaggedType literal,
    JSTaggedType base, JSTaggedType lexenv, JSTaggedType constpool)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::ResolveClass(thread, JSTaggedValue(ctor), reinterpret_cast<TaggedArray *>(literal),
        JSTaggedValue(base), JSTaggedValue(lexenv), reinterpret_cast<ConstantPool *>(constpool)).GetRawData();
}

JSTaggedType RuntimeTrampolines::CloneClassFromTemplate(uintptr_t argGlue, JSTaggedType ctor, JSTaggedType base,
    JSTaggedType lexenv, JSTaggedType constpool)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::CloneClassFromTemplate(thread, JSTaggedValue(ctor), JSTaggedValue(base),
        JSTaggedValue(lexenv), reinterpret_cast<ConstantPool *>(constpool)).GetRawData();
}

JSTaggedType RuntimeTrampolines::SetClassConstructorLength(uintptr_t argGlue, JSTaggedType ctor, uint16_t length)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::SetClassConstructorLength(thread, JSTaggedValue(ctor), JSTaggedValue(length)).GetRawData();
}

JSTaggedType RuntimeTrampolines::UpdateHotnessCounter(uintptr_t argGlue, uintptr_t sp)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    InterpretedFrame *state = GET_FRAME(sp);
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

JSTaggedType RuntimeTrampolines::LoadICByName(uintptr_t argGlue, JSTaggedType profileTypeInfo,
    JSTaggedType receiver, JSTaggedType propKey, int32_t slotId)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    INTERPRETER_TRACE(thread, LoadICByName);

    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto keyHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(propKey));
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(receiver));
    auto profileHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(profileTypeInfo));
    if (profileHandle->IsUndefined()) {
        auto res = JSTaggedValue::GetProperty(thread, receiverHandle, keyHandle).GetValue().GetTaggedValue();
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
        return res.GetRawData();
    }
    LoadICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileHandle), slotId, ICKind::NamedLoadIC);
    return icRuntime.LoadMiss(receiverHandle, keyHandle).GetRawData();
}

JSTaggedType RuntimeTrampolines::StoreICByName(uintptr_t argGlue, JSTaggedType profileTypeInfo,
    JSTaggedType receiver, JSTaggedType propKey, JSTaggedType value, int32_t slotId)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    INTERPRETER_TRACE(thread, StoreICByName);

    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    auto profileHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(profileTypeInfo));
    auto receiverHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(receiver));
    auto keyHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(propKey));
    auto valueHandle = JSHandle<JSTaggedValue>(thread, reinterpret_cast<TaggedObject *>(value));
    if (profileHandle->IsUndefined()) {
        JSTaggedValue::SetProperty(thread, receiverHandle, keyHandle, valueHandle, true);
        RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, JSTaggedValue::Exception().GetRawData());
        return JSTaggedValue::True().GetRawData();
    }
    StoreICRuntime icRuntime(thread, JSHandle<ProfileTypeInfo>::Cast(profileHandle), slotId, ICKind::NamedStoreIC);
    return icRuntime.StoreMiss(receiverHandle, keyHandle, valueHandle).GetRawData();
}

void RuntimeTrampolines::SetFunctionNameNoPrefix(uintptr_t argGlue, JSTaggedType argFunc, JSTaggedType argName)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    JSFunction::SetFunctionNameNoPrefix(thread, reinterpret_cast<JSFunction *>(argFunc), JSTaggedValue(argName));
}

JSTaggedType RuntimeTrampolines::StOwnByValueWithNameSet(uintptr_t argGlue, JSTaggedType obj, JSTaggedType key,
                                                         JSTaggedType value)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::StOwnByValueWithNameSet(
        thread, JSTaggedValue(obj), JSTaggedValue(key), JSTaggedValue(value)).GetRawData();
}

JSTaggedType RuntimeTrampolines::StOwnByName(uintptr_t argGlue, JSTaggedType obj, JSTaggedType prop, JSTaggedType value)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::StOwnByName(
        thread, JSTaggedValue(obj), JSTaggedValue(prop), JSTaggedValue(value)).GetRawData();
}

JSTaggedType RuntimeTrampolines::StOwnByNameWithNameSet(uintptr_t argGlue, JSTaggedType obj, JSTaggedType prop,
                                                        JSTaggedType value)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::StOwnByValueWithNameSet(
        thread, JSTaggedValue(obj), JSTaggedValue(prop), JSTaggedValue(value)).GetRawData();
}

JSTaggedType RuntimeTrampolines::SuspendGenerator(uintptr_t argGlue, JSTaggedType obj, JSTaggedType value)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::SuspendGenerator(thread, JSTaggedValue(obj), JSTaggedValue(value)).GetRawData();
}

uintptr_t RuntimeTrampolines::UpFrame(uintptr_t argGlue, uintptr_t sp)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    InterpretedFrameHandler frameHandler(reinterpret_cast<JSTaggedType *>(sp));
    uint32_t pcOffset = panda_file::INVALID_OFFSET;
    for (; frameHandler.HasFrame(); frameHandler.PrevInterpretedFrame()) {
        if (frameHandler.IsBreakFrame()) {
            return reinterpret_cast<uintptr_t>(nullptr);
        }
        auto method = frameHandler.GetMethod();
        pcOffset = EcmaInterpreter::FindCatchBlock(method, frameHandler.GetBytecodeOffset());
        if (pcOffset != panda_file::INVALID_OFFSET) {
            thread->SetCurrentSPFrame(frameHandler.GetSp());
            return reinterpret_cast<uintptr_t>(method->GetBytecodeArray() + pcOffset);
        }
    }
    return reinterpret_cast<uintptr_t>(nullptr);
}

JSTaggedType RuntimeTrampolines::ImportModule(uintptr_t argGlue, JSTaggedType moduleName)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::ImportModule(thread, JSTaggedValue(moduleName)).GetRawData();
}

void RuntimeTrampolines::StModuleVar(uintptr_t argGlue, JSTaggedType exportName, JSTaggedType exportObj)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    SlowRuntimeStub::StModuleVar(thread, JSTaggedValue(exportName), JSTaggedValue(exportObj));
}

JSTaggedType RuntimeTrampolines::LdModvarByName(uintptr_t argGlue, JSTaggedType moduleObj, JSTaggedType itemName)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::LdModvarByName(thread, JSTaggedValue(moduleObj), JSTaggedValue(itemName)).GetRawData();
}

void RuntimeTrampolines::ThrowDyn(uintptr_t argGlue, JSTaggedType value)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    SlowRuntimeStub::ThrowDyn(thread, JSTaggedValue(value));
}

JSTaggedType RuntimeTrampolines::GetPropIterator(uintptr_t argGlue, JSTaggedType value)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::GetPropIterator(thread, JSTaggedValue(value)).GetRawData();
}

JSTaggedType RuntimeTrampolines::AsyncFunctionEnter(uintptr_t argGlue)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::AsyncFunctionEnter(thread).GetRawData();
}

JSTaggedType RuntimeTrampolines::GetIterator(uintptr_t argGlue, JSTaggedType obj)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::GetIterator(thread, JSTaggedValue(obj)).GetRawData();
}

void RuntimeTrampolines::ThrowThrowNotExists(uintptr_t argGlue)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    SlowRuntimeStub::ThrowThrowNotExists(thread);
}

void RuntimeTrampolines::ThrowPatternNonCoercible(uintptr_t argGlue)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    SlowRuntimeStub::ThrowPatternNonCoercible(thread);
}

void RuntimeTrampolines::ThrowDeleteSuperProperty(uintptr_t argGlue)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    SlowRuntimeStub::ThrowDeleteSuperProperty(thread);
}

JSTaggedType RuntimeTrampolines::EqDyn(uintptr_t argGlue, JSTaggedType left, JSTaggedType right)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::EqDyn(thread, JSTaggedValue(left), JSTaggedValue(right)).GetRawData();
}

JSTaggedType RuntimeTrampolines::LdGlobalRecord(uintptr_t argGlue, JSTaggedType key)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::LdGlobalRecord(thread, JSTaggedValue(key)).GetRawData();
}

JSTaggedType RuntimeTrampolines::GetGlobalOwnProperty(uintptr_t argGlue, JSTaggedType key)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    JSTaggedValue globalObj = globalEnv->GetGlobalObject();
    return FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, JSTaggedValue(key)).GetRawData();
}

JSTaggedType RuntimeTrampolines::TryLdGlobalByName(uintptr_t argGlue, JSTaggedType prop)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    JSTaggedValue globalObj = globalEnv->GetGlobalObject();
    return SlowRuntimeStub::TryLdGlobalByName(thread, globalObj, JSTaggedValue(prop)).GetRawData();
}

JSTaggedType RuntimeTrampolines::LoadMiss(uintptr_t argGlue, JSTaggedType profileTypeInfo, JSTaggedType receiver,
                                          JSTaggedType key, uint32_t slotId, uint32_t kind)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return ICRuntimeStub::LoadMiss(thread, reinterpret_cast<ProfileTypeInfo *>(profileTypeInfo),
        JSTaggedValue(receiver), JSTaggedValue(key), slotId, static_cast<ICKind>(kind)).GetRawData();
}

JSTaggedType RuntimeTrampolines::StoreMiss(uintptr_t argGlue, JSTaggedType profileTypeInfo, JSTaggedType receiver,
                                           JSTaggedType key, JSTaggedType value, uint32_t slotId, uint32_t kind)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return ICRuntimeStub::StoreMiss(
        thread, reinterpret_cast<ProfileTypeInfo *>(profileTypeInfo), JSTaggedValue(receiver), JSTaggedValue(key),
        JSTaggedValue(value), slotId, static_cast<ICKind>(kind)).GetRawData();
}

JSTaggedType RuntimeTrampolines::TryUpdateGlobalRecord(uintptr_t argGlue, JSTaggedType prop, JSTaggedType value)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::TryUpdateGlobalRecord(thread, JSTaggedValue(prop), JSTaggedValue(value)).GetRawData();
}

JSTaggedType RuntimeTrampolines::ThrowReferenceError(uintptr_t argGlue, JSTaggedType prop)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::ThrowReferenceError(
        thread, JSTaggedValue(prop), " is not defined").GetRawData();
}

JSTaggedType RuntimeTrampolines::LdGlobalVar(uintptr_t argGlue, JSTaggedType global, JSTaggedType prop)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::LdGlobalVar(thread, JSTaggedValue(global), JSTaggedValue(prop)).GetRawData();
}

JSTaggedType RuntimeTrampolines::StGlobalVar(uintptr_t argGlue, JSTaggedType prop, JSTaggedType value)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::StGlobalVar(thread, JSTaggedValue(prop), JSTaggedValue(value)).GetRawData();
}

JSTaggedType RuntimeTrampolines::ToNumber(uintptr_t argGlue, JSTaggedType value)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::ToNumber(thread, JSTaggedValue(value)).GetRawData();
}

bool RuntimeTrampolines::ToBoolean(JSTaggedType value)
{
    return JSTaggedValue(value).ToBoolean();
}

JSTaggedType RuntimeTrampolines::NotEqDyn(uintptr_t argGlue, JSTaggedType left, JSTaggedType right)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::NotEqDyn(thread, JSTaggedValue(left), JSTaggedValue(right)).GetRawData();
}

JSTaggedType RuntimeTrampolines::LessDyn(uintptr_t argGlue, JSTaggedType left, JSTaggedType right)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::LessDyn(thread, JSTaggedValue(left), JSTaggedValue(right)).GetRawData();
}

JSTaggedType RuntimeTrampolines::LessEqDyn(uintptr_t argGlue, JSTaggedType left, JSTaggedType right)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::LessEqDyn(thread, JSTaggedValue(left), JSTaggedValue(right)).GetRawData();
}

JSTaggedType RuntimeTrampolines::GreaterDyn(uintptr_t argGlue, JSTaggedType left, JSTaggedType right)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::GreaterDyn(thread, JSTaggedValue(left), JSTaggedValue(right)).GetRawData();
}

JSTaggedType RuntimeTrampolines::GreaterEqDyn(uintptr_t argGlue, JSTaggedType left, JSTaggedType right)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::GreaterEqDyn(thread, JSTaggedValue(left), JSTaggedValue(right)).GetRawData();
}

JSTaggedType RuntimeTrampolines::Add2Dyn(uintptr_t argGlue, JSTaggedType left, JSTaggedType right)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    return SlowRuntimeStub::Add2Dyn(thread, ecmaVm, JSTaggedValue(left), JSTaggedValue(right)).GetRawData();
}

JSTaggedType RuntimeTrampolines::Sub2Dyn(uintptr_t argGlue, JSTaggedType left, JSTaggedType right)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::Sub2Dyn(thread, JSTaggedValue(left), JSTaggedValue(right)).GetRawData();
}

JSTaggedType RuntimeTrampolines::Mul2Dyn(uintptr_t argGlue, JSTaggedType left, JSTaggedType right)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::Mul2Dyn(thread, JSTaggedValue(left), JSTaggedValue(right)).GetRawData();
}

JSTaggedType RuntimeTrampolines::Div2Dyn(uintptr_t argGlue, JSTaggedType left, JSTaggedType right)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::Div2Dyn(thread, JSTaggedValue(left), JSTaggedValue(right)).GetRawData();
}

JSTaggedType RuntimeTrampolines::Mod2Dyn(uintptr_t argGlue, JSTaggedType left, JSTaggedType right)
{
    auto thread = JSThread::GlueToJSThread(argGlue);
    return SlowRuntimeStub::Mod2Dyn(thread, JSTaggedValue(left), JSTaggedValue(right)).GetRawData();
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
