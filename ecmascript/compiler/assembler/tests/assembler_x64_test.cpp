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

#include "ecmascript/tests/test_helper.h"

#include "ecmascript/compiler/assembler/assembler_x64.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/dyn_chunk.h"

namespace panda::test {
using namespace panda::ecmascript;
using namespace panda::ecmascript::x64;

class AssemblerTest : public testing::Test {
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
HWTEST_F_L0(AssemblerTest, Emit)
{
    x64::AssemblerX64 masm(chunk_);
    Label lable1;
 
    size_t current = 0;
    __ Pushq(rbp);
    uint32_t value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x55U);

    __ Pushq(0);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x6AU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    __ Popq(rbp);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x5DU);
    __ Bind(&lable1);
    __ Movq(rcx, rbx);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x48U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x89U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xCBU);
    __ Movq(Operand(rsp, 0x40U), rbx);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x48U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x8BU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x5CU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x24U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x40U);
    __ Jmp(&lable1);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xEBU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xF6U);
    __ Ret();
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xC3U);
}

HWTEST_F_L0(AssemblerTest, Emit1)
{
    x64::AssemblerX64 masm(chunk_);

    size_t current = 0;
    __ Movl(Operand(rax, 0x38), rax);
    uint32_t value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x8BU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x40U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x38U);

    // 41 89 f6 movl    %esi, %r14d
    __ Movl(rsi, r14);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x41U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x89U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xF6U);

    // movzbq  (%rcx), %rax
    __ Movzbq(Operand(rcx, 0), rax);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x48U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x0FU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xB6U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x01U);

    // 48 ba 02 00 00 00 00 00 00 00   movabs $0x2,%rdx
    __ Movabs(0x2, rdx);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x48U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xBAU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x02U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);

    // ba 05 00 00 00 movq    $JUMP_SIZE_PREF_IMM16_V8, %rdx
    __ Movq(0x5, rdx);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xBAU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x05U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);

    // 49 89 e0        mov    %rsp,%r8
    __ Movq(rsp, r8);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x49U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x89U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xE0U);
}

HWTEST_F_L0(AssemblerTest, Emit2)
{
    x64::AssemblerX64 masm(chunk_);

    size_t current = 0;
    // 81 fa ff ff ff 09       cmpl    $0x9ffffff,%edx
    __ Cmpl(0x9FFFFFF, rdx);
    uint32_t value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x81U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFAU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x09U);

    // 39 cb   cmpl    %ecx,%ebx
    __ Cmpl(rcx, rbx);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x39U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xCBU);

    //48 83 fa 00     cmp    $0x0,%rdx
    __ Cmp(0x0, rdx);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x48U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x83U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFAU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);

    // 4c 39 D8 cmpq    %r11, %rax
    __ Cmpq(r11, rax);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x4CU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x39U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xD8U);


    // 0f ba e0 08     bt     $0x8,%eax
    __ Btl(0x8, rax);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x0FU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xBAU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xE0U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x08U);
}

HWTEST_F_L0(AssemblerTest, Emit3)
{
    x64::AssemblerX64 masm(chunk_);
    size_t current = 0;

    // cmovbe  %ebx, %ecx
    __ CMovbe(rbx, rcx);
    uint32_t value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x0FU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x46U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xCBU);

    // testb   $0x1,%r10b
    __ Testb(0x1, r14);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x41U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xF6U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xC6U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x01U);

    // 48 f6 c4 0f testq   $15, %rsp
    __ Testq(15, rsp);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x48U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xF6U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xC4U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x0FU);

    // andq    $ASM_JS_METHOD_NUM_VREGS_MASK, %r11
    __ Andq(0xfffffff, r11);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x49U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x81U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xE3U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x0FU);

    // andl 0xfffffff, %eax
    __ Andl(0xfffffff, rax);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x25U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x0FU);

    // and     %rax, %rdx
    __ And(rax, rdx);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x48U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x21U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xC2U);
}

HWTEST_F_L0(AssemblerTest, Emit4)
{
    x64::AssemblerX64 masm(chunk_);
    size_t current = 0;

    // 4a 8d 0c f5 00 00 00 00 leaq    0x0(,%r14,8),%rcx
    __ Leaq(Operand(r14, Scale::Times8, 0), rcx);
    uint32_t value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x4AU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x8DU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x0CU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xF5U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);

    // 8d 90 ff ff ff fc       leal    -0x3000001(%rax),%edx
    __ Leal(Operand(rax, -50331649), rdx);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x8DU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x90U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFCU);

    // c1 e0 18        shl    $0x18,%eax
    __ Shll(0x18, rax);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xC1U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xE0U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x18U);

    // shrq    $ASM_JS_METHOD_NUM_ARGS_START_BIT(32), %r11
    __ Shrq(32, r11);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x49U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xC1U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xEBU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x20U);

    // int3
    __ Int3();
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xCCU);
}
#undef __
}  // namespace panda::test
