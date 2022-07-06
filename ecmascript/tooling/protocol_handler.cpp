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
    ProcessCommand();
}

void ProtocolHandler::RunIfWaitingForDebugger()
{
    waitingForDebugger_ = false;
}

void ProtocolHandler::DispatchCommand(std::string &&msg)
{
    std::unique_lock<std::mutex> queueLock(requestLock_);
    requestQueue_.push(std::move(msg));
    requestQueueCond_.notify_one();
}

// called after DispatchCommand
int32_t ProtocolHandler::GetDispatchStatus()
{
    if (isDispatchingMessage_ || waitingForDebugger_) {
        return DispatchStatus::DISPATCHING;
    }
    std::unique_lock<std::mutex> queueLock(requestLock_);
    if (requestQueue_.empty()) {
        return DispatchStatus::DISPATCHED;
    }
    return DispatchStatus::UNKNOWN;
}

void ProtocolHandler::ProcessCommand()
{
    std::queue<std::string> dispatchingQueue;
    do {
        {
            std::unique_lock<std::mutex> queueLock(requestLock_);
            if (requestQueue_.empty()) {
                if (!waitingForDebugger_) {
                    return;
                }
                requestQueueCond_.wait(queueLock);
            }
            requestQueue_.swap(dispatchingQueue);
        }

        isDispatchingMessage_ = true;
        while (!dispatchingQueue.empty()) {
            std::string msg = std::move(dispatchingQueue.front());
            dispatchingQueue.pop();

            [[maybe_unused]] LocalScope scope(vm_);
            auto exception = DebuggerApi::GetAndClearException(vm_);
            dispatcher_.Dispatch(DispatchRequest(msg));
            DebuggerApi::SetException(vm_, exception);
        }
        isDispatchingMessage_ = false;
    } while (true);
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

    callback_(reinterpret_cast<const void *>(vm_), str);
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
