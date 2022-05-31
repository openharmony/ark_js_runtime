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

#ifndef ECMASCRIPT_TOOLING_PROTOCOL_CHANNEL_H
#define ECMASCRIPT_TOOLING_PROTOCOL_CHANNEL_H

#include "ecmascript/tooling/base/pt_events.h"
#include "ecmascript/tooling/base/pt_returns.h"
#include "ecmascript/tooling/dispatcher.h"

#include "libpandabase/macros.h"

namespace panda::ecmascript::tooling {
class ProtocolChannel {
public:
    ProtocolChannel() = default;
    virtual ~ProtocolChannel() = default;

    virtual void WaitForDebugger() = 0;
    virtual void RunIfWaitingForDebugger() = 0;
    virtual void SendResponse(const DispatchRequest &request, const DispatchResponse &response,
                              const PtBaseReturns &result) = 0;
    virtual void SendNotification(const PtBaseEvents &events) = 0;

private:
    NO_COPY_SEMANTIC(ProtocolChannel);
    NO_MOVE_SEMANTIC(ProtocolChannel);
};
}  // namespace panda::ecmascript::tooling

#endif  // ECMASCRIPT_TOOLING_PROTOCOL_CHANNEL_H