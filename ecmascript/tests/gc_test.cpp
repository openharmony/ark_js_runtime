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
#include "ecmascript/mem/full_gc.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/mem/stw_young_gc.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class GCTest : public testing::Test {
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
        JSRuntimeOptions options;
        instance = JSNApi::CreateEcmaVM(options);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        scope = new EcmaHandleScope(thread);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(GCTest, FullGCOne)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto heap = thread->GetEcmaVM()->GetHeap();
    auto fullGc = heap->GetFullGC();
    fullGc->RunPhases();
    auto oldSizebase = heap->GetOldSpace()->GetHeapObjectSize();
    size_t oldSizeBefore = 0;
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope baseScope(thread);
        for (int i = 0; i < 1024; i++) {
            factory->NewTaggedArray(512, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        }
        oldSizeBefore = heap->GetOldSpace()->GetHeapObjectSize();
        EXPECT_TRUE(oldSizeBefore > oldSizebase);
    }
    fullGc->RunPhases();
    auto oldSizeAfter = heap->GetOldSpace()->GetHeapObjectSize();
    EXPECT_TRUE(oldSizeBefore > oldSizeAfter);
}

HWTEST_F_L0(GCTest, ChangeGCParams)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    EXPECT_EQ(heap->GetMemGrowingType(), MemGrowingType::HIGH_THROUGHPUT);
    uint32_t markTaskNum = heap->GetMaxMarkTaskCount();
    uint32_t evacuateTaskNum = heap->GetMaxEvacuateTaskCount();

    auto partialGc = heap->GetPartialGC();
    partialGc->RunPhases();
    heap->ChangeGCParams(true);
    heap->Prepare();
    uint32_t markTaskNumBackground = heap->GetMaxMarkTaskCount();
    uint32_t evacuateTaskNumBackground = heap->GetMaxEvacuateTaskCount();
    EXPECT_TRUE(markTaskNum > markTaskNumBackground);
    EXPECT_TRUE(evacuateTaskNum > evacuateTaskNumBackground);
    EXPECT_EQ(heap->GetMemGrowingType(), MemGrowingType::CONSERVATIVE);

    partialGc->RunPhases();
    heap->ChangeGCParams(false);
    heap->Prepare();
    uint32_t markTaskNumForeground = heap->GetMaxMarkTaskCount();
    uint32_t evacuateTaskNumForeground = heap->GetMaxEvacuateTaskCount();
    EXPECT_EQ(markTaskNum, markTaskNumForeground);
    EXPECT_EQ(evacuateTaskNum, evacuateTaskNumForeground);
    EXPECT_EQ(heap->GetMemGrowingType(), MemGrowingType::HIGH_THROUGHPUT);
}

HWTEST_F_L0(GCTest, NotifyMemoryPressure)
{
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    EXPECT_EQ(heap->GetMemGrowingType(), MemGrowingType::HIGH_THROUGHPUT);
    uint32_t markTaskNum = heap->GetMaxMarkTaskCount();
    uint32_t evacuateTaskNum = heap->GetMaxEvacuateTaskCount();

    auto partialGc = heap->GetPartialGC();
    partialGc->RunPhases();
    heap->ChangeGCParams(true);
    heap->NotifyMemoryPressure(true);
    heap->Prepare();
    uint32_t markTaskNumBackground = heap->GetMaxMarkTaskCount();
    uint32_t evacuateTaskNumBackground = heap->GetMaxEvacuateTaskCount();
    EXPECT_TRUE(markTaskNum > markTaskNumBackground);
    EXPECT_TRUE(evacuateTaskNum > evacuateTaskNumBackground);
    EXPECT_EQ(heap->GetMemGrowingType(), MemGrowingType::PRESSURE);

    partialGc->RunPhases();
    heap->ChangeGCParams(false);
    heap->Prepare();
    uint32_t markTaskNumForeground = heap->GetMaxMarkTaskCount();
    uint32_t evacuateTaskNumForeground = heap->GetMaxEvacuateTaskCount();
    EXPECT_EQ(markTaskNum, markTaskNumForeground);
    EXPECT_EQ(evacuateTaskNum, evacuateTaskNumForeground);
    EXPECT_EQ(heap->GetMemGrowingType(), MemGrowingType::PRESSURE);

    heap->NotifyMemoryPressure(false);
    EXPECT_EQ(heap->GetMemGrowingType(), MemGrowingType::CONSERVATIVE);
}
}  // namespace panda::test
