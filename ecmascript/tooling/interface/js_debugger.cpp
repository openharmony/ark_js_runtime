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

#include "ecmascript/tooling/interface/js_debugger.h"

#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"

namespace panda::ecmascript::tooling {
bool JSDebugger::SetBreakpoint(const JSPtLocation &location)
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

    if (!breakpoints_.emplace(method, location.GetBytecodeOffset()).second) {
        LOG(ERROR, DEBUGGER) << "SetBreakpoint: Breakpoint already exists";
        return false;
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
    HandleStep(method, bcOffset);
    HandleBreakpoint(method, bcOffset);
}

bool JSDebugger::HandleBreakpoint(const JSMethod *method, uint32_t bcOffset)
{
    if (hooks_ == nullptr || !FindBreakpoint(method, bcOffset)) {
        return false;
    }

    auto *pf = method->GetPandaFile();
    JSPtLocation location {pf->GetFilename().c_str(), method->GetFileId(), bcOffset};

    hooks_->Breakpoint(location);
    return true;
}

void JSDebugger::HandleExceptionThrowEvent(const JSThread *thread, const JSMethod *method, uint32_t bcOffset)
{
    if (hooks_ == nullptr || !thread->HasPendingException()) {
        return;
    }

    auto *pf = method->GetPandaFile();
    JSPtLocation throwLocation {pf->GetFilename().c_str(), method->GetFileId(), bcOffset};

    hooks_->Exception(throwLocation);
}

bool JSDebugger::HandleStep(const JSMethod *method, uint32_t bcOffset)
{
    if (hooks_ == nullptr) {
        return false;
    }

    auto *pf = method->GetPandaFile();
    JSPtLocation location {pf->GetFilename().c_str(), method->GetFileId(), bcOffset};

    hooks_->SingleStep(location);
    return true;
}

bool JSDebugger::FindBreakpoint(const JSMethod *method, uint32_t bcOffset) const
{
    for (const auto &bp : breakpoints_) {
        if (bp.GetBytecodeOffset() == bcOffset && bp.GetMethod()->GetPandaFile()->GetFilename() ==
            method->GetPandaFile()->GetFilename() && bp.GetMethod()->GetFileId() == method->GetFileId()) {
            return true;
        }
    }

    return false;
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
                if (methodsData[i].GetFileId() == location.GetMethodId()) {
                    method = methodsData + i;
                    return false;
                }
            }
        }
        return true;
    });
    return method;
}
}  // namespace panda::ecmascript::tooling
