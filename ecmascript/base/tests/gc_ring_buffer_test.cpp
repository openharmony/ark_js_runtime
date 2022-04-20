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

#include "ecmascript/base/gc_ring_buffer.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class GCRingBufferTest : public testing::Test {
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
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

/**
 * @tc.name: Push
 * @tc.desc: Define a fixed length container for storing int type data, call the push function to assign a value
 *           to each position of the container, and when the maximum length is reached, overwrite the original
 *           value from scratch.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(GCRingBufferTest, Push)
{
    constexpr int LENGTH = 10;
    GCRingBuffer<int, LENGTH> gcBuffer;
    EXPECT_EQ(gcBuffer.Count(), 0);
    for (int i = 0 ; i < 2 * LENGTH ; i++) {
        gcBuffer.Push(i);
    }
    EXPECT_EQ(gcBuffer.Count(), LENGTH);
}

static int SumCallback(const int initial, int elements)
{
    return initial + elements;
}

/**
 * @tc.name: Sum
 * @tc.desc: The "Sum" function calculates the sum of stored data by calling the callback function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(GCRingBufferTest, Sum)
{
    constexpr int LENGTH = 10;
    GCRingBuffer<int, LENGTH> gcBuffer;

    for (int i = 0 ; i < LENGTH ; i++) {
        gcBuffer.Push(i);
    }
    EXPECT_EQ(gcBuffer.Sum(SumCallback, 0), 45);

    for (int i = 0 ; i < LENGTH ; i++) {
        gcBuffer.Push(1);
    }
    EXPECT_EQ(gcBuffer.Count(), LENGTH);
    EXPECT_EQ(gcBuffer.Sum(SumCallback, 0), 10);
}

/**
 * @tc.name: Reset
 * @tc.desc: Set the subscript of the start position and the subscript of the end position of the container to zero.
 *           The next time you store data, store it from the first position. then call the "Count" function to check
 *           whether the count of the container is zero.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(GCRingBufferTest, Reset)
{
    constexpr int LENGTH = 10;
    GCRingBuffer<int, LENGTH> gcBuffer;

    for (int i = 0 ; i < LENGTH ; i++) {
        gcBuffer.Reset();
        gcBuffer.Push(i);
    }
    EXPECT_EQ(gcBuffer.Count(), 1);
    // reset count to zero
    gcBuffer.Reset();
    EXPECT_EQ(gcBuffer.Count(), 0);
}
} // namespace panda::ecmascript