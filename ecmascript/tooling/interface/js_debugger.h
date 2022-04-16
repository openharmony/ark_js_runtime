/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#include "ecmascript/tooling/interface/notification_manager.h"

namespace panda::ecmascript::tooling {
class JSBreakpoint {
public:
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    JSBreakpoint(JSMethod *method, uint32_t bcOffset, const std::optional<CString> &condition = {})
        : method_(method), bcOffset_(bcOffset), condition_(condition) {}
    ~JSBreakpoint() = default;

    JSMethod *GetMethod() const
    {
        return method_;
    }

    uint32_t GetBytecodeOffset() const
    {
        return bcOffset_;
    }

    bool operator==(const JSBreakpoint &bpoint) const
    {
        return GetMethod() == bpoint.GetMethod() && GetBytecodeOffset() == bpoint.GetBytecodeOffset();
    }

    const CString &GetCondition() const
    {
        return condition_.value();
    }

    bool HasCondition() const
    {
        return condition_.has_value();
    }

    DEFAULT_COPY_SEMANTIC(JSBreakpoint);
    DEFAULT_MOVE_SEMANTIC(JSBreakpoint);

private:
    JSMethod *method_;
    uint32_t bcOffset_;
    std::optional<CString> condition_;
};

class HashJSBreakpoint {
public:
    size_t operator()(const JSBreakpoint &bpoint) const
    {
        return (std::hash<JSMethod *>()(bpoint.GetMethod())) ^ (std::hash<uint32_t>()(bpoint.GetBytecodeOffset()));
    }
};

class JSDebugger : public JSDebugInterface, RuntimeListener {
public:
    explicit JSDebugger(const EcmaVM *vm) : ecmaVm_(vm)
    {
        auto notificationMgr = ecmaVm_->GetNotificationManager();
        if (notificationMgr != nullptr) {
            notificationMgr->AddListener(this);
        }
    }
    ~JSDebugger() override
    {
        auto notificationMgr = ecmaVm_->GetNotificationManager();
        if (notificationMgr != nullptr) {
            notificationMgr->RemoveListener();
        }
    }

    void RegisterHooks(PtHooks *hooks) override
    {
        hooks_ = hooks;
    }
    void UnregisterHooks() override
    {
        hooks_ = nullptr;
    }

    void Init();

    bool SetBreakpoint(const JSPtLocation &location, const std::optional<CString> &condition) override;
    bool RemoveBreakpoint(const JSPtLocation &location) override;
    void BytecodePcChanged(JSThread *thread, JSMethod *method, uint32_t bcOffset) override;
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
    void VmDeath() override
    {
        if (hooks_ == nullptr) {
            return;
        }
        hooks_->VmDeath();
    }

private:
    JSMethod *FindMethod(const JSPtLocation &location) const;
    std::optional<JSBreakpoint> FindBreakpoint(const JSMethod *method, uint32_t bcOffset) const;
    bool RemoveBreakpoint(const JSMethod *method, uint32_t bcOffset);
    void HandleExceptionThrowEvent(const JSThread *thread, const JSMethod *method, uint32_t bcOffset);
    bool HandleStep(const JSMethod *method, uint32_t bcOffset);
    bool HandleBreakpoint(const JSMethod *method, uint32_t bcOffset);
    void SetGlobalFunction(const JSHandle<JSTaggedValue> &funcName, EcmaEntrypoint nativeFunc, int32_t numArgs) const;

    static void PrepareEvaluateEnv(const EcmaVM *ecmaVm, InterpretedFrameHandler &frameHandler);
    static JSTaggedValue DebuggerSetValue(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue DebuggerGetValue(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetGlobalValue(const EcmaVM *ecmaVm, JSTaggedValue key);
    static JSTaggedValue SetGlobalValue(const EcmaVM *ecmaVm, JSTaggedValue key, JSTaggedValue value);
    static bool EvaluateLocalValue(JSMethod *method, JSThread *thread, const CString &varName, int32_t &regIndex);

    const EcmaVM *ecmaVm_;
    PtHooks *hooks_ {nullptr};

    CUnorderedSet<JSBreakpoint, HashJSBreakpoint> breakpoints_ {};
};
}  // namespace panda::ecmascript::tooling

#endif  // ECMASCRIPT_TOOLING_JS_DEBUGGER_H
