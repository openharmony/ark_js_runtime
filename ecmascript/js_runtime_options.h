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
        parser->Add(&enable_cpuprofiler_);
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

    bool IsEnableForceGC() const
    {
        return enable_force_gc_.GetValue();
    }

    void SetEnableForceGC(bool value)
    {
        enable_force_gc_.SetValue(value);
    }

    bool IsForceCompressGC() const
    {
        return force_compress_gc_.GetValue();
    }

    void SetForceCompressGC(bool value)
    {
        force_compress_gc_.SetValue(value);
    }

    bool IsEnableConcurrentSweep() const
    {
        return enable_concurrent_sweep_.GetValue();
    }

    void SetEnableConcurrentSweep(bool value)
    {
        enable_concurrent_sweep_.SetValue(value);
    }

    bool IsEnableCpuProfiler() const
    {
        return enable_cpuprofiler_.GetValue();
    }

    void SetEnableCpuprofiler(bool value)
    {
        enable_cpuprofiler_.SetValue(value);
    }

private:
    PandArg<bool> enable_ark_tools_{"enable-ark-tools", false, R"(Enable ark tools to debug. Default: false)"};
    PandArg<bool> enable_cpuprofiler_{"enable-cpuprofiler", false,
        R"(Enable cpuprofiler to sample call stack and output to json file. Default: false)"};
    PandArg<bool> enable_stub_aot_{"enable-stub-aot", false, R"(enable aot of fast stub. Default: false)"};
    PandArg<std::string> stub_module_file_{"stub-module-file",
        R"(stub.m)",
        R"(Path to stub module file. Default: "stub.m")"};
    PandArg<bool> enable_force_gc_{"enable-force-gc", true, R"(enable force gc when allocating object)"};
    PandArg<bool> force_compress_gc_{"force-compress-gc",
        true,
        R"(if true trigger compress gc, else trigger semi and old gc)"};
    PandArg<bool> enable_concurrent_sweep_{"enable_concurrent_sweep",
        true,
        R"(If true enable concurrent sweep, else disable concurrent sweep. Default: true)"};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_RUNTIME_OPTIONS_H_
