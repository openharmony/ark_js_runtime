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

#include "ecmascript/builtins/builtins_function.h"
#include "ecmascript/builtins/builtins_boolean.h"

#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_object-inl.h"

#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
using BuiltinsBase = panda::ecmascript::base::BuiltinsBase;
using JSArray = panda::ecmascript::JSArray;

namespace panda::test {
class BuiltinsFunctionTest : public testing::Test {
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

// native function for test apply and call
JSTaggedValue TestFunctionApplyAndCall(EcmaRuntimeCallInfo *argv)
{
    JSThread *thread = argv->GetThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    int result = 0;
    for (uint32_t index = 0; index < argv->GetArgsNumber(); ++index) {
        result += BuiltinsBase::GetCallArg(argv, index)->GetInt();
    }
    JSHandle<JSTaggedValue> thisValue(BuiltinsBase::GetThis(argv));

    JSTaggedValue testA =
        JSObject::GetProperty(thread, thisValue,
                              JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_a")))
            .GetValue()
            .GetTaggedValue();
    JSTaggedValue testB =
        JSObject::GetProperty(thread, thisValue,
                              JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_b")))
            .GetValue()
            .GetTaggedValue();

    result = result + testA.GetInt() + testB.GetInt();
    return BuiltinsBase::GetTaggedInt(result);
}

// func.apply(thisArg)
HWTEST_F_L0(BuiltinsFunctionTest, FunctionPrototypeApply)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // ecma 19.2.3.1: func
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestFunctionApplyAndCall));

    // ecma 19.2.3.1: thisArg
    JSHandle<JSObject> thisArg(thread, env->GetGlobalObject());
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(thisArg),
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_a")),
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(thisArg),
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_b")),
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(func.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, (thisArg.GetTaggedValue()));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsFunction::FunctionPrototypeApply(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(3).GetRawData());

    JSObject::DeleteProperty(thread, (thisArg),
                             JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_a")));
    JSObject::DeleteProperty(thread, (thisArg),
                             JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_b")));
}

// func.apply(thisArg, argArray)
HWTEST_F_L0(BuiltinsFunctionTest, FunctionPrototypeApply1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // ecma 19.2.3.1: func
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestFunctionApplyAndCall));

    // ecma 19.2.3.1: thisArg
    JSHandle<JSObject> thisArg(thread, env->GetGlobalObject());
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(thisArg),
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_a")),
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(10)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(thisArg),
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_b")),
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(20)));

    // ecma 19.2.3.1: argArray
    JSHandle<JSObject> array(JSArray::ArrayCreate(thread, JSTaggedNumber(2)));
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(30)));
    JSArray::DefineOwnProperty(thread, array, JSHandle<JSTaggedValue>(thread, JSTaggedValue(0)), desc);

    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(40)));
    JSArray::DefineOwnProperty(thread, array, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), desc1);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(func.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, (thisArg.GetTaggedValue()));
    ecmaRuntimeCallInfo->SetCallArg(1, array.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsFunction::FunctionPrototypeApply(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(100).GetRawData());

    JSObject::DeleteProperty(thread, (thisArg),
                             JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_a")));
    JSObject::DeleteProperty(thread, (thisArg),
                             JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_b")));
}

// target.bind(thisArg)
HWTEST_F_L0(BuiltinsFunctionTest, FunctionPrototypeBind)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSFunction> target = factory->NewJSFunction(env);
    JSFunction::SetFunctionName(thread, JSHandle<JSFunctionBase>(target),
                                JSHandle<JSTaggedValue>(factory->NewFromASCII("target")),
                                JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    JSFunction::SetFunctionLength(thread, target, JSTaggedValue(2));

    JSHandle<JSObject> thisArg(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(target.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, (thisArg.GetTaggedValue()));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsFunction::FunctionPrototypeBind(ecmaRuntimeCallInfo.get());

    ASSERT_TRUE(result.IsECMAObject());

    JSHandle<JSBoundFunction> resultFunc(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    // test BoundTarget
    ASSERT_EQ(resultFunc->GetBoundTarget(), target.GetTaggedValue());
    // test BoundThis
    ASSERT_EQ(resultFunc->GetBoundThis(), thisArg.GetTaggedValue());
    // test BoundArguments
    JSHandle<TaggedArray> array(thread, resultFunc->GetBoundArguments());
    ASSERT_EQ(array->GetLength(), 0U);
    // test name property
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> nameKey = globalConst->GetHandledNameString();
    JSHandle<JSTaggedValue> resultFuncHandle(thread, *resultFunc);
    JSHandle<EcmaString> resultName(JSObject::GetProperty(thread, resultFuncHandle, nameKey).GetValue());
    JSHandle<EcmaString> boundTarget = factory->NewFromASCII("bound target");
    ASSERT_EQ(resultName->Compare(*boundTarget), 0);
    // test length property
    JSHandle<JSTaggedValue> lengthKey = globalConst->GetHandledLengthString();
    JSHandle<JSTaggedValue> resultLength(JSObject::GetProperty(thread, resultFuncHandle, lengthKey).GetValue());
    ASSERT_EQ(JSTaggedValue::ToNumber(thread, resultLength).GetNumber(), 2.0);
}

// target.bind(thisArg, 123, "helloworld")
HWTEST_F_L0(BuiltinsFunctionTest, FunctionPrototypeBind1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSFunction> target = factory->NewJSFunction(env);
    JSFunction::SetFunctionName(thread, JSHandle<JSFunctionBase>(target),
                                JSHandle<JSTaggedValue>(factory->NewFromASCII("target1")),
                                JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
    JSFunction::SetFunctionLength(thread, target, JSTaggedValue(5));

    JSHandle<JSObject> thisArg(thread, env->GetGlobalObject());
    JSHandle<EcmaString> str = factory->NewFromASCII("helloworld");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(target.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, thisArg.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(123)));
    ecmaRuntimeCallInfo->SetCallArg(2, str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsFunction::FunctionPrototypeBind(ecmaRuntimeCallInfo.get());

    ASSERT_TRUE(result.IsECMAObject());

    JSHandle<JSBoundFunction> resultFunc(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    // test BoundTarget
    ASSERT_EQ(resultFunc->GetBoundTarget(), target.GetTaggedValue());
    // test BoundThis
    ASSERT_EQ(resultFunc->GetBoundThis(), thisArg.GetTaggedValue());
    // test BoundArguments
    JSHandle<TaggedArray> array(thread, resultFunc->GetBoundArguments());
    ASSERT_EQ(array->GetLength(), 2U);
    JSTaggedValue elem = array->Get(0);
    JSTaggedValue elem1 = array->Get(1);
    ASSERT_EQ(elem.GetRawData(), JSTaggedValue(123).GetRawData());

    ASSERT_EQ(elem1.GetRawData(), str.GetTaggedType());
    ASSERT_TRUE(elem1.IsString());
    // test name property
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> nameKey = globalConst->GetHandledNameString();
    JSHandle<JSTaggedValue> resultFuncHandle(thread, *resultFunc);
    JSHandle<EcmaString> resultName(JSObject::GetProperty(thread, resultFuncHandle, nameKey).GetValue());
    JSHandle<EcmaString> rulerName = factory->NewFromASCII("bound target1");
    ASSERT_EQ(resultName->Compare(*rulerName), 0);
    // test length property
    JSHandle<JSTaggedValue> lengthKey = globalConst->GetHandledLengthString();
    JSHandle<JSTaggedValue> resultLength(JSObject::GetProperty(thread, resultFuncHandle, lengthKey).GetValue());
    // target.length is 5, (...args) length is 2
    ASSERT_EQ(JSTaggedValue::ToNumber(thread, resultLength).GetNumber(), 3.0);
}

// target.bind(thisArg, 123, "helloworld") set target_name = EmptyString()
HWTEST_F_L0(BuiltinsFunctionTest, FunctionPrototypeBind2)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSFunction> target = factory->NewJSFunction(env);
    PropertyDescriptor nameDesc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(123)), false, false, true);
    JSTaggedValue::DefinePropertyOrThrow(thread, JSHandle<JSTaggedValue>(target),
                                         thread->GlobalConstants()->GetHandledNameString(), nameDesc);
    JSFunction::SetFunctionLength(thread, target, JSTaggedValue(5));

    JSHandle<JSObject> thisArg(thread, env->GetGlobalObject());
    JSHandle<EcmaString> str = factory->NewFromASCII("helloworld");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(target.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, (thisArg.GetTaggedValue()));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(123)));
    ecmaRuntimeCallInfo->SetCallArg(2, str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsFunction::FunctionPrototypeBind(ecmaRuntimeCallInfo.get());

    ASSERT_TRUE(result.IsECMAObject());

    JSHandle<JSBoundFunction> resultFunc(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    // test BoundTarget
    ASSERT_EQ(resultFunc->GetBoundTarget(), target.GetTaggedValue());
    // test BoundThis
    ASSERT_EQ(resultFunc->GetBoundThis(), thisArg.GetTaggedValue());
    // test BoundArguments
    JSHandle<TaggedArray> array(thread, resultFunc->GetBoundArguments());
    ASSERT_EQ(array->GetLength(), 2U);
    JSTaggedValue elem = array->Get(0);
    JSTaggedValue elem1 = array->Get(1);
    ASSERT_EQ(elem.GetRawData(), JSTaggedValue(123).GetRawData());

    ASSERT_EQ(elem1.GetRawData(), str.GetTaggedType());
    ASSERT_TRUE(elem1.IsString());
    // test name property
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> nameKey = globalConst->GetHandledNameString();
    JSHandle<JSTaggedValue> resultFuncHandle(resultFunc);
    JSHandle<EcmaString> resultName(JSObject::GetProperty(thread, resultFuncHandle, nameKey).GetValue());
    JSHandle<EcmaString> rulerName = factory->NewFromASCII("bound ");
    ASSERT_EQ(resultName->Compare(*rulerName), 0);
    // test length property
    JSHandle<JSTaggedValue> lengthKey = globalConst->GetHandledLengthString();
    JSHandle<JSTaggedValue> resultLength(JSObject::GetProperty(thread, resultFuncHandle, lengthKey).GetValue());
    // target.length is 5, (...args) length is 2
    ASSERT_EQ(JSTaggedValue::ToNumber(thread, resultLength).GetNumber(), 3.0);
}

// func.call(thisArg)
HWTEST_F_L0(BuiltinsFunctionTest, FunctionPrototypeCall)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // ecma 19.2.3.3: func
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestFunctionApplyAndCall));

    // ecma 19.2.3.3: thisArg
    JSHandle<JSObject> thisArg(thread, env->GetGlobalObject());
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(thisArg),
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_a")),
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(thisArg),
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_b")),
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(func.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, (thisArg.GetTaggedValue()));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsFunction::FunctionPrototypeCall(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(3).GetRawData());

    JSObject::DeleteProperty(thread, (thisArg),
                             JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_a")));
    JSObject::DeleteProperty(thread, (thisArg),
                             JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_b")));
}

// func.call(thisArg, 123, 456, 789)
HWTEST_F_L0(BuiltinsFunctionTest, FunctionPrototypeCall1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // ecma 19.2.3.3: func
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestFunctionApplyAndCall));

    // ecma 19.2.3.3: thisArg
    JSHandle<JSObject> thisArg(thread, env->GetGlobalObject());
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(thisArg),
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_a")),
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(thisArg),
                          JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_b")),
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)));

    // func thisArg ...args
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 12);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(func.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, (thisArg.GetTaggedValue()));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(123)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(456)));
    ecmaRuntimeCallInfo->SetCallArg(3, JSTaggedValue(static_cast<int32_t>(789)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsFunction::FunctionPrototypeCall(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(1371).GetRawData());

    JSObject::DeleteProperty(thread, (thisArg),
                             JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_a")));
    JSObject::DeleteProperty(thread, (thisArg),
                             JSHandle<JSTaggedValue>(factory->NewFromASCII("test_builtins_function_b")));
}

HWTEST_F_L0(BuiltinsFunctionTest, FunctionPrototypeHasInstance)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSFunction> booleanCtor(env->GetBooleanFunction());

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*booleanCtor), 6);
    ecmaRuntimeCallInfo1->SetFunction(booleanCtor.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(123)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1.get());
    JSTaggedValue result = BuiltinsBoolean::BooleanConstructor(ecmaRuntimeCallInfo1.get());
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSObject> booleanInstance(thread, result);

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(booleanCtor.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, booleanInstance.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2.get());
    EXPECT_TRUE(BuiltinsFunction::FunctionPrototypeHasInstance(ecmaRuntimeCallInfo2.get()).GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}
}  // namespace panda::test
