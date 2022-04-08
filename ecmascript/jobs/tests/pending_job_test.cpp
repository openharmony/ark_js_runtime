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

#include "ecmascript/jobs/pending_job.h"
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
using PendingJob = ecmascript::job::PendingJob;
class PendingJobTest : public testing::Test {
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

/**
 * @tc.name: GetJob
 * @tc.desc: Check whether the result returned through "GetJob" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(PendingJobTest, GetJob)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<TaggedArray> handleArgv = factory->NewTaggedArray(0);
    JSHandle<JSFunction> handleFunc = factory->NewJSFunction(env);

    JSHandle<JSTaggedValue> handlePendingJobVal(factory->NewPendingJob(handleFunc, handleArgv));
    EXPECT_TRUE(handlePendingJobVal->IsPendingJob());
    JSHandle<PendingJob> handlePendingJob(handlePendingJobVal);
    EXPECT_TRUE(handlePendingJob->GetJob().IsJSFunction());

    JSHandle<JSFunction> handleNativeFunc(env->GetTypedArrayFunction());
    handlePendingJob->SetJob(thread, handleNativeFunc.GetTaggedValue());
    EXPECT_EQ(JSTaggedValue::SameValue(handlePendingJob->GetJob(), handleNativeFunc.GetTaggedValue()), true);
}

/**
 * @tc.name: GetArguments
 * @tc.desc: Check whether the result returned through "GetArguments" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(PendingJobTest, GetArguments)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<TaggedArray> handleArgv1 = factory->NewTaggedArray(0);
    JSHandle<JSFunction> handleFunc = factory->NewJSFunction(env);

    JSHandle<JSTaggedValue> handlePendingJobVal(factory->NewPendingJob(handleFunc, handleArgv1));
    EXPECT_TRUE(handlePendingJobVal->IsPendingJob());
    JSHandle<PendingJob> handlePendingJob(handlePendingJobVal);
    EXPECT_TRUE(handlePendingJob->GetArguments().IsTaggedArray());

    JSHandle<TaggedArray> handleArgv2 = factory->NewTaggedArray(1);
    handleArgv2->Set(thread, 0, JSTaggedValue(1));
    handlePendingJob->SetArguments(thread, handleArgv2.GetTaggedValue());

    JSHandle<TaggedArray> resultArray(thread, handlePendingJob->GetArguments());
    EXPECT_EQ(resultArray->GetLength(), 1U);
    EXPECT_EQ(resultArray->Get(0).GetInt(), 1);
}

/**
 * @tc.name: ExecutePendingJob_001
 * @tc.desc: Get a function called PromiseReactionJob from env.According to the definition of function,define a
 *           TaggedArray object with length of two.set the required value and define a pendingjob object according
 *           to both.the pendingjob object call "ExecutePendingJob" function to execute the method of function and
 *           return the value of the method.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(PendingJobTest, ExecutePendingJob_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> promiseFunc = env->GetPromiseFunction();
    
    JSHandle<PromiseCapability> capbility = JSPromise::NewPromiseCapability(thread, promiseFunc);
    JSHandle<JSTaggedValue> reject(thread, capbility->GetReject());

    JSHandle<PromiseReaction> rejectReaction = factory->NewPromiseReaction();
    rejectReaction->SetPromiseCapability(thread, capbility.GetTaggedValue());
    rejectReaction->SetHandler(thread, reject.GetTaggedValue());

    JSHandle<JSFunction> promiseReactionsJob(env->GetPromiseReactionJob());
    JSHandle<TaggedArray> handleArgv = factory->NewTaggedArray(2);
    handleArgv->Set(thread, 0, rejectReaction.GetTaggedValue());
    handleArgv->Set(thread, 1, JSTaggedValue(44));

    JSHandle<PendingJob> handlePendingJob = factory->NewPendingJob(promiseReactionsJob, handleArgv);
    JSTaggedValue callResult = PendingJob::ExecutePendingJob(handlePendingJob, thread);
    EXPECT_EQ(callResult, JSTaggedValue::Undefined());
    JSHandle<JSPromise> jsPromise(thread, capbility->GetPromise());
    EXPECT_EQ(jsPromise->GetPromiseState(), PromiseState::REJECTED);
    EXPECT_EQ(JSTaggedValue::SameValue(jsPromise->GetPromiseResult(), JSTaggedValue(44)), true);
}

/**
 * @tc.name: ExecutePendingJob_002
 * @tc.desc: Get a function called PromiseReactionJob from env.According to the definition of function,define a
 *           TaggedArray object with length of two.set the required value and define a pendingjob object according
 *           to both.the pendingjob object call "ExecutePendingJob" function to execute the method of function and
 *           return the value of the method.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(PendingJobTest, ExecutePendingJob_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> promiseFunc = env->GetPromiseFunction();
    JSHandle<JSTaggedValue> paramMsg(factory->NewFromCanBeCompressString("resolve"));
    
    JSHandle<PromiseCapability> capbility = JSPromise::NewPromiseCapability(thread, promiseFunc);
    JSHandle<JSTaggedValue> resolve(thread, capbility->GetResolve());

    JSHandle<PromiseReaction> fulfillReaction = factory->NewPromiseReaction();
    fulfillReaction->SetPromiseCapability(thread, capbility.GetTaggedValue());
    fulfillReaction->SetHandler(thread, resolve.GetTaggedValue());

    JSHandle<JSFunction> promiseReactionsJob(env->GetPromiseReactionJob());
    JSHandle<TaggedArray> handleArgv = factory->NewTaggedArray(2);
    handleArgv->Set(thread, 0, fulfillReaction.GetTaggedValue());
    handleArgv->Set(thread, 1, paramMsg.GetTaggedValue());

    JSHandle<PendingJob> handlePendingJob = factory->NewPendingJob(promiseReactionsJob, handleArgv);
    JSTaggedValue callResult = PendingJob::ExecutePendingJob(handlePendingJob, thread);
    EXPECT_EQ(callResult, JSTaggedValue::Undefined());
    JSHandle<JSPromise> jsPromise(thread, capbility->GetPromise());
    EXPECT_EQ(jsPromise->GetPromiseState(), PromiseState::FULFILLED);
    EXPECT_EQ(JSTaggedValue::SameValue(jsPromise->GetPromiseResult(), paramMsg.GetTaggedValue()), true);
}

/**
 * @tc.name: ExecutePendingJob_003
 * @tc.desc: Get a function called PromiseReactionJob from env.According to the definition of function,define a
 *           TaggedArray object with length of two.set the required value and define a pendingjob object according
 *           to both.the pendingjob object call "ExecutePendingJob" function to execute the method of function and
 *           return the value of the method.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(PendingJobTest, ExecutePendingJob_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> promiseFunc = env->GetPromiseFunction();
    JSHandle<JSTaggedValue> paramMsg(factory->NewFromCanBeCompressString("Thrower"));
    
    JSHandle<PromiseCapability> capbility = JSPromise::NewPromiseCapability(thread, promiseFunc);
    JSHandle<PromiseReaction> rejectReaction = factory->NewPromiseReaction();
    rejectReaction->SetPromiseCapability(thread, capbility.GetTaggedValue());
    rejectReaction->SetHandler(thread, paramMsg.GetTaggedValue());

    JSHandle<JSFunction> promiseReactionsJob(env->GetPromiseReactionJob());
    JSHandle<TaggedArray> handleArgv = factory->NewTaggedArray(2);
    handleArgv->Set(thread, 0, rejectReaction.GetTaggedValue());
    handleArgv->Set(thread, 1, JSTaggedValue::Undefined());

    JSHandle<PendingJob> handlePendingJob = factory->NewPendingJob(promiseReactionsJob, handleArgv);
    JSTaggedValue callResult = PendingJob::ExecutePendingJob(handlePendingJob, thread);
    EXPECT_EQ(callResult, JSTaggedValue::Undefined());
    JSHandle<JSPromise> jsPromise(thread, capbility->GetPromise());
    EXPECT_EQ(jsPromise->GetPromiseState(), PromiseState::REJECTED);
    EXPECT_EQ(JSTaggedValue::SameValue(jsPromise->GetPromiseResult(), JSTaggedValue::Undefined()), true);
}

JSTaggedValue TestPromiseOnResolved(EcmaRuntimeCallInfo *argv)
{
    auto factory = argv->GetThread()->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> result = BuiltinsBase::GetCallArg(argv, 0);
    EXPECT_TRUE(result->IsPromiseReaction());
    auto handlerMsg = factory->NewFromCanBeCompressString("after_resolve");
    JSHandle<PromiseReaction> reaction = JSHandle<PromiseReaction>::Cast(result);
    JSHandle<JSTaggedValue> handler(argv->GetThread(), reaction->GetHandler());
    EXPECT_EQ(JSTaggedValue::SameValue(handler.GetTaggedValue(), handlerMsg.GetTaggedValue()), true);
    return JSTaggedValue::Undefined();
}

/**
 * @tc.name: ExecutePendingJob_004
 * @tc.desc: Create a function called TestPromiseOnResolved.According to the definition of function,define a TaggedArray
 *           object with length of two.set the required value and define a pendingjob object according to both.
 *           the pendingjob object call "ExecutePendingJob" function to execute the method of function and return the
 *           value of the method.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(PendingJobTest, ExecutePendingJob_004)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> promiseFunc = env->GetPromiseFunction();
    JSHandle<JSTaggedValue> paramMsg(factory->NewFromCanBeCompressString("after_resolve"));
    
    JSHandle<PromiseCapability> capbility = JSPromise::NewPromiseCapability(thread, promiseFunc);
    JSHandle<JSTaggedValue> resolve(thread, capbility->GetResolve());

    JSHandle<PromiseReaction> fulfillReaction = factory->NewPromiseReaction();
    fulfillReaction->SetPromiseCapability(thread, capbility.GetTaggedValue());
    fulfillReaction->SetHandler(thread, paramMsg.GetTaggedValue());

    JSHandle<JSFunction> testPromiseResolved =
        factory->NewJSFunction(env, reinterpret_cast<void *>(TestPromiseOnResolved));
    JSHandle<TaggedArray> handleArgv = factory->NewTaggedArray(2);
    handleArgv->Set(thread, 0, fulfillReaction.GetTaggedValue());
    handleArgv->Set(thread, 1, JSTaggedValue::Undefined());

    JSHandle<PendingJob> handlePendingJob = factory->NewPendingJob(testPromiseResolved, handleArgv);
    JSTaggedValue callResult = PendingJob::ExecutePendingJob(handlePendingJob, thread);
    EXPECT_EQ(callResult, JSTaggedValue::Undefined());
    JSHandle<JSPromise> jsPromise(thread, capbility->GetPromise());
    EXPECT_EQ(jsPromise->GetPromiseState(), PromiseState::PENDING);
    EXPECT_EQ(jsPromise->GetPromiseResult().IsUndefined(), true);
}

JSTaggedValue TestPromiseResolveThenableJob(EcmaRuntimeCallInfo *argv)
{
    JSHandle<JSTaggedValue> result = BuiltinsBase::GetCallArg(argv, 0);
    EXPECT_TRUE(result->IsJSFunction());
    JSHandle<JSTaggedValue> undefined(argv->GetThread(), JSTaggedValue::Undefined());
    EcmaRuntimeCallInfo info =
        EcmaInterpreter::NewRuntimeCallInfo(argv->GetThread(), result, undefined, undefined, 1);
    info.SetCallArg(JSTaggedValue(44));  // 44 : 44 promise result
    return JSFunction::Call(&info);
}

/**
 * @tc.name: ExecutePendingJob_005
 * @tc.desc: Get a function called promiseresolvethenablejob from env. According to the definition of function,
 *           define a TaggedArray object with length of three.set the required value and define a pendingjob object
 *           according to both.The pendingjob object call "ExecutePendingJob" function to execute the method of function
 *           and return the value of the method.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(PendingJobTest, ExecutePendingJob_005)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> promiseFunc = env->GetPromiseFunction();
    JSHandle<JSTaggedValue> paramMsg(thread, JSTaggedValue::Undefined());

    JSHandle<JSPromise> jsPromise =
        JSHandle<JSPromise>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(promiseFunc), promiseFunc));
    JSHandle<JSFunction> testPromiseResolveThenableJob =
        factory->NewJSFunction(env, reinterpret_cast<void *>(TestPromiseResolveThenableJob));

    JSHandle<JSFunction> promiseResolveThenableJob(env->GetPromiseResolveThenableJob());
    JSHandle<TaggedArray> handleArgv = factory->NewTaggedArray(3);
    handleArgv->Set(thread, 0, jsPromise.GetTaggedValue());
    handleArgv->Set(thread, 1, paramMsg.GetTaggedValue());
    handleArgv->Set(thread, 2, testPromiseResolveThenableJob.GetTaggedValue());

    JSHandle<PendingJob> handlePendingJob = factory->NewPendingJob(promiseResolveThenableJob, handleArgv);
    JSTaggedValue callResult = PendingJob::ExecutePendingJob(handlePendingJob, thread);
    EXPECT_EQ(callResult, JSTaggedValue::Undefined());

    EXPECT_EQ(jsPromise->GetPromiseState(), PromiseState::FULFILLED);
    EXPECT_EQ(JSTaggedValue::SameValue(jsPromise->GetPromiseResult(), JSTaggedValue(44)), true);
}
} // namespace panda::test