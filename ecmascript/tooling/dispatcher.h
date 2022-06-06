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

#ifndef ECMASCRIPT_TOOLING_DISPATCHER_H
#define ECMASCRIPT_TOOLING_DISPATCHER_H

#include <map>
#include <memory>

#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/tooling/backend/js_debugger_interface.h"
#include "ecmascript/tooling/base/pt_returns.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::tooling {
class ProtocolChannel;
class PtBaseReturns;
class PtBaseEvents;

enum class RequestCode : uint8_t {
    OK = 0,
    NOK,

    // Json parse errors
    JSON_PARSE_ERROR,
    PARSE_ID_ERROR,
    ID_FORMAT_ERROR,
    PARSE_METHOD_ERROR,
    METHOD_FORMAT_ERROR,
    PARSE_PARAMS_ERROR,
    PARAMS_FORMAT_ERROR
};

enum class ResponseCode : uint8_t { OK, NOK };

class DispatchRequest {
public:
    explicit DispatchRequest(const EcmaVM *ecmaVm, const std::string &message);

    bool IsValid() const
    {
        return code_ == RequestCode::OK;
    }
    int32_t GetCallId() const
    {
        return callId_;
    }
    Local<JSValueRef> GetParams() const
    {
        return params_;
    }
    const std::string &GetDomain() const
    {
        return domain_;
    }
    const std::string &GetMethod() const
    {
        return method_;
    }
    const EcmaVM *GetEcmaVM() const
    {
        return ecmaVm_;
    }

    ~DispatchRequest() = default;

private:
    const EcmaVM *ecmaVm_ {nullptr};
    int32_t callId_ {-1};
    std::string domain_ {};
    std::string method_ {};
    Local<JSValueRef> params_ {};
    RequestCode code_ {RequestCode::OK};
    std::string errorMsg_ {};
};

class DispatchResponse {
public:
    bool IsOk() const
    {
        return code_ == ResponseCode::OK;
    }

    ResponseCode GetError() const
    {
        return code_;
    }

    const std::string &GetMessage() const
    {
        return errorMsg_;
    }

    static DispatchResponse Create(ResponseCode code, const std::string &msg = "");
    static DispatchResponse Create(std::optional<std::string> error);
    static DispatchResponse Ok();
    static DispatchResponse Fail(const std::string &message);

    ~DispatchResponse() = default;

private:
    DispatchResponse() = default;

    ResponseCode code_ {ResponseCode::OK};
    std::string errorMsg_ {};
};

class DispatcherBase {
public:
    explicit DispatcherBase(ProtocolChannel *channel) : channel_(channel) {}
    virtual ~DispatcherBase()
    {
        channel_ = nullptr;
    };
    virtual void Dispatch(const DispatchRequest &request) = 0;

protected:
    void SendResponse(const DispatchRequest &request, const DispatchResponse &response,
                      const PtBaseReturns &result = PtBaseReturns());

private:
    ProtocolChannel *channel_ {nullptr};

    NO_COPY_SEMANTIC(DispatcherBase);
    NO_MOVE_SEMANTIC(DispatcherBase);
};

class Dispatcher {
public:
    explicit Dispatcher(const EcmaVM *vm, ProtocolChannel *channel);
    ~Dispatcher() = default;
    void Dispatch(const DispatchRequest &request);

private:
    std::unordered_map<std::string, std::unique_ptr<DispatcherBase>> dispatchers_ {};

    NO_COPY_SEMANTIC(Dispatcher);
    NO_MOVE_SEMANTIC(Dispatcher);
};
}  // namespace panda::ecmascript::tooling
#endif
