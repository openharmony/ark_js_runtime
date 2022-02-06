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
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/object_factory.h"
#include "gtest/gtest.h"
#include "include/runtime_options.h"

namespace panda::test {
using panda::ecmascript::EcmaHandleScope;
using panda::ecmascript::EcmaRuntimeCallInfo;
using panda::ecmascript::EcmaVM;
using panda::ecmascript::InterpretedFrame;
using panda::ecmascript::JSTaggedType;
using panda::ecmascript::JSTaggedValue;
using panda::ecmascript::JSThread;
using panda::ecmascript::NUM_MANDATORY_JSFUNC_ARGS;
using ecmascript::JSRuntimeOptions;

// Add for hmf tests platform, define to TEST_F or TEST_P when running gtest in gitlab
#define HWTEST_F_L0(testsuit, testcase) HWTEST_F(testsuit, testcase, testing::ext::TestSize.Level0)
#define HWTEST_P_L0(testsuit, testcase) HWTEST_P(testsuit, testcase, testing::ext::TestSize.Level0)

class TestHelper {
public:
    static std::unique_ptr<EcmaRuntimeCallInfo> CreateEcmaRuntimeCallInfo(JSThread *thread, JSTaggedValue newTgt,
                                                                          uint32_t argvLength)
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

        InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(newSp) - 1;
        state->base.type = ecmascript::FrameType::INTERPRETER_FRAME;
        state->base.prev = sp;
        state->pc = nullptr;
        state->sp = newSp;
        state->function = methodFunction_.GetTaggedValue();
        thread->SetCurrentSPFrame(newSp);
        return sp;
    }

    static void TearDownFrame(JSThread *thread, JSTaggedType *prev)
    {
        thread->SetCurrentSPFrame(prev);
    }

    // If you want to call once create, you can refer to BuiltinsMathTest for detail.
    static void CreateEcmaVMWithScope(PandaVM *&instance, JSThread *&thread, EcmaHandleScope *&scope,
                                      const char *libraryPath = nullptr)
    {
        JSRuntimeOptions options;
        options.SetShouldLoadBootPandaFiles(false);
        options.SetShouldInitializeIntrinsics(false);
        options.SetBootClassSpaces({"ecmascript"});
        options.SetRuntimeType("ecmascript");
        options.SetPreGcHeapVerifyEnabled(true);
        options.SetEnableForceGC(true);
        if (libraryPath != nullptr) {
            // for class Runtime to StartDebugger
            options.SetDebuggerLibraryPath(libraryPath);
        }
        static EcmaLanguageContext lcEcma;
        [[maybe_unused]] bool success = Runtime::Create(options, {&lcEcma});
        ASSERT_TRUE(success) << "Cannot create Runtime";
        // create jspandafile manager for process
        EcmaVM::CreateJSPandaFileManager();
        instance = Runtime::GetCurrent()->GetPandaVM();
        EcmaVM::Cast(instance)->SetEnableForceGC(true);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = EcmaVM::Cast(instance)->GetJSThread();
        scope = new EcmaHandleScope(thread);
        EcmaVM *ecmaVm = thread->GetEcmaVM();
        auto globalEnv = ecmaVm->GetGlobalEnv();
        methodFunction_ = ecmaVm->GetFactory()->NewJSFunction(globalEnv);
    }

    static inline void DestroyEcmaVMWithScope(PandaVM *instance, EcmaHandleScope *scope)
    {
        delete scope;
        EcmaVM::Cast(instance)->SetEnableForceGC(false);
        auto thread = EcmaVM::Cast(instance)->GetJSThread();
        thread->ClearException();
        JSNApi::DestroyJSVM(EcmaVM::Cast(instance));
    }

private:
    inline static ecmascript::JSHandle<ecmascript::JSFunction> methodFunction_;
};
}  // namespace panda::test
#endif  // ECMASCRIPT_TESTS_TEST_HELPER_H
