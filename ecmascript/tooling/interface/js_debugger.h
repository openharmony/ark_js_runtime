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
#include "ecmascript/tooling/interface/notification_manager.h"
#include "ecmascript/tooling/interface/js_debugger_manager.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/ecma_runtime_call_info.h"

namespace panda::ecmascript::tooling {
using panda::ecmascript::CUnorderedSet;
using panda::ecmascript::JSHandle;
using panda::ecmascript::JSTaggedValue;
using panda::ecmascript::EcmaEntrypoint;
using panda::ecmascript::EcmaRuntimeCallInfo;

class JSBreakpoint {
public:
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-member-init)
    JSBreakpoint(JSMethod *method, uint32_t bcOffset) : method_(method), bc_offset_(bcOffset) {}
    ~JSBreakpoint() = default;

    JSMethod *GetMethod() const
    {
        return method_;
    }

    uint32_t GetBytecodeOffset() const
    {
        return bc_offset_;
    }

    bool operator==(const JSBreakpoint &bpoint) const
    {
        return GetMethod() == bpoint.GetMethod() && GetBytecodeOffset() == bpoint.GetBytecodeOffset();
    }

    DEFAULT_COPY_SEMANTIC(JSBreakpoint);
    DEFAULT_MOVE_SEMANTIC(JSBreakpoint);

private:
    JSMethod *method_;
    uint32_t bc_offset_;
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
    JSDebugger(const EcmaVM *vm) : ecmaVm_(vm)
    {
        notificationMgr_ = ecmaVm_->GetJsDebuggerManager()->GetNotificationManager();
        notificationMgr_->AddListener(this);
    }
    ~JSDebugger() override
    {
        notificationMgr_->RemoveListener();
    }

    void RegisterHooks(PtHooks *hooks) override
    {
        hooks_ = hooks;
        // send vm start event after add hooks
        notificationMgr_->VmStartEvent();
    }
    void UnregisterHooks() override
    {
        // send vm death event before delete hooks
        notificationMgr_->VmDeathEvent();
        hooks_ = nullptr;
    }

    bool SetBreakpoint(const JSPtLocation &location) override;
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

    void Init();

private:
    using LocalEvalFunc =
        std::function<JSTaggedValue(const EcmaVM *, InterpretedFrameHandler *, int32_t, const CString &,
                      Local<JSValueRef>)>;
    using LexEvalFunc =
        std::function<JSTaggedValue(const EcmaVM *, int32_t, uint32_t, const CString &, Local<JSValueRef>)>;
    using GlobalEvalFunc =
        std::function<JSTaggedValue(const EcmaVM *, JSTaggedValue, const CString &, JSTaggedValue)>;

    JSMethod *FindMethod(const JSPtLocation  &location) const;
    bool FindBreakpoint(const JSMethod *method, uint32_t bcOffset) const;
    bool RemoveBreakpoint(const JSMethod *method, uint32_t bcOffset);
    void HandleExceptionThrowEvent(const JSThread *thread, const JSMethod *method, uint32_t bcOffset);
    bool HandleStep(const JSMethod *method, uint32_t bcOffset);
    bool HandleBreakpoint(const JSMethod *method, uint32_t bcOffset);

    void SetGlobalFunction(const JSHandle<JSTaggedValue> &funcName, EcmaEntrypoint nativeFunc, int32_t numArgs) const;
    static JSTaggedValue Evaluate(EcmaRuntimeCallInfo *argv, LocalEvalFunc localEvalFunc,
        LexEvalFunc lexEvalFunc, GlobalEvalFunc globalEvalFunc);
    static JSTaggedValue DebuggerSetValue(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue DebuggerGetValue(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetGlobalValue(const EcmaVM *ecmaVm, JSTaggedValue key);
    static JSTaggedValue SetGlobalValue(const EcmaVM *ecmaVm, JSTaggedValue key, JSTaggedValue value);
    static bool EvaluateLocalValue(JSMethod *method, JSThread *thread, const CString &varName, int32_t &regIndex);

    const EcmaVM *ecmaVm_;
    PtHooks *hooks_ {nullptr};
    NotificationManager *notificationMgr_ {nullptr};

    CUnorderedSet<JSBreakpoint, HashJSBreakpoint> breakpoints_ {};
};
}  // namespace panda::ecmascript::tooling

#endif  // ECMASCRIPT_TOOLING_JS_DEBUGGER_H
