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
    StubDescriptor fastAdd("FastAdd", 0, 2, ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED); // number or hole
    *descriptor = fastAdd;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FastSub)
{
    // 2 : 2 input parameters
    StubDescriptor fastSub("FastSub", 0, 2, ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED); // number or hole
    *descriptor = fastSub;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FastMul)
{
    // 2 : 2 input parameters
    StubDescriptor fastMul("FastMul", 0, 2, ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED); // number or hole
    *descriptor = fastMul;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FastDiv)
{
    // 2 : 2 input parameters
    StubDescriptor fastDiv("FastDiv", 0, 2, ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED); // float or hole
    *descriptor = fastDiv;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FastMod)
{
    // 3 : 3 input parameters
    StubDescriptor fastMod("FastMod", 0, 3, ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED); // int,float or hole
    *descriptor = fastMod;
    // 3 : 3 input parameters
    std::array<MachineType, 3> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FastTypeOf)
{
    // 2 input parameters
    StubDescriptor fastTypeOf("FastTypeOf", 0, 2, ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED_POINTER);
    *descriptor = fastTypeOf;
    // 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::NATIVE_POINTER, // glue
        MachineType::TAGGED, // ACC
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FastEqual)
{
    // 2 input parameters, return may be true/false/hole
    StubDescriptor fastEqual("FastEqual", 0, 2, ArgumentsOrder::DEFAULT_ORDER, MachineType::UINT64);
    *descriptor = fastEqual;
    // 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(SetPropertyByName)
{
    // 4 : 4 input parameters
    StubDescriptor setPropertyByName("SetPropertyByName", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        MachineType::UINT64);
    *descriptor = setPropertyByName;

    std::array<MachineType, 4> params = { // 4 : 4 input parameters
        MachineType::NATIVE_POINTER, MachineType::TAGGED_POINTER, MachineType::TAGGED_POINTER,
        MachineType::TAGGED_POINTER
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(SetPropertyByNameWithOwn)
{
    // 4 : 4 input parameters
    StubDescriptor setPropertyByNameWithOwn("SetPropertyByNameWithOwn", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        MachineType::UINT64);
    *descriptor = setPropertyByNameWithOwn;

    std::array<MachineType, 4> params = { // 4 : 4 input parameters
        MachineType::NATIVE_POINTER, MachineType::TAGGED_POINTER, MachineType::TAGGED_POINTER,
        MachineType::TAGGED_POINTER
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(GetPropertyByName)
{
        // 3 : 3 input parameters
    StubDescriptor getPropertyByName("GetPropertyByName", 0, 3, ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = getPropertyByName;
    // 3 : 3 input parameters
    std::array<MachineType, 3> params = {
        MachineType::NATIVE_POINTER, // glue
        MachineType::TAGGED,         // receiver
        MachineType::TAGGED_POINTER, // key
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(GetPropertyByIndex)
{
    // 3 : 3 input parameters
    StubDescriptor getPropertyByIndex("GetPropertyByIndex", 0, 3, ArgumentsOrder::DEFAULT_ORDER,
        MachineType::TAGGED);
    *descriptor = getPropertyByIndex;
    // 3 : 3 input parameters
    std::array<MachineType, 3> params = {
        MachineType::NATIVE_POINTER, // glue
        MachineType::TAGGED, // receiver
        MachineType::UINT32, // index
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(SetPropertyByIndex)
{
    // 4 : 4 input parameters
    StubDescriptor setPropertyByIndex("SetPropertyByIndex", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        MachineType::UINT64); // hole or undefined
    *descriptor = setPropertyByIndex;
    // 4 : 4 input parameters
    std::array<MachineType, 4> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::UINT32,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(GetPropertyByValue)
{
    // 3 : 3 input parameters
    StubDescriptor getPropertyByValue("GetPropertyByValue", 0, 3, ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = getPropertyByValue;
    // 3 : 3 input parameters
    std::array<MachineType, 3> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(JSHClassAddProperty)
{
    // 4 : 4 input parameters
    StubDescriptor jsHClassAddProperty("JSHClassAddProperty", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::NONE);
    *descriptor = jsHClassAddProperty;
    // 4 : 4 input parameters
    std::array<MachineType, 4> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(LoadICByName)
{
    // 5 : 5 input parameters
    StubDescriptor loadICByName("LoadICByName", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = loadICByName;
    // 5 : 5 input parameters
    std::array<MachineType, 5> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
        MachineType::TAGGED,
        MachineType::INT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(TryLoadICByName)
{
    // 4 : 4 input parameters
    StubDescriptor tryLoadICByName("TryLoadICByName", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = tryLoadICByName;
    // 4 : 4 input parameters
    std::array<MachineType, 4> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(TryLoadICByValue)
{
    // 5 : 5 input parameters
    StubDescriptor tryLoadICByValue("TryLoadICByValue", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = tryLoadICByValue;
    // 5 : 5 input parameters
    std::array<MachineType, 5> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(TryStoreICByName)
{
    // 5 : 5 input parameters
    StubDescriptor tryStoreICByName("TryStoreICByName", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::UINT64); // undefined or hole
    *descriptor = tryStoreICByName;
    // 5 : 5 input parameters
    std::array<MachineType, 5> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
        MachineType::TAGGED_POINTER,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(TryStoreICByValue)
{
    // 6 : 6 input parameters
    StubDescriptor tryStoreICByValue("TryStoreICByValue", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::UINT64); // undefined or hole
    *descriptor = tryStoreICByValue;
    // 6 : 6 input parameters
    std::array<MachineType, 6> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
        MachineType::TAGGED,
        MachineType::TAGGED_POINTER,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FloatMod)
{
    // 2 : 2 input parameters
    StubDescriptor floatMod("FloatMod", 0, 2, ArgumentsOrder::DEFAULT_ORDER, MachineType::FLOAT64);
    *descriptor = floatMod;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::FLOAT64,
        MachineType::FLOAT64,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(AddElementInternal)
{
    // 5 : 5 input parameters
    StubDescriptor addElementInternal("AddElementInternal", 0, 5, ArgumentsOrder::DEFAULT_ORDER,
        MachineType::BOOL);
    *descriptor = addElementInternal;
    // 5 : 5 input parameters
    std::array<MachineType, 5> params = {
        MachineType::NATIVE_POINTER, MachineType::TAGGED_POINTER, MachineType::UINT32,
        MachineType::TAGGED, MachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetTaggedArrayPtrTest)
{
    StubDescriptor getTaggedArrayPtr("GetTaggedArrayPtrTest", 0, 1, ArgumentsOrder::DEFAULT_ORDER,
                                     MachineType::TAGGED_POINTER);
    *descriptor = getTaggedArrayPtr;
    std::array<MachineType, 1> params = {
        MachineType::NATIVE_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CallSetter)
{
    // 5 : 5 input parameters
    StubDescriptor callSetter("CallSetter", 0, 5, ArgumentsOrder::DEFAULT_ORDER, MachineType::BOOL);
    *descriptor = callSetter;
    // 5 : 5 input parameters
    std::array<MachineType, 5> params = {
        MachineType::NATIVE_POINTER, MachineType::TAGGED, MachineType::TAGGED_POINTER,
        MachineType::TAGGED, MachineType::BOOL,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CallSetter2)
{
    // 4 : 4 input parameters, return Undefined or Exception
    StubDescriptor callSetter("CallSetter2", 0, 4, ArgumentsOrder::DEFAULT_ORDER, MachineType::UINT64);
    *descriptor = callSetter;
    // 4 : 4 input parameters
    std::array<MachineType, 4> params = {
        MachineType::NATIVE_POINTER, MachineType::TAGGED, MachineType::TAGGED_POINTER,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CallGetter)
{
    // 3 : 3 input parameters
    StubDescriptor callGetter("CallGetter", 0, 3, ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = callGetter;
    // 3 : 3 input parameters
    std::array<MachineType, 3> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED, // getter
        MachineType::TAGGED_POINTER, // receiver
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CallGetter2)
{
    // 4 : 4 input parameters
    StubDescriptor callGetter("CallGetter2", 0, 4, ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = callGetter;
    // 4 : 4 input parameters
    std::array<MachineType, 4> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED_POINTER, // receiver
        MachineType::TAGGED_POINTER, // holder
        MachineType::TAGGED, // accessor
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CallInternalGetter)
{
    // 3 : 3 input parameters
    StubDescriptor accessorGetter("CallInternalGetter", 0, 3, ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = accessorGetter;
    // 3 : 3 input parameters
    std::array<MachineType, 3> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED, // accessor
        MachineType::TAGGED_POINTER, // receiver
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowTypeError)
{
    // 2 : 2 input parameters
    StubDescriptor throwTypeError("ThrowTypeError", 0, 2, ArgumentsOrder::DEFAULT_ORDER, MachineType::BOOL);
    *descriptor = throwTypeError;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::NATIVE_POINTER,
        MachineType::UINT32, // messageStringId
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(JSProxySetProperty)
{
    // 6 : 6 input parameters
    StubDescriptor jsproxySetproperty("JSProxySetProperty", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::BOOL);
    *descriptor = jsproxySetproperty;
    // 6 : 6 input parameters
    std::array<MachineType, 6> params = {
        MachineType::NATIVE_POINTER, MachineType::TAGGED_POINTER, MachineType::UINT64,
        MachineType::TAGGED, MachineType::TAGGED_POINTER, MachineType::BOOL,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetHash32)
{
    // 2 : 2 input parameters
    StubDescriptor getHash32("GetHash32", 0, 2, ArgumentsOrder::DEFAULT_ORDER, MachineType::UINT32);
    *descriptor = getHash32;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::NATIVE_POINTER,
        MachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(FindElementWithCache)
{
    // 4 : 4 input parameters
    StubDescriptor findElementWithCache("FindElementWithCache", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::INT32);
    *descriptor = findElementWithCache;
    std::array<MachineType, 4> params = {  // 4 : 4 input parameters
        MachineType::NATIVE_POINTER, // thread
        MachineType::TAGGED_POINTER, // hclass
        MachineType::TAGGED,         // key
        MachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(Execute)
{
    // 5 : 5 input parameters
    StubDescriptor execute("Execute", 0, 5, ArgumentsOrder::DEFAULT_ORDER, MachineType::UINT64);
    *descriptor = execute;
    std::array<MachineType, 5> params = {  // 5 : 5 input parameters
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED,
        MachineType::UINT32,
        MachineType::NATIVE_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StringGetHashCode)
{
    StubDescriptor stringGetHashCode("StringGetHashCode", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::UINT32);
    *descriptor = stringGetHashCode;
    std::array<MachineType, 1> params = {
        MachineType::TAGGED_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(SetValueWithBarrier)
{
    // 4 : 4 input parameters
    static StubDescriptor SetValueWithBarrier("SetValueWithBarrier", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::NONE);
    *descriptor = SetValueWithBarrier;
    // 4 : 4 input parameters
    std::array<MachineType, 4> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NewInternalString)
{
    // 2 : 2 input parameters
    StubDescriptor stringGetHashCode("NewInternalString", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED_POINTER);
    *descriptor = stringGetHashCode;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NewEcmaDynClass)
{
    // 4 : 4 input parameters
    StubDescriptor newEcmaDynClass("NewEcmaDynClass", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED_POINTER);
    *descriptor = newEcmaDynClass;
    // 4 : 4 input parameters
    std::array<MachineType, 4> params = {
        MachineType::NATIVE_POINTER,
        MachineType::UINT32,
        MachineType::UINT32,
        MachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(UpdateLayOutAndAddTransition)
{
    // 5 : 5 input parameters
    StubDescriptor updateLayOutAndAddTransition("UpdateLayOutAndAddTransition", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::NONE);
    *descriptor = updateLayOutAndAddTransition;
    // 5 : 5 input parameters
    std::array<MachineType, 5> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(PropertiesSetValue)
{
    // 6 : 6 input parameters
    StubDescriptor propertiesSetValue("PropertiesSetValue", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::NONE);
    *descriptor = propertiesSetValue;
    // 6 : 6 input parameters
    std::array<MachineType, 6> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED_POINTER,
        MachineType::UINT32,
        MachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(TaggedArraySetValue)
{
    // 6 : 6 input parameters
    StubDescriptor taggedArraySetValue("TaggedArraySetValue", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::UINT64); // undefined or hole
    *descriptor = taggedArraySetValue;
    // 6 : 6 input parameters
    std::array<MachineType, 6> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED_POINTER,
        MachineType::UINT32,
        MachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NewTaggedArray)
{
    // 2 : 2 input parameters
    StubDescriptor newTaggedArray("NewTaggedArray", 0, 2, ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED_POINTER);
    *descriptor = newTaggedArray;
    std::array<MachineType, 2> params = { // 2 : 2 input parameters
        MachineType::NATIVE_POINTER, MachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CopyArray)
{
    // 4 : 4 input parameters
    StubDescriptor copyArray("CopyArray", 0, 4, ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED_POINTER);
    *descriptor = copyArray;
    std::array<MachineType, 4> params = { // 4 : 4 input parameters
        MachineType::NATIVE_POINTER, MachineType::TAGGED_POINTER, MachineType::UINT32, MachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NameDictPutIfAbsent)
{
    // 7 : 7 input parameters
    StubDescriptor nameDictPutIfAbsent("NameDictPutIfAbsent", 0, 7, ArgumentsOrder::DEFAULT_ORDER,
        MachineType::TAGGED_POINTER);
    *descriptor = nameDictPutIfAbsent;
    std::array<MachineType, 7> params = { // 7 : 7 input parameters
        MachineType::NATIVE_POINTER, MachineType::TAGGED_POINTER, MachineType::TAGGED_POINTER, MachineType::TAGGED,
        MachineType::TAGGED, MachineType::UINT32, MachineType::BOOL,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NoticeThroughChainAndRefreshUser)
{
    // 3 : 3 input parameters
    StubDescriptor noticeIC("NoticeThroughChainAndRefreshUser", 0, 3, ArgumentsOrder::DEFAULT_ORDER, MachineType::NONE);
    *descriptor = noticeIC;
    // 3 : 3 input parameters
    std::array<MachineType, 3> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(BytecodeHandler)
{
    // 7 : 7 input parameters
    StubDescriptor bytecodeHandler("bytecodeHandler", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::NONE);
    *descriptor = bytecodeHandler;
    std::array<MachineType, 7> params = { // 7 : 7 input parameters
        MachineType::NATIVE_POINTER,
        MachineType::NATIVE_POINTER,
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED,
        MachineType::INT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::BYTECODE_HANDLER);
}

CALL_STUB_INIT_DESCRIPTOR(SingleStepDebugging)
{
    // 7 : 7 input parameters
    StubDescriptor bytecodeHandler("singleStepDebugging", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::NONE);
    *descriptor = bytecodeHandler;
    std::array<MachineType, 7> params = { // 7 : 7 input parameters
        MachineType::NATIVE_POINTER,
        MachineType::NATIVE_POINTER,
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED,
        MachineType::INT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::BYTECODE_HANDLER);
}

CALL_STUB_INIT_DESCRIPTOR(AsmInterpreterEntry)
{
    // 7 : 7 input parameters
    StubDescriptor asmInterpreterEntry("AsmInterpreterEntry", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::NONE);
    *descriptor = asmInterpreterEntry;
    std::array<MachineType, 7> params = { // 7 : 7 input parameters
        MachineType::NATIVE_POINTER,
        MachineType::NATIVE_POINTER,
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED,
        MachineType::INT32,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(JumpToCInterpreter)
{
    // 7 : 7 input parameters
    StubDescriptor bytecodeHandler("jumpToCInterpreter", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::NATIVE_POINTER);
    *descriptor = bytecodeHandler;
    std::array<MachineType, 7> params = { // 7 : 7 input parameters
        MachineType::NATIVE_POINTER,
        MachineType::NATIVE_POINTER,
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED,
        MachineType::INT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(DebugPrint)
{
    // 1 : 1 input parameters
    StubDescriptor debugPrint("DebugPrint", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::NONE);
    *descriptor = debugPrint;
    // 1 : 1 input parameters
    std::array<MachineType, 1> params = {
        MachineType::INT32,
    };
    descriptor->SetVariableArgs(true);
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(IncDyn)
{
    // 2 : 2 input parameters
    StubDescriptor incDyn("IncDyn", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = incDyn;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(DecDyn)
{
    // 2 : 2 input parameters
    StubDescriptor decDyn("DecDyn", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = decDyn;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ExpDyn)
{
    // 3 : 3 input parameters
    StubDescriptor expDyn("ExpDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = expDyn;
    // 3 : 3 input parameters
    std::array<MachineType, 3> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(IsInDyn)
{
    // 3 : 3 input parameters
    StubDescriptor isInDyn("IsInDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = isInDyn;
    // 3 : 3 input parameters
    std::array<MachineType, 3> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(InstanceOfDyn)
{
    // 3 : 3 input parameters
    StubDescriptor instanceOfDyn("InstanceOfDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = instanceOfDyn;
    // 3 : 3 input parameters
    std::array<MachineType, 3> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(FastStrictNotEqual)
{
    // 2 : 2 input parameters
    StubDescriptor fastStrictNotEqual("FastStrictNotEqual", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = fastStrictNotEqual;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(FastStrictEqual)
{
    // 2 : 2 input parameters
    StubDescriptor fastStrictEqual("FastStrictEqual", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = fastStrictEqual;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CreateGeneratorObj)
{
    // 2 : 2 input parameters
    StubDescriptor createGeneratorObj("CreateGeneratorObj", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = createGeneratorObj;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowConstAssignment)
{
    // 2 : 2 input parameters
    StubDescriptor throwConstAssignment("ThrowConstAssignment", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::NONE);
    *descriptor = throwConstAssignment;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetTemplateObject)
{
    // 2 : 2 input parameters
    StubDescriptor getTemplateObject("GetTemplateObject", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = getTemplateObject;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetNextPropName)
{
    // 2 : 2 input parameters
    StubDescriptor getNextPropName("GetNextPropName", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = getNextPropName;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowIfNotObject)
{
    // 1 : 1 input parameter
    StubDescriptor throwIfNotObject("ThrowIfNotObject", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::NONE);
    *descriptor = throwIfNotObject;
    // 1 : 1 input parameter
    std::array<MachineType, 1> params = {
        MachineType::NATIVE_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(IterNext)
{
    // 2 : 2 input parameters
    StubDescriptor iterNext("IterNext", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = iterNext;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CloseIterator)
{
    // 2 : 2 input parameters
    StubDescriptor closeIterator("CloseIterator", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = closeIterator;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CopyModule)
{
    // 2 : 2 input parameters
    StubDescriptor copyModule("CopyModule", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::NONE);
    *descriptor = copyModule;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(DelObjProp)
{
    // 3 : 3 input parameters
    StubDescriptor delObjProp("DelObjProp", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = delObjProp;
    // 3 : 3 input parameters
    std::array<MachineType, 3> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NewObjSpreadDyn)
{
    // 4 : 4 input parameters
    StubDescriptor newObjSpreadDyn("NewObjSpreadDyn", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = newObjSpreadDyn;
    // 4 : 4 input parameters
    std::array<MachineType, 4> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CreateIterResultObj)
{
    // 3 : 3 input parameters
    StubDescriptor createIterResultObj("CreateIterResultObj", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = createIterResultObj;
    // 3 : 3 input parameters
    std::array<MachineType, 3> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(AsyncFunctionAwaitUncaught)
{
    // 3 : 3 input parameters
    StubDescriptor asyncFunctionAwaitUncaught("AsyncFunctionAwaitUncaught", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = asyncFunctionAwaitUncaught;
    // 3 : 3 input parameters
    std::array<MachineType, 3> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowUndefinedIfHole)
{
    // 2 : 2 input parameters
    StubDescriptor throwUndefinedIfHole("ThrowUndefinedIfHole", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::NONE);
    *descriptor = throwUndefinedIfHole;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StGlobalRecord)
{
    // 4 : 4 input parameters
    StubDescriptor stGlobalRecord("StGlobalRecord", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = stGlobalRecord;
    std::array<MachineType, 4> params = { // 4 : 4 input parameters
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
        MachineType::BOOL,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NegDyn)
{
    // 2 : 2 input parameters
    StubDescriptor NegDyn("NegDyn", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = NegDyn;
    // 2 : 2 input parameters
    std::array<MachineType, 2> params = {
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ResolveClass)
{
    // 6 : 6 input parameters
    StubDescriptor resolveClass("ResolveClass", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = resolveClass;
    std::array<MachineType, 6> params = { // 6 : 6 input parameters
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
        MachineType::TAGGED_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CloneClassFromTemplate)
{
    // 5 : 5 input parameters
    StubDescriptor cloneClassFromTemplate("CloneClassFromTemplate", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = cloneClassFromTemplate;
    std::array<MachineType, 5> params = { // 5 : 5 input parameters
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
        MachineType::TAGGED,
        MachineType::TAGGED_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(SetClassConstructorLength)
{
    // 3 : 3 input parameters
    StubDescriptor setClassConstructorLength("SetClassConstructorLength", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = setClassConstructorLength;
    std::array<MachineType, 3> params = { // 3 : 3 input parameters
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(UpdateHotnessCounter)
{
    // 2 : 2 input parameters
    StubDescriptor updateHotnessCounter("UpdateHotnessCounter", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = updateHotnessCounter;
    std::array<MachineType, 2> params = { // 2 : 2 input parameters
        MachineType::NATIVE_POINTER,
        MachineType::NATIVE_POINTER,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ImportModule)
{
    // 2 : 2 input parameters
    StubDescriptor importModule("ImportModule", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = importModule;
    std::array<MachineType, 2> params = { // 2 : 2 input parameters
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StModuleVar)
{
    // 3 : 3 input parameters
    StubDescriptor stModuleVar("StModuleVar", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::NONE);
    *descriptor = stModuleVar;
    std::array<MachineType, 3> params = { // 3 : 3 input parameters
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LdModvarByName)
{
    // 3 : 3 input parameters
    StubDescriptor ldModvarByName("LdModvarByName", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = ldModvarByName;
    std::array<MachineType, 3> params = { // 3 : 3 input parameters
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowDyn)
{
    // 2 : 2 input parameters
    StubDescriptor throwDyn("ThrowDyn", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::NONE);
    *descriptor = throwDyn;
    std::array<MachineType, 2> params = { // 2 : 2 input parameters
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

#ifndef NDEBUG
CALL_STUB_INIT_DESCRIPTOR(FastMulGCTest)
{
    // 3 : 3 input parameters
    StubDescriptor fastMulGC("FastMulGCTest", 0, 3, ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = fastMulGC;
    // 3 : 3 input parameters
    std::array<MachineType, 3> params = {
        MachineType::NATIVE_POINTER,
        MachineType::UINT64,
        MachineType::UINT64,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::TEST_FUNC);
}
CALL_STUB_INIT_DESCRIPTOR(PhiGateTest)
{
    StubDescriptor phiGateTest("PhiGateTest", 0, 1, ArgumentsOrder::DEFAULT_ORDER, MachineType::UINT32);
    *descriptor = phiGateTest;
    std::array<MachineType, 1> params = {
        MachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::TEST_FUNC);
}

CALL_STUB_INIT_DESCRIPTOR(LoopTest)
{
    StubDescriptor loopTest("LoopTest", 0, 1, ArgumentsOrder::DEFAULT_ORDER, MachineType::UINT32);
    *descriptor = loopTest;
    std::array<MachineType, 1> params = {
        MachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::TEST_FUNC);
}

CALL_STUB_INIT_DESCRIPTOR(LoopTest1)
{
    StubDescriptor loopTest1("LoopTest1", 0, 1, ArgumentsOrder::DEFAULT_ORDER, MachineType::UINT32);
    *descriptor = loopTest1;
    std::array<MachineType, 1> params = {
        MachineType::UINT32,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::TEST_FUNC);
}
#endif

CALL_STUB_INIT_DESCRIPTOR(SetPropertyByValue)
{
    // 4 : 4 input parameters
    StubDescriptor setPropertyByValue("SetPropertyByValue", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = setPropertyByValue;
    std::array<MachineType, 4> params = { // 7 : 7 input parameters
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(SetFunctionNameNoPrefix)
{
    // 3 : 3 input parameters
    StubDescriptor setFunctionNameNoPrefix("SetFunctionNameNoPrefix", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = setFunctionNameNoPrefix;
    std::array<MachineType, 3> params = { // 3 : 3 input parameters
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StOwnByNameWithNameSet)
{
    // 4 : 4 input parameters
    StubDescriptor stOwnByNameWithNameSet("StOwnByNameWithNameSet", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = stOwnByNameWithNameSet;
    std::array<MachineType, 4> params = { // 7 : 7 input parameters
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
        MachineType::TAGGED,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StOwnByValueWithNameSet)
{
    // 4 : 4 input parameters
    StubDescriptor stOwnByValueWithNameSet("StOwnByValueWithNameSet", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, MachineType::TAGGED);
    *descriptor = stOwnByValueWithNameSet;
    std::array<MachineType, 4> params = { // 4 : 4 input parameters
        MachineType::NATIVE_POINTER,
        MachineType::TAGGED,
        MachineType::TAGGED,
        MachineType::TAGGED,
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
