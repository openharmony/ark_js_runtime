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
#include <string>
#include <vector>

#include "ecmascript/common.h"
#include "libpandabase/macros.h"

namespace panda {
namespace ecmascript {
class EcmaVM;
}
class DFXJSNApi;
using EcmaVM = ecmascript::EcmaVM;

class PUBLIC_API DFXJSNApi {
public:
    static void DumpHeapSnapShot(EcmaVM *vm,  int dumpFormat, const std::string &path, bool isVmMode = true);
    static bool BuildNativeAndJsBackStackTrace(EcmaVM *vm, std::string &stackTraceStr);
    static bool StartHeapTracking(EcmaVM *vm, double timeInterval, bool isVmMode = true);
    static bool StopHeapTracking(EcmaVM *vm, int dumpFormat, const std::string &filePath);
    static void PrintStatisticResult(const EcmaVM *vm);
    static void StartRuntimeStat(EcmaVM *vm);
    static void StopRuntimeStat(EcmaVM *vm);
    static size_t GetArrayBufferSize(EcmaVM *vm);
    static size_t GetHeapTotalSize(EcmaVM *vm);
    static size_t GetHeapUsedSize(EcmaVM *vm);

    // profile generator
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    static void StartCpuProfiler(const EcmaVM *vm, const std::string &fileName);
    static void StopCpuProfiler();
#endif

    static void ResumeVM(const EcmaVM *vm);
    static bool SuspendVM(const EcmaVM *vm);
    static bool IsSuspended(const EcmaVM *vm);
    static bool CheckSafepoint(const EcmaVM *vm);
};
}
#endif