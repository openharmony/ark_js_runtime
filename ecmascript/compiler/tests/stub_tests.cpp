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
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/llvm_codegen.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/compiler/llvm/llvm_stackmap_parser.h"
#include "ecmascript/compiler/scheduler.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/compiler/verifier.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/js_array.h"
#include "ecmascript/message_string.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/trampoline/trampoline.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/SourceMgr.h"

namespace panda::test {
using namespace panda::coretypes;
using namespace panda::ecmascript;
using namespace panda::ecmascript::kungfu;
using BuiltinsPromiseHandler = builtins::BuiltinsPromiseHandler;

class StubTest : public testing::Test {
public:
    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
        BytecodeStubCSigns::Initialize();
        CommonStubCSigns::Initialize();
        RuntimeStubCSigns::Initialize();
        stubModule.SetUpForCommonStubs();
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    void PrintCircuitByBasicBlock([[maybe_unused]] const std::vector<std::vector<GateRef>> &cfg,
                                  [[maybe_unused]] const Circuit &netOfGates) const
    {
        if (thread->GetEcmaVM()->GetJSOptions().WasSetlogCompiledMethods()) {
            for (size_t bbIdx = 0; bbIdx < cfg.size(); bbIdx++) {
                COMPILER_LOG(INFO) << (netOfGates.GetOpCode(cfg[bbIdx].front()).IsCFGMerge() ? "MERGE_" : "BB_")
                                   << bbIdx << ":";
                for (size_t instIdx = cfg[bbIdx].size(); instIdx > 0; instIdx--) {
                    netOfGates.Print(cfg[bbIdx][instIdx - 1]);
                }
            }
        }
    }

    JSTaggedValue NewAotFunction(uint32_t numArgs, uintptr_t codeEntry)
    {
        auto ecmaVm = thread->GetEcmaVM();
        ObjectFactory *factory = ecmaVm->GetFactory();
        JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
        JSMethod *method = ecmaVm->GetMethodForNativeFunction(reinterpret_cast<void *>(codeEntry));
        method->SetAotCodeBit(true);
        method->SetNativeBit(false);
        method->SetNumArgsWithCallField(numArgs);
        JSHandle<JSFunction> jsfunc = factory->NewJSFunction(env, method, FunctionKind::NORMAL_FUNCTION);
        jsfunc->SetCodeEntry(reinterpret_cast<uintptr_t>(codeEntry));
        return jsfunc.GetTaggedValue();
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
    LLVMModule stubModule {"stub_tests", "x86_64-unknown-linux-gnu"};
};

HWTEST_F_L0(StubTest, FastAddTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::Add);
    Circuit netOfGates;
    AddStub optimizer(&netOfGates);
    optimizer.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run();
    // Testcase build and run
    auto fn = reinterpret_cast<JSTaggedValue (*)(uintptr_t, int64_t, int64_t)>(
        assembler.GetFuncPtrFromCompiledModule(function));
    auto resA = fn(thread->GetGlueAddr(), JSTaggedValue(1).GetRawData(),
        JSTaggedValue(1).GetRawData());
    auto resB = fn(thread->GetGlueAddr(), JSTaggedValue(2).GetRawData(),
        JSTaggedValue(2).GetRawData());     // 2 : test case
    auto resC = fn(thread->GetGlueAddr(), JSTaggedValue(11).GetRawData(),
        JSTaggedValue(11).GetRawData());  // 11 : test case
    COMPILER_LOG(INFO) << "res for FastAdd(1, 1) = " << resA.GetNumber();
    COMPILER_LOG(INFO) << "res for FastAdd(2, 2) = " << resB.GetNumber();
    COMPILER_LOG(INFO) << "res for FastAdd(11, 11) = " << resC.GetNumber();
    EXPECT_EQ(resA.GetNumber(), JSTaggedValue(2).GetNumber());
    EXPECT_EQ(resB.GetNumber(), JSTaggedValue(4).GetNumber());
    EXPECT_EQ(resC.GetNumber(), JSTaggedValue(22).GetNumber());
    int x1 = 2147483647;
    int y1 = 15;
    auto resG = fn(thread->GetGlueAddr(), JSTaggedValue(x1).GetRawData(), JSTaggedValue(y1).GetRawData());
    auto expectedG = FastRuntimeStub::FastAdd(JSTaggedValue(x1), JSTaggedValue(y1));
    EXPECT_EQ(resG, expectedG);
}

HWTEST_F_L0(StubTest, FastSubTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::Sub);
    Circuit netOfGates;
    SubStub optimizer(&netOfGates);
    optimizer.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run();
    // Testcase build and run
    auto fn = reinterpret_cast<JSTaggedValue (*)(uintptr_t, int64_t, int64_t)>(
        assembler.GetFuncPtrFromCompiledModule(function));
    auto resA = fn(thread->GetGlueAddr(), JSTaggedValue(2).GetRawData(),
        JSTaggedValue(1).GetRawData());    // 2 : test case
    auto resB = fn(thread->GetGlueAddr(), JSTaggedValue(7).GetRawData(),
        JSTaggedValue(2).GetRawData());    // 7, 2 : test cases
    auto resC = fn(thread->GetGlueAddr(), JSTaggedValue(11).GetRawData(),
        JSTaggedValue(11).GetRawData());  // 11 : test case
    COMPILER_LOG(INFO) << "res for FastSub(2, 1) = " << resA.GetNumber();
    COMPILER_LOG(INFO) << "res for FastSub(7, 2) = " << resB.GetNumber();
    COMPILER_LOG(INFO) << "res for FastSub(11, 11) = " << resC.GetNumber();
    EXPECT_EQ(resA, JSTaggedValue(1));
    EXPECT_EQ(resB, JSTaggedValue(5));
    EXPECT_EQ(resC, JSTaggedValue(0));
}


HWTEST_F_L0(StubTest, FastMulTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::Mul);
    Circuit netOfGates;
    MulStub optimizer(&netOfGates);
    optimizer.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run();
    // Testcase build and run
    auto fn = reinterpret_cast<JSTaggedValue (*)(uintptr_t, int64_t, int64_t)>(
        assembler.GetFuncPtrFromCompiledModule(function));
    auto resA = fn(thread->GetGlueAddr(), JSTaggedValue(-2).GetRawData(),
        JSTaggedValue(1).GetRawData());   // -2 : test case
    auto resB = fn(thread->GetGlueAddr(), JSTaggedValue(-7).GetRawData(),
        JSTaggedValue(-2).GetRawData());  // -7, -2 : test case
    auto resC = fn(thread->GetGlueAddr(), JSTaggedValue(11).GetRawData(),
        JSTaggedValue(11).GetRawData());  // 11 : test case
    COMPILER_LOG(INFO) << "res for FastMul(-2, 1) = " << std::dec << resA.GetNumber();
    COMPILER_LOG(INFO) << "res for FastMul(-7, -2) = " << std::dec << resB.GetNumber();
    COMPILER_LOG(INFO) << "res for FastMul(11, 11) = " << std::dec << resC.GetNumber();
    EXPECT_EQ(resA.GetNumber(), -2); // -2: test case
    EXPECT_EQ(resB.GetNumber(), 14); // 14: test case
    EXPECT_EQ(resC.GetNumber(), 121); // 121: test case
    int x = 7;
    double y = 1125899906842624;
    auto resD = fn(thread->GetGlueAddr(), JSTaggedValue(x).GetRawData(), JSTaggedValue(y).GetRawData());
    JSTaggedValue expectedD = FastRuntimeStub::FastMul(JSTaggedValue(x), JSTaggedValue(y));
    EXPECT_EQ(resD, expectedD);
    x = -1;
    y = 1.7976931348623157e+308;
    auto resE = fn(thread->GetGlueAddr(), JSTaggedValue(x).GetRawData(), JSTaggedValue(y).GetRawData());
    JSTaggedValue expectedE = FastRuntimeStub::FastMul(JSTaggedValue(x), JSTaggedValue(y));
    EXPECT_EQ(resE, expectedE);
    x = -1;
    y = -1 * std::numeric_limits<double>::infinity();
    auto resF = fn(thread->GetGlueAddr(), JSTaggedValue(x).GetRawData(), JSTaggedValue(y).GetRawData());
    JSTaggedValue expectedF = FastRuntimeStub::FastMul(JSTaggedValue(x), JSTaggedValue(y));
    EXPECT_EQ(resF, expectedF);
    int x1 = 2147483647;
    int y1 = 15;
    auto resG = fn(thread->GetGlueAddr(), JSTaggedValue(x1).GetRawData(), JSTaggedValue(y1).GetRawData());
    auto expectedG = FastRuntimeStub::FastMul(JSTaggedValue(x1), JSTaggedValue(y1));
    EXPECT_EQ(resG, expectedG);
}

HWTEST_F_L0(StubTest, FastDivTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::Div);
    Circuit netOfGates;
    DivStub optimizer(&netOfGates);
    optimizer.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run();
    auto fn = reinterpret_cast<JSTaggedValue (*)(uintptr_t, int64_t, int64_t)>(
        assembler.GetFuncPtrFromCompiledModule(function));
    // test normal Division operation
    uint64_t x1 = JSTaggedValue(50).GetRawData();
    uint64_t y1 = JSTaggedValue(25).GetRawData();
    COMPILER_LOG(INFO) << "x1 = " << x1 << "  y1 = " << y1;
    auto res1 = fn(thread->GetGlueAddr(), x1, y1);
    COMPILER_LOG(INFO) << "res for FastDiv(50, 25) = " << res1.GetRawData();
    auto expectedG1 = FastRuntimeStub::FastDiv(JSTaggedValue(x1), JSTaggedValue(y1));
    EXPECT_EQ(res1, expectedG1);

    // test x == 0.0 or std::isnan(x)
    uint64_t x2 = JSTaggedValue(base::NAN_VALUE).GetRawData();
    uint64_t y2 = JSTaggedValue(0).GetRawData();
    COMPILER_LOG(INFO) << "x2 = " << x1 << "  y2 = " << y2;
    auto res2 = fn(thread->GetGlueAddr(), x2, y2);
    COMPILER_LOG(INFO) << "res for FastDiv(base::NAN_VALUE, 0) = " << res2.GetRawData();
    auto expectedG2 = FastRuntimeStub::FastDiv(JSTaggedValue(x2), JSTaggedValue(y2));
    EXPECT_EQ(res2, expectedG2);

    // test other
    uint64_t x3 = JSTaggedValue(7).GetRawData();
    uint64_t y3 = JSTaggedValue(0).GetRawData();
    COMPILER_LOG(INFO) << "x2 = " << x3 << "  y2 = " << y3;
    auto res3 = fn(thread->GetGlueAddr(), x3, y3);
    COMPILER_LOG(INFO) << "res for FastDiv(7, 0) = " << res3.GetRawData();
    auto expectedG3 = FastRuntimeStub::FastDiv(JSTaggedValue(x3), JSTaggedValue(y3));
    EXPECT_EQ(res3, expectedG3);
}

HWTEST_F_L0(StubTest, FastModTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::Mod);
    Circuit netOfGates;
    ModStub optimizer(&netOfGates);
    optimizer.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run();
    auto fn = reinterpret_cast<JSTaggedValue (*)(uintptr_t, int64_t, int64_t)>(
        assembler.GetFuncPtrFromCompiledModule(function));
    // test left, right are all integer
    int x = 7;
    int y = 3;
    auto result = fn(thread->GetGlueAddr(), JSTaggedValue(x).GetRawData(), JSTaggedValue(y).GetRawData());
    JSTaggedValue expectRes = FastRuntimeStub::FastMod(JSTaggedValue(x), JSTaggedValue(y));
    EXPECT_EQ(result, expectRes);

    // test y == 0.0 || std::isnan(y) || std::isnan(x) || std::isinf(x) return NAN_VALUE
    double x2 = 7.3;
    int y2 = base::NAN_VALUE;
    auto result2 = fn(thread->GetGlueAddr(), JSTaggedValue(x2).GetRawData(), JSTaggedValue(y2).GetRawData());
    auto expectRes2 = FastRuntimeStub::FastMod(JSTaggedValue(x2), JSTaggedValue(y2));
    EXPECT_EQ(result2, expectRes2);
    COMPILER_LOG(INFO) << "result2 for FastMod(7, 'helloworld') = " << result2.GetRawData();
    COMPILER_LOG(INFO) << "expectRes2 for FastMod(7, 'helloworld') = " << expectRes2.GetRawData();

    // // test modular operation under normal conditions
    auto sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    double x3 = 33.0;
    double y3 = 44.0;
    auto result3 = fn(thread->GetGlueAddr(), JSTaggedValue(x3).GetRawData(), JSTaggedValue(y3).GetRawData());
    auto expectRes3 = FastRuntimeStub::FastMod(JSTaggedValue(x3), JSTaggedValue(y3));
    EXPECT_EQ(result3, expectRes3);
    thread->SetCurrentSPFrame(sp);

    // test x == 0.0 || std::isinf(y) return x
    double x4 = base::NAN_VALUE;
    int y4 = 7;
    auto result4 = fn(thread->GetGlueAddr(), JSTaggedValue(x4).GetRawData(), JSTaggedValue(y4).GetRawData());
    auto expectRes4 = FastRuntimeStub::FastMod(JSTaggedValue(x4), JSTaggedValue(y4));

    COMPILER_LOG(INFO) << "result4 for FastMod(base::NAN_VALUE, 7) = " << result4.GetRawData();
    COMPILER_LOG(INFO) << "expectRes4 for FastMod(base::NAN_VALUE, 7) = " << expectRes4.GetRawData();
    EXPECT_EQ(result4, expectRes4);

    // test all non-conforming conditions
    int x5 = 7;
    auto *factory = thread->GetEcmaVM()->GetFactory();
    thread->SetLastLeaveFrame(nullptr);
    auto y5 = factory->NewFromASCII("hello world");
    auto result5 = fn(thread->GetGlueAddr(), JSTaggedValue(x5).GetRawData(), y5.GetTaggedValue().GetRawData());
    EXPECT_EQ(result5, JSTaggedValue::Hole());
    auto expectRes5 = FastRuntimeStub::FastMod(JSTaggedValue(x5), y5.GetTaggedValue());
    COMPILER_LOG(INFO) << "result1 for FastMod(7, 'helloworld') = " << result5.GetRawData();
    EXPECT_EQ(result5, expectRes5);
}

HWTEST_F_L0(StubTest, TryLoadICByName)
{
    auto module = stubModule.GetModule();
    auto findFunction = stubModule.GetFunction(CommonStubCSigns::TryLoadICByName);
    Circuit netOfGates;
    TryLoadICByNameStub optimizer(&netOfGates);
    optimizer.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, findFunction, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run();
}

HWTEST_F_L0(StubTest, TryLoadICByValue)
{
    auto module = stubModule.GetModule();
    auto findFunction = stubModule.GetFunction(CommonStubCSigns::TryLoadICByValue);
    Circuit netOfGates;
    TryLoadICByValueStub optimizer(&netOfGates);
    optimizer.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, findFunction, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run();
}

HWTEST_F_L0(StubTest, TryStoreICByName)
{
    auto module = stubModule.GetModule();
    auto findFunction = stubModule.GetFunction(CommonStubCSigns::TryStoreICByName);
    Circuit netOfGates;
    TryStoreICByNameStub optimizer(&netOfGates);
    optimizer.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, findFunction, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run();
}

HWTEST_F_L0(StubTest, TryStoreICByValue)
{
    auto module = stubModule.GetModule();
    auto findFunction = stubModule.GetFunction(CommonStubCSigns::TryStoreICByValue);
    Circuit netOfGates;
    TryStoreICByValueStub optimizer(&netOfGates);
    optimizer.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, findFunction, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
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
        COMPILER_LOG(INFO) << "StubCallRunTimeThreadFpLock newFp: " << newFp << " oldRbp_ : " << oldRbp_
                           << " thread_->fp:" << thread_->fp;
    }
    ~StubCallRunTimeThreadFpLock()
    {
        COMPILER_LOG(INFO) << "~StubCallRunTimeThreadFpLock oldRbp_: " << oldRbp_ << " thread_->fp:" << thread_->fp;
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
    COMPILER_LOG(INFO) << "RuntimeFunc1  -";
    int64_t newRbp;
    asm("mov %%rbp, %0" : "=rm"(newRbp));
    StubCallRunTimeThreadFpLock lock(fpInfo, newRbp);

    COMPILER_LOG(INFO) << std::hex << "g_stub2Func " << reinterpret_cast<uintptr_t>(g_stub2Func);
    if (g_stub2Func != nullptr) {
        g_stub2Func(fpInfo);
    }
    COMPILER_LOG(INFO) << "RuntimeFunc1  +";
    return 0;
}

int RuntimeFunc2(struct ThreadTy *fpInfo)
{
    COMPILER_LOG(INFO) << "RuntimeFunc2  -";
    // update thread.fp
    int64_t newRbp;
    asm("mov %%rbp, %0" : "=rm"(newRbp));
    StubCallRunTimeThreadFpLock lock(fpInfo, newRbp);
    auto rbp = reinterpret_cast<int64_t *>(fpInfo->fp);

    COMPILER_LOG(INFO) << " RuntimeFunc2 rbp:" << rbp;
    for (int i = 0; i < 40; i++) { // print 40 ptr value for debug
        COMPILER_LOG(INFO) << std::hex << &(rbp[i]) << " :" << rbp[i];
    }
    /* walk back
      stack frame:           0     pre rbp  <-- rbp
                            -8     type
                            -16    pre frame thread fp
    */
    int64_t *frameType = nullptr;
    int64_t *gcFp = nullptr;
    COMPILER_LOG(INFO) << "-----------------walkback----------------";
    do {
        frameType = rbp - 1;
        if (*frameType == 1) {
            gcFp = rbp - 2; // 2: 2 stack slot
        } else {
            gcFp = rbp;
        }
        rbp = reinterpret_cast<intptr_t *>(*gcFp);
        COMPILER_LOG(INFO) << std::hex << "frameType :" << *frameType << " gcFp:" << *gcFp;
    } while (*gcFp != 0);
    COMPILER_LOG(INFO) << "+++++++++++++++++walkback++++++++++++++++";
    COMPILER_LOG(INFO) << "call RuntimeFunc2 func ThreadTy fp: " << fpInfo->fp << " magic:" << fpInfo->magic;
    COMPILER_LOG(INFO) << "RuntimeFunc2  +";
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

LLVMValueRef CallingFp(LLVMModuleRef &module, LLVMBuilderRef &builder)
{
    /* 0:calling 1:its caller */
    std::vector<LLVMValueRef> args = {LLVMConstInt(LLVMInt32Type(), 0, false)};
    auto fn = LLVMGetNamedFunction(module, "llvm.frameaddress.p0i8");
    if (!fn) {
        COMPILER_LOG(INFO) << "Could not find function ";
        return LLVMConstInt(LLVMInt64Type(), 0, false);
    }
    LLVMValueRef fAddrRet = LLVMBuildCall(builder, fn, args.data(), 1, "");
    LLVMValueRef frameAddr = LLVMBuildPtrToInt(builder, fAddrRet, LLVMInt64Type(), "cast_int64_t");
    return frameAddr;
}

#ifdef ARK_GC_SUPPORT
HWTEST_F_L0(StubTest, JSEntryTest)
{
    COMPILER_LOG(INFO) << " ---------- JSEntryTest ------------- ";
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
    LLVMValueRef frameAddr = CallingFp(module, builder);
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
    /* ThreadTy fpInfo struct;
        fpInfo.fp assign calling frame address
    */
    gep2 = LLVMBuildGEP2(builder, threadTy, value2, indexes2.data(), indexes2.size(), "fpAddr");
    frameAddr = CallingFp(module, builder);
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
    /* ThreadTy fpInfo
        fpInfo.fp set calling frame address
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
    char *error = nullptr;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);

    LLVMAssembler assembler(module);
    assembler.Run();
    auto engine = assembler.GetEngine();
    uint64_t stub1Code = LLVMGetFunctionAddress(engine, "stub1");
    uint64_t stub2Code = LLVMGetFunctionAddress(engine, "stub2");
    uint64_t stub3Code = LLVMGetFunctionAddress(engine, "stub3");
    struct ThreadTy parameters = {0x0, 0x0};
    auto stub1Func = reinterpret_cast<int64_t (*)(struct ThreadTy *)>(stub1Code);
    g_stub2Func = reinterpret_cast<int64_t (*)(struct ThreadTy *)>(stub2Code);
    int64_t result = stub1Func(&parameters);
    COMPILER_LOG(INFO) << "parameters magic:" << parameters.magic << " parameters.fp " << parameters.fp;
    EXPECT_EQ(parameters.fp, 0x0);
    EXPECT_EQ(result, 1);
    COMPILER_LOG(INFO) << " ++++++++++ JSEntryTest +++++++++++++ ";
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
    COMPILER_LOG(INFO) << " ---------- Prologue ------------- ";
    LLVMModuleRef module = LLVMModuleCreateWithName("simple_module");
    LLVMSetTarget(module);
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
    char *error = nullptr;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);

    LLVMAssembler assembler(module);
    assembler.Run();
    auto engine = assembler.GetEngine();
    uint64_t mainCode = LLVMGetFunctionAddress(engine, "main");
    auto mainFunc = reinterpret_cast<int64_t (*)(int64_t, int64_t)>(mainCode);
    int64_t result = mainFunc(1, 2);
    EXPECT_EQ(result, 3);
    COMPILER_LOG(INFO) << " ++++++++++ Prologue +++++++++++++ ";
}

/*
verify llvm.frameaddress.p0i8 ok
test:
    js(stub):main
        call RuntimeFunc
*/
HWTEST_F_L0(StubTest, CEntryFp)
{
    COMPILER_LOG(INFO) << " ---------- CEntryFp ------------- ";
    LLVMModuleRef module = LLVMModuleCreateWithName("simple_module");
    LLVMSetTarget(module);
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
    COMPILER_LOG(INFO);
    LLVMValueRef retVal = LLVMBuildCall(builder, runTimeFunc, argValue.data(), 1, "");
    LLVMBuildRet(builder, retVal);
    char *error = nullptr;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);

    LLVMAssembler assembler(module);
    assembler.Run();
    auto engine = assembler.GetEngine();
    uint64_t nativeCode = LLVMGetFunctionAddress(engine, "main");
    COMPILER_LOG(INFO) << " nativeCode : " << nativeCode;
    struct ThreadTy parameters = {0x0, 0x0};

    auto mainFunc = reinterpret_cast<int64_t (*)(struct ThreadTy *)>(nativeCode);
    int64_t result = mainFunc(&parameters);
    EXPECT_EQ(result, 1);
    COMPILER_LOG(INFO) << " ++++++++++ CEntryFp +++++++++++++ ";
}

HWTEST_F_L0(StubTest, LoadGCIRTest)
{
    COMPILER_LOG(INFO) << "--------------LoadGCIRTest--------------------";
    char *path = get_current_dir_name();
    std::string filePath = std::string(path) + "/ark/js_runtime/ecmascript/compiler/tests/satepoint_GC_0.ll";

    char resolvedPath[PATH_MAX];
    char *res = realpath(filePath.c_str(), resolvedPath);
    if (res == nullptr) {
        COMPILER_LOG(ERROR) << "filePath :" << filePath.c_str() << "   is not exist !";
        return;
    }

    llvm::SMDiagnostic err;
    llvm::LLVMContext context;
    llvm::StringRef inputFilename(resolvedPath);
    // Load the input module...
    std::unique_ptr<llvm::Module> rawModule = parseIRFile(inputFilename, err, context);
    if (!rawModule) {
        COMPILER_LOG(INFO) << "parseIRFile :" << inputFilename.data() << " failed !";
        err.print("parseIRFile ", llvm::errs());
        return;
    }
    LLVMModuleRef module = LLVMCloneModule(wrap(rawModule.get()));
    LLVMAssembler assembler(module);
    assembler.Run();
    auto engine = assembler.GetEngine();
    LLVMValueRef function = LLVMGetNamedFunction(module, "main");

    auto *mainPtr = reinterpret_cast<int (*)()>(LLVMGetPointerToGlobal(engine, function));
    uint8_t *ptr = assembler.GetStackMapsSection();
    LLVMStackMapParser::GetInstance().CalculateStackMap(ptr);

    int value = reinterpret_cast<int (*)()>(mainPtr)();
    COMPILER_LOG(INFO) << " value:" << value;
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
        const kungfu::CallSiteInfo *infos =
            kungfu::LLVMStackMapParser::GetInstance().GetCallSiteInfoByPc(returnAddr);
        if (infos != nullptr) {
            for (auto &info : *infos) {
                uintptr_t **address = nullptr;
                if (info.first == FrameConstants::SP_DWARF_REG_NUM) {
                    address = reinterpret_cast<uintptr_t **>(reinterpret_cast<uint8_t *>(rsp) + info.second);
                } else if (info.first == FrameConstants::FP_DWARF_REG_NUM) {
                    address = reinterpret_cast<uintptr_t **>(reinterpret_cast<uint8_t *>(rbp) + info.second);
                }
                // print ref and vlue for debug
                COMPILER_LOG(INFO) << std::hex << "ref addr:" << address;
                COMPILER_LOG(INFO) << "  value:" << *address;
                COMPILER_LOG(INFO) << " *value :" << **address;
            }
        }
        COMPILER_LOG(INFO) << std::hex << "+++++++++++++++++++ returnAddr : 0x" << returnAddr << " rbp:" << rbp
                           << " rsp: " << rsp;
    }
    COMPILER_LOG(INFO) << "do_safepoint +++ ";
}
}

HWTEST_F_L0(StubTest, GetPropertyByIndexStub)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::GetPropertyByIndex);
    Circuit netOfGates;
    GetPropertyByIndexStub optimizer(&netOfGates);
    optimizer.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run();
    auto *getpropertyByIndex = reinterpret_cast<JSTaggedValue (*)(uintptr_t, JSTaggedValue, uint32_t)>(
        reinterpret_cast<uintptr_t>(assembler.GetFuncPtrFromCompiledModule(function)));
    auto *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    int x = 213;
    int y = 10;
    FastRuntimeStub::SetOwnElement(thread, obj.GetTaggedValue(), 1, JSTaggedValue(x));
    FastRuntimeStub::SetOwnElement(thread, obj.GetTaggedValue(), 10250, JSTaggedValue(y));
    JSTaggedValue resVal = getpropertyByIndex(thread->GetGlueAddr(), obj.GetTaggedValue(), 1);
    EXPECT_EQ(resVal.GetNumber(), x);
    resVal = getpropertyByIndex(thread->GetGlueAddr(), obj.GetTaggedValue(), 10250);
    EXPECT_EQ(resVal.GetNumber(), y);
}

#ifdef ARK_GC_SUPPORT
HWTEST_F_L0(StubTest, SetPropertyByIndexStub)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::SetPropertyByIndex);
    Circuit netOfGates;
    SetPropertyByIndexStub optimizer(&netOfGates);
    optimizer.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    bool result = Verifier::Run(&netOfGates);
    ASSERT_TRUE(result);
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run();
    auto *setpropertyByIndex = reinterpret_cast<JSTaggedValue (*)(uintptr_t, JSTaggedValue, uint32_t, JSTaggedValue)>(
        reinterpret_cast<uintptr_t>(assembler.GetFuncPtrFromCompiledModule(function)));
    auto *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArray> array = factory->NewJSArray();
    // set value to array
    array->SetArrayLength(thread, 20);
    for (int i = 0; i < 20; i++) {
        auto taggedArray = array.GetTaggedValue();
        setpropertyByIndex(thread->GetGlueAddr(), taggedArray, i, JSTaggedValue(i));
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
    auto function = stubModule.GetFunction(CommonStubCSigns::GetPropertyByName);
    Circuit netOfGates;
    GetPropertyByNameStub optimizer(&netOfGates);
    optimizer.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    bool result = Verifier::Run(&netOfGates);
    ASSERT_TRUE(result);
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run();
    auto *getPropertyByNamePtr = reinterpret_cast<JSTaggedValue (*)(uintptr_t, uint64_t, uint64_t)>(
        reinterpret_cast<uintptr_t>(assembler.GetFuncPtrFromCompiledModule(function)));
    auto *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    int x = 256;
    int y = 10;
    JSHandle<JSTaggedValue> strA(factory->NewFromASCII("a"));
    JSHandle<JSTaggedValue> strBig(factory->NewFromASCII("biggest"));
    FastRuntimeStub::SetPropertyByName(thread, obj.GetTaggedValue(), strA.GetTaggedValue(), JSTaggedValue(x));
    FastRuntimeStub::SetPropertyByName(thread, obj.GetTaggedValue(), strBig.GetTaggedValue(), JSTaggedValue(y));
    JSTaggedValue resVal = getPropertyByNamePtr(thread->GetGlueAddr(), obj.GetTaggedValue().GetRawData(),
        strA.GetTaggedValue().GetRawData());
    EXPECT_EQ(resVal.GetNumber(), x);
    resVal = getPropertyByNamePtr(thread->GetGlueAddr(), obj.GetTaggedValue().GetRawData(),
                                  strBig.GetTaggedValue().GetRawData());
    EXPECT_EQ(resVal.GetNumber(), y);
}

#ifdef ARK_GC_SUPPORT
HWTEST_F_L0(StubTest, SetPropertyByNameStub)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::SetPropertyByName);
    Circuit netOfGates;
    SetPropertyByNameStub optimizer(&netOfGates);
    optimizer.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run();
    auto *setPropertyByName = reinterpret_cast<JSTaggedValue (*)(uintptr_t, JSTaggedValue,
        JSTaggedValue, JSTaggedValue, bool)>
        (reinterpret_cast<uintptr_t>(assembler.GetFuncPtrFromCompiledModule(function)));
    auto *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    int x = 256;
    int y = 10;
    JSHandle<JSTaggedValue> strA(factory->NewFromASCII("hello"));
    JSHandle<JSTaggedValue> strBig(factory->NewFromASCII("biggest"));
    setPropertyByName(thread->GetGlueAddr(), obj.GetTaggedValue(), strA.GetTaggedValue(), JSTaggedValue(x), false);
    setPropertyByName(thread->GetGlueAddr(), obj.GetTaggedValue(), strBig.GetTaggedValue(), JSTaggedValue(y), false);
    auto resA = FastRuntimeStub::GetPropertyByName(thread, obj.GetTaggedValue(), strA.GetTaggedValue());
    EXPECT_EQ(resA.GetNumber(), x);
    auto resB = FastRuntimeStub::GetPropertyByName(thread, obj.GetTaggedValue(), strBig.GetTaggedValue());
    EXPECT_EQ(resB.GetNumber(), y);
}
#endif

HWTEST_F_L0(StubTest, GetPropertyByValueStub)
{
    auto module = stubModule.GetModule();
    LLVMValueRef getPropertyByIndexfunction = stubModule.GetFunction(CommonStubCSigns::GetPropertyByIndex);
    Circuit netOfGates2;
    GetPropertyByIndexStub getPropertyByIndexStub(&netOfGates2);
    getPropertyByIndexStub.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates2.PrintAllGates();
    auto cfg2 = Scheduler::Run(&netOfGates2);
    LLVMIRBuilder llvmBuilder2(&cfg2, &netOfGates2, &stubModule, getPropertyByIndexfunction,
        stubModule.GetCompilationConfig(),  CallSignature::CallConv::CCallConv);
    llvmBuilder2.Build();

    LLVMValueRef getPropertyByNamefunction = stubModule.GetFunction(CommonStubCSigns::GetPropertyByName);
    Circuit netOfGates1;
    GetPropertyByNameStub getPropertyByNameStub(&netOfGates1);
    getPropertyByNameStub.GenerateCircuit(stubModule.GetCompilationConfig());
    bool result = Verifier::Run(&netOfGates1);
    ASSERT_TRUE(result);
    auto cfg1 = Scheduler::Run(&netOfGates1);
    LLVMIRBuilder llvmBuilder1(&cfg1, &netOfGates1, &stubModule, getPropertyByNamefunction,
        stubModule.GetCompilationConfig(), CallSignature::CallConv::CCallConv);
    llvmBuilder1.Build();

    LLVMValueRef function = stubModule.GetFunction(CommonStubCSigns::GetPropertyByValue);
    Circuit netOfGates;
    GetPropertyByValueStub optimizer(&netOfGates);
    optimizer.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    result = Verifier::Run(&netOfGates);
    ASSERT_TRUE(result);
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);

    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run();
    auto *getPropertyByValuePtr = reinterpret_cast<JSTaggedValue (*)(uintptr_t, uint64_t, uint64_t)>(
        reinterpret_cast<uintptr_t>(assembler.GetFuncPtrFromCompiledModule(function)));
    auto *getPropertyByNamePtr = reinterpret_cast<JSTaggedValue (*)(uintptr_t, uint64_t, uint64_t)>(
        reinterpret_cast<uintptr_t>(assembler.GetFuncPtrFromCompiledModule(getPropertyByNamefunction)));
    auto *getpropertyByIndexPtr = reinterpret_cast<JSTaggedValue (*)(uintptr_t, JSTaggedValue, uint32_t)>(
        reinterpret_cast<uintptr_t>(assembler.GetFuncPtrFromCompiledModule(getPropertyByIndexfunction)));

    thread->SetFastStubEntry(CommonStubCSigns::GetPropertyByIndex, reinterpret_cast<uintptr_t>(getpropertyByIndexPtr));
    thread->SetFastStubEntry(CommonStubCSigns::GetPropertyByName, reinterpret_cast<uintptr_t>(getPropertyByNamePtr));
    auto *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    int x = 213;
    int y = 10;
    auto sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    FastRuntimeStub::SetOwnElement(thread, obj.GetTaggedValue(), 1, JSTaggedValue(x));
    FastRuntimeStub::SetOwnElement(thread, obj.GetTaggedValue(), 10250, JSTaggedValue(y));

    JSHandle<JSTaggedValue> strA(factory->NewFromASCII("a"));
    JSHandle<JSTaggedValue> strBig(factory->NewFromASCII("biggest"));
    JSHandle<JSTaggedValue> strDigit(factory->NewFromASCII("10250"));

    FastRuntimeStub::SetPropertyByName(thread, obj.GetTaggedValue(), strA.GetTaggedValue(), JSTaggedValue(x));
    FastRuntimeStub::SetPropertyByName(thread, obj.GetTaggedValue(), strBig.GetTaggedValue(), JSTaggedValue(y));
    JSTaggedValue resVal1 = getPropertyByNamePtr(thread->GetGlueAddr(), obj.GetTaggedValue().GetRawData(),
        strA.GetTaggedValue().GetRawData());
    EXPECT_EQ(resVal1.GetNumber(), x);
    JSTaggedValue resVal = getPropertyByValuePtr(thread->GetGlueAddr(), obj.GetTaggedValue().GetRawData(),
        strA.GetTaggedValue().GetRawData());
    EXPECT_EQ(resVal.GetNumber(), x);
    resVal = getPropertyByValuePtr(thread->GetGlueAddr(), obj.GetTaggedValue().GetRawData(),
                                   strBig.GetTaggedValue().GetRawData());
    EXPECT_EQ(resVal.GetNumber(), y);
    resVal = getpropertyByIndexPtr(thread->GetGlueAddr(), obj.GetTaggedValue(), 1);
    EXPECT_EQ(resVal.GetNumber(), x);
    resVal = getPropertyByValuePtr(thread->GetGlueAddr(), obj.GetTaggedValue().GetRawData(),
                                   JSTaggedValue(10250).GetRawData());
    EXPECT_EQ(resVal.GetNumber(), y);
    resVal = getPropertyByValuePtr(thread->GetGlueAddr(), obj.GetTaggedValue().GetRawData(),
                                   strDigit.GetTaggedValue().GetRawData());
    EXPECT_EQ(resVal.GetNumber(), y);
    thread->SetCurrentSPFrame(sp);
    thread->SetLastLeaveFrame(nullptr);
    JSHandle<JSTaggedValue> strHello(factory->NewFromASCII("hello world"));
    double key = 4.29497e+09;
    resVal = getPropertyByValuePtr(thread->GetGlueAddr(), strHello.GetTaggedValue().GetRawData(),
                                   JSTaggedValue(key).GetRawData());
    EXPECT_EQ(resVal.GetRawData(), 0ULL);
}

HWTEST_F_L0(StubTest, FastTypeOfTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::TypeOf);
    Circuit netOfGates;
    TypeOfStub optimizer(&netOfGates);
    optimizer.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    bool verRes = Verifier::Run(&netOfGates);
    ASSERT_TRUE(verRes);
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    char *error = nullptr;
    LLVMVerifyModule(module, LLVMAbortProcessAction, &error);
    LLVMAssembler assembler(module);
    assembler.Run();
    auto *typeOfPtr =
        reinterpret_cast<JSTaggedValue (*)(uintptr_t, uint64_t)>(assembler.GetFuncPtrFromCompiledModule(function));
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();

    // obj is JSTaggedValue::VALUE_TRUE
    JSTaggedValue resultVal = typeOfPtr(thread->GetGlueAddr(), JSTaggedValue::True().GetRawData());
    JSTaggedValue expectResult = FastRuntimeStub::FastTypeOf(thread, JSTaggedValue::True());
    EXPECT_EQ(resultVal, globalConst->GetBooleanString());
    EXPECT_EQ(resultVal, expectResult);

    // obj is JSTaggedValue::VALUE_FALSE
    JSTaggedValue resultVal2 = typeOfPtr(thread->GetGlueAddr(), JSTaggedValue::False().GetRawData());
    JSTaggedValue expectResult2 = FastRuntimeStub::FastTypeOf(thread, JSTaggedValue::False());
    EXPECT_EQ(resultVal2, globalConst->GetBooleanString());
    EXPECT_EQ(resultVal2, expectResult2);

    // obj is JSTaggedValue::VALUE_NULL
    JSTaggedValue resultVal3 = typeOfPtr(thread->GetGlueAddr(), JSTaggedValue::Null().GetRawData());
    JSTaggedValue expectResult3 = FastRuntimeStub::FastTypeOf(thread, JSTaggedValue::Null());
    EXPECT_EQ(resultVal3, globalConst->GetObjectString());
    EXPECT_EQ(resultVal3, expectResult3);

    // obj is JSTaggedValue::VALUE_UNDEFINED
    JSTaggedValue resultVal4 = typeOfPtr(thread->GetGlueAddr(), JSTaggedValue::Undefined().GetRawData());
    JSTaggedValue expectResult4 = FastRuntimeStub::FastTypeOf(thread, JSTaggedValue::Undefined());
    EXPECT_EQ(resultVal4, globalConst->GetUndefinedString());
    EXPECT_EQ(resultVal4, expectResult4);

    // obj is IsNumber
    JSTaggedValue resultVal5 = typeOfPtr(thread->GetGlueAddr(), JSTaggedValue(5).GetRawData());
    JSTaggedValue expectResult5 = FastRuntimeStub::FastTypeOf(thread, JSTaggedValue(5));
    EXPECT_EQ(resultVal5, globalConst->GetNumberString());
    EXPECT_EQ(resultVal5, expectResult5);

    // obj is String
    auto *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> str1 = factory->NewFromASCII("a");
    JSHandle<EcmaString> str2 = factory->NewFromASCII("a");
    JSTaggedValue expectResult6 = FastRuntimeStub::FastTypeOf(thread, str1.GetTaggedValue());
    JSTaggedValue resultVal6 = typeOfPtr(thread->GetGlueAddr(), str2.GetTaggedValue().GetRawData());
    EXPECT_EQ(resultVal6, globalConst->GetStringString());
    EXPECT_EQ(resultVal6, expectResult6);

    // obj is Symbol
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSTaggedValue symbol = globalEnv->GetIteratorSymbol().GetTaggedValue();
    JSTaggedValue expectResult7= FastRuntimeStub::FastTypeOf(thread, symbol);
    JSTaggedValue resultVal7 = typeOfPtr(thread->GetGlueAddr(), symbol.GetRawData());
    EXPECT_EQ(resultVal7, globalConst->GetSymbolString());
    EXPECT_EQ(resultVal7, expectResult7);

    // obj is callable
    JSHandle<JSPromiseReactionsFunction> resolveCallable =
        factory->CreateJSPromiseReactionsFunction(reinterpret_cast<void *>(BuiltinsPromiseHandler::Resolve));
    JSTaggedValue expectResult8= FastRuntimeStub::FastTypeOf(thread, resolveCallable.GetTaggedValue());
    JSTaggedValue resultVal8 = typeOfPtr(thread->GetGlueAddr(), resolveCallable.GetTaggedValue().GetRawData());
    EXPECT_EQ(resultVal8, globalConst->GetFunctionString());
    EXPECT_EQ(resultVal8, expectResult8);

    // obj is heapObject
    JSHandle<JSObject> object = factory->NewEmptyJSObject();
    JSTaggedValue expectResult9= FastRuntimeStub::FastTypeOf(thread, object.GetTaggedValue());
    JSTaggedValue resultVal9 = typeOfPtr(thread->GetGlueAddr(), object.GetTaggedValue().GetRawData());
    EXPECT_EQ(resultVal9, globalConst->GetObjectString());
    EXPECT_EQ(resultVal9, expectResult9);
}

HWTEST_F_L0(StubTest, FastEqualTest)
{
    auto module = stubModule.GetModule();
    auto function = stubModule.GetFunction(CommonStubCSigns::Equal);
    Circuit netOfGates;
    EqualStub optimizer(&netOfGates);
    optimizer.GenerateCircuit(stubModule.GetCompilationConfig());
    netOfGates.PrintAllGates();
    auto cfg = Scheduler::Run(&netOfGates);
    PrintCircuitByBasicBlock(cfg, netOfGates);
    LLVMIRBuilder llvmBuilder(&cfg, &netOfGates, &stubModule, function, stubModule.GetCompilationConfig(),
        CallSignature::CallConv::CCallConv);
    llvmBuilder.Build();
    LLVMAssembler assembler(module);
    assembler.Run();
    auto fn = reinterpret_cast<JSTaggedValue (*)(uintptr_t, int64_t, int64_t)>(
        assembler.GetFuncPtrFromCompiledModule(function));
    // test for 1 == 1
    auto resA = fn(thread->GetGlueAddr(), JSTaggedValue(1).GetRawData(), JSTaggedValue(1).GetRawData());
    auto expectA = FastRuntimeStub::FastEqual(JSTaggedValue(1), JSTaggedValue(1));
    EXPECT_EQ(resA, expectA);

    // test for nan == nan
    double nan = std::numeric_limits<double>::quiet_NaN();
    auto resB = fn(thread->GetGlueAddr(), JSTaggedValue(nan).GetRawData(), JSTaggedValue(nan).GetRawData());
    auto expectB = FastRuntimeStub::FastEqual(JSTaggedValue(nan), JSTaggedValue(nan));
    EXPECT_EQ(resB, expectB);

    // test for undefined == null
    auto resC = fn(thread->GetGlueAddr(), JSTaggedValue::Undefined().GetRawData(), JSTaggedValue::Null().GetRawData());
    auto expectC = FastRuntimeStub::FastEqual(JSTaggedValue::Undefined(), JSTaggedValue::Null());
    EXPECT_EQ(resC, expectC);

    // test for "hello world" == undefined
    auto *factory = thread->GetEcmaVM()->GetFactory();
    auto str = factory->NewFromASCII("hello world");
    auto resD = fn(thread->GetGlueAddr(), str.GetTaggedValue().GetRawData(), JSTaggedValue::Undefined().GetRawData());
    auto expectD = FastRuntimeStub::FastEqual(str.GetTaggedValue(), JSTaggedValue::Undefined());
    EXPECT_EQ(resD, expectD);

    // test for true == hole
    auto resE = fn(thread->GetGlueAddr(), JSTaggedValue::True().GetRawData(), JSTaggedValue::Hole().GetRawData());
    auto expectE = FastRuntimeStub::FastEqual(JSTaggedValue::True(), JSTaggedValue::Hole());
    EXPECT_EQ(resE, expectE);

    // test for "hello world" == "hello world"
    auto resF = fn(thread->GetGlueAddr(), str.GetTaggedValue().GetRawData(), str.GetTaggedValue().GetRawData());
    auto expectF = FastRuntimeStub::FastEqual(str.GetTaggedValue(), str.GetTaggedValue());
    EXPECT_EQ(resF, expectF);

    // test for 5.2 == 5.2
    auto resG = fn(thread->GetGlueAddr(), JSTaggedValue(5.2).GetRawData(), JSTaggedValue(5.2).GetRawData());
    auto expectG = FastRuntimeStub::FastEqual(JSTaggedValue(5.2), JSTaggedValue(5.2));
    EXPECT_EQ(resG, expectG);

    // test for false == false
    auto resH = fn(thread->GetGlueAddr(), JSTaggedValue::False().GetRawData(), JSTaggedValue::False().GetRawData());
    auto expectH = FastRuntimeStub::FastEqual(JSTaggedValue::False(), JSTaggedValue::False());
    EXPECT_EQ(resH, expectH);

    // test for obj == obj
    JSHandle<JSObject> obj1 = factory->NewEmptyJSObject();
    JSHandle<JSObject> obj2 = factory->NewEmptyJSObject();
    FastRuntimeStub::SetOwnElement(thread, obj1.GetTaggedValue(), 1, JSTaggedValue(1));
    FastRuntimeStub::SetOwnElement(thread, obj2.GetTaggedValue(), 1, JSTaggedValue(1));
    auto resI = fn(thread->GetGlueAddr(), obj1.GetTaggedValue().GetRawData(), obj2.GetTaggedValue().GetRawData());
    auto expectI = FastRuntimeStub::FastEqual(obj1.GetTaggedValue(), obj2.GetTaggedValue());
    EXPECT_EQ(resI, expectI);
}

#ifdef ECMASCRIPT_ENABLE_TEST_STUB
HWTEST_F_L0(StubTest, JSCallTest)
{
    auto fooEntry = thread->GetFastStubEntry(CommonStubCSigns::FooAOT);
    auto footarget = NewAotFunction(2, fooEntry);
    auto glue = thread->GetGlueAddr();
    int x = 1;
    int y = 2;
    JSTaggedType argV[6] = {
        footarget.GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue(x).GetRawData(),
        JSTaggedValue(y).GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
    };
    auto result = JSFunctionEntry(glue, reinterpret_cast<uintptr_t>(thread->GetCurrentSPFrame()), 5, 5,
                                  argV, fooEntry);
    EXPECT_EQ(result, JSTaggedValue(3.0).GetRawData());

    auto result1 = JSFunctionEntry(glue, reinterpret_cast<uintptr_t>(thread->GetCurrentSPFrame()), 5, 6,
                                   argV, fooEntry);
    EXPECT_EQ(result1, JSTaggedValue(3.0).GetRawData());
}

HWTEST_F_L0(StubTest, JSCallTest1)
{
    auto foo2Entry = thread->GetFastStubEntry(CommonStubCSigns::Foo2AOT);
    auto foo2target = NewAotFunction(2, foo2Entry);
    auto glue = thread->GetGlueAddr();
    int x = 1;
    int y = 2;
    JSTaggedType argV[6] = {
        foo2target.GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue(x).GetRawData(),
        JSTaggedValue(y).GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
    };
    auto result = JSFunctionEntry(glue, reinterpret_cast<uintptr_t>(thread->GetCurrentSPFrame()), 5, 5,
                                  argV, foo2Entry);
    EXPECT_EQ(result, JSTaggedValue(3.0).GetRawData());
}

HWTEST_F_L0(StubTest, JSCallTest2)
{
    auto foo1Entry = thread->GetFastStubEntry(CommonStubCSigns::Foo1AOT);
    auto foo1target = NewAotFunction(2, foo1Entry);
    auto glue = thread->GetGlueAddr();
    int x = 1;
    int y = 2;
    JSTaggedType argV[5] = {
        foo1target.GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue(x).GetRawData(),
        JSTaggedValue(y).GetRawData(),
    };
    auto result = JSFunctionEntry(glue, reinterpret_cast<uintptr_t>(thread->GetCurrentSPFrame()), 5, 5,
                                  argV, foo1Entry);
    EXPECT_EQ(result, 0x7ff9000000000000UL);
}

HWTEST_F_L0(StubTest, JSCallNativeTest)
{
    auto fooEntry = thread->GetFastStubEntry(CommonStubCSigns::FooNativeAOT);
    auto footarget = NewAotFunction(2, fooEntry);
    auto glue = thread->GetGlueAddr();
    int x = 1;
    int y = 2;
    JSTaggedType argV[5] = {
        footarget.GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue(x).GetRawData(),
        JSTaggedValue(y).GetRawData(),
    };
    auto result = JSFunctionEntry(glue, reinterpret_cast<uintptr_t>(thread->GetCurrentSPFrame()), 5, 5,
                                  argV, fooEntry);
    EXPECT_EQ(result, JSTaggedValue::Undefined().GetRawData());
}

HWTEST_F_L0(StubTest, JSCallBoundTest)
{
    auto fooEntry = thread->GetFastStubEntry(CommonStubCSigns::FooBoundAOT);
    auto footarget = NewAotFunction(2, fooEntry);
    auto glue = thread->GetGlueAddr();
    int x = 1;
    int y = 2;
    JSTaggedType argV[5] = {
        footarget.GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue(x).GetRawData(),
        JSTaggedValue(y).GetRawData(),
    };
    auto result = JSFunctionEntry(glue, reinterpret_cast<uintptr_t>(thread->GetCurrentSPFrame()), 5, 5,
                                  argV, fooEntry);
    EXPECT_EQ(result, JSTaggedValue(38.0).GetRawData());
}

// test for proxy method is undefined
HWTEST_F_L0(StubTest, JSCallTest3)
{
    auto fooProxyEntry = thread->GetFastStubEntry(CommonStubCSigns::FooProxyAOT);
    auto foo2target = NewAotFunction(2, fooProxyEntry);
    auto glue = thread->GetGlueAddr();
    int x = 1;
    int y = 2;
    JSTaggedType argV[6] = {
        foo2target.GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue(x).GetRawData(),
        JSTaggedValue(y).GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
    };
    JSThread::GlueData::GetCOStubEntriesOffset(false);
    JSThread::GlueData::GetCOStubEntriesOffset(true);
    auto result = JSFunctionEntry(glue, reinterpret_cast<uintptr_t>(thread->GetCurrentSPFrame()),
        5, 5, argV, fooProxyEntry);
    EXPECT_EQ(result, JSTaggedValue(3.0).GetRawData());
}

// test for proxy method isn't undefined
HWTEST_F_L0(StubTest, JSCallTest4)
{
    auto fooProxyEntry = thread->GetFastStubEntry(CommonStubCSigns::FooProxy2AOT);
    auto foo2target = NewAotFunction(2, fooProxyEntry);
    auto glue = thread->GetGlueAddr();
    int x = 1;
    int y = 2;
    JSTaggedType argV[6] = {
        foo2target.GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
        JSTaggedValue(x).GetRawData(),
        JSTaggedValue(y).GetRawData(),
        JSTaggedValue::Undefined().GetRawData(),
    };
    JSThread::GlueData::GetCOStubEntriesOffset(false);
    JSThread::GlueData::GetCOStubEntriesOffset(true);
    [[maybe_unused]] auto result = JSFunctionEntry(glue, reinterpret_cast<uintptr_t>(thread->GetCurrentSPFrame()),
        5, 5, argV, fooProxyEntry);
}
#endif
}  // namespace panda::test
