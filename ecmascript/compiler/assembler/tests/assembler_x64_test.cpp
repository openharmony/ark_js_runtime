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
#undef __
}  // namespace panda::test
