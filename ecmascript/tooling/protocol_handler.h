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

#ifndef ECMASCRIPT_TOOLING_PROTOCOL_HANDLER_H
#define ECMASCRIPT_TOOLING_PROTOCOL_HANDLER_H

#include <functional>
#include <queue>
#include <memory>

#include "ecmascript/tooling/front_end.h"

namespace panda::tooling::ecmascript {
class ProtocolHandler final : public FrontEnd {
public:
    explicit ProtocolHandler(std::function<void(std::string)> callback, const EcmaVM *vm);
    ~ProtocolHandler() override = default;

    void WaitForDebugger(const EcmaVM *ecmaVm) override;
    void RunIfWaitingForDebugger() override;
    void ProcessCommand(const EcmaVM *ecmaVm) override;
    void SendCommand(const CString &msg);
    void SendResponse(const DispatchRequest &request, const DispatchResponse &response,
                      std::unique_ptr<PtBaseReturns> result) override;
    void SendNotification(const EcmaVM *ecmaVm, std::unique_ptr<PtBaseEvents> events) override;
    const EcmaVM *GetEcmaVM() const
    {
        return vm_;
    }

private:
    NO_MOVE_SEMANTIC(ProtocolHandler);
    NO_COPY_SEMANTIC(ProtocolHandler);
    Local<ObjectRef> CreateErrorReply(const EcmaVM *ecmaVm, const DispatchResponse &response);
    void SendReply(const EcmaVM *ecmaVm, Local<ObjectRef> reply);

    std::function<void(std::string)> callback_;
    std::unique_ptr<Dispatcher> dispatcher_ {};

    bool waitingForDebugger_ {false};
    CQueue<CString> msgQueue_ {};
    os::memory::Mutex queueLock_;
    os::memory::ConditionVariable queueCond_ GUARDED_BY(queueLock_);
    const EcmaVM *vm_ {nullptr};
};
}  // namespace panda::tooling::ecmascript

#endif
