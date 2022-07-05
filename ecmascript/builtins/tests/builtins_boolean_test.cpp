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

#include "ecmascript/builtins/builtins_boolean.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
using BuiltinsBase = panda::ecmascript::base::BuiltinsBase;

namespace panda::test {
class BuiltinsBooleanTest : public testing::Test {
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

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

// new Boolean(123)
HWTEST_F_L0(BuiltinsBooleanTest, BooleanConstructor)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSFunction> boolean(env->GetBooleanFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*boolean), 6);
    ecmaRuntimeCallInfo->SetFunction(boolean.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(123)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsBoolean::BooleanConstructor(ecmaRuntimeCallInfo);

    ASSERT_TRUE(result.IsECMAObject());
    ASSERT_EQ(JSPrimitiveRef::Cast(result.GetTaggedObject())->GetValue().IsTrue(), 1);
}

// new Boolean(undefined)
HWTEST_F_L0(BuiltinsBooleanTest, BooleanConstructor1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSFunction> boolean(env->GetBooleanFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*boolean), 6);
    ecmaRuntimeCallInfo->SetFunction(boolean.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsBoolean::BooleanConstructor(ecmaRuntimeCallInfo);

    ASSERT_TRUE(result.IsECMAObject());
    ASSERT_EQ(JSPrimitiveRef::Cast(result.GetTaggedObject())->GetValue().IsFalse(), 1);
}

// Boolean("helloworld")
HWTEST_F_L0(BuiltinsBooleanTest, BooleanConstructor2)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSFunction> boolean(env->GetBooleanFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    JSHandle<EcmaString> str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("helloworld");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(boolean.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsBoolean::BooleanConstructor(ecmaRuntimeCallInfo);

    JSTaggedValue ruler = BuiltinsBase::GetTaggedBoolean(true);
    ASSERT_EQ(result.GetRawData(), ruler.GetRawData());
}

// false.toString()
HWTEST_F_L0(BuiltinsBooleanTest, BooleanPrototypeToString)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::False());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsBoolean::BooleanPrototypeToString(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> res(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    auto ruler = thread->GetEcmaVM()->GetFactory()->NewFromASCII("false");
    ASSERT_EQ(res->Compare(*ruler), 0);
}

// (new Boolean(true)).toString()
HWTEST_F_L0(BuiltinsBooleanTest, BooleanPrototypeToString1)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSFunction> booleanObject(env->GetBooleanFunction());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue::True());
    JSHandle<JSPrimitiveRef> boolean = thread->GetEcmaVM()->GetFactory()->NewJSPrimitiveRef(booleanObject, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(boolean.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsBoolean::BooleanPrototypeToString(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> res(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    auto ruler = thread->GetEcmaVM()->GetFactory()->NewFromASCII("true");
    ASSERT_EQ(res->Compare(*ruler), 0);
}

// true.valueOf()
HWTEST_F_L0(BuiltinsBooleanTest, BooleanPrototypeValueOf)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::True());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsBoolean::BooleanPrototypeValueOf(ecmaRuntimeCallInfo);

    JSTaggedValue ruler = BuiltinsBase::GetTaggedBoolean(true);
    ASSERT_EQ(result.GetRawData(), ruler.GetRawData());
}

// (new Boolean(false)).valueOf()
HWTEST_F_L0(BuiltinsBooleanTest, BooleanPrototypeValueOf1)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSFunction> booleanObject(env->GetBooleanFunction());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue::False());
    JSHandle<JSPrimitiveRef> boolean = thread->GetEcmaVM()->GetFactory()->NewJSPrimitiveRef(booleanObject, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(boolean.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsBoolean::BooleanPrototypeValueOf(ecmaRuntimeCallInfo);

    JSTaggedValue ruler = BuiltinsBase::GetTaggedBoolean(false);
    ASSERT_EQ(result.GetRawData(), ruler.GetRawData());
}
}  // namespace panda::test
