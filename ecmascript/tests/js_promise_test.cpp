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

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class JSPromiseTest : public testing::Test {
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

    JSThread *thread {nullptr};
    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
};

HWTEST_F_L0(JSPromiseTest, CreateResolvingFunctions)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> promiseFunc = env->GetPromiseFunction();
    JSHandle<JSPromise> jsPromise =
        JSHandle<JSPromise>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(promiseFunc), promiseFunc));
    JSHandle<ResolvingFunctionsRecord> reactions = JSPromise::CreateResolvingFunctions(thread, jsPromise);
    JSHandle<JSTaggedValue> resolve(thread, reactions->GetResolveFunction());
    JSHandle<JSTaggedValue> reject(thread, reactions->GetRejectFunction());
    EXPECT_EQ(resolve->IsCallable(), true);
    EXPECT_EQ(reject->IsCallable(), true);
}

HWTEST_F_L0(JSPromiseTest, NewPromiseCapability)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> promise = env->GetPromiseFunction();

    JSHandle<PromiseCapability> capbility = JSPromise::NewPromiseCapability(thread, promise);
    JSHandle<JSPromise> newPromise(thread, capbility->GetPromise());
    EXPECT_EQ(newPromise->GetPromiseState(), PromiseState::PENDING);

    JSHandle<JSPromiseReactionsFunction> resolve(thread, capbility->GetResolve());
    JSHandle<JSPromiseReactionsFunction> reject(thread, capbility->GetReject());
    EXPECT_EQ(resolve.GetTaggedValue().IsCallable(), true);
    EXPECT_EQ(resolve.GetTaggedValue().IsCallable(), true);

    JSHandle<JSPromise> resolve_promise(thread, resolve->GetPromise());
    JSHandle<JSPromise> reject_promise(thread, reject->GetPromise());
    EXPECT_EQ(JSTaggedValue::SameValue(newPromise.GetTaggedValue(), resolve_promise.GetTaggedValue()), true);
    EXPECT_EQ(JSTaggedValue::SameValue(newPromise.GetTaggedValue(), reject_promise.GetTaggedValue()), true);
}

HWTEST_F_L0(JSPromiseTest, FullFillPromise)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> promise = env->GetPromiseFunction();
    JSHandle<PromiseCapability> capbility = JSPromise::NewPromiseCapability(thread, promise);
    JSHandle<JSPromise> newPromise(thread, capbility->GetPromise());
    EXPECT_EQ(newPromise->GetPromiseState(), PromiseState::PENDING);
    EXPECT_EQ(newPromise->GetPromiseResult().IsUndefined(), true);

    JSHandle<JSTaggedValue> resolve(thread, capbility->GetResolve());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, resolve, undefined, undefined, 1);
    info->SetCallArg(JSTaggedValue(33));
    JSFunction::Call(info);
    EXPECT_EQ(newPromise->GetPromiseState(), PromiseState::FULFILLED);
    EXPECT_EQ(JSTaggedValue::SameValue(newPromise->GetPromiseResult(), JSTaggedValue(33)), true);
}

HWTEST_F_L0(JSPromiseTest, RejectPromise)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> promise = env->GetPromiseFunction();
    JSHandle<PromiseCapability> capbility = JSPromise::NewPromiseCapability(thread, promise);
    JSHandle<JSPromise> newPromise(thread, capbility->GetPromise());
    EXPECT_EQ(newPromise->GetPromiseState(), PromiseState::PENDING);
    EXPECT_EQ(newPromise->GetPromiseResult().IsUndefined(), true);

    JSHandle<JSTaggedValue> reject(thread, capbility->GetReject());
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, reject, undefined, undefined, 1);
    info->SetCallArg(JSTaggedValue(44));
    JSFunction::Call(info);
    EXPECT_EQ(newPromise->GetPromiseState(), PromiseState::REJECTED);
    EXPECT_EQ(JSTaggedValue::SameValue(newPromise->GetPromiseResult(), JSTaggedValue(44)), true);
}
}  // namespace panda::test
