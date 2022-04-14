/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "initializedebugger_fuzzer.h"
#include <cstddef>
#include <cstdint>
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/tooling/debugger_service.h"
#include "ecmascript/tooling/protocol_handler.h"
#include "unistd.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::tooling::ecmascript;

bool createstatus = true;
namespace OHOS {
    bool doSomethingInterestingWithMyAPI(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        if (createstatus) {
            JSNApi::CreateJSVM(option);
            createstatus = false;
        }
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        auto jsvm = JSNApi::CreateJSVM(option);
        auto onResponse = [data, size](std::string) {
            return;
        };
        panda::tooling::ecmascript::InitializeDebugger(onResponse, jsvm);
        panda::tooling::ecmascript::UninitializeDebugger();
        JSNApi::DestroyJSVM(jsvm);
        return true;
    }
}

// Fuzzer entry point.
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    // Run your code on data.
    OHOS::doSomethingInterestingWithMyAPI(data, size);
    return 0;
}