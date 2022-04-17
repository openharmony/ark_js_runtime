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

#include "ecmascript/tooling/agent/profiler_impl.h"
#include "ecmascript/tooling/base/pt_returns.h"
#include "ecmascript/tooling/front_end.h"
#include "libpandabase/utils/logger.h"

namespace panda::ecmascript::tooling {
ProfilerImpl::DispatcherImpl::DispatcherImpl(FrontEnd *frontend, std::unique_ptr<ProfilerImpl> profiler)
    : DispatcherBase(frontend), profiler_(std::move(profiler))
{
    dispatcherTable_["disable"] = &ProfilerImpl::DispatcherImpl::Disable;
    dispatcherTable_["enable"] = &ProfilerImpl::DispatcherImpl::Enable;
    dispatcherTable_["start"] = &ProfilerImpl::DispatcherImpl::Start;
    dispatcherTable_["stop"] = &ProfilerImpl::DispatcherImpl::Stop;
}

void ProfilerImpl::DispatcherImpl::Dispatch(const DispatchRequest &request)
{
    CString method = request.GetMethod();
    LOG(DEBUG, DEBUGGER) << "dispatch [" << method << "] to ProfilerImpl";
    auto entry = dispatcherTable_.find(method);
    if (entry != dispatcherTable_.end() && entry->second != nullptr) {
        (this->*(entry->second))(request);
    } else {
        SendResponse(request, DispatchResponse::Fail("Unknown method: " + method), nullptr);
    }
}

void ProfilerImpl::DispatcherImpl::Disable(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->Disable();
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void ProfilerImpl::DispatcherImpl::Enable(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->Enable();
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void ProfilerImpl::DispatcherImpl::Start(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->Start();
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}

void ProfilerImpl::DispatcherImpl::Stop(const DispatchRequest &request)
{
    DispatchResponse response = profiler_->Stop();
    std::unique_ptr<PtBaseReturns> result = std::make_unique<PtBaseReturns>();
    SendResponse(request, response, std::move(result));
}


DispatchResponse ProfilerImpl::Disable()
{
    return DispatchResponse::Ok();
}

DispatchResponse ProfilerImpl::Enable()
{
    return DispatchResponse::Ok();
}

DispatchResponse ProfilerImpl::Start()
{
    return DispatchResponse::Ok();
}

DispatchResponse ProfilerImpl::Stop()
{
    return DispatchResponse::Ok();
}
}  // namespace panda::ecmascript::tooling

