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

// asm interpreter control parsed option
struct AsmInterParsedOption {
    int handleStart {-1};
    int handleEnd {-1};
    bool enableAsm {false};
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
        parser->Add(&comStubFile_);
        parser->Add(&bcStubFile_);
        parser->Add(&enableCpuprofiler_);
        parser->Add(&arkProperties_);
        parser->Add(&enableTsAot_);
        parser->Add(&maxNonmovableSpaceCapacity_);
        parser->Add(&asmInter_);
        parser->Add(&aotOutputFile_);
        parser->Add(&aotTargetTriple_);
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

    std::string GetComStubFile() const
    {
        return comStubFile_.GetValue();
    }

    void SetComStubFile(std::string value)
    {
        comStubFile_.SetValue(std::move(value));
    }

    bool WasSetComStubFile() const
    {
        return comStubFile_.WasSet();
    }

    std::string GetBcStubFile() const
    {
        return bcStubFile_.GetValue();
    }

    void SetBcStubFile(std::string value)
    {
        bcStubFile_.SetValue(std::move(value));
    }

    bool WasBcComStubFile() const
    {
        return bcStubFile_.WasSet();
    }

    std::string GetAOTOutputFile() const
    {
        return aotOutputFile_.GetValue();
    }

    void SetAOTOutputFile(std::string value)
    {
        aotOutputFile_.SetValue(std::move(value));
    }

    std::string GetAotTargetTriple() const
    {
        return aotTargetTriple_.GetValue();
    }

    void SetAotTargetTriple(std::string value)
    {
        aotTargetTriple_.SetValue(std::move(value));
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

    void SetAsmInterOption(std::string value)
    {
        asmInter_.SetValue(std::move(value));
    }

    void ParseAsmInterOption()
    {
        std::string strAsmInterOption = asmInter_.GetValue();
        if (strAsmInterOption.empty()) {
            return;
        }
        std::vector<std::string> vec;
        size_t pos = 0;
        size_t len = strAsmInterOption.length();
        while (pos < len) {
            size_t delimPos = strAsmInterOption.find("#", pos);
            if (delimPos == std::string::npos) {
                vec.emplace_back(strAsmInterOption.substr(pos));
                break;
            }
            vec.emplace_back(strAsmInterOption.substr(pos, delimPos - pos));
            pos = delimPos + 1;
        }

        // enable or not asm interpreter
        if (vec.size() > 0) {
            std::string enableAsm = vec[0];
            asmInterParsedOption_.enableAsm = (enableAsm == "1") ? true : false;
        }

        // asm interpreter handle disable range
        if (vec.size() > 1) {
            std::string handleDisableRange = vec[1];
            pos = handleDisableRange.find(",");
            if (pos != std::string::npos) {
                std::string strStart = handleDisableRange.substr(0, pos);
                std::string strEnd = handleDisableRange.substr(pos + 1);
                int start =  strStart.empty() ? 0 : std::stoi(strStart);
                int end = strEnd.empty() ? kungfu::BYTECODE_STUB_END_ID : std::stoi(strEnd);
                if (start >= 0 && start < kungfu::BytecodeStubCSigns::NUM_OF_ALL_STUBS
                    && end >= 0 && end < kungfu::BytecodeStubCSigns::NUM_OF_ALL_STUBS
                    && start <= end) {
                    asmInterParsedOption_.handleStart = start;
                    asmInterParsedOption_.handleEnd = end;
                }
            }
        }
    }

    AsmInterParsedOption GetAsmInterParsedOption() const
    {
        return asmInterParsedOption_;
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
    PandArg<std::string> comStubFile_ {"com-stub-file",
        R"(com_stub.m)",
        R"(Path of file includes common stubs module compiled by stub compiler. Default: "com_stub.m")"};
    PandArg<std::string> bcStubFile_ {"bc-stub-file",
        R"(bc_stub.m)",
        R"(Path of file includes bytecode handler stubs module compiled by stub compiler. Default: "bc_stub.m")"};
    PandArg<bool> enableForceGc_ {"enable-force-gc", true, R"(enable force gc when allocating object)"};
    PandArg<bool> forceFullGc_ {"force-full-gc",
        true,
        R"(if true trigger full gc, else trigger semi and old gc)"};
    PandArg<int> arkProperties_ {"ark-properties", GetDefaultProperties(), R"(set ark properties)"};
    PandArg<int> enableTsAot_ {"enable-ts-aot", true, R"(enable aot of fast stub. Default: false)"};
    PandArg<std::string> aotOutputFile_ {"aot-output-file",
        R"(aot_output_file.m)",
        R"(Path to AOT output file. Default: "aot_output_file.m")"};
    PandArg<std::string> aotTargetTriple_ {"aot-target-triple", R"(x86_64-unknown-linux-gnu)",
        R"(stub aot compiler target triple.
        Possible values: ["x86_64-unknown-linux-gnu", "arm-unknown-linux-gnu", "aarch64-unknown-linux-gnu"].
        Default: "x86_64-unknown-linux-gnu")"};
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
    PandArg<std::string> asmInter_ {"asmInter",
        "1",
        R"(set asm interpreter control properties)"};
    AsmInterParsedOption asmInterParsedOption_;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_RUNTIME_OPTIONS_H_
