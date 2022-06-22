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

#include "ecmascript/tooling/agent/debugger_impl.h"
#include "utils/logger.h"

namespace panda::ecmascript::tooling {
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

void ProtocolHandler::ProcessCommand(const std::string &msg)
{
    LOG(DEBUG, DEBUGGER) << "ProtocolHandler::ProcessCommand: " << msg;
    [[maybe_unused]] LocalScope scope(vm_);
    Local<JSValueRef> exception = DebuggerApi::GetAndClearException(vm_);
    dispatcher_.Dispatch(DispatchRequest(msg));
    DebuggerApi::SetException(vm_, exception);
}

void ProtocolHandler::SendResponse(const DispatchRequest &request, const DispatchResponse &response,
    const PtBaseReturns &result)
{
    LOG(INFO, DEBUGGER) << "ProtocolHandler::SendResponse: "
                        << (response.IsOk() ? "success" : "failed: " + response.GetMessage());

    std::unique_ptr<PtJson> reply = PtJson::CreateObject();
    reply->Add("id", request.GetCallId());
    std::unique_ptr<PtJson> resultObj;
    if (response.IsOk()) {
        resultObj = result.ToJson();
    } else {
        resultObj = CreateErrorReply(response);
    }
    reply->Add("result", resultObj);
    SendReply(*reply);
}

void ProtocolHandler::SendNotification(const PtBaseEvents &events)
{
    LOG(DEBUG, DEBUGGER) << "ProtocolHandler::SendNotification: " << events.GetName();
    SendReply(*events.ToJson());
}

void ProtocolHandler::SendReply(const PtJson &reply)
{
    std::string str = reply.Stringify();
    if (str.empty()) {
        LOG(ERROR, DEBUGGER) << "ProtocolHandler::SendReply: json stringify error";
        return;
    }

    callback_(str);
}

std::unique_ptr<PtJson> ProtocolHandler::CreateErrorReply(const DispatchResponse &response)
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    if (!response.IsOk()) {
        result->Add("code", static_cast<int32_t>(response.GetError()));
        result->Add("message", response.GetMessage().c_str());
    }

    return result;
}
}  // namespace panda::ecmascript::tooling
