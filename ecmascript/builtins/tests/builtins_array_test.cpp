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

#include "ecmascript/builtins/builtins_array.h"

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_array_iterator.h"

#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"

#include "ecmascript/object_factory.h"
#include "ecmascript/object_operator.h"
#include "ecmascript/tests/test_helper.h"
#include "utils/bit_utils.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
using namespace panda::ecmascript::base;

namespace panda::test {
using Array = ecmascript::builtins::BuiltinsArray;
class BuiltinsArrayTest : public testing::Test {
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

    class TestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
        {
            JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
            if (key->IsUndefined()) {
                return JSTaggedValue::Undefined();
            }
            JSArray *jsArray = JSArray::Cast(GetThis(argv)->GetTaggedObject());
            uint32_t length = jsArray->GetArrayLength() + 1U;
            jsArray->SetArrayLength(argv->GetThread(), length);
            return JSTaggedValue::Undefined();
        }

        static JSTaggedValue TestEveryFunc(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                if (GetCallArg(argv, 0)->GetInt() > 10) { // 10 : test case
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }

        static JSTaggedValue TestMapFunc(EcmaRuntimeCallInfo *argv)
        {
            int accumulator = GetCallArg(argv, 0)->GetInt();
            accumulator = accumulator * 2; // 2 : mapped to 2 times the original value
            return BuiltinsBase::GetTaggedInt(accumulator);
        }

        static JSTaggedValue TestFlatMapFunc(EcmaRuntimeCallInfo *argv)
        {
            int accumulator = GetCallArg(argv, 0)->GetInt();
            accumulator = accumulator * 2; // 2 : mapped to 2 times the original value

            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
            JSArray *arr =
                JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
            EXPECT_TRUE(arr != nullptr);
            JSHandle<JSObject> obj(thread, arr);
            auto property = JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle);
            EXPECT_EQ(property.GetValue()->GetInt(), 0);

            JSHandle<JSTaggedValue> key(thread, JSTaggedValue(0));
            PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(accumulator)),
                                                                    true, true, true);
            JSArray::DefineOwnProperty(thread, obj, key, desc);
            return obj.GetTaggedValue();
        }

        static JSTaggedValue TestFindFunc(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                // 10 : test case
                if (GetCallArg(argv, 0)->GetInt() > 10) {
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }

        static JSTaggedValue TestFindIndexFunc(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                // 10 : test case
                if (GetCallArg(argv, 0)->GetInt() > 10) {
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }

        static JSTaggedValue TestReduceFunc(EcmaRuntimeCallInfo *argv)
        {
            int accumulator = GetCallArg(argv, 0)->GetInt();
            accumulator = accumulator + GetCallArg(argv, 1)->GetInt();
            return BuiltinsBase::GetTaggedInt(accumulator);
        }

        static JSTaggedValue TestReduceRightFunc(EcmaRuntimeCallInfo *argv)
        {
            int accumulator = GetCallArg(argv, 0)->GetInt();
            accumulator = accumulator + GetCallArg(argv, 1)->GetInt();
            return BuiltinsBase::GetTaggedInt(accumulator);
        }

        static JSTaggedValue TestSomeFunc(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                if (GetCallArg(argv, 0)->GetInt() > 10) { // 10 : test case
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }
    };
};

JSTaggedValue CreateBuiltinsArrayJSObject(JSThread *thread, const CString keyCStr)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();

    JSHandle<JSTaggedValue> dynclass = globalEnv->GetObjectFunction();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> obj(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    JSHandle<JSTaggedValue> key(factory->NewFromASCII(&keyCStr[0]));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSObject::SetProperty(thread, obj, key, value);
    return obj.GetTaggedValue();
}

HWTEST_F_L0(BuiltinsArrayTest, ArrayConstructor)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSFunction> array(env->GetArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::ArrayConstructor(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    JSObject::GetOwnProperty(thread, valueHandle, key0, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(1));
    JSObject::GetOwnProperty(thread, valueHandle, key1, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(3));
    JSObject::GetOwnProperty(thread, valueHandle, key2, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(5));
}

// 22.1.2.1 Array.from ( items [ , mapfn [ , thisArg ] ] )
HWTEST_F_L0(BuiltinsArrayTest, From)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);
    JSHandle<JSTaggedValue> key3(thread, JSTaggedValue(3));
    PropertyDescriptor desc3(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(4)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key3, desc3);
    JSHandle<JSTaggedValue> key4(thread, JSTaggedValue(4));
    PropertyDescriptor desc4(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(5)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key4, desc4);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::From(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    JSObject::GetOwnProperty(thread, valueHandle, key0, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(1));
    JSObject::GetOwnProperty(thread, valueHandle, key1, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(2));
    JSObject::GetOwnProperty(thread, valueHandle, key2, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(3));
    JSObject::GetOwnProperty(thread, valueHandle, key3, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(4));
    JSObject::GetOwnProperty(thread, valueHandle, key4, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(5));
}

// 22.1.2.2 Array.isArray(arg)
HWTEST_F_L0(BuiltinsArrayTest, IsArray)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::IsArray(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    result = Array::IsArray(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
}

HWTEST_F_L0(BuiltinsArrayTest, Of)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSFunction> array(env->GetArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::Of(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));

    JSObject::GetOwnProperty(thread, valueHandle, key0, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(1));
    JSObject::GetOwnProperty(thread, valueHandle, key1, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(3));
    JSObject::GetOwnProperty(thread, valueHandle, key2, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(5));
}

HWTEST_F_L0(BuiltinsArrayTest, Species)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSFunction> array(env->GetArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(globalObject.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::Species(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsArrayTest, Concat)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);

    JSArray *arr1 = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    EXPECT_TRUE(arr1 != nullptr);
    JSHandle<JSObject> obj1(thread, arr1);
    JSHandle<JSTaggedValue> key4(thread, JSTaggedValue(0));
    PropertyDescriptor desc4(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(4)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj1, key4, desc4);
    JSHandle<JSTaggedValue> key5(thread, JSTaggedValue(1));
    PropertyDescriptor desc5(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(5)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj1, key5, desc5);
    JSHandle<JSTaggedValue> key6(thread, JSTaggedValue(2));
    PropertyDescriptor desc6(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(6)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj1, key6, desc6);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, obj1.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::Concat(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());

    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    JSHandle<JSTaggedValue> key7(thread, JSTaggedValue(5));
    EXPECT_EQ(
        JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle).GetValue()->GetInt(), 6);
    JSObject::GetOwnProperty(thread, valueHandle, key7, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(6));
}

// 22.1.3.3 new Array(1,2,3,4,5).CopyWithin(0,3,5)
HWTEST_F_L0(BuiltinsArrayTest, CopyWithin)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);
    JSHandle<JSTaggedValue> key3(thread, JSTaggedValue(3));
    PropertyDescriptor desc3(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(4)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key3, desc3);
    JSHandle<JSTaggedValue> key4(thread, JSTaggedValue(4));
    PropertyDescriptor desc4(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(5)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key4, desc4);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::CopyWithin(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    JSObject::GetOwnProperty(thread, valueHandle, key0, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(4));
    JSObject::GetOwnProperty(thread, valueHandle, key1, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(5));
    JSObject::GetOwnProperty(thread, valueHandle, key2, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(3));
    JSObject::GetOwnProperty(thread, valueHandle, key3, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(4));
    JSObject::GetOwnProperty(thread, valueHandle, key4, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(5));
}

HWTEST_F_L0(BuiltinsArrayTest, Every)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(100)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);

    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(200)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(300)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);

    JSHandle<JSArray> jsArray = factory->NewJSArray();
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestEveryFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result2 = Array::Every(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_EQ(result2.GetRawData(), JSTaggedValue::True().GetRawData());
}

HWTEST_F_L0(BuiltinsArrayTest, Map)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(50)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(200)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);
    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestMapFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::Map(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());

    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    EXPECT_EQ(
        JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle).GetValue()->GetInt(), 3);
    JSObject::GetOwnProperty(thread, valueHandle, key0, descRes);

    ASSERT_EQ(descRes.GetValue()->GetInt(), 100);
    JSObject::GetOwnProperty(thread, valueHandle, key1, descRes);
    ASSERT_EQ(descRes.GetValue()->GetInt(), 400);
    JSObject::GetOwnProperty(thread, valueHandle, key2, descRes);
    ASSERT_EQ(descRes.GetValue()->GetInt(), 6);
}

HWTEST_F_L0(BuiltinsArrayTest, Reverse)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(50)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(200)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::Reverse(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());

    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    EXPECT_EQ(
        JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle).GetValue()->GetInt(), 3);
    JSObject::GetOwnProperty(thread, valueHandle, key0, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(3));
    JSObject::GetOwnProperty(thread, valueHandle, key1, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(200));
    JSObject::GetOwnProperty(thread, valueHandle, key2, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(50));
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 3);
    JSObject::GetOwnProperty(thread, obj, key0, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(3));
    JSObject::GetOwnProperty(thread, obj, key1, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(200));
    JSObject::GetOwnProperty(thread, obj, key2, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(50));
}

HWTEST_F_L0(BuiltinsArrayTest, Slice)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);
    JSHandle<JSTaggedValue> key3(thread, JSTaggedValue(3));
    PropertyDescriptor desc3(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(4)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key3, desc3);
    JSHandle<JSTaggedValue> key4(thread, JSTaggedValue(4));
    PropertyDescriptor desc4(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key4, desc4);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(4)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::Slice(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());

    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    EXPECT_EQ(
        JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle).GetValue()->GetInt(), 3);
    JSObject::GetOwnProperty(thread, valueHandle, key0, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(2));
    JSObject::GetOwnProperty(thread, valueHandle, key1, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(3));
    JSObject::GetOwnProperty(thread, valueHandle, key2, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(4));
}

HWTEST_F_L0(BuiltinsArrayTest, Splice)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);
    JSHandle<JSTaggedValue> key3(thread, JSTaggedValue(3));
    PropertyDescriptor desc3(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(4)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key3, desc3);
    JSHandle<JSTaggedValue> key4(thread, JSTaggedValue(4));
    PropertyDescriptor desc4(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(5)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key4, desc4);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(2)));
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(100)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::Splice(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 4);

    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    EXPECT_EQ(
        JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle).GetValue()->GetInt(), 2);
    JSObject::GetOwnProperty(thread, valueHandle, key0, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(2));
}

// 22.1.3.6 new Array(1,2,3,4,5).Fill(0,1,3)
HWTEST_F_L0(BuiltinsArrayTest, Fill)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);
    JSHandle<JSTaggedValue> key3(thread, JSTaggedValue(3));
    PropertyDescriptor desc3(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(4)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key3, desc3);
    JSHandle<JSTaggedValue> key4(thread, JSTaggedValue(4));
    PropertyDescriptor desc4(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(5)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key4, desc4);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(3)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::Fill(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    JSObject::GetOwnProperty(thread, valueHandle, key0, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(1));
    JSObject::GetOwnProperty(thread, valueHandle, key1, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(0));
    JSObject::GetOwnProperty(thread, valueHandle, key2, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(0));
    JSObject::GetOwnProperty(thread, valueHandle, key3, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(4));
    JSObject::GetOwnProperty(thread, valueHandle, key4, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(5));
}

HWTEST_F_L0(BuiltinsArrayTest, Find)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(102)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);
    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFindFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result2 = Array::Find(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(result2.GetRawData(), JSTaggedValue(102).GetRawData());
}

HWTEST_F_L0(BuiltinsArrayTest, FindIndex)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);

    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(30)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);

    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFindIndexFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result2 = Array::FindIndex(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(2)).GetRawData());
}

HWTEST_F_L0(BuiltinsArrayTest, ForEach)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);

    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForEachFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result2 = Array::ForEach(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result2.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);
    EXPECT_EQ(jsArray->GetArrayLength(), 3U);
}

// 22.1.3.11 new Array(1,2,3,4,3).IndexOf(searchElement [ , fromIndex ])
HWTEST_F_L0(BuiltinsArrayTest, IndexOf)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);
    JSHandle<JSTaggedValue> key3(thread, JSTaggedValue(3));
    PropertyDescriptor desc3(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(4)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key3, desc3);
    JSHandle<JSTaggedValue> key4(thread, JSTaggedValue(4));
    PropertyDescriptor desc4(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key4, desc4);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::IndexOf(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(2)).GetRawData());

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo2->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(3)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    result = Array::IndexOf(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(4)).GetRawData());

    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo3->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo3->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(5)));
    ecmaRuntimeCallInfo3->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3.get());
    result = Array::IndexOf(ecmaRuntimeCallInfo3.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(-1).GetRawData());

    auto ecmaRuntimeCallInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo4->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo4->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo4->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(3)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo4.get());
    result = Array::IndexOf(ecmaRuntimeCallInfo4.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(2)).GetRawData());
}

// 22.1.3.14 new Array(1,2,3,4,3).LastIndexOf(searchElement [ , fromIndex ])
HWTEST_F_L0(BuiltinsArrayTest, LastIndexOf)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);
    JSHandle<JSTaggedValue> key3(thread, JSTaggedValue(3));
    PropertyDescriptor desc3(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(4)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key3, desc3);
    JSHandle<JSTaggedValue> key4(thread, JSTaggedValue(4));
    PropertyDescriptor desc4(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key4, desc4);

    // new Array(1,2,3,4,3).LastIndexOf(3,4)
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(4)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::LastIndexOf(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(4)).GetRawData());

    // new Array(1,2,3,4,3).LastIndexOf(3,3)
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo2->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(3)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    result = Array::LastIndexOf(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(2)).GetRawData());

    // new Array(1,2,3,4,3).LastIndexOf(5,4)
    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo3->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo3->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(5)));
    ecmaRuntimeCallInfo3->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(4)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3.get());
    result = Array::LastIndexOf(ecmaRuntimeCallInfo3.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(-1).GetRawData());

    // new Array(1,2,3,4,3).LastIndexOf(3)
    auto ecmaRuntimeCallInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo4->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo4->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo4->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(3)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo4.get());
    result = Array::LastIndexOf(ecmaRuntimeCallInfo4.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(4)).GetRawData());
}

// 22.1.3.11 new Array().Pop()
HWTEST_F_L0(BuiltinsArrayTest, Pop)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::Pop(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(obj.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    result = Array::Pop(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(3).GetRawData());
}

// 22.1.3.11 new Array(1,2,3).Push(...items)
HWTEST_F_L0(BuiltinsArrayTest, Push)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(4)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::Push(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetNumber(), 5);

    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 5);
    JSHandle<JSTaggedValue> key3(thread, JSTaggedValue(3));
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), key3).GetValue()->GetInt(), 4);
    JSHandle<JSTaggedValue> key4(thread, JSTaggedValue(4));
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), key4).GetValue()->GetInt(), 5);
}

HWTEST_F_L0(BuiltinsArrayTest, Reduce)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);

    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestReduceFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(10)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::Reduce(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(16).GetRawData());
}

HWTEST_F_L0(BuiltinsArrayTest, ReduceRight)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);

    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestReduceRightFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(10)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::ReduceRight(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(16).GetRawData());
}

HWTEST_F_L0(BuiltinsArrayTest, Shift)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::Shift(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(1).GetRawData());
}

HWTEST_F_L0(BuiltinsArrayTest, Some)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(20)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);

    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestSomeFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result2 = Array::Some(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue::True().GetRawData());
}

HWTEST_F_L0(BuiltinsArrayTest, Sort)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result2 = Array::Sort(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result2.IsECMAObject());
    JSHandle<JSTaggedValue> resultArr =
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<JSTaggedType>(result2.GetRawData())));
    EXPECT_EQ(JSArray::GetProperty(thread, resultArr, key0).GetValue()->GetInt(), 1);
    EXPECT_EQ(JSArray::GetProperty(thread, resultArr, key1).GetValue()->GetInt(), 2);
    EXPECT_EQ(JSArray::GetProperty(thread, resultArr, key2).GetValue()->GetInt(), 3);
}

HWTEST_F_L0(BuiltinsArrayTest, Unshift)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(4)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::Unshift(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(5)).GetRawData());

    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 5);
    JSHandle<JSTaggedValue> key3(thread, JSTaggedValue(0));
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), key3).GetValue()->GetInt(), 4);
    JSHandle<JSTaggedValue> key4(thread, JSTaggedValue(1));
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), key4).GetValue()->GetInt(), 5);
}

HWTEST_F_L0(BuiltinsArrayTest, Join)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, obj, lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)));
    JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(obj), key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)));
    JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(obj), key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(4)));
    JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(obj), key2, desc2);

    JSHandle<EcmaString> str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("2,3,4");
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::Join(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    [[maybe_unused]] auto *res = EcmaString::Cast(resultHandle.GetTaggedValue().GetTaggedObject());

    ASSERT_EQ(res->Compare(*str), 0);
}

HWTEST_F_L0(BuiltinsArrayTest, ToString)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, obj, lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)));
    JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(obj), key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)));
    JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(obj), key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(4)));
    JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(obj), key2, desc2);

    JSHandle<EcmaString> str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("2,3,4");
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::Join(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    [[maybe_unused]] auto *res = EcmaString::Cast(resultHandle.GetTaggedValue().GetTaggedObject());

    ASSERT_EQ(res->Compare(*str), 0);
}

HWTEST_F_L0(BuiltinsArrayTest, Includes)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, obj, lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)));
    JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(obj), key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)));
    JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(obj), key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(4)));
    JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(obj), key2, desc2);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    [[maybe_unused]] JSTaggedValue result = Array::Includes(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.JSTaggedValue::ToBoolean());  // new Int8Array[2,3,4].includes(2)

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    result = Array::Includes(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(!result.JSTaggedValue::ToBoolean());  // new Int8Array[2,3,4].includes(1)

    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo3->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo3->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo3->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3.get());
    result = Array::Includes(ecmaRuntimeCallInfo3.get());
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.JSTaggedValue::ToBoolean());  // new Int8Array[2,3,4].includes(3, 1)

    auto ecmaRuntimeCallInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo4->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo4->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo4->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(2)));
    ecmaRuntimeCallInfo4->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(5)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo4.get());
    result = Array::Includes(ecmaRuntimeCallInfo4.get());
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(!result.JSTaggedValue::ToBoolean());  // new Int8Array[2,3,4].includes(2, 5)

    auto ecmaRuntimeCallInfo5 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo5->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo5->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo5->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(2)));
    ecmaRuntimeCallInfo5->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(-2)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo5.get());
    result = Array::Includes(ecmaRuntimeCallInfo5.get());
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(!result.JSTaggedValue::ToBoolean());  // new Int8Array[2,3,4].includes(2, -2)
}

// es12 23.1.3.10 new Array(1,[2,3]).flat()
HWTEST_F_L0(BuiltinsArrayTest, Flat)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr1 = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr1 != nullptr);
    JSHandle<JSObject> obj1(thread, arr1);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj1), lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj1, key0, desc0);

    JSArray *arr2 = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr2 != nullptr);
    JSHandle<JSObject> obj2(thread, arr2);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj2), lengthKeyHandle).GetValue()->GetInt(), 0);

    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj2, key0, desc1);

    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj2, key1, desc2);

    PropertyDescriptor desc3(thread, JSHandle<JSTaggedValue>(thread, obj2.GetTaggedValue()), true, true, true);
    JSArray::DefineOwnProperty(thread, obj1, key1, desc3);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj1.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::Flat(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    JSObject::GetOwnProperty(thread, valueHandle, key0, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(1));
    JSObject::GetOwnProperty(thread, valueHandle, key1, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(2));
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    JSObject::GetOwnProperty(thread, valueHandle, key2, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(3));
}

// es12 23.1.3.10 new Array(1,50,3]).flatMap(x => [x * 2])
HWTEST_F_L0(BuiltinsArrayTest, FlatMap)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(50)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);
    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFlatMapFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = Array::FlatMap(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());

    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    EXPECT_EQ(
        JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle).GetValue()->GetInt(), 3);
    JSObject::GetOwnProperty(thread, valueHandle, key0, descRes);

    ASSERT_EQ(descRes.GetValue()->GetInt(), 2);
    JSObject::GetOwnProperty(thread, valueHandle, key1, descRes);
    ASSERT_EQ(descRes.GetValue()->GetInt(), 100);
    JSObject::GetOwnProperty(thread, valueHandle, key2, descRes);
    ASSERT_EQ(descRes.GetValue()->GetInt(), 6);
}
}  // namespace panda::test
