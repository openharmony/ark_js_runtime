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

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/builtins.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/tests/test_helper.h"
#include "thread_manager.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class GlueRegsTest : public testing::Test {
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

HWTEST_F_L0(GlueRegsTest, ConstantClassTest)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ASSERT_NE(globalConst, nullptr);

    const JSTaggedValue *address = globalConst->BeginSlot();
    while (address < globalConst->EndSlot()) {
        EXPECT_TRUE(!(*address).IsNull());  // Visit barely
        address += sizeof(JSTaggedValue);
    }
}

HWTEST_F_L0(GlueRegsTest, ConstantSpecialTest)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ASSERT_NE(globalConst, nullptr);

    EXPECT_TRUE(globalConst->GetUndefined().IsUndefined());
    EXPECT_TRUE(globalConst->GetHandledUndefined()->IsUndefined());
    EXPECT_TRUE(globalConst->GetNull().IsNull());
    EXPECT_TRUE(globalConst->GetHandledNull()->IsNull());
    EXPECT_TRUE(globalConst->GetEmptyString().IsString());
    EXPECT_TRUE(globalConst->GetHandledEmptyString()->IsString());
}

HWTEST_F_L0(GlueRegsTest, ConstantStringTest)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ASSERT_NE(globalConst, nullptr);

#define CONSTANT_STRING_ITERATOR(Type, Name, Index, Desc)                \
    Type Name##value = globalConst->Get##Name();                         \
    EXPECT_TRUE(!Name##value.IsNull());                                  \
    JSHandle<Type> Name##handledValue = globalConst->GetHandled##Name(); \
    EXPECT_TRUE(!Name##handledValue->IsNull());
    GLOBAL_ENV_CONSTANT_CONSTANT(CONSTANT_STRING_ITERATOR)
#undef CONSTANT_STRING_ITERATOR
}

HWTEST_F_L0(GlueRegsTest, ConstantAccessorTest)
{
    const GlobalEnvConstants *globalConst = thread->GlobalConstants();
    ASSERT_NE(globalConst, nullptr);

#define CONSTANT_ACCESSOR_ITERATOR(Type, Name, Index, Desc)              \
    Type Name##value = globalConst->Get##Name();                         \
    EXPECT_TRUE(!Name##value.IsNull());                                  \
    JSHandle<Type> Name##handledValue = globalConst->GetHandled##Name(); \
    EXPECT_TRUE(!Name##handledValue->IsNull());
    GLOBAL_ENV_CONSTANT_ACCESSOR(CONSTANT_ACCESSOR_ITERATOR)
#undef CONSTANT_ACCESSOR_ITERATOR
}
}  // namespace panda::test