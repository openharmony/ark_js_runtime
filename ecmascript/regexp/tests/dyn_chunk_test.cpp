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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/dyn_chunk.h"
#include "ecmascript/object_factory.h"

namespace panda::test {
using namespace panda::ecmascript;

class DynChunkTest : public testing::Test {
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

HWTEST_F_L0(DynChunkTest, EmitAndGet)
{
    DynChunk dynChunk = DynChunk(chunk_);
    dynChunk.EmitChar(65);
    dynChunk.EmitU16(66);
    dynChunk.EmitU32(67);
    ASSERT_EQ(dynChunk.GetSize(), 7U);
    ASSERT_EQ(dynChunk.GetAllocatedSize(), DynChunk::ALLOCATE_MIN_SIZE);
    ASSERT_EQ(dynChunk.GetError(), false);
    dynChunk.Insert(1, 1);
    uint32_t val1 = dynChunk.GetU8(0);
    uint32_t val2 = dynChunk.GetU16(2);
    uint32_t val3 = dynChunk.GetU32(4);
    ASSERT_EQ(val1, 65U);
    ASSERT_EQ(val2, 66U);
    ASSERT_EQ(val3, 67U);
}

HWTEST_F_L0(DynChunkTest, EmitSelfAndGet)
{
    DynChunk dynChunk = DynChunk(chunk_);
    dynChunk.EmitChar(65);
    dynChunk.EmitSelf(0, 1);
    ASSERT_EQ(dynChunk.GetSize(), 2U);
    ASSERT_EQ(dynChunk.GetAllocatedSize(), DynChunk::ALLOCATE_MIN_SIZE);
    ASSERT_EQ(dynChunk.GetError(), false);
    uint32_t val1 = dynChunk.GetU8(0);
    uint32_t val2 = dynChunk.GetU8(1);
    ASSERT_EQ(val1, 65U);
    ASSERT_EQ(val2, 65U);
}

HWTEST_F_L0(DynChunkTest, EmitStrAndGet)
{
    DynChunk dynChunk = DynChunk(chunk_);
    dynChunk.EmitStr("abc");
    ASSERT_EQ(dynChunk.GetSize(), 4U);
    ASSERT_EQ(dynChunk.GetAllocatedSize(), DynChunk::ALLOCATE_MIN_SIZE);
    ASSERT_EQ(dynChunk.GetError(), false);
    uint32_t val1 = dynChunk.GetU8(0);
    uint32_t val2 = dynChunk.GetU8(1);
    uint32_t val3 = dynChunk.GetU8(2);
    uint32_t val4 = dynChunk.GetU8(3);
    ASSERT_EQ(val1, 97U);
    ASSERT_EQ(val2, 98U);
    ASSERT_EQ(val3, 99U);
    ASSERT_EQ(val4, 0U);
}
}  // namespace panda::test
