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
    // 3 : 3 input parameters
    StubDescriptor fastAdd("FastAdd", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY()); // number or hole
    *descriptor = fastAdd;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FastSub)
{
    // 3 : 3 input parameters
    StubDescriptor fastSub("FastSub", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY()); // number or hole
    *descriptor = fastSub;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FastMul)
{
    // 3 : 3 input parameters
    StubDescriptor fastMul("FastMul", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY()); // number or hole
    *descriptor = fastMul;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
}

#ifndef NDEBUG
CALL_STUB_INIT_DESCRIPTOR(FastMulGCTest)
{
    // 3 : 3 input parameters
    StubDescriptor fastMulGC("FastMulGCTest", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = fastMulGC;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::INT64(),
        VariableType::INT64(),
    };
    descriptor->SetParameters(params.data());
}
#else
CALL_STUB_INIT_DESCRIPTOR(FastMulGCTest) {}
#endif

CALL_STUB_INIT_DESCRIPTOR(FastDiv)
{
    // 3 : 3 input parameters
    StubDescriptor fastDiv("FastDiv", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY()); // float or hole
    *descriptor = fastDiv;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FastMod)
{
    // 3 : 3 input parameters
    StubDescriptor fastMod("FastMod", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY()); // int,float or hole
    *descriptor = fastMod;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FastTypeOf)
{
    // 2 input parameters
    StubDescriptor fastTypeOf("FastTypeOf", 0, 2, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_POINTER());
    *descriptor = fastTypeOf;
    // 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::POINTER(), // glue
        VariableType::JS_ANY(), // ACC
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FastEqual)
{
    // 3 input parameters, return may be true/false/hole
    StubDescriptor fastEqual("FastEqual", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::INT64());
    *descriptor = fastEqual;
    // 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(SetPropertyByName)
{
    // 4 : 4 input parameters
    StubDescriptor setPropertyByName("SetPropertyByName", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::INT64());
    *descriptor = setPropertyByName;

    std::array<VariableType, 4> params = { // 4 : 4 input parameters
        VariableType::POINTER(), VariableType::JS_POINTER(), VariableType::JS_POINTER(),
        VariableType::JS_POINTER()
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(SetPropertyByNameWithOwn)
{
    // 4 : 4 input parameters
    StubDescriptor setPropertyByNameWithOwn("SetPropertyByNameWithOwn", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::INT64());
    *descriptor = setPropertyByNameWithOwn;

    std::array<VariableType, 4> params = { // 4 : 4 input parameters
        VariableType::POINTER(), VariableType::JS_POINTER(), VariableType::JS_POINTER(),
        VariableType::JS_POINTER()
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(GetPropertyByName)
{
        // 3 : 3 input parameters
    StubDescriptor getPropertyByName("GetPropertyByName", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = getPropertyByName;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(), // glue
        VariableType::JS_ANY(),         // receiver
        VariableType::JS_POINTER(), // key
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(GetPropertyByIndex)
{
    // 3 : 3 input parameters
    StubDescriptor getPropertyByIndex("GetPropertyByIndex", 0, 3, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_ANY());
    *descriptor = getPropertyByIndex;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(), // glue
        VariableType::JS_ANY(), // receiver
        VariableType::INT32(), // index
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(SetPropertyByIndex)
{
    // 4 : 4 input parameters
    StubDescriptor setPropertyByIndex("SetPropertyByIndex", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::INT64()); // hole or undefined
    *descriptor = setPropertyByIndex;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::INT32(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(GetPropertyByValue)
{
    // 3 : 3 input parameters
    StubDescriptor getPropertyByValue("GetPropertyByValue", 0, 3, ArgumentsOrder::DEFAULT_ORDER,
                                      VariableType::JS_ANY());
    *descriptor = getPropertyByValue;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(JSHClassAddProperty)
{
    // 4 : 4 input parameters
    StubDescriptor jsHClassAddProperty("JSHClassAddProperty", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = jsHClassAddProperty;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(LoadICByName)
{
    // 5 : 5 input parameters
    StubDescriptor loadICByName("LoadICByName", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = loadICByName;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(TryLoadICByName)
{
    // 4 : 4 input parameters
    StubDescriptor tryLoadICByName("TryLoadICByName", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = tryLoadICByName;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(TryLoadICByValue)
{
    // 5 : 5 input parameters
    StubDescriptor tryLoadICByValue("TryLoadICByValue", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = tryLoadICByValue;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(StoreICByName)
{
    // 6 : 6 input parameters
    StubDescriptor storeICByName("StoreICByName", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::INT64());
    *descriptor = storeICByName;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(TryStoreICByName)
{
    // 5 : 5 input parameters
    StubDescriptor tryStoreICByName("TryStoreICByName", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::INT64()); // undefined or hole
    *descriptor = tryStoreICByName;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_POINTER(),
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(TryStoreICByValue)
{
    // 6 : 6 input parameters
    StubDescriptor tryStoreICByValue("TryStoreICByValue", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::INT64()); // undefined or hole
    *descriptor = tryStoreICByValue;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_POINTER(),
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(TestAbsoluteAddressRelocation)
{
    // 2 : 2 input parameters
    StubDescriptor TestAbsoluteAddressRelocation("TestAbsoluteAddressRelocation", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::INT64()); // undefined or hole
    *descriptor = TestAbsoluteAddressRelocation;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::INT64(),
        VariableType::INT64(),
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(FloatMod)
{
    // 2 : 2 input parameters
    StubDescriptor floatMod("FloatMod", 0, 2, ArgumentsOrder::DEFAULT_ORDER, VariableType::FLOAT64());
    *descriptor = floatMod;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::FLOAT64(),
        VariableType::FLOAT64(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB_NO_GC);
}

CALL_STUB_INIT_DESCRIPTOR(AddElementInternal)
{
    // 5 : 5 input parameters
    StubDescriptor addElementInternal("AddElementInternal", 0, 5, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::BOOL());
    *descriptor = addElementInternal;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::POINTER(), VariableType::JS_POINTER(), VariableType::INT32(),
        VariableType::JS_ANY(), VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetTaggedArrayPtrTest)
{
    // 2 : 2 input parameters
    StubDescriptor getTaggedArrayPtr("GetTaggedArrayPtrTest", 0, 2, ArgumentsOrder::DEFAULT_ORDER,
                                     VariableType::JS_POINTER());
    *descriptor = getTaggedArrayPtr;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CallSetter)
{
    // 5 : 5 input parameters
    StubDescriptor callSetter("CallSetter", 0, 5, ArgumentsOrder::DEFAULT_ORDER, VariableType::BOOL());
    *descriptor = callSetter;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::POINTER(), VariableType::JS_ANY(), VariableType::JS_POINTER(),
        VariableType::JS_ANY(), VariableType::BOOL(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CallSetter2)
{
    // 4 : 4 input parameters, return Undefined or Exception
    StubDescriptor callSetter("CallSetter2", 0, 4, ArgumentsOrder::DEFAULT_ORDER, VariableType::INT64());
    *descriptor = callSetter;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::POINTER(), VariableType::JS_ANY(), VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CallGetter)
{
    // 3 : 3 input parameters
    StubDescriptor callGetter("CallGetter", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = callGetter;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(), // getter
        VariableType::JS_POINTER(), // receiver
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CallGetter2)
{
    // 4 : 4 input parameters
    StubDescriptor callGetter("CallGetter2", 0, 4, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = callGetter;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::POINTER(),
        VariableType::JS_POINTER(), // receiver
        VariableType::JS_POINTER(), // holder
        VariableType::JS_ANY(), // accessor
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CallInternalGetter)
{
    // 3 : 3 input parameters
    StubDescriptor accessorGetter("CallInternalGetter", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = accessorGetter;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(), // accessor
        VariableType::JS_POINTER(), // receiver
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowTypeError)
{
    // 2 : 2 input parameters
    StubDescriptor throwTypeError("ThrowTypeError", 0, 2, ArgumentsOrder::DEFAULT_ORDER, VariableType::BOOL());
    *descriptor = throwTypeError;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::POINTER(),
        VariableType::INT32(), // messageStringId
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(JSProxySetProperty)
{
    // 6 : 6 input parameters
    StubDescriptor jsproxySetproperty("JSProxySetProperty", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::BOOL());
    *descriptor = jsproxySetproperty;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::POINTER(), VariableType::JS_POINTER(), VariableType::INT64(),
        VariableType::JS_ANY(), VariableType::JS_POINTER(), VariableType::BOOL(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetHash32)
{
    // 2 : 2 input parameters
    StubDescriptor getHash32("GetHash32", 0, 2, ArgumentsOrder::DEFAULT_ORDER, VariableType::INT32());
    *descriptor = getHash32;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::POINTER(),
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(FindElementWithCache)
{
    // 4 : 4 input parameters
    StubDescriptor findElementWithCache("FindElementWithCache", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::INT32());
    *descriptor = findElementWithCache;
    std::array<VariableType, 4> params = {  // 4 : 4 input parameters
        VariableType::POINTER(), // thread
        VariableType::JS_POINTER(), // hclass
        VariableType::JS_ANY(),         // key
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(Execute)
{
    // 5 : 5 input parameters
    StubDescriptor execute("Execute", 0, 5, ArgumentsOrder::DEFAULT_ORDER, VariableType::INT64());
    *descriptor = execute;
    std::array<VariableType, 5> params = {  // 5 : 5 input parameters
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StringGetHashCode)
{
    StubDescriptor stringGetHashCode("StringGetHashCode", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::INT32());
    *descriptor = stringGetHashCode;
    std::array<VariableType, 1> params = {
        VariableType::JS_POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB_NO_GC);
}

CALL_STUB_INIT_DESCRIPTOR(NewInternalString)
{
    // 2 : 2 input parameters
    StubDescriptor stringGetHashCode("NewInternalString", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_POINTER());
    *descriptor = stringGetHashCode;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NewEcmaDynClass)
{
    // 4 : 4 input parameters
    StubDescriptor newEcmaDynClass("NewEcmaDynClass", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_POINTER());
    *descriptor = newEcmaDynClass;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(UpdateLayOutAndAddTransition)
{
    // 5 : 5 input parameters
    StubDescriptor updateLayOutAndAddTransition("UpdateLayOutAndAddTransition", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = updateLayOutAndAddTransition;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(PropertiesSetValue)
{
    // 6 : 6 input parameters
    StubDescriptor propertiesSetValue("PropertiesSetValue", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = propertiesSetValue;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(TaggedArraySetValue)
{
    // 6 : 6 input parameters
    StubDescriptor taggedArraySetValue("TaggedArraySetValue", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::INT64()); // undefined or hole
    *descriptor = taggedArraySetValue;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_POINTER(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NewTaggedArray)
{
    // 2 : 2 input parameters
    StubDescriptor newTaggedArray("NewTaggedArray", 0, 2, ArgumentsOrder::DEFAULT_ORDER,
                                  VariableType::JS_POINTER());
    *descriptor = newTaggedArray;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(), VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CopyArray)
{
    // 4 : 4 input parameters
    StubDescriptor copyArray("CopyArray", 0, 4, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_POINTER());
    *descriptor = copyArray;
    std::array<VariableType, 4> params = { // 4 : 4 input parameters
        VariableType::POINTER(), VariableType::JS_POINTER(), VariableType::INT32(),
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NameDictPutIfAbsent)
{
    // 7 : 7 input parameters
    StubDescriptor nameDictPutIfAbsent("NameDictPutIfAbsent", 0, 7, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::JS_POINTER());
    *descriptor = nameDictPutIfAbsent;
    std::array<VariableType, 7> params = { // 7 : 7 input parameters
        VariableType::POINTER(), VariableType::JS_POINTER(), VariableType::JS_POINTER(),
        VariableType::JS_ANY(), VariableType::JS_ANY(), VariableType::INT32(), VariableType::BOOL(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NoticeThroughChainAndRefreshUser)
{
    // 3 : 3 input parameters
    StubDescriptor noticeIC("NoticeThroughChainAndRefreshUser", 0, 3, ArgumentsOrder::DEFAULT_ORDER,
                            VariableType::VOID());
    *descriptor = noticeIC;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(BytecodeHandler)
{
    // 7 : 7 input parameters
    StubDescriptor bytecodeHandler("bytecodeHandler", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = bytecodeHandler;
    std::array<VariableType, 7> params = { // 7 : 7 input parameters
        VariableType::POINTER(),
        VariableType::POINTER(),
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::BYTECODE_HANDLER);
}

CALL_STUB_INIT_DESCRIPTOR(SingleStepDebugging)
{
    // 7 : 7 input parameters
    StubDescriptor singleStepDebugging("singleStepDebugging", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = singleStepDebugging;
    std::array<VariableType, 7> params = { // 7 : 7 input parameters
        VariableType::POINTER(),
        VariableType::POINTER(),
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::BYTECODE_HANDLER);
}

CALL_STUB_INIT_DESCRIPTOR(AsmInterpreterEntry)
{
    // 7 : 7 input parameters
    StubDescriptor asmInterpreterEntry("AsmInterpreterEntry", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = asmInterpreterEntry;
    std::array<VariableType, 7> params = { // 7 : 7 input parameters
        VariableType::POINTER(),
        VariableType::POINTER(),
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(JumpToCInterpreter)
{
    // 7 : 7 input parameters
    StubDescriptor jumpToCInterpreter("jumpToCInterpreter", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::POINTER());
    *descriptor = jumpToCInterpreter;
    std::array<VariableType, 7> params = { // 7 : 7 input parameters
        VariableType::POINTER(),
        VariableType::POINTER(),
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(DebugPrint)
{
    // 1 : 1 input parameters
    StubDescriptor debugPrint("DebugPrint", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = debugPrint;
    // 1 : 1 input parameters
    std::array<VariableType, 1> params = {
        VariableType::INT32(),
    };
    descriptor->SetVariableArgs(true);
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(IncDyn)
{
    // 2 : 2 input parameters
    StubDescriptor incDyn("IncDyn", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = incDyn;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(DecDyn)
{
    // 2 : 2 input parameters
    StubDescriptor decDyn("DecDyn", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = decDyn;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ExpDyn)
{
    // 3 : 3 input parameters
    StubDescriptor expDyn("ExpDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = expDyn;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(IsInDyn)
{
    // 3 : 3 input parameters
    StubDescriptor isInDyn("IsInDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = isInDyn;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(InstanceOfDyn)
{
    // 3 : 3 input parameters
    StubDescriptor instanceOfDyn("InstanceOfDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = instanceOfDyn;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(FastStrictNotEqual)
{
    // 2 : 2 input parameters
    StubDescriptor fastStrictNotEqual("FastStrictNotEqual", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = fastStrictNotEqual;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(FastStrictEqual)
{
    // 2 : 2 input parameters
    StubDescriptor fastStrictEqual("FastStrictEqual", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = fastStrictEqual;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CreateGeneratorObj)
{
    // 2 : 2 input parameters
    StubDescriptor createGeneratorObj("CreateGeneratorObj", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = createGeneratorObj;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowConstAssignment)
{
    // 2 : 2 input parameters
    StubDescriptor throwConstAssignment("ThrowConstAssignment", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = throwConstAssignment;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetTemplateObject)
{
    // 2 : 2 input parameters
    StubDescriptor getTemplateObject("GetTemplateObject", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = getTemplateObject;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetNextPropName)
{
    // 2 : 2 input parameters
    StubDescriptor getNextPropName("GetNextPropName", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = getNextPropName;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowIfNotObject)
{
    // 1 : 1 input parameter
    StubDescriptor throwIfNotObject("ThrowIfNotObject", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = throwIfNotObject;
    // 1 : 1 input parameter
    std::array<VariableType, 1> params = {
        VariableType::POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(InsertOldToNewRememberedSet)
{
    // 3 : 3 input parameters
    StubDescriptor index("InsertOldToNewRememberedSet", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = index;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::POINTER(),
        VariableType::POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(DoubleToInt)
{
    // 1 : 1 input parameters
    StubDescriptor index("DoubleToInt", 0, 1, ArgumentsOrder::DEFAULT_ORDER, VariableType::INT32());
    *descriptor = index;
    // 1 : 1 input parameters
    std::array<VariableType, 1> params = {
        VariableType::FLOAT64(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(MarkingBarrier)
{
    // 5 : 5 input parameters
    StubDescriptor index("MarkingBarrier", 0, 5, ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = index;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::POINTER(),
        VariableType::POINTER(),
        VariableType::POINTER(),
        VariableType::POINTER(),
        VariableType::POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(IterNext)
{
    // 2 : 2 input parameters
    StubDescriptor iterNext("IterNext", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = iterNext;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(RuntimeCallTrampoline)
{
    /* 3 : 3 input parameters */
    StubDescriptor runtimeCallTrampoline("RuntimeCallTrampoline", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = runtimeCallTrampoline;
    std::array<VariableType, 3> params = { /* 3 : 3 input parameters */
        VariableType::POINTER(),
        VariableType::INT64(),
        VariableType::INT64(),
    };
    descriptor->SetVariableArgs(true);
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CloseIterator)
{
    // 2 : 2 input parameters
    StubDescriptor closeIterator("CloseIterator", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = closeIterator;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CopyModule)
{
    // 2 : 2 input parameters
    StubDescriptor copyModule("CopyModule", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = copyModule;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(SuperCallSpread)
{
    // 4 : 4 input parameters
    StubDescriptor superCallSpread("SuperCallSpread", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = superCallSpread;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(DelObjProp)
{
    // 3 : 3 input parameters
    StubDescriptor delObjProp("DelObjProp", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = delObjProp;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NewObjSpreadDyn)
{
    // 4 : 4 input parameters
    StubDescriptor newObjSpreadDyn("NewObjSpreadDyn", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = newObjSpreadDyn;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CreateIterResultObj)
{
    // 3 : 3 input parameters
    StubDescriptor createIterResultObj("CreateIterResultObj", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = createIterResultObj;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(AsyncFunctionAwaitUncaught)
{
    // 3 : 3 input parameters
    StubDescriptor asyncFunctionAwaitUncaught("AsyncFunctionAwaitUncaught", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = asyncFunctionAwaitUncaught;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(AsyncFunctionResolveOrReject)
{
    // 4: 4 input parameters
    StubDescriptor asyncFunctionResolveOrReject("AsyncFunctionResolveOrReject", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = asyncFunctionResolveOrReject;
    // 4: 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::BOOL(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}


CALL_STUB_INIT_DESCRIPTOR(ThrowUndefinedIfHole)
{
    // 2 : 2 input parameters
    StubDescriptor throwUndefinedIfHole("ThrowUndefinedIfHole", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = throwUndefinedIfHole;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CopyDataProperties)
{
    // 3 : 3 input parameters
    StubDescriptor copyDataProperties("CopyDataProperties", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = copyDataProperties;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StArraySpread)
{
    // 4 : 4 input parameters
    StubDescriptor stArraySpread("StArraySpread", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = stArraySpread;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetIteratorNext)
{
    // 3 : 3 input parameters
    StubDescriptor getIteratorNext("GetIteratorNext", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = getIteratorNext;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(SetObjectWithProto)
{
    // 3 : 3 input parameters
    StubDescriptor setObjectWithProto("SetObjectWithProto", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = setObjectWithProto;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LoadICByValue)
{
    // 5 : 5 input parameters
    StubDescriptor loadICByValue("LoadICByValue", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = loadICByValue;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StoreICByValue)
{
    // 6 : 6 input parameters
    StubDescriptor storeICByValue("StoreICByValue", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = storeICByValue;
    // 6 : 6 input parameters
    std::array<VariableType, 6> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StOwnByValue)
{
    // 4 : 4 input parameters
    StubDescriptor stOwnByValue("StOwnByValue", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = stOwnByValue;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LdSuperByValue)
{
    // 4 : 4 input parameters
    StubDescriptor ldSuperByValue("LdSuperByValue", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = ldSuperByValue;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StSuperByValue)
{
    // 5 : 5 input parameters
    StubDescriptor stSuperByValue("StSuperByValue", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = stSuperByValue;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LdObjByIndex)
{
    // 5 : 5 input parameters
    StubDescriptor ldObjByIndex("LdObjByIndex", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = ldObjByIndex;
    // 5 : 5 input parameters
    std::array<VariableType, 5> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::BOOL(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StObjByIndex)
{
    // 4 : 4 input parameters
    StubDescriptor stObjByIndex("StObjByIndex", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = stObjByIndex;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StOwnByIndex)
{
    // 4 : 4 input parameters
    StubDescriptor stOwnByIndex("StOwnByIndex", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = stOwnByIndex;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StGlobalRecord)
{
    // 4 : 4 input parameters
    StubDescriptor stGlobalRecord("StGlobalRecord", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = stGlobalRecord;
    std::array<VariableType, 4> params = { // 4 : 4 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::BOOL(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NegDyn)
{
    // 2 : 2 input parameters
    StubDescriptor NegDyn("NegDyn", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = NegDyn;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NotDyn)
{
    // 2 : 2 input parameters
    StubDescriptor NotDyn("NotDyn", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = NotDyn;
    // 2 : 2 input parameters
    std::array<VariableType, 2> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ChangeTwoInt32AndToJSTaggedValue)
{
    // 3 : 3 input parameters
    StubDescriptor ChangeTwoInt32AndToJSTaggedValue("ChangeTwoInt32AndToJSTaggedValue", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = ChangeTwoInt32AndToJSTaggedValue;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ChangeTwoInt32XorToJSTaggedValue)
{
    // 3 : 3 input parameters
    StubDescriptor ChangeTwoInt32XorToJSTaggedValue("ChangeTwoInt32XorToJSTaggedValue", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = ChangeTwoInt32XorToJSTaggedValue;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ChangeTwoInt32OrToJSTaggedValue)
{
    // 3 : 3 input parameters
    StubDescriptor ChangeTwoInt32OrToJSTaggedValue("ChangeTwoInt32OrToJSTaggedValue", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = ChangeTwoInt32OrToJSTaggedValue;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ChangeTwoUint32AndToJSTaggedValue)
{
    // 3 : 3 input parameters
    StubDescriptor ChangeTwoUint32AndToJSTaggedValue("ChangeTwoUint32AndToJSTaggedValue", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = ChangeTwoUint32AndToJSTaggedValue;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ChangeUintAndIntShrToJSTaggedValue)
{
    // 3 : 3 input parameters
    StubDescriptor ChangeUintAndIntShrToJSTaggedValue("ChangeUintAndIntShrToJSTaggedValue", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = ChangeUintAndIntShrToJSTaggedValue;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ChangeUintAndIntShlToJSTaggedValue)
{
    // 3 : 3 input parameters
    StubDescriptor ChangeUintAndIntShlToJSTaggedValue("ChangeUintAndIntShlToJSTaggedValue", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = ChangeUintAndIntShlToJSTaggedValue;
    // 3 : 3 input parameters
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ResolveClass)
{
    // 6 : 6 input parameters
    StubDescriptor resolveClass("ResolveClass", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = resolveClass;
    std::array<VariableType, 6> params = { // 6 : 6 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CloneClassFromTemplate)
{
    // 5 : 5 input parameters
    StubDescriptor cloneClassFromTemplate("CloneClassFromTemplate", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = cloneClassFromTemplate;
    std::array<VariableType, 5> params = { // 5 : 5 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(SetClassConstructorLength)
{
    // 3 : 3 input parameters
    StubDescriptor setClassConstructorLength("SetClassConstructorLength", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = setClassConstructorLength;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT16(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(UpdateHotnessCounter)
{
    // 2 : 2 input parameters
    StubDescriptor updateHotnessCounter("UpdateHotnessCounter", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = updateHotnessCounter;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetModuleNamespace)
{
    // 2 : 2 input parameters
    StubDescriptor getModuleNamespace("GetModuleNamespace", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = getModuleNamespace;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StModuleVar)
{
    // 3 : 3 input parameters
    StubDescriptor stModuleVar("StModuleVar", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = stModuleVar;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LdModuleVar)
{
    // 3 : 3 input parameters
    StubDescriptor ldModuleVar("LdModuleVar", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = ldModuleVar;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT16(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowDyn)
{
    // 2 : 2 input parameters
    StubDescriptor throwDyn("ThrowDyn", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = throwDyn;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetPropIterator)
{
    // 2 : 2 input parameters
    StubDescriptor getPropIterator("GetPropIterator", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = getPropIterator;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LdGlobalRecord)
{
    // 2 : 2 input parameters
    StubDescriptor ldGlobalRecord("LdGlobalRecord", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = ldGlobalRecord;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(AsyncFunctionEnter)
{
    // 1 : 1 input parameters
    StubDescriptor asyncFunctionEnter("AsyncFunctionEnter", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = asyncFunctionEnter;
    std::array<VariableType, 1> params = { // 1 : 1 input parameters
        VariableType::POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetGlobalOwnProperty)
{
    // 2 : 2 input parameters
    StubDescriptor getGlobalOwnProperty("GetGlobalOwnProperty", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = getGlobalOwnProperty;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetIterator)
{
    // 2 : 2 input parameters
    StubDescriptor getIterator("GetIterator", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = getIterator;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowThrowNotExists)
{
    // 1 : 1 input parameters
    StubDescriptor throwThrowNotExists("ThrowThrowNotExists", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = throwThrowNotExists;
    std::array<VariableType, 1> params = { // 1 : 1 input parameters
        VariableType::POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(TryLdGlobalByName)
{
    // 2 : 2 input parameters
    StubDescriptor tryLdGlobalByName("TryLdGlobalByName", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = tryLdGlobalByName;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LoadMiss)
{
    // 6 : 6 input parameters
    StubDescriptor loadMiss("LoadMiss", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = loadMiss;
    std::array<VariableType, 6> params = { // 6 : 6 input parameters
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StoreMiss)
{
    // 7 : 7 input parameters
    StubDescriptor storeMiss("StoreMiss", 0, 7,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = storeMiss;
    std::array<VariableType, 7> params = { // 7 : 7 input parameters
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::INT32(),
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowPatternNonCoercible)
{
    // 1 : 1 input parameters
    StubDescriptor throwPatternNonCoercible("ThrowPatternNonCoercible", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = throwPatternNonCoercible;
    std::array<VariableType, 1> params = { // 1 : 1 input parameters
        VariableType::POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(TryUpdateGlobalRecord)
{
    // 3 : 3 input parameters
    StubDescriptor tryUpdateGlobalRecord("TryUpdateGlobalRecord", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = tryUpdateGlobalRecord;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowDeleteSuperProperty)
{
    // 1 : 1 input parameters
    StubDescriptor throwDeleteSuperProperty("ThrowDeleteSuperProperty", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::VOID());
    *descriptor = throwDeleteSuperProperty;
    std::array<VariableType, 1> params = { // 1 : 1 input parameters
        VariableType::POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowReferenceError)
{
    // 2 : 2 input parameters
    StubDescriptor throwReferenceError("ThrowReferenceError", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = throwReferenceError;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(EqDyn)
{
    // 3 : 3 input parameters
    StubDescriptor eqDyn("EqDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = eqDyn;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LdGlobalVar)
{
    // 3 : 3 input parameters
    StubDescriptor ldGlobalVar("LdGlobalVar", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = ldGlobalVar;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StGlobalVar)
{
    // 3 : 3 input parameters
    StubDescriptor stGlobalVar("StGlobalVar", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = stGlobalVar;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ToBoolean)
{
    // 1 : 1 input parameters
    StubDescriptor toBoolean("ToBoolean", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::BOOL());
    *descriptor = toBoolean;
    std::array<VariableType, 1> params = { // 1 : 1 input parameters
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NotEqDyn)
{
    // 3 : 3 input parameters
    StubDescriptor notEqDyn("NotEqDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = notEqDyn;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LessDyn)
{
    // 3 : 3 input parameters
    StubDescriptor lessDyn("LessDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = lessDyn;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LessEqDyn)
{
    // 3 : 3 input parameters
    StubDescriptor lessEqDyn("LessEqDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = lessEqDyn;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GreaterDyn)
{
    // 3 : 3 input parameters
    StubDescriptor greaterDyn("GreaterDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = greaterDyn;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GreaterEqDyn)
{
    // 3 : 3 input parameters
    StubDescriptor greaterEqDyn("GreaterEqDyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = greaterEqDyn;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(SetPropertyByValue)
{
    // 4 : 4 input parameters
    StubDescriptor setPropertyByValue("SetPropertyByValue", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::INT64());
    *descriptor = setPropertyByValue;
    // 4 : 4 input parameters
    std::array<VariableType, 4> params = {
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(SetFunctionNameNoPrefix)
{
    // 3 : 3 input parameters
    StubDescriptor setFunctionNameNoPrefix("SetFunctionNameNoPrefix", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = setFunctionNameNoPrefix;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StOwnByName)
{
    // 4 : 4 input parameters
    StubDescriptor stOwnByName("StOwnByName", 0, 4, ArgumentsOrder::DEFAULT_ORDER, VariableType::INT64());
    *descriptor = stOwnByName;
    std::array<VariableType, 4> params = { // 4 : 4 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StOwnByNameWithNameSet)
{
    // 4 : 4 input parameters
    StubDescriptor stOwnByNameWithNameSet("StOwnByNameWithNameSet", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = stOwnByNameWithNameSet;
    std::array<VariableType, 4> params = { // 4 : 4 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(StOwnByValueWithNameSet)
{
    // 4 : 4 input parameters
    StubDescriptor stOwnByValueWithNameSet("StOwnByValueWithNameSet", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = stOwnByValueWithNameSet;
    std::array<VariableType, 4> params = { // 4 : 4 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(SuspendGenerator)
{
    // 3 : 3 input parameters
    StubDescriptor suspendGenerator("SuspendGenerator", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = suspendGenerator;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(UpFrame)
{
    // 2 : 2 input parameters
    StubDescriptor upFrame("UpFrame", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::POINTER());
    *descriptor = upFrame;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ToNumber)
{
    // 2 : 2 input parameters
    StubDescriptor toNumber("ToNumber", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = toNumber;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(Add2Dyn)
{
    // 3 : 3 input parameters
    StubDescriptor add2Dyn("Add2Dyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = add2Dyn;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(Sub2Dyn)
{
    // 3 : 3 input parameters
    StubDescriptor sub2Dyn("Sub2Dyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = sub2Dyn;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(Mul2Dyn)
{
    // 3 : 3 input parameters
    StubDescriptor mul2Dyn("Mul2Dyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = mul2Dyn;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(Div2Dyn)
{
    // 3 : 3 input parameters
    StubDescriptor div2Dyn("Div2Dyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = div2Dyn;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(Mod2Dyn)
{
    // 3 : 3 input parameters
    StubDescriptor mod2Dyn("Mod2Dyn", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = mod2Dyn;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetLexicalEnv)
{
    StubDescriptor getLexicalEnv("GetLexicalEnv", 0, 1, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = getLexicalEnv;
    std::array<VariableType, 1> params = {
        VariableType::POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(LoadValueFromConstantPool)
{
    // 3 : 3 input parameter
    StubDescriptor loadValue("LoadValueFromConstantPool", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = loadValue;
    // 3 : 3 input parameter
    std::array<VariableType, 3> params = {
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::INT32(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CreateEmptyObject)
{
    // 1 : 1 input parameters
    StubDescriptor createEmptyObject("CreateEmptyObject", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = createEmptyObject;
    std::array<VariableType, 1> params = { // 1 : 1 input parameters
        VariableType::POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CreateEmptyArray)
{
    // 1 : 1 input parameters
    StubDescriptor createEmptyArray("CreateEmptyArray", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = createEmptyArray;
    std::array<VariableType, 1> params = { // 1 : 1 input parameters
        VariableType::POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetSymbolFunction)
{
    // 1 : 1 input parameters
    StubDescriptor getSymbolFunction("GetSymbolFunction", 0, 1,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = getSymbolFunction;
    std::array<VariableType, 1> params = { // 1 : 1 input parameters
        VariableType::POINTER(),
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetUnmapedArgs)
{
    // 2 : 2 input parameters
    StubDescriptor getUnmapedArgs("GetUnmapedArgs", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = getUnmapedArgs;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::POINTER()
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CopyRestArgs)
{
    // 3 : 3 input parameters
    StubDescriptor copyRestArgs("CopyRestArgs", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = copyRestArgs;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::POINTER(),
        VariableType::INT32()
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CreateArrayWithBuffer)
{
    // 2 : 2 input parameters
    StubDescriptor createArrayWithBuffer("CreateArrayWithBuffer", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = createArrayWithBuffer;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::JS_POINTER()
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CreateObjectWithBuffer)
{
    // 2 : 2 input parameters
    StubDescriptor createObjectWithBuffer("CreateObjectWithBuffer", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = createObjectWithBuffer;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::JS_POINTER()
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NewLexicalEnvDyn)
{
    // 2 : 2 input parameters
    StubDescriptor newLexicalEnvDyn("NewLexicalEnvDyn", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = newLexicalEnvDyn;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::INT16()
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(NewObjDynRange)
{
    // 5 : 5 input parameters
    StubDescriptor newObjDynRange("NewObjDynRange", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = newObjDynRange;
    std::array<VariableType, 5> params = { // 5 : 5 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::INT16(),
        VariableType::INT16()
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(DefinefuncDyn)
{
    // 2 : 2 input parameters
    StubDescriptor definefuncDyn("DefinefuncDyn", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = definefuncDyn;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::JS_POINTER()
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CreateRegExpWithLiteral)
{
    // 3 : 3 input parameters
    StubDescriptor createRegExpWithLiteral("CreateRegExpWithLiteral", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = createRegExpWithLiteral;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::INT8()
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowIfSuperNotCorrectCall)
{
    // 3 : 3 input parameters
    StubDescriptor throwIfSuperNotCorrectCall("ThrowIfSuperNotCorrectCall", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = throwIfSuperNotCorrectCall;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::INT16(),
        VariableType::JS_ANY()
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CreateObjectHavingMethod)
{
    // 4 : 4 input parameters
    StubDescriptor createObjectHavingMethod("CreateObjectHavingMethod", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = createObjectHavingMethod;
    std::array<VariableType, 4> params = { // 4 : 4 input parameters
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_POINTER()
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CreateObjectWithExcludedKeys)
{
    // 4 : 4 input parameters
    StubDescriptor createObjectWithExcludedKeys("CreateObjectWithExcludedKeys", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = createObjectWithExcludedKeys;
    std::array<VariableType, 4> params = { // 4 : 4 input parameters
        VariableType::POINTER(),
        VariableType::INT16(),
        VariableType::JS_ANY(),
        VariableType::INT16()
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(DefineNCFuncDyn)
{
    // 2 : 2 input parameters
    StubDescriptor defineNCFuncDyn("DefineNCFuncDyn", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = defineNCFuncDyn;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::JS_POINTER()
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(DefineGeneratorFunc)
{
    // 2 : 2 input parameters
    StubDescriptor defineGeneratorFunc("DefineGeneratorFunc", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = defineGeneratorFunc;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::JS_POINTER()
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(DefineAsyncFunc)
{
    // 2 : 2 input parameters
    StubDescriptor defineAsyncFunc("DefineAsyncFunc", 0, 2,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = defineAsyncFunc;
    std::array<VariableType, 2> params = { // 2 : 2 input parameters
        VariableType::POINTER(),
        VariableType::JS_POINTER()
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(DefineMethod)
{
    // 3 : 3 input parameters
    StubDescriptor defineMethod("DefineMethod", 0, 3,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = defineMethod;
    std::array<VariableType, 3> params = { // 3 : 3 input parameters
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY()
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CallSpreadDyn)
{
    // 4 : 4 input parameters
    StubDescriptor callSpreadDyn("CallSpreadDyn", 0, 4,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = callSpreadDyn;
    std::array<VariableType, 4> params = { // 4 : 4 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY()
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(DefineGetterSetterByValue)
{
    // 6 : 6 input parameters
    StubDescriptor defineGetterSetterByValue("DefineGetterSetterByValue", 0, 6,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = defineGetterSetterByValue;
    std::array<VariableType, 6> params = { // 6 : 6 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY()
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubDescriptor::CallStubKind::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(SuperCall)
{
    // 5 : 5 input parameters
    StubDescriptor superCall("SuperCall", 0, 5,
        ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
    *descriptor = superCall;
    std::array<VariableType, 5> params = { // 5 : 5 input parameters
        VariableType::POINTER(),
        VariableType::JS_ANY(),
        VariableType::POINTER(),
        VariableType::INT16(),
        VariableType::INT16()
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
