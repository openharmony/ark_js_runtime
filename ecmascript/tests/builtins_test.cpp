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

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/builtins.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_thread.h"
#include "thread_manager.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class BuiltinsTest : public testing::Test {
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

HWTEST_F_L0(BuiltinsTest, ObjectInit)
{
    ASSERT_NE(thread, nullptr);
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSFunction> objectFunction(env->GetObjectFunction());
    ASSERT_NE(*objectFunction, nullptr);
}

HWTEST_F_L0(BuiltinsTest, FunctionInit)
{
    ASSERT_NE(thread, nullptr);
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSFunction> functionFunction(env->GetFunctionFunction());
    ASSERT_NE(*functionFunction, nullptr);
}

HWTEST_F_L0(BuiltinsTest, NumberInit)
{
    ASSERT_NE(thread, nullptr);
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSFunction> numberFunction(env->GetNumberFunction());
    ASSERT_NE(*numberFunction, nullptr);
}

HWTEST_F_L0(BuiltinsTest, SetInit)
{
    ASSERT_NE(thread, nullptr);
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSFunction> setFunction(env->GetBuiltinsSetFunction());
    ASSERT_NE(*setFunction, nullptr);
}

HWTEST_F_L0(BuiltinsTest, MapInit)
{
    ASSERT_NE(thread, nullptr);
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSFunction> mapFunction(env->GetBuiltinsMapFunction());
    ASSERT_NE(*mapFunction, nullptr);
}

HWTEST_F_L0(BuiltinsTest, StrictModeForbiddenAccess)
{
    ASSERT_NE(thread, nullptr);
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSFunction> function = factory->NewJSFunction(env, static_cast<void *>(nullptr));

    JSHandle<JSTaggedValue> callerKey(factory->NewFromString("caller"));
    JSHandle<JSTaggedValue> argumentsKey(factory->NewFromString("arguments"));

    JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(function), callerKey);
    ASSERT_EQ(thread->HasPendingException(), true);

    JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(function), argumentsKey);
    ASSERT_EQ(thread->HasPendingException(), true);
}
}  // namespace panda::test
