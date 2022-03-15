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
    ENABLE_ARKTOOLS = 1 << 6,
};

class JSRuntimeOptions : public RuntimeOptions {
public:
    explicit JSRuntimeOptions(const std::string &exePath = "") : RuntimeOptions(exePath) {}
    ~JSRuntimeOptions() = default;
    DEFAULT_COPY_SEMANTIC(JSRuntimeOptions);
    DEFAULT_MOVE_SEMANTIC(JSRuntimeOptions);

    void AddOptions(PandArgParser *parser)
    {
        RuntimeOptions::AddOptions(parser);
        parser->Add(&enableArkTools_);
        parser->Add(&enableStubAot_);
        parser->Add(&stubModuleFile_);
        parser->Add(&enableCpuprofiler_);
        parser->Add(&arkProperties_);
        parser->Add(&enableTsAot_);
        parser->Add(&maxNonmovableSpaceCapacity_);
    }

    bool IsEnableArkTools() const
    {
        return (enableArkTools_.GetValue()) || ((arkProperties_.GetValue() & ArkProperties::ENABLE_ARKTOOLS) != 0);
    }

    void SetEnableArkTools(bool value)
    {
        enableArkTools_.SetValue(value);
    }

    bool WasSetEnableArkTools() const
    {
        return enableArkTools_.WasSet();
    }

    bool IsEnableStubAot() const
    {
        return enableStubAot_.GetValue();
    }

    void SetEnableStubAot(bool value)
    {
        enableStubAot_.SetValue(value);
    }

    bool WasSetEnableStubAot() const
    {
        return enableStubAot_.WasSet();
    }

    std::string GetStubModuleFile() const
    {
        return stubModuleFile_.GetValue();
    }

    void SetStubModuleFile(std::string value)
    {
        stubModuleFile_.SetValue(std::move(value));
    }

    bool WasSetStubModuleFile() const
    {
        return stubModuleFile_.WasSet();
    }

    bool IsEnableForceGC() const
    {
        return enableForceGc_.GetValue();
    }

    void SetEnableForceGC(bool value)
    {
        enableForceGc_.SetValue(value);
    }

    bool IsForceFullGC() const
    {
        return forceFullGc_.GetValue();
    }

    void SetForceFullGC(bool value)
    {
        forceFullGc_.SetValue(value);
    }

    bool IsEnableCpuProfiler() const
    {
        return enableCpuprofiler_.GetValue();
    }

    void SetEnableCpuprofiler(bool value)
    {
        enableCpuprofiler_.SetValue(value);
    }

    bool IsEnableTsAot() const
    {
        return enableTsAot_.GetValue();
    }

    void SetEnableTsAot(bool value)
    {
        enableTsAot_.SetValue(value);
    }

    bool WasSetEnableTsAot() const
    {
        return enableTsAot_.WasSet();
    }

    void SetArkProperties(int prop)
    {
        if (prop != ArkProperties::DEFAULT) {
            arkProperties_.SetValue(prop);
        }
    }

    int GetDefaultProperties()
    {
        return ArkProperties::PARALLEL_GC | ArkProperties::CONCURRENT_MARK | ArkProperties::CONCURRENT_SWEEP;
    }

    int GetArkProperties()
    {
        return arkProperties_.GetValue();
    }

    bool IsEnableOptionalLog() const
    {
        return (arkProperties_.GetValue() & ArkProperties::OPTIONAL_LOG) != 0;
    }

    bool IsEnableGCStatsPrint() const
    {
        return (arkProperties_.GetValue() & ArkProperties::GC_STATS_PRINT) != 0;
    }

    bool IsEnableParallelGC() const
    {
        return (arkProperties_.GetValue() & ArkProperties::PARALLEL_GC) != 0;
    }

    bool IsEnableConcurrentMark() const
    {
        return (arkProperties_.GetValue() & ArkProperties::CONCURRENT_MARK) != 0;
    }

    bool IsEnableConcurrentSweep() const
    {
        return (arkProperties_.GetValue() & ArkProperties::CONCURRENT_SWEEP) != 0;
    }

    bool IsEnableThreadCheck() const
    {
        return (arkProperties_.GetValue() & ArkProperties::THREAD_CHECK) != 0;
    }

    size_t MaxSemiSpaceCapacity() const
    {
        return maxSemiSpaceCapacity_.GetValue();
    }

    size_t MaxOldSpaceCapacity() const
    {
        return maxOldSpaceCapacity_.GetValue();
    }

    size_t MaxNonmovableSpaceCapacity() const
    {
        return maxNonmovableSpaceCapacity_.GetValue();
    }

    size_t MaxMachineCodeSpaceCapacity() const
    {
        return maxMachineCodeSpaceCapacity_.GetValue();
    }

    size_t MaxSnapshotSpaceCapacity() const
    {
        return maxSnapshotSpaceCapacity_.GetValue();
    }

    size_t DefaultSemiSpaceCapacity() const
    {
        return defaultSemiSpaceCapacity_.GetValue();
    }

    size_t DefaultSnapshotSpaceCapacity() const
    {
        return defaultSnapshotSpaceCapacity_.GetValue();
    }

    static const JSRuntimeOptions &GetTemporaryOptions()
    {
        return temporary_options;
    }

    static JSRuntimeOptions temporary_options;

private:
    PandArg<bool> enableArkTools_ {"enable-ark-tools", false, R"(Enable ark tools to debug. Default: false)"};
    PandArg<bool> enableCpuprofiler_ {"enable-cpuprofiler", false,
        R"(Enable cpuprofiler to sample call stack and output to json file. Default: false)"};
    PandArg<bool> enableStubAot_ {"enable-stub-aot", false, R"(enable aot of fast stub. Default: false)"};
    PandArg<std::string> stubModuleFile_ {"stub-module-file",
        R"(stub.m)",
        R"(Path to stub module file. Default: "stub.m")"};
    PandArg<bool> enableForceGc_ {"enable-force-gc", true, R"(enable force gc when allocating object)"};
    PandArg<bool> forceFullGc_ {"force-full-gc",
        true,
        R"(if true trigger full gc, else trigger semi and old gc)"};
    PandArg<int> arkProperties_ {"ark-properties", GetDefaultProperties(), R"(set ark properties)"};
    PandArg<int> enableTsAot_ {"enable-ts-aot", true, R"(enable aot of fast stub. Default: false)"};
    PandArg<size_t> maxSemiSpaceCapacity_ {"maxSemiSpaceCapacity",
        16 * 1024 * 1024,
        R"(set max semi space capacity)"};
    PandArg<size_t> maxOldSpaceCapacity_ {"maxOldSpaceCapacity",
        256 * 1024 * 1024,
        R"(set max old space capacity)"};
    PandArg<size_t> maxNonmovableSpaceCapacity_ {"maxNonmovableSpaceCapacity",
        4 * 1024 * 1024,
        R"(set max nonmovable space capacity)"};
    PandArg<size_t> maxMachineCodeSpaceCapacity_ {"maxMachineCodeSpaceCapacity",
        8 * 1024 * 1024,
        R"(set max machine code space capacity)"};
    PandArg<size_t> maxSnapshotSpaceCapacity_ {"maxSnapshotSpaceCapacity",
        8 * 1024 * 1024,
        R"(set max snapshot space capacity)"};
    PandArg<size_t> defaultSemiSpaceCapacity_ {"defaultSemiSpaceCapacity",
        2 * 1024 * 1024,
        R"(set default semi space capacity)"};
    PandArg<size_t> defaultSnapshotSpaceCapacity_ {"defaultSnapshotSpaceCapacity",
        256 * 1024,
        R"(set default snapshot space capacity)"};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_RUNTIME_OPTIONS_H_
