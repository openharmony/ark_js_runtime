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

#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_global_storage-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JSHandleTest : public testing::Test {
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
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(JSHandleTest, NewGlobalHandle)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto global = thread->GetEcmaGlobalStorage();

    uintptr_t globalString = 0;
    {
        [[maybe_unused]] EcmaHandleScope scope(thread);
        auto string1 = factory->NewFromString("test1");
        globalString = global->NewGlobalHandle(string1.GetTaggedType());
    }
    // trigger GC
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::COMPRESS_FULL_GC);

    // check result
    EXPECT_TRUE(factory->NewFromString("test1")->Compare(*reinterpret_cast<EcmaString **>(globalString)) == 0);
}

HWTEST_F_L0(JSHandleTest, NewWeakGlobalHandle)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto global = thread->GetEcmaGlobalStorage();

    uintptr_t globalString = 0;
    {
        [[maybe_unused]] EcmaHandleScope scope(thread);
        auto string1 = factory->NewFromString("test1");
        globalString = global->NewGlobalHandle(string1.GetTaggedType());
        globalString = global->SetWeak(globalString);

        // trigger GC
        thread->GetEcmaVM()->CollectGarbage(TriggerGCType::COMPRESS_FULL_GC);

        // check result
        EXPECT_TRUE(factory->NewFromString("test1")->Compare(*reinterpret_cast<EcmaString **>(globalString)) == 0);
        EXPECT_TRUE(global->IsWeak(globalString));
    }
    // trigger GC
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::COMPRESS_FULL_GC);

    // check weak reference
    JSTaggedType result = *reinterpret_cast<JSTaggedType *>(globalString);
    EXPECT_TRUE(result == JSTaggedValue::Undefined().GetRawData());
}
}  // namespace panda::test
