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

#include "ecmascript/compiler/stub_descriptor.h"

#include "llvm-c/Core.h"
#include "llvm/Support/Host.h"

namespace panda::ecmascript::kungfu {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_STUB_INIT_DESCRIPTOR(name)                              \
    class Stub##name##InterfaceDescriptor final {                    \
    public:                                                          \
        static void Initialize(StubDescriptor *descriptor); \
    };                                                               \
    void Stub##name##InterfaceDescriptor::Initialize(StubDescriptor *descriptor)


CALL_STUB_INIT_DESCRIPTOR(FastAdd)
{
    // 2 : 2 input parameters
    StubDescriptor fastAdd("FastAdd", 0, 2, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT64); // number or hole
    *descriptor = fastAdd;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FastSub)
{
    // 2 : 2 input parameters
    StubDescriptor fastSub("FastSub", 0, 2, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT64); // number or hole
    *descriptor = fastSub;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FastMul)
{
    // 2 : 2 input parameters
    StubDescriptor fastMul("FastMul", 0, 2, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT64); // number or hole
    *descriptor = fastMul;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

#ifndef NDEBUG
CALL_STUB_INIT_DESCRIPTOR(FastMulGCTest)
{
    // 3 : 3 input parameters
    StubDescriptor fastMulGC("FastMulGCTest", 0, 3, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT64);
    *descriptor = fastMulGC;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::UINT64,
        StubMachineType::UINT64,
    };
    descriptor->SetParameters(params.data());
}
#else
CALL_STUB_INIT_DESCRIPTOR(FastMulGCTest) {}
#endif

CALL_STUB_INIT_DESCRIPTOR(FastDiv)
{
    // 2 : 2 input parameters
    StubDescriptor fastDiv("FastDiv", 0, 2, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT64); // float or hole
    *descriptor = fastDiv;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FastMod)
{
    // 3 : 3 input parameters
    StubDescriptor fastMod("FastMod", 0, 3, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT64);
    *descriptor = fastMod;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FastTypeOf)
{
    // 2 input parameters
    StubDescriptor fastTypeOf("FastTypeOf", 0, 2, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED_POINTER);
    *descriptor = fastTypeOf;
    // 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::NATIVE_POINTER, // glue
        StubMachineType::TAGGED, // ACC
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FastEqual)
{
    // 2 input parameters, return may be true/false/hole
    StubDescriptor fastEqual("FastEqual", 0, 2, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT64);
    *descriptor = fastEqual;
    // 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(SetPropertyByName)
{
    // 4 : 4 input parameters
    StubDescriptor setPropertyByName("SetPropertyByName", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        StubMachineType::UINT64);
    *descriptor = setPropertyByName;

    std::array<StubMachineType, 4> params = { // 4 : 4 input parameters
        StubMachineType::NATIVE_POINTER, StubMachineType::TAGGED_POINTER, StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED_POINTER
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(SetPropertyByNameWithOwn)
{
    // 4 : 4 input parameters
    StubDescriptor setPropertyByNameWithOwn("SetPropertyByNameWithOwn", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        StubMachineType::UINT64);
    *descriptor = setPropertyByNameWithOwn;

    std::array<StubMachineType, 4> params = { // 4 : 4 input parameters
        StubMachineType::NATIVE_POINTER, StubMachineType::TAGGED_POINTER, StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED_POINTER
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(GetPropertyByName)
{
        // 3 : 3 input parameters
    StubDescriptor getPropertyByName("GetPropertyByName", 0, 3, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = getPropertyByName;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER, // glue
        StubMachineType::TAGGED,         // receiver
        StubMachineType::TAGGED_POINTER, // key
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(GetPropertyByIndex)
{
    // 3 : 3 input parameters
    StubDescriptor getPropertyByIndex("GetPropertyByIndex", 0, 3, ArgumentsOrder::DEFAULT_ORDER,
        StubMachineType::TAGGED);
    *descriptor = getPropertyByIndex;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER, // glue
        StubMachineType::TAGGED, // receiver
        StubMachineType::UINT32, // index
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(SetPropertyByIndex)
{
    // 4 : 4 input parameters
    StubDescriptor setPropertyByIndex("SetPropertyByIndex", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        StubMachineType::UINT64); // hole or undefined
    *descriptor = setPropertyByIndex;
    // 4 : 4 input parameters
    std::array<StubMachineType, 4> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::UINT32,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(GetPropertyByValue)
{
    // 3 : 3 input parameters
    StubDescriptor getPropertyByValue("GetPropertyByValue", 0, 3, ArgumentsOrder::DEFAULT_ORDER,
                                      StubMachineType::TAGGED);
    *descriptor = getPropertyByValue;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(JSHClassAddProperty)
{
    // 4 : 4 input parameters
    StubDescriptor jsHClassAddProperty("JSHClassAddProperty", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = jsHClassAddProperty;
    // 4 : 4 input parameters
    std::array<StubMachineType, 4> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(LoadICByName)
{
    // 5 : 5 input parameters
    StubDescriptor loadICByName("LoadICByName", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = loadICByName;
    // 5 : 5 input parameters
    std::array<StubMachineType, 5> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::INT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(TryLoadICByName)
{
    // 4 : 4 input parameters
    StubDescriptor tryLoadICByName("TryLoadICByName", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = tryLoadICByName;
    // 4 : 4 input parameters
    std::array<StubMachineType, 4> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(TryLoadICByValue)
{
    // 5 : 5 input parameters
    StubDescriptor tryLoadICByValue("TryLoadICByValue", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = tryLoadICByValue;
    // 5 : 5 input parameters
    std::array<StubMachineType, 5> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(StoreICByName)
{
    // 6 : 6 input parameters
    StubDescriptor storeICByName("StoreICByName", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT64);
    *descriptor = storeICByName;
    // 6 : 6 input parameters
    std::array<StubMachineType, 6> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::INT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(TryStoreICByName)
{
    // 5 : 5 input parameters
    StubDescriptor tryStoreICByName("TryStoreICByName", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT64); // undefined or hole
    *descriptor = tryStoreICByName;
    // 5 : 5 input parameters
    std::array<StubMachineType, 5> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED_POINTER,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(TryStoreICByValue)
{
    // 6 : 6 input parameters
    StubDescriptor tryStoreICByValue("TryStoreICByValue", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT64); // undefined or hole
    *descriptor = tryStoreICByValue;
    // 6 : 6 input parameters
    std::array<StubMachineType, 6> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED_POINTER,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(TestAbsoluteAddressRelocation)
{
    // 2 : 2 input parameters
    StubDescriptor TestAbsoluteAddressRelocation("TestAbsoluteAddressRelocation", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT64); // undefined or hole
    *descriptor = TestAbsoluteAddressRelocation;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::INT64,
        StubMachineType::INT64,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FloatMod)
{
    // 2 : 2 input parameters
    StubDescriptor floatMod("FloatMod", 0, 2, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::FLOAT64);
    *descriptor = floatMod;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::FLOAT64,
        StubMachineType::FLOAT64,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB_NO_GC);
}

CALL_STUB_INIT_DESCRIPTOR(AddElementInternal)
{
    // 5 : 5 input parameters
    StubDescriptor addElementInternal("AddElementInternal", 0, 5, ArgumentsOrder::DEFAULT_ORDER,
        StubMachineType::BOOL);
    *descriptor = addElementInternal;
    // 5 : 5 input parameters
    std::array<StubMachineType, 5> params = {
        StubMachineType::NATIVE_POINTER, StubMachineType::TAGGED_POINTER, StubMachineType::UINT32,
        StubMachineType::TAGGED, StubMachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetTaggedArrayPtrTest)
{
    StubDescriptor getTaggedArrayPtr("GetTaggedArrayPtrTest", 0, 1, ArgumentsOrder::DEFAULT_ORDER,
                                     StubMachineType::TAGGED_POINTER);
    *descriptor = getTaggedArrayPtr;
    std::array<StubMachineType, 1> params = {
        StubMachineType::NATIVE_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CallSetter)
{
    // 5 : 5 input parameters
    StubDescriptor callSetter("CallSetter", 0, 5, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::BOOL);
    *descriptor = callSetter;
    // 5 : 5 input parameters
    std::array<StubMachineType, 5> params = {
        StubMachineType::NATIVE_POINTER, StubMachineType::TAGGED, StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED, StubMachineType::BOOL,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CallSetter2)
{
    // 4 : 4 input parameters, return Undefined or Exception
    StubDescriptor callSetter("CallSetter2", 0, 4, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT64);
    *descriptor = callSetter;
    // 4 : 4 input parameters
    std::array<StubMachineType, 4> params = {
        StubMachineType::NATIVE_POINTER, StubMachineType::TAGGED, StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CallGetter)
{
    // 3 : 3 input parameters
    StubDescriptor callGetter("CallGetter", 0, 3, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = callGetter;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED, // getter
        StubMachineType::TAGGED_POINTER, // receiver
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CallGetter2)
{
    // 4 : 4 input parameters
    StubDescriptor callGetter("CallGetter2", 0, 4, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = callGetter;
    // 4 : 4 input parameters
    std::array<StubMachineType, 4> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER, // receiver
        StubMachineType::TAGGED_POINTER, // holder
        StubMachineType::TAGGED, // accessor
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CallInternalGetter)
{
    // 3 : 3 input parameters
    StubDescriptor accessorGetter("CallInternalGetter", 0, 3, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = accessorGetter;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED, // accessor
        StubMachineType::TAGGED_POINTER, // receiver
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowTypeError)
{
    // 2 : 2 input parameters
    StubDescriptor throwTypeError("ThrowTypeError", 0, 2, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::BOOL);
    *descriptor = throwTypeError;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::UINT32, // messageStringId
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(JSProxySetProperty)
{
    // 6 : 6 input parameters
    StubDescriptor jsproxySetproperty("JSProxySetProperty", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::BOOL);
    *descriptor = jsproxySetproperty;
    // 6 : 6 input parameters
    std::array<StubMachineType, 6> params = {
        StubMachineType::NATIVE_POINTER, StubMachineType::TAGGED_POINTER, StubMachineType::UINT64,
        StubMachineType::TAGGED, StubMachineType::TAGGED_POINTER, StubMachineType::BOOL,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetHash32)
{
    // 2 : 2 input parameters
    StubDescriptor getHash32("GetHash32", 0, 2, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT32);
    *descriptor = getHash32;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(FindElementWithCache)
{
    // 4 : 4 input parameters
    StubDescriptor findElementWithCache("FindElementWithCache", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::INT32);
    *descriptor = findElementWithCache;
    std::array<StubMachineType, 4> params = {  // 4 : 4 input parameters
        StubMachineType::NATIVE_POINTER, // thread
        StubMachineType::TAGGED_POINTER, // hclass
        StubMachineType::TAGGED,         // key
        StubMachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(Execute)
{
    // 5 : 5 input parameters
    StubDescriptor execute("Execute", 0, 5, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT64);
    *descriptor = execute;
    std::array<StubMachineType, 5> params = {  // 5 : 5 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::UINT32,
        StubMachineType::NATIVE_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StringGetHashCode)
{
    StubDescriptor stringGetHashCode("StringGetHashCode", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT32);
    *descriptor = stringGetHashCode;
    std::array<StubMachineType, 1> params = {
        StubMachineType::TAGGED_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB_NO_GC);
}

CALL_STUB_INIT_DESCRIPTOR(NewInternalString)
{
    // 2 : 2 input parameters
    StubDescriptor stringGetHashCode("NewInternalString", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED_POINTER);
    *descriptor = stringGetHashCode;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NewEcmaDynClass)
{
    // 4 : 4 input parameters
    StubDescriptor newEcmaDynClass("NewEcmaDynClass", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED_POINTER);
    *descriptor = newEcmaDynClass;
    // 4 : 4 input parameters
    std::array<StubMachineType, 4> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::UINT32,
        StubMachineType::UINT32,
        StubMachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(UpdateLayOutAndAddTransition)
{
    // 5 : 5 input parameters
    StubDescriptor updateLayOutAndAddTransition("UpdateLayOutAndAddTransition", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = updateLayOutAndAddTransition;
    // 5 : 5 input parameters
    std::array<StubMachineType, 5> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(PropertiesSetValue)
{
    // 6 : 6 input parameters
    StubDescriptor propertiesSetValue("PropertiesSetValue", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = propertiesSetValue;
    // 6 : 6 input parameters
    std::array<StubMachineType, 6> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::UINT32,
        StubMachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(TaggedArraySetValue)
{
    // 6 : 6 input parameters
    StubDescriptor taggedArraySetValue("TaggedArraySetValue", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT64); // undefined or hole
    *descriptor = taggedArraySetValue;
    // 6 : 6 input parameters
    std::array<StubMachineType, 6> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::UINT32,
        StubMachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NewTaggedArray)
{
    // 2 : 2 input parameters
    StubDescriptor newTaggedArray("NewTaggedArray", 0, 2, ArgumentsOrder::DEFAULT_ORDER,
                                  StubMachineType::TAGGED_POINTER);
    *descriptor = newTaggedArray;
    std::array<StubMachineType, 2> params = { // 2 : 2 input parameters
        StubMachineType::NATIVE_POINTER, StubMachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CopyArray)
{
    // 4 : 4 input parameters
    StubDescriptor copyArray("CopyArray", 0, 4, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED_POINTER);
    *descriptor = copyArray;
    std::array<StubMachineType, 4> params = { // 4 : 4 input parameters
        StubMachineType::NATIVE_POINTER, StubMachineType::TAGGED_POINTER, StubMachineType::UINT32,
        StubMachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NameDictPutIfAbsent)
{
    // 7 : 7 input parameters
    StubDescriptor nameDictPutIfAbsent("NameDictPutIfAbsent", 0, 7, ArgumentsOrder::DEFAULT_ORDER,
        StubMachineType::TAGGED_POINTER);
    *descriptor = nameDictPutIfAbsent;
    std::array<StubMachineType, 7> params = { // 7 : 7 input parameters
        StubMachineType::NATIVE_POINTER, StubMachineType::TAGGED_POINTER, StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED, StubMachineType::TAGGED, StubMachineType::UINT32, StubMachineType::BOOL,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NoticeThroughChainAndRefreshUser)
{
    // 3 : 3 input parameters
    StubDescriptor noticeIC("NoticeThroughChainAndRefreshUser", 0, 3, ArgumentsOrder::DEFAULT_ORDER,
                            StubMachineType::NONE);
    *descriptor = noticeIC;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(BytecodeHandler)
{
    // 7 : 7 input parameters
    StubDescriptor bytecodeHandler("bytecodeHandler", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = bytecodeHandler;
    std::array<StubMachineType, 7> params = { // 7 : 7 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::NATIVE_POINTER,
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::INT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::BYTECODE_HANDLER);
}

CALL_STUB_INIT_DESCRIPTOR(SingleStepDebugging)
{
    // 7 : 7 input parameters
    StubDescriptor bytecodeHandler("singleStepDebugging", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = bytecodeHandler;
    std::array<StubMachineType, 7> params = { // 7 : 7 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::NATIVE_POINTER,
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::INT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::BYTECODE_HANDLER);
}

CALL_STUB_INIT_DESCRIPTOR(AsmInterpreterEntry)
{
    // 7 : 7 input parameters
    StubDescriptor asmInterpreterEntry("AsmInterpreterEntry", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = asmInterpreterEntry;
    std::array<StubMachineType, 7> params = { // 7 : 7 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::NATIVE_POINTER,
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::INT32,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(JumpToCInterpreter)
{
    // 7 : 7 input parameters
    StubDescriptor bytecodeHandler("jumpToCInterpreter", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NATIVE_POINTER);
    *descriptor = bytecodeHandler;
    std::array<StubMachineType, 7> params = { // 7 : 7 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::NATIVE_POINTER,
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::INT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(DebugPrint)
{
    // 1 : 1 input parameters
    StubDescriptor debugPrint("DebugPrint", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = debugPrint;
    // 1 : 1 input parameters
    std::array<StubMachineType, 1> params = {
        StubMachineType::INT32,
    };
    descriptor->SetVariableArgs(true);
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(IncDyn)
{
    // 2 : 2 input parameters
    StubDescriptor incDyn("IncDyn", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = incDyn;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(DecDyn)
{
    // 2 : 2 input parameters
    StubDescriptor decDyn("DecDyn", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = decDyn;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ExpDyn)
{
    // 3 : 3 input parameters
    StubDescriptor expDyn("ExpDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = expDyn;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(IsInDyn)
{
    // 3 : 3 input parameters
    StubDescriptor isInDyn("IsInDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = isInDyn;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(InstanceOfDyn)
{
    // 3 : 3 input parameters
    StubDescriptor instanceOfDyn("InstanceOfDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = instanceOfDyn;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(FastStrictNotEqual)
{
    // 2 : 2 input parameters
    StubDescriptor fastStrictNotEqual("FastStrictNotEqual", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = fastStrictNotEqual;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(FastStrictEqual)
{
    // 2 : 2 input parameters
    StubDescriptor fastStrictEqual("FastStrictEqual", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = fastStrictEqual;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CreateGeneratorObj)
{
    // 2 : 2 input parameters
    StubDescriptor createGeneratorObj("CreateGeneratorObj", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = createGeneratorObj;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowConstAssignment)
{
    // 2 : 2 input parameters
    StubDescriptor throwConstAssignment("ThrowConstAssignment", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = throwConstAssignment;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetTemplateObject)
{
    // 2 : 2 input parameters
    StubDescriptor getTemplateObject("GetTemplateObject", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = getTemplateObject;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetNextPropName)
{
    // 2 : 2 input parameters
    StubDescriptor getNextPropName("GetNextPropName", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = getNextPropName;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowIfNotObject)
{
    // 1 : 1 input parameter
    StubDescriptor throwIfNotObject("ThrowIfNotObject", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = throwIfNotObject;
    // 1 : 1 input parameter
    std::array<StubMachineType, 1> params = {
        StubMachineType::NATIVE_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(JSArrayListSetByIndex)
{
    // 4 : 4 input parameters
    StubDescriptor arraylistSetByIndex("JSArrayListSetByIndex", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
                                       StubMachineType::NONE);
    *descriptor = arraylistSetByIndex;
    // 4 : 4 input parameters
    std::array<StubMachineType, 4> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::INT32,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(InsertOldToNewRememberedSet)
{
    // 3 : 3 input parameters
    StubDescriptor index("InsertOldToNewRememberedSet", 0, 3, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = index;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::NATIVE_POINTER,
        StubMachineType::NATIVE_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(MarkingBarrier)
{
    // 5 : 5 input parameters
    StubDescriptor index("MarkingBarrier", 0, 5, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = index;
    // 5 : 5 input parameters
    std::array<StubMachineType, 5> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::NATIVE_POINTER,
        StubMachineType::NATIVE_POINTER,
        StubMachineType::NATIVE_POINTER,
        StubMachineType::NATIVE_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(IterNext)
{
    // 2 : 2 input parameters
    StubDescriptor iterNext("IterNext", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = iterNext;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CallRuntimeTrampoline)
{
    /* 4 : 4 input parameters */
    StubDescriptor callRuntimeTrampoline("CallRuntimeTrampoline", 0, 4, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = callRuntimeTrampoline;
    std::array<StubMachineType, 4> params = { /* 4 : 4 input parameters */
        StubMachineType::NATIVE_POINTER,
        StubMachineType::UINT64,
        StubMachineType::UINT64,
        StubMachineType::UINT32,
    };
    descriptor->SetVariableArgs(true);
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CloseIterator)
{
    // 2 : 2 input parameters
    StubDescriptor closeIterator("CloseIterator", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = closeIterator;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CopyModule)
{
    // 2 : 2 input parameters
    StubDescriptor copyModule("CopyModule", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = copyModule;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(SuperCallSpread)
{
    // 4 : 4 input parameters
    StubDescriptor superCallSpread("SuperCallSpread", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = superCallSpread;
    // 4 : 4 input parameters
    std::array<StubMachineType, 4> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(DelObjProp)
{
    // 3 : 3 input parameters
    StubDescriptor delObjProp("DelObjProp", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = delObjProp;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NewObjSpreadDyn)
{
    // 4 : 4 input parameters
    StubDescriptor newObjSpreadDyn("NewObjSpreadDyn", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = newObjSpreadDyn;
    // 4 : 4 input parameters
    std::array<StubMachineType, 4> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CreateIterResultObj)
{
    // 3 : 3 input parameters
    StubDescriptor createIterResultObj("CreateIterResultObj", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = createIterResultObj;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(AsyncFunctionAwaitUncaught)
{
    // 3 : 3 input parameters
    StubDescriptor asyncFunctionAwaitUncaught("AsyncFunctionAwaitUncaught", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = asyncFunctionAwaitUncaught;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowUndefinedIfHole)
{
    // 2 : 2 input parameters
    StubDescriptor throwUndefinedIfHole("ThrowUndefinedIfHole", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = throwUndefinedIfHole;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CopyDataProperties)
{
    // 3 : 3 input parameters
    StubDescriptor copyDataProperties("CopyDataProperties", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = copyDataProperties;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StArraySpread)
{
    // 4 : 4 input parameters
    StubDescriptor stArraySpread("StArraySpread", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = stArraySpread;
    // 4 : 4 input parameters
    std::array<StubMachineType, 4> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetIteratorNext)
{
    // 3 : 3 input parameters
    StubDescriptor getIteratorNext("GetIteratorNext", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = getIteratorNext;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(SetObjectWithProto)
{
    // 3 : 3 input parameters
    StubDescriptor setObjectWithProto("SetObjectWithProto", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = setObjectWithProto;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LoadICByValue)
{
    // 5 : 5 input parameters
    StubDescriptor loadICByValue("LoadICByValue", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = loadICByValue;
    // 5 : 5 input parameters
    std::array<StubMachineType, 5> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::INT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StoreICByValue)
{
    // 6 : 6 input parameters
    StubDescriptor storeICByValue("StoreICByValue", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = storeICByValue;
    // 6 : 6 input parameters
    std::array<StubMachineType, 6> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::INT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StOwnByValue)
{
    // 4 : 4 input parameters
    StubDescriptor stOwnByValue("StOwnByValue", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = stOwnByValue;
    // 4 : 4 input parameters
    std::array<StubMachineType, 4> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LdSuperByValue)
{
    // 4 : 4 input parameters
    StubDescriptor ldSuperByValue("LdSuperByValue", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = ldSuperByValue;
    // 4 : 4 input parameters
    std::array<StubMachineType, 4> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::NATIVE_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StSuperByValue)
{
    // 5 : 5 input parameters
    StubDescriptor stSuperByValue("StSuperByValue", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = stSuperByValue;
    // 5 : 5 input parameters
    std::array<StubMachineType, 5> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::NATIVE_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LdObjByIndex)
{
    // 5 : 5 input parameters
    StubDescriptor ldObjByIndex("LdObjByIndex", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = ldObjByIndex;
    // 5 : 5 input parameters
    std::array<StubMachineType, 5> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::UINT32,
        StubMachineType::BOOL,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StObjByIndex)
{
    // 4 : 4 input parameters
    StubDescriptor stObjByIndex("StObjByIndex", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = stObjByIndex;
    // 4 : 4 input parameters
    std::array<StubMachineType, 4> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::UINT32,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StOwnByIndex)
{
    // 4 : 4 input parameters
    StubDescriptor stOwnByIndex("StOwnByIndex", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = stOwnByIndex;
    // 4 : 4 input parameters
    std::array<StubMachineType, 4> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::UINT32,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StGlobalRecord)
{
    // 4 : 4 input parameters
    StubDescriptor stGlobalRecord("StGlobalRecord", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = stGlobalRecord;
    std::array<StubMachineType, 4> params = { // 4 : 4 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::BOOL,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NegDyn)
{
    // 2 : 2 input parameters
    StubDescriptor NegDyn("NegDyn", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = NegDyn;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NotDyn)
{
    // 2 : 2 input parameters
    StubDescriptor NotDyn("NotDyn", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = NotDyn;
    // 2 : 2 input parameters
    std::array<StubMachineType, 2> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ChangeTwoInt32AndToJSTaggedValue)
{
    // 3 : 3 input parameters
    StubDescriptor ChangeTwoInt32AndToJSTaggedValue("ChangeTwoInt32AndToJSTaggedValue", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = ChangeTwoInt32AndToJSTaggedValue;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ChangeTwoInt32XorToJSTaggedValue)
{
    // 3 : 3 input parameters
    StubDescriptor ChangeTwoInt32XorToJSTaggedValue("ChangeTwoInt32XorToJSTaggedValue", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = ChangeTwoInt32XorToJSTaggedValue;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ChangeTwoInt32OrToJSTaggedValue)
{
    // 3 : 3 input parameters
    StubDescriptor ChangeTwoInt32OrToJSTaggedValue("ChangeTwoInt32OrToJSTaggedValue", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = ChangeTwoInt32OrToJSTaggedValue;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ChangeTwoUint32AndToJSTaggedValue)
{
    // 3 : 3 input parameters
    StubDescriptor ChangeTwoUint32AndToJSTaggedValue("ChangeTwoUint32AndToJSTaggedValue", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = ChangeTwoUint32AndToJSTaggedValue;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ChangeUintAndIntShrToJSTaggedValue)
{
    // 3 : 3 input parameters
    StubDescriptor ChangeUintAndIntShrToJSTaggedValue("ChangeUintAndIntShrToJSTaggedValue", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = ChangeUintAndIntShrToJSTaggedValue;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ChangeUintAndIntShlToJSTaggedValue)
{
    // 3 : 3 input parameters
    StubDescriptor ChangeUintAndIntShlToJSTaggedValue("ChangeUintAndIntShlToJSTaggedValue", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = ChangeUintAndIntShlToJSTaggedValue;
    // 3 : 3 input parameters
    std::array<StubMachineType, 3> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ResolveClass)
{
    // 6 : 6 input parameters
    StubDescriptor resolveClass("ResolveClass", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = resolveClass;
    std::array<StubMachineType, 6> params = { // 6 : 6 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CloneClassFromTemplate)
{
    // 5 : 5 input parameters
    StubDescriptor cloneClassFromTemplate("CloneClassFromTemplate", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = cloneClassFromTemplate;
    std::array<StubMachineType, 5> params = { // 5 : 5 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(SetClassConstructorLength)
{
    // 3 : 3 input parameters
    StubDescriptor setClassConstructorLength("SetClassConstructorLength", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = setClassConstructorLength;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::UINT16,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(UpdateHotnessCounter)
{
    // 2 : 2 input parameters
    StubDescriptor updateHotnessCounter("UpdateHotnessCounter", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = updateHotnessCounter;
    std::array<StubMachineType, 2> params = { // 2 : 2 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::NATIVE_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ImportModule)
{
    // 2 : 2 input parameters
    StubDescriptor importModule("ImportModule", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = importModule;
    std::array<StubMachineType, 2> params = { // 2 : 2 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StModuleVar)
{
    // 3 : 3 input parameters
    StubDescriptor stModuleVar("StModuleVar", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = stModuleVar;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LdModvarByName)
{
    // 3 : 3 input parameters
    StubDescriptor ldModvarByName("LdModvarByName", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = ldModvarByName;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowDyn)
{
    // 2 : 2 input parameters
    StubDescriptor throwDyn("ThrowDyn", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = throwDyn;
    std::array<StubMachineType, 2> params = { // 2 : 2 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetPropIterator)
{
    // 2 : 2 input parameters
    StubDescriptor getPropIterator("GetPropIterator", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = getPropIterator;
    std::array<StubMachineType, 2> params = { // 2 : 2 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LdGlobalRecord)
{
    // 2 : 2 input parameters
    StubDescriptor ldGlobalRecord("LdGlobalRecord", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = ldGlobalRecord;
    std::array<StubMachineType, 2> params = { // 2 : 2 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(AsyncFunctionEnter)
{
    // 1 : 1 input parameters
    StubDescriptor asyncFunctionEnter("AsyncFunctionEnter", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = asyncFunctionEnter;
    std::array<StubMachineType, 1> params = { // 1 : 1 input parameters
        StubMachineType::NATIVE_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetGlobalOwnProperty)
{
    // 2 : 2 input parameters
    StubDescriptor getGlobalOwnProperty("GetGlobalOwnProperty", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = getGlobalOwnProperty;
    std::array<StubMachineType, 2> params = { // 2 : 2 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetIterator)
{
    // 2 : 2 input parameters
    StubDescriptor getIterator("GetIterator", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = getIterator;
    std::array<StubMachineType, 2> params = { // 2 : 2 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowThrowNotExists)
{
    // 1 : 1 input parameters
    StubDescriptor throwThrowNotExists("ThrowThrowNotExists", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = throwThrowNotExists;
    std::array<StubMachineType, 1> params = { // 1 : 1 input parameters
        StubMachineType::NATIVE_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(TryLdGlobalByName)
{
    // 2 : 2 input parameters
    StubDescriptor tryLdGlobalByName("TryLdGlobalByName", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = tryLdGlobalByName;
    std::array<StubMachineType, 2> params = { // 2 : 2 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LoadMiss)
{
    // 6 : 6 input parameters
    StubDescriptor loadMiss("LoadMiss", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = loadMiss;
    std::array<StubMachineType, 6> params = { // 6 : 6 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::UINT32,
        StubMachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StoreMiss)
{
    // 7 : 7 input parameters
    StubDescriptor storeMiss("StoreMiss", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = storeMiss;
    std::array<StubMachineType, 7> params = { // 7 : 7 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::UINT32,
        StubMachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowPatternNonCoercible)
{
    // 1 : 1 input parameters
    StubDescriptor throwPatternNonCoercible("ThrowPatternNonCoercible", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = throwPatternNonCoercible;
    std::array<StubMachineType, 1> params = { // 1 : 1 input parameters
        StubMachineType::NATIVE_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(TryUpdateGlobalRecord)
{
    // 3 : 3 input parameters
    StubDescriptor tryUpdateGlobalRecord("TryUpdateGlobalRecord", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = tryUpdateGlobalRecord;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowDeleteSuperProperty)
{
    // 1 : 1 input parameters
    StubDescriptor throwDeleteSuperProperty("ThrowDeleteSuperProperty", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = throwDeleteSuperProperty;
    std::array<StubMachineType, 1> params = { // 1 : 1 input parameters
        StubMachineType::NATIVE_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowReferenceError)
{
    // 2 : 2 input parameters
    StubDescriptor throwReferenceError("ThrowReferenceError", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = throwReferenceError;
    std::array<StubMachineType, 2> params = { // 2 : 2 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(EqDyn)
{
    // 3 : 3 input parameters
    StubDescriptor eqDyn("EqDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = eqDyn;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LdGlobalVar)
{
    // 3 : 3 input parameters
    StubDescriptor ldGlobalVar("LdGlobalVar", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = ldGlobalVar;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StGlobalVar)
{
    // 3 : 3 input parameters
    StubDescriptor stGlobalVar("StGlobalVar", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = stGlobalVar;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ToBoolean)
{
    // 1 : 1 input parameters
    StubDescriptor toBoolean("ToBoolean", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::BOOL);
    *descriptor = toBoolean;
    std::array<StubMachineType, 1> params = { // 1 : 1 input parameters
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NotEqDyn)
{
    // 3 : 3 input parameters
    StubDescriptor notEqDyn("NotEqDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = notEqDyn;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LessDyn)
{
    // 3 : 3 input parameters
    StubDescriptor lessDyn("LessDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = lessDyn;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LessEqDyn)
{
    // 3 : 3 input parameters
    StubDescriptor lessEqDyn("LessEqDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = lessEqDyn;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GreaterDyn)
{
    // 3 : 3 input parameters
    StubDescriptor greaterDyn("GreaterDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = greaterDyn;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GreaterEqDyn)
{
    // 3 : 3 input parameters
    StubDescriptor greaterEqDyn("GreaterEqDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = greaterEqDyn;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(PhiGateTest)
{
    StubDescriptor phiGateTest("PhiGateTest", 0, 1, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT32);
    *descriptor = phiGateTest;
    std::array<StubMachineType, 1> params = {
        StubMachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::TEST_FUNC);
}

CALL_STUB_INIT_DESCRIPTOR(LoopTest)
{
    StubDescriptor loopTest("LoopTest", 0, 1, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT32);
    *descriptor = loopTest;
    std::array<StubMachineType, 1> params = {
        StubMachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::TEST_FUNC);
}

CALL_STUB_INIT_DESCRIPTOR(LoopTest1)
{
    StubDescriptor loopTest1("LoopTest1", 0, 1, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT32);
    *descriptor = loopTest1;
    std::array<StubMachineType, 1> params = {
        StubMachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::TEST_FUNC);
}

CALL_STUB_INIT_DESCRIPTOR(SetPropertyByValue)
{
    // 4 : 4 input parameters
    StubDescriptor setPropertyByValue("SetPropertyByValue", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT64);
    *descriptor = setPropertyByValue;
    // 4 : 4 input parameters
    std::array<StubMachineType, 4> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(SetFunctionNameNoPrefix)
{
    // 3 : 3 input parameters
    StubDescriptor setFunctionNameNoPrefix("SetFunctionNameNoPrefix", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = setFunctionNameNoPrefix;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StOwnByName)
{
    // 4 : 4 input parameters
    StubDescriptor stOwnByName("StOwnByName", 0, 4, ArgumentsOrder::DEFAULT_ORDER, StubMachineType::UINT64);
    *descriptor = stOwnByName;
    std::array<StubMachineType, 4> params = { // 4 : 4 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StOwnByNameWithNameSet)
{
    // 4 : 4 input parameters
    StubDescriptor stOwnByNameWithNameSet("StOwnByNameWithNameSet", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = stOwnByNameWithNameSet;
    std::array<StubMachineType, 4> params = { // 4 : 4 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StOwnByValueWithNameSet)
{
    // 4 : 4 input parameters
    StubDescriptor stOwnByValueWithNameSet("StOwnByValueWithNameSet", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = stOwnByValueWithNameSet;
    std::array<StubMachineType, 4> params = { // 4 : 4 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(SuspendGenerator)
{
    // 3 : 3 input parameters
    StubDescriptor suspendGenerator("SuspendGenerator", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = suspendGenerator;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(UpFrame)
{
    // 2 : 2 input parameters
    StubDescriptor upFrame("UpFrame", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NATIVE_POINTER);
    *descriptor = upFrame;
    std::array<StubMachineType, 2> params = { // 2 : 2 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::NATIVE_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ToNumber)
{
    // 2 : 2 input parameters
    StubDescriptor toNumber("ToNumber", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = toNumber;
    std::array<StubMachineType, 2> params = { // 2 : 2 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(Add2Dyn)
{
    // 3 : 3 input parameters
    StubDescriptor add2Dyn("Add2Dyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = add2Dyn;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(Sub2Dyn)
{
    // 3 : 3 input parameters
    StubDescriptor sub2Dyn("Sub2Dyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = sub2Dyn;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(Mul2Dyn)
{
    // 3 : 3 input parameters
    StubDescriptor mul2Dyn("Mul2Dyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = mul2Dyn;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(Div2Dyn)
{
    // 3 : 3 input parameters
    StubDescriptor div2Dyn("Div2Dyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = div2Dyn;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(Mod2Dyn)
{
    // 3 : 3 input parameters
    StubDescriptor mod2Dyn("Mod2Dyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::TAGGED);
    *descriptor = mod2Dyn;
    std::array<StubMachineType, 3> params = { // 3 : 3 input parameters
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

void FastStubDescriptors::InitializeStubDescriptors()
{
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEF_CALL_STUB(name) NAME_##name
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INITIALIZE_CALL_STUB_DESCRIPTOR(name, argcounts) \
    Stub##name##InterfaceDescriptor::Initialize(&callStubsDescriptor_[DEF_CALL_STUB(name)]);
    CALL_STUB_LIST(INITIALIZE_CALL_STUB_DESCRIPTOR)
#ifndef NDEBUG
    TEST_FUNC_LIST(INITIALIZE_CALL_STUB_DESCRIPTOR)
#endif
#undef INITIALIZE_CALL_STUB_DESCRIPTOR
#undef DEF_CALL_STUB
}
}  // namespace panda::ecmascript::kungfu
