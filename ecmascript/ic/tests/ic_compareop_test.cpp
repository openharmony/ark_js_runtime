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

#include <thread>
#include "ecmascript/builtins/builtins_boolean.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/global_env_constants.h"
#include "ecmascript/global_env_constants-inl.h"
#include "ecmascript/ic/ic_compare_op.cpp"
#include "ecmascript/ic/ic_compare_op.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
namespace panda::test {
class IcCompareOPTest : public testing::Test {
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
        TestHelper::CreateEcmaVMWithScope(ecmaVm, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(ecmaVm, scope);
    }

    EcmaVM *ecmaVm {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(IcCompareOPTest, EqualWithIC)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<JSTaggedValue> Str1 = JSHandle<JSTaggedValue>(factory->NewFromASCII("1"));
    JSTaggedValue arg1(static_cast<uint32_t>(1));
    JSTaggedValue arg2(static_cast<double>(1.0));
    JSTaggedValue arg3(false);
    JSTaggedValue arg4(true);
    JSHandle<JSTaggedValue> arg1Handle(thread, arg1);
    JSHandle<JSTaggedValue> arg2Handle(thread, arg2);
    JSHandle<JSTaggedValue> arg3Handle(thread, arg3);
    JSHandle<JSTaggedValue> arg4Handle(thread, arg4);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    JSHandle<JSFunction> boolean(env->GetBooleanFunction());
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*boolean), 6);
    ecmaRuntimeCallInfo->SetFunction(boolean.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    JSTaggedValue booleanObj = builtins::BuiltinsBoolean::BooleanConstructor(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> booleanObjHandle(thread, booleanObj);

    JSTaggedValue resInSlowPath1 = SlowRuntimeStub::EqDyn(thread, arg1Handle.GetTaggedValue(),
                                                          arg2Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath2 = SlowRuntimeStub::EqDyn(thread, Str1.GetTaggedValue(), arg1Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath3 = SlowRuntimeStub::EqDyn(thread, Str1.GetTaggedValue(), arg3Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath4 = SlowRuntimeStub::EqDyn(thread, Str1.GetTaggedValue(), arg4Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath5 = SlowRuntimeStub::EqDyn(thread, booleanObjHandle.GetTaggedValue(),
                                                          arg4Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath9 = SlowRuntimeStub::EqDyn(thread, JSTaggedValue::Undefined(), JSTaggedValue::Null());
    JSTaggedValue resInSlowPath10 = SlowRuntimeStub::EqDyn(thread, JSTaggedValue::Undefined(), JSTaggedValue::True());

    JSTaggedValue resInICPath1 = CompareOp::EqualWithIC(thread, arg1Handle.GetTaggedValue(),
                                                        arg2Handle.GetTaggedValue(), CompareOpType::NUMBER_NUMBER);
    JSTaggedValue resInICPath2 = CompareOp::EqualWithIC(thread, Str1.GetTaggedValue(),
                                                        arg1Handle.GetTaggedValue(), CompareOpType::STRING_NUMBER);
    JSTaggedValue resInICPath3 = CompareOp::EqualWithIC(thread, Str1.GetTaggedValue(),
                                                        arg3Handle.GetTaggedValue(), CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath4 = CompareOp::EqualWithIC(thread, Str1.GetTaggedValue(),
                                                        arg4Handle.GetTaggedValue(), CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath5 = CompareOp::EqualWithIC(thread, booleanObjHandle.GetTaggedValue(),
                                                        arg4Handle.GetTaggedValue(), CompareOpType::OBJ_BOOLEAN);
    JSTaggedValue resInICPath9 = CompareOp::EqualWithIC(thread, JSTaggedValue::Undefined(),
                                                        JSTaggedValue::Null(), CompareOpType::UNDEFINED_NULL);
    JSTaggedValue resInICPath10 = CompareOp::EqualWithIC(thread, JSTaggedValue::Undefined(),
                                                        JSTaggedValue::True(), CompareOpType::OTHER);

    EXPECT_EQ(resInSlowPath1, resInICPath1);
    EXPECT_EQ(resInSlowPath2, resInICPath2);
    EXPECT_EQ(resInSlowPath3, resInICPath3);
    EXPECT_EQ(resInSlowPath4, resInICPath4);
    EXPECT_EQ(resInSlowPath5, resInICPath5);
    EXPECT_EQ(resInSlowPath9, resInICPath9);
    EXPECT_EQ(resInSlowPath10, resInICPath10);
};

HWTEST_F_L0(IcCompareOPTest, NotEqualWithIC)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<JSTaggedValue> Str1 = JSHandle<JSTaggedValue>(factory->NewFromASCII("1"));
    JSTaggedValue arg1(static_cast<uint32_t>(1));
    JSTaggedValue arg2(static_cast<double>(2.0));
    JSTaggedValue arg3(false);
    JSTaggedValue arg4(true);
    JSHandle<JSTaggedValue> arg1Handle(thread, arg1);
    JSHandle<JSTaggedValue> arg2Handle(thread, arg2);
    JSHandle<JSTaggedValue> arg3Handle(thread, arg3);
    JSHandle<JSTaggedValue> arg4Handle(thread, arg4);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    JSHandle<JSFunction> boolean(env->GetBooleanFunction());
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*boolean), 6);
    ecmaRuntimeCallInfo->SetFunction(boolean.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(123)));

    JSTaggedValue booleanObj = builtins::BuiltinsBoolean::BooleanConstructor(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> booleanObjHandle(thread, booleanObj);
    JSTaggedValue resInSlowPath1 = SlowRuntimeStub::NotEqDyn(thread, arg1Handle.GetTaggedValue(),
                                                             arg2Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath2 = SlowRuntimeStub::NotEqDyn(thread, Str1.GetTaggedValue(),
                                                             arg1Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath3 = SlowRuntimeStub::NotEqDyn(thread, Str1.GetTaggedValue(),
                                                             arg3Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath4 = SlowRuntimeStub::NotEqDyn(thread, Str1.GetTaggedValue(),
                                                             arg4Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath5 = SlowRuntimeStub::NotEqDyn(thread, arg1Handle.GetTaggedValue(),
                                                             booleanObjHandle.GetTaggedValue());
    JSTaggedValue resInSlowPath9 = SlowRuntimeStub::NotEqDyn(thread, JSTaggedValue::Undefined(),
                                                             JSTaggedValue::Null());
    JSTaggedValue resInSlowPath10 = SlowRuntimeStub::NotEqDyn(thread, JSTaggedValue::Undefined(),
                                                              JSTaggedValue::True());

    JSTaggedValue resInICPath1 = CompareOp::NotEqualWithIC(thread, arg1Handle.GetTaggedValue(),
                                                           arg2Handle.GetTaggedValue(),
                                                           CompareOpType::NUMBER_NUMBER);
    JSTaggedValue resInICPath2 = CompareOp::NotEqualWithIC(thread, Str1.GetTaggedValue(),
                                                           arg1Handle.GetTaggedValue(), CompareOpType::STRING_NUMBER);
    JSTaggedValue resInICPath3 = CompareOp::NotEqualWithIC(thread, Str1.GetTaggedValue(),
                                                           arg3Handle.GetTaggedValue(), CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath4 = CompareOp::NotEqualWithIC(thread, Str1.GetTaggedValue(),
                                                           arg4Handle.GetTaggedValue(), CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath5 = CompareOp::NotEqualWithIC(thread, arg1Handle.GetTaggedValue(),
                                                           booleanObjHandle.GetTaggedValue(),
                                                           CompareOpType::NUMBER_OBJ);
    JSTaggedValue resInICPath9 = CompareOp::NotEqualWithIC(thread, JSTaggedValue::Undefined(),
                                                           JSTaggedValue::Null(), CompareOpType::UNDEFINED_NULL);
    JSTaggedValue resInICPath10 = CompareOp::NotEqualWithIC(thread, JSTaggedValue::Undefined(),
                                                            JSTaggedValue::True(), CompareOpType::OTHER);

    EXPECT_EQ(resInSlowPath1, resInICPath1);
    EXPECT_EQ(resInSlowPath2, resInICPath2);
    EXPECT_EQ(resInSlowPath3, resInICPath3);
    EXPECT_EQ(resInSlowPath4, resInICPath4);
    EXPECT_EQ(resInSlowPath5, resInICPath5);
    EXPECT_EQ(resInSlowPath9, resInICPath9);
    EXPECT_EQ(resInSlowPath10, resInICPath10);
};


HWTEST_F_L0(IcCompareOPTest, LessDynWithIC)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<JSTaggedValue> Str1 = JSHandle<JSTaggedValue>(factory->NewFromASCII("0"));
    JSTaggedValue arg1(static_cast<uint32_t>(1));
    JSTaggedValue arg2(static_cast<double>(0.5));
    JSTaggedValue arg3(false);
    JSTaggedValue arg4(true);
    JSHandle<JSTaggedValue> arg1Handle(thread, arg1);
    JSHandle<JSTaggedValue> arg2Handle(thread, arg2);
    JSHandle<JSTaggedValue> arg3Handle(thread, arg3);
    JSHandle<JSTaggedValue> arg4Handle(thread, arg4);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    JSHandle<JSFunction> boolean(env->GetBooleanFunction());
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*boolean), 6);
    ecmaRuntimeCallInfo->SetFunction(boolean.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(123)));

    JSTaggedValue booleanObj = builtins::BuiltinsBoolean::BooleanConstructor(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> booleanObjHandle(thread, booleanObj);

    JSTaggedValue resInSlowPath1 = SlowRuntimeStub::LessDyn(thread, arg1Handle.GetTaggedValue(),
                                                            arg2Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath2 = SlowRuntimeStub::LessDyn(thread, Str1.GetTaggedValue(),
                                                            arg1Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath3 = SlowRuntimeStub::LessDyn(thread, Str1.GetTaggedValue(),
                                                            arg3Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath4 = SlowRuntimeStub::LessDyn(thread, Str1.GetTaggedValue(),
                                                            arg4Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath5 = SlowRuntimeStub::LessDyn(thread, arg1Handle.GetTaggedValue(),
                                                            booleanObjHandle.GetTaggedValue());
    JSTaggedValue resInSlowPath9 = SlowRuntimeStub::LessDyn(thread,
        JSTaggedValue::Undefined(), JSTaggedValue::Null());
    JSTaggedValue resInSlowPath10 = SlowRuntimeStub::LessDyn(thread,
        JSTaggedValue::Undefined(), JSTaggedValue::True());

    JSTaggedValue resInICPath1 = CompareOp::LessDynWithIC(thread,  arg1Handle.GetTaggedValue(),
                                                          arg2Handle.GetTaggedValue(), CompareOpType::NUMBER_NUMBER);
    JSTaggedValue resInICPath2 = CompareOp::LessDynWithIC(thread, Str1.GetTaggedValue(),
                                                           arg1Handle.GetTaggedValue(), CompareOpType::STRING_NUMBER);
    JSTaggedValue resInICPath3 = CompareOp::LessDynWithIC(thread, Str1.GetTaggedValue(),
                                                          arg3Handle.GetTaggedValue(), CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath4 = CompareOp::LessDynWithIC(thread, Str1.GetTaggedValue(),
                                                          arg4Handle.GetTaggedValue(), CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath5 = CompareOp::LessDynWithIC(thread,  arg1Handle.GetTaggedValue(),
                                                          booleanObjHandle.GetTaggedValue(),
                                                          CompareOpType::NUMBER_OBJ);
    JSTaggedValue resInICPath9 = CompareOp::LessDynWithIC(thread, JSTaggedValue::Undefined(),
                                                          JSTaggedValue::Null(), CompareOpType::UNDEFINED_NULL);
    JSTaggedValue resInICPath10 = CompareOp::LessDynWithIC(thread, JSTaggedValue::Undefined(),
                                                           JSTaggedValue::True(), CompareOpType::OTHER);

    EXPECT_EQ(resInSlowPath1, resInICPath1);
    EXPECT_EQ(resInSlowPath2, resInICPath2);
    EXPECT_EQ(resInSlowPath3, resInICPath3);
    EXPECT_EQ(resInSlowPath4, resInICPath4);
    EXPECT_EQ(resInSlowPath5, resInICPath5);
    EXPECT_EQ(resInSlowPath9, resInICPath9);
    EXPECT_EQ(resInSlowPath10, resInICPath10);
};


HWTEST_F_L0(IcCompareOPTest, LessEqDynWithIC)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<JSTaggedValue> Str1 = JSHandle<JSTaggedValue>(factory->NewFromASCII("1"));
    JSTaggedValue arg1(static_cast<uint32_t>(1));
    JSTaggedValue arg2(static_cast<double>(0.5));
    JSTaggedValue arg3(false);
    JSTaggedValue arg4(true);
    JSHandle<JSTaggedValue> arg1Handle(thread, arg1);
    JSHandle<JSTaggedValue> arg2Handle(thread, arg2);
    JSHandle<JSTaggedValue> arg3Handle(thread, arg3);
    JSHandle<JSTaggedValue> arg4Handle(thread, arg4);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    JSHandle<JSFunction> boolean(env->GetBooleanFunction());
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*boolean), 6);
    ecmaRuntimeCallInfo->SetFunction(boolean.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(123)));

    JSTaggedValue booleanObj = builtins::BuiltinsBoolean::BooleanConstructor(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> booleanObjHandle(thread, booleanObj);
    JSTaggedValue resInSlowPath1 = SlowRuntimeStub::LessEqDyn(thread, arg1Handle.GetTaggedValue(),
                                                              arg2Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath2 = SlowRuntimeStub::LessEqDyn(thread, Str1.GetTaggedValue(),
                                                              arg1Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath3 = SlowRuntimeStub::LessEqDyn(thread, Str1.GetTaggedValue(),
                                                              arg3Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath4 = SlowRuntimeStub::LessEqDyn(thread, Str1.GetTaggedValue(),
                                                              arg4Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath5 = SlowRuntimeStub::LessEqDyn(thread, arg1Handle.GetTaggedValue(),
                                                              booleanObjHandle.GetTaggedValue());
    JSTaggedValue resInSlowPath9 = SlowRuntimeStub::LessEqDyn(thread, JSTaggedValue::Undefined(),
                                                              JSTaggedValue::Null());
    JSTaggedValue resInSlowPath10 = SlowRuntimeStub::LessEqDyn(thread, JSTaggedValue::Undefined(),
                                                               JSTaggedValue::True());
    JSTaggedValue resInICPath1 = CompareOp::LessEqDynWithIC(thread, arg1Handle.GetTaggedValue(),
                                                            arg2Handle.GetTaggedValue(),
                                                            CompareOpType::NUMBER_NUMBER);
    JSTaggedValue resInICPath2 = CompareOp::LessEqDynWithIC(thread, Str1.GetTaggedValue(),
                                                            arg1Handle.GetTaggedValue(),
                                                            CompareOpType::STRING_NUMBER);
    JSTaggedValue resInICPath3 = CompareOp::LessEqDynWithIC(thread, Str1.GetTaggedValue(),
                                                            arg3Handle.GetTaggedValue(),
                                                            CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath4 = CompareOp::LessEqDynWithIC(thread, Str1.GetTaggedValue(),
                                                            arg4Handle.GetTaggedValue(),
                                                            CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath5 = CompareOp::LessEqDynWithIC(thread,
                                                            arg1Handle.GetTaggedValue(),
                                                            booleanObjHandle.GetTaggedValue(),
                                                            CompareOpType::NUMBER_OBJ);
    JSTaggedValue resInICPath9 = CompareOp::LessEqDynWithIC(thread, JSTaggedValue::Undefined(),
                                                            JSTaggedValue::Null(), CompareOpType::UNDEFINED_NULL);
    JSTaggedValue resInICPath10 = CompareOp::LessEqDynWithIC(thread, JSTaggedValue::Undefined(),
                                                            JSTaggedValue::True(), CompareOpType::OTHER);

    EXPECT_EQ(resInSlowPath1, resInICPath1);
    EXPECT_EQ(resInSlowPath2, resInICPath2);
    EXPECT_EQ(resInSlowPath3, resInICPath3);
    EXPECT_EQ(resInSlowPath4, resInICPath4);
    EXPECT_EQ(resInSlowPath5, resInICPath5);
    EXPECT_EQ(resInSlowPath9, resInICPath9);
    EXPECT_EQ(resInSlowPath10, resInICPath10);
};


HWTEST_F_L0(IcCompareOPTest, GreaterDynWithIC)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<JSTaggedValue> Str1 = JSHandle<JSTaggedValue>(factory->NewFromASCII("1"));
    JSTaggedValue arg1(static_cast<uint32_t>(1));
    JSTaggedValue arg2(static_cast<double>(1.0));
    JSTaggedValue arg3(false);
    JSTaggedValue arg4(true);
    JSHandle<JSTaggedValue> arg1Handle(thread, arg1);
    JSHandle<JSTaggedValue> arg2Handle(thread, arg2);
    JSHandle<JSTaggedValue> arg3Handle(thread, arg3);
    JSHandle<JSTaggedValue> arg4Handle(thread, arg4);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    JSHandle<JSFunction> boolean(env->GetBooleanFunction());
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*boolean), 6);
    ecmaRuntimeCallInfo->SetFunction(boolean.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    JSTaggedValue booleanObj = builtins::BuiltinsBoolean::BooleanConstructor(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> booleanObjHandle(thread, booleanObj);
    JSTaggedValue resInSlowPath1 = SlowRuntimeStub::GreaterDyn(thread, arg1Handle.GetTaggedValue(),
                                                               arg2Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath2 = SlowRuntimeStub::GreaterDyn(thread, Str1.GetTaggedValue(),
                                                               arg1Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath3 = SlowRuntimeStub::GreaterDyn(thread, Str1.GetTaggedValue(),
                                                               arg3Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath4 = SlowRuntimeStub::GreaterDyn(thread, Str1.GetTaggedValue(),
                                                               arg4Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath5 = SlowRuntimeStub::GreaterDyn(thread, arg1Handle.GetTaggedValue(),
                                                               booleanObjHandle.GetTaggedValue());
    JSTaggedValue resInSlowPath9 = SlowRuntimeStub::GreaterDyn(thread, JSTaggedValue::Undefined(),
                                                               JSTaggedValue::Null());
    JSTaggedValue resInSlowPath10 = SlowRuntimeStub::GreaterDyn(thread, JSTaggedValue::Undefined(),
                                                                JSTaggedValue::True());

    JSTaggedValue resInICPath1 = CompareOp::GreaterDynWithIC(thread, arg1Handle.GetTaggedValue(),
                                                             arg2Handle.GetTaggedValue(),
                                                             CompareOpType::NUMBER_NUMBER);
    JSTaggedValue resInICPath2 = CompareOp::GreaterDynWithIC(thread, Str1.GetTaggedValue(),
                                                             arg1Handle.GetTaggedValue(),
                                                             CompareOpType::STRING_NUMBER);
    JSTaggedValue resInICPath3 = CompareOp::GreaterDynWithIC(thread, Str1.GetTaggedValue(),
                                                             arg3Handle.GetTaggedValue(),
                                                             CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath4 = CompareOp::GreaterDynWithIC(thread, Str1.GetTaggedValue(),
                                                             arg4Handle.GetTaggedValue(),
                                                             CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath5 = CompareOp::GreaterDynWithIC(thread, arg1Handle.GetTaggedValue(),
                                                             booleanObjHandle.GetTaggedValue(),
                                                             CompareOpType::NUMBER_OBJ);
    JSTaggedValue resInICPath9 = CompareOp::GreaterDynWithIC(thread, JSTaggedValue::Undefined(),
                                                             JSTaggedValue::Null(), CompareOpType::UNDEFINED_NULL);
    JSTaggedValue resInICPath10 = CompareOp::GreaterDynWithIC(thread, JSTaggedValue::Undefined(),
                                                              JSTaggedValue::True(), CompareOpType::OTHER);

    EXPECT_EQ(resInSlowPath1, resInICPath1);
    EXPECT_EQ(resInSlowPath2, resInICPath2);
    EXPECT_EQ(resInSlowPath3, resInICPath3);
    EXPECT_EQ(resInSlowPath4, resInICPath4);
    EXPECT_EQ(resInSlowPath5, resInICPath5);
    EXPECT_EQ(resInSlowPath9, resInICPath9);
    EXPECT_EQ(resInSlowPath10, resInICPath10);
};


HWTEST_F_L0(IcCompareOPTest, GreaterEqDynWithIC)
{
    ObjectFactory *factory = ecmaVm->GetFactory();

    JSHandle<JSTaggedValue> Str1 = JSHandle<JSTaggedValue>(factory->NewFromASCII("1"));
    JSTaggedValue arg1(static_cast<uint32_t>(1));
    JSTaggedValue arg2(static_cast<double>(1.0));
    JSTaggedValue arg3(false);
    JSTaggedValue arg4(true);
    JSHandle<JSTaggedValue> arg1Handle(thread, arg1);
    JSHandle<JSTaggedValue> arg2Handle(thread, arg2);
    JSHandle<JSTaggedValue> arg3Handle(thread, arg3);
    JSHandle<JSTaggedValue> arg4Handle(thread, arg4);

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    JSHandle<JSFunction> boolean(env->GetBooleanFunction());
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*boolean), 6);
    ecmaRuntimeCallInfo->SetFunction(boolean.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(0)));

    JSTaggedValue booleanObj = builtins::BuiltinsBoolean::BooleanConstructor(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> booleanObjHandle(thread, booleanObj);
    JSTaggedValue resInSlowPath1 = SlowRuntimeStub::GreaterEqDyn(thread, arg1Handle.GetTaggedValue(),
                                                                 arg2Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath2 = SlowRuntimeStub::GreaterEqDyn(thread, Str1.GetTaggedValue(),
                                                                 arg1Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath3 = SlowRuntimeStub::GreaterEqDyn(thread, Str1.GetTaggedValue(),
                                                                 arg3Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath4 = SlowRuntimeStub::GreaterEqDyn(thread, Str1.GetTaggedValue(),
                                                                 arg4Handle.GetTaggedValue());
    JSTaggedValue resInSlowPath5 = SlowRuntimeStub::GreaterEqDyn(thread, arg1Handle.GetTaggedValue(),
                                                                 booleanObjHandle.GetTaggedValue());
    JSTaggedValue resInSlowPath9 = SlowRuntimeStub::GreaterEqDyn(thread, JSTaggedValue::Undefined(),
                                                                 JSTaggedValue::Null());
    JSTaggedValue resInSlowPath10 = SlowRuntimeStub::GreaterEqDyn(thread, JSTaggedValue::Undefined(),
                                                                  JSTaggedValue::True());

    JSTaggedValue resInICPath1 = CompareOp::GreaterEqDynWithIC(thread, arg1Handle.GetTaggedValue(),
                                                               arg2Handle.GetTaggedValue(),
                                                               CompareOpType::NUMBER_NUMBER);
    JSTaggedValue resInICPath2 = CompareOp::GreaterEqDynWithIC(thread, Str1.GetTaggedValue(),
                                                               arg1Handle.GetTaggedValue(),
                                                               CompareOpType::STRING_NUMBER);
    JSTaggedValue resInICPath3 = CompareOp::GreaterEqDynWithIC(thread, Str1.GetTaggedValue(),
                                                               arg3Handle.GetTaggedValue(),
                                                               CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath4 = CompareOp::GreaterEqDynWithIC(thread, Str1.GetTaggedValue(),
                                                               arg4Handle.GetTaggedValue(),
                                                               CompareOpType::STRING_BOOLEAN);
    JSTaggedValue resInICPath5 = CompareOp::GreaterEqDynWithIC(thread, arg1Handle.GetTaggedValue(),
                                                               booleanObjHandle.GetTaggedValue(),
                                                               CompareOpType::NUMBER_OBJ);
    JSTaggedValue resInICPath9 = CompareOp::GreaterEqDynWithIC(thread, JSTaggedValue::Undefined(),
                                                               JSTaggedValue::Null(), CompareOpType::UNDEFINED_NULL);
    JSTaggedValue resInICPath10 = CompareOp::GreaterEqDynWithIC(thread, JSTaggedValue::Undefined(),
                                                                JSTaggedValue::True(), CompareOpType::OTHER);

    EXPECT_EQ(resInSlowPath1, resInICPath1);
    EXPECT_EQ(resInSlowPath2, resInICPath2);
    EXPECT_EQ(resInSlowPath3, resInICPath3);
    EXPECT_EQ(resInSlowPath4, resInICPath4);
    EXPECT_EQ(resInSlowPath5, resInICPath5);
    EXPECT_EQ(resInSlowPath9, resInICPath9);
    EXPECT_EQ(resInSlowPath10, resInICPath10);
};
}  // namespace panda::test
