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
#include <unistd.h>

#include "gtest/gtest.h"
#include "ecmascript/builtins/builtins_promise_handler.h"
#include "ecmascript/compiler/fast_stub.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/compiler/llvm_mcjit_engine.h"
#include "ecmascript/compiler/llvm/llvm_stackmap_parser.h"
#include "ecmascript/compiler/scheduler.h"
#include "ecmascript/compiler/stub_descriptor.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/js_array.h"
#include "ecmascript/message_string.h"
#include "ecmascript/tests/test_helper.h"

#include "llvm/Support/SourceMgr.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/SourceMgr.h"

namespace panda::test {
using namespace panda::coretypes;
using namespace panda::ecmascript;
using namespace kungfu;
using BuiltinsPromiseHandler = builtins::BuiltinsPromiseHandler;

class StubTest : public testing::Test {
public:
    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
        stubModule.Initialize();
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    void PrintCircuitByBasicBlock(const std::vector<std::vector<AddrShift>> &cfg, const Circuit &netOfGates)
    {
        for (size_t bbIdx = 0; bbIdx < cfg.size(); bbIdx++) {
            std::cout << (netOfGates.GetOpCode(cfg[bbIdx].front()).IsCFGMerge() ? "MERGE_" : "BB_") << bbIdx << ":"
                    << std::endl;
            for (size_t instIdx = cfg[bbIdx].size(); instIdx > 0; instIdx--) {
                netOfGates.Print(cfg[bbIdx][instIdx - 1]);
            }
        }
    }

    PandaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
    LLVMStubModule stubModule {"fast_stub", "x86_64-unknown-linux-gnu"};
};

HWTEST_F_L0(StubTest, FastLoadElement)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetTestFunction(FAST_STUB_ID(FastLoadElement));
    Circuit netOfGates;
    FastArrayLoadElementStub optimizer(&netOfGates);
    optimizer.GenerateCircuit();
    netOfGates.PrintAllGates();
    bool result = Verifier::Run(&netOfGates);
    ASSERT_TRUE(result);
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function);
    llvmBuilder.Build();
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
    // Testcase build and run
    auto fn = reinterpret_cast<uint64_t (*)(JSArray *, int)>(assembler.GetFuncPtrFromCompiledModule(function));
    auto *factory = JSThread::Cast(thread)->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> values(factory->NewTaggedArray(5)); // 5 :  5 elements in array
    for (int i = 0; i < 5; i++) { // 5 :  5 elements in array
        values->Set(thread, i, JSTaggedValue(i));
    }
    JSHandle<JSObject> array(JSArray::CreateArrayFromList(thread, values));
    JSArray *arr = array.GetObject<JSArray>();
    auto valValid = fn(arr, 1);
    EXPECT_EQ(valValid, 0xffff000000000001);
    auto valUndefine = fn(arr, 6); // 6 : size of array
    EXPECT_EQ(valUndefine, 0xa);
}

class PhiStub : public Stub {
public:
    explicit PhiStub(Circuit *circuit) : Stub("Phi", 1, circuit) {}
    ~PhiStub() = default;
    NO_MOVE_SEMANTIC(PhiStub);
    NO_COPY_SEMANTIC(PhiStub);
    void GenerateCircuit() override
    {
        auto env = GetEnvironment();
        DEFVARIABLE(z, MachineType::INT32_TYPE, GetInteger32Constant(0));
        DEFVARIABLE(x, MachineType::INT32_TYPE, Int32Argument(0));
        Label ifTrue(env);
        Label ifFalse(env);
        Label next(env);
        Branch(Word32Equal(*x, GetInteger32Constant(10)), &ifTrue, &ifFalse);  // 10 : size of entry
        Bind(&ifTrue);
        {
            z = Int32Add(*x, GetInteger32Constant(10));  // 10 : size of entry
            Jump(&next);
        }
        Bind(&ifFalse);
        {
            z = Int32Add(*x, GetInteger32Constant(100));  // 100 : size of entry
            Jump(&next);
        }
        Bind(&next);
        Return(*z);
    }
};

HWTEST_F_L0(StubTest, PhiGateTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetTestFunction(FAST_STUB_ID(PhiGateTest));
    Circuit netOfGates;
    PhiStub optimizer(&netOfGates);
    optimizer.GenerateCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function);
    llvmBuilder.Build();
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
    // Testcase build and run
    auto fn = reinterpret_cast<int (*)(int)>(assembler.GetFuncPtrFromCompiledModule(function));
    auto valA = fn(3);  // 3 : size of array
    auto valB = fn(0);
    EXPECT_EQ(valA, 103); // 103 : eXpert res for fn(3)
    EXPECT_EQ(valB, 100); // 100 : eXpert res for fn(0)
}

class LoopStub : public Stub {
public:
    explicit LoopStub(Circuit *circuit) : Stub("loop", 1, circuit) {}
    ~LoopStub() = default;
    NO_MOVE_SEMANTIC(LoopStub);
    NO_COPY_SEMANTIC(LoopStub);
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

HWTEST_F_L0(StubTest, LoopTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetTestFunction(FAST_STUB_ID(LoopTest));
    Circuit netOfGates;
    LoopStub optimizer(&netOfGates);
    optimizer.GenerateCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, module, function);
    llvmBuilder.Build();
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
    // Testcase build and run
    auto fn = reinterpret_cast<int (*)(int)>(assembler.GetFuncPtrFromCompiledModule(function));
    auto valA = fn(1);
    auto valB = fn(9);    // 9 : size of array
    auto valC = fn(11);  // 11 : size of array
    EXPECT_EQ(valA, 109); // 109 : eXpert res for fn(1)
    EXPECT_EQ(valB, 19); // 10 : eXpert res for fn(9)
    EXPECT_EQ(valC, 0);
}

class LoopStub1 : public Stub {
public:
    explicit LoopStub1(Circuit *circuit) : Stub("loop1", 1, circuit) {}
    ~LoopStub1() = default;
    NO_MOVE_SEMANTIC(LoopStub1);
    NO_COPY_SEMANTIC(LoopStub1);
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

HWTEST_F_L0(StubTest, LoopTest1)
{
    LLVMModuleRef module = LLVMModuleCreateWithName("simple_module");
    LLVMSetTarget(module, "x86_64-unknown-linux-gnu");
    LLVMTypeRef paramTys[] = {
        LLVMInt32Type(),
    };
    LLVMValueRef function = LLVMAddFunction(module, "LoopTest1", LLVMFunctionType(LLVMInt32Type(), paramTys, 1, 0));
    Circuit netOfGates;
    LoopStub1 optimizer(&netOfGates);
    optimizer.GenerateCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, module, function);
    llvmBuilder.Build();
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
    auto engine = assembler.GetEngine();
    // Testcase build and run
    auto fn = reinterpret_cast<int (*)(int)>(LLVMGetPointerToGlobal(engine, function));
    auto valA = fn(1);
    auto valB = fn(9);    // 9 : size of array
    auto valC = fn(11);  // 11 : size of array
    EXPECT_EQ(valA, 10); // 10 : eXpert res for fn(1)
    EXPECT_EQ(valB, 10); // 10 : eXpert res for fn(9)
    EXPECT_EQ(valC, 11); // 10 : eXpert res for fn(11)
}

HWTEST_F_L0(StubTest, FastAddTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetStubFunction(FAST_STUB_ID(FastAdd));
    Circuit netOfGates;
    FastAddStub optimizer(&netOfGates);
    optimizer.GenerateCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function);
    llvmBuilder.Build();
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
    // Testcase build and run
    auto fn = reinterpret_cast<JSTaggedValue (*)(int64_t, int64_t)>(assembler.GetFuncPtrFromCompiledModule(function));
    auto resA = fn(JSTaggedValue(1).GetRawData(), JSTaggedValue(1).GetRawData());
    auto resB = fn(JSTaggedValue(2).GetRawData(), JSTaggedValue(2).GetRawData());     // 2 : test case
    auto resC = fn(JSTaggedValue(11).GetRawData(), JSTaggedValue(11).GetRawData());  // 11 : test case
    LOG_ECMA(INFO) << "res for FastAdd(1, 1) = " << resA.GetNumber();
    LOG_ECMA(INFO) << "res for FastAdd(2, 2) = " << resB.GetNumber();
    LOG_ECMA(INFO) << "res for FastAdd(11, 11) = " << resC.GetNumber();
    EXPECT_EQ(resA.GetNumber(), JSTaggedValue(2).GetNumber());
    EXPECT_EQ(resB.GetNumber(), JSTaggedValue(4).GetNumber());
    EXPECT_EQ(resC.GetNumber(), JSTaggedValue(22).GetNumber());
    int x1 = 2147483647;
    int y1 = 15;
    auto resG = fn(JSTaggedValue(x1).GetRawData(), JSTaggedValue(y1).GetRawData());
    auto expectedG = FastRuntimeStub::FastAdd(JSTaggedValue(x1), JSTaggedValue(y1));
    EXPECT_EQ(resG, expectedG);
}

HWTEST_F_L0(StubTest, FastSubTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetStubFunction(FAST_STUB_ID(FastSub));
    Circuit netOfGates;
    FastSubStub optimizer(&netOfGates);
    optimizer.GenerateCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function);
    llvmBuilder.Build();
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
    // Testcase build and run
    auto fn = reinterpret_cast<JSTaggedValue (*)(int64_t, int64_t)>(assembler.GetFuncPtrFromCompiledModule(function));
    auto resA = fn(JSTaggedValue(2).GetRawData(), JSTaggedValue(1).GetRawData());    // 2 : test case
    auto resB = fn(JSTaggedValue(7).GetRawData(), JSTaggedValue(2).GetRawData());    // 7, 2 : test cases
    auto resC = fn(JSTaggedValue(11).GetRawData(), JSTaggedValue(11).GetRawData());  // 11 : test case
    LOG_ECMA(INFO) << "res for FastSub(2, 1) = " << resA.GetNumber();
    LOG_ECMA(INFO) << "res for FastSub(7, 2) = " << resB.GetNumber();
    LOG_ECMA(INFO) << "res for FastSub(11, 11) = " << resC.GetNumber();
    EXPECT_EQ(resA, JSTaggedValue(1));
    EXPECT_EQ(resB, JSTaggedValue(5));
    EXPECT_EQ(resC, JSTaggedValue(0));
}

HWTEST_F_L0(StubTest, FastMulTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetStubFunction(FAST_STUB_ID(FastMul));
    Circuit netOfGates;
    FastMulStub optimizer(&netOfGates);
    optimizer.GenerateCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function);
    llvmBuilder.Build();
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
    // Testcase build and run
    auto fn = reinterpret_cast<JSTaggedValue (*)(int64_t, int64_t)>(assembler.GetFuncPtrFromCompiledModule(function));
    auto resA = fn(JSTaggedValue(-2).GetRawData(), JSTaggedValue(1).GetRawData());   // -2 : test case
    auto resB = fn(JSTaggedValue(-7).GetRawData(), JSTaggedValue(-2).GetRawData());  // -7, -2 : test case
    auto resC = fn(JSTaggedValue(11).GetRawData(), JSTaggedValue(11).GetRawData());  // 11 : test case
    LOG_ECMA(INFO) << "res for FastMul(-2, 1) = " << std::dec << resA.GetNumber();
    LOG_ECMA(INFO) << "res for FastMul(-7, -2) = " << std::dec << resB.GetNumber();
    LOG_ECMA(INFO) << "res for FastMul(11, 11) = " << std::dec << resC.GetNumber();
    int x = 7;
    double y = 1125899906842624;
    auto resD = fn(JSTaggedValue(x).GetRawData(), JSTaggedValue(y).GetRawData());
    JSTaggedValue expectedD = FastRuntimeStub::FastMul(JSTaggedValue(x), JSTaggedValue(y));
    EXPECT_EQ(resD, expectedD);
    x = -1;
    y = 1.7976931348623157e+308;
    auto resE = fn(JSTaggedValue(x).GetRawData(), JSTaggedValue(y).GetRawData());
    JSTaggedValue expectedE = FastRuntimeStub::FastMul(JSTaggedValue(x), JSTaggedValue(y));
    EXPECT_EQ(resE, expectedE);
    x = -1;
    y = -1 * std::numeric_limits<double>::infinity();
    auto resF = fn(JSTaggedValue(x).GetRawData(), JSTaggedValue(y).GetRawData());
    JSTaggedValue expectedF = FastRuntimeStub::FastMul(JSTaggedValue(x), JSTaggedValue(y));
    EXPECT_EQ(resF, expectedF);
    int x1 = 2147483647;
    int y1 = 15;
    auto resG = fn(JSTaggedValue(x1).GetRawData(), JSTaggedValue(y1).GetRawData());
    auto expectedG = FastRuntimeStub::FastMul(JSTaggedValue(x1), JSTaggedValue(y1));
    EXPECT_EQ(resG, expectedG);
}

HWTEST_F_L0(StubTest, FastDivTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetStubFunction(FAST_STUB_ID(FastDiv));
    Circuit netOfGates;
    FastDivStub optimizer(&netOfGates);
    optimizer.GenerateCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function);
    llvmBuilder.Build();
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
    auto fn = reinterpret_cast<JSTaggedValue (*)(int64_t, int64_t)>(assembler.GetFuncPtrFromCompiledModule(function));
    // test normal Division operation
    uint64_t x1 = JSTaggedValue(50).GetRawData();
    uint64_t y1 = JSTaggedValue(25).GetRawData();
    LOG_ECMA(INFO) << "x1 = " << x1 << "  y1 = " << y1;
    auto res1 = fn(x1, y1);
    LOG_ECMA(INFO) << "res for FastDiv(50, 25) = " << res1.GetRawData();
    auto expectedG1 = FastRuntimeStub::FastDiv(JSTaggedValue(x1), JSTaggedValue(y1));
    EXPECT_EQ(res1, expectedG1);

    // test x == 0.0 or std::isnan(x)
    uint64_t x2 = JSTaggedValue(base::NAN_VALUE).GetRawData();
    uint64_t y2 = JSTaggedValue(0).GetRawData();
    LOG_ECMA(INFO) << "x2 = " << x1 << "  y2 = " << y2;
    auto res2 = fn(x2, y2);
    LOG_ECMA(INFO) << "res for FastDiv(base::NAN_VALUE, 0) = " << res2.GetRawData();
    auto expectedG2 = FastRuntimeStub::FastDiv(JSTaggedValue(x2), JSTaggedValue(y2));
    EXPECT_EQ(res2, expectedG2);

    // test other
    uint64_t x3 = JSTaggedValue(7).GetRawData();
    uint64_t y3 = JSTaggedValue(0).GetRawData();
    LOG_ECMA(INFO) << "x2 = " << x3 << "  y2 = " << y3;
    auto res3 = fn(x3, y3);
    LOG_ECMA(INFO) << "res for FastDiv(7, 0) = " << res3.GetRawData();
    auto expectedG3 = FastRuntimeStub::FastDiv(JSTaggedValue(x3), JSTaggedValue(y3));
    EXPECT_EQ(res3, expectedG3);
}

HWTEST_F_L0(StubTest, FastModTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetStubFunction(FAST_STUB_ID(FastMod));
    Circuit netOfGates;
    FastModStub optimizer(&netOfGates);
    optimizer.GenerateCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function);
    llvmBuilder.Build();
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
    auto fn = reinterpret_cast<JSTaggedValue (*)(int64_t, int64_t)>(assembler.GetFuncPtrFromCompiledModule(function));

    // test left, right are all integer
    int x = 7;
    int y = 3;
    auto result = fn(JSTaggedValue(x).GetRawData(), JSTaggedValue(y).GetRawData());
    JSTaggedValue expectRes = FastRuntimeStub::FastMod(JSTaggedValue(x), JSTaggedValue(y));
    EXPECT_EQ(result, expectRes);

    // test y == 0.0 || std::isnan(y) || std::isnan(x) || std::isinf(x) return NAN_VALUE
    double x2 = 7.3;
    int y2 = base::NAN_VALUE;
    auto result2 = fn(JSTaggedValue(x2).GetRawData(), JSTaggedValue(y2).GetRawData());
    auto expectRes2 = FastRuntimeStub::FastMod(JSTaggedValue(x2), JSTaggedValue(y2));
    EXPECT_EQ(result2, expectRes2);
    LOG_ECMA(INFO) << "result2 for FastMod(7, 'helloworld') = " << result2.GetRawData();
    LOG_ECMA(INFO) << "expectRes2 for FastMod(7, 'helloworld') = " << expectRes2.GetRawData();

    // // test modular operation under normal conditions
    double x3 = 33.0;
    double y3 = 44.0;
    auto result3 = fn(JSTaggedValue(x3).GetRawData(), JSTaggedValue(y3).GetRawData());
    auto expectRes3 = FastRuntimeStub::FastMod(JSTaggedValue(x3), JSTaggedValue(y3));
    EXPECT_EQ(result3, expectRes3);

    // test x == 0.0 || std::isinf(y) return x
    double x4 = base::NAN_VALUE;
    int y4 = 7;
    auto result4 = fn(JSTaggedValue(x4).GetRawData(), JSTaggedValue(y4).GetRawData());
    auto expectRes4 = FastRuntimeStub::FastMod(JSTaggedValue(x4), JSTaggedValue(y4));

    LOG_ECMA(INFO) << "result4 for FastMod(base::NAN_VALUE, 7) = " << result4.GetRawData();
    LOG_ECMA(INFO) << "expectRes4 for FastMod(base::NAN_VALUE, 7) = " << expectRes4.GetRawData();
    EXPECT_EQ(result4, expectRes4);

    // test all non-conforming conditions
    int x5 = 7;
    auto *factory = JSThread::Cast(thread)->GetEcmaVM()->GetFactory();
    auto y5 = factory->NewFromStdString("hello world");
    auto result5 = fn(JSTaggedValue(x5).GetRawData(), y5.GetTaggedValue().GetRawData());
    EXPECT_EQ(result5, JSTaggedValue::Hole());
    auto expectRes5 = FastRuntimeStub::FastMod(JSTaggedValue(x5), y5.GetTaggedValue());
    LOG_ECMA(INFO) << "result1 for FastMod(7, 'helloworld') = " << result5.GetRawData();
    EXPECT_EQ(result5, expectRes5);
}

HWTEST_F_L0(StubTest, FastFindOwnElementStub)
{
    auto module = stubModule.GetModule();
    auto findFunction = stubModule.GetStubFunction(FAST_STUB_ID(FindOwnElement));
    Circuit netOfGates;
    FindOwnElementStub optimizer(&netOfGates);
    optimizer.GenerateCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, findFunction);
    llvmBuilder.Build();
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
}

HWTEST_F_L0(StubTest, FunctionCallInternal)
{
    auto module = stubModule.GetModule();
    auto findFunction = stubModule.GetStubFunction(FAST_STUB_ID(FunctionCallInternal));
    Circuit netOfGates;
    FunctionCallInternalStub optimizer(&netOfGates);
    optimizer.GenerateCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, findFunction);
    llvmBuilder.Build();
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
}

HWTEST_F_L0(StubTest, GetElementStub)
{
    auto module = stubModule.GetModule();
    auto findFunction = stubModule.GetStubFunction(FAST_STUB_ID(FindOwnElement));
    Circuit netOfGates;
    FindOwnElementStub findOptimizer(&netOfGates);
    findOptimizer.GenerateCircuit();
    auto cfg = Scheduler::Run(&netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, findFunction);
    llvmBuilder.Build();
    Circuit getNetOfGates;
    GetElementStub getOptimizer(&getNetOfGates);
    getOptimizer.GenerateCircuit();
    getNetOfGates.PrintAllGates();
    auto getCfg = Scheduler::Run(&getNetOfGates);
    PrintCircuitByBasicBlock(getCfg, getNetOfGates);
    llvmBuilder.Build();
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
}

HWTEST_F_L0(StubTest, FindOwnElement2Stub)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetStubFunction(FAST_STUB_ID(FindOwnElement2));
    Circuit netOfGates;
    FindOwnElement2Stub optimizer(&netOfGates);
    optimizer.GenerateCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function);
    llvmBuilder.Build();
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
    auto *findOwnElement2Ptr = reinterpret_cast<JSTaggedValue (*)(JSThread *, TaggedArray *, uint32_t, bool,
                                                                  PropertyAttributes *, uint32_t *)>(
        reinterpret_cast<uintptr_t>(assembler.GetFuncPtrFromCompiledModule(function)));
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
    assembler.Disassemble();
    JSTaggedValue resVal = findOwnElement2Ptr(thread, elements, 1, isDict, &attr, &indexOrEntry);
    EXPECT_EQ(resVal.GetNumber(), x);
    resVal = findOwnElement2Ptr(thread, elements, 10250, isDict, &attr, &indexOrEntry);
    EXPECT_EQ(resVal.GetNumber(), y);
}

HWTEST_F_L0(StubTest, SetElementStub)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetStubFunction(FAST_STUB_ID(SetElement));
    Circuit netOfGates;
    SetElementStub optimizer(&netOfGates);
    optimizer.GenerateCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function);
    llvmBuilder.Build();
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
}

struct ThreadTy {
    intptr_t magic;  // 0x11223344
    intptr_t fp;
};
class StubCallRunTimeThreadFpLock {
public:
    StubCallRunTimeThreadFpLock(struct ThreadTy *thread, intptr_t newFp) : oldRbp_(thread->fp), thread_(thread)
    {
        thread_->fp = *(reinterpret_cast<int64_t *>(newFp));
        std::cout << "StubCallRunTimeThreadFpLock newFp: " << newFp << " oldRbp_ : " << oldRbp_
                  << " thread_->fp:" << thread_->fp << std::endl;
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
    for (int i = 0; i < 40; i++) { // print 40 ptr value for debug
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
            gcFp = rbp - 2; // 2: 2 stack slot
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
#7  0x000055555561197c in panda::test::StubTest_JSEntryTest_Test::TestBody() ()
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

#ifdef ARK_GC_SUPPORT
HWTEST_F_L0(StubTest, JSEntryTest)
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
    LLVMTypeRef paramTys0[] = {threadTyPtr};
    /* implement stub1 */
    LLVMValueRef stub1 = LLVMAddFunction(module, "stub1", LLVMFunctionType(LLVMInt64Type(), paramTys0, 1, 0));
    LLVMAddTargetDependentFunctionAttr(stub1, "frame-pointer", "all");
    LLVMAddTargetDependentFunctionAttr(stub1, "js-stub-call", "1");

    LLVMBasicBlockRef entryBb = LLVMAppendBasicBlock(stub1, "entry");
    LLVMPositionBuilderAtEnd(builder, entryBb);

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
    char *error = nullptr;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);

    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
    auto engine = assembler.GetEngine();
    uint64_t stub1Code = LLVMGetFunctionAddress(engine, "stub1");
    uint64_t stub2Code = LLVMGetFunctionAddress(engine, "stub2");
    uint64_t stub3Code = LLVMGetFunctionAddress(engine, "stub3");
    std::map<uint64_t, std::string> addr2name = {{stub1Code, "stub1"}, {stub2Code, "stub2"}, {stub3Code, "stub3"}};
    std::cout << std::endl << " stub1Code : " << stub1Code << std::endl;
    std::cout << std::endl << " stub2Code : " << stub2Code << std::endl;
    std::cout << std::endl << " stub3Code : " << stub3Code << std::endl;
    assembler.Disassemble(addr2name);
    struct ThreadTy parameters = {0x0, 0x0};
    auto stub1Func = reinterpret_cast<int64_t (*)(struct ThreadTy *)>(stub1Code);
    g_stub2Func = reinterpret_cast<int64_t (*)(struct ThreadTy *)>(stub2Code);
    int64_t result = stub1Func(&parameters);
    std::cout << std::endl
              << "parameters magic:" << parameters.magic << " parameters.fp " << parameters.fp << std::endl;
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
HWTEST_F_L0(StubTest, Prologue)
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
    char *error = nullptr;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);

    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
    auto engine = assembler.GetEngine();
    uint64_t mainCode = LLVMGetFunctionAddress(engine, "main");
    assembler.Disassemble();
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
HWTEST_F_L0(StubTest, CEntryFp)
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
    LLVMTypeRef paramTys0[] = {threadTyPtr};
    /* implement main call RuntimeFunc */
    LLVMValueRef func = LLVMAddFunction(module, "main", LLVMFunctionType(LLVMInt64Type(), paramTys0, 1, 0));
    LLVMAddTargetDependentFunctionAttr(func, "frame-pointer", "all");
    LLVMAddTargetDependentFunctionAttr(func, "js-stub-call", "1");
    LLVMBasicBlockRef entryBb = LLVMAppendBasicBlock(func, "entry");
    LLVMPositionBuilderAtEnd(builder, entryBb);

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
    char *error = nullptr;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);

    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
    auto engine = assembler.GetEngine();
    uint64_t nativeCode = LLVMGetFunctionAddress(engine, "main");
    std::cout << std::endl << " nativeCode : " << nativeCode << std::endl;
    assembler.Disassemble();
    struct ThreadTy parameters = {0x0, 0x0};

    auto mainFunc = reinterpret_cast<int64_t (*)(struct ThreadTy *)>(nativeCode);
    int64_t result = mainFunc(&parameters);
    EXPECT_EQ(result, 1);
    std::cout << " ++++++++++ CEntryFp +++++++++++++ " << std::endl;
}

HWTEST_F_L0(StubTest, LoadGCIRTest)
{
    std::cout << "--------------LoadGCIRTest--------------------" << std::endl;
    char *path = get_current_dir_name();
    std::string filePath = std::string(path) + "/ark/js_runtime/ecmascript/compiler/tests/satepoint_GC_0.ll";

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
    std::unique_ptr<llvm::Module> rawModule = parseIRFile(inputFilename, err, context);
    if (!rawModule) {
        std::cout << "parseIRFile :" << inputFilename.data() << " failed !" << std::endl;
        return;
    }
    LLVMModuleRef module = LLVMCloneModule(wrap(rawModule.get()));
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
    auto engine = assembler.GetEngine();
    LLVMValueRef function = LLVMGetNamedFunction(module, "main");
    LLVMDumpValue(function);

    auto *mainPtr = reinterpret_cast<int (*)()>(LLVMGetPointerToGlobal(engine, function));
    uint8_t *ptr = assembler.GetStackMapsSection();
    LLVMStackMapParser::GetInstance().CalculateStackMap(ptr);
    LLVMStackMapParser::GetInstance().Print();

    assembler.Disassemble();
    LLVMDumpModule(module);
    int value = reinterpret_cast<int (*)()>(mainPtr)();
    std::cout << " value:" << value << std::endl;
}
#endif

extern "C" {
void DoSafepoint()
{
    uintptr_t *rbp;
    asm("mov %%rbp, %0" : "=rm" (rbp));
    for (int i = 0; i < 3; i++) { // 3: call back depth
        uintptr_t returnAddr =  *(rbp + 1);
        uintptr_t *rsp = rbp + 2;  // move 2 steps from rbp to get rsp
        rbp = reinterpret_cast<uintptr_t *>(*rbp);
        DwarfRegAndOffsetTypeVector infos;
        bool found = LLVMStackMapParser::GetInstance().StackMapByAddr(returnAddr, infos);
        if (found) {
            for (auto &info : infos) {
                uintptr_t **address = nullptr;
                if (info.first == SP_DWARF_REG_NUM) {
                    address = reinterpret_cast<uintptr_t **>(reinterpret_cast<uint8_t *>(rsp) + info.second);
                } else if (info.first == FP_DWARF_REG_NUM) {
                    address = reinterpret_cast<uintptr_t **>(reinterpret_cast<uint8_t *>(rbp) + info.second);
                }
                // print ref and vlue for debug
                std::cout << std::hex << "ref addr:" << address;
                std::cout << "  value:" << *address;
                std::cout << " *value :" << **address << std::endl;
            }
        }
        std::cout << std::endl << std::endl;
        std::cout << std::hex << "+++++++++++++++++++ returnAddr : 0x" << returnAddr << " rbp:" << rbp
                  << " rsp: " << rsp << std::endl;
    }
    std::cout << "do_safepoint +++ " << std::endl;
}
}

#ifdef NDEBUG
HWTEST_F_L0(StubTest, GetPropertyByIndexStub)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetStubFunction(FAST_STUB_ID(GetPropertyByIndex));
    Circuit netOfGates;
    GetPropertyByIndexStub optimizer(&netOfGates);
    optimizer.GenerateCircuit();
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function);
    llvmBuilder.Build();
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
    auto *getpropertyByIndex = reinterpret_cast<JSTaggedValue (*)(JSThread *, JSTaggedValue, uint32_t)>(
        reinterpret_cast<uintptr_t>(assembler.GetFuncPtrFromCompiledModule(function)));
    auto *factory = JSThread::Cast(thread)->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    int x = 213;
    int y = 10;
    FastRuntimeStub::SetOwnElement(thread, obj.GetTaggedValue(), 1, JSTaggedValue(x));
    FastRuntimeStub::SetOwnElement(thread, obj.GetTaggedValue(), 10250, JSTaggedValue(y));
    assembler.Disassemble();
    JSTaggedValue resVal = getpropertyByIndex(thread, obj.GetTaggedValue(), 1);
    EXPECT_EQ(resVal.GetNumber(), x);
    resVal = getpropertyByIndex(thread, obj.GetTaggedValue(), 10250);
    EXPECT_EQ(resVal.GetNumber(), y);
}

HWTEST_F_L0(StubTest, SetPropertyByIndexStub)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetStubFunction(FAST_STUB_ID(SetPropertyByIndex));
    Circuit netOfGates;
    SetPropertyByIndexStub optimizer(&netOfGates);
    optimizer.GenerateCircuit();
    netOfGates.PrintAllGates();
    bool result = Verifier::Run(&netOfGates);
    ASSERT_TRUE(result);
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function);
    llvmBuilder.Build();
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
    auto *setpropertyByIndex = reinterpret_cast<JSTaggedValue (*)(JSThread *, JSTaggedValue, uint32_t, JSTaggedValue)>(
        reinterpret_cast<uintptr_t>(assembler.GetFuncPtrFromCompiledModule(function)));
    auto *factory = JSThread::Cast(thread)->GetEcmaVM()->GetFactory();
    JSHandle<JSArray> array = factory->NewJSArray();
    assembler.Disassemble();
    // set value to array
    array->SetArrayLength(thread, 20);
    for (int i = 0; i < 20; i++) {
        auto taggedArray = array.GetTaggedValue();
        setpropertyByIndex(thread, taggedArray, i, JSTaggedValue(i));
    }
    for (int i = 0; i < 20; i++) {
        EXPECT_EQ(JSTaggedValue(i),
                  JSArray::FastGetPropertyByValue(thread, JSHandle<JSTaggedValue>::Cast(array), i).GetTaggedValue());
    }
}
#endif

HWTEST_F_L0(StubTest, GetPropertyByNameStub)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetStubFunction(FAST_STUB_ID(GetPropertyByName));
    Circuit netOfGates;
    GetPropertyByNameStub optimizer(&netOfGates);
    optimizer.GenerateCircuit();
    netOfGates.PrintAllGates();
    bool result = Verifier::Run(&netOfGates);
    ASSERT_TRUE(result);
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function);
    llvmBuilder.Build();
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
    auto *getPropertyByNamePtr = reinterpret_cast<JSTaggedValue (*)(JSThread *, uint64_t, uint64_t)>(
        reinterpret_cast<uintptr_t>(assembler.GetFuncPtrFromCompiledModule(function)));
    auto *factory = JSThread::Cast(thread)->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    int x = 213;
    int y = 10;
    JSHandle<JSTaggedValue> strA(factory->NewFromCanBeCompressString("a"));
    JSHandle<JSTaggedValue> strBig(factory->NewFromCanBeCompressString("biggest"));
    FastRuntimeStub::SetPropertyByName(thread, obj.GetTaggedValue(), strA.GetTaggedValue(), JSTaggedValue(x));
    FastRuntimeStub::SetPropertyByName(thread, obj.GetTaggedValue(), strBig.GetTaggedValue(), JSTaggedValue(y));
    assembler.Disassemble();
    JSTaggedValue resVal = getPropertyByNamePtr(thread, obj.GetTaggedValue().GetRawData(),
        strA.GetTaggedValue().GetRawData());
    EXPECT_EQ(resVal.GetNumber(), x);
    resVal = getPropertyByNamePtr(thread, obj.GetTaggedValue().GetRawData(), strBig.GetTaggedValue().GetRawData());
    EXPECT_EQ(resVal.GetNumber(), y);
}

HWTEST_F_L0(StubTest, FastTypeOfTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetStubFunction(FAST_STUB_ID(FastMod));
    Circuit netOfGates;
    FastTypeOfStub optimizer(&netOfGates);
    optimizer.GenerateCircuit();
    netOfGates.PrintAllGates();
    bool verRes = Verifier::Run(&netOfGates);
    ASSERT_TRUE(verRes);
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function);
    llvmBuilder.Build();
    char *error = nullptr;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
    LLVMAssembler assembler(module, "x86_64-unknown-linux-gnu");
    assembler.Run();
    LLVMDumpModule(module);
    auto *typeOfPtr =
        reinterpret_cast<JSTaggedValue (*)(JSThread *, uint64_t)>(assembler.GetFuncPtrFromCompiledModule(function));
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    // obj is JSTaggedValue::VALUE_TRUE
    JSTaggedValue resultVal = typeOfPtr(thread, JSTaggedValue::True().GetRawData());
    JSTaggedValue expectResult = FastRuntimeStub::FastTypeOf(thread, JSTaggedValue::True());
    EXPECT_EQ(resultVal, globalConst->GetBooleanString());
    EXPECT_EQ(resultVal, expectResult);

    // obj is JSTaggedValue::VALUE_FALSE
    JSTaggedValue resultVal2 = typeOfPtr(thread, JSTaggedValue::False().GetRawData());
    JSTaggedValue expectResult2 = FastRuntimeStub::FastTypeOf(thread, JSTaggedValue::False());
    EXPECT_EQ(resultVal2, globalConst->GetBooleanString());
    EXPECT_EQ(resultVal2, expectResult2);

    // obj is JSTaggedValue::VALUE_NULL
    JSTaggedValue resultVal3 = typeOfPtr(thread, JSTaggedValue::Null().GetRawData());
    JSTaggedValue expectResult3 = FastRuntimeStub::FastTypeOf(thread, JSTaggedValue::Null());
    EXPECT_EQ(resultVal3, globalConst->GetObjectString());
    EXPECT_EQ(resultVal3, expectResult3);

    // obj is JSTaggedValue::VALUE_UNDEFINED
    JSTaggedValue resultVal4 = typeOfPtr(thread, JSTaggedValue::Undefined().GetRawData());
    JSTaggedValue expectResult4 = FastRuntimeStub::FastTypeOf(thread, JSTaggedValue::Undefined());
    EXPECT_EQ(resultVal4, globalConst->GetUndefinedString());
    EXPECT_EQ(resultVal4, expectResult4);

    // obj is IsNumber
    JSTaggedValue resultVal5 = typeOfPtr(thread, JSTaggedValue(5).GetRawData());
    JSTaggedValue expectResult5 = FastRuntimeStub::FastTypeOf(thread, JSTaggedValue(5));
    EXPECT_EQ(resultVal5, globalConst->GetNumberString());
    EXPECT_EQ(resultVal5, expectResult5);

    // obj is String
    auto *factory = JSThread::Cast(thread)->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> str1 = factory->NewFromStdString("a");
    JSHandle<EcmaString> str2 = factory->NewFromStdString("a");
    JSTaggedValue expectResult6 = FastRuntimeStub::FastTypeOf(thread, str1.GetTaggedValue());
    JSTaggedValue resultVal6 = typeOfPtr(thread, str2.GetTaggedValue().GetRawData());
    EXPECT_EQ(resultVal6, globalConst->GetStringString());
    EXPECT_EQ(resultVal6, expectResult6);

    // obj is Symbol
    JSHandle<GlobalEnv> globalEnv = JSThread::Cast(thread)->GetEcmaVM()->GetGlobalEnv();
    JSTaggedValue symbol = globalEnv->GetIteratorSymbol().GetTaggedValue();
    JSTaggedValue expectResult7= FastRuntimeStub::FastTypeOf(thread, symbol);
    JSTaggedValue resultVal7 = typeOfPtr(thread, symbol.GetRawData());
    EXPECT_EQ(resultVal7, globalConst->GetSymbolString());
    EXPECT_EQ(resultVal7, expectResult7);

    // obj is callable
    JSHandle<JSPromiseReactionsFunction> resolveCallable =
        factory->CreateJSPromiseReactionsFunction(reinterpret_cast<void *>(BuiltinsPromiseHandler::Resolve));
    JSTaggedValue expectResult8= FastRuntimeStub::FastTypeOf(thread, resolveCallable.GetTaggedValue());
    JSTaggedValue resultVal8 = typeOfPtr(thread, resolveCallable.GetTaggedValue().GetRawData());
    EXPECT_EQ(resultVal8, globalConst->GetFunctionString());
    EXPECT_EQ(resultVal8, expectResult8);

    // obj is heapObject
    JSHandle<JSObject> object = factory->NewEmptyJSObject();
    JSTaggedValue expectResult9= FastRuntimeStub::FastTypeOf(thread, object.GetTaggedValue());
    JSTaggedValue resultVal9 = typeOfPtr(thread, object.GetTaggedValue().GetRawData());
    EXPECT_EQ(resultVal9, globalConst->GetObjectString());
    EXPECT_EQ(resultVal9, expectResult9);
}
}  // namespace panda::test
