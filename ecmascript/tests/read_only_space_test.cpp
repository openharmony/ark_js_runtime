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
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"
#include "generated/base_options.h"

#include <csetjmp>
#include <csignal>
using namespace panda::ecmascript;

namespace panda::test {
class ReadOnlySpaceTest : public testing::Test {
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
        InitializeLogger();
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
        factory = thread->GetEcmaVM()->GetFactory();
        const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->SetMarkType(MarkType::MARK_FULL);
    }

    void InitializeLogger()
    {
        base_options::Options baseOptions("");
        baseOptions.SetLogLevel("error");
        arg_list_t logComponents;
        logComponents.emplace_back("all");
        baseOptions.SetLogComponents(logComponents);
        Logger::Initialize(baseOptions);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    JSThread *thread {nullptr};
    ObjectFactory *factory {nullptr};
    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
};

static sigjmp_buf g_env;
static bool g_segmentfault_flag = false;
class ReadOnlyTestManager {
public:
    // static constexpr int RO_SEGMENTFAULT = 1;
    static void ProcessReadOnlySegmentFault(int sig)
    {
        g_segmentfault_flag = true;
        siglongjmp(g_env, sig);
    }

    static int RegisterSignal()
    {
        struct sigaction act;
        act.sa_handler = ProcessReadOnlySegmentFault;
        sigemptyset(&act.sa_mask);
        sigaddset(&act.sa_mask, SIGQUIT);
        act.sa_flags = SA_RESETHAND;
        return sigaction(SIGSEGV, &act, NULL);
    }
};

HWTEST_F_L0(ReadOnlySpaceTest, ReadOnlyTest)
{
    auto *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    if (ReadOnlyTestManager::RegisterSignal() == -1) {
        perror("sigaction error");
        exit(1);
    }
    auto ret = sigsetjmp(g_env, 1);
    if (ret != SIGSEGV) {
        heap->AllocateReadOnlyOrHugeObject(
            JSHClass::Cast(thread->GlobalConstants()->GetBigIntClass().GetTaggedObject()));
    } else {
        // catch signal SIGSEGV caused by modify read only memory
        EXPECT_TRUE(g_segmentfault_flag);
    }
}

HWTEST_F_L0(ReadOnlySpaceTest, AllocateTest)
{
    auto *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetReadOnlySpace()->ClearReadOnly();
    auto *object = heap->AllocateReadOnlyOrHugeObject(
        JSHClass::Cast(thread->GlobalConstants()->GetBigIntClass().GetTaggedObject()));
    auto *region = Region::ObjectAddressToRange(object);
    EXPECT_TRUE(region->InReadOnlySpace());
}

HWTEST_F_L0(ReadOnlySpaceTest, CompactHeapBeforeForkTest)
{
    auto *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetReadOnlySpace()->ClearReadOnly();
    std::string rawStr = "test string";
    JSHandle<EcmaString> string = factory->NewFromStdString(rawStr);
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    auto *regionBefore = Region::ObjectAddressToRange(string.GetObject<TaggedObject>());
    auto *objRegionBefore = Region::ObjectAddressToRange(obj.GetObject<TaggedObject>());
    EXPECT_FALSE(regionBefore->InReadOnlySpace());
    EXPECT_FALSE(objRegionBefore->InReadOnlySpace());
    heap->CompactHeapBeforeFork();
    auto *regionAfter = Region::ObjectAddressToRange(string.GetObject<TaggedObject>());
    auto *objRegionAfter = Region::ObjectAddressToRange(obj.GetObject<TaggedObject>());
    EXPECT_TRUE(regionAfter->InReadOnlySpace());
    EXPECT_FALSE(objRegionAfter->InReadOnlySpace());
}

HWTEST_F_L0(ReadOnlySpaceTest, GCTest)
{
    auto *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
    heap->GetReadOnlySpace()->ClearReadOnly();
    auto *object = heap->AllocateReadOnlyOrHugeObject(
        JSHClass::Cast(thread->GlobalConstants()->GetBigIntClass().GetTaggedObject()));
    heap->CollectGarbage(TriggerGCType::YOUNG_GC);
    heap->CollectGarbage(TriggerGCType::OLD_GC);
    heap->CollectGarbage(TriggerGCType::FULL_GC);
    auto *region = Region::ObjectAddressToRange(object);
    EXPECT_TRUE(region->InReadOnlySpace());
}
}  // namespace panda::test
