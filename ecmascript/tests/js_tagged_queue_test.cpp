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

#include "ecmascript/ecma_vm.h"

#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/snapshot/mem/snapshot.h"
#include "ecmascript/tagged_queue.h"
#include "ecmascript/tagged_queue.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JSTaggedQueueTest : public testing::Test {
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
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(JSTaggedQueueTest, Create)
{
    JSHandle<TaggedQueue> queue = thread->GetEcmaVM()->GetFactory()->NewTaggedQueue(0);
    EXPECT_TRUE(*queue != nullptr);
    EXPECT_TRUE(queue->Empty());
    EXPECT_EQ(queue->Size(), 0U);
    EXPECT_EQ(queue->Front(), JSTaggedValue::Hole());
    EXPECT_EQ(queue->Back(), JSTaggedValue::Hole());
}

HWTEST_F_L0(JSTaggedQueueTest, PopAndPush)
{
    JSHandle<TaggedQueue> queue = thread->GetEcmaVM()->GetFactory()->NewTaggedQueue(0);
    EXPECT_TRUE(queue->Empty());

    JSHandle<TaggedQueue> queue2(thread,
                                 TaggedQueue::Push(thread, queue, JSHandle<JSTaggedValue>(thread, JSTaggedValue(0))));
    EXPECT_FALSE(queue2->Empty());
    EXPECT_EQ(queue2->Size(), 1U);
    EXPECT_EQ(queue2->Front(), JSTaggedValue(0));
    EXPECT_EQ(queue2->Back(), JSTaggedValue(0));

    JSHandle<TaggedQueue> queue3(thread,
                                 TaggedQueue::Push(thread, queue2, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1))));
    EXPECT_EQ(queue3->Size(), 2U);
    EXPECT_EQ(queue3->Front(), JSTaggedValue(0));
    EXPECT_EQ(queue3->Back(), JSTaggedValue(1));
    EXPECT_NE(queue3.GetTaggedValue(), queue2.GetTaggedValue());

    JSHandle<TaggedQueue> queue4(thread,
                                 TaggedQueue::Push(thread, queue3, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2))));
    EXPECT_EQ(queue4->Size(), 3U);
    EXPECT_EQ(queue4->Front(), JSTaggedValue(0));
    EXPECT_EQ(queue4->Back(), JSTaggedValue(2));
    EXPECT_NE(queue4.GetTaggedValue(), queue3.GetTaggedValue());

    JSHandle<TaggedQueue> queue5(thread,
                                 TaggedQueue::Push(thread, queue4, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3))));
    EXPECT_EQ(queue5->Size(), 4U);
    EXPECT_EQ(queue5->Front(), JSTaggedValue(0));
    EXPECT_EQ(queue5->Back(), JSTaggedValue(3));
    EXPECT_NE(queue5.GetTaggedValue(), queue4.GetTaggedValue());

    EXPECT_EQ(queue5->Pop(thread), JSTaggedValue(0));
    EXPECT_EQ(queue5->Size(), 3U);
    EXPECT_EQ(queue5->Front(), JSTaggedValue(1));

    EXPECT_EQ(queue5->Pop(thread), JSTaggedValue(1));
    EXPECT_EQ(queue5->Size(), 2U);
    EXPECT_EQ(queue5->Front(), JSTaggedValue(2));

    EXPECT_EQ(queue5->Pop(thread), JSTaggedValue(2));
    EXPECT_EQ(queue5->Size(), 1U);
    EXPECT_EQ(queue5->Front(), JSTaggedValue(3));

    EXPECT_EQ(queue5->Pop(thread), JSTaggedValue(3));
    EXPECT_EQ(queue5->Size(), 0U);
    EXPECT_EQ(queue5->Front(), JSTaggedValue::Hole());
    EXPECT_TRUE(queue5->Empty());
}
}  // namespace panda::test
