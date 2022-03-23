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

using namespace panda::ecmascript;

namespace panda::test {
class ConcurrentSweepTest : public testing::Test {
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
    JSThread *thread;
};

TEST_F(ConcurrentSweepTest, ConcurrentSweep)
{
    auto vm = EcmaVM::Cast(instance);
    const uint8_t *utf8 = reinterpret_cast<const uint8_t *>("test");
    JSHandle<EcmaString> test1(thread, EcmaString::CreateFromUtf8(utf8, 4, vm, false));
    if (vm->IsInitialized()) {
        vm->CollectGarbage(ecmascript::TriggerGCType::OLD_GC);
    }
    JSHandle<EcmaString> test2(thread, EcmaString::CreateFromUtf8(utf8, 4, vm, false));
    ASSERT_EQ(test1->GetLength(), 4U);
    ASSERT_NE(test1.GetTaggedValue().GetHeapObject(), test2.GetTaggedValue().GetHeapObject());
}
}  // namespace panda::test
