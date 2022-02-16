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
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);

    // check result
    EXPECT_TRUE(factory->NewFromString("test1")->Compare(*reinterpret_cast<EcmaString **>(globalString)) == 0);
}

HWTEST_F_L0(JSHandleTest, NewGlobalHandle1)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto global = thread->GetEcmaGlobalStorage();

    uintptr_t globalString[600] = {0};
    {
        [[maybe_unused]] EcmaHandleScope scope(thread);
        for (int i = 0; i < 600; i++) {
            std::string test = "test" + std::to_string(i);
            auto string1 = factory->NewFromString(test.c_str());
            globalString[i] = global->NewGlobalHandle(string1.GetTaggedType());
        }
    }
    // trigger GC
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);
    for (int i = 300; i > 200; i--) {
        global->DisposeGlobalHandle(globalString[i]);
    }
    // check result
    for (int i = 0; i <= 200; i++) {
        std::string test = "test" + std::to_string(i);
        EXPECT_TRUE(
            factory->NewFromString(test.c_str())->Compare(*reinterpret_cast<EcmaString **>(globalString[i])) == 0);
    }
    // trigger GC
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);
    for (int i = 301; i < 600; i++) {
        std::string test = "test" + std::to_string(i);
        EXPECT_TRUE(
            factory->NewFromString(test.c_str())->Compare(*reinterpret_cast<EcmaString **>(globalString[i])) == 0);
    }
}

HWTEST_F_L0(JSHandleTest, DisposeGlobalHandle)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto global = thread->GetEcmaGlobalStorage();

    uintptr_t globalString[600] = {0};
    {
        [[maybe_unused]] EcmaHandleScope scope(thread);
        for (int i = 0; i < 600; i++) {
            std::string test = "test" + std::to_string(i);
            auto string1 = factory->NewFromString(test.c_str());
            globalString[i] = global->NewGlobalHandle(string1.GetTaggedType());
        }
    }
    for (int i = 512; i > 200; i--) {
        global->DisposeGlobalHandle(globalString[i]);
    }
    int count = 0;
    global->IterateUsageGlobal([&count] (EcmaGlobalStorage::Node *node) {
        JSTaggedValue value(node->GetObject());
        EXPECT_TRUE(value.IsString());
        count++;
    });
    EXPECT_TRUE(count == 288);
}

HWTEST_F_L0(JSHandleTest, DisposeAndNewGlobalHandle)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto global = thread->GetEcmaGlobalStorage();

    uintptr_t globalString[768] = {0};
    {
        [[maybe_unused]] EcmaHandleScope scope(thread);
        for (int i = 0; i < 768; i++) {
            std::string test = "test" + std::to_string(i);
            auto string1 = factory->NewFromString(test.c_str());
            globalString[i] = global->NewGlobalHandle(string1.GetTaggedType());
        }
    }
    for (int i = 767; i > 200; i--) {
        global->DisposeGlobalHandle(globalString[i]);
    }
    // trigger GC
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);
    {
        [[maybe_unused]] EcmaHandleScope scope(thread);
        for (int i = 200; i < 400; i++) {
            std::string test = "test" + std::to_string(i);
            auto string1 = factory->NewFromString(test.c_str());
            globalString[i] = global->NewGlobalHandle(string1.GetTaggedType());
        }
    }
    // check result
    for (int i = 0; i <= 300; i++) {
        std::string test = "test" + std::to_string(i);
        EXPECT_TRUE(
            factory->NewFromString(test.c_str())->Compare(*reinterpret_cast<EcmaString **>(globalString[i])) == 0);
    }
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
        thread->GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);

        // check result
        EXPECT_TRUE(factory->NewFromString("test1")->Compare(*reinterpret_cast<EcmaString **>(globalString)) == 0);
        EXPECT_TRUE(global->IsWeak(globalString));
    }
    // trigger GC
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);

    // check weak reference
    JSTaggedType result = *reinterpret_cast<JSTaggedType *>(globalString);
    EXPECT_TRUE(result == JSTaggedValue::Undefined().GetRawData());
}

HWTEST_F_L0(JSHandleTest, NewWeakGlobalHandle1)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto global = thread->GetEcmaGlobalStorage();

    uintptr_t globalString[800] = {0};
    {
        [[maybe_unused]] EcmaHandleScope scope(thread);
        for (int i = 0; i < 800; i++) {
            std::string test = "test" + std::to_string(i);
            auto string1 = factory->NewFromString(test.c_str());
            globalString[i] = global->NewGlobalHandle(string1.GetTaggedType());
            globalString[i] = global->SetWeak(globalString[i]);
            EXPECT_TRUE(global->IsWeak(globalString[i]));
        }
        for (int i = 600; i > 200; i--) {
            global->DisposeGlobalHandle(globalString[i]);
        }
        // trigger GC
        thread->GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);
        // check result
        for (int i = 0; i <= 200; i++) {
            std::string test = "test" + std::to_string(i);
            EXPECT_TRUE(
                factory->NewFromString(test.c_str())->Compare(*reinterpret_cast<EcmaString **>(globalString[i])) == 0);
        }
    }
    // trigger GC
    thread->GetEcmaVM()->CollectGarbage(TriggerGCType::FULL_GC);
    for (int i = 601; i < 800; i++) {
        EXPECT_TRUE(*reinterpret_cast<TaggedType *>(globalString[i]) == JSTaggedValue::Undefined().GetRawData());
    }
}
}  // namespace panda::test
