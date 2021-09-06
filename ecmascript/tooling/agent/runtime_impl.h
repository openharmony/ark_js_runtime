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

#ifndef PANDA_TOOLING_ECMASCRIPT_RUNTIME_IMPL_H
#define PANDA_TOOLING_ECMASCRIPT_RUNTIME_IMPL_H

#include "libpandabase/macros.h"
#include "ecmascript/tooling/agent/js_backend.h"
#include "ecmascript/tooling/base/pt_params.h"
#include "ecmascript/tooling/dispatcher.h"

namespace panda::tooling::ecmascript {
using panda::ecmascript::CString;

class RuntimeImpl final {
public:
    explicit RuntimeImpl(JSBackend *backend) : backend_(backend) {}
    ~RuntimeImpl() = default;

    DispatchResponse Enable();
    DispatchResponse RunIfWaitingForDebugger();
    DispatchResponse GetProperties(
        std::unique_ptr<GetPropertiesParams> params,
        CVector<std::unique_ptr<PropertyDescriptor>> *outPropertyDesc,
        std::optional<CVector<std::unique_ptr<InternalPropertyDescriptor>>> *outInternalDescs,
        std::optional<CVector<std::unique_ptr<PrivatePropertyDescriptor>>> *outPrivateProps,
        std::optional<std::unique_ptr<ExceptionDetails>> *outExceptionDetails);

    class DispatcherImpl final : public DispatcherBase {
    public:
        DispatcherImpl(FrontEnd *frontend, std::unique_ptr<RuntimeImpl> runtime);
        ~DispatcherImpl() override = default;

        void Dispatch(const DispatchRequest &request) override;
        void Enable(const DispatchRequest &request);
        void RunIfWaitingForDebugger(const DispatchRequest &request);
        void GetProperties(const DispatchRequest &request);

    private:
        using AgentHandler = void (RuntimeImpl::DispatcherImpl::*)(const DispatchRequest &request);
        CMap<CString, AgentHandler> dispatcherTable_ {};
        std::unique_ptr<RuntimeImpl> runtime_ {};

        NO_COPY_SEMANTIC(DispatcherImpl);
        NO_MOVE_SEMANTIC(DispatcherImpl);
    };

private:
    NO_COPY_SEMANTIC(RuntimeImpl);
    NO_MOVE_SEMANTIC(RuntimeImpl);

    JSBackend *backend_{nullptr};
};
}  // namespace panda::tooling::ecmascript
#endif