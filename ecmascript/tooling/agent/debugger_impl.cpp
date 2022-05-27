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

#include "ecmascript/tooling/agent/debugger_impl.h"

#include "ecmascript/tooling/base/pt_events.h"
#include "ecmascript/tooling/base/pt_params.h"
#include "ecmascript/tooling/base/pt_returns.h"
#include "ecmascript/tooling/dispatcher.h"
#include "ecmascript/tooling/front_end.h"
#include "libpandabase/utils/logger.h"

namespace panda::ecmascript::tooling {
DebuggerImpl::DispatcherImpl::DispatcherImpl(FrontEnd *frontend, std::unique_ptr<DebuggerImpl> debugger)
    : DispatcherBase(frontend), debugger_(std::move(debugger))
{
    dispatcherTable_["enable"] = &DebuggerImpl::DispatcherImpl::Enable;
    dispatcherTable_["evaluateOnCallFrame"] = &DebuggerImpl::DispatcherImpl::EvaluateOnCallFrame;
    dispatcherTable_["getPossibleBreakpoints"] = &DebuggerImpl::DispatcherImpl::GetPossibleBreakpoints;
    dispatcherTable_["getScriptSource"] = &DebuggerImpl::DispatcherImpl::GetScriptSource;
    dispatcherTable_["pause"] = &DebuggerImpl::DispatcherImpl::Pause;
    dispatcherTable_["removeBreakpoint"] = &DebuggerImpl::DispatcherImpl::RemoveBreakpoint;
    dispatcherTable_["resume"] = &DebuggerImpl::DispatcherImpl::Resume;
    dispatcherTable_["setAsyncCallStackDepth"] = &DebuggerImpl::DispatcherImpl::SetAsyncCallStackDepth;
    dispatcherTable_["setBreakpointByUrl"] = &DebuggerImpl::DispatcherImpl::SetBreakpointByUrl;
    dispatcherTable_["setPauseOnExceptions"] = &DebuggerImpl::DispatcherImpl::SetPauseOnExceptions;
    dispatcherTable_["stepInto"] = &DebuggerImpl::DispatcherImpl::StepInto;
    dispatcherTable_["stepOut"] = &DebuggerImpl::DispatcherImpl::StepOut;
    dispatcherTable_["stepOver"] = &DebuggerImpl::DispatcherImpl::StepOver;
    dispatcherTable_["setBlackboxPatterns"] = &DebuggerImpl::DispatcherImpl::SetBlackboxPatterns;
}

void DebuggerImpl::DispatcherImpl::Dispatch(const DispatchRequest &request)
{
    CString method = request.GetMethod();
    LOG(DEBUG, DEBUGGER) << "dispatch [" << method << "] to DebuggerImpl";
    auto entry = dispatcherTable_.find(method);
    if (entry != dispatcherTable_.end() && entry->second != nullptr) {
        (this->*(entry->second))(request);
    } else {
        SendResponse(request, DispatchResponse::Fail("Unknown method: " + method), nullptr);
    }
}

void DebuggerImpl::DispatcherImpl::Enable(const DispatchRequest &request)
{
    std::unique_ptr<EnableParams> params = EnableParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("Debugger got wrong params"), nullptr);
        return;
    }

    UniqueDebuggerId id;
    DispatchResponse response = debugger_->Enable(std::move(params), &id);

    std::unique_ptr<PtBaseReturns> result = std::make_unique<EnableReturns>(id);
    SendResponse(request, response, std::move(result));
}

void DebuggerImpl::DispatcherImpl::EvaluateOnCallFrame(const DispatchRequest &request)
{
    std::unique_ptr<EvaluateOnCallFrameParams> params =
        EvaluateOnCallFrameParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("Debugger got wrong params"), nullptr);
        return;
    }
    std::unique_ptr<RemoteObject> result1 = std::make_unique<RemoteObject>();
    DispatchResponse response = debugger_->EvaluateOnCallFrame(std::move(params), &result1);

    std::unique_ptr<EvaluateOnCallFrameReturns> result =
        std::make_unique<EvaluateOnCallFrameReturns>(std::move(result1));
    SendResponse(request, response, std::move(result));
}

void DebuggerImpl::DispatcherImpl::GetPossibleBreakpoints(const DispatchRequest &request)
{
    std::unique_ptr<GetPossibleBreakpointsParams> params =
        GetPossibleBreakpointsParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("Debugger got wrong params"), nullptr);
        return;
    }
    CVector<std::unique_ptr<BreakLocation>> locations;
    DispatchResponse response = debugger_->GetPossibleBreakpoints(std::move(params), &locations);
    std::unique_ptr<GetPossibleBreakpointsReturns> points =
        std::make_unique<GetPossibleBreakpointsReturns>(std::move(locations));
    SendResponse(request, response, std::move(points));
}

void DebuggerImpl::DispatcherImpl::GetScriptSource(const DispatchRequest &request)
{
    std::unique_ptr<GetScriptSourceParams> params =
        GetScriptSourceParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("Debugger got wrong params"), nullptr);
        return;
    }
    CString source;
    DispatchResponse response = debugger_->GetScriptSource(std::move(params), &source);
    std::unique_ptr<GetScriptSourceReturns> result = std::make_unique<GetScriptSourceReturns>(std::move(source));
    SendResponse(request, response, std::move(result));
}

void DebuggerImpl::DispatcherImpl::Pause(const DispatchRequest &request)
{
    DispatchResponse response = debugger_->Pause();
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void DebuggerImpl::DispatcherImpl::RemoveBreakpoint(const DispatchRequest &request)
{
    std::unique_ptr<RemoveBreakpointParams> params =
        RemoveBreakpointParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("Debugger got wrong params"), nullptr);
        return;
    }
    DispatchResponse response = debugger_->RemoveBreakpoint(std::move(params));
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void DebuggerImpl::DispatcherImpl::Resume(const DispatchRequest &request)
{
    std::unique_ptr<ResumeParams> params = ResumeParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("Debugger got wrong params"), nullptr);
        return;
    }
    DispatchResponse response = debugger_->Resume(std::move(params));
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void DebuggerImpl::DispatcherImpl::SetAsyncCallStackDepth(const DispatchRequest &request)
{
    DispatchResponse response = debugger_->SetAsyncCallStackDepth();
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void DebuggerImpl::DispatcherImpl::SetBreakpointByUrl(const DispatchRequest &request)
{
    std::unique_ptr<SetBreakpointByUrlParams> params =
        SetBreakpointByUrlParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("Debugger got wrong params"), nullptr);
        return;
    }

    CString out_id;
    CVector<std::unique_ptr<Location>> outLocations;
    DispatchResponse response = debugger_->SetBreakpointByUrl(std::move(params), &out_id, &outLocations);
    std::unique_ptr<SetBreakpointByUrlReturns> result =
        std::make_unique<SetBreakpointByUrlReturns>(out_id, std::move(outLocations));
    SendResponse(request, response, std::move(result));
}

void DebuggerImpl::DispatcherImpl::SetPauseOnExceptions(const DispatchRequest &request)
{
    std::unique_ptr<SetPauseOnExceptionsParams> params =
        SetPauseOnExceptionsParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("Debugger got wrong params"), nullptr);
        return;
    }

    DispatchResponse response = debugger_->SetPauseOnExceptions(std::move(params));
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void DebuggerImpl::DispatcherImpl::StepInto(const DispatchRequest &request)
{
    std::unique_ptr<StepIntoParams> params = StepIntoParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("Debugger got wrong params"), nullptr);
        return;
    }
    DispatchResponse response = debugger_->StepInto(std::move(params));
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void DebuggerImpl::DispatcherImpl::StepOut(const DispatchRequest &request)
{
    DispatchResponse response = debugger_->StepOut();
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void DebuggerImpl::DispatcherImpl::StepOver(const DispatchRequest &request)
{
    std::unique_ptr<StepOverParams> params = StepOverParams::Create(request.GetEcmaVM(), request.GetParams());
    if (params == nullptr) {
        SendResponse(request, DispatchResponse::Fail("Debugger got wrong params"), nullptr);
        return;
    }
    DispatchResponse response = debugger_->StepOver(std::move(params));
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void DebuggerImpl::DispatcherImpl::SetBlackboxPatterns(const DispatchRequest &request)
{
    DispatchResponse response = debugger_->SetBlackboxPatterns();
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

DispatchResponse DebuggerImpl::Enable([[maybe_unused]] std::unique_ptr<EnableParams> params, UniqueDebuggerId *id)
{
    ASSERT(id != nullptr);
    *id = "0";
    return DispatchResponse::Ok();
}

DispatchResponse DebuggerImpl::EvaluateOnCallFrame(std::unique_ptr<EvaluateOnCallFrameParams> params,
                                                   std::unique_ptr<RemoteObject> *result)
{
    CString callFrameId = params->GetCallFrameId();
    CString expression = params->GetExpression();
    return DispatchResponse::Create(backend_->EvaluateValue(callFrameId, expression, result));
}

DispatchResponse DebuggerImpl::GetPossibleBreakpoints(std::unique_ptr<GetPossibleBreakpointsParams> params,
                                                      CVector<std::unique_ptr<BreakLocation>> *locations)
{
    Location *locationStart = params->GetStart();
    Location *locationEnd = params->GetEnd();

    return DispatchResponse::Create(backend_->GetPossibleBreakpoints(locationStart, locationEnd, locations));
}

DispatchResponse DebuggerImpl::GetScriptSource(std::unique_ptr<GetScriptSourceParams> params, CString *source)
{
    auto scriptFunc = [source](PtScript *script) -> bool {
        *source = script->GetScriptSource();
        return true;
    };
    if (!backend_->MatchScripts(scriptFunc, params->GetScriptId(), ScriptMatchType::SCRIPT_ID)) {
        *source = "";
        return DispatchResponse::Fail("unknown script id: " + params->GetScriptId());
    }

    return DispatchResponse::Ok();
}

DispatchResponse DebuggerImpl::Pause()
{
    return DispatchResponse::Create(backend_->Pause());
}

DispatchResponse DebuggerImpl::RemoveBreakpoint(std::unique_ptr<RemoveBreakpointParams> params)
{
    CString id = params->GetBreakpointId();
    LOG(INFO, DEBUGGER) << "RemoveBreakpoint: " << id;
    BreakpointDetails metaData{};
    if (!BreakpointDetails::ParseBreakpointId(id, &metaData)) {
        return DispatchResponse::Fail("Parse breakpoint id failed");
    }
    return DispatchResponse::Create(backend_->RemoveBreakpoint(metaData));
}

DispatchResponse DebuggerImpl::Resume([[maybe_unused]] std::unique_ptr<ResumeParams> params)
{
    return DispatchResponse::Create(backend_->Resume());
}

DispatchResponse DebuggerImpl::SetAsyncCallStackDepth()
{
    return DispatchResponse::Ok();
}

DispatchResponse DebuggerImpl::SetBreakpointByUrl(std::unique_ptr<SetBreakpointByUrlParams> params,
                                                  CString *outId,
                                                  CVector<std::unique_ptr<Location>> *outLocations)
{
    return DispatchResponse::Create(
        backend_->SetBreakpointByUrl(params->GetUrl(), params->GetLine(), params->GetColumn(),
        (params->HasCondition() ? params->GetCondition() : std::optional<CString> {}), outId, outLocations));
}

DispatchResponse DebuggerImpl::SetPauseOnExceptions(std::unique_ptr<SetPauseOnExceptionsParams> params)
{
    PauseOnExceptionsState state = params->GetState();
    if (state == PauseOnExceptionsState::UNCAUGHT) {
        backend_->SetPauseOnException(false);
    } else {
        backend_->SetPauseOnException(true);
    }
    return DispatchResponse::Ok();
}

DispatchResponse DebuggerImpl::StepInto([[maybe_unused]] std::unique_ptr<StepIntoParams> params)
{
    return DispatchResponse::Create(backend_->StepInto());
}

DispatchResponse DebuggerImpl::StepOut()
{
    return DispatchResponse::Create(backend_->StepOut());
}

DispatchResponse DebuggerImpl::StepOver([[maybe_unused]] std::unique_ptr<StepOverParams> params)
{
    return DispatchResponse::Create(backend_->StepOver());
}

DispatchResponse DebuggerImpl::SetBlackboxPatterns()
{
    return DispatchResponse::Ok();
}
}  // namespace panda::ecmascript::tooling
