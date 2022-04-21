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

#include "ecmascript/js_arguments.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JsArgumentsTest : public testing::Test {
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
    ecmascript::EcmaHandleScope *scope {nullptr};
    EcmaVM *instance {nullptr};
    JSThread *thread {nullptr};
};

static JSFunction *JSObjectTestCreate(JSThread *thread)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    return globalEnv->GetObjectFunction().GetObject<JSFunction>();
}

HWTEST_F_L0(JsArgumentsTest, SetProperty)
{
    JSHandle<JSTaggedValue> argFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSObject> jsarg =
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(argFunc), argFunc);
    JSHandle<JSArguments> arg = thread->GetEcmaVM()->GetFactory()->NewJSArguments();

    char array[] = "x";
    JSHandle<JSTaggedValue> key(thread->GetEcmaVM()->GetFactory()->NewFromASCII(array));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));

    // receive must be jsarg's conversion
    JSHandle<JSTaggedValue> receiver = JSHandle<JSTaggedValue>::Cast(jsarg);
    EXPECT_TRUE(JSArguments::SetProperty(thread, arg, key, value, receiver));
    EXPECT_EQ(JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(jsarg), key).GetValue()->GetInt(), 1);
    EXPECT_EQ(JSArguments::GetProperty(thread, jsarg, key).GetValue()->GetInt(), 1);

    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(2));
    EXPECT_TRUE(JSArguments::SetProperty(thread, arg, key, value2, receiver));
    EXPECT_EQ(JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(jsarg), key).GetValue()->GetInt(), 2);
    EXPECT_EQ(JSArguments::GetProperty(thread, jsarg, key).GetValue()->GetInt(), 2);
}

HWTEST_F_L0(JsArgumentsTest, GetProperty)
{
    JSHandle<JSTaggedValue> argFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSObject> jsarg =
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(argFunc), argFunc);
    JSHandle<JSArguments> arg = thread->GetEcmaVM()->GetFactory()->NewJSArguments();

    char array[] = "x";
    JSHandle<JSTaggedValue> key(thread->GetEcmaVM()->GetFactory()->NewFromASCII(array));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));

    JSHandle<JSTaggedValue> receiver = JSHandle<JSTaggedValue>::Cast(jsarg);
    JSArguments::SetProperty(thread, arg, key, value, receiver);
    EXPECT_EQ(JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(jsarg), key).GetValue()->GetInt(), 1);
    EXPECT_EQ(JSArguments::GetProperty(thread, JSHandle<JSArguments>(jsarg), key, receiver).GetValue()->GetInt(), 1);

    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(2));
    JSArguments::SetProperty(thread, arg, key, value2, receiver);
    EXPECT_EQ(JSArguments::GetProperty(thread, jsarg, key).GetValue()->GetInt(), 2);
    EXPECT_EQ(JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(jsarg), key).GetValue()->GetInt(), 2);
}

HWTEST_F_L0(JsArgumentsTest, DeleteProperty)
{
    JSHandle<JSTaggedValue> argFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSObject> jsarg =
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(argFunc), argFunc);
    JSHandle<JSArguments> arg = thread->GetEcmaVM()->GetFactory()->NewJSArguments();

    char array[] = "delete";
    JSHandle<JSTaggedValue> key(thread->GetEcmaVM()->GetFactory()->NewFromASCII(array));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> receiver = JSHandle<JSTaggedValue>::Cast(jsarg);
    JSArguments::SetProperty(thread, arg, key, value, receiver);
    EXPECT_EQ(JSArguments::GetProperty(thread, jsarg, key).GetValue()->GetInt(), 1);

    // test delete
    bool result = JSArguments::DeleteProperty(thread, JSHandle<JSArguments>(jsarg), key);
    EXPECT_TRUE(result);
    EXPECT_TRUE(JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(jsarg), key).GetValue()->IsUndefined());
}

HWTEST_F_L0(JsArgumentsTest, DefineOwnProperty)
{
    JSHandle<JSTaggedValue> argFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSObject> jsarg =
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(argFunc), argFunc);
    JSHandle<JSArguments> arg = thread->GetEcmaVM()->GetFactory()->NewJSArguments();

    JSHandle<JSTaggedValue> key(thread->GetEcmaVM()->GetFactory()->NewFromASCII("x"));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> receiver = JSHandle<JSTaggedValue>::Cast(jsarg);
    JSArguments::SetProperty(thread, arg, key, value2, receiver);
    EXPECT_EQ(JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(jsarg), key).GetValue()->GetInt(), 2);

    PropertyDescriptor Desc(thread);
    // set value1
    Desc.SetValue(value1);
    Desc.SetWritable(false);
    EXPECT_TRUE(JSArguments::DefineOwnProperty(thread, JSHandle<JSArguments>(jsarg), key, Desc));
    EXPECT_EQ(JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(jsarg), key).GetValue()->GetInt(), 1);
}

HWTEST_F_L0(JsArgumentsTest, GetOwnProperty)
{
    JSHandle<JSTaggedValue> argFunc(thread, JSObjectTestCreate(thread));
    JSHandle<JSObject> jsarg =
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(argFunc), argFunc);
    JSHandle<JSArguments> arg = thread->GetEcmaVM()->GetFactory()->NewJSArguments();

    JSHandle<JSTaggedValue> key(thread->GetEcmaVM()->GetFactory()->NewFromASCII("x"));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> receiver = JSHandle<JSTaggedValue>::Cast(jsarg);
    JSArguments::SetProperty(thread, arg, key, value, receiver);

    PropertyDescriptor Desc(thread);
    JSHandle<EcmaString> caller = thread->GetEcmaVM()->GetFactory()->NewFromASCII("caller");
    // key is not caller
    EXPECT_FALSE(JSTaggedValue::SameValue(key.GetTaggedValue(), caller.GetTaggedValue()));
    EXPECT_TRUE(JSArguments::GetOwnProperty(thread, JSHandle<JSArguments>(jsarg), key, Desc));
    EXPECT_EQ(Desc.GetValue()->GetInt(), 1);
}
}  // namespace panda::test
