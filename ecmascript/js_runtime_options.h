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

#include "libpandabase/utils/pandargs.h"

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
    ENABLE_SNAPSHOT_SERIALIZE = 1 << 7,
    ENABLE_SNAPSHOT_DESERIALIZE = 1 << 8,
};

// asm interpreter control parsed option
struct AsmInterParsedOption {
    int handleStart {-1};
    int handleEnd {-1};
    bool enableAsm {false};
};

class JSRuntimeOptions {
public:
    explicit JSRuntimeOptions() {}
    ~JSRuntimeOptions() = default;
    DEFAULT_COPY_SEMANTIC(JSRuntimeOptions);
    DEFAULT_MOVE_SEMANTIC(JSRuntimeOptions);

    void AddOptions(PandArgParser *parser)
    {
        parser->Add(&enableArkTools_);
        parser->Add(&enableCpuprofiler_);
        parser->Add(&arkProperties_);
        parser->Add(&maxNonmovableSpaceCapacity_);
        parser->Add(&enableAsmInterpreter_);
        parser->Add(&asmOpcodeDisableRange_);
        parser->Add(&stubFile_);
        parser->Add(&aotOutputFile_);
        parser->Add(&targetTriple_);
        parser->Add(&asmOptLevel_);
        parser->Add(&logCompiledMethods);
        parser->Add(&internal_memory_size_limit_);
        parser->Add(&heap_size_limit_);
        parser->Add(&enableIC_);
        parser->Add(&snapshot_file_);
        parser->Add(&framework_abc_file_);
        parser->Add(&icu_data_path_);
        parser->Add(&startup_time_);
        parser->Add(&snapshotOutputFile_);
        parser->Add(&enableRuntimeStat_);
        parser->Add(&logTypeInfer_);
    }

    bool EnableArkTools() const
    {
        return (enableArkTools_.GetValue()) ||
            ((static_cast<uint32_t>(arkProperties_.GetValue()) & ArkProperties::ENABLE_ARKTOOLS) != 0);
    }

    void SetEnableArkTools(bool value)
    {
        enableArkTools_.SetValue(value);
    }

    bool WasSetEnableArkTools() const
    {
        return enableArkTools_.WasSet();
    }

    bool IsEnableRuntimeStat() const
    {
        return enableRuntimeStat_.GetValue();
    }

    void SetEnableRuntimeStat(bool value)
    {
        enableRuntimeStat_.SetValue(value);
    }

    bool WasSetEnableRuntimeStat() const
    {
        return enableRuntimeStat_.WasSet();
    }

    std::string GetStubFile() const
    {
        return stubFile_.GetValue();
    }

    void SetStubFile(std::string value)
    {
        stubFile_.SetValue(std::move(value));
    }

    bool WasStubFileSet() const
    {
        return stubFile_.WasSet();
    }

    std::string GetAOTOutputFile() const
    {
        return aotOutputFile_.GetValue();
    }

    void SetAOTOutputFile(std::string value)
    {
        aotOutputFile_.SetValue(std::move(value));
    }

    bool WasAOTOutputFileSet() const
    {
        return aotOutputFile_.WasSet();
    }

    std::string GetTargetTriple() const
    {
        return targetTriple_.GetValue();
    }

    void SetTargetTriple(std::string value)
    {
        targetTriple_.SetValue(std::move(value));
    }

    size_t GetOptLevel() const
    {
        return asmOptLevel_.GetValue();
    }

    void SetOptLevel(size_t value)
    {
        asmOptLevel_.SetValue(value);
    }

    bool EnableForceGC() const
    {
        return enableForceGc_.GetValue();
    }

    void SetEnableForceGC(bool value)
    {
        enableForceGc_.SetValue(value);
    }

    bool ForceFullGC() const
    {
        return forceFullGc_.GetValue();
    }

    void SetForceFullGC(bool value)
    {
        forceFullGc_.SetValue(value);
    }

    bool EnableCpuProfiler() const
    {
        return enableCpuprofiler_.GetValue();
    }

    void SetEnableCpuprofiler(bool value)
    {
        enableCpuprofiler_.SetValue(value);
    }

    void SetGcThreadNum(size_t num)
    {
        gcThreadNum_.SetValue(num);
    }

    size_t GetGcThreadNum() const
    {
        return gcThreadNum_.GetValue();
    }

    void SetLongPauseTime(size_t time)
    {
        longPauseTime_.SetValue(time);
    }

    size_t GetLongPauseTime() const
    {
        return longPauseTime_.GetValue();
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

    bool EnableOptionalLog() const
    {
        return (static_cast<uint32_t>(arkProperties_.GetValue()) & ArkProperties::OPTIONAL_LOG) != 0;
    }

    bool EnableGCStatsPrint() const
    {
        return (static_cast<uint32_t>(arkProperties_.GetValue()) & ArkProperties::GC_STATS_PRINT) != 0;
    }

    bool EnableParallelGC() const
    {
        return (static_cast<uint32_t>(arkProperties_.GetValue()) & ArkProperties::PARALLEL_GC) != 0;
    }

    bool EnableConcurrentMark() const
    {
        return (static_cast<uint32_t>(arkProperties_.GetValue()) & ArkProperties::CONCURRENT_MARK) != 0;
    }

    bool EnableConcurrentSweep() const
    {
        return (static_cast<uint32_t>(arkProperties_.GetValue()) & ArkProperties::CONCURRENT_SWEEP) != 0;
    }

    bool EnableThreadCheck() const
    {
        return (static_cast<uint32_t>(arkProperties_.GetValue()) & ArkProperties::THREAD_CHECK) != 0;
    }

    bool EnableSnapshotSerialize() const
    {
        return (static_cast<uint32_t>(arkProperties_.GetValue()) & ArkProperties::ENABLE_SNAPSHOT_SERIALIZE) != 0;
    }

    bool EnableSnapshotDeserialize() const
    {
        if (WIN_OR_MAC_PLATFORM) {
            return false;
        }

        return (static_cast<uint32_t>(arkProperties_.GetValue()) & ArkProperties::ENABLE_SNAPSHOT_DESERIALIZE) != 0;
    }

    bool WasSetMaxNonmovableSpaceCapacity() const
    {
        return maxNonmovableSpaceCapacity_.WasSet();
    }

    size_t MaxNonmovableSpaceCapacity() const
    {
        return maxNonmovableSpaceCapacity_.GetValue();
    }

    void SetEnableAsmInterpreter(bool value)
    {
        enableAsmInterpreter_.SetValue(value);
    }

    bool GetEnableAsmInterpreter() const
    {
        return enableAsmInterpreter_.GetValue();
    }

    void SetAsmOpcodeDisableRange(std::string value)
    {
        asmOpcodeDisableRange_.SetValue(std::move(value));
    }

    void ParseAsmInterOption()
    {
        asmInterParsedOption_.enableAsm = enableAsmInterpreter_.GetValue();
        std::string strAsmOpcodeDisableRange = asmOpcodeDisableRange_.GetValue();
        if (strAsmOpcodeDisableRange.empty()) {
            return;
        }

        // asm interpreter handle disable range
        size_t pos = strAsmOpcodeDisableRange.find(",");
        if (pos != std::string::npos) {
            std::string strStart = strAsmOpcodeDisableRange.substr(0, pos);
            std::string strEnd = strAsmOpcodeDisableRange.substr(pos + 1);
            int start =  strStart.empty() ? 0 : std::stoi(strStart);
            int end = strEnd.empty() ? kungfu::BYTECODE_STUB_END_ID : std::stoi(strEnd);
            if (start >= 0 && start < kungfu::BytecodeStubCSigns::NUM_OF_ALL_NORMAL_STUBS
                && end >= 0 && end < kungfu::BytecodeStubCSigns::NUM_OF_ALL_NORMAL_STUBS
                && start <= end) {
                asmInterParsedOption_.handleStart = start;
                asmInterParsedOption_.handleEnd = end;
            }
        }
    }

    AsmInterParsedOption GetAsmInterParsedOption() const
    {
        return asmInterParsedOption_;
    }

    std::string GetlogCompiledMethods() const
    {
        return logCompiledMethods.GetValue();
    }

    void SetlogCompiledMethods(std::string value)
    {
        logCompiledMethods.SetValue(std::move(value));
    }

    bool WasSetlogCompiledMethods() const
    {
        return logCompiledMethods.WasSet() && GetlogCompiledMethods().compare("none") != 0;
    }

    uint64_t GetInternalMemorySizeLimit() const
    {
        return internal_memory_size_limit_.GetValue();
    }

    uint32_t GetHeapSizeLimit() const
    {
        return heap_size_limit_.GetValue();
    }

    void SetHeapSizeLimit(uint32_t value)
    {
        heap_size_limit_.SetValue(value);
    }

    bool WasSetHeapSizeLimit() const
    {
        return heap_size_limit_.WasSet();
    }

    void SetIsWorker(bool isWorker)
    {
        isWorker_.SetValue(isWorker);
    }

    bool IsWorker() const
    {
        return isWorker_.GetValue();
    }

    bool EnableIC() const
    {
        return enableIC_.GetValue();
    }

    void SetEnableIC(bool value)
    {
        enableIC_.SetValue(value);
    }

    bool WasSetEnableIC() const
    {
        return enableIC_.WasSet();
    }

    std::string GetSnapshotFile() const
    {
        return snapshot_file_.GetValue();
    }

    void SetSnapshotFile(std::string value)
    {
        snapshot_file_.SetValue(std::move(value));
    }

    bool WasSetSnapshotFile() const
    {
        return snapshot_file_.WasSet();
    }

    std::string GetFrameworkAbcFile() const
    {
        return framework_abc_file_.GetValue();
    }

    void SetFrameworkAbcFile(std::string value)
    {
        framework_abc_file_.SetValue(std::move(value));
    }

    bool WasSetFrameworkAbcFile() const
    {
        return framework_abc_file_.WasSet();
    }

    std::string GetIcuDataPath() const
    {
        return icu_data_path_.GetValue();
    }

    void SetIcuDataPath(std::string value)
    {
        icu_data_path_.SetValue(std::move(value));
    }

    bool WasSetIcuDataPath() const
    {
        return icu_data_path_.WasSet();
    }

    bool IsStartupTime() const
    {
        return startup_time_.GetValue();
    }

    void SetStartupTime(bool value)
    {
        startup_time_.SetValue(value);
    }

    bool WasSetStartupTime() const
    {
        return startup_time_.WasSet();
    }

    std::string GetSnapshotOutputFile() const
    {
        return snapshotOutputFile_.GetValue();
    }

    void SetSnapshotOutputFile(std::string value)
    {
        snapshotOutputFile_.SetValue(std::move(value));
    }

    bool GetLogTypeInfer() const
    {
        return logTypeInfer_.GetValue();
    }

    void SetLogTypeInfer(bool value)
    {
        logTypeInfer_.SetValue(value);
    }

private:
    static constexpr uint64_t INTERNAL_MEMORY_SIZE_LIMIT_DEFAULT = 2147483648;
    static constexpr uint64_t COMPILER_MEMORY_SIZE_LIMIT_DEFAULT = 268435456;
    static constexpr uint64_t CODE_CACHE_SIZE_LIMIT_DEFAULT = 33554432;

    PandArg<bool> enableArkTools_ {"enable-ark-tools", false, R"(Enable ark tools to debug. Default: false)"};
    PandArg<bool> enableCpuprofiler_ {"enable-cpuprofiler", false,
        R"(Enable cpuprofiler to sample call stack and output to json file. Default: false)"};
    PandArg<std::string> stubFile_ {"stub-file",
        R"(stub.m)",
        R"(Path of file includes common stubs module compiled by stub compiler. Default: "stub.m")"};
    PandArg<bool> enableForceGc_ {"enable-force-gc", true, R"(enable force gc when allocating object)"};
    PandArg<bool> forceFullGc_ {"force-full-gc",
        true,
        R"(if true trigger full gc, else trigger semi and old gc)"};
    PandArg<int> arkProperties_ {"ark-properties", GetDefaultProperties(), R"(set ark properties)"};
    PandArg<uint32_t> gcThreadNum_ {"gcThreadNum", 7, R"(set gcThreadNum. Default: 7)"};
    PandArg<uint32_t> longPauseTime_ {"longPauseTime", 40, R"(set longPauseTime. Default: 40ms)"};
    PandArg<std::string> aotOutputFile_ {"aot-file",
        R"(aot_file.m)",
        R"(Path to AOT output file. Default: "aot_file.m")"};
    PandArg<std::string> targetTriple_ {"target-triple", R"(x86_64-unknown-linux-gnu)",
        R"(target triple for aot compiler or stub compiler.
        Possible values: ["x86_64-unknown-linux-gnu", "arm-unknown-linux-gnu", "aarch64-unknown-linux-gnu"].
        Default: "x86_64-unknown-linux-gnu")"};
    PandArg<uint32_t> asmOptLevel_ {"opt-level", 3,
        R"(Optimization level configuration on llvm back end. Default: "3")"};
    PandArg<uint32_t> maxNonmovableSpaceCapacity_ {"maxNonmovableSpaceCapacity",
        4 * 1024 * 1024,
        R"(set max nonmovable space capacity)"};
    PandArg<bool> enableAsmInterpreter_ {"asm-interpreter", false,
        R"(Enable asm interpreter. Default: false)"};
    PandArg<std::string> asmOpcodeDisableRange_ {"asm-opcode-disable-range",
        "",
        R"(Opcode range when asm interpreter is enabled.)"};
    AsmInterParsedOption asmInterParsedOption_;
    PandArg<uint64_t> internal_memory_size_limit_ {"internal-memory-size-limit", INTERNAL_MEMORY_SIZE_LIMIT_DEFAULT,
        R"(Max internal memory used by the VM. Default: 2147483648)"};
    PandArg<uint32_t> heap_size_limit_ {"heap-size-limit", 512 * 1024 * 1024,
        R"(Max heap size. Default: 512M)"};
    PandArg<bool> enableIC_ {"enable-ic", true, R"(switch of inline cache. Default: true)"};
    PandArg<std::string> snapshot_file_ {"snapshot-file", R"(/system/etc/snapshot)",
        R"(snapshot file. Default: "/system/etc/snapshot")"};
    PandArg<std::string> framework_abc_file_ {"framework-abc-file", R"(strip.native.min.abc)",
        R"(snapshot file. Default: "strip.native.min.abc")"};
    PandArg<std::string> icu_data_path_ {"icu-data-path", R"(default)",
        R"(Path to generated icu data file. Default: "default")"};
    PandArg<bool> startup_time_ {"startup-time", false, R"(Print the start time of command execution. Default: false)"};
    PandArg<std::string> logCompiledMethods {"log-compiled-methods",
        R"(none)",
        R"(print stub or aot logs in units of method, "none": no log, "all": every method,"
        "asm": log all disassemble code)"};
    PandArg<std::string> snapshotOutputFile_ {"snapshot-output-file",
        R"(snapshot)",
        R"(Path to snapshot output file. Default: "snapshot")"};
    PandArg<bool> enableRuntimeStat_ {"enable-runtime-stat", false,
        R"(enable statistics of runtime state. Default: false)"};
    PandArg<bool> logTypeInfer_ {"log-Type-Infer", false,
        R"(print aot type infer log. Default: false)"};
    PandArg<bool> isWorker_ {"IsWorker", false,
        R"(whether is worker vm)"};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_RUNTIME_OPTIONS_H_
