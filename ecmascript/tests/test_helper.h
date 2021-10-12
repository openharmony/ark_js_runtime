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

#ifndef ECMASCRIPT_TESTS_TEST_HELPER_H
#define ECMASCRIPT_TESTS_TEST_HELPER_H

#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/ecma_language_context.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/object_factory.h"
#include "gtest/gtest.h"
#include "include/runtime_options.h"

namespace panda::test {
using panda::ecmascript::EcmaHandleScope;
using panda::ecmascript::EcmaRuntimeCallInfo;
using panda::ecmascript::EcmaVM;
using panda::ecmascript::FrameState;
using panda::ecmascript::JSTaggedType;
using panda::ecmascript::JSTaggedValue;
using panda::ecmascript::JSThread;
using panda::ecmascript::NUM_MANDATORY_JSFUNC_ARGS;

// Add for hmf tests platform, define to TEST_F or TEST_P when running gtest in gitlab
#define HWTEST_F_L0(testsuit, testcase) HWTEST_F(testsuit, testcase, testing::ext::TestSize.Level0)
#define HWTEST_P_L0(testsuit, testcase) HWTEST_P(testsuit, testcase, testing::ext::TestSize.Level0)

class TestHelper {
public:
    static std::unique_ptr<EcmaRuntimeCallInfo> CreateEcmaRuntimeCallInfo(JSThread *thread, JSTaggedValue newTgt,
                                                                          array_size_t argvLength)
    {
        const uint8_t testDecodedSize = 2;
        // argvLength includes number of int64_t to store value and tag of function, 'this' and call args
        // It doesn't include new.target argument
        uint32_t numActualArgs = argvLength / testDecodedSize + 1;
        JSTaggedType *sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
        size_t frameSize = ecmascript::FRAME_STATE_SIZE + numActualArgs;
        JSTaggedType *newSp = sp - frameSize;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

        auto callInfo =
            std::make_unique<EcmaRuntimeCallInfo>(thread, numActualArgs, reinterpret_cast<JSTaggedValue *>(newSp));
        callInfo->SetNewTarget(newTgt);
        return callInfo;
    }

    static JSTaggedType *SetupFrame(JSThread *thread, EcmaRuntimeCallInfo *info)
    {
        JSTaggedType *sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
        size_t frameSize = ecmascript::FRAME_STATE_SIZE + info->GetArgsNumber() + NUM_MANDATORY_JSFUNC_ARGS;
        JSTaggedType *newSp = sp - frameSize;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

        FrameState *state = reinterpret_cast<FrameState *>(newSp) - 1;
        state->base.frameType = ecmascript::FrameType::INTERPRETER_FRAME;
        state->base.prev = sp;
        state->pc = nullptr;
        state->sp = newSp;
        state->method = thread->GetEcmaVM()->GetMethodForNativeFunction(nullptr);
        thread->SetCurrentSPFrame(newSp);
        return sp;
    }

    static void TearDownFrame(JSThread *thread, JSTaggedType *prev)
    {
        thread->SetCurrentSPFrame(prev);
    }

    // If you want to call once create, you can refer to BuiltinsMathTest for detail.
    static void CreateEcmaVMWithScope(PandaVM *&instance, JSThread *&thread, EcmaHandleScope *&scope)
    {
        RuntimeOptions options;
        options.SetShouldLoadBootPandaFiles(false);
        options.SetShouldInitializeIntrinsics(false);
        options.SetBootClassSpaces({"ecmascript"});
        options.SetRuntimeType("ecmascript");
        options.SetPreGcHeapVerifyEnabled(true);
        static EcmaLanguageContext lcEcma;
        [[maybe_unused]] bool success = Runtime::Create(options, {&lcEcma});
        ASSERT_TRUE(success) << "Cannot create Runtime";
        instance = Runtime::GetCurrent()->GetPandaVM();
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = EcmaVM::Cast(instance)->GetJSThread();
        scope = new EcmaHandleScope(thread);
        thread->SetIsEcmaInterpreter(true);
        EcmaVM::Cast(instance)->GetFactory()->SetTriggerGc(true);
    }

    static inline void DestroyEcmaVMWithScope(PandaVM *instance, EcmaHandleScope *scope)
    {
        delete scope;
        EcmaVM::Cast(instance)->GetFactory()->SetTriggerGc(false);
        auto thread = EcmaVM::Cast(instance)->GetJSThread();
        thread->ClearException();
        [[maybe_unused]] bool success = Runtime::Destroy();
        ASSERT_TRUE(success) << "Cannot destroy Runtime";
    }
};
}  // namespace panda::test
#endif  // ECMASCRIPT_TESTS_TEST_HELPER_H
