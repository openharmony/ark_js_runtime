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

CALL_STUB_INIT_DESCRIPTOR(SetPropertyByValue)
{
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

CALL_STUB_INIT_DESCRIPTOR(SetValueWithBarrier)
{
    // 4 : 4 input parameters
    static StubDescriptor SetValueWithBarrier("SetValueWithBarrier", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, StubMachineType::NONE);
    *descriptor = SetValueWithBarrier;
    // 4 : 4 input parameters
    std::array<StubMachineType, 4> params = {
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED_POINTER,
        StubMachineType::NATIVE_POINTER,
        StubMachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
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

void FastStubDescriptors::InitializeStubDescriptors()
{
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEF_CALL_STUB(name) NAME_##name
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INITIALIZE_CALL_STUB_DESCRIPTOR(name, argcounts) \
    Stub##name##InterfaceDescriptor::Initialize(&callStubsDescriptor_[DEF_CALL_STUB(name)]);
    CALL_STUB_LIST(INITIALIZE_CALL_STUB_DESCRIPTOR)
#undef INITIALIZE_CALL_STUB_DESCRIPTOR
#undef DEF_CALL_STUB
}
}  // namespace panda::ecmascript::kungfu
