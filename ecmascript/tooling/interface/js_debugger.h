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
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/tooling/interface/debugger_api.h"
#include "tooling/debugger.h"

namespace panda::tooling::ecmascript {
using panda::ecmascript::CUnorderedSet;

class JSDebugger : public DebugInterface, RuntimeListener {
public:
    JSDebugger(const Runtime *runtime, const EcmaVM *vm) : runtime_(runtime), ecmaVm_(vm)
    {
        auto notificationMgr = runtime_->GetNotificationManager();
        // set EcmaVM rendezvous
        notificationMgr->SetRendezvous(ecmaVm_->GetRendezvous());
        notificationMgr->AddListener(this, JSDEBUG_EVENT_MASK);
        // set PandaVM rendezvous
        notificationMgr->SetRendezvous(runtime_->GetPandaVM()->GetRendezvous());
    }
    ~JSDebugger() override
    {
        auto notificationMgr = runtime_->GetNotificationManager();
        // set EcmaVM rendezvous
        notificationMgr->SetRendezvous(ecmaVm_->GetRendezvous());
        notificationMgr->RemoveListener(this, JSDEBUG_EVENT_MASK);
        // set PandaVM rendezvous
        notificationMgr->SetRendezvous(runtime_->GetPandaVM()->GetRendezvous());
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
    void LoadModule(std::string_view filename) override
    {
        if (hooks_ != nullptr) {
            hooks_->LoadModule(filename);
        }
    }
    void VmStart() override
    {
        if (hooks_ != nullptr) {
            hooks_->VmStart();
        }
    }
    void VmInitialization(ManagedThread::ThreadId threadId) override
    {
        if (hooks_ != nullptr) {
            hooks_->VmInitialization(PtThread(threadId));
        }
    }
    void VmDeath() override
    {
        if (hooks_ != nullptr) {
            hooks_->VmDeath();
        }
    }

    PtLangExt *GetLangExtension() const override
    {
        return nullptr;
    }
    Expected<PtMethod, Error> GetPtMethod(const PtLocation &location) const override
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
    std::optional<Error> SetNotification(PtThread thread, bool enable, PtHookType hookType) override
    {
        return {};
    }
    Expected<std::unique_ptr<PtFrame>, Error> GetCurrentFrame(PtThread thread) const override
    {
        return Unexpected(Error(Error::Type::INVALID_VALUE, "Unsupported GetCurrentFrame"));
    }
    std::optional<Error> EnumerateFrames(PtThread thread, std::function<bool(const PtFrame &)> callback) const override
    {
        return {};
    }
    void ThreadStart(ManagedThread::ThreadId threadId) override {}
    void ThreadEnd(ManagedThread::ThreadId threadId) override {}
    void GarbageCollectorStart() override {}
    void GarbageCollectorFinish() override {}
    void ObjectAlloc(BaseClass *klass, ObjectHeader *object, ManagedThread *thread, size_t size) override {}
    void ExceptionCatch(const ManagedThread *thread, const Method *method, uint32_t bcOffset) override {}
    void MethodEntry(ManagedThread *thread, Method *method) override {}
    void MethodExit(ManagedThread *thread, Method *method) override {}
    void ClassLoad(Class *klass) override {}
    void ClassPrepare(Class *klass) override {}
    void MonitorWait(ObjectHeader *object, int64_t timeout) override {}
    void MonitorWaited(ObjectHeader *object, bool timedOut) override {}
    void MonitorContendedEnter(ObjectHeader *object) override {}
    void MonitorContendedEntered(ObjectHeader *object) override {}

    std::optional<Error> GetThreadList(PandaVector<PtThread> *threadList) const override
    {
        return {};
    }
    std::optional<Error> GetThreadInfo(PtThread thread, ThreadInfo *infoPtr) const override
    {
        return {};
    }
    std::optional<Error> SuspendThread(PtThread thread) const override
    {
        return {};
    }
    std::optional<Error> ResumeThread(PtThread thread) const override
    {
        return {};
    }
    std::optional<Error> SetVariable(PtThread thread, uint32_t frameDepth, int32_t regNumber,
        const PtValue &value) const override
    {
        return {};
    }
    std::optional<Error> GetVariable(PtThread thread, uint32_t frameDepth, int32_t regNumber,
        PtValue *result) const override
    {
        return {};
    }
    std::optional<Error> GetProperty([[maybe_unused]] PtObject object, [[maybe_unused]] PtProperty property,
        PtValue *value) const override
    {
        return {};
    }
    std::optional<Error> SetProperty([[maybe_unused]] PtObject object, [[maybe_unused]] PtProperty property,
        [[maybe_unused]] const PtValue &value) const override
    {
        return {};
    }
    std::optional<Error> EvaluateExpression([[maybe_unused]] PtThread thread, [[maybe_unused]] uint32_t frameNumber,
        ExpressionWrapper expr, PtValue *result) const override
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
    std::optional<Error> AwaitPromise([[maybe_unused]] PtObject promiseObject, PtValue *result) const override
    {
        return {};
    }
    std::optional<Error> CallFunctionOn([[maybe_unused]] PtObject object, [[maybe_unused]] PtMethod method,
        [[maybe_unused]] const PandaVector<PtValue> &arguments, PtValue *returnValue) const override
    {
        return {};
    }
    std::optional<Error> GetProperties(uint32_t *countPtr, [[maybe_unused]] char ***propertyPtr) const override
    {
        return {};
    }
    std::optional<Error> NotifyFramePop(PtThread thread, uint32_t depth) const override
    {
        return {};
    }
    std::optional<Error> SetPropertyAccessWatch(PtClass klass, PtProperty property) override
    {
        return {};
    }
    std::optional<Error> ClearPropertyAccessWatch(PtClass klass, PtProperty property) override
    {
        return {};
    }
    std::optional<Error> SetPropertyModificationWatch(PtClass klass, PtProperty property) override
    {
        return {};
    }
    std::optional<Error> ClearPropertyModificationWatch(PtClass klass, PtProperty property) override
    {
        return {};
    }

private:
    static constexpr uint32_t JSDEBUG_EVENT_MASK = RuntimeNotificationManager::Event::LOAD_MODULE |
                                                   RuntimeNotificationManager::Event::BYTECODE_PC_CHANGED |
                                                   RuntimeNotificationManager::Event::VM_EVENTS;

    JSMethod *FindMethod(const PtLocation &location) const;
    bool FindBreakpoint(const JSMethod *method, uint32_t bcOffset) const;
    bool RemoveBreakpoint(const JSMethod *method, uint32_t bcOffset);
    bool HandleBreakpoint(const JSThread *thread, const JSMethod *method, uint32_t bcOffset);
    void HandleExceptionThrowEvent(const JSThread *thread, const JSMethod *method, uint32_t bcOffset);
    bool HandleStep(const JSThread *thread, const JSMethod *method, uint32_t bcOffset);

    const Runtime *runtime_;
    const EcmaVM *ecmaVm_;
    PtHooks *hooks_ {nullptr};

    CUnorderedSet<tooling::Breakpoint, tooling::HashBreakpoint> breakpoints_ {};
};
}  // namespace panda::tooling::ecmascript

#endif  // ECMASCRIPT_TOOLING_JS_DEBUGGER_H
