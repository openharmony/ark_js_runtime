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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/tooling/protocol_handler.h"

namespace panda::ecmascript::tooling {
void InitializeDebugger(const std::function<void(const std::string &)> &onResponse, ::panda::ecmascript::EcmaVM *vm)
{
    ProtocolHandler *handler = vm->GetJsDebuggerManager()->GetDebuggerHandler();
    if (handler != nullptr) {
        LOG(FATAL, DEBUGGER) << "JS debugger was initialized";
    }
    vm->GetJsDebuggerManager()->SetDebuggerHandler(new ProtocolHandler(onResponse, vm));
}

void UninitializeDebugger(::panda::ecmascript::EcmaVM *vm)
{
    ProtocolHandler *handler = vm->GetJsDebuggerManager()->GetDebuggerHandler();
    delete handler;
    vm->GetJsDebuggerManager()->SetDebuggerHandler(nullptr);
}

void DispatchProtocolMessage(const ::panda::ecmascript::EcmaVM *vm, const std::string &message)
{
    ProtocolHandler *handler = vm->GetJsDebuggerManager()->GetDebuggerHandler();
    if (handler != nullptr) {
        handler->ProcessCommand(message.c_str());
    }
}
}  // namespace panda::ecmascript::tooling