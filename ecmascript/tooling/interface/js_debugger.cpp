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
#include "runtime/tooling/pt_method_private.h"

namespace panda::tooling::ecmascript {
using panda::ecmascript::Program;

std::optional<Error> JSDebugger::SetBreakpoint(const PtLocation &location)
{
    JSMethod *method = FindMethod(location);
    if (method == nullptr) {
        return Error(Error::Type::METHOD_NOT_FOUND,
                     std::string("Cannot find JSMethod with id ") + std::to_string(location.GetMethodId().GetOffset()) +
                         " in panda file '" + std::string(location.GetPandaFile()) + "'");
    }

    if (location.GetBytecodeOffset() >= method->GetCodeSize()) {
        return Error(Error::Type::INVALID_BREAKPOINT, std::string("Invalid breakpoint location: bytecode offset (") +
                                                          std::to_string(location.GetBytecodeOffset()) +
                                                          ") >= JSMethod code size (" +
                                                          std::to_string(method->GetCodeSize()) + ")");
    }

    if (!breakpoints_.emplace(method, location.GetBytecodeOffset()).second) {
        return Error(Error::Type::BREAKPOINT_ALREADY_EXISTS,
                     std::string("Breakpoint already exists: bytecode offset ") +
                         std::to_string(location.GetBytecodeOffset()));
    }

    return {};
}

std::optional<Error> JSDebugger::RemoveBreakpoint(const PtLocation &location)
{
    JSMethod *method = FindMethod(location);
    if (method == nullptr) {
        return Error(Error::Type::METHOD_NOT_FOUND,
                     std::string("Cannot find JSMethod with id ") + std::to_string(location.GetMethodId().GetOffset()) +
                         " in panda file '" + std::string(location.GetPandaFile()) + "'");
    }

    if (!RemoveBreakpoint(method, location.GetBytecodeOffset())) {
        return Error(Error::Type::BREAKPOINT_NOT_FOUND, "Breakpoint not found");
    }

    return {};
}

void JSDebugger::BytecodePcChanged(ManagedThread *thread, Method *method, uint32_t bcOffset)
{
    ASSERT(bcOffset < method->GetCodeSize() && "code size of current JSMethod less then bcOffset");

    HandleExceptionThrowEvent(JSThread::Cast(thread), JSMethod::Cast(method), bcOffset);

    // Step event is reported before breakpoint, according to the spec.
    HandleStep(JSThread::Cast(thread), JSMethod::Cast(method), bcOffset);
    HandleBreakpoint(JSThread::Cast(thread), JSMethod::Cast(method), bcOffset);
}

bool JSDebugger::HandleBreakpoint(const JSThread *thread, const JSMethod *method, uint32_t bcOffset)
{
    if (hooks_ == nullptr || !FindBreakpoint(method, bcOffset)) {
        return false;
    }

    auto *pf = method->GetPandaFile();
    PtLocation location {pf->GetFilename().c_str(), method->GetFileId(), bcOffset};

    hooks_->Breakpoint(PtThread(thread->GetId()), location);
    return true;
}

void JSDebugger::HandleExceptionThrowEvent(const JSThread *thread, const JSMethod *method, uint32_t bcOffset)
{
    if (hooks_ == nullptr || !thread->HasPendingException()) {
        return;
    }

    auto *pf = method->GetPandaFile();
    PtLocation throwLocation {pf->GetFilename().c_str(), method->GetFileId(), bcOffset};

    hooks_->Exception(PtThread(thread->GetId()), throwLocation, PtObject(), throwLocation);
}

bool JSDebugger::HandleStep(const JSThread *thread, const JSMethod *method, uint32_t bcOffset)
{
    if (hooks_ == nullptr) {
        return false;
    }

    auto *pf = method->GetPandaFile();
    PtLocation location {pf->GetFilename().c_str(), method->GetFileId(), bcOffset};

    hooks_->SingleStep(PtThread(thread->GetId()), location);
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

JSMethod *JSDebugger::FindMethod(const PtLocation &location) const
{
    JSMethod *method = nullptr;
    EcmaVM::GetJSPandaFileManager()->EnumerateJSPandaFiles([&method, location](
        const panda::ecmascript::JSPandaFile *jsPandaFile) {
        if (CString(location.GetPandaFile()) == jsPandaFile->GetJSPandaFileDesc()) {
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

void JSDebugger::MethodEntry(ManagedThread *thread, Method *method)
{
    if (hooks_ == nullptr) {
        return;
    }

    uint32_t threadId = thread->GetId();
    PtThread ptThread(threadId);
    hooks_->MethodEntry(ptThread, MethodToPtMethod(method));
}

void JSDebugger::MethodExit(ManagedThread *thread, Method *method)
{
    if (hooks_ == nullptr) {
        return;
    }
    bool isExceptionTriggered = thread->HasPendingException();
    PtThread ptThread(thread->GetId());
    auto sp = const_cast<JSTaggedType *>(JSThread::Cast(thread)->GetCurrentSPFrame());
    InterpretedFrameHandler frameHandler(sp);
    PtValue retValue(frameHandler.GetAcc().GetRawData());
    hooks_->MethodExit(ptThread, MethodToPtMethod(method), isExceptionTriggered, retValue);
}
}  // namespace panda::tooling::ecmascript
