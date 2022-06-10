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
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/verification.h"

using namespace panda::ecmascript;

namespace panda::test {
class ConcurrentMarkingTest : public testing::Test {
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
        instance->SetEnableForceGC(false);
        auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
        heap->GetConcurrentMarker()->EnableConcurrentMarking(EnableConcurrentMarkType::ENABLE);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    JSHandle<TaggedArray> CreateTaggedArray(uint32_t length, JSTaggedValue initVal, MemSpaceType spaceType)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        return factory->NewTaggedArray(length, initVal, spaceType);
    }

    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(ConcurrentMarkingTest, PerformanceWithConcurrentMarking)
{
    uint32_t rootLength = 1024;
    JSHandle<TaggedArray> rootArray =
        CreateTaggedArray(rootLength, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    for (uint32_t i = 0; i < rootLength; i++) {
        uint32_t subArrayLength = 1024;
        auto array = CreateTaggedArray(subArrayLength, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        rootArray->Set(thread, i, array);
    }
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->TriggerConcurrentMarking();  // concurrent mark
    for (uint32_t i = 0; i < rootLength; i++) {
        uint32_t subArrayLength = 1024;
        auto array = CreateTaggedArray(subArrayLength, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        rootArray->Set(thread, i, array);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
}

HWTEST_F_L0(ConcurrentMarkingTest, PerformanceWithoutConcurrentMarking)
{
    uint32_t rootLength = 1024;
    JSHandle<TaggedArray> rootArray =
        CreateTaggedArray(rootLength, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
    for (uint32_t i = 0; i < rootLength; i++) {
        uint32_t subArrayLength = 1024;
        auto array = CreateTaggedArray(subArrayLength, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        rootArray->Set(thread, i, array);
    }
    auto heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    for (uint32_t i = 0; i < rootLength; i++) {
        uint32_t subArrayLength = 1024;
        auto array = CreateTaggedArray(subArrayLength, JSTaggedValue::Undefined(), MemSpaceType::OLD_SPACE);
        rootArray->Set(thread, i, array);
    }
    heap->CollectGarbage(TriggerGCType::OLD_GC);
}
}  // namespace panda::test
