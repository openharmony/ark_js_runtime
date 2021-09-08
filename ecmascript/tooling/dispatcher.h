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

#include "libpandabase/macros.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/c_string.h"
#include "include/tooling/debug_interface.h"

namespace panda::tooling::ecmascript {
using panda::ecmascript::CMap;
using panda::ecmascript::CString;
class FrontEnd;
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
    explicit DispatchRequest(const EcmaVM *ecmaVm, const CString &message);

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
    CString GetDomain() const
    {
        return domain_;
    }
    CString GetMethod() const
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
    CString domain_ {};
    CString method_ {};
    Local<JSValueRef> params_ {};
    RequestCode code_ {RequestCode::OK};
    CString errorMsg_ {};
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

    CString GetMessage() const
    {
        return errorMsg_;
    }

    static DispatchResponse Create(ResponseCode code, const CString &msg = "");
    static DispatchResponse Create(std::optional<Error> error);
    static DispatchResponse Ok();
    static DispatchResponse Fail(const CString &message);

    ~DispatchResponse() = default;

private:
    DispatchResponse() = default;

    ResponseCode code_ {ResponseCode::OK};
    CString errorMsg_ {};
};

class DispatcherBase {
public:
    explicit DispatcherBase(FrontEnd *frontend) : frontend_(frontend) {}
    virtual ~DispatcherBase()
    {
        frontend_ = nullptr;
    };
    virtual void Dispatch(const DispatchRequest &request) = 0;

protected:
    void SendResponse(const DispatchRequest &request, const DispatchResponse &response,
                      std::unique_ptr<PtBaseReturns> result);

private:
    FrontEnd *frontend_ {nullptr};

    NO_COPY_SEMANTIC(DispatcherBase);
    NO_MOVE_SEMANTIC(DispatcherBase);
};

class Dispatcher {
public:
    explicit Dispatcher(FrontEnd *front);
    ~Dispatcher() = default;
    void Dispatch(const DispatchRequest &request);

private:
    CMap<CString, std::unique_ptr<DispatcherBase>> dispatchers_ {};

    NO_COPY_SEMANTIC(Dispatcher);
    NO_MOVE_SEMANTIC(Dispatcher);
};
}  // namespace panda::tooling::ecmascript
#endif
