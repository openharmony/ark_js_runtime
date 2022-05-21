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

namespace panda::ecmascript::tooling {
ProtocolHandler::ProtocolHandler(std::function<void(const std::string &)> callback, const EcmaVM *vm)
    : callback_(std::move(callback)), vm_(vm)
{
    dispatcher_ = std::make_unique<Dispatcher>(vm_, this);
}

void ProtocolHandler::WaitForDebugger()
{
    waitingForDebugger_ = true;
    static constexpr int DEBUGGER_WAIT_SLEEP_TIME = 100;
    while (waitingForDebugger_) {
        usleep(DEBUGGER_WAIT_SLEEP_TIME);
    }
}

void ProtocolHandler::RunIfWaitingForDebugger()
{
    waitingForDebugger_ = false;
}

void ProtocolHandler::ProcessCommand(const CString &msg)
{
    LOG(DEBUG, DEBUGGER) << "ProtocolHandler::ProcessCommand: " << msg;
    [[maybe_unused]] LocalScope scope(vm_);
    Local<JSValueRef> exception = DebuggerApi::GetAndClearException(vm_);
    dispatcher_->Dispatch(DispatchRequest(vm_, msg));
    DebuggerApi::SetException(vm_, exception);
}

void ProtocolHandler::SendResponse(const DispatchRequest &request, const DispatchResponse &response,
    std::unique_ptr<PtBaseReturns> result)
{
    LOG(INFO, DEBUGGER) << "ProtocolHandler::SendResponse: "
                        << (response.IsOk() ? "success" : "failed: " + response.GetMessage());

    Local<ObjectRef> reply = PtBaseTypes::NewObject(vm_);
    reply->Set(vm_, StringRef::NewFromUtf8(vm_, "id"), IntegerRef::New(vm_, request.GetCallId()));
    Local<ObjectRef> resultObj;
    if (response.IsOk() && result != nullptr) {
        resultObj = result->ToObject(vm_);
    } else {
        resultObj = CreateErrorReply(response);
    }
    reply->Set(vm_, StringRef::NewFromUtf8(vm_, "result"), Local<JSValueRef>(resultObj));
    SendReply(reply);
}

void ProtocolHandler::SendNotification(std::unique_ptr<PtBaseEvents> events)
{
    if (events == nullptr) {
        return;
    }
    LOG(DEBUG, DEBUGGER) << "ProtocolHandler::SendNotification: " << events->GetName();
    SendReply(events->ToObject(vm_));
}

void ProtocolHandler::SendReply(Local<ObjectRef> reply)
{
    Local<JSValueRef> str = JSON::Stringify(vm_, reply);
    if (str->IsException()) {
        DebuggerApi::ClearException(vm_);
        LOG(ERROR, DEBUGGER) << "json stringifier throw exception";
        return;
    }
    if (!str->IsString()) {
        LOG(ERROR, DEBUGGER) << "ProtocolHandler::SendReply: json stringify error";
        return;
    }

    callback_(StringRef::Cast(*str)->ToString());
}

Local<ObjectRef> ProtocolHandler::CreateErrorReply(const DispatchResponse &response)
{
    Local<ObjectRef> result = PtBaseTypes::NewObject(vm_);

    if (!response.IsOk()) {
        result->Set(vm_,
            Local<JSValueRef>(StringRef::NewFromUtf8(vm_, "code")),
            IntegerRef::New(vm_, static_cast<int32_t>(response.GetError())));
        result->Set(vm_,
            Local<JSValueRef>(StringRef::NewFromUtf8(vm_, "message")),
            Local<JSValueRef>(StringRef::NewFromUtf8(vm_, response.GetMessage().c_str())));
    }

    return result;
}
}  // namespace panda::ecmascript::tooling
