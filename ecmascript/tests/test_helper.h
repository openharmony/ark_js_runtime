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
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/object_factory.h"
#include "gtest/gtest.h"

namespace panda::test {
using panda::ecmascript::EcmaHandleScope;
using panda::ecmascript::EcmaRuntimeCallInfo;
using panda::ecmascript::EcmaVM;
using panda::ecmascript::InterpretedFrame;
using panda::ecmascript::InterpretedBuiltinFrame;
using panda::ecmascript::InterpretedEntryFrame;
using panda::ecmascript::JSTaggedType;
using panda::ecmascript::JSTaggedValue;
using panda::ecmascript::JSThread;
using panda::ecmascript::NUM_MANDATORY_JSFUNC_ARGS;
using ecmascript::JSRuntimeOptions;

#define HWTEST_F_L0(testsuit, testcase) HWTEST_F(testsuit, testcase, testing::ext::TestSize.Level0)
#define HWTEST_P_L0(testsuit, testcase) HWTEST_P(testsuit, testcase, testing::ext::TestSize.Level0)

class TestHelper {
public:
    static EcmaRuntimeCallInfo* CreateEcmaRuntimeCallInfo(JSThread *thread, JSTaggedValue newTgt, uint32_t argvLength)
    {
        const uint8_t testDecodedSize = 2;
        // argvLength includes number of int64_t to store value and tag of function, 'this' and call args
        // It doesn't include new.target argument
        int32_t numActualArgs = argvLength / testDecodedSize + 1;
        JSTaggedType *sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
        size_t frameSize = InterpretedFrame::NumOfMembers() + numActualArgs;
        JSTaggedType *newSp = sp - frameSize;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        for (int i = numActualArgs; i > 0; i--) {
            newSp[i - 1] = JSTaggedValue::Undefined().GetRawData();
        }
        EcmaRuntimeCallInfo *ecmaRuntimeCallInfo = reinterpret_cast<EcmaRuntimeCallInfo *>(newSp - 2);
        *(--newSp) = numActualArgs;
        *(--newSp) = ToUintPtr(thread);
        ecmaRuntimeCallInfo->SetNewTarget(newTgt);
        return ecmaRuntimeCallInfo;
    }

    static JSTaggedType *SetupFrame(JSThread *thread, EcmaRuntimeCallInfo *info)
    {
        JSTaggedType *sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
        size_t frameSize =
            InterpretedFrame::NumOfMembers() + info->GetArgsNumber() + NUM_MANDATORY_JSFUNC_ARGS + 2;
        JSTaggedType *newSp = sp - frameSize;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)

        InterpretedBuiltinFrame *state = reinterpret_cast<InterpretedBuiltinFrame *>(newSp) - 1;
        state->base.type = ecmascript::FrameType::INTERPRETER_BUILTIN_FRAME;
        state->base.prev = sp;
        state->pc = nullptr;
        state->function = methodFunction_.GetTaggedValue();
        thread->SetCurrentSPFrame(newSp);
        return sp;
    }

    static void TearDownFrame(JSThread *thread, JSTaggedType *prev)
    {
        thread->SetCurrentSPFrame(prev);
    }

    // If you want to call once create, you can refer to BuiltinsMathTest for detail.
    static void CreateEcmaVMWithScope(EcmaVM *&instance, JSThread *&thread, EcmaHandleScope *&scope,
        bool tryLoadStubFile = false)
    {
        JSRuntimeOptions options;
        options.SetEnableForceGC(true);
        if (tryLoadStubFile) {
            options.SetEnableAsmInterpreter(true);
        }
        instance = JSNApi::CreateEcmaVM(options);
        instance->SetEnableForceGC(true);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        scope = new EcmaHandleScope(thread);
        auto globalEnv = instance->GetGlobalEnv();
        methodFunction_ = instance->GetFactory()->NewJSFunction(globalEnv);
    }

    static inline void DestroyEcmaVMWithScope(EcmaVM *instance, EcmaHandleScope *scope)
    {
        delete scope;
        scope = nullptr;
        instance->SetEnableForceGC(false);
        auto thread = instance->GetJSThread();
        thread->ClearException();
        JSNApi::DestroyJSVM(instance);
    }

private:
    inline static ecmascript::JSHandle<ecmascript::JSFunction> methodFunction_;
};
}  // namespace panda::test
#endif  // ECMASCRIPT_TESTS_TEST_HELPER_H
