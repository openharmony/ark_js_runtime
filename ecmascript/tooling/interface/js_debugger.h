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

#ifndef ECMASCRIPT_TOOLING_INTERFACE_JS_DEBUGGER_H
#define ECMASCRIPT_TOOLING_INTERFACE_JS_DEBUGGER_H

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_method.h"
#include "ecmascript/tooling/interface/debugger_api.h"
#include "tooling/debugger.h"

namespace panda::tooling::ecmascript {
using panda::ecmascript::CUnorderedSet;

class JSDebugger : public DebugInterface, RuntimeListener {
public:
    explicit JSDebugger(const EcmaVM *vm) : ecmaVm_(vm)
    {
        auto notificationMgr = ecmaVm_->GetNotificationManager();
        notificationMgr->AddListener(this, JSDEBUG_EVENT_MASK);
    }
    ~JSDebugger() override
    {
        auto notificationMgr = ecmaVm_->GetNotificationManager();
        notificationMgr->RemoveListener(this, JSDEBUG_EVENT_MASK);
    }

    std::optional<Error> RegisterHooks(PtHooks *hooks) override
    {
        hooks_ = hooks;
        return {};
    }
    std::optional<Error> UnregisterHooks() override
    {
        hooks_ = nullptr;
        return {};
    }

    std::optional<Error> SetBreakpoint(const PtLocation &location) override;
    std::optional<Error> RemoveBreakpoint(const PtLocation &location) override;
    void BytecodePcChanged(ManagedThread *thread, Method *method, uint32_t bcOffset) override;
    void MethodEntry(ManagedThread *thread, Method *method) override;
    void MethodExit(ManagedThread *thread, Method *method) override;
    void LoadModule(std::string_view filename) override
    {
        if (hooks_ == nullptr) {
            return;
        }
        hooks_->LoadModule(filename);
    }
    void VmStart() override
    {
        if (hooks_ == nullptr) {
            return;
        }
        hooks_->VmStart();
    }
    void VmInitialization(ManagedThread::ThreadId threadId) override
    {
        if (hooks_ == nullptr) {
            return;
        }
        hooks_->VmInitialization(PtThread(threadId));
    }
    void VmDeath() override
    {
        if (hooks_ == nullptr) {
            return;
        }
        hooks_->VmDeath();
    }

    PtLangExt *GetLangExtension() const override
    {
        return nullptr;
    }
    Expected<PtMethod, Error> GetPtMethod([[maybe_unused]] const PtLocation &location) const override
    {
        return Unexpected(Error(Error::Type::INVALID_VALUE, "Unsupported GetPtMethod"));
    }
    std::optional<Error> EnableAllGlobalHook() override
    {
        return {};
    }
    std::optional<Error> DisableAllGlobalHook() override
    {
        return {};
    }
    std::optional<Error> SetNotification([[maybe_unused]] PtThread thread, [[maybe_unused]] bool enable,
                                         [[maybe_unused]] PtHookType hookType) override
    {
        return {};
    }
    Expected<std::unique_ptr<PtFrame>, Error> GetCurrentFrame([[maybe_unused]] PtThread thread) const override
    {
        return Unexpected(Error(Error::Type::INVALID_VALUE, "Unsupported GetCurrentFrame"));
    }
    std::optional<Error> EnumerateFrames([[maybe_unused]] PtThread thread,
                                         [[maybe_unused]] std::function<bool(const PtFrame &)> callback) const override
    {
        return {};
    }
    std::optional<Error> GetThisVariableByFrame([[maybe_unused]] PtThread thread, [[maybe_unused]] uint32_t frameDepth,
                                                [[maybe_unused]] PtValue *value) override
    {
        return {};
    }
    void ThreadStart([[maybe_unused]] ManagedThread::ThreadId threadId) override {}
    void ThreadEnd([[maybe_unused]] ManagedThread::ThreadId threadId) override {}
    void GarbageCollectorStart() override {}
    void GarbageCollectorFinish() override {}
    void ObjectAlloc([[maybe_unused]] BaseClass *klass, [[maybe_unused]] ObjectHeader *object,
                     [[maybe_unused]] ManagedThread *thread, [[maybe_unused]] size_t size) override {}
    void ExceptionCatch([[maybe_unused]] const ManagedThread *thread, [[maybe_unused]] const Method *method,
                        [[maybe_unused]] uint32_t bcOffset) override {}
    void ClassLoad([[maybe_unused]] Class *klass) override {}
    void ClassPrepare([[maybe_unused]] Class *klass) override {}
    void MonitorWait([[maybe_unused]] ObjectHeader *object, [[maybe_unused]] int64_t timeout) override {}
    void MonitorWaited([[maybe_unused]] ObjectHeader *object, [[maybe_unused]] bool timedOut) override {}
    void MonitorContendedEnter([[maybe_unused]] ObjectHeader *object) override {}
    void MonitorContendedEntered([[maybe_unused]] ObjectHeader *object) override {}

    std::optional<Error> GetThreadList([[maybe_unused]] PandaVector<PtThread> *threadList) const override
    {
        return {};
    }
    std::optional<Error> GetThreadInfo([[maybe_unused]] PtThread thread,
                                       [[maybe_unused]] ThreadInfo *infoPtr) const override
    {
        return {};
    }
    std::optional<Error> SuspendThread([[maybe_unused]] PtThread thread) const override
    {
        return {};
    }
    std::optional<Error> ResumeThread([[maybe_unused]] PtThread thread) const override
    {
        return {};
    }
    std::optional<Error> SetVariable([[maybe_unused]] PtThread thread, [[maybe_unused]] uint32_t frameDepth,
                                     [[maybe_unused]] int32_t regNumber,
                                     [[maybe_unused]] const PtValue &value) const override
    {
        return {};
    }
    std::optional<Error> GetVariable([[maybe_unused]] PtThread thread, [[maybe_unused]] uint32_t frameDepth,
                                     [[maybe_unused]] int32_t regNumber,
                                     [[maybe_unused]] PtValue *result) const override
    {
        return {};
    }
    std::optional<Error> GetProperty([[maybe_unused]] PtObject object, [[maybe_unused]] PtProperty property,
                                     [[maybe_unused]] PtValue *value) const override
    {
        return {};
    }
    std::optional<Error> SetProperty([[maybe_unused]] PtObject object, [[maybe_unused]] PtProperty property,
                                     [[maybe_unused]] const PtValue &value) const override
    {
        return {};
    }
    std::optional<Error> EvaluateExpression([[maybe_unused]] PtThread thread, [[maybe_unused]] uint32_t frameNumber,
                                            [[maybe_unused]] ExpressionWrapper expr,
                                            [[maybe_unused]] PtValue *result) const override
    {
        return {};
    }
    std::optional<Error> RetransformClasses([[maybe_unused]] int32_t classCount,
                                            [[maybe_unused]] const PtClass *classes) const override
    {
        return {};
    }
    std::optional<Error> RedefineClasses([[maybe_unused]] int32_t classCount,
                                         [[maybe_unused]] const PandaClassDefinition *classes) const override
    {
        return {};
    }
    std::optional<Error> RestartFrame([[maybe_unused]] PtThread thread,
                                      [[maybe_unused]] uint32_t frameNumber) const override
    {
        return {};
    }
    std::optional<Error> SetAsyncCallStackDepth([[maybe_unused]] uint32_t maxDepth) const override
    {
        return {};
    }
    std::optional<Error> AwaitPromise([[maybe_unused]] PtObject promiseObject,
                                      [[maybe_unused]] PtValue *result) const override
    {
        return {};
    }
    std::optional<Error> CallFunctionOn([[maybe_unused]] PtObject object, [[maybe_unused]] PtMethod method,
                                        [[maybe_unused]] const PandaVector<PtValue> &arguments,
                                        [[maybe_unused]] PtValue *returnValue) const override
    {
        return {};
    }
    std::optional<Error> GetProperties([[maybe_unused]] uint32_t *countPtr,
                                       [[maybe_unused]] char ***propertyPtr) const override
    {
        return {};
    }
    std::optional<Error> NotifyFramePop([[maybe_unused]] PtThread thread,
                                        [[maybe_unused]] uint32_t depth) const override
    {
        return {};
    }
    std::optional<Error> SetPropertyAccessWatch([[maybe_unused]] PtClass klass,
                                                [[maybe_unused]] PtProperty property) override
    {
        return {};
    }
    std::optional<Error> ClearPropertyAccessWatch([[maybe_unused]] PtClass klass,
                                                  [[maybe_unused]] PtProperty property) override
    {
        return {};
    }
    std::optional<Error> SetPropertyModificationWatch([[maybe_unused]] PtClass klass,
                                                      [[maybe_unused]] PtProperty property) override
    {
        return {};
    }
    std::optional<Error> ClearPropertyModificationWatch([[maybe_unused]] PtClass klass,
                                                        [[maybe_unused]] PtProperty property) override
    {
        return {};
    }

private:
    static constexpr uint32_t JSDEBUG_EVENT_MASK = RuntimeNotificationManager::Event::LOAD_MODULE |
                                                   RuntimeNotificationManager::Event::BYTECODE_PC_CHANGED |
                                                   RuntimeNotificationManager::Event::VM_EVENTS |
                                                   RuntimeNotificationManager::Event::METHOD_EVENTS;

    JSMethod *FindMethod(const PtLocation &location) const;
    bool FindBreakpoint(const JSMethod *method, uint32_t bcOffset) const;
    bool RemoveBreakpoint(const JSMethod *method, uint32_t bcOffset);
    bool HandleBreakpoint(const JSThread *thread, const JSMethod *method, uint32_t bcOffset);
    void HandleExceptionThrowEvent(const JSThread *thread, const JSMethod *method, uint32_t bcOffset);
    bool HandleStep(const JSThread *thread, const JSMethod *method, uint32_t bcOffset);

    const EcmaVM *ecmaVm_;
    PtHooks *hooks_ {nullptr};

    CUnorderedSet<tooling::Breakpoint, tooling::HashBreakpoint> breakpoints_ {};
};
}  // namespace panda::tooling::ecmascript

#endif  // ECMASCRIPT_TOOLING_JS_DEBUGGER_H
