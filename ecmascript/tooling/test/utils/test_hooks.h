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

#ifndef ECMASCRIPT_TOOLING_TEST_UTILS_TEST_HOOKS_H
#define ECMASCRIPT_TOOLING_TEST_UTILS_TEST_HOOKS_H

#include "ecmascript/tooling/agent/js_pt_hooks.h"
#include "ecmascript/tooling/agent/js_backend.h"
#include "ecmascript/tooling/test/utils/test_util.h"

namespace panda::ecmascript::tooling::test {
class TestHooks : public PtHooks {
public:
    TestHooks(const char *testName, const EcmaVM *vm)
    {
        backend_ = std::make_unique<JSBackend>(vm);
        testName_ = testName;
        test_ = TestUtil::GetTest(testName);
        test_->backend_ = backend_.get();
        test_->debugInterface_ = backend_->GetDebugger();
        debugInterface_ = backend_->GetDebugger();
        TestUtil::Reset();
        debugInterface_->RegisterHooks(this);
    }

    void Run()
    {
        if (test_->scenario) {
            test_->scenario();
        }
    }

    void Breakpoint(const JSPtLocation &location) override
    {
        if (test_->breakpoint) {
            test_->breakpoint(location);
        }
    }

    void LoadModule(std::string_view panda_file_name) override
    {
        if (test_->loadModule) {
            test_->loadModule(panda_file_name);
        }
    }

    void Paused(PauseReason reason) override
    {
        if (test_->paused) {
            test_->paused(reason);
        }
    };

    void Exception(const JSPtLocation &location) override
    {
        if (test_->exception) {
            Local<JSValueRef> exception = DebuggerApi::GetAndClearException(backend_->GetEcmaVm());

            test_->exception(location);

            if (!exception->IsHole()) {
                DebuggerApi::SetException(backend_->GetEcmaVm(), exception);
            }
        }
    }

    void SingleStep(const JSPtLocation &location) override
    {
        if (test_->singleStep) {
            test_->singleStep(location);
        }
    }

    void VmDeath() override
    {
        if (test_->vmDeath) {
            test_->vmDeath();
        }
        TestUtil::Event(DebugEvent::VM_DEATH);
    }

    void VmStart() override
    {
        if (test_->vmStart) {
            test_->vmStart();
        }
    }

    void TerminateTest()
    {
        debugInterface_->RegisterHooks(nullptr);
        if (TestUtil::IsTestFinished()) {
            return;
        }
        LOG(FATAL, DEBUGGER) << "Test " << testName_ << " failed";
    }

    ~TestHooks() = default;

private:
    std::unique_ptr<JSBackend> backend_ {nullptr};
    JSDebugger *debugInterface_;
    const char *testName_;
    TestEvents *test_;
};
}  // namespace panda::ecmascript::tooling::test

#endif  // ECMASCRIPT_TOOLING_TEST_UTILS_TEST_HOOKS_H
