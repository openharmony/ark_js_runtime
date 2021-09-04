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

#ifndef PANDA_RUNTIME_DEBUG_TEST_TEST_RUNNER_H
#define PANDA_RUNTIME_DEBUG_TEST_TEST_RUNNER_H

#include "ecmascript/tooling/test/test_util.h"
#include "ecmascript/tooling/agent/js_pt_hooks.h"
#include "ecmascript/tooling/agent/js_backend.h"

namespace panda::tooling::ecmascript::test {
class TestRunner : public PtHooks {
public:
    TestRunner(const char *test_name, EcmaVM *vm)
    {
        backend_ = std::make_unique<JSBackend>(vm);
        test_name_ = test_name;
        test_ = TestUtil::GetTest(test_name);
        test_->backend = backend_.get();
        test_->debug_interface = backend_->GetDebugger();
        debug_interface_ = backend_->GetDebugger();
        TestUtil::Reset();
        debug_interface_->RegisterHooks(this);
    }

    void Run()
    {
        if (test_->scenario) {
            test_->scenario();
        }
    }

    void Breakpoint(PtThread ecmaVm, const PtLocation &location) override
    {
        if (test_->breakpoint) {
            test_->breakpoint(ecmaVm, location);
        }
    }

    void LoadModule(std::string_view panda_file_name) override
    {
        if (test_->load_module) {
            test_->load_module(panda_file_name);
        }
    }

    void Paused(PauseReason reason) override
    {
        if (test_->paused) {
            test_->paused(reason);
        }
    };

    void Exception(PtThread ecmaVm, const PtLocation &location, [[maybe_unused]] PtObject exceptionObject,
                   [[maybe_unused]] const PtLocation &catchLocation) override
    {
        if (test_->exception) {
            test_->exception(ecmaVm, location);
        }
    }

    void MethodEntry(PtThread ecmaVm, PtMethod method) override
    {
        if (test_->method_entry) {
            test_->method_entry(ecmaVm, method);
        }
    }

    void SingleStep(PtThread ecmaVm, const PtLocation &location) override
    {
        if (test_->single_step) {
            test_->single_step(ecmaVm, location);
        }
    }

    void VmDeath() override
    {
        if (test_->vm_death) {
            test_->vm_death();
        }
        TestUtil::Event(DebugEvent::VM_DEATH);
    }

    void VmInitialization([[maybe_unused]] PtThread ecmaVm) override
    {
        if (test_->vm_init) {
            test_->vm_init();
        }
        TestUtil::Event(DebugEvent::VM_INITIALIZATION);
    }

    void VmStart() override
    {
        if (test_->vm_start) {
            test_->vm_start();
        }
    }

    void TerminateTest()
    {
        debug_interface_->RegisterHooks(nullptr);
        if (TestUtil::IsTestFinished()) {
            return;
        }
        LOG(FATAL, DEBUGGER) << "Test " << test_name_ << " failed";
    }

    void ThreadStart(PtThread) override {}
    void ThreadEnd(PtThread) override {}
    void ClassLoad(PtThread, PtClass) override {}
    void ClassPrepare(PtThread, PtClass) override {}
    void MonitorWait(PtThread, PtObject, int64_t) override {}
    void MonitorWaited(PtThread, PtObject, bool) override {}
    void MonitorContendedEnter(PtThread, PtObject) override {}
    void MonitorContendedEntered(PtThread, PtObject) override {}
    void ExceptionCatch(PtThread, const PtLocation &, PtObject) override {}
    void PropertyAccess(PtThread, const PtLocation &, PtObject, PtProperty) override {}
    void PropertyModification(PtThread, const PtLocation &, PtObject, PtProperty, PtValue) override {}
    void FramePop(PtThread, PtMethod, bool) override {}
    void GarbageCollectionFinish() override {}
    void GarbageCollectionStart() override {}
    void ObjectAlloc(PtClass, PtObject, PtThread, size_t) override {}
    void MethodExit(PtThread, PtMethod, bool, PtValue) override {}
    void ExceptionRevoked(ExceptionWrapper, ExceptionID) override {}
    void ExecutionContextCreated(ExecutionContextWrapper) override {}
    void ExecutionContextDestroyed(ExecutionContextWrapper) override {}
    void ExecutionContextsCleared() override {}
    void InspectRequested(PtObject, PtObject) override {}

    ~TestRunner() = default;

private:
    std::unique_ptr<JSBackend> backend_ {nullptr};
    JSDebugger *debug_interface_;
    const char *test_name_;
    ApiTest *test_;
};
}  // namespace panda::tooling::ecmascript::test

#endif  // PANDA_RUNTIME_DEBUG_TEST_TEST_RUNNER_H
