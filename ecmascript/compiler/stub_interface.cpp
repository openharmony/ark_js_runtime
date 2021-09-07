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

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LLVM_STUB_GETFUCTION_TYPE(name)              \
    class LLVM##name##Stub##Type final {             \
    public:                                          \
        static LLVMTypeRef InitializeFunctionType(); \
    };                                               \
    LLVMTypeRef LLVM##name##Stub##Type::InitializeFunctionType()

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
    // 6 : 6 input parameters
    std::array<LLVMTypeRef, 6> paramTys = {
        LLVMInt64Type(), LLVMInt64Type(), LLVMInt32Type(), LLVMInt1Type(), LLVMInt64Type(), LLVMInt64Type(),
    };
    // 6 : 6 input parameters
    return LLVMAddFunction(module, "FindOwnElement2", LLVMFunctionType(LLVMInt64Type(), paramTys.data(), 6, 0));
}

LLVM_STUB_GETFUCTION_TYPE(FastAdd)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(FastSub)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(FastMul)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(FastDiv)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(FastMod)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(FastEqual)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(FastTypeOf)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(FastStrictEqual)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(IsSpecialIndexedObjForSet)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(IsSpecialIndexedObjForGet)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(GetElement)
{
    // 2 : 2 input parameters
    std::array<LLVMTypeRef, 2> paramTys = {
        LLVMInt64Type(),
        LLVMInt32Type(),
    };
    return LLVMFunctionType(LLVMInt64Type(), paramTys.data(), 2, 0);  // 2 : 2  parameters number
}

LLVM_STUB_GETFUCTION_TYPE(SetElement)
{
    // 5 : 5 input parameters
    std::array<LLVMTypeRef, 5> paramTys = {
        LLVMInt64Type(), LLVMInt64Type(), LLVMInt32Type(), LLVMInt64Type(), LLVMInt32Type(),
    };
    return LLVMFunctionType(LLVMInt1Type(), paramTys.data(), 5, 0);  // 5 : 5 parameters number
}

LLVM_STUB_GETFUCTION_TYPE(SetPropertyByName)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(GetPropertyByName)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(SetGlobalOwnProperty)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(GetGlobalOwnProperty)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(SetOwnPropertyByName)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(SetOwnElement)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(FastSetProperty)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(FastGetProperty)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(FindOwnProperty)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(FindOwnElement)
{
    // 2 : 2 parameters number
    std::array<LLVMTypeRef, 2> paramTys = {
        LLVMInt64Type(),
        LLVMInt32Type(),
    };
    return LLVMFunctionType(LLVMInt64Type(), paramTys.data(), 2, 0);  // 2 : 2  parameters number
}

LLVM_STUB_GETFUCTION_TYPE(NewLexicalEnvDyn)
{
    return nullptr;
}

LLVM_STUB_GETFUCTION_TYPE(FindOwnProperty2)
{
    // 5 : 5 parameters number
    std::array<LLVMTypeRef, 5> paramTys = {
        LLVMInt64Type(), LLVMInt32Type(), LLVMInt1Type(), LLVMInt64Type(), LLVMInt64Type(),
    };
    return LLVMFunctionType(LLVMInt64Type(), paramTys.data(), 5, 0);  // 5 : 5 parameters number
}

LLVM_STUB_GETFUCTION_TYPE(FindOwnElement2)
{
    // 5 : 5 parameters number
    std::array<LLVMTypeRef, 5> paramTys = {
        LLVMInt64Type(), LLVMInt32Type(), LLVMInt1Type(), LLVMInt64Type(), LLVMInt64Type(),
    };
    return LLVMFunctionType(LLVMInt64Type(), paramTys.data(), 5, 0);  // 5 : 5 parameters number
}

LLVM_STUB_GETFUCTION_TYPE(AddElementInternal)
{
    // 5 : 5 parameters number
    std::array<LLVMTypeRef, 5> paramTys = {
        LLVMInt64Type(), LLVMInt64Type(), LLVMInt32Type(), LLVMInt64Type(), LLVMInt32Type(),
    };
    return LLVMFunctionType(LLVMInt1Type(), paramTys.data(), 5, 0);  // 5 : 5 parameters number
}

LLVM_STUB_GETFUCTION_TYPE(CallSetter)
{
    // 5 : 5 parameters number
    std::array<LLVMTypeRef, 5> paramTys = {
        LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type(), LLVMInt1Type(),
    };
    return LLVMFunctionType(LLVMInt1Type(), paramTys.data(), 5, 0);  // 5 : 5 parameters number
}

LLVM_STUB_GETFUCTION_TYPE(ThrowTypeError)
{
    // 2 : 2 parameters number
    std::array<LLVMTypeRef, 2> paramTys = {
        LLVMInt64Type(),
        LLVMInt32Type(),
    };
    return LLVMFunctionType(LLVMVoidType(), paramTys.data(), 2, 0);  // 2 : 2 parameters number
}

LLVM_STUB_GETFUCTION_TYPE(JSProxySetProperty)
{
    // 6 : 6 parameters number
    std::array<LLVMTypeRef, 6> paramTys = {
        LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type(), LLVMInt64Type(), LLVMInt1Type(),
    };
    return LLVMFunctionType(LLVMInt1Type(), paramTys.data(), 6, 0);  // 6 : 6 parameters number
}

LLVM_STUB_GETFUCTION_TYPE(GetHash32)
{
    // 2 : 2 parameters number
    std::array<LLVMTypeRef, 2> paramTys = {LLVMInt64Type(), LLVMInt32Type()};
    return LLVMFunctionType(LLVMInt32Type(), paramTys.data(), 2, 0);  // 2 : 2 parameters number
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
    FAST_RUNTIME_STUB_LIST(INITIALIZE_CALL_STUB)
#undef INITIALIZE_CALL_STUB
#undef DEF_CALL_STUB

// Intialize Stubs Function
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEF_CALL_STUB(name) NAME_##name
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INITIALIZE_CALL_STUB(name, argcounts) \
    llvm_fuction_type_[DEF_CALL_STUB(name)] = LLVM##name##Stub##Type::InitializeFunctionType();
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

void *LLVMStubsImplement::GetRunTimeLLVMType(int index)
{
    ASSERT(index < CALL_STUB_MAXCOUNT && index >= 0);
    return reinterpret_cast<void *>(llvm_fuction_type_[index]);
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
    std::array<MachineType, 5> params = {  // 5 : 5 input parameters
        MachineType::UINT64_TYPE,
        MachineType::UINT32_TYPE,
        MachineType::BOOL_TYPE,
        MachineType::UINT64_TYPE,
        MachineType::UINT64_TYPE,
    };
    descriptor->SetParameters(params.data());
}

CALL_STUB_INIT_DESCRIPTOR(AddElementInternal)
{
    // 5 : 5 input parameters
    static StubInterfaceDescriptor addElementInternal(0, 5, DEFAULT_ORDER, BOOL_TYPE);
    *descriptor = addElementInternal;
    std::array<MachineType, 5> params = {  // 5 : 5 input parameters
        MachineType::UINT64_TYPE,
        MachineType::UINT64_TYPE,
        MachineType::UINT32_TYPE,
        MachineType::UINT64_TYPE,
        MachineType::UINT32_TYPE,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubInterfaceDescriptor::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(CallSetter)
{
    // 5 : 5 input parameters
    static StubInterfaceDescriptor callSetter(0, 5, DEFAULT_ORDER, NONE_TYPE);
    *descriptor = callSetter;
    std::array<MachineType, 5> params = { // 5 : 5 input parameters
        MachineType::UINT64_TYPE,
        MachineType::UINT64_TYPE,
        MachineType::UINT64_TYPE,
        MachineType::UINT64_TYPE,
        MachineType::BOOL_TYPE,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubInterfaceDescriptor::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(ThrowTypeError)
{
    // 2 : 2 input parameters
    static StubInterfaceDescriptor throwTypeError(0, 2, DEFAULT_ORDER, NONE_TYPE);
    *descriptor = throwTypeError;
    std::array<MachineType, 2> params = {  // 2 : 2 input parameters
        MachineType::UINT64_TYPE,
        MachineType::UINT32_TYPE,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubInterfaceDescriptor::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(JSProxySetProperty)
{
    // 6 : 6 input parameters
    static StubInterfaceDescriptor jsproxySetproperty(0, 6, DEFAULT_ORDER, BOOL_TYPE);
    *descriptor = jsproxySetproperty;
    std::array<MachineType, 6> params = {  // 6 : 6 input parameters
        MachineType::UINT64_TYPE,
        MachineType::UINT64_TYPE,
        MachineType::UINT64_TYPE,
        MachineType::UINT64_TYPE,
        MachineType::UINT64_TYPE,
        MachineType::BOOL_TYPE,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubInterfaceDescriptor::RUNTIME_STUB);
}

CALL_STUB_INIT_DESCRIPTOR(GetHash32)
{
    // 2 : 2 input parameters
    static StubInterfaceDescriptor getHash32(0, 2, DEFAULT_ORDER, UINT32_TYPE);
    *descriptor = getHash32;
    std::array<MachineType, 2> params = {  // 2 : 2 input parameters
        MachineType::POINTER_TYPE,
        MachineType::UINT32_TYPE,
    };
    descriptor->SetParameters(params.data());
    descriptor->SetStubKind(StubInterfaceDescriptor::RUNTIME_STUB);
}

void FastStubs::InitializeStubDescriptors()
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
}  // namespace kungfu
