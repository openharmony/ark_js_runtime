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
#include "ecmascript/mem/stw_young_gc_for_testing.h"
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
        options.SetShouldLoadBootPandaFiles(false);
        options.SetShouldInitializeIntrinsics(false);
        options.SetBootClassSpaces({"ecmascript"});
        options.SetRuntimeType("ecmascript");
        options.SetEnableParalledYoungGc(false);
        static EcmaLanguageContext lcEcma;
        [[maybe_unused]] bool success = Runtime::Create(options, {&lcEcma});
        ASSERT_TRUE(success) << "Cannot create Runtime";
        // create jspandafile manager for process
        EcmaVM::CreateJSPandaFileManager();
        instance = Runtime::GetCurrent()->GetPandaVM();
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = EcmaVM::Cast(instance)->GetJSThread();
        scope = new EcmaHandleScope(thread);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    PandaVM *instance {nullptr};
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
}  // namespace panda::test
