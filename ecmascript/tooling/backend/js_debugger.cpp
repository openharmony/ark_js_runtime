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

#include "ecmascript/tooling/backend/js_debugger.h"

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"

namespace panda::ecmascript::tooling {
using panda::ecmascript::base::BuiltinsBase;

bool JSDebugger::SetBreakpoint(const JSPtLocation &location, const Local<FunctionRef> &condFuncRef)
{
    JSMethod *method = FindMethod(location);
    if (method == nullptr) {
        LOG(ERROR, DEBUGGER) << "SetBreakpoint: Cannot find JSMethod";
        return false;
    }

    if (location.GetBytecodeOffset() >= method->GetCodeSize()) {
        LOG(ERROR, DEBUGGER) << "SetBreakpoint: Invalid breakpoint location";
        return false;
    }

    auto [_, success] = breakpoints_.emplace(method, location.GetBytecodeOffset(),
        Global<FunctionRef>(ecmaVm_, condFuncRef));
    if (!success) {
        // also return true
        LOG(WARNING, DEBUGGER) << "SetBreakpoint: Breakpoint already exists";
    }

    return true;
}

bool JSDebugger::RemoveBreakpoint(const JSPtLocation &location)
{
    JSMethod *method = FindMethod(location);
    if (method == nullptr) {
        LOG(ERROR, DEBUGGER) << "RemoveBreakpoint: Cannot find JSMethod";
        return false;
    }

    if (!RemoveBreakpoint(method, location.GetBytecodeOffset())) {
        LOG(ERROR, DEBUGGER) << "RemoveBreakpoint: Breakpoint not found";
        return false;
    }

    return true;
}

void JSDebugger::BytecodePcChanged(JSThread *thread, JSMethod *method, uint32_t bcOffset)
{
    ASSERT(bcOffset < method->GetCodeSize() && "code size of current JSMethod less then bcOffset");

    HandleExceptionThrowEvent(thread, method, bcOffset);

    // Step event is reported before breakpoint, according to the spec.
    if (!HandleStep(method, bcOffset)) {
        HandleBreakpoint(method, bcOffset);
    }
}

bool JSDebugger::HandleBreakpoint(const JSMethod *method, uint32_t bcOffset)
{
    auto breakpoint = FindBreakpoint(method, bcOffset);
    if (hooks_ == nullptr || !breakpoint.has_value()) {
        return false;
    }

    JSThread *thread = ecmaVm_->GetJSThread();
    auto condFuncRef = breakpoint.value().GetConditionFunction();
    if (condFuncRef->IsFunction()) {
        LOG(INFO, DEBUGGER) << "HandleBreakpoint: begin evaluate condition";
        auto handlerPtr = std::make_shared<FrameHandler>(ecmaVm_->GetJSThread());
        auto res = DebuggerApi::EvaluateViaFuncCall(const_cast<EcmaVM *>(ecmaVm_),
            condFuncRef.ToLocal(ecmaVm_), handlerPtr);
        if (thread->HasPendingException()) {
            LOG(ERROR, DEBUGGER) << "HandleBreakpoint: has pending exception";
            thread->ClearException();
            return false;
        }
        bool isMeet = res->ToBoolean(ecmaVm_)->Value();
        if (!isMeet) {
            LOG(ERROR, DEBUGGER) << "HandleBreakpoint: condition not meet";
            return false;
        }
    }

    auto *pf = method->GetJSPandaFile();
    JSPtLocation location {pf->GetJSPandaFileDesc().c_str(), method->GetMethodId(), bcOffset};

    hooks_->Breakpoint(location);
    return true;
}

void JSDebugger::HandleExceptionThrowEvent(const JSThread *thread, const JSMethod *method, uint32_t bcOffset)
{
    if (hooks_ == nullptr || !thread->HasPendingException()) {
        return;
    }

    auto *pf = method->GetJSPandaFile();
    JSPtLocation throwLocation {pf->GetJSPandaFileDesc().c_str(), method->GetMethodId(), bcOffset};

    hooks_->Exception(throwLocation);
}

bool JSDebugger::HandleStep(const JSMethod *method, uint32_t bcOffset)
{
    if (hooks_ == nullptr) {
        return false;
    }

    auto *pf = method->GetJSPandaFile();
    JSPtLocation location {pf->GetJSPandaFileDesc().c_str(), method->GetMethodId(), bcOffset};

    return hooks_->SingleStep(location);
}

std::optional<JSBreakpoint> JSDebugger::FindBreakpoint(const JSMethod *method, uint32_t bcOffset) const
{
    for (const auto &bp : breakpoints_) {
        if (bp.GetBytecodeOffset() == bcOffset &&
            bp.GetMethod()->GetJSPandaFile()->GetJSPandaFileDesc() == method->GetJSPandaFile()->GetJSPandaFileDesc() &&
            bp.GetMethod()->GetMethodId() == method->GetMethodId()) {
            return bp;
        }
    }
    return {};
}

bool JSDebugger::RemoveBreakpoint(const JSMethod *method, uint32_t bcOffset)
{
    for (auto it = breakpoints_.begin(); it != breakpoints_.end(); ++it) {
        const auto &bp = *it;
        if (bp.GetBytecodeOffset() == bcOffset && bp.GetMethod() == method) {
            it = breakpoints_.erase(it);
            return true;
        }
    }

    return false;
}

JSMethod *JSDebugger::FindMethod(const JSPtLocation &location) const
{
    JSMethod *method = nullptr;
    ::panda::ecmascript::JSPandaFileManager::GetInstance()->EnumerateJSPandaFiles([&method, location](
        const panda::ecmascript::JSPandaFile *jsPandaFile) {
        if (jsPandaFile->GetJSPandaFileDesc() == location.GetPandaFile()) {
            JSMethod *methodsData = jsPandaFile->GetMethods();
            uint32_t numberMethods = jsPandaFile->GetNumMethods();
            for (uint32_t i = 0; i < numberMethods; ++i) {
                if (methodsData[i].GetMethodId() == location.GetMethodId()) {
                    method = methodsData + i;
                    return false;
                }
            }
        }
        return true;
    });
    return method;
}
}  // namespace panda::tooling::ecmascript
