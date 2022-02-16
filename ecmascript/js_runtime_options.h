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
enum ArkProperties {
    DEFAULT = -1,
    OPTIONAL_LOG = 1,
    GC_STATS_PRINT = 1 << 1,
    PARALLEL_GC = 1 << 2,
    CONCURRENT_MARK = 1 << 3,
    CONCURRENT_SWEEP = 1 << 4,
    THREAD_CHECK = 1 << 5,
};

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
        parser->Add(&ark_properties_);
        parser->Add(&enable_ts_aot_);
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

    bool IsForceFullGC() const
    {
        return force_full_gc_.GetValue();
    }

    void SetForceFullGC(bool value)
    {
        force_full_gc_.SetValue(value);
    }

    bool IsEnableCpuProfiler() const
    {
        return enable_cpuprofiler_.GetValue();
    }

    void SetEnableCpuprofiler(bool value)
    {
        enable_cpuprofiler_.SetValue(value);
    }

    bool IsEnableTsAot() const
    {
        return enable_ts_aot_.GetValue();
    }

    void SetEnableTsAot(bool value)
    {
        enable_ts_aot_.SetValue(value);
    }

    bool WasSetEnableTsAot() const
    {
        return enable_ts_aot_.WasSet();
    }

    void SetArkProperties(int prop)
    {
        if (prop != ArkProperties::DEFAULT) {
            ark_properties_.SetValue(prop);
        }
    }

    int GetDefaultProperties()
    {
        return ArkProperties::PARALLEL_GC | ArkProperties::CONCURRENT_MARK | ArkProperties::CONCURRENT_SWEEP;
    }

    int GetArkProperties()
    {
        return ark_properties_.GetValue();
    }

    bool IsEnableOptionalLog() const
    {
        return (ark_properties_.GetValue() & ArkProperties::OPTIONAL_LOG) != 0;
    }

    bool IsEnableGCStatsPrint() const
    {
        return (ark_properties_.GetValue() & ArkProperties::GC_STATS_PRINT) != 0;
    }

    bool IsEnableParallelGC() const
    {
        return (ark_properties_.GetValue() & ArkProperties::PARALLEL_GC) != 0;
    }

    bool IsEnableConcurrentMark() const
    {
        return (ark_properties_.GetValue() & ArkProperties::CONCURRENT_MARK) != 0;
    }

    bool IsEnableConcurrentSweep() const
    {
        return (ark_properties_.GetValue() & ArkProperties::CONCURRENT_SWEEP) != 0;
    }

    bool IsEnableThreadCheck() const
    {
        return (ark_properties_.GetValue() & ArkProperties::THREAD_CHECK) != 0;
    }

private:
    PandArg<bool> enable_ark_tools_ {"enable-ark-tools", false, R"(Enable ark tools to debug. Default: false)"};
    PandArg<bool> enable_cpuprofiler_ {"enable-cpuprofiler", false,
        R"(Enable cpuprofiler to sample call stack and output to json file. Default: false)"};
    PandArg<bool> enable_stub_aot_ {"enable-stub-aot", false, R"(enable aot of fast stub. Default: false)"};
    PandArg<std::string> stub_module_file_ {"stub-module-file",
        R"(stub.m)",
        R"(Path to stub module file. Default: "stub.m")"};
    PandArg<bool> enable_force_gc_ {"enable-force-gc", true, R"(enable force gc when allocating object)"};
    PandArg<bool> force_full_gc_ {"force-full-gc",
        true,
        R"(if true trigger full gc, else trigger semi and old gc)"};
    PandArg<int> ark_properties_ {"ark-properties", GetDefaultProperties(), R"(set ark properties)"};
    PandArg<int> enable_ts_aot_ {"enable-ts-aot", false, R"(enable aot of fast stub. Default: false)"};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_RUNTIME_OPTIONS_H_
