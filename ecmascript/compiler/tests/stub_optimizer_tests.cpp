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

#include <cstdint>
#include <filesystem>
#include <unistd.h>

#include "gtest/gtest.h"
#include "ecmascript/compiler/fastpath_optimizer.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/compiler/llvm_mcjit_compiler.h"
#include "ecmascript/compiler/scheduler.h"
#include "ecmascript/compiler/stub_interface.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/js_array.h"
#include "ecmascript/message_string.h"
#include "ecmascript/tests/test_helper.h"

#include "llvm/IR/Instructions.h"
#include "llvm/Support/Host.h"

namespace panda::test {
using namespace panda::coretypes;
using namespace panda::ecmascript;
using namespace kungfu;

class StubOptimizerTest : public testing::Test {
public:
    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    PandaVM *instance{nullptr};
    EcmaHandleScope *scope{nullptr};
    JSThread *thread{nullptr};
};

HWTEST_F_L0(StubOptimizerTest, FastLoadElement)
{
    auto *factory = JSThread::Cast(thread)->GetEcmaVM()->GetFactory();
    LLVMModuleRef module = LLVMModuleCreateWithName("simple_module");
    LLVMSetTarget(module, "x86_64-unknown-linux-gnu");
    LLVMTypeRef paramTys[] = {
        LLVMPointerType(LLVMInt64Type(), 0),
        LLVMInt32Type(),
    };
    LLVMValueRef function =
        // 2 : parameter number
        LLVMAddFunction(module, "FastLoadElement", LLVMFunctionType(LLVMInt64Type(), paramTys, 2, 0));
    LLVMDumpModule(module);
    Environment fastEnv("FastLoadElement", 2);  // 2 : parameter count
    FastArrayLoadElementOptimizer optimizer(&fastEnv);
    optimizer.GenerateCircuit();
    auto netOfGates = fastEnv.GetCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    for (size_t bbIdx = 0; bbIdx < cfg.size(); bbIdx++) {
        std::cout << (netOfGates.GetOpCode(cfg[bbIdx].front()).IsCFGMerge() ? "MERGE_" : "BB_") << bbIdx << ":"
                  << std::endl;
        for (size_t instIdx = cfg[bbIdx].size(); instIdx > 0; instIdx--) {
            netOfGates.Print(cfg[bbIdx][instIdx - 1]);
        }
    }
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, module, function);
    llvmBuilder.Build();
    LLVMMCJITCompiler compiler(module);
    compiler.Run();
    auto engine = compiler.GetEngine();
    /* exec function */
    auto fn = reinterpret_cast<uint64_t (*)(JSArray *, int)>(LLVMGetPointerToGlobal(engine, function));
    // 5 : 5 means that there are 5 cases in total.
    JSHandle<TaggedArray> values(factory->NewTaggedArray(5));
    // 5 : 5 means that there are 5 cases in total.
    for (int i = 0; i < 5; i++) {
        values->Set(thread, i, JSTaggedValue(i));
    }
    JSHandle<JSObject> array(JSArray::CreateArrayFromList(thread, values));
    JSArray *arr = array.GetObject<JSArray>();
    auto valValid = fn(arr, 1);
    EXPECT_EQ(valValid, 0xffff000000000001);
    // 6 : size of array
    auto valUndefine = fn(arr, 6);
    EXPECT_EQ(valUndefine, 0xa);
    std::cout << "valValid = " << std::hex << valValid << std::endl;
    std::cout << "valUndefine = " << std::hex << valUndefine << std::endl;
}

class StubPhiOptimizer : public StubOptimizer {
public:
    explicit StubPhiOptimizer(Environment *env) : StubOptimizer(env) {}
    ~StubPhiOptimizer() = default;
    NO_MOVE_SEMANTIC(StubPhiOptimizer);
    NO_COPY_SEMANTIC(StubPhiOptimizer);
    void GenerateCircuit() override
    {
        auto env = GetEnvironment();
        DEFVARIABLE(z, MachineType::INT32_TYPE, GetInteger32Constant(0));
        DEFVARIABLE(x, MachineType::INT32_TYPE, Int32Argument(0));
        StubOptimizerLabel ifTrue(env);
        StubOptimizerLabel ifFalse(env);
        StubOptimizerLabel next(env);

        Branch(Word32Equal(*x, GetInteger32Constant(10)), &ifTrue, &ifFalse);  // 10 : size of entry
        Bind(&ifTrue);
        z = Int32Add(*x, GetInteger32Constant(10));  // 10 : size of entry
        Jump(&next);
        Bind(&ifFalse);                               // else
        z = Int32Add(*x, GetInteger32Constant(100));  // 100 : size of entry
        Jump(&next);
        Bind(&next);
        Return(*z);
    }
};

HWTEST_F_L0(StubOptimizerTest, PhiGateTest)
{
    std::cout << "---------------------PhiGateTest-----------------------------------------------------" << std::endl;
    LLVMModuleRef module = LLVMModuleCreateWithName("simple_module");
    LLVMSetTarget(module, "x86_64-unknown-linux-gnu");
    LLVMTypeRef paramTys[] = {
        LLVMInt32Type(),
    };
    LLVMValueRef function = LLVMAddFunction(module, "PhiGateTest", LLVMFunctionType(LLVMInt32Type(), paramTys, 1, 0));
    Environment env("PhiGateTest", 1);
    StubPhiOptimizer optimizer(&env);
    optimizer.GenerateCircuit();
    auto netOfGates = env.GetCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    for (size_t bbIdx = 0; bbIdx < cfg.size(); bbIdx++) {
        std::cout << (netOfGates.GetOpCode(cfg[bbIdx].front()).IsCFGMerge() ? "MERGE_" : "BB_") << bbIdx << ":"
                  << std::endl;
        for (size_t instIdx = cfg[bbIdx].size(); instIdx > 0; instIdx--) {
            netOfGates.Print(cfg[bbIdx][instIdx - 1]);
        }
    }
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, module, function);
    llvmBuilder.Build();
    LLVMMCJITCompiler compiler(module);
    compiler.Run();
    auto engine = compiler.GetEngine();
    /* exec function */
    auto fn = reinterpret_cast<int (*)(int)>(LLVMGetPointerToGlobal(engine, function));
    auto val = fn(3);  // 3 : size of array
    auto val2 = fn(0);
    std::cout << "val = " << std::dec << val << std::endl;
    std::cout << "val2 = " << std::dec << val2 << std::endl;
    std::cout << "+++++++++++++++++++++PhiGateTest+++++++++++++++++++++++++++++++++++++++++++++++++++++" << std::endl;
}

class StubCallPhiOptimizer : public StubOptimizer {
public:
    explicit StubCallPhiOptimizer(Environment *env)
        : StubOptimizer(env), phi_descriptor_(0, 1, DEFAULT_ORDER, MachineType::INT32_TYPE)
    {
        std::array<MachineType, 1> *params = new std::array<MachineType, 1>();
        (*params)[0] = MachineType::INT32_TYPE;
        phi_descriptor_.SetParameters(params->data());
    }
    ~StubCallPhiOptimizer() = default;
    NO_MOVE_SEMANTIC(StubCallPhiOptimizer);
    NO_COPY_SEMANTIC(StubCallPhiOptimizer);
    void GenerateCircuit() override
    {
        DEFVARIABLE(x, MachineType::INT32_TYPE, Int32Argument(0));
        x = Int32Add(*x, GetInteger32Constant(1));
        AddrShift callFoo = CallStub(&phi_descriptor_, GetWord64Constant(0), {*x});
        Return(Int32Add(callFoo, GetInteger32Constant(1)));
    }

private:
    StubInterfaceDescriptor phi_descriptor_;
};

HWTEST_F_L0(StubOptimizerTest, CallPhiGateTest)
{
    LLVMModuleRef module = static_cast<LLVMModuleRef>(FastStubs::GetInstance().GetModule());
    LLVMTypeRef fooParamTys[] = {
        LLVMInt32Type(),
    };
    LLVMValueRef function = LLVMAddFunction(module, "PhiTest", LLVMFunctionType(LLVMInt32Type(), fooParamTys, 1, 0));
    LLVMTypeRef barParamTys[] = {
        LLVMInt32Type(),
    };
    FastStubs::GetInstance().SetFastStub(0, reinterpret_cast<void *>(function));
    LLVMValueRef barfunction =
        LLVMAddFunction(module, "CallPhiGateTest_bar", LLVMFunctionType(LLVMInt32Type(), barParamTys, 1, 0));
    Environment env("PhiTest", 1);
    StubPhiOptimizer optimizer(&env);
    optimizer.GenerateCircuit();
    auto netOfGates = env.GetCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, module, function);
    llvmBuilder.Build();
    Environment barEnv("CallPhiGateTest_bar", 1);
    StubCallPhiOptimizer callphiOptimizer(&barEnv);
    callphiOptimizer.GenerateCircuit();
    auto callNetOfGates = barEnv.GetCircuit();
    callNetOfGates.PrintAllGates();
    auto callCfg = Scheduler::Run(&callNetOfGates);
    for (size_t bbIdx = 0; bbIdx < callCfg.size(); bbIdx++) {
        std::cout << (callNetOfGates.GetOpCode(callCfg[bbIdx].front()).IsCFGMerge() ? "MERGE_" : "BB_") << bbIdx << ":"
                  << std::endl;
        for (size_t instIdx = callCfg[bbIdx].size(); instIdx > 0; instIdx--) {
            callNetOfGates.Print(callCfg[bbIdx][instIdx - 1]);
        }
    }
    LLVMIRBuilder barllvmBuilder(&callCfg, &callNetOfGates, module, barfunction);
    barllvmBuilder.Build();
    LLVMMCJITCompiler compiler(module);
    compiler.Run();
    auto engine = compiler.GetEngine();
    auto *phiTestPointer =
        reinterpret_cast<int (*)(int)>(reinterpret_cast<uintptr_t>(LLVMGetPointerToGlobal(engine, function)));
    LLVMAddGlobalMapping(engine, function, reinterpret_cast<void *>(phiTestPointer));
    auto *barPointerbar =
        reinterpret_cast<int (*)(int)>(reinterpret_cast<uintptr_t>(LLVMGetPointerToGlobal(engine, barfunction)));
    std::cout << std::hex << "phiTestPointer:" << reinterpret_cast<uintptr_t>(phiTestPointer)
              << " barPointerbar:" << reinterpret_cast<uintptr_t>(barPointerbar) << std::endl;
    compiler.Disassemble();
    int result = barPointerbar(9);
    EXPECT_EQ(result, 21);
    result = barPointerbar(10);
    EXPECT_EQ(result, 112);
}

class LoopOptimizer : public StubOptimizer {
public:
    explicit LoopOptimizer(Environment *env) : StubOptimizer(env) {}
    ~LoopOptimizer() = default;
    NO_MOVE_SEMANTIC(LoopOptimizer);
    NO_COPY_SEMANTIC(LoopOptimizer);
    void GenerateCircuit() override
    {
        auto env = GetEnvironment();
        DEFVARIABLE(z, MachineType::INT32_TYPE, GetInteger32Constant(0));
        DEFVARIABLE(y, MachineType::INT32_TYPE, Int32Argument(0));
        Label loopHead(env);
        Label loopEnd(env);
        Label afterLoop(env);
        Branch(Int32LessThan(*y, GetInteger32Constant(10)), &loopHead, &afterLoop);  // 10 : size of entry
        LoopBegin(&loopHead);
        Label ifTrue(env);
        Label ifFalse(env);
        Label next(env);
        Branch(Word32Equal(Int32Argument(0), GetInteger32Constant(9)), &ifTrue, &ifFalse);  // 9 : size of entry
        Bind(&ifTrue);
        z = Int32Add(*y, GetInteger32Constant(10));  // 10 : size of entry
        y = Int32Add(*z, GetInteger32Constant(1));
        Jump(&next);
        Bind(&ifFalse);
        z = Int32Add(*y, GetInteger32Constant(100));  // 100 : size of entry
        Jump(&next);
        Bind(&next);
        y = Int32Add(*y, GetInteger32Constant(1));
        Branch(Int32LessThan(*y, GetInteger32Constant(10)), &loopEnd, &afterLoop);  // 10 : size of entry
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&afterLoop);
        Return(*z);
    }
};

HWTEST_F_L0(StubOptimizerTest, LoopTest)
{
    LLVMModuleRef module = LLVMModuleCreateWithName("simple_module");
    LLVMSetTarget(module, "x86_64-unknown-linux-gnu");
    LLVMTypeRef paramTys[] = {
        LLVMInt32Type(),
    };
    LLVMValueRef function = LLVMAddFunction(module, "LoopTest", LLVMFunctionType(LLVMInt32Type(), paramTys, 1, 0));
    Environment env("loop", 1);
    LoopOptimizer optimizer(&env);
    optimizer.GenerateCircuit();
    auto netOfGates = env.GetCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    for (size_t bbIdx = 0; bbIdx < cfg.size(); bbIdx++) {
        std::cout << (netOfGates.GetOpCode(cfg[bbIdx].front()).IsCFGMerge() ? "MERGE_" : "BB_") << bbIdx << ":"
                  << std::endl;
        for (size_t inst_idx = cfg[bbIdx].size(); inst_idx > 0; inst_idx--) {
            netOfGates.Print(cfg[bbIdx][inst_idx - 1]);
        }
    }
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, module, function);
    llvmBuilder.Build();
    LLVMMCJITCompiler compiler(module);
    compiler.Run();
    auto engine = compiler.GetEngine();

    /* exec function */
    auto fn = reinterpret_cast<int (*)(int)>(LLVMGetPointerToGlobal(engine, function));
    auto resValid = fn(1);
    auto resValid2 = fn(9);    // 9 : size of array
    auto resInvalid = fn(11);  // 11 : size of array
    std::cout << "res for loop(1) = " << std::dec << resValid << std::endl;
    std::cout << "res for loop(9) = " << std::dec << resValid2 << std::endl;
    std::cout << "res for loop(11) = " << std::dec << resInvalid << std::endl;
}

class LoopOptimizer1 : public StubOptimizer {
public:
    explicit LoopOptimizer1(Environment *env) : StubOptimizer(env) {}
    ~LoopOptimizer1() = default;
    NO_MOVE_SEMANTIC(LoopOptimizer1);
    NO_COPY_SEMANTIC(LoopOptimizer1);
    void GenerateCircuit() override
    {
        auto env = GetEnvironment();
        DEFVARIABLE(y, MachineType::INT32_TYPE, Int32Argument(0));
        DEFVARIABLE(x, MachineType::INT32_TYPE, Int32Argument(0));
        DEFVARIABLE(z, MachineType::INT32_TYPE, Int32Argument(0));
        Label loopHead(env);
        Label loopEnd(env);
        Label afterLoop(env);
        Branch(Int32LessThan(*y, GetInteger32Constant(10)), &loopHead, &afterLoop);  // 10 : size of entry
        LoopBegin(&loopHead);
        x = Int32Add(*z, GetInteger32Constant(3));  // 3 : size of entry
        Label ifTrue(env);
        Label next(env);
        Branch(Word32Equal(*x, GetInteger32Constant(9)), &ifTrue, &next);  // 9 : size of entry
        Bind(&ifTrue);
        y = Int32Add(*z, *x);
        Jump(&next);
        Bind(&next);
        y = Int32Add(*y, GetInteger32Constant(1));
        Branch(Int32LessThan(*y, GetInteger32Constant(10)), &loopEnd, &afterLoop);  // 10 : size of entry
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&afterLoop);
        z = *y;
        Return(*z);
    }
};

HWTEST_F_L0(StubOptimizerTest, LoopTest1)
{
    LLVMModuleRef module = LLVMModuleCreateWithName("simple_module");
    LLVMSetTarget(module, "x86_64-unknown-linux-gnu");
    LLVMTypeRef paramTys[] = {
        LLVMInt32Type(),
    };
    LLVMValueRef function = LLVMAddFunction(module, "LoopTest1", LLVMFunctionType(LLVMInt32Type(), paramTys, 1, 0));
    Environment env("loop1", 1);
    LoopOptimizer1 optimizer(&env);
    optimizer.GenerateCircuit();
    auto netOfGates = env.GetCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    for (size_t bbIdx = 0; bbIdx < cfg.size(); bbIdx++) {
        std::cout << (netOfGates.GetOpCode(cfg[bbIdx].front()).IsCFGMerge() ? "MERGE_" : "BB_") << bbIdx << ":"
                  << std::endl;
        for (size_t instIdx = cfg[bbIdx].size(); instIdx > 0; instIdx--)
            netOfGates.Print(cfg[bbIdx][instIdx - 1]);
    }
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, module, function);
    llvmBuilder.Build();
    LLVMMCJITCompiler compiler(module);
    compiler.Run();
    auto engine = compiler.GetEngine();
    /* exec function */
    auto fn = reinterpret_cast<int (*)(int)>(LLVMGetPointerToGlobal(engine, function));
    auto resValid = fn(1);
    auto resValid2 = fn(9);    // 9 : size of array
    auto resInvalid = fn(11);  // 11 : size of array
    std::cout << "res for loop1(1) = " << std::dec << resValid << std::endl;
    std::cout << "res for loop1(9) = " << std::dec << resValid2 << std::endl;
    std::cout << "res for loop1(11) = " << std::dec << resInvalid << std::endl;
}

class FastAddOptimizer : public StubOptimizer {
public:
    explicit FastAddOptimizer(Environment *env) : StubOptimizer(env) {}
    ~FastAddOptimizer() = default;
    NO_MOVE_SEMANTIC(FastAddOptimizer);
    NO_COPY_SEMANTIC(FastAddOptimizer);
    void GenerateCircuit() override
    {
        auto env = GetEnvironment();
        AddrShift x = Int64Argument(0);
        AddrShift y = Int64Argument(1);
        DEFVARIABLE(doubleX, MachineType::FLOAT64_TYPE, 0);
        DEFVARIABLE(doubleY, MachineType::FLOAT64_TYPE, 0);
        StubOptimizerLabel ifTrue1(env);
        StubOptimizerLabel ifTrue2(env);
        StubOptimizerLabel ifFalse1(env);
        StubOptimizerLabel ifFalse2(env);
        StubOptimizerLabel ifTrue3(env);
        StubOptimizerLabel ifTrue4(env);
        StubOptimizerLabel ifTrue5(env);
        StubOptimizerLabel ifFalse3(env);
        StubOptimizerLabel ifFalse4(env);
        StubOptimizerLabel ifFalse5(env);
        StubOptimizerLabel next1(env);
        StubOptimizerLabel next2(env);
        StubOptimizerLabel next3(env);
        // if x is number
        Branch(TaggedIsNumber(x), &ifTrue1, &ifFalse1);
        Bind(&ifTrue1);
        // if y is number
        Branch(TaggedIsNumber(y), &ifTrue2, &ifFalse2);
        Bind(&ifTrue2);
        Branch(TaggedIsInt(x), &ifTrue3, &ifFalse3);
        Bind(&ifTrue3);
        AddrShift intX = TaggedCastToInt32(x);
        Branch(TaggedIsInt(y), &ifTrue4, &ifFalse4);
        Bind(&ifTrue4);
        AddrShift intY = TaggedCastToInt32(y);
        intX = Int32Add(intX, intY);
        Jump(&next3);
        Bind(&ifFalse4);
        doubleY = TaggedCastToDouble(y);
        doubleX = CastInt32ToFloat64(intX);
        Jump(&next2);
        Bind(&ifFalse3);
        doubleX = TaggedCastToDouble(x);
        Branch(TaggedIsInt(y), &ifTrue5, &ifFalse5);
        Bind(&ifTrue5);
        intY = TaggedCastToInt32(y);
        doubleY = CastInt32ToFloat64(intY);
        Jump(&next2);
        Bind(&ifFalse5);
        doubleY = TaggedCastToDouble(y);
        Jump(&next2);
        Bind(&ifFalse2);
        Jump(&next1);
        Bind(&ifFalse1);
        Jump(&next1);
        Bind(&next1);
        Return(GetHoleConstant());
        Bind(&next2);
        doubleX = DoubleAdd(*doubleX, *doubleY);
        Return(DoubleBuildTagged(*doubleX));
        Bind(&next3);
        Return(IntBuildTagged(intX));
    }
};

HWTEST_F_L0(StubOptimizerTest, FastAddTest)
{
    LLVMModuleRef module = LLVMModuleCreateWithName("fast_add_module");
    LLVMSetTarget(module, "x86_64-unknown-linux-gnu");
    LLVMTypeRef paramTys[] = {
        LLVMInt64Type(),
        LLVMInt64Type(),
    };
    LLVMValueRef function = LLVMAddFunction(module, "FastAddTest", LLVMFunctionType(LLVMInt64Type(), paramTys, 2, 0));
    LLVMDumpModule(module);
    Environment env("FastAdd", 2);  // 2 : parameter count
    FastAddOptimizer optimizer(&env);
    optimizer.GenerateCircuit();
    auto netOfGates = env.GetCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    for (size_t bbIdx = 0; bbIdx < cfg.size(); bbIdx++) {
        std::cout << (netOfGates.GetOpCode(cfg[bbIdx].front()).IsCFGMerge() ? "MERGE_" : "BB_") << bbIdx << ":"
                  << std::endl;
        for (size_t instIdx = cfg[bbIdx].size(); instIdx > 0; instIdx--) {
            netOfGates.Print(cfg[bbIdx][instIdx - 1]);
        }
    }
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, module, function);
    llvmBuilder.Build();
    LLVMMCJITCompiler compiler(module);
    compiler.Run();
    auto engine = compiler.GetEngine();
    /* exec function */
    auto fn = reinterpret_cast<JSTaggedValue (*)(int64_t, int64_t)>(LLVMGetPointerToGlobal(engine, function));
    auto resValid = fn(JSTaggedValue(1).GetRawData(), JSTaggedValue(1).GetRawData());
    auto resValid2 = fn(JSTaggedValue(2).GetRawData(), JSTaggedValue(2).GetRawData());     // 2 : test case
    auto resInvalid = fn(JSTaggedValue(11).GetRawData(), JSTaggedValue(11).GetRawData());  // 11 : test case
    std::cout << "res for FastAdd(1, 1) = " << std::dec << resValid.GetNumber() << std::endl;
    std::cout << "res for FastAdd(2, 2) = " << std::dec << resValid2.GetNumber() << std::endl;
    std::cout << "res for FastAdd(11, 11) = " << std::dec << resInvalid.GetNumber() << std::endl;
}

class FastSubOptimizer : public StubOptimizer {
public:
    explicit FastSubOptimizer(Environment *env) : StubOptimizer(env) {}
    ~FastSubOptimizer() = default;
    NO_MOVE_SEMANTIC(FastSubOptimizer);
    NO_COPY_SEMANTIC(FastSubOptimizer);
    void GenerateCircuit() override
    {
        auto env = GetEnvironment();
        AddrShift x = Int64Argument(0);
        AddrShift y = Int64Argument(1);
        DEFVARIABLE(doubleX, MachineType::FLOAT64_TYPE, 0);
        DEFVARIABLE(doubleY, MachineType::FLOAT64_TYPE, 0);
        StubOptimizerLabel ifTrue1(env);
        StubOptimizerLabel ifTrue2(env);
        StubOptimizerLabel ifFalse1(env);
        StubOptimizerLabel ifFalse2(env);
        StubOptimizerLabel ifTrue3(env);
        StubOptimizerLabel ifTrue4(env);
        StubOptimizerLabel ifTrue5(env);
        StubOptimizerLabel ifFalse3(env);
        StubOptimizerLabel ifFalse4(env);
        StubOptimizerLabel ifFalse5(env);
        StubOptimizerLabel next1(env);
        StubOptimizerLabel next2(env);
        StubOptimizerLabel next3(env);
        // if x is number
        Branch(TaggedIsNumber(x), &ifTrue1, &ifFalse1);
        Bind(&ifTrue1);
        // if y is number
        Branch(TaggedIsNumber(y), &ifTrue2, &ifFalse2);
        Bind(&ifTrue2);
        Branch(TaggedIsInt(x), &ifTrue3, &ifFalse3);
        Bind(&ifTrue3);
        AddrShift intX = TaggedCastToInt32(x);
        Branch(TaggedIsInt(y), &ifTrue4, &ifFalse4);
        Bind(&ifTrue4);
        AddrShift intY = TaggedCastToInt32(y);
        intX = Int32Sub(intX, intY);
        Jump(&next3);
        Bind(&ifFalse4);
        doubleY = TaggedCastToDouble(y);
        doubleX = CastInt32ToFloat64(intX);
        Jump(&next2);
        Bind(&ifFalse3);
        doubleX = TaggedCastToDouble(x);
        Branch(TaggedIsInt(y), &ifTrue5, &ifFalse5);
        Bind(&ifTrue5);
        intY = TaggedCastToInt32(y);
        doubleY = CastInt32ToFloat64(intY);
        Jump(&next2);
        Bind(&ifFalse5);
        doubleY = TaggedCastToDouble(y);
        Jump(&next2);
        Bind(&ifFalse2);
        Jump(&next1);
        Bind(&ifFalse1);
        Jump(&next1);
        Bind(&next1);
        Return(GetHoleConstant());
        Bind(&next2);
        doubleX = DoubleSub(*doubleX, *doubleY);
        Return(DoubleBuildTagged(*doubleX));
        Bind(&next3);
        Return(IntBuildTagged(intX));
    }
};

HWTEST_F_L0(StubOptimizerTest, FastSubTest)
{
    LLVMModuleRef module = LLVMModuleCreateWithName("fast_sub_module");
    LLVMSetTarget(module, "x86_64-unknown-linux-gnu");
    LLVMTypeRef paramTys[] = {
        LLVMInt64Type(),
        LLVMInt64Type(),
    };
    LLVMValueRef function = LLVMAddFunction(module, "FastSubTest", LLVMFunctionType(LLVMInt64Type(), paramTys, 2, 0));
    LLVMDumpModule(module);
    Environment env("FastSub", 2);  // 2 : parameter
    FastSubOptimizer optimizer(&env);
    optimizer.GenerateCircuit();
    auto netOfGates = env.GetCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    for (size_t bbIdx = 0; bbIdx < cfg.size(); bbIdx++) {
        std::cout << (netOfGates.GetOpCode(cfg[bbIdx].front()).IsCFGMerge() ? "MERGE_" : "BB_") << bbIdx << ":"
                  << std::endl;
        for (size_t instIdx = cfg[bbIdx].size(); instIdx > 0; instIdx--) {
            netOfGates.Print(cfg[bbIdx][instIdx - 1]);
        }
    }
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, module, function);
    llvmBuilder.Build();
    LLVMMCJITCompiler compiler(module);
    compiler.Run();
    auto engine = compiler.GetEngine();
    /* exec function */
    auto fn = reinterpret_cast<JSTaggedValue (*)(int64_t, int64_t)>(LLVMGetPointerToGlobal(engine, function));
    auto resA = fn(JSTaggedValue(2).GetRawData(), JSTaggedValue(1).GetRawData());    // 2 : test case
    auto resB = fn(JSTaggedValue(7).GetRawData(), JSTaggedValue(2).GetRawData());    // 7, 2 : test cases
    auto resC = fn(JSTaggedValue(11).GetRawData(), JSTaggedValue(11).GetRawData());  // 11 : test case
    std::cout << "res for FastSub(2, 1) = " << std::dec << resA.GetNumber() << std::endl;
    std::cout << "res for FastSub(7, 2) = " << std::dec << resB.GetNumber() << std::endl;
    std::cout << "res for FastSub(11, 11) = " << std::dec << resC.GetNumber() << std::endl;
}

class FastMulOptimizer : public StubOptimizer {
public:
    explicit FastMulOptimizer(Environment *env) : StubOptimizer(env) {}
    ~FastMulOptimizer() = default;
    NO_MOVE_SEMANTIC(FastMulOptimizer);
    NO_COPY_SEMANTIC(FastMulOptimizer);
    void GenerateCircuit() override
    {
        auto env = GetEnvironment();
        AddrShift x = Int64Argument(0);
        AddrShift y = Int64Argument(1);
        DEFVARIABLE(doubleX, MachineType::FLOAT64_TYPE, 0);
        DEFVARIABLE(doubleY, MachineType::FLOAT64_TYPE, 0);
        StubOptimizerLabel ifTrue1(env);
        StubOptimizerLabel ifTrue2(env);
        StubOptimizerLabel ifFalse1(env);
        StubOptimizerLabel ifFalse2(env);
        StubOptimizerLabel ifTrue3(env);
        StubOptimizerLabel ifTrue4(env);
        StubOptimizerLabel ifTrue5(env);
        StubOptimizerLabel ifFalse3(env);
        StubOptimizerLabel ifFalse4(env);
        StubOptimizerLabel ifFalse5(env);
        StubOptimizerLabel next1(env);
        StubOptimizerLabel next2(env);
        StubOptimizerLabel next3(env);
        // if x is number
        Branch(TaggedIsNumber(x), &ifTrue1, &ifFalse1);
        Bind(&ifTrue1);
        // if y is number
        Branch(TaggedIsNumber(y), &ifTrue2, &ifFalse2);
        Bind(&ifTrue2);
        Branch(TaggedIsInt(x), &ifTrue3, &ifFalse3);
        Bind(&ifTrue3);
        AddrShift intX = TaggedCastToInt32(x);
        Branch(TaggedIsInt(y), &ifTrue4, &ifFalse4);
        Bind(&ifTrue4);
        AddrShift intY = TaggedCastToInt32(y);
        intX = Int32Mul(intX, intY);
        Jump(&next3);
        Bind(&ifFalse4);
        doubleY = TaggedCastToDouble(y);
        doubleX = CastInt32ToFloat64(intX);
        Jump(&next2);
        Bind(&ifFalse3);
        doubleX = TaggedCastToDouble(x);
        Branch(TaggedIsInt(y), &ifTrue5, &ifFalse5);
        Bind(&ifTrue5);
        intY = TaggedCastToInt32(y);
        doubleY = CastInt32ToFloat64(intY);
        Jump(&next2);
        Bind(&ifFalse5);
        doubleY = TaggedCastToDouble(y);
        Jump(&next2);
        Bind(&ifFalse2);
        Jump(&next1);
        Bind(&ifFalse1);
        Jump(&next1);
        Bind(&next1);
        Return(GetHoleConstant());
        Bind(&next2);
        doubleX = DoubleMul(*doubleX, *doubleY);
        Return(DoubleBuildTagged(*doubleX));
        Bind(&next3);
        Return(IntBuildTagged(intX));
    }
};

HWTEST_F_L0(StubOptimizerTest, FastMulTest)
{
    LLVMModuleRef module = LLVMModuleCreateWithName("fast_mul_module");
    LLVMSetTarget(module, "x86_64-unknown-linux-gnu");
    LLVMTypeRef paramTys[] = {
        LLVMInt64Type(),
        LLVMInt64Type(),
    };
    LLVMValueRef function = LLVMAddFunction(module, "FastMulTest", LLVMFunctionType(LLVMInt64Type(), paramTys, 2, 0));
    LLVMDumpModule(module);
    Environment env("FastMul", 2);  // 2 : parameter count
    FastMulOptimizer optimizer(&env);
    optimizer.GenerateCircuit();
    auto netOfGates = env.GetCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    for (size_t bbIdx = 0; bbIdx < cfg.size(); bbIdx++) {
        std::cout << (netOfGates.GetOpCode(cfg[bbIdx].front()).IsCFGMerge() ? "MERGE_" : "BB_") << bbIdx << ":"
                  << std::endl;
        for (size_t instIdx = cfg[bbIdx].size(); instIdx > 0; instIdx--) {
            netOfGates.Print(cfg[bbIdx][instIdx - 1]);
        }
    }
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, module, function);
    llvmBuilder.Build();
    LLVMMCJITCompiler compiler(module);
    compiler.Run();
    auto engine = compiler.GetEngine();

    /* exec function */
    auto fn = reinterpret_cast<JSTaggedValue (*)(int64_t, int64_t)>(LLVMGetPointerToGlobal(engine, function));
    auto resA = fn(JSTaggedValue(-2).GetRawData(), JSTaggedValue(1).GetRawData());   // -2 : test case
    auto resB = fn(JSTaggedValue(-7).GetRawData(), JSTaggedValue(-2).GetRawData());  // -7, -2 : test case
    auto resC = fn(JSTaggedValue(11).GetRawData(), JSTaggedValue(11).GetRawData());  // 11 : test case
    std::cout << "res for FastMul(-2, 1) = " << std::dec << resA.GetNumber() << std::endl;
    std::cout << "res for FastMul(-7, -2) = " << std::dec << resB.GetNumber() << std::endl;
    std::cout << "res for FastMul(11, 11) = " << std::dec << resC.GetNumber() << std::endl;
}

class FastDivOptimizer : public StubOptimizer {
public:
    explicit FastDivOptimizer(Environment *env) : StubOptimizer(env) {}
    ~FastDivOptimizer() = default;
    NO_MOVE_SEMANTIC(FastDivOptimizer);
    NO_COPY_SEMANTIC(FastDivOptimizer);
    void GenerateCircuit() override
    {
        auto env = GetEnvironment();
        AddrShift x = Int64Argument(0);
        AddrShift y = Int64Argument(1);
        DEFVARIABLE(doubleX, MachineType::FLOAT64_TYPE, 0);
        DEFVARIABLE(doubleY, MachineType::FLOAT64_TYPE, 0);
        StubOptimizerLabel ifTrue1(env);
        StubOptimizerLabel ifTrue2(env);
        StubOptimizerLabel ifFalse1(env);
        StubOptimizerLabel ifFalse2(env);
        StubOptimizerLabel ifTrue3(env);
        StubOptimizerLabel ifTrue4(env);
        StubOptimizerLabel ifTrue5(env);
        StubOptimizerLabel ifTrue6(env);
        StubOptimizerLabel ifFalse3(env);
        StubOptimizerLabel ifFalse4(env);
        StubOptimizerLabel ifFalse5(env);
        StubOptimizerLabel ifFalse6(env);
        StubOptimizerLabel next1(env);
        StubOptimizerLabel next2(env);
        StubOptimizerLabel next3(env);
        StubOptimizerLabel next4(env);
        Branch(TaggedIsNumber(x), &ifTrue1, &ifFalse1);
        Bind(&ifTrue1);
        // if right.IsNumber()
        Branch(TaggedIsNumber(y), &ifTrue2, &ifFalse2);
        Bind(&ifTrue2);
        Branch(TaggedIsInt(x), &ifTrue3, &ifFalse3);
        Bind(&ifTrue3);
        AddrShift intX = TaggedCastToInt32(x);
        doubleX = CastInt32ToFloat64(intX);
        Jump(&next2);
        Bind(&ifFalse3);
        doubleX = TaggedCastToDouble(x);
        Jump(&next2);
        Bind(&ifFalse2);
        Jump(&next1);
        Bind(&ifFalse1);
        Jump(&next1);
        Bind(&next1);
        Return(GetHoleConstant());
        Bind(&next2);
        Branch(TaggedIsInt(y), &ifTrue4, &ifFalse4);
        Bind(&ifTrue4);
        AddrShift intY = TaggedCastToInt32(y);
        doubleY = CastInt32ToFloat64(intY);
        Jump(&next3);
        Bind(&ifFalse4);
        doubleY = TaggedCastToDouble(y);
        Jump(&next3);
        Bind(&next3);
        Branch(DoubleEqual(*doubleY, GetDoubleConstant(0.0)), &ifTrue5, &ifFalse5);
        Bind(&ifTrue5);
        // dLeft == 0.0 || std::isnan(dLeft)
        Branch(TruncInt64ToInt1(Word64Or(SExtInt1ToInt64(DoubleEqual(*doubleX, GetDoubleConstant(0.0))),
                                         SExtInt32ToInt64(DoubleIsNAN(*doubleX)))),
               &ifTrue6, &ifFalse6);
        Bind(&ifTrue6);
        Return(DoubleBuildTagged(GetDoubleConstant(base::NAN_VALUE)));
        Bind(&ifFalse6);
        Jump(&next4);
        Bind(&next4);
        AddrShift intXTmp = CastDoubleToInt64(*doubleX);
        AddrShift intYtmp = CastDoubleToInt64(*doubleY);
        intXTmp = Word64And(Word64Xor(intXTmp, intYtmp), GetWord64Constant(base::DOUBLE_SIGN_MASK));
        intXTmp = Word64Xor(intXTmp, CastDoubleToInt64(GetDoubleConstant(base::POSITIVE_INFINITY)));
        doubleX = CastInt64ToFloat64(intXTmp);
        Return(DoubleBuildTagged(*doubleX));
        Bind(&ifFalse5);
        doubleX = DoubleDiv(*doubleX, *doubleY);
        Return(DoubleBuildTagged(*doubleX));
    }
};

HWTEST_F_L0(StubOptimizerTest, FastDivTest)
{
    LLVMModuleRef module = LLVMModuleCreateWithName("fast_div_module");
    LLVMSetTarget(module, "x86_64-unknown-linux-gnu");
    LLVMTypeRef paramTys[] = {
        LLVMInt64Type(),
        LLVMInt64Type(),
    };
    LLVMValueRef function = LLVMAddFunction(module, "FastDiv", LLVMFunctionType(LLVMInt64Type(), paramTys, 2, 0));
    Environment env("FastDiv", 2);
    FastDivOptimizer optimizer(&env);
    optimizer.GenerateCircuit();
    auto netOfGates = env.GetCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    for (size_t bbIdx = 0; bbIdx < cfg.size(); bbIdx++) {
        std::cout << (netOfGates.GetOpCode(cfg[bbIdx].front()).IsCFGMerge() ? "MERGE_" : "BB_") << bbIdx << ":"
                  << std::endl;
        for (size_t instIdx = cfg[bbIdx].size(); instIdx > 0; instIdx--) {
            netOfGates.Print(cfg[bbIdx][instIdx - 1]);
        }
    }
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, module, function);
    llvmBuilder.Build();
    LLVMMCJITCompiler compiler(module);
    compiler.Run();
    auto engine = compiler.GetEngine();
    auto fn = reinterpret_cast<JSTaggedValue (*)(int64_t, int64_t)>(LLVMGetPointerToGlobal(engine, function));
    uint64_t x1 = JSTaggedValue(50).GetRawData();
    uint64_t x2 = JSTaggedValue(25).GetRawData();
    std::cout << "x1 = " << x1 << "  x2 = " << x2 << std::endl;
    auto res = fn(x1, x2);
    std::cout << "res for FastDiv(50, 25) = " << res.GetRawData() << std::endl;
}

class FastFindOwnElementStub : public StubOptimizer {
public:
    explicit FastFindOwnElementStub(Environment *env) : StubOptimizer(env) {}
    ~FastFindOwnElementStub() = default;
    NO_MOVE_SEMANTIC(FastFindOwnElementStub);
    NO_COPY_SEMANTIC(FastFindOwnElementStub);
    void GenerateCircuit() override
    {
        auto env = GetEnvironment();
        AddrShift obj = PtrArgument(0);
        AddrShift index = Int32Argument(1);

        StubOptimizerLabel notDict(env);
        StubOptimizerLabel lessOrEqual1(env);
        StubOptimizerLabel greaterThan1(env);
        StubOptimizerLabel isHole1(env);
        StubOptimizerLabel isHole2(env);
        StubOptimizerLabel isHole3(env);
        StubOptimizerLabel notHole1(env);
        StubOptimizerLabel notHole2(env);
        StubOptimizerLabel notHole3(env);
        StubOptimizerLabel isDict(env);
        StubOptimizerLabel lessThan(env);
        StubOptimizerLabel greaterOrEqual(env);
        StubOptimizerLabel greaterThan2(env);
        StubOptimizerLabel lessOrEqual2(env);
        StubOptimizerLabel isUndef1(env);
        StubOptimizerLabel isUndef2(env);
        StubOptimizerLabel notUndef1(env);
        StubOptimizerLabel notUndef2(env);
        StubOptimizerLabel indexIsInt(env);
        StubOptimizerLabel indexIsNotInt(env);
        StubOptimizerLabel equal1(env);
        StubOptimizerLabel equal2(env);
        StubOptimizerLabel notEqual1(env);
        StubOptimizerLabel notEqual2(env);
        StubOptimizerLabel lessThanZero(env);
        StubOptimizerLabel greaterOrEqualZero(env);
        StubOptimizerLabel greaterThanLength(env);
        StubOptimizerLabel elelmentIsInt(env);
        StubOptimizerLabel elelmentIsNotInt(env);
        StubOptimizerLabel notGreaterThanLength(env);
        StubOptimizerLabel next1(env);
        AddrShift elements = Load(POINTER_TYPE, obj, GetPtrConstant(JSObject::ELEMENTS_OFFSET));

        Branch(IsDictionaryMode(elements), &isDict, &notDict);
        Bind(&notDict);
        AddrShift arrayLength = Load(UINT32_TYPE, elements, GetPtrConstant(panda::coretypes::Array::GetLengthOffset()));

        Branch(Int32LessThanOrEqual(arrayLength, index), &lessOrEqual1, &greaterThan1);
        Bind(&lessOrEqual1);
        Jump(&next1);
        Bind(&greaterThan1);
        AddrShift offset = PtrMul(ChangeInt32ToPointer(index), GetPtrConstant(JSTaggedValue::TaggedTypeSize()));
        AddrShift dataIndex = PtrAdd(offset, GetPtrConstant(panda::coretypes::Array::GetDataOffset()));
        AddrShift value = Load(TAGGED_TYPE, elements, dataIndex);

        Branch(TaggedIsHole(value), &isHole1, &notHole1);
        Bind(&isHole1);
        Jump(&next1);
        Bind(&notHole1);
        Return(value);
        Bind(&next1);
        Return(GetHoleConstant());
        // IsDictionary
        Bind(&isDict);
        offset = PtrMul(GetPtrConstant(JSTaggedValue::TaggedTypeSize()),
                        GetPtrConstant(GetInteger32Constant(panda::ecmascript::NumberDictionary::SIZE_INDEX)));
        AddrShift data = Load(POINTER_TYPE, elements, GetPtrConstant(panda::coretypes::Array::GetDataOffset()));
        AddrShift capacity = TaggedCastToInt32(Load(TAGGED_TYPE, data, offset));
        AddrShift count = GetInteger32Constant(1);
        AddrShift taggedIndex = IntBuildTagged(index);
        // 15 : size of entry
        AddrShift entry = Word32And(GetInteger32Constant(15), Int32Sub(capacity, GetInteger32Constant(1)));
        AddrShift dictionaryLength =
            Load(INT32_TYPE, elements, GetPtrConstant(panda::coretypes::Array::GetLengthOffset()));
        Label loopHead(env);
        Label loopEnd(env);
        Label afterLoop(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        AddrShift arrayIndex =
            Int32Add(GetInteger32Constant(panda::ecmascript::NumberDictionary::TABLE_HEADER_SIZE),
                     Int32Mul(entry, GetInteger32Constant(panda::ecmascript::NumberDictionary::ENTRY_SIZE)));

        Branch(Int32LessThan(arrayIndex, GetInteger32Constant(0)), &lessThan, &greaterOrEqual);
        Bind(&lessThan);
        Return(GetHoleConstant());
        Bind(&greaterOrEqual);
        Branch(Int32GreaterThan(arrayIndex, dictionaryLength), &greaterThan2, &lessOrEqual2);
        Bind(&greaterThan2);
        Return(GetHoleConstant());
        Bind(&lessOrEqual2);
        offset = PtrMul(ChangeInt32ToPointer(arrayIndex), GetPtrConstant(JSTaggedValue::TaggedTypeSize()));
        data = Load(POINTER_TYPE, elements, GetPtrConstant(panda::coretypes::Array::GetDataOffset()));
        AddrShift element = Load(TAGGED_TYPE, data, offset);
        Branch(TaggedIsHole(element), &isHole2, &notHole2);
        Bind(&isHole2);
        Jump(&loopEnd);
        // if element is undefined
        Bind(&notHole2);
        Branch(TaggedIsUndefined(element), &isUndef1, &notUndef1);
        Bind(&isUndef1);
        Return(GetHoleConstant());
        Bind(&notUndef1);
        Branch(TaggedIsHole(taggedIndex), &isHole3, &notHole3);
        Bind(&isHole3);
        Jump(&loopEnd);
        Bind(&notHole3);
        Branch(TaggedIsUndefined(taggedIndex), &isUndef2, &notUndef2);
        Bind(&isUndef2);
        Jump(&loopEnd);
        Bind(&notUndef2);
        Branch(TaggedIsInt(taggedIndex), &indexIsInt, &indexIsNotInt);
        Bind(&indexIsNotInt);
        Jump(&loopEnd);
        Bind(&indexIsInt);
        Branch(TaggedIsInt(element), &elelmentIsInt, &elelmentIsNotInt);
        Bind(&elelmentIsNotInt);
        Jump(&loopEnd);
        Bind(&elelmentIsInt);
        Branch(Word32Equal(TaggedCastToInt32(taggedIndex), TaggedCastToInt32(element)), &equal1, &notEqual1);
        Bind(&notEqual1);
        Jump(&loopEnd);
        Bind(&equal1);
        Branch(Word32Equal(entry, GetInteger32Constant(-1)), &equal2, &notEqual2);
        Bind(&equal2);
        Return(GetHoleConstant());
        Bind(&notEqual2);
        entry =
            Int32Add(GetInteger32Constant(panda::ecmascript::NumberDictionary::ENTRY_VALUE_INDEX),
                     Int32Add(GetInteger32Constant(panda::ecmascript::NumberDictionary::TABLE_HEADER_SIZE),
                              Int32Mul(entry, GetInteger32Constant(panda::ecmascript::NumberDictionary::ENTRY_SIZE))));
        Branch(Int32LessThan(entry, GetInteger32Constant(0)), &lessThanZero, &greaterOrEqualZero);
        Bind(&lessThanZero);
        Return(GetUndefinedConstant());
        Bind(&greaterOrEqualZero);
        Branch(Int32GreaterThan(entry, dictionaryLength), &greaterThanLength, &notGreaterThanLength);
        Bind(&greaterThanLength);
        Return(GetUndefinedConstant());
        Bind(&notGreaterThanLength);
        offset = PtrMul(ChangeInt32ToPointer(arrayIndex), GetPtrConstant(JSTaggedValue::TaggedTypeSize()));
        data = Load(POINTER_TYPE, elements, GetPtrConstant(panda::coretypes::Array::GetDataOffset()));
        Return(Load(TAGGED_TYPE, data, offset));
        Bind(&loopEnd);
        entry = Word32And(Int32Add(entry, count), Int32Sub(capacity, GetInteger32Constant(1)));
        count = Int32Add(count, GetInteger32Constant(1));
        LoopEnd(&loopHead);
    }
};

HWTEST_F_L0(StubOptimizerTest, FastFindOwnElementStub)
{
    LLVMModuleRef module = static_cast<LLVMModuleRef>(FastStubs::GetInstance().GetModule());
    LLVMValueRef findFunction = LLVMGetNamedFunction(module, "FindOwnElement");
    Environment env("FastFindOwnElementStub", 2);
    FastFindOwnElementStub optimizer(&env);
    optimizer.GenerateCircuit();
    auto netOfGates = env.GetCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    for (size_t bbIdx = 0; bbIdx < cfg.size(); bbIdx++) {
        std::cout << (netOfGates.GetOpCode(cfg[bbIdx].front()).IsCFGMerge() ? "MERGE_" : "BB_") << bbIdx << ":"
                  << std::endl;
        for (size_t instIdx = cfg[bbIdx].size(); instIdx > 0; instIdx--) {
            netOfGates.Print(cfg[bbIdx][instIdx - 1]);
        }
    }
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, module, findFunction);
    llvmBuilder.Build();
    LLVMMCJITCompiler compiler(module);
    compiler.Run();
}
class FastGetElementStub : public StubOptimizer {
public:
    explicit FastGetElementStub(Environment *env)
        // 2 : parameter number
        : StubOptimizer(env), findOwnElementDescriptor(0, 2, DEFAULT_ORDER, MachineType::TAGGED_TYPE)
    {
        // 2 : 2 means that there are 2 cases in total.
        std::array<MachineType, 2> *params = new std::array<MachineType, 2>();
        (*params)[0] = MachineType::POINTER_TYPE;
        (*params)[1] = MachineType::UINT32_TYPE;
        findOwnElementDescriptor.SetParameters(params->data());
    }
    ~FastGetElementStub() = default;
    NO_MOVE_SEMANTIC(FastGetElementStub);
    NO_COPY_SEMANTIC(FastGetElementStub);

    void GenerateCircuit() override
    {
        auto env = GetEnvironment();
        AddrShift receiver = Int64Argument(0);
        AddrShift index = Int64Argument(1);
        StubOptimizerLabel isHole(env);
        StubOptimizerLabel notHole(env);
        Label loopHead(env);
        Label loopEnd(env);
        Label afterLoop(env);
        Label notHeapObj(env);

        Jump(&loopHead);
        LoopBegin(&loopHead);
        AddrShift objPtr = ChangeInt64ToPointer(receiver);
        AddrShift callFindOwnElementVal =
            CallStub(&findOwnElementDescriptor, GetWord64Constant(FAST_STUB_ID(FindOwnElement)), {objPtr, index});
        Branch(TaggedIsHole(callFindOwnElementVal), &isHole, &notHole);
        Bind(&notHole);
        Return(callFindOwnElementVal);
        Bind(&isHole);
        receiver = Load(TAGGED_TYPE, LoadHClass(objPtr), GetPtrConstant(JSHClass::PROTOTYPE_OFFSET));
        Branch(TaggedIsHeapObject(receiver), &loopEnd, &notHeapObj);
        Bind(&notHeapObj);
        Return(GetUndefinedConstant());
        Bind(&loopEnd);
        LoopEnd(&loopHead);
    }

    StubInterfaceDescriptor findOwnElementDescriptor;
};

HWTEST_F_L0(StubOptimizerTest, FastGetElementStub)
{
    LLVMModuleRef module = static_cast<LLVMModuleRef>(FastStubs::GetInstance().GetModule());
    LLVMValueRef findFunction = LLVMGetNamedFunction(module, "FindOwnElement");
    Environment findFuncEnv("FastFindOwnElement_Foo", 2);
    FastFindOwnElementStub findOptimizer(&findFuncEnv);
    findOptimizer.GenerateCircuit();
    auto netOfGates = findFuncEnv.GetCircuit();
    auto cfg = Scheduler::Run(&netOfGates);
    for (size_t bbIdx = 0; bbIdx < cfg.size(); bbIdx++) {
        std::cout << (netOfGates.GetOpCode(cfg[bbIdx].front()).IsCFGMerge() ? "MERGE_" : "BB_") << bbIdx << ":"
                  << std::endl;
        for (size_t instIdx = cfg[bbIdx].size(); instIdx > 0; instIdx--) {
            netOfGates.Print(cfg[bbIdx][instIdx - 1]);
        }
    }
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, module, findFunction);
    llvmBuilder.Build();
    Environment getFuncEnv("FastGetElement", 2);
    FastGetElementStub getOptimizer(&getFuncEnv);
    getOptimizer.GenerateCircuit();
    auto getNetOfGates = getFuncEnv.GetCircuit();
    getNetOfGates.PrintAllGates();
    auto getCfg = Scheduler::Run(&getNetOfGates);
    for (size_t bbIdx = 0; bbIdx < getCfg.size(); bbIdx++) {
        std::cout << (getNetOfGates.GetOpCode(getCfg[bbIdx].front()).IsCFGMerge() ? "MERGE_" : "BB_") << bbIdx << ":"
                  << std::endl;
        for (size_t instIdx = getCfg[bbIdx].size(); instIdx > 0; instIdx--) {
            getNetOfGates.Print(getCfg[bbIdx][instIdx - 1]);
        }
    }
    llvmBuilder.Build();
    LLVMMCJITCompiler compiler(module);
    compiler.Run();
}

class FastFindOwnElement2Stub : public StubOptimizer {
public:
    explicit FastFindOwnElement2Stub(Environment *env) : StubOptimizer(env) {}
    ~FastFindOwnElement2Stub() = default;
    NO_MOVE_SEMANTIC(FastFindOwnElement2Stub);
    NO_COPY_SEMANTIC(FastFindOwnElement2Stub);
    void GenerateCircuit() override
    {
        auto env = GetEnvironment();
        AddrShift thread = PtrArgument(0);
        AddrShift elements = PtrArgument(1);
        AddrShift index = Int32Argument(2);
        AddrShift isDict = Int32Argument(3);
        AddrShift attr = PtrArgument(4);
        AddrShift indexOrEntry = PtrArgument(5);
        isDict = ZExtInt1ToInt32(isDict);
        Label notDictionary(env);
        Label isDictionary(env);
        Label end(env);
        Branch(Word32Equal(isDict, GetInteger32Constant(0)), &notDictionary, &isDictionary);
        Bind(&notDictionary);
        {
            AddrShift elementsLength =
                Load(UINT32_TYPE, elements, GetPtrConstant(panda::coretypes::Array::GetLengthOffset()));
            Label outOfElements(env);
            Label notOutOfElements(env);
            Branch(Int32LessThanOrEqual(elementsLength, index), &outOfElements, &notOutOfElements);
            Bind(&outOfElements);
            {
                Return(GetHoleConstant());
            }
            Bind(&notOutOfElements);
            {
                AddrShift value = GetValueFromTaggedArray(elements, index);
                Label isHole(env);
                Label notHole(env);
                Branch(TaggedIsHole(value), &isHole, &notHole);
                Bind(&isHole);
                Jump(&end);
                Bind(&notHole);
                {
                    Store(UINT32_TYPE, attr, GetPtrConstant(0),
                          GetInteger32Constant(PropertyAttributes::GetDefaultAttributes()));
                    Store(UINT32_TYPE, indexOrEntry, GetPtrConstant(0), index);
                    Return(value);
                }
            }
        }
        Bind(&isDictionary);
        {
            Label afterFindElement(env);
            AddrShift entry =
                FindElementFromNumberDictionary(thread, elements, IntBuildTagged(index), &afterFindElement);
            Label notNegtiveOne(env);
            Label negtiveOne(env);
            Branch(Word32NotEqual(entry, GetInteger32Constant(-1)), &notNegtiveOne, &negtiveOne);
            Bind(&notNegtiveOne);
            {
                Store(UINT32_TYPE, attr, GetPtrConstant(0), GetDetailsFromDictionary(elements, entry));
                Store(UINT32_TYPE, indexOrEntry, GetPtrConstant(0), entry);
                Return(GetValueFromDictionary(elements, entry));
            }
            Bind(&negtiveOne);
            Jump(&end);
        }
        Bind(&end);
        Return(GetHoleConstant());
    }
};

class SetElementStub : public StubOptimizer {
public:
    explicit SetElementStub(Environment *env) : StubOptimizer(env) {}
    ~SetElementStub() = default;
    NO_MOVE_SEMANTIC(SetElementStub);
    NO_COPY_SEMANTIC(SetElementStub);
    void GenerateCircuit() override
    {
        auto env = GetEnvironment();
        AddrShift thread = PtrArgument(0);
        AddrShift receiver = PtrArgument(1);
        DEFVARIABLE(holder, MachineType::TAGGED_POINTER_TYPE, receiver);
        AddrShift index = Int32Argument(2);     // 2 : 3rd argument
        AddrShift value = Int64Argument(3);     // 3 : 4th argument
        AddrShift mayThrow = Int32Argument(4);  // 4 : 5th argument
        DEFVARIABLE(onPrototype, MachineType::BOOL_TYPE, FalseConstant());

        AddrShift pattr = Alloca(static_cast<int>(MachineRep::K_WORD32));
        AddrShift pindexOrEntry = Alloca(static_cast<int>(MachineRep::K_WORD32));
        Label loopHead(env);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            AddrShift elements = GetElements(*holder);
            AddrShift isDictionary = IsDictionaryMode(elements);
            StubInterfaceDescriptor *findOwnElemnt2 = GET_STUBDESCRIPTOR(FindOwnElement2);
            AddrShift val = CallStub(findOwnElemnt2, GetWord64Constant(FAST_STUB_ID(FindOwnElement2)),
                                     {thread, elements, index, isDictionary, pattr, pindexOrEntry});
            Label notHole(env);
            Label isHole(env);
            Branch(TaggedIsNotHole(val), &notHole, &isHole);
            Bind(&notHole);
            {
                Label isOnProtoType(env);
                Label notOnProtoType(env);
                Label afterOnProtoType(env);
                Branch(*onPrototype, &isOnProtoType, &notOnProtoType);
                Bind(&notOnProtoType);
                Jump(&afterOnProtoType);
                Bind(&isOnProtoType);
                {
                    Label isExtensible(env);
                    Label notExtensible(env);
                    Label nextExtensible(env);
                    Branch(IsExtensible(receiver), &isExtensible, &notExtensible);
                    Bind(&isExtensible);
                    Jump(&nextExtensible);
                    Bind(&notExtensible);
                    {
                        Label isThrow(env);
                        Label notThrow(env);
                        Branch(Word32NotEqual(mayThrow, GetInteger32Constant(0)), &isThrow, &notThrow);
                        Bind(&isThrow);
                        ThrowTypeAndReturn(thread, GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensible),
                                           FalseConstant());
                        Bind(&notThrow);
                        Return(FalseConstant());
                    }
                    Bind(&nextExtensible);
                    StubInterfaceDescriptor *addElementInternal = GET_STUBDESCRIPTOR(AddElementInternal);
                    Return(CallRuntime(addElementInternal, thread, GetWord64Constant(FAST_STUB_ID(AddElementInternal)),
                                       {thread, receiver, index, value,
                                        GetInteger32Constant(PropertyAttributes::GetDefaultAttributes())}));
                }
                Bind(&afterOnProtoType);
                {
                    AddrShift attr = Load(INT32_TYPE, pattr);
                    Label isAccessor(env);
                    Label notAccessor(env);
                    Branch(IsAcesscor(attr), &isAccessor, &notAccessor);
                    Bind(&notAccessor);
                    {
                        Label isWritable(env);
                        Label notWritable(env);
                        Branch(IsWritable(attr), &isWritable, &notWritable);
                        Bind(&isWritable);
                        {
                            AddrShift elements = GetElements(receiver);
                            Label isDict(env);
                            Label notDict(env);
                            AddrShift indexOrEntry = Load(INT32_TYPE, pindexOrEntry);
                            Branch(isDictionary, &isDict, &notDict);
                            Bind(&notDict);
                            {
                                StoreElement(elements, indexOrEntry, value);
                                Label updateRepLabel(env);
                                UpdateRepresention(LoadHClass(receiver), value, &updateRepLabel);
                                Return(TrueConstant());
                            }
                            Bind(&isDict);
                            {
                                UpdateValueAndDetails(elements, indexOrEntry, value, attr);
                                Return(TrueConstant());
                            }
                        }
                        Bind(&notWritable);
                        {
                            Label isThrow(env);
                            Label notThrow(env);
                            Branch(Word32NotEqual(mayThrow, GetInteger32Constant(0)), &isThrow, &notThrow);
                            Bind(&isThrow);
                            ThrowTypeAndReturn(thread, GET_MESSAGE_STRING_ID(SetReadOnlyProperty), FalseConstant());
                            Bind(&notThrow);
                            Return(FalseConstant());
                        }
                    }
                    Bind(&isAccessor);
                    {
                        StubInterfaceDescriptor *callsetter = GET_STUBDESCRIPTOR(CallSetter);
                        AddrShift setter = GetSetterFromAccessor(val);
                        Return(CallRuntime(callsetter, thread, GetWord64Constant(FAST_STUB_ID(CallSetter)),
                                           {thread, setter, receiver, value, TruncInt32ToInt1(mayThrow)}));
                    }
                }
            }
            Bind(&isHole);
            {
                // holder equals to JSObject::Cast(holder)->GetJSHClass()->GetPrototype();
                holder = GetPrototypeFromHClass(LoadHClass(*holder));
                Label isHeapObj(env);
                Label notHeapObj(env);
                Branch(TaggedIsObject(*holder), &isHeapObj, &notHeapObj);
                Bind(&notHeapObj);
                {
                    Label isExtensible(env);
                    Label notExtensible(env);
                    Label nextExtensible(env);
                    Branch(IsExtensible(receiver), &isExtensible, &notExtensible);
                    Bind(&isExtensible);
                    Jump(&nextExtensible);
                    Bind(&notExtensible);
                    {
                        Label isThrow(env);
                        Label notThrow(env);
                        Branch(Word32NotEqual(mayThrow, GetInteger32Constant(0)), &isThrow, &notThrow);
                        Bind(&isThrow);
                        ThrowTypeAndReturn(thread, GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensible),
                                           FalseConstant());
                        Bind(&notThrow);
                        Return(FalseConstant());
                    }
                    Bind(&nextExtensible);
                    {
                        StubInterfaceDescriptor *addElementInternal = GET_STUBDESCRIPTOR(AddElementInternal);
                        Return(CallRuntime(addElementInternal, thread,
                                           GetWord64Constant(FAST_STUB_ID(AddElementInternal)),
                                           {thread, receiver, index, value,
                                            GetInteger32Constant(PropertyAttributes::GetDefaultAttributes())}));
                    }
                }
                Bind(&isHeapObj);
                {
                    Label isJsProxy(env);
                    Label notJsProxy(env);
                    Branch(IsJsProxy(*holder), &isJsProxy, &notJsProxy);
                    Bind(&isJsProxy);
                    {
                        StubInterfaceDescriptor *setProperty = GET_STUBDESCRIPTOR(JSProxySetProperty);
                        Return(CallRuntime(
                            setProperty, thread, GetWord64Constant(FAST_STUB_ID(JSProxySetProperty)),
                            {thread, *holder, IntBuildTagged(index), value, receiver, TruncInt32ToInt1(mayThrow)}));
                    }
                    Bind(&notJsProxy);
                    onPrototype = TrueConstant();
                }
            }
        }
        LoopEnd(&loopHead);
    }
};

HWTEST_F_L0(StubOptimizerTest, FastFindOwnElement2Stub)
{
    std::cout << " ------------------------FastFindOwnElement2Stub ---------------------" << std::endl;
    LLVMModuleRef module = static_cast<LLVMModuleRef>(FastStubs::GetInstance().GetModule());
    LLVMValueRef function = LLVMGetNamedFunction(module, "FindOwnElement2");
    Environment env("FindOwnElement2", 6);
    FastFindOwnElement2Stub optimizer(&env);
    optimizer.GenerateCircuit();
    auto netOfGates = env.GetCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    for (size_t bbIdx = 0; bbIdx < cfg.size(); bbIdx++) {
        std::cout << (netOfGates.GetOpCode(cfg[bbIdx].front()).IsCFGMerge() ? "MERGE_" : "BB_") << bbIdx << ":"
                  << std::endl;
        for (size_t instIdx = cfg[bbIdx].size(); instIdx > 0; instIdx--) {
            netOfGates.Print(cfg[bbIdx][instIdx - 1]);
        }
    }
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, module, function);
    llvmBuilder.Build();
    LLVMMCJITCompiler compiler(module);
    compiler.Run();
    auto engine = compiler.GetEngine();
    auto *findOwnElement2Ptr = reinterpret_cast<JSTaggedValue (*)(JSThread *, TaggedArray *, uint32_t, bool,
                                                                  PropertyAttributes *, uint32_t *)>(
        reinterpret_cast<uintptr_t>(LLVMGetPointerToGlobal(engine, function)));
    auto *factory = JSThread::Cast(thread)->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    int x = 213;
    int y = 10;
    FastRuntimeStub::SetOwnElement(thread, obj.GetTaggedValue(), 1, JSTaggedValue(x));
    FastRuntimeStub::SetOwnElement(thread, obj.GetTaggedValue(), 10250, JSTaggedValue(y));
    TaggedArray *elements = TaggedArray::Cast(obj->GetElements().GetHeapObject());
    PropertyAttributes attr;
    uint32_t indexOrEntry;
    bool isDict = elements->IsDictionaryMode();
    compiler.Disassemble();
    JSTaggedValue resVal = findOwnElement2Ptr(thread, elements, 1, isDict, &attr, &indexOrEntry);
    EXPECT_EQ(resVal.GetNumber(), x);
    resVal = findOwnElement2Ptr(thread, elements, 10250, isDict, &attr, &indexOrEntry);
    EXPECT_EQ(resVal.GetNumber(), y);
    std::cout << " ++++++++++++++++++++FastFindOwnElement2Stub ++++++++++++++++++" << std::endl;
}

HWTEST_F_L0(StubOptimizerTest, SetElementStub)
{
    LLVMModuleRef module = static_cast<LLVMModuleRef>(FastStubs::GetInstance().GetModule());
    LLVMValueRef function = LLVMGetNamedFunction(module, "SetElement");
    Environment env("SetElementStub", 5);
    SetElementStub optimizer(&env);
    optimizer.GenerateCircuit();
    auto netOfGates = env.GetCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    for (size_t bbIdx = 0; bbIdx < cfg.size(); bbIdx++) {
        std::cout << (netOfGates.GetOpCode(cfg[bbIdx].front()).IsCFGMerge() ? "MERGE_" : "BB_") << bbIdx << ":"
                  << std::endl;
        for (size_t instIdx = cfg[bbIdx].size(); instIdx > 0; instIdx--) {
            netOfGates.Print(cfg[bbIdx][instIdx - 1]);
        }
    }
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, module, function);
    llvmBuilder.Build();
    LLVMMCJITCompiler compiler(module);
    compiler.Run();
}

struct ThreadTy {
    intptr_t magic; // 0x11223344
    intptr_t fp;
};
class StubCallRunTimeThreadFpLock {
public:
    StubCallRunTimeThreadFpLock(struct ThreadTy *thread, intptr_t newFp): oldRbp_(thread->fp),
        thread_(thread)
    {
        thread_->fp = *(reinterpret_cast<int64_t *>(newFp));
        std::cout << "StubCallRunTimeThreadFpLock newFp: " << newFp << " oldRbp_ : " << oldRbp_
            << " thread_->fp:" << thread_->fp <<std::endl;
    }
    ~StubCallRunTimeThreadFpLock()
    {
        std::cout << "~StubCallRunTimeThreadFpLock oldRbp_: " << oldRbp_ << " thread_->fp:" << thread_->fp << std::endl;
        thread_->fp = oldRbp_;
    }
private:
    intptr_t oldRbp_;
    struct ThreadTy *thread_;
};

extern "C" {
int64_t RuntimeFunc(struct ThreadTy *fpInfo)
{
    int64_t *rbp;
    asm("mov %%rbp, %0" : "=rm"(rbp));
    if (fpInfo->fp == *rbp) {
        return 1;
    }
    return 0;
}

int64_t (*g_stub2Func)(struct ThreadTy *) = nullptr;

int RuntimeFunc1(struct ThreadTy *fpInfo)
{
    std::cout << "RuntimeFunc1  -" << std::endl;
    int64_t newRbp;
    asm("mov %%rbp, %0" : "=rm"(newRbp));
    StubCallRunTimeThreadFpLock lock(fpInfo, newRbp);

    std::cout << std::hex << "g_stub2Func " << reinterpret_cast<uintptr_t>(g_stub2Func) << std::endl;
    if (g_stub2Func != nullptr) {
        g_stub2Func(fpInfo);
    }
    std::cout << "RuntimeFunc1  +" << std::endl;
    return 0;
}

int RuntimeFunc2(struct ThreadTy *fpInfo)
{
    std::cout << "RuntimeFunc2  -" << std::endl;
    // update thread.fp
    int64_t newRbp;
    asm("mov %%rbp, %0" : "=rm"(newRbp));
    StubCallRunTimeThreadFpLock lock(fpInfo, newRbp);
    auto rbp = reinterpret_cast<int64_t *>(fpInfo->fp);

    std::cout << " RuntimeFunc2 rbp:" << rbp <<std::endl;
    for (int i = 0; i < 40; i++) {
        std::cout << std::hex << &(rbp[i]) << " :" << rbp[i] << std::endl;
    }
    /* walk back
      stack frame:           0     pre rbp  <-- rbp
                            -8     type
                            -16    pre frame thread fp
    */
    int64_t *frameType = nullptr;
    int64_t *gcFp = nullptr;
    std::cout << "-----------------walkback----------------" << std::endl;
    do {
        frameType = rbp - 1;
        if (*frameType == 1) {
            gcFp = rbp - 2;
        } else {
            gcFp = rbp;
        }
        rbp = reinterpret_cast<intptr_t *>(*gcFp);
        std::cout << std::hex << "frameType :" << *frameType << " gcFp:" << *gcFp << std::endl;
    } while (*gcFp != 0);
    std::cout << "+++++++++++++++++walkback++++++++++++++++" << std::endl;
    std::cout << "call RuntimeFunc2 func ThreadTy fp: " << fpInfo->fp << " magic:" << fpInfo->magic << std::endl;
    std::cout << "RuntimeFunc2  +" << std::endl;
    return 0;
}
}

/*
c++:main
  --> js (stub1(struct ThreadTy *))
        stack frame:           0     pre rbp  <-- rbp
                              -8     type
                              -16    pre frame thread fp
  --> c++(int RuntimeFunc1(struct ThreadTy *fpInfo))
  --> js (int stub2(struct ThreadTy *))
                                stack frame:           0     pre rbp  <-- rbp
                                -8     type
                                -16    pre frame thread fp
  --> js (int stub3(struct ThreadTy *))
                                stack frame:           0     pre rbp  <-- rbp
                                -8     type
  --> c++(int RuntimeFunc2(struct ThreadTy *fpInfo))

result:
-----------------walkback----------------
frameType :0 gcFp:7fffffffd780
frameType :1 gcFp:7fffffffd820
frameType :1 gcFp:0
+++++++++++++++++walkback++++++++++++++++
#0  __GI_raise (sig=sig@entry=6) at ../sysdeps/unix/sysv/linux/raise.c:40
#1  0x00007ffff03778b1 in __GI_abort () at abort.c:79
#2  0x0000555555610f1c in RuntimeFunc2 ()
#3  0x00007fffebf7b1fb in stub3 ()
#4  0x00007fffebf7b1ab in stub2 ()
#5  0x0000555555610afe in RuntimeFunc1 ()
#6  0x00007fffebf7b14e in stub1 ()
#7  0x000055555561197c in panda::test::StubOptimizerTest_JSEntryTest_Test::TestBody() ()
*/

LLVMValueRef LLVMCallingFp(LLVMModuleRef &module, LLVMBuilderRef &builder)
{
    /* 0:calling 1:its caller */
    std::vector<LLVMValueRef> args = {LLVMConstInt(LLVMInt32Type(), 0, false)};
    auto fn = LLVMGetNamedFunction(module, "llvm.frameaddress.p0i8");
    if (!fn) {
        std::cout << "Could not find function " << std::endl;
        return LLVMConstInt(LLVMInt64Type(), 0, false);
    }
    LLVMValueRef fAddrRet = LLVMBuildCall(builder, fn, args.data(), 1, "");
    LLVMValueRef frameAddr = LLVMBuildPtrToInt(builder, fAddrRet, LLVMInt64Type(), "cast_int64_t");
    return frameAddr;
}

HWTEST_F_L0(StubOptimizerTest, JSEntryTest)
{
    std::cout << " ---------- JSEntryTest ------------- " << std::endl;
    LLVMModuleRef module = LLVMModuleCreateWithName("simple_module");
    LLVMSetTarget(module, "x86_64-unknown-linux-gnu");
    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMContextRef context = LLVMGetGlobalContext();

    /* init instrinsic function declare */
    LLVMTypeRef paramTys1[] = {
        LLVMInt32Type(),
    };
    auto llvmFnType = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0), paramTys1, 1, 0);
    LLVMValueRef llvmFn = LLVMAddFunction(module, "llvm.frameaddress.p0i8", llvmFnType);
    (void)llvmFn;

    // struct ThreadTy
    LLVMTypeRef llvmI32 = LLVMInt32TypeInContext(context);
    LLVMTypeRef llvmI64 = LLVMInt64TypeInContext(context);
    std::vector<LLVMTypeRef> elements_t {llvmI64, llvmI64};
    LLVMTypeRef threadTy = LLVMStructTypeInContext(context, elements_t.data(), elements_t.size(), 0);
    LLVMTypeRef threadTyPtr = LLVMPointerType(threadTy, 0);
    LLVMTypeRef paramTys0[] = {
        threadTyPtr
    };
    /* implement stub1 */
    LLVMValueRef stub1 = LLVMAddFunction(module, "stub1", LLVMFunctionType(LLVMInt64Type(), paramTys0, 1, 0));
    LLVMAddTargetDependentFunctionAttr(stub1, "frame-pointer", "all");
    LLVMAddTargetDependentFunctionAttr(stub1, "js-stub-call", "1");

    LLVMBasicBlockRef entryBb = LLVMAppendBasicBlock(stub1, "entry");
    LLVMPositionBuilderAtEnd(builder, entryBb);
    /* struct ThreadTy fpInfo;
        fpInfo.magic = 0x11223344;
        fpInfo.fp = calling frame address
    */
    LLVMValueRef value = LLVMGetParam(stub1, 0);
    LLVMValueRef c0 = LLVMConstInt(LLVMInt32Type(), 0, false);
    LLVMValueRef c1 = LLVMConstInt(LLVMInt32Type(), 1, false);
    std::vector<LLVMValueRef> indexes1 = {c0, c0};
    std::vector<LLVMValueRef> indexes2 = {c0, c1};
    LLVMValueRef gep1 = LLVMBuildGEP2(builder, threadTy, value, indexes1.data(), indexes1.size(), "");

    LLVMValueRef gep2 = LLVMBuildGEP2(builder, threadTy, value, indexes2.data(), indexes2.size(), "fpAddr");
    LLVMBuildStore(builder, LLVMConstInt(LLVMInt64Type(), 0x11223344, false), gep1);
    LLVMValueRef frameAddr = LLVMCallingFp(module, builder);
    /* current frame
         0     pre rbp  <-- rbp
         -8    type
        -16    pre frame thread fp
    */
    LLVMValueRef preFp = LLVMBuildLoad2(builder, LLVMInt64Type(), gep2, "thread.fp");
    LLVMValueRef addr = LLVMBuildSub(builder, frameAddr, LLVMConstInt(LLVMInt64Type(), 16, false), "");
    LLVMValueRef preFpAddr = LLVMBuildIntToPtr(builder, addr, LLVMPointerType(LLVMInt64Type(), 0), "thread.fp.Addr");
    LLVMBuildStore(builder, preFp, preFpAddr);

    /* stub1 call RuntimeFunc1 */
    std::vector<LLVMTypeRef> argsTy(1, threadTyPtr);
    LLVMTypeRef funcType = LLVMFunctionType(llvmI32, argsTy.data(), argsTy.size(), 1);
    LLVMValueRef runtimeFunc1 = LLVMAddFunction(module, "RuntimeFunc1", funcType);

    std::vector<LLVMValueRef> argValue = {value};
    LLVMBuildCall(builder, runtimeFunc1, argValue.data(), 1, "");
    LLVMValueRef retVal = LLVMConstInt(LLVMInt64Type(), 1, false);
    LLVMBuildRet(builder, retVal);

    /* implement stub2 call stub3 */
    LLVMValueRef stub2 = LLVMAddFunction(module, "stub2", LLVMFunctionType(LLVMInt64Type(), paramTys0, 1, 0));
    LLVMAddTargetDependentFunctionAttr(stub2, "frame-pointer", "all");
    LLVMAddTargetDependentFunctionAttr(stub2, "js-stub-call", "1");

    entryBb = LLVMAppendBasicBlock(stub2, "entry");
    LLVMPositionBuilderAtEnd(builder, entryBb);
    LLVMValueRef value2 = LLVMGetParam(stub2, 0);
    /* struct ThreadTy fpInfo;
        fpInfo.fp = calling frame address
    */
    gep2 = LLVMBuildGEP2(builder, threadTy, value2, indexes2.data(), indexes2.size(), "fpAddr");
    frameAddr = LLVMCallingFp(module, builder);
    /* current frame
         0     pre rbp  <-- rbp
         -8    type
        -16    pre frame thread fp
    */
    preFp = LLVMBuildLoad2(builder, LLVMInt64Type(), gep2, "thread.fp");
    addr = LLVMBuildSub(builder, frameAddr, LLVMConstInt(LLVMInt64Type(), 16, false), "");
    preFpAddr = LLVMBuildIntToPtr(builder, addr, LLVMPointerType(LLVMInt64Type(), 0), "thread.fp.Addr");
    LLVMBuildStore(builder, preFp, preFpAddr);

    LLVMValueRef stub3 = LLVMAddFunction(module, "stub3", LLVMFunctionType(LLVMInt64Type(), paramTys0, 1, 0));
    /* stub2 call stub3 */
    argValue = {value2};

    LLVMBuildCall(builder, stub3, argValue.data(), 1, "");
    retVal = LLVMConstInt(LLVMInt64Type(), 2, false);
    LLVMBuildRet(builder, retVal);

    /* implement stub3 call RuntimeFunc2 */
    LLVMAddTargetDependentFunctionAttr(stub3, "frame-pointer", "all");
    LLVMAddTargetDependentFunctionAttr(stub3, "js-stub-call", "0");

    entryBb = LLVMAppendBasicBlock(stub3, "entry");
    LLVMPositionBuilderAtEnd(builder, entryBb);
    LLVMValueRef value3 = LLVMGetParam(stub3, 0);
    /* struct ThreadTy fpInfo;
        fpInfo.fp = calling frame address
    */
    gep2 = LLVMBuildGEP2(builder, threadTy, value3, indexes2.data(), indexes2.size(), "fpAddr");
    /* current frame
         0     pre rbp  <-- rbp
         -8    type
    */
    /* stub2 call RuntimeFunc2 */
    LLVMValueRef runtimeFunc2 = LLVMAddFunction(module, "RuntimeFunc2", funcType);
    argValue = {value3};
    LLVMBuildCall(builder, runtimeFunc2, argValue.data(), 1, "");
    retVal = LLVMConstInt(LLVMInt64Type(), 3, false);
    LLVMBuildRet(builder, retVal);
    LLVMDumpModule(module);
    char* error = nullptr;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);

    LLVMMCJITCompiler compiler(module);
    compiler.Run();
    auto engine = compiler.GetEngine();
    uint64_t stub1Code = LLVMGetFunctionAddress(engine, "stub1");
    uint64_t stub2Code = LLVMGetFunctionAddress(engine, "stub2");
    uint64_t stub3Code = LLVMGetFunctionAddress(engine, "stub3");
    std::map<uint64_t, std::string> addr2name = {
        {stub1Code, "stub1"}, {stub2Code, "stub2"}, {stub3Code, "stub3"}
    };
    std::cout << std::endl << " stub1Code : " << stub1Code << std::endl;
    std::cout << std::endl << " stub2Code : " << stub2Code << std::endl;
    std::cout << std::endl << " stub3Code : " << stub3Code << std::endl;
    compiler.Disassemble(addr2name);
    struct ThreadTy parameters = {0x0, 0x0};
    auto stub1Func = reinterpret_cast<int64_t (*)(struct ThreadTy *)>(stub1Code);
    g_stub2Func = reinterpret_cast<int64_t (*)(struct ThreadTy *)>(stub2Code);
    int64_t result = stub1Func(&parameters);
    std::cout << std::endl << "parameters magic:" << parameters.magic
        << " parameters.fp " << parameters.fp << std::endl;
    EXPECT_EQ(parameters.fp, 0x0);
    EXPECT_EQ(result, 1);
    std::cout << " ++++++++++ JSEntryTest +++++++++++++ " << std::endl;
}

/*
verify modify llvm prologue : call main ok means don't destory c abi
test:
main          push rbp
              push type
              push thread.fp  // reserved for modification
    call bar
              push rbp
              push type
*/
HWTEST_F_L0(StubOptimizerTest, Prologue)
{
    std::cout << " ---------- Prologue ------------- " << std::endl;
    LLVMModuleRef module = LLVMModuleCreateWithName("simple_module");
    LLVMSetTarget(module, "x86_64-unknown-linux-gnu");
    LLVMBuilderRef builder = LLVMCreateBuilder();
    // struct ThreadTy
    LLVMTypeRef paramTys0[] = {
        LLVMInt64Type(),
        LLVMInt64Type(),
    };

    /* implement main implement */
    LLVMValueRef func = LLVMAddFunction(module, "main", LLVMFunctionType(LLVMInt64Type(), nullptr, 0, 0));
    LLVMAddTargetDependentFunctionAttr(func, "frame-pointer", "all");
    LLVMAddTargetDependentFunctionAttr(func, "js-stub-call", "1");

    LLVMBasicBlockRef entryBb = LLVMAppendBasicBlock(func, "entry");
    LLVMPositionBuilderAtEnd(builder, entryBb);

    /* implement main call RuntimeFunc */
    LLVMBuilderRef builderBar = LLVMCreateBuilder();
    LLVMValueRef bar = LLVMAddFunction(module, "bar", LLVMFunctionType(LLVMInt64Type(), paramTys0, 2, 0));
    LLVMAddTargetDependentFunctionAttr(bar, "frame-pointer", "all");
    LLVMAddTargetDependentFunctionAttr(bar, "js-stub-call", "0");
    LLVMBasicBlockRef entryBbBar = LLVMAppendBasicBlock(bar, "entry");
    LLVMPositionBuilderAtEnd(builderBar, entryBbBar);
    LLVMValueRef value0Bar = LLVMGetParam(bar, 0);
    LLVMValueRef value1Bar = LLVMGetParam(bar, 1);
    LLVMValueRef retValBar = LLVMBuildAdd(builderBar, value0Bar, value1Bar, "");
    LLVMBuildRet(builderBar, retValBar);

    LLVMValueRef value0 = LLVMConstInt(LLVMInt64Type(), 1, false);
    LLVMValueRef value1 = LLVMConstInt(LLVMInt64Type(), 2, false);
    std::vector<LLVMValueRef> args = {value0, value1};
    auto retVal = LLVMBuildCall(builder, bar, args.data(), 2, "");
    LLVMBuildRet(builder, retVal);

    LLVMDumpModule(module);
    char* error = nullptr;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);

    LLVMMCJITCompiler compiler(module);
    compiler.Run();
    auto engine = compiler.GetEngine();
    uint64_t mainCode = LLVMGetFunctionAddress(engine, "main");
    compiler.Disassemble();
    auto mainFunc = reinterpret_cast<int64_t (*)(int64_t, int64_t)>(mainCode);
    int64_t result = mainFunc(1, 2);
    EXPECT_EQ(result, 3);
    std::cout << " ++++++++++ Prologue +++++++++++++ " << std::endl;
}

/*
verify llvm.frameaddress.p0i8 ok
test:
    js(stub):main
        call RuntimeFunc
*/
HWTEST_F_L0(StubOptimizerTest, CEntryFp)
{
    std::cout << " ---------- CEntryFp ------------- " << std::endl;
    LLVMModuleRef module = LLVMModuleCreateWithName("simple_module");
    LLVMSetTarget(module, "x86_64-unknown-linux-gnu");
    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMContextRef context = LLVMGetGlobalContext();

    // struct ThreadTy
    LLVMTypeRef llvmI64 = LLVMInt64TypeInContext(context);
    std::vector<LLVMTypeRef> elements_t {llvmI64, llvmI64};
    LLVMTypeRef threadTy = LLVMStructTypeInContext(context, elements_t.data(), elements_t.size(), 0);
    LLVMTypeRef threadTyPtr = LLVMPointerType(threadTy, 0);
    LLVMTypeRef paramTys0[] = {
        threadTyPtr
    };
    /* implement main call RuntimeFunc */
    LLVMValueRef func = LLVMAddFunction(module, "main", LLVMFunctionType(LLVMInt64Type(), paramTys0, 1, 0));
    LLVMAddTargetDependentFunctionAttr(func, "frame-pointer", "all");
    LLVMAddTargetDependentFunctionAttr(func, "js-stub-call", "1");
    LLVMBasicBlockRef entryBb = LLVMAppendBasicBlock(func, "entry");
    LLVMPositionBuilderAtEnd(builder, entryBb);
    /* struct ThreadTy fpInfo;
        fpInfo.magic = 0x11223344;
        fpInfo.fp = calling frame address
    */
    LLVMValueRef value = LLVMGetParam(func, 0);

    LLVMValueRef c0 = LLVMConstInt(LLVMInt32Type(), 0, false);
    LLVMValueRef c1 = LLVMConstInt(LLVMInt32Type(), 1, false);
    std::vector<LLVMValueRef> indexes1 = {c0, c0};
    std::vector<LLVMValueRef> indexes2 = {c0, c1};
    LLVMValueRef gep1 = LLVMBuildGEP2(builder, threadTy, value, indexes1.data(), indexes1.size(), "");
    LLVMDumpValue(gep1);
    std::cout << std::endl;
    LLVMValueRef gep2 = LLVMBuildGEP2(builder, threadTy, value, indexes2.data(), indexes2.size(), "fpAddr");
    LLVMBuildStore(builder, LLVMConstInt(LLVMInt64Type(), 0x11223344, false), gep1);

    LLVMTypeRef paramTys1[] = {
        LLVMInt32Type(),
    };
    auto fnType = LLVMFunctionType(LLVMPointerType(LLVMInt8Type(), 0), paramTys1, 1, 0);
    /* 0:calling 1:its caller */
    std::vector<LLVMValueRef> args = {LLVMConstInt(LLVMInt32Type(), 0, false)};
    LLVMValueRef fn = LLVMAddFunction(module, "llvm.frameaddress.p0i8", fnType);
    LLVMValueRef fAddrRet = LLVMBuildCall(builder, fn, args.data(), 1, "");
    LLVMValueRef frameAddr = LLVMBuildPtrToInt(builder, fAddrRet, LLVMInt64Type(), "cast_int64_t");

    LLVMBuildStore(builder, frameAddr, gep2);

    /* main call RuntimeFunc */
    std::vector<LLVMTypeRef> argsTy(1, threadTyPtr);
    LLVMTypeRef funcType = LLVMFunctionType(llvmI64, argsTy.data(), argsTy.size(), 1);
    LLVMValueRef runTimeFunc = LLVMAddFunction(module, "RuntimeFunc", funcType);

    std::vector<LLVMValueRef> argValue = {value};
    std::cout << std::endl;
    LLVMValueRef retVal = LLVMBuildCall(builder, runTimeFunc, argValue.data(), 1, "");
    LLVMBuildRet(builder, retVal);

    LLVMDumpModule(module);
    char* error = nullptr;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);

    LLVMMCJITCompiler compiler(module);
    compiler.Run();
    auto engine = compiler.GetEngine();
    uint64_t nativeCode = LLVMGetFunctionAddress(engine, "main");
    std::cout << std::endl << " nativeCode : " << nativeCode << std::endl;
    compiler.Disassemble();
    struct ThreadTy parameters = {0x0, 0x0};

    auto mainFunc = reinterpret_cast<int64_t (*)(struct ThreadTy *)>(nativeCode);
    int64_t result = mainFunc(&parameters);
    EXPECT_EQ(result, 1);
    std::cout << " ++++++++++ CEntryFp +++++++++++++ " << std::endl;
}

HWTEST_F_L0(StubOptimizerTest, LoadGCIRTest)
{
    std::cout << "--------------LoadGCIRTest--------------------" << std::endl;
    char *path = get_current_dir_name();
    std::string filePath = std::string(path) + "/../../ark/js_runtime/ecmascript/compiler/tests/satepoint_GC_0.ll";

    char resolvedPath[PATH_MAX];
    char *res = realpath(filePath.c_str(), resolvedPath);
    if (res == nullptr) {
        std::cerr << "filePath :" << filePath.c_str() << "   is not exist !" << std::endl;
        return;
    }

    llvm::SMDiagnostic err;
    llvm::LLVMContext context;
    llvm::StringRef inputFilename(resolvedPath);
    // Load the input module...
    std::unique_ptr<llvm::Module> rawModule =
        parseIRFile(inputFilename, err, context);
    if (!rawModule) {
        std::cout << "parseIRFile :" << inputFilename.data() << " failed !" << std::endl;
        return;
    }
    LLVMModuleRef module  = LLVMCloneModule(wrap(rawModule.get()));
    LLVMMCJITCompiler compiler(module);
    compiler.Run();
    auto engine = compiler.GetEngine();
    LLVMValueRef function =
         LLVMGetNamedFunction(module, "main");
    LLVMDumpValue(function);

    auto *mainPtr = reinterpret_cast<int (*)()>(LLVMGetPointerToGlobal(engine, function));
    uint8_t *ptr = compiler.GetStackMapsSection();
    LLVMStackMapParse::GetInstance().CalculateStackMap(ptr);
    LLVMStackMapParse::GetInstance().Print();

    compiler.Disassemble();
    LLVMDumpModule(module);
    int value = reinterpret_cast<int (*)()>(mainPtr)();
    std::cout << " value:" << value << std::endl;
}
extern "C" {
void DoSafepoint()
{
    uintptr_t *rbp;
    asm("mov %%rbp, %0" : "=rm" (rbp));
    for (int i = 0; i < 3; i++) {
        uintptr_t returnAddr =  *(rbp + 1);
        uintptr_t *rsp = rbp + 2;
        rbp = reinterpret_cast<uintptr_t *>(*rbp);
        DwarfRegAndOffsetType info;
        bool found = LLVMStackMapParse::GetInstance().StackMapByAddr(returnAddr, info);
        if (found) {
            uintptr_t **address = nullptr;
            if (info.first == 7) {
                address = reinterpret_cast<uintptr_t **>(reinterpret_cast<uint8_t *>(rsp) + info.second);
            // rbp
            } else if (info.first == 6) {
                address = reinterpret_cast<uintptr_t **>(reinterpret_cast<uint8_t *>(rbp) + info.second);
            }
            std::cout << std::hex << "ref addr:" << address;
            std::cout << "  value:" << *address;
            std::cout << " *value :" << **address << std::endl;
        }
        std::cout << std::endl << std::endl;
        std::cout << std::hex << "+++++++++++++++++++ returnAddr : 0x" << returnAddr << " rbp:" << rbp
            << " rsp: " << rsp << std::endl;
    }
    std::cout << "do_safepoint +++ " << std::endl;
}
}
}  // namespace panda::test
