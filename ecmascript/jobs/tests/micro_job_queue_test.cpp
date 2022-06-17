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

#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/tagged_queue.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
using MicroJobQueue = ecmascript::job::MicroJobQueue;
using PendingJob = ecmascript::job::PendingJob;
using QueueType = job::QueueType;
class MicroJobQueueTest : public testing::Test {
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

/**
 * @tc.name: GetJobQueue
 * @tc.desc: Check whether the result returned through "GetPromiseJobQueue" and "GetScriptJobQueue" function
 *           is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(MicroJobQueueTest, GetJobQueue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint32_t capacity = 4;
    JSHandle<JSTaggedValue> handleValue(thread, JSTaggedValue(123));

    JSHandle<TaggedQueue> handlePromiseQueue = factory->NewTaggedQueue(capacity);
    TaggedQueue::PushFixedQueue(thread, handlePromiseQueue, handleValue);
    JSHandle<TaggedQueue> handleScriptQueue = factory->NewTaggedQueue(capacity - 1);

    JSHandle<MicroJobQueue> handleMicroJobQueue = factory->NewMicroJobQueue();
    EXPECT_TRUE(*handleMicroJobQueue != nullptr);

    handleMicroJobQueue->SetPromiseJobQueue(thread, handlePromiseQueue.GetTaggedValue());
    handleMicroJobQueue->SetScriptJobQueue(thread, handleScriptQueue.GetTaggedValue());

    JSHandle<TaggedQueue> promiseQueue(thread, handleMicroJobQueue->GetPromiseJobQueue());
    JSHandle<TaggedQueue> scriptQueue(thread, handleMicroJobQueue->GetScriptJobQueue());

    EXPECT_EQ(promiseQueue->Size(), 1U);
    EXPECT_EQ(scriptQueue->Size(), 0U);

    EXPECT_EQ(promiseQueue->Back().GetInt(), 123);
    EXPECT_TRUE(scriptQueue->Back().IsHole());
}

/**
 * @tc.name: EnqueuePromiseJob
 * @tc.desc: Get a JobQueue called MicroJobQueue from vm.define a function and TaggedArray object,call EnqueuePromiseJob
 *           function to enter the "function" and TaggedArray object into the promise job queueof the MicroJobQueue,then
 *           check whether the object out of the queue is the same as the object in the queue.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(MicroJobQueueTest, EnqueuePromiseJob)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<MicroJobQueue> handleMicrojob = thread->GetEcmaVM()->GetMicroJobQueue();
    JSHandle<TaggedQueue> originalPromiseQueue(thread, handleMicrojob->GetPromiseJobQueue());
    JSHandle<JSTaggedValue> scriptQueue(thread, handleMicrojob->GetScriptJobQueue());

    JSHandle<TaggedArray> arguments = factory->NewTaggedArray(2);
    arguments->Set(thread, 0, JSTaggedValue::Undefined());
    arguments->Set(thread, 1, JSTaggedValue::Undefined());
    JSHandle<JSFunction> promiseReactionsJob(globalEnv->GetPromiseReactionJob());

    QueueType type = QueueType::QUEUE_PROMISE;
    MicroJobQueue::EnqueueJob(thread, handleMicrojob, type, promiseReactionsJob, arguments);

    JSHandle<TaggedQueue> promiseQueue(thread, handleMicrojob->GetPromiseJobQueue());
    EXPECT_EQ(JSTaggedValue::SameValue(promiseQueue.GetTaggedValue(), originalPromiseQueue.GetTaggedValue()), false);
    EXPECT_EQ(JSTaggedValue::SameValue(handleMicrojob->GetScriptJobQueue(), scriptQueue.GetTaggedValue()), true);

    JSTaggedValue result = promiseQueue->Pop(thread);
    EXPECT_TRUE(result.IsPendingJob());

    JSHandle<PendingJob> pendingJob(thread, result);
    EXPECT_EQ(JSTaggedValue::SameValue(pendingJob->GetJob(), promiseReactionsJob.GetTaggedValue()), true);
    EXPECT_EQ(JSTaggedValue::SameValue(pendingJob->GetArguments(), arguments.GetTaggedValue()), true);
}

/**
 * @tc.name: EnqueuePromiseJob
 * @tc.desc: Get a JobQueue called MicroJobQueue from vm.define a function and TaggedArray object,call EnqueuePromiseJob
 *           function to enter the "function" and TaggedArray object into the script job queue of the MicroJobQueue,then
 *           check whether the object out of the queue is the same as the object in the queue.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(MicroJobQueueTest, EnqueueScriptJob)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<MicroJobQueue> handleMicrojob = thread->GetEcmaVM()->GetMicroJobQueue();
    JSHandle<JSTaggedValue> promiseQueue(thread, handleMicrojob->GetPromiseJobQueue());
    JSHandle<TaggedQueue> originalScriptQueue(thread, handleMicrojob->GetScriptJobQueue());

    JSHandle<TaggedArray> arguments1 = factory->NewTaggedArray(2);
    JSHandle<JSFunction> promiseReactionsJob(globalEnv->GetPromiseReactionJob());

    QueueType type = QueueType::QUEUE_SCRIPT;
    MicroJobQueue::EnqueueJob(thread, handleMicrojob, type, promiseReactionsJob, arguments1);

    JSHandle<JSFunction> promiseResolveThenableJob(globalEnv->GetPromiseResolveThenableJob());
    JSHandle<TaggedArray> arguments2 = factory->NewTaggedArray(2);
    arguments2->Set(thread, 0, JSTaggedValue(134));
    arguments2->Set(thread, 1, JSTaggedValue::Undefined());
    MicroJobQueue::EnqueueJob(thread, handleMicrojob, type, promiseResolveThenableJob, arguments2);

    JSHandle<TaggedQueue> scriptQueue(thread, handleMicrojob->GetScriptJobQueue());
    EXPECT_EQ(JSTaggedValue::SameValue(scriptQueue.GetTaggedValue(), originalScriptQueue.GetTaggedValue()), false);
    EXPECT_EQ(JSTaggedValue::SameValue(handleMicrojob->GetPromiseJobQueue(), promiseQueue.GetTaggedValue()), true);

    JSTaggedValue result1 = scriptQueue->Pop(thread);
    EXPECT_TRUE(result1.IsPendingJob());
    // FIFO
    JSHandle<PendingJob> pendingJob1(thread, result1);
    EXPECT_EQ(JSTaggedValue::SameValue(pendingJob1->GetJob(), promiseReactionsJob.GetTaggedValue()), true);
    EXPECT_EQ(JSTaggedValue::SameValue(pendingJob1->GetArguments(), arguments1.GetTaggedValue()), true);

    JSTaggedValue result2 = scriptQueue->Pop(thread);
    EXPECT_TRUE(result2.IsPendingJob());
    JSHandle<PendingJob> pendingJob2(thread, result2);
    EXPECT_EQ(JSTaggedValue::SameValue(pendingJob2->GetJob(), promiseResolveThenableJob.GetTaggedValue()), true);
    EXPECT_EQ(JSTaggedValue::SameValue(pendingJob2->GetArguments(), arguments2.GetTaggedValue()), true);
}

/**
 * @tc.name: ExecutePendingJob_001
 * @tc.desc: Get a JobQueue called MicroJobQueue from vm and get a function called PromiseReactionJob from env.
 *           According to the definition of function,define a TaggedArray object with length of two.set the required
 *           value and enter "function" and TaggedArray object into the promise job queue.Calling "ExecutePendingJob"
 *           function to execute the method of function and return the value of the method.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(MicroJobQueueTest, ExecutePendingJob_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> promiseReactionsJob(env->GetPromiseReactionJob());
    JSHandle<MicroJobQueue> handleMicrojob = thread->GetEcmaVM()->GetMicroJobQueue();
    
    JSHandle<JSTaggedValue> promiseFunc = env->GetPromiseFunction();
    JSHandle<PromiseCapability> capbility = JSPromise::NewPromiseCapability(thread, promiseFunc);
    JSHandle<JSTaggedValue> resolve(thread, capbility->GetResolve());

    JSHandle<PromiseReaction> fulfillReaction = factory->NewPromiseReaction();
    fulfillReaction->SetPromiseCapability(thread, capbility.GetTaggedValue());
    fulfillReaction->SetHandler(thread, resolve.GetTaggedValue());

    JSHandle<TaggedArray> arguments = factory->NewTaggedArray(2);
    arguments->Set(thread, 0, fulfillReaction.GetTaggedValue());
    arguments->Set(thread, 1, JSTaggedValue::Undefined());
    MicroJobQueue::EnqueueJob(thread, handleMicrojob, QueueType::QUEUE_PROMISE, promiseReactionsJob, arguments);

    JSHandle<PromiseReaction> rejectReaction = factory->NewPromiseReaction();
    rejectReaction->SetPromiseCapability(thread, capbility.GetTaggedValue());
    rejectReaction->SetHandler(thread, resolve.GetTaggedValue());

    // get into the promise queue and execute PendingJob
    if (!thread->HasPendingException()) {
        MicroJobQueue::ExecutePendingJob(thread, handleMicrojob);
    }
    JSHandle<JSPromise> jsPromise(thread, capbility->GetPromise());
    EXPECT_EQ(jsPromise->GetPromiseState(), PromiseState::FULFILLED);
    EXPECT_EQ(JSTaggedValue::SameValue(jsPromise->GetPromiseResult(), JSTaggedValue::Undefined()), true);
}

/**
 * @tc.name: ExecutePendingJob_002
 * @tc.desc: Get a JobQueue called MicroJobQueue from vm and get a function called PromiseReactionJob from env.
 *           According to the definition of function,define a TaggedArray object with length of two.set the required
 *           value and enter the "function" and TaggedArray object into the script job queue and promise job queue.
 *           Calling "ExecutePendingJob" function to execute the method of Two queue function and return the value
 *           of the method.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(MicroJobQueueTest, ExecutePendingJob_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> promiseReactionsJob(env->GetPromiseReactionJob());
    JSHandle<MicroJobQueue> handleMicrojob = thread->GetEcmaVM()->GetMicroJobQueue();
    
    JSHandle<JSTaggedValue> promiseFunc = env->GetPromiseFunction();
    JSHandle<PromiseCapability> capbility1 = JSPromise::NewPromiseCapability(thread, promiseFunc);
    JSHandle<JSTaggedValue> resolve(thread, capbility1->GetResolve());

    JSHandle<PromiseReaction> fulfillReaction = factory->NewPromiseReaction();
    fulfillReaction->SetPromiseCapability(thread, capbility1.GetTaggedValue());
    fulfillReaction->SetHandler(thread, resolve.GetTaggedValue());

    JSHandle<TaggedArray> arguments1 = factory->NewTaggedArray(2);
    arguments1->Set(thread, 0, fulfillReaction.GetTaggedValue());
    arguments1->Set(thread, 1, JSTaggedValue::Undefined());
    MicroJobQueue::EnqueueJob(thread, handleMicrojob, QueueType::QUEUE_PROMISE, promiseReactionsJob, arguments1);

    JSHandle<PromiseCapability> capbility2 = JSPromise::NewPromiseCapability(thread, promiseFunc);
    JSHandle<JSTaggedValue> reject(thread, capbility2->GetReject());
    JSHandle<PromiseReaction> rejectReaction = factory->NewPromiseReaction();
    rejectReaction->SetPromiseCapability(thread, capbility2.GetTaggedValue());
    rejectReaction->SetHandler(thread, reject.GetTaggedValue());

    JSHandle<TaggedArray> arguments2 = factory->NewTaggedArray(2);
    arguments2->Set(thread, 0, rejectReaction.GetTaggedValue());
    arguments2->Set(thread, 1, JSTaggedValue(32));
    MicroJobQueue::EnqueueJob(thread, handleMicrojob, QueueType::QUEUE_SCRIPT, promiseReactionsJob, arguments2);

    // get into the promise queue and execute PendingJob
    if (!thread->HasPendingException()) {
        MicroJobQueue::ExecutePendingJob(thread, handleMicrojob);
    }
    JSHandle<JSPromise> resolvePromise(thread, capbility1->GetPromise());
    EXPECT_EQ(resolvePromise->GetPromiseState(), PromiseState::FULFILLED);
    EXPECT_EQ(JSTaggedValue::SameValue(resolvePromise->GetPromiseResult(), JSTaggedValue::Undefined()), true);

    JSHandle<JSPromise> rejectPromise(thread, capbility2->GetPromise());
    EXPECT_EQ(rejectPromise->GetPromiseState(), PromiseState::REJECTED);
    EXPECT_EQ(JSTaggedValue::SameValue(rejectPromise->GetPromiseResult(), JSTaggedValue(32)), true);
}
} // namespace panda::test