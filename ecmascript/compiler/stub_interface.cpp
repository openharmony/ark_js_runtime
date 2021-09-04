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

#include "ecmascript/compiler/stub_interface.h"

#include "llvm-c/Core.h"
#include "llvm/Support/Host.h"

namespace kungfu {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)

#define LLVM_STUB_GETFUCTION(name)                                    \
    class LLVM##name##Stub final {                                    \
    public:                                                           \
        static LLVMValueRef InitializeFunction(LLVMModuleRef module); \
    };                                                                \
    LLVMValueRef LLVM##name##Stub::InitializeFunction(LLVMModuleRef module)

LLVM_STUB_GETFUCTION(FastAdd)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(FastSub)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(FastMul)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(FastDiv)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(FastMod)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(FastEqual)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(FastTypeOf)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(FastStrictEqual)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(IsSpecialIndexedObjForSet)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(IsSpecialIndexedObjForGet)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(GetElement)
{
    // 2 : 2 input parameters
    std::array<LLVMTypeRef, 2> paramTys = {
        LLVMInt64Type(),
        LLVMInt32Type(),
    };
    // 2 : 2 input parameters
    return LLVMAddFunction(module, "GetElement", LLVMFunctionType(LLVMInt64Type(), paramTys.data(), 2, 0));
}

LLVM_STUB_GETFUCTION(SetElement)
{
    // 5 : 5 input parameters
    std::array<LLVMTypeRef, 5> paramTys = {
        LLVMInt64Type(), LLVMInt64Type(), LLVMInt32Type(), LLVMInt64Type(), LLVMInt32Type(),
    };
    // 5 : 5 input parameters
    return LLVMAddFunction(module, "SetElement", LLVMFunctionType(LLVMInt1Type(), paramTys.data(), 5, 0));
}

LLVM_STUB_GETFUCTION(SetPropertyByName)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(GetPropertyByName)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(SetGlobalOwnProperty)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(GetGlobalOwnProperty)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(SetOwnPropertyByName)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(SetOwnElement)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(FastSetProperty)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(FastGetProperty)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(FindOwnProperty)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(FindOwnElement)
{
    // 2 : 2 input parameters
    std::array<LLVMTypeRef, 2> paramTys = {
        LLVMInt64Type(),
        LLVMInt32Type(),
    };
    // 2 : 2 input parameters
    return LLVMAddFunction(module, "FindOwnElement", LLVMFunctionType(LLVMInt64Type(), paramTys.data(), 2, 0));
}

LLVM_STUB_GETFUCTION(NewLexicalEnvDyn)
{
    (void)module;
    return nullptr;
}

LLVM_STUB_GETFUCTION(FindOwnProperty2)
{
    // 5 : 5 input parameters
    std::array<LLVMTypeRef, 5> paramTys = {
        LLVMInt64Type(), LLVMInt32Type(), LLVMInt1Type(), LLVMInt64Type(), LLVMInt64Type(),
    };
    // 5 : 5 input parameters
    return LLVMAddFunction(module, "FindOwnProperty2", LLVMFunctionType(LLVMInt64Type(), paramTys.data(), 5, 0));
}

LLVM_STUB_GETFUCTION(FindOwnElement2)
{
    // 5 : 5 input parameters
    std::array<LLVMTypeRef, 5> paramTys = {
        LLVMInt64Type(), LLVMInt32Type(), LLVMInt1Type(), LLVMInt64Type(), LLVMInt64Type(),
    };
    // 5 : 5 input parameters
    return LLVMAddFunction(module, "FindOwnElement2", LLVMFunctionType(LLVMInt64Type(), paramTys.data(), 5, 0));
}

LLVM_STUB_GETFUCTION(AddElementInternal)
{
    // 5 : 5 input parameters
    std::array<LLVMTypeRef, 5> paramTys = {
        LLVMInt64Type(), LLVMInt64Type(), LLVMInt32Type(), LLVMInt64Type(), LLVMInt32Type(),
    };
    return LLVMAddFunction(module, "_ZN5panda10ecmascript11RuntimeStub18AddElementInternalEmmjmj",
                           LLVMFunctionType(LLVMInt1Type(), paramTys.data(), 5, 0));  // 5 : 5 input parameters
}

LLVM_STUB_GETFUCTION(CallSetter)
{
    // 5 : 5 input parameters
    std::array<LLVMTypeRef, 5> paramTys = {
        LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type(), LLVMInt1Type(),
    };
    return LLVMAddFunction(module, "_ZN5panda10ecmascript11RuntimeStub10CallSetterEmmmmb",
                           LLVMFunctionType(LLVMInt1Type(), paramTys.data(), 5, 0));  // 5 : 5 input parameters
}

LLVM_STUB_GETFUCTION(ThrowTypeError)
{
    // 2 : 2 input parameter
    std::array<LLVMTypeRef, 2> paramTys = {
        LLVMInt64Type(),
        LLVMInt32Type(),
    };
    return LLVMAddFunction(module, " _ZN5panda10ecmascript11RuntimeStub14ThrowTypeErrorEmi",
                           LLVMFunctionType(LLVMVoidType(), paramTys.data(), 2, 0));  // 2 : 2 input parameters
}

LLVM_STUB_GETFUCTION(JSProxySetProperty)
{
    // 6 : 6 input parameters
    std::array<LLVMTypeRef, 6> paramTys = {
        LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type(), LLVMInt1Type(),
    };
    return LLVMAddFunction(module, "_ZN5panda10ecmascript11RuntimeStub18JSProxySetPropertyEmmmmmb",
                           LLVMFunctionType(LLVMInt1Type(), paramTys.data(), 6, 0));  // 6 : 6 input parameters
}

LLVM_STUB_GETFUCTION(GetHash32)
{
    std::array<LLVMTypeRef, 2> paramTys = {  // 2 : 2 input parameters
                                           LLVMInt64Type(), LLVMInt32Type()};
    return LLVMAddFunction(module, "_ZN5panda9GetHash32EPKhm",
                           LLVMFunctionType(LLVMInt32Type(), paramTys.data(), 2, 0));  // 2 : 2 input parameters
}

LLVM_STUB_GETFUCTION(PhiTest)
{
    std::array<LLVMTypeRef, 1> paramTys = {LLVMInt32Type()};
    return LLVMAddFunction(module, "PhiTest", LLVMFunctionType(LLVMInt32Type(), paramTys.data(), 1, 0));
}

LLVMStubsImplement::LLVMStubsImplement()
{
    stubsModule_ = LLVMModuleCreateWithName("fast_stubs");
    LLVMSetTarget(stubsModule_, "x86_64-unknown-linux-gnu");
}

void LLVMStubsImplement::Initialize()
{
// Initialize Stubs Function
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEF_CALL_STUB(name) NAME_##name
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INITIALIZE_CALL_STUB(name, argcounts) \
    llvmCallStubs_[DEF_CALL_STUB(name)] = LLVM##name##Stub::InitializeFunction(stubsModule_);
    CALL_STUB_LIST(INITIALIZE_CALL_STUB)
#undef INITIALIZE_CALL_STUB
#undef DEF_CALL_STUB
}

void *LLVMStubsImplement::GetFastStub(int index)
{
    ASSERT(index < CALL_STUB_MAXCOUNT && index >= 0);
    return reinterpret_cast<void *>(llvmCallStubs_[index]);
}

void LLVMStubsImplement::SetFastStub(int index, void *code)
{
    ASSERT(index < CALL_STUB_MAXCOUNT && index >= 0);
    llvmCallStubs_[index] = reinterpret_cast<LLVMValueRef>(code);
}

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_STUB_INIT_DESCRIPTOR(name)                              \
    class Stub##name##InterfaceDescriptor final {                    \
    public:                                                          \
        static void Initialize(StubInterfaceDescriptor *descriptor); \
    };                                                               \
    void Stub##name##InterfaceDescriptor::Initialize(StubInterfaceDescriptor *descriptor)

CALL_STUB_INIT_DESCRIPTOR(FastAdd) {}

CALL_STUB_INIT_DESCRIPTOR(FastSub) {}

CALL_STUB_INIT_DESCRIPTOR(FastMul) {}

CALL_STUB_INIT_DESCRIPTOR(FastDiv) {}

CALL_STUB_INIT_DESCRIPTOR(FastMod) {}

CALL_STUB_INIT_DESCRIPTOR(FastEqual) {}

CALL_STUB_INIT_DESCRIPTOR(FastTypeOf) {}

CALL_STUB_INIT_DESCRIPTOR(FastStrictEqual) {}

CALL_STUB_INIT_DESCRIPTOR(IsSpecialIndexedObjForSet) {}

CALL_STUB_INIT_DESCRIPTOR(IsSpecialIndexedObjForGet) {}

CALL_STUB_INIT_DESCRIPTOR(GetElement) {}

CALL_STUB_INIT_DESCRIPTOR(SetElement) {}

CALL_STUB_INIT_DESCRIPTOR(SetPropertyByName) {}

CALL_STUB_INIT_DESCRIPTOR(GetPropertyByName) {}

CALL_STUB_INIT_DESCRIPTOR(SetGlobalOwnProperty) {}

CALL_STUB_INIT_DESCRIPTOR(GetGlobalOwnProperty) {}

CALL_STUB_INIT_DESCRIPTOR(SetOwnPropertyByName) {}

CALL_STUB_INIT_DESCRIPTOR(SetOwnElement) {}

CALL_STUB_INIT_DESCRIPTOR(FastSetProperty) {}

CALL_STUB_INIT_DESCRIPTOR(FastGetProperty) {}

CALL_STUB_INIT_DESCRIPTOR(FindOwnProperty) {}

CALL_STUB_INIT_DESCRIPTOR(FindOwnElement) {}

CALL_STUB_INIT_DESCRIPTOR(NewLexicalEnvDyn) {}

CALL_STUB_INIT_DESCRIPTOR(FindOwnProperty2) {}

CALL_STUB_INIT_DESCRIPTOR(FindOwnElement2)
{
    // 5 : 5 input parameters
    static StubInterfaceDescriptor findOwnElement2(0, 5, DEFAULT_ORDER, UINT64_TYPE);
    *descriptor = findOwnElement2;
    auto params = new std::array<MachineType, 5>();  // 5 : 5 input parameters
    (*params)[0] = MachineType::UINT64_TYPE;
    (*params)[1] = MachineType::UINT32_TYPE;
    (*params)[2] = MachineType::BOOL_TYPE;    // 2 : 3rd para
    (*params)[3] = MachineType::UINT64_TYPE;  // 3 : 4th para
    (*params)[4] = MachineType::UINT64_TYPE;  // 4 : 5th para
    descriptor->SetParameters(params->data());
}

CALL_STUB_INIT_DESCRIPTOR(AddElementInternal)
{
    // 5 : 5 input parameters
    static StubInterfaceDescriptor addElementInternal(0, 5, DEFAULT_ORDER, BOOL_TYPE);
    *descriptor = addElementInternal;
    auto params = new std::array<MachineType, 5>();  // 5 : 5 input parameters
    (*params)[0] = MachineType::UINT64_TYPE;
    (*params)[1] = MachineType::UINT64_TYPE;
    (*params)[2] = MachineType::UINT32_TYPE;  // 2 : 3rd para
    (*params)[3] = MachineType::UINT64_TYPE;  // 3 : 4th para
    (*params)[4] = MachineType::UINT32_TYPE;  // 4 : 5th para
    descriptor->SetParameters(params->data());
}

CALL_STUB_INIT_DESCRIPTOR(CallSetter)
{
    // 5 : 5 input parameters
    static StubInterfaceDescriptor callSetter(0, 5, DEFAULT_ORDER, NONE_TYPE);
    *descriptor = callSetter;
    auto params = new std::array<MachineType, 5>();  // 5 : 5 input parameters
    (*params)[0] = MachineType::UINT64_TYPE;
    (*params)[1] = MachineType::UINT64_TYPE;
    (*params)[2] = MachineType::UINT64_TYPE;  // 2 : 3rd para
    (*params)[3] = MachineType::UINT64_TYPE;  // 3 : 4th para
    (*params)[4] = MachineType::BOOL_TYPE;    // 4 : 5th para
    descriptor->SetParameters(params->data());
}

CALL_STUB_INIT_DESCRIPTOR(ThrowTypeError)
{
    // 2 : 2 input parameters
    static StubInterfaceDescriptor throwTypeError(0, 2, DEFAULT_ORDER, NONE_TYPE);
    *descriptor = throwTypeError;
    auto params = new std::array<MachineType, 2>();  // 2 : 2 input parameters
    (*params)[0] = MachineType::UINT64_TYPE;
    (*params)[1] = MachineType::UINT32_TYPE;
    descriptor->SetParameters(params->data());
}

CALL_STUB_INIT_DESCRIPTOR(JSProxySetProperty)
{
    // 6 : 6 input parameters
    static StubInterfaceDescriptor jsproxySetproperty(0, 6, DEFAULT_ORDER, BOOL_TYPE);
    *descriptor = jsproxySetproperty;
    auto params = new std::array<MachineType, 6>();  // 6 : 6 input parameters
    (*params)[0] = MachineType::UINT64_TYPE;
    (*params)[1] = MachineType::UINT64_TYPE;
    (*params)[2] = MachineType::UINT64_TYPE;  // 2 : 3rd para
    (*params)[3] = MachineType::UINT64_TYPE;  // 3 : 4th para
    (*params)[4] = MachineType::UINT64_TYPE;  // 4 : 5th para
    (*params)[5] = MachineType::BOOL_TYPE;    // 5 : 6th para
    descriptor->SetParameters(params->data());
}

CALL_STUB_INIT_DESCRIPTOR(GetHash32)
{
    // 2 : 2 input parameters
    static StubInterfaceDescriptor getHash32(0, 2, DEFAULT_ORDER, UINT32_TYPE);
    *descriptor = getHash32;
    auto params = new std::array<MachineType, 2>();  // 2 : 2 input parameters
    (*params)[0] = MachineType::POINTER_TYPE;
    (*params)[1] = MachineType::UINT32_TYPE;
    descriptor->SetParameters(params->data());
}

CALL_STUB_INIT_DESCRIPTOR(PhiTest) {}

void FastStubs::InitializeStubDescriptors()
{
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEF_CALL_STUB(name) CallStubsImplement::NAME_##name
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INITIALIZE_CALL_STUB_DESCRIPTOR(name, argcounts) \
    Stub##name##InterfaceDescriptor::Initialize(&callStubsDescriptor_[DEF_CALL_STUB(name)]);
    CALL_STUB_LIST(INITIALIZE_CALL_STUB_DESCRIPTOR)
#undef INITIALIZE_CALL_STUB_DESCRIPTOR
#undef DEF_CALL_STUB
}
}  // namespace kungfu
