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

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <memory>

// those head files is in ace_engine
#include "ark_js_runtime.h"
#include "js_value.h"

using OHOS::Ace::Framework::ArkJSRuntime;
using OHOS::Ace::Framework::JsRuntime;
using OHOS::Ace::Framework::JsValue;
using OHOS::Ace::Framework::RegisterFunctionType;
using std::shared_ptr;

std::string GetLogContent(const shared_ptr<JsRuntime> &runtime, const std::vector<shared_ptr<JsValue>> &argument)
{
    std::string context;
    for (const auto &value : argument) {
        context += value->ToString(runtime);
    }
    return context;
}

shared_ptr<JsValue> AppDebugLogPrint(const shared_ptr<JsRuntime> &runtime, const shared_ptr<JsValue> &,
                                     const std::vector<shared_ptr<JsValue>> &argument, int32_t)
{
    std::string context = GetLogContent(runtime, argument);
    std::cout << context.c_str() << std::endl;
    return runtime->NewObject();
}

shared_ptr<JsValue> Setter(const shared_ptr<JsRuntime> &runtime, const shared_ptr<JsValue> &,
                           const std::vector<shared_ptr<JsValue>> &argument, int32_t)
{
    std::string context = GetLogContent(runtime, argument);
    std::cout << context.c_str() << std::endl;
    return runtime->NewObject();
}

shared_ptr<JsValue> Getter(const shared_ptr<JsRuntime> &runtime, const shared_ptr<JsValue> &,
                           const std::vector<shared_ptr<JsValue>> &argument, int32_t)
{
    std::string context = GetLogContent(runtime, argument);
    std::cout << "Getter" << std::endl;
    return runtime->NewObject();
}

int PrintLog([[maybe_unused]] int id, [[maybe_unused]] int level, const char *tag, [[maybe_unused]] const char *fmt,
             const char *message)
{
    std::cout << tag << "::" << message;
    return 0;
}

// NOLINTNEXTLINE(cppcoreguidelines-special-member-functions, hicpp-special-member-functions)
class TestRuntime {
public:
    TestRuntime()
    {
        runtime_ = ArkJSRuntime::GetInstance();
        runtime_->SetLogPrint(PrintLog);
        runtime_->Initialize("");
    }
    ~TestRuntime()
    {
        runtime_->Reset();
    }
    inline const shared_ptr<JsRuntime> &operator*() const
    {
        return runtime_;
    }
    inline const shared_ptr<JsRuntime> &operator->() const
    {
        return runtime_;
    }

private:
    shared_ptr<JsRuntime> runtime_;
};

int main()
{
    TestRuntime runtime;
    RegisterFunctionType func = AppDebugLogPrint;
    shared_ptr<JsValue> global = runtime->GetGlobal();
    shared_ptr<JsValue> logFunc = runtime->NewFunction(func);
    shared_ptr<JsValue> consoleObj = runtime->NewObject();
    global->SetProperty(*runtime, "console", consoleObj);
    consoleObj->SetProperty(*runtime, "log", logFunc);
    consoleObj->SetProperty(*runtime, "info", logFunc);

    shared_ptr<JsValue> getSetTest = runtime->NewObject();
    shared_ptr<JsValue> setter = runtime->NewFunction(Setter);
    shared_ptr<JsValue> getter = runtime->NewFunction(Getter);
    bool getset = getSetTest->SetAccessorProperty(*runtime, "GetSetTest", getter, setter);
    std::cout << "SetAccessorProperty result: " << getset << std::endl;
    global->SetProperty(*runtime, "GetSet", getSetTest);

    std::vector<shared_ptr<JsValue>> arguments;
    arguments.emplace_back(runtime->NewString("Hello world"));

    consoleObj = global->GetProperty(*runtime, "console");
    logFunc = consoleObj->GetProperty(*runtime, "log");
    shared_ptr<JsValue> testObj = logFunc->Call(*runtime, runtime->NewUndefined(), arguments, 1);

    shared_ptr<JsValue> testArrayValue = runtime->NewString("1");
    shared_ptr<JsValue> testValue = runtime->NewArray();
    testValue->SetProperty(*runtime, "0", testArrayValue);
    testValue->SetProperty(*runtime, "1", testArrayValue);
    std::cout << "GetProperty test: " << testValue->GetArrayLength(*runtime) << std::endl;

    testObj->SetProperty(*runtime, "test", testValue);

    shared_ptr<JsValue> result = testObj->GetProperty(*runtime, "test");
    std::cout << "GetProperty test: " << result->IsArray(*runtime) << ";" << result->GetArrayLength(*runtime)
              << std::endl;

    std::vector<std::string> argument;
    runtime->ExecuteJsBin("native.aex");

    return 0;
}
