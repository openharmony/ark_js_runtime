/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_JS_RUNTIME_OPTIONS_H_
#define ECMASCRIPT_JS_RUNTIME_OPTIONS_H_

#include "runtime/include/runtime_options.h"
#include "utils/logger.h"

// namespace panda {
namespace panda::ecmascript {
class JSRuntimeOptions : public RuntimeOptions {
public:
    explicit JSRuntimeOptions(const std::string &exe_path = "") : RuntimeOptions(exe_path) {}
    ~JSRuntimeOptions() = default;
    DEFAULT_COPY_SEMANTIC(JSRuntimeOptions);
    DEFAULT_MOVE_SEMANTIC(JSRuntimeOptions);

    void AddOptions(PandArgParser *parser)
    {
        RuntimeOptions::AddOptions(parser);
        parser->Add(&enable_ark_tools_);
        parser->Add(&enable_stub_aot_);
        parser->Add(&stub_module_file_);
    }

    bool IsEnableArkTools() const
    {
        return enable_ark_tools_.GetValue();
    }

    void SetEnableArkTools(bool value)
    {
        enable_ark_tools_.SetValue(value);
    }

    bool WasSetEnableArkTools() const
    {
        return enable_ark_tools_.WasSet();
    }

    bool IsEnableStubAot() const
    {
        return enable_stub_aot_.GetValue();
    }

    void SetEnableStubAot(bool value)
    {
        enable_stub_aot_.SetValue(value);
    }

    bool WasSetEnableStubAot() const
    {
        return enable_stub_aot_.WasSet();
    }

    std::string GetStubModuleFile() const
    {
        return stub_module_file_.GetValue();
    }

    void SetStubModuleFile(std::string value)
    {
        stub_module_file_.SetValue(std::move(value));
    }

    bool WasSetStubModuleFile() const
    {
        return stub_module_file_.WasSet();
    }

private:
    PandArg<bool> enable_ark_tools_{"enable-ark-tools", false, R"(Enable ark tools to debug. Default: false)"};
    PandArg<bool> enable_stub_aot_{"enable-stub-aot", false, R"(enable aot of fast stub. Default: false)"};
    PandArg<std::string> stub_module_file_{"stub-module-file", R"(stub.m)",
                                           R"(Path to stub module file. Default: "stub.m")"};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_RUNTIME_OPTIONS_H_
