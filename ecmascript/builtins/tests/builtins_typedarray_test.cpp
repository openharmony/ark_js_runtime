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

#include "ecmascript/base/typed_array_helper-inl.h"
#include "ecmascript/base/typed_array_helper.h"

#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/builtins/builtins_object.h"
#include "ecmascript/builtins/builtins_typedarray.h"
#include "ecmascript/ecma_runtime_call_info.h"
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
#include "ecmascript/js_typed_array.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/object_operator.h"

#include "ecmascript/tests/test_helper.h"
#include "utils/bit_utils.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
using namespace panda::ecmascript::base;

namespace panda::test {
using Array = ecmascript::builtins::BuiltinsArray;
using TypedArray = ecmascript::builtins::BuiltinsTypedArray;
using TypedArrayHelper = ecmascript::base::TypedArrayHelper;

class BuiltinsTypedArrayTest : public testing::Test {
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

protected:
    PandaVM *instance {nullptr};
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
            int length = jsArray->GetArrayLength() + 1;
            jsArray->SetArrayLength(argv->GetThread(), length);
            return JSTaggedValue::Undefined();
        }

        static JSTaggedValue TestEveryFunc(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                [[maybe_unused]] int aaa = GetCallArg(argv, 0)->GetInt();
                //  10 : test case
                if (GetCallArg(argv, 0)->GetInt() > 10) {
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }

        static JSTaggedValue TestFilterFunc(EcmaRuntimeCallInfo *argv)
        {
            ASSERT(argv);
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                // 10 : test case
                if (GetCallArg(argv, 0)->GetInt() > 10) {
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
                //  10 : test case
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
                //  10 : test case
                if (GetCallArg(argv, 0)->GetInt() > 10) {
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }
    };
};

JSTaggedValue CreateBuiltinsTypeArrayJSObject(JSThread *thread, const CString keyCStr)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> dynclass = env->GetObjectFunction();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> obj(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dynclass), dynclass));
    JSHandle<JSTaggedValue> key(factory->NewFromASCII(&keyCStr[0]));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSObject::SetProperty(thread, obj, key, value);
    return obj.GetTaggedValue();
}

JSTypedArray *CreateTypedArrayFromList(JSThread *thread, const JSHandle<TaggedArray> &array)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> int8_array(env->GetInt8ArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    //  6 : test case
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetNewTarget(JSTaggedValue(*int8_array));
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
    ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = TypedArray::Int8ArrayConstructor(ecmaRuntimeCallInfo1.get());

    EXPECT_TRUE(result.IsECMAObject());
    JSTypedArray *int8arr = JSTypedArray::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData()));
    return int8arr;
}


HWTEST_F_L0(BuiltinsTypedArrayTest, Species)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSFunction> array(env->GetArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(globalObject.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = TypedArray::Species(ecmaRuntimeCallInfo1.get());
    ASSERT_TRUE(result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsTypedArrayTest, Includes)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(2));
    array->Set(thread, 1, JSTaggedValue(3));
    array->Set(thread, 2, JSTaggedValue(4));

    [[maybe_unused]] JSHandle<JSTaggedValue> obj =
        JSHandle<JSTaggedValue>(thread, CreateTypedArrayFromList(thread, array));
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    [[maybe_unused]] JSTaggedValue result = TypedArray::Includes(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.JSTaggedValue::ToBoolean()); // new Int8Array[2,3,4].includes(2)

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    result = TypedArray::Includes(ecmaRuntimeCallInfo2.get());
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(!result.JSTaggedValue::ToBoolean()); // new Int8Array[2,3,4].includes(1)

    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo3->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo3->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo3->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3.get());
    result = TypedArray::Includes(ecmaRuntimeCallInfo3.get());
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.JSTaggedValue::ToBoolean()); // new Int8Array[2,3,4].includes(3, 1)

    auto ecmaRuntimeCallInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo4->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo4->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo4->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(2)));
    ecmaRuntimeCallInfo4->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(5)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo4.get());
    result = Array::Includes(ecmaRuntimeCallInfo4.get());
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(!result.JSTaggedValue::ToBoolean()); // new Int8Array[2,3,4].includes(2, 5)

    auto ecmaRuntimeCallInfo5 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo5->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo5->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo5->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(2)));
    ecmaRuntimeCallInfo5->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(-2)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo5.get());
    result = Array::Includes(ecmaRuntimeCallInfo5.get());
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(!result.JSTaggedValue::ToBoolean()); // new Int8Array[2,3,4].includes(2, -2)
}
}  // namespace panda::test
