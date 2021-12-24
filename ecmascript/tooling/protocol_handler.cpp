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

#include "ecmascript/tooling/protocol_handler.h"

#include "ecmascript/base/string_helper.h"
#include "ecmascript/tooling/agent/debugger_impl.h"
#include "utils/logger.h"

namespace panda::tooling::ecmascript {
ProtocolHandler::ProtocolHandler(std::function<void(std::string)> callback, const EcmaVM *vm)
    : callback_(std::move(callback)), vm_(vm)
{
    dispatcher_ = std::make_unique<Dispatcher>(this);
}

void ProtocolHandler::WaitForDebugger(const EcmaVM *ecmaVm)
{
    waitingForDebugger_ = true;
    ProcessCommand(ecmaVm);
}

void ProtocolHandler::RunIfWaitingForDebugger()
{
    waitingForDebugger_ = false;
}

void ProtocolHandler::ProcessCommand(const EcmaVM *ecmaVm)
{
    static constexpr int DEBUGGER_WAIT_SLEEP_TIME = 100;
    while (waitingForDebugger_) {
        usleep(DEBUGGER_WAIT_SLEEP_TIME);
    }
}

void ProtocolHandler::SendCommand(const CString &msg)
{
    LOG(DEBUG, DEBUGGER) << "ProtocolHandler::SendCommand: " << msg;
    Local<JSValueRef> exception = DebuggerApi::GetException(vm_);
    if (!exception->IsHole()) {
        DebuggerApi::ClearException(vm_);
    }
    dispatcher_->Dispatch(DispatchRequest(vm_, msg));
    DebuggerApi::ClearException(vm_);
    if (!exception->IsHole()) {
        DebuggerApi::SetException(vm_, exception);
    }
    std::string startDebugging("Runtime.runIfWaitingForDebugger");
    if (msg.find(startDebugging, 0) != std::string::npos) {
        waitingForDebugger_ = false;
    }
}

void ProtocolHandler::SendResponse(const DispatchRequest &request, const DispatchResponse &response,
    std::unique_ptr<PtBaseReturns> result)
{
    LOG(INFO, DEBUGGER) << "ProtocolHandler::SendResponse: "
                        << (response.IsOk() ? "success" : "failed: " + response.GetMessage());
    const EcmaVM *ecmaVm = request.GetEcmaVM();

    Local<ObjectRef> reply = PtBaseTypes::NewObject(ecmaVm);
    reply->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "id"), IntegerRef::New(ecmaVm, request.GetCallId()));
    Local<ObjectRef> resultObj;
    if (response.IsOk() && result != nullptr) {
        resultObj = result->ToObject(ecmaVm);
    } else {
        resultObj = CreateErrorReply(ecmaVm, response);
    }
    reply->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "result"), Local<JSValueRef>(resultObj));
    SendReply(ecmaVm, reply);
}

void ProtocolHandler::SendNotification(const EcmaVM *ecmaVm, std::unique_ptr<PtBaseEvents> events)
{
        if (!Runtime::GetCurrent()->IsDebugMode() || events == nullptr) {
            return;
        }
        LOG(DEBUG, DEBUGGER) << "ProtocolHandler::SendNotification: " << events->GetName();
        SendReply(ecmaVm, events->ToObject(ecmaVm));
}

void ProtocolHandler::SendReply(const EcmaVM *ecmaVm, Local<ObjectRef> reply)
{
    Local<JSValueRef> str = JSON::Stringify(ecmaVm, reply);
    if (str->IsException()) {
        DebuggerApi::ClearException(ecmaVm);
        LOG(ERROR, DEBUGGER) << "json stringifier throw exception";
        return;
    }
    if (!str->IsString()) {
        LOG(ERROR, DEBUGGER) << "ProtocolHandler::SendReply: json stringify error";
        return;
    }

    callback_(StringRef::Cast(*str)->ToString());
}

Local<ObjectRef> ProtocolHandler::CreateErrorReply(const EcmaVM *ecmaVm, const DispatchResponse &response)
{
    Local<ObjectRef> result = PtBaseTypes::NewObject(ecmaVm);

    if (!response.IsOk()) {
        result->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "code")),
            IntegerRef::New(ecmaVm, static_cast<int32_t>(response.GetError())));
        result->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "message")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, response.GetMessage().c_str())));
    }

    return result;
}
}  // namespace panda::tooling::ecmascript
