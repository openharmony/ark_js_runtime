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

#include "ecmascript/tooling/agent/debugger_impl.h"
#include "ecmascript/tooling/backend/js_pt_hooks.h"
#include "ecmascript/tooling/test/utils/test_util.h"

namespace panda::ecmascript::tooling::test {
class TestHooks : public PtHooks {
public:
    TestHooks(const std::string &testName, const EcmaVM *vm) : vm_(vm)
    {
        runtime_ = std::make_unique<RuntimeImpl>(vm, nullptr);
        debugger_ = std::make_unique<DebuggerImpl>(vm, nullptr, runtime_.get());
        testName_ = testName;
        test_ = TestUtil::GetTest(testName);
        test_->vm_ = vm;
        test_->debugger_ = debugger_.get();
        test_->debugInterface_ = debugger_->jsDebugger_;
        debugInterface_ = debugger_->jsDebugger_;
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

    void Exception(const JSPtLocation &location) override
    {
        if (test_->exception) {
            Local<JSValueRef> exception = DebuggerApi::GetAndClearException(vm_);

            test_->exception(location);

            if (!exception->IsHole()) {
                DebuggerApi::SetException(vm_, exception);
            }
        }
    }

    bool SingleStep(const JSPtLocation &location) override
    {
        if (test_->singleStep) {
            return test_->singleStep(location);
        }
        return false;
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
        TestUtil::Event(DebugEvent::VM_START);
    }

    void PendingJobEntry() override {}

    void TerminateTest()
    {
        debugInterface_->UnregisterHooks();
        if (TestUtil::IsTestFinished()) {
            return;
        }
        LOG(FATAL, DEBUGGER) << "Test " << testName_ << " failed";
    }

    ~TestHooks() = default;

private:
    const EcmaVM *vm_ {nullptr};
    std::unique_ptr<RuntimeImpl> runtime_ {nullptr};
    std::unique_ptr<DebuggerImpl> debugger_ {nullptr};
    JSDebugger *debugInterface_;
    std::string testName_;
    TestEvents *test_;
};
}  // namespace panda::ecmascript::tooling::test

#endif  // ECMASCRIPT_TOOLING_TEST_UTILS_TEST_HOOKS_H
