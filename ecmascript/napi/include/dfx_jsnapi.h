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

#ifndef ECMASCRIPT_NAPI_INCLUDE_DFX_JSNAPI_H
#define ECMASCRIPT_NAPI_INCLUDE_DFX_JSNAPI_H

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "ecmascript/common.h"
#include "libpandabase/macros.h"
#include "ecmascript/tooling/interface/stream.h"

namespace panda {
namespace ecmascript {
class EcmaVM;
class Stream;
class Progress;
struct ProfileInfo;
}
class DFXJSNApi;
using EcmaVM = ecmascript::EcmaVM;
using Stream = ecmascript::Stream;
using Progress = ecmascript::Progress;
using ProfileInfo = ecmascript::ProfileInfo;

class PUBLIC_API DFXJSNApi {
public:
    // progress pointer is used to report the object number for IDE.
    // isVmMode means the internal class in vm is visible. isPrivate means the number and string is not visible.
    static void DumpHeapSnapshot(const EcmaVM *vm, int dumpFormat, const std::string &path, bool isVmMode = true,
                                 bool isPrivate = false);
    static void DumpHeapSnapshot(const EcmaVM *vm, int dumpFormat, Stream *stream, Progress *progress = nullptr,
                                 bool isVmMode = true, bool isPrivate = false);
    static void DumpHeapSnapshot(const EcmaVM *vm, int dumpFormat, bool isVmMode = true, bool isPrivate = false);

    static bool BuildNativeAndJsStackTrace(const EcmaVM *vm, std::string &stackTraceStr);
    static bool BuildJsStackTrace(const EcmaVM *vm, std::string &stackTraceStr);
    static bool StartHeapTracking(const EcmaVM *vm, double timeInterval, bool isVmMode = true,
                                  Stream *stream = nullptr);
    static bool StopHeapTracking(const EcmaVM *vm, const std::string &filePath);
    static bool StopHeapTracking(const EcmaVM *vm, Stream *stream, Progress *progress = nullptr);
    static void PrintStatisticResult(const EcmaVM *vm);
    static void StartRuntimeStat(EcmaVM *vm);
    static void StopRuntimeStat(EcmaVM *vm);
    static size_t GetArrayBufferSize(const EcmaVM *vm);
    static size_t GetHeapTotalSize(const EcmaVM *vm);
    static size_t GetHeapUsedSize(const EcmaVM *vm);

    // profile generator
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    static void StartCpuProfilerForFile(const EcmaVM *vm, const std::string &fileName);
    static void StopCpuProfilerForFile();
    static void StartCpuProfilerForInfo(const EcmaVM *vm);
    static std::unique_ptr<ProfileInfo> StopCpuProfilerForInfo();
#endif

    static void ResumeVM(const EcmaVM *vm);
    static bool SuspendVM(const EcmaVM *vm);
    static bool IsSuspended(const EcmaVM *vm);
    static bool CheckSafepoint(const EcmaVM *vm);
};
}
#endif