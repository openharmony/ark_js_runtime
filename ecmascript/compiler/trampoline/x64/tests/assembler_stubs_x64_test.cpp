/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/tests/test_helper.h"

#include "ecmascript/compiler/assembler/assembler_x64.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/dyn_chunk.h"
#include "ecmascript/compiler/llvm_codegen.h"
#include "ecmascript/compiler/trampoline/x64/assembler_stubs_x64.h"
#include "ecmascript/compiler/assembler/extended_assembler_x64.h"

namespace panda::test {
using namespace panda::ecmascript;
using namespace panda::ecmascript::x64;

class AssemblerStubsTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
        chunk_ = thread->GetEcmaVM()->GetChunk();
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    JSThread *thread {nullptr};
    EcmaHandleScope *scope {nullptr};
    Chunk *chunk_ {nullptr};
};

#define __ masm.
HWTEST_F_L0(AssemblerStubsTest, JSFunctionEntry)
{
    x64::AssemblerX64 masm(chunk_);
    x64::ExtendedAssemblerX64 *assemblerX64 = static_cast<ExtendedAssemblerX64 *>(&masm);
    x64::AssemblerStubsX64::JSFunctionEntry(assemblerX64);
    ecmascript::kungfu::LLVMAssembler::Disassemble(masm.GetBegin(), masm.GetCurrentPosition());
}

HWTEST_F_L0(AssemblerStubsTest, OptimizedCallOptimized)
{
    x64::AssemblerX64 masm(chunk_);
    x64::ExtendedAssemblerX64 *assemblerX64 = static_cast<ExtendedAssemblerX64 *>(&masm);
    x64::AssemblerStubsX64::OptimizedCallOptimized(assemblerX64);
    ecmascript::kungfu::LLVMAssembler::Disassemble(masm.GetBegin(), masm.GetCurrentPosition());
}

HWTEST_F_L0(AssemblerStubsTest, CallNativeTrampoline)
{
    x64::AssemblerX64 masm(chunk_);
    x64::ExtendedAssemblerX64 *assemblerX64 = static_cast<ExtendedAssemblerX64 *>(&masm);
    x64::AssemblerStubsX64::CallNativeTrampoline(assemblerX64);
    ecmascript::kungfu::LLVMAssembler::Disassemble(masm.GetBegin(), masm.GetCurrentPosition());
}

HWTEST_F_L0(AssemblerStubsTest, JSCallWithArgv)
{
    x64::AssemblerX64 masm(chunk_);
    x64::ExtendedAssemblerX64 *assemblerX64 = static_cast<ExtendedAssemblerX64 *>(&masm);
    x64::AssemblerStubsX64::JSCallWithArgv(assemblerX64);
    ecmascript::kungfu::LLVMAssembler::Disassemble(masm.GetBegin(), masm.GetCurrentPosition());
}

HWTEST_F_L0(AssemblerStubsTest, JSCall)
{
    x64::AssemblerX64 masm(chunk_);
    x64::ExtendedAssemblerX64 *assemblerX64 = static_cast<ExtendedAssemblerX64 *>(&masm);
    x64::AssemblerStubsX64::JSCall(assemblerX64);
    ecmascript::kungfu::LLVMAssembler::Disassemble(masm.GetBegin(), masm.GetCurrentPosition());
}

HWTEST_F_L0(AssemblerStubsTest, CallRuntimeWithArgv)
{
    x64::AssemblerX64 masm(chunk_);
    x64::ExtendedAssemblerX64 *assemblerX64 = static_cast<ExtendedAssemblerX64 *>(&masm);
    x64::AssemblerStubsX64::CallRuntimeWithArgv(assemblerX64);
    ecmascript::kungfu::LLVMAssembler::Disassemble(masm.GetBegin(), masm.GetCurrentPosition());
}
#undef __
}  // namespace panda::test
