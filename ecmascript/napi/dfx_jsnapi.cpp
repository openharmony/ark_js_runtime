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
#include "ecmascript/ecma_module.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/mem/heap-inl.h"

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

void DFXJSNApi::DumpHeapSnapShot(EcmaVM *vm,  int dumpFormat, const std::string &path, bool isVmMode)
{
    if (dumpFormat == 0) {
        ecmascript::HeapProfilerInterface::DumpHeapSnapShot(vm->GetJSThread(), ecmascript::DumpFormat::JSON,
                                                            path, isVmMode);
    } else if (dumpFormat == 1) {
        ecmascript::HeapProfilerInterface::DumpHeapSnapShot(vm->GetJSThread(), ecmascript::DumpFormat::BINARY,
                                                            path, isVmMode);
    } else if (dumpFormat == 2) { // 2: enum is 2
        ecmascript::HeapProfilerInterface::DumpHeapSnapShot(vm->GetJSThread(), ecmascript::DumpFormat::OTHER,
                                                            path, isVmMode);
    }
}

bool DFXJSNApi::BuildNativeAndJsBackStackTrace(EcmaVM *vm, std::string &stackTraceStr)
{
    CString trace = ecmascript::base::ErrorHelper::BuildNativeAndJsStackTrace(vm->GetJSThread());
    stackTraceStr = CstringConvertToStdString(trace);
    if (stackTraceStr == "") {
        return false;
    }
    return true;
}

bool DFXJSNApi::StartHeapTracking(EcmaVM *vm, double timeInterval, bool isVmMode)
{
    ecmascript::HeapProfilerInterface *heapProfile = ecmascript::HeapProfilerInterface::GetInstance(vm->GetJSThread());
    return heapProfile->StartHeapTracking(vm->GetJSThread(), timeInterval, isVmMode);
}

bool DFXJSNApi::StopHeapTracking(EcmaVM *vm,  int dumpFormat, const std::string &filePath)
{
    bool result = false;
    ecmascript::HeapProfilerInterface *heapProfile = ecmascript::HeapProfilerInterface::GetInstance(vm->GetJSThread());
    if (dumpFormat == 0) {
        result = heapProfile->StopHeapTracking(vm->GetJSThread(), ecmascript::DumpFormat::JSON, filePath);
    }
    if (dumpFormat == 1) {
        result = heapProfile->StopHeapTracking(vm->GetJSThread(), ecmascript::DumpFormat::BINARY, filePath);
    }
    if (dumpFormat == 2) { // 2: enum is 2
        result = heapProfile->StopHeapTracking(vm->GetJSThread(), ecmascript::DumpFormat::OTHER, filePath);
    }
    const ecmascript::Heap *heap = vm->GetJSThread()->GetEcmaVM()->GetHeap();
    const_cast<ecmascript::NativeAreaAllocator *>(heap->GetNativeAreaAllocator())->Delete(heapProfile);
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
}
