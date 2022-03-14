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
    StubDescriptor fastEqual("FastEqual", 0, 3, ArgumentsOrder::DEFAULT_ORDER, VariableType::JS_ANY());
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
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY()
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
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY()
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(SetPropertyByValue)
{
    // 4 : 4 input parameters
    StubDescriptor setPropertyByName("SetPropertyByValue", 0, 4, ArgumentsOrder::DEFAULT_ORDER,
        VariableType::INT64());
    *descriptor = setPropertyByName;

    std::array<VariableType, 4> params = { // 4 : 4 input parameters
        VariableType::POINTER(),
        VariableType::JS_POINTER(),
        VariableType::JS_ANY(),
        VariableType::JS_ANY()
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

CALL_STUB_INIT_DESCRIPTOR(RuntimeCallTrampolineInterpreterAsm)
{
    /* 3 : 3 input parameters */
    StubDescriptor runtimeCallTrampoline("RuntimeCallTrampolineInterpreterAsm", 0, 3,
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

CALL_STUB_INIT_DESCRIPTOR(RuntimeCallTrampolineAot)
{
    /* 3 : 3 input parameters */
    StubDescriptor runtimeCallTrampoline("RuntimeCallTrampolineAot", 0, 3,
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
