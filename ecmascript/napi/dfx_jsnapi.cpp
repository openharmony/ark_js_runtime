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

#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/base/error_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/tooling/interface/file_stream.h"

#if defined(ENABLE_DUMP_IN_FAULTLOG)
#include "include/faultloggerd_client.h"
#endif

namespace panda {
using ecmascript::CString;
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
using ecmascript::CpuProfiler;
#endif
using ecmascript::EcmaString;
using ecmascript::JSTaggedValue;
using ecmascript::GCStats;
template<typename T>
using JSHandle = ecmascript::JSHandle<T>;
using ecmascript::FileStream;
using ecmascript::FileDescriptorStream;

void DFXJSNApi::DumpHeapSnapshot(EcmaVM *vm, int dumpFormat, const std::string &path, bool isVmMode, bool isPrivate)
{
    FileStream stream(path);
    DumpHeapSnapshot(vm, dumpFormat, &stream, nullptr, isVmMode, isPrivate);
}

void DFXJSNApi::DumpHeapSnapshot(EcmaVM *vm, int dumpFormat, Stream *stream, Progress *progress,
                                 bool isVmMode, bool isPrivate)
{
    ecmascript::HeapProfilerInterface *heapProfile = ecmascript::HeapProfilerInterface::GetInstance(vm);
    heapProfile->DumpHeapSnapshot(ecmascript::DumpFormat(dumpFormat), stream, progress, isVmMode, isPrivate);
    ecmascript::HeapProfilerInterface::Destroy(vm);
}

#if defined(ENABLE_DUMP_IN_FAULTLOG)
void DFXJSNApi::DumpHeapSnapshot(EcmaVM *vm, int dumpFormat, bool isVmMode, bool isPrivate)
{
    // Write in faultlog for heap leak.
    int32_t fd = RequestFileDescriptor(static_cast<int32_t>(FaultLoggerType::JS_HEAP_SNAPSHOT));
    if (fd < 0) {
        LOG_ECMA(ERROR) << "Write FD failed, fd" << fd;
        return;
    }
    FileDescriptorStream stream(fd);
    DumpHeapSnapshot(vm, dumpFormat, &stream, nullptr, isVmMode, isPrivate);
    close(fd);
}
#endif

bool DFXJSNApi::BuildNativeAndJsBackStackTrace(EcmaVM *vm, std::string &stackTraceStr)
{
    CString trace = ecmascript::base::ErrorHelper::BuildNativeAndJsStackTrace(vm->GetJSThreadNoCheck());
    stackTraceStr = CstringConvertToStdString(trace);
    if (stackTraceStr == "") {
        return false;
    }
    return true;
}

bool DFXJSNApi::StartHeapTracking(EcmaVM *vm, double timeInterval, bool isVmMode)
{
    ecmascript::HeapProfilerInterface *heapProfile = ecmascript::HeapProfilerInterface::GetInstance(vm);
    return heapProfile->StartHeapTracking(timeInterval, isVmMode);
}

bool DFXJSNApi::StopHeapTracking(EcmaVM *vm, const std::string &filePath)
{
    FileStream stream(filePath);
    return StopHeapTracking(vm, &stream, nullptr);
}

bool DFXJSNApi::StopHeapTracking(EcmaVM *vm, Stream* stream, Progress *progress)
{
    bool result = false;
    ecmascript::HeapProfilerInterface *heapProfile = ecmascript::HeapProfilerInterface::GetInstance(vm);
    result = heapProfile->StopHeapTracking(stream, progress);
    ecmascript::HeapProfilerInterface::Destroy(vm);
    return result;
}

void DFXJSNApi::PrintStatisticResult(const EcmaVM *vm)
{
    ecmascript::GCStats gcstats(vm->GetHeap());
    gcstats.PrintStatisticResult(true);
}

void DFXJSNApi::StartRuntimeStat(EcmaVM *vm)
{
    vm->SetRuntimeStatEnable(true);
}

void DFXJSNApi::StopRuntimeStat(EcmaVM *vm)
{
    vm->SetRuntimeStatEnable(false);
}

size_t DFXJSNApi::GetArrayBufferSize(EcmaVM *vm)
{
    return vm->GetHeap()->GetArrayBufferSize();
}

size_t DFXJSNApi::GetHeapTotalSize(EcmaVM *vm)
{
    return vm->GetHeap()->GetCommittedSize();
}

size_t DFXJSNApi::GetHeapUsedSize(EcmaVM *vm)
{
    return vm->GetHeap()->GetHeapObjectSize();
}

#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
void DFXJSNApi::StartCpuProfiler(const EcmaVM *vm, const std::string &fileName)
{
    panda::ecmascript::CpuProfiler* singleton = panda::ecmascript::CpuProfiler::GetInstance();
    singleton->StartCpuProfiler(vm, fileName);
}

void DFXJSNApi::StopCpuProfiler()
{
    panda::ecmascript::CpuProfiler* singleton = panda::ecmascript::CpuProfiler::GetInstance();
    singleton->StopCpuProfiler();
}
#endif

bool DFXJSNApi::SuspendVM(const EcmaVM *vm)
{
    ecmascript::VmThreadControl* vmThreadControl = vm->GetJSThreadNoCheck()->GetVmThreadControl();
    return vmThreadControl->NotifyVMThreadSuspension();
}

void DFXJSNApi::ResumeVM(const EcmaVM *vm)
{
    ecmascript::VmThreadControl* vmThreadControl = vm->GetJSThreadNoCheck()->GetVmThreadControl();
    vmThreadControl->ResumeVM();
}

bool DFXJSNApi::IsSuspended(const EcmaVM *vm)
{
    ecmascript::VmThreadControl* vmThreadControl = vm->GetJSThreadNoCheck()->GetVmThreadControl();
    return vmThreadControl->IsSuspended();
}

bool DFXJSNApi::CheckSafepoint(const EcmaVM *vm)
{
    ecmascript::JSThread* thread = vm->GetJSThread();
    return  thread->CheckSafepoint();
}
} // namespace panda
