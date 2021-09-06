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

    PandaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
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
    Environment fastEnv("FastLoadElement", 2); // 2 : parameter count
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

        Branch(Word32Equal(*x, GetInteger32Constant(10)), &ifTrue, &ifFalse); // 10 : size of entry
        Bind(&ifTrue);
        z = Int32Add(*x, GetInteger32Constant(10)); // 10 : size of entry
        Jump(&next);
        Bind(&ifFalse);  // else
        z = Int32Add(*x, GetInteger32Constant(100)); // 100 : size of entry
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
    LLVMValueRef function =
        LLVMAddFunction(module, "PhiGateTest", LLVMFunctionType(LLVMInt32Type(), paramTys, 1, 0));
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
    auto val = fn(3); // 3 : size of array
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
        AddrShift callFoo = CallStub(&phi_descriptor_, GetWord64Constant(FAST_STUB_ID(PhiTest)), {*x});
        Return(Int32Add(callFoo, GetInteger32Constant(1)));
    }

private:
    StubInterfaceDescriptor phi_descriptor_;
};

HWTEST_F_L0(StubOptimizerTest, CallPhiGateTest)
{
    LLVMModuleRef module = static_cast<LLVMModuleRef>(FastStubs::GetInstance().GetModule());
    LLVMValueRef function =  LLVMGetNamedFunction(module, "PhiTest");
    LLVMTypeRef barParamTys[] = {
        LLVMInt32Type(),
    };
    LLVMValueRef barfunction = LLVMAddFunction(module, "CallPhiGateTest_bar",
        LLVMFunctionType(LLVMInt32Type(), barParamTys, 1, 0));
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
    auto *phiTestPointer = reinterpret_cast<int (*)(int)>(
        reinterpret_cast<uintptr_t>(LLVMGetPointerToGlobal(engine, function)));
    LLVMAddGlobalMapping(engine, function, reinterpret_cast<void *>(phiTestPointer));
    auto *barPointerbar = reinterpret_cast<int (*)(int)>(
        reinterpret_cast<uintptr_t>(LLVMGetPointerToGlobal(engine, barfunction)));
    std::cout << std::hex << "phiTestPointer:" << reinterpret_cast<uintptr_t>(phiTestPointer) <<
        " barPointerbar:" << reinterpret_cast<uintptr_t>(barPointerbar) << std::endl;
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
        Branch(Int32LessThan(*y, GetInteger32Constant(10)), &loopHead, &afterLoop); // 10 : size of entry
        LoopBegin(&loopHead);
        Label ifTrue(env);
        Label ifFalse(env);
        Label next(env);
        Branch(Word32Equal(Int32Argument(0), GetInteger32Constant(9)), &ifTrue, &ifFalse); // 9 : size of entry
        Bind(&ifTrue);
        z = Int32Add(*y, GetInteger32Constant(10)); // 10 : size of entry
        y = Int32Add(*z, GetInteger32Constant(1));
        Jump(&next);
        Bind(&ifFalse);
        z = Int32Add(*y, GetInteger32Constant(100)); // 100 : size of entry
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
    auto resValid2 = fn(9); // 9 : size of array
    auto resInvalid = fn(11); // 11 : size of array
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
        Branch(Int32LessThan(*y, GetInteger32Constant(10)), &loopHead, &afterLoop); // 10 : size of entry
        LoopBegin(&loopHead);
        x = Int32Add(*z, GetInteger32Constant(3));   // 3 : size of entry
        Label ifTrue(env);
        Label next(env);
        Branch(Word32Equal(*x, GetInteger32Constant(9)), &ifTrue, &next);   // 9 : size of entry
        Bind(&ifTrue);
        y = Int32Add(*z, *x);
        Jump(&next);
        Bind(&next);
        y = Int32Add(*y, GetInteger32Constant(1));
        Branch(Int32LessThan(*y, GetInteger32Constant(10)), &loopEnd, &afterLoop);   // 10 : size of entry
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
    auto resValid2 = fn(9); // 9 : size of array
    auto resInvalid = fn(11); // 11 : size of array
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
        AddrShift doubleY = TaggedCastToDouble(y);
        AddrShift doubleX = CastInt32ToFloat64(intX);
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
        doubleX = DoubleAdd(doubleX, doubleY);
        Return(DoubleBuildTagged(doubleX));
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
    auto resValid2 = fn(JSTaggedValue(2).GetRawData(), JSTaggedValue(2).GetRawData()); // 2 : test case
    auto resInvalid = fn(JSTaggedValue(11).GetRawData(), JSTaggedValue(11).GetRawData()); // 11 : test case
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
        AddrShift doubleY = TaggedCastToDouble(y);
        AddrShift doubleX = CastInt32ToFloat64(intX);
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
        doubleX = DoubleSub(doubleX, doubleY);
        Return(DoubleBuildTagged(doubleX));
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
    Environment env("FastSub", 2); // 2 : parameter
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
    auto resA = fn(JSTaggedValue(2).GetRawData(), JSTaggedValue(1).GetRawData()); // 2 : test case
    auto resB = fn(JSTaggedValue(7).GetRawData(), JSTaggedValue(2).GetRawData()); // 7, 2 : test cases
    auto resC = fn(JSTaggedValue(11).GetRawData(), JSTaggedValue(11).GetRawData()); // 11 : test case
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
        AddrShift doubleY = TaggedCastToDouble(y);
        AddrShift doubleX = CastInt32ToFloat64(intX);
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
        doubleX = DoubleMul(doubleX, doubleY);
        Return(DoubleBuildTagged(doubleX));
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
    Environment env("FastMul", 2); // 2 : parameter count
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
    auto resA = fn(JSTaggedValue(-2).GetRawData(), JSTaggedValue(1).GetRawData()); // -2 : test case
    auto resB = fn(JSTaggedValue(-7).GetRawData(), JSTaggedValue(-2).GetRawData()); // -7, -2 : test case
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
        AddrShift doubleX = CastInt32ToFloat64(intX);
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
        AddrShift doubleY = CastInt32ToFloat64(intY);
        Jump(&next3);
        Bind(&ifFalse4);
        doubleY = TaggedCastToDouble(y);
        Jump(&next3);
        Bind(&next3);
        Branch(DoubleEqual(doubleY, GetDoubleConstant(0.0)), &ifTrue5, &ifFalse5);
        Bind(&ifTrue5);
        // dLeft == 0.0 || std::isnan(dLeft)
        Branch(TruncInt64ToInt1(Word64Or(SExtInt1ToInt64(DoubleEqual(doubleX, GetDoubleConstant(0.0))),
            SExtInt32ToInt64(DoubleIsNAN(doubleX)))), &ifTrue6, &ifFalse6);
        Bind(&ifTrue6);
        Return(DoubleBuildTagged(GetDoubleConstant(base::NAN_VALUE)));
        Bind(&ifFalse6);
        Jump(&next4);
        Bind(&next4);
        AddrShift taggedX = CastDoubleToInt64(doubleX);
        AddrShift taggedY = CastDoubleToInt64(doubleY);
        taggedX = Word64And(Word64Or(taggedX, taggedY), GetWord64Constant(base::DOUBLE_SIGN_MASK));
        taggedX = Word64Or(taggedX, CastDoubleToInt64(GetDoubleConstant(base::POSITIVE_INFINITY)));
        doubleX = TaggedCastToDouble(taggedX);
        Return(DoubleBuildTagged(doubleX));
        Bind(&ifFalse5);
        doubleX = DoubleDiv(doubleX, doubleY);
        Return(DoubleBuildTagged(doubleX));
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
        AddrShift callFindOwnElementVal = CallStub(&findOwnElementDescriptor,
            GetWord64Constant(FAST_STUB_ID(FindOwnElement)), {objPtr, index});
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
        AddrShift elements = PtrArgument(0);
        AddrShift index = Int32Argument(1);
        AddrShift isDict = Int32Argument(2); // 2 : 3rd argument
        AddrShift attr = PtrArgument(3); // 3 : 4th argument
        AddrShift indexOrEntry = PtrArgument(4); // 4 : 5th argument
        isDict = ZExtInt1ToInt32(isDict);
        Label notDictionary(env);
        Label isDictionary(env);
        Label end(env);
        Branch(Word32Equal(isDict, GetInteger32Constant(0)), &notDictionary, &isDictionary);
        Bind(&notDictionary);
        {
            AddrShift elementsLength = Load(UINT32_TYPE, elements,
                GetPtrConstant(panda::coretypes::Array::GetLengthOffset()));
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
            AddrShift entry = FindElementFromNumberDictionary(elements, IntBuildTagged(index), &afterFindElement);
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
        AddrShift index = Int32Argument(2); // 2 : 3rd argument
        AddrShift value = Int64Argument(3); // 3 : 4th argument
        AddrShift mayThrow = Int32Argument(4); // 4 : 5th argument
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
                                     {elements, index, isDictionary, pattr, pindexOrEntry});
            Label notHole(env);
            Label isHole(env);
            Branch(TaggedIsNotHole(val), &notHole, &isHole);
            Bind(&notHole);
            {
                Label isOnProtoType(env);
                Label notOnProtoType(env);
                Label afterOnProtoType(env);
                Branch(*onPrototype, &isOnProtoType, &notOnProtoType);
                Bind(&isOnProtoType);
                Jump(&afterOnProtoType);
                Bind(&notOnProtoType);
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
                    Return(CallStub(addElementInternal, GetWord64Constant(FAST_STUB_ID(AddElementInternal)),
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
                                UpdateRepresention(LoadHClass(receiver), value);
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
                        Return(CallStub(callsetter, GetWord64Constant(FAST_STUB_ID(CallSetter)),
                            { thread, setter, receiver, value, TruncInt32ToInt1(mayThrow) }));
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
                        Return(CallStub(addElementInternal, GetWord64Constant(FAST_STUB_ID(AddElementInternal)),
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
                        Return(CallStub(setProperty, GetWord64Constant(FAST_STUB_ID(JSProxySetProperty)),
                            { thread, *holder, IntBuildTagged(index), value, receiver, TruncInt32ToInt1(mayThrow) }));
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
    Environment env("FindOwnElement2", 5);
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
    std::cout << " ++++++++++++++++++++FastFindOwnElement2Stub ++++++++++++++++++" << std::endl;
}
}  // namespace panda::test
