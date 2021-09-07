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

#include <iomanip>
#include <sstream>

#include "algorithm"
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/builtins/builtins_json.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_invoker.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsJsonTest : public testing::Test {
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

    class TestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestForParse(EcmaRuntimeCallInfo *argv)
        {
            array_size_t argc = argv->GetArgsNumber();
            if (argc > 0) {
            }
            JSTaggedValue key = GetCallArg(argv, 0).GetTaggedValue();
            if (key.IsUndefined()) {
                return JSTaggedValue::Undefined();
            }
            JSTaggedValue value = GetCallArg(argv, 1).GetTaggedValue();
            if (value.IsUndefined()) {
                return JSTaggedValue::Undefined();
            }

            return JSTaggedValue(value);
        }

        static JSTaggedValue TestForParse1(EcmaRuntimeCallInfo *argv)
        {
            array_size_t argc = argv->GetArgsNumber();
            if (argc > 0) {
            }
            return JSTaggedValue::Undefined();
        }

        static JSTaggedValue TestForStringfy(EcmaRuntimeCallInfo *argv)
        {
            array_size_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                JSTaggedValue key = GetCallArg(argv, 0).GetTaggedValue();
                if (key.IsUndefined()) {
                    return JSTaggedValue::Undefined();
                }
                JSTaggedValue value = GetCallArg(argv, 1).GetTaggedValue();
                if (value.IsUndefined()) {
                    return JSTaggedValue::Undefined();
                }
                return JSTaggedValue(value);
            }

            return JSTaggedValue::Undefined();
        }
    };
};

JSTaggedValue CreateBuiltinJSObject1(JSThread *thread, const CString keyCStr)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> objectFunc(globalEnv->GetObjectFunction());

    JSHandle<JSObject> jsobject(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc));
    EXPECT_TRUE(*jsobject != nullptr);

    JSHandle<JSTaggedValue> key(factory->NewFromCanBeCompressString(&keyCStr[0]));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsobject), key, value);

    CString str2 = "y";
    JSHandle<JSTaggedValue> key2(factory->NewFromCanBeCompressString(str2));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(2.5)); // 2.5 : test case
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsobject), key2, value2);

    CString str3 = "z";
    JSHandle<JSTaggedValue> key3(factory->NewFromCanBeCompressString(str3));
    JSHandle<JSTaggedValue> value3(factory->NewFromCanBeCompressString("abc"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsobject), key3, value3);

    return jsobject.GetTaggedValue();
}
// Math.abs(-10)

HWTEST_F_L0(BuiltinsJsonTest, Parse10)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> msg(factory->NewFromCanBeCompressString(
        "\t\r \n{\t\r \n \"property\"\t\r \n:\t\r \n{\t\r \n}\t\r \n,\t\r \n \"prop2\"\t\r \n:\t\r \n [\t\r \ntrue\t\r "
        "\n,\t\r \nnull\t\r \n,123.456\t\r \n] \t\r \n}\t\r \n"));
    JSHandle<EcmaString> str(JSTaggedValue::ToString(thread, msg));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsJson::Parse(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsJsonTest, Parse21)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> msg(factory->NewFromCanBeCompressString("[100,2.5,\"abc\"]"));

    JSHandle<JSFunction> handleFunc = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForParse));
    JSHandle<EcmaString> str(JSTaggedValue::ToString(thread, msg));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, handleFunc.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsJson::Parse(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsJsonTest, Parse)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();

    JSHandle<JSTaggedValue> msg(factory->NewFromCanBeCompressString("[100,2.5,\"abc\"]"));
    JSHandle<EcmaString> str(JSTaggedValue::ToString(thread, msg));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsJson::Parse(ecmaRuntimeCallInfo.get());
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    JSHandle<JSObject> valueHandle(thread, value);
    JSHandle<JSTaggedValue> lenResult =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle).GetValue();
    uint32_t length = JSTaggedValue::ToLength(thread, lenResult).ToUint32();
    EXPECT_EQ(length, 3);
}

HWTEST_F_L0(BuiltinsJsonTest, Parse2)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> msg(factory->NewFromCanBeCompressString("{\"epf\":100,\"key1\":200}"));
    JSHandle<EcmaString> str(JSTaggedValue::ToString(thread, msg));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsJson::Parse(ecmaRuntimeCallInfo.get());
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    JSHandle<JSObject> valueHandle(thread, value);

    JSHandle<TaggedArray> nameList(JSObject::EnumerableOwnNames(thread, valueHandle));
    JSHandle<JSArray> nameResult = JSArray::CreateArrayFromList(thread, nameList);

    JSHandle<JSTaggedValue> handleKey(nameResult);
    JSHandle<JSTaggedValue> lengthKey(factory->NewFromCanBeCompressString("length"));
    JSHandle<JSTaggedValue> lenResult = JSObject::GetProperty(thread, handleKey, lengthKey).GetValue();
    uint32_t length = JSTaggedValue::ToLength(thread, lenResult).ToUint32();
    ASSERT_EQ(length, 2);
}

HWTEST_F_L0(BuiltinsJsonTest, Stringify11)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateBuiltinJSObject1(thread, "x"));
    JSHandle<JSFunction> handleFunc =
        factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForStringfy));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, handleFunc.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsJson::Stringify(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
}

HWTEST_F_L0(BuiltinsJsonTest, Stringify12)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateBuiltinJSObject1(thread, "x"));
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> handleFunc =
        factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForStringfy));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, handleFunc.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(10)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsJson::Stringify(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
}

HWTEST_F_L0(BuiltinsJsonTest, Stringify13)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateBuiltinJSObject1(thread, "x"));
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> handleFunc =
        factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForStringfy));
    JSHandle<JSTaggedValue> msg(factory->NewFromCanBeCompressString("tttt"));
    JSHandle<EcmaString> str(JSTaggedValue::ToString(thread, msg));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, handleFunc.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsJson::Stringify(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
}

HWTEST_F_L0(BuiltinsJsonTest, Stringify14)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateBuiltinJSObject1(thread, "x"));
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());

    JSHandle<JSObject> obj1(thread, arr);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> value0(factory->NewFromCanBeCompressString("x"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key0, value0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value1(factory->NewFromCanBeCompressString("z"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key1, value1);

    JSHandle<JSTaggedValue> msg(factory->NewFromCanBeCompressString("tttt"));
    JSHandle<EcmaString> str(JSTaggedValue::ToString(thread, msg));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, obj1.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsJson::Stringify(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
}

HWTEST_F_L0(BuiltinsJsonTest, Stringify)
{
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateBuiltinJSObject1(thread, "x"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsJson::Stringify(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
}

HWTEST_F_L0(BuiltinsJsonTest, Stringify1)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());

    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));

    JSHandle<JSTaggedValue> value(factory->NewFromCanBeCompressString("def"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key0, value);

    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(200)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);

    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value2(factory->NewFromCanBeCompressString("abc"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key2, value2);

    JSHandle<JSFunction> handleFunc =
        factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForStringfy));
    JSHandle<JSTaggedValue> msg(factory->NewFromCanBeCompressString("tttt"));
    JSHandle<EcmaString> str(JSTaggedValue::ToString(thread, msg));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, handleFunc.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsJson::Stringify(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
}

HWTEST_F_L0(BuiltinsJsonTest, Stringify2)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    // 2.5 : test case
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2.5)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    // 2 : test case
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value2(factory->NewFromCanBeCompressString("abc"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key2, value2);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsJson::Stringify(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsString());
}
}  // namespace panda::test
