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

#include "ark_Execute_fuzzer.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "unistd.h"

using namespace panda;
using namespace panda::ecmascript;
bool createstatus = true;
namespace OHOS {
    // staic constexpr auto PANDA_MAIN_PATH = "pandastdlib/pandastdlib.bin";
    static constexpr auto PANDA_MAIN_FUNCTION = "_GLOBAL::func_main_0";

    bool doSomethingInterestingWithMyAPI(const uint8_t* data, size_t size)
    {
        RuntimeOption option;
        if (createstatus) {
            JSNApi::CreateJSVM(option);
            createstatus = false;
        }

        option.SetGcType(RuntimeOption::GC_TYPE::GEN_GC);
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        auto jsvm = JSNApi::CreateJSVM(option);
        Local<StringRef> entry = StringRef::NewFromUtf8(jsvm, PANDA_MAIN_FUNCTION);
        std::string a = entry->StringRef::ToString();
        JSNApi::Execute(jsvm, data, size, a);
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