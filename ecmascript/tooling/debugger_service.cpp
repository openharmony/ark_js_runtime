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

#include "ecmascript/tooling/debugger_service.h"

#include "ecmascript/tooling/protocol_handler.h"

namespace panda::tooling::ecmascript {
static std::unique_ptr<ProtocolHandler> g_handler = nullptr;  // NOLINT(fuchsia-statically-constructed-objects)

void InitializeDebugger(const std::function<void(std::string)> &onResponse, const EcmaVM *vm)
{
    g_handler = std::make_unique<ProtocolHandler>(onResponse, vm);
}

void UninitializeDebugger()
{
    g_handler.reset();
}

void DispatchProtocolMessage(const std::string &message)
{
    if (g_handler != nullptr) {
        g_handler->ProcessCommand(message.c_str());
    }
}
}  // namespace panda::tooling::ecmascript