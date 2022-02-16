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

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_thread.h"

#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class MemControllerTest : public testing::Test {
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

    PandaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(MemControllerTest, AllocationVerify)
{
#ifdef NDEBUG
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    auto memController = heap->GetMemController();

    heap->CollectGarbage(TriggerGCType::FULL_GC);

    for (int i = 0; i < 1024; i++) {
        // old space object
        [[maybe_unused]] auto oldArray = objectFactory->NewTaggedArray(128, JSTaggedValue::Undefined(),
                                                                       MemSpaceType::OLD_SPACE);
    }
    sleep(5);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    double mutatorSpeed1 = memController->GetCurrentOldSpaceAllocationThroughtputPerMS(0);
    for (int i = 0; i < 1024; i++) {
        // old space object
        [[maybe_unused]] auto oldArray = objectFactory->NewTaggedArray(128, JSTaggedValue::Undefined(),
                                                                       MemSpaceType::OLD_SPACE);
    }
    sleep(10);

    heap->CollectGarbage(TriggerGCType::FULL_GC);
    double mutatorSpeed2 = memController->GetCurrentOldSpaceAllocationThroughtputPerMS(0);
    ASSERT_TRUE(mutatorSpeed2 < mutatorSpeed1);
#endif
}

HWTEST_F_L0(MemControllerTest, VerifyMutatorSpeed)
{
#ifdef NDEBUG
    auto ecmaVm = thread->GetEcmaVM();
    auto heap = const_cast<Heap *>(ecmaVm->GetHeap());
    auto objectFactory = ecmaVm->GetFactory();
    auto memController = heap->GetMemController();

    heap->CollectGarbage(TriggerGCType::SEMI_GC);
    size_t oldSpaceAllocatedSizeBefore = memController->GetOldSpaceAllocAccumulatorSize();
    size_t nonMovableSpaceAllocatedSizeBefore = memController->GetNonMovableSpaceAllocAccumulatorSize();
    double allocDurationBefore = memController->GetAllocTimeMs();
    sleep(1);

    // new space object
    auto newArray = objectFactory->NewTaggedArray(2, JSTaggedValue::Undefined(), MemSpaceType::SEMI_SPACE);
    // old space object
    auto oldArray = objectFactory->NewTaggedArray(2, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    // non movable object
    auto nonMovableArray = objectFactory->NewTaggedArray(2, JSTaggedValue::Undefined(), MemSpaceType::NON_MOVABLE);

    // huge space object
    static constexpr size_t SIZE = 1024 * 1024;
    auto hugeArray = objectFactory->NewTaggedArray(SIZE);

    ASSERT_TRUE(heap->GetNewSpace()->GetAllocatedSizeSinceGC()
                == newArray->ComputeSize(JSTaggedValue::TaggedTypeSize(), 2));

    heap->CollectGarbage(TriggerGCType::SEMI_GC);

    size_t oldSpaceAllocatedSizeAfter = memController->GetOldSpaceAllocAccumulatorSize();
    size_t nonMovableSpaceAllocatedSizeAfter = memController->GetNonMovableSpaceAllocAccumulatorSize();
    double allocDurationAfter = memController->GetAllocTimeMs();

    size_t hugeObjectAllocSizeInLastGC = memController->GetHugeObjectAllocSizeSinceGC();

    ASSERT_TRUE(allocDurationAfter - allocDurationBefore > 1000);
    ASSERT_TRUE(oldSpaceAllocatedSizeAfter - oldSpaceAllocatedSizeBefore
                == oldArray->ComputeSize(JSTaggedValue::TaggedTypeSize(), 2));
    ASSERT_TRUE(nonMovableSpaceAllocatedSizeAfter - nonMovableSpaceAllocatedSizeBefore
                == nonMovableArray->ComputeSize(JSTaggedValue::TaggedTypeSize(), 2));
    // The allocated size of huge object must be larger than the object size.
    ASSERT_TRUE(hugeObjectAllocSizeInLastGC > hugeArray->ComputeSize(JSTaggedValue::TaggedTypeSize(), SIZE));
#endif
}
}  // namespace panda::test
