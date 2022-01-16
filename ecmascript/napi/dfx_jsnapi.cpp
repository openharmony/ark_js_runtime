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
#include "ecmascript/ecma_module.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/hprof/heap_profiler.h"
namespace panda {
using ecmascript::CString;
using ecmascript::EcmaString;
using ecmascript::JSTaggedValue;
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

std::string DFXJSNApi::BuildNativeAndJsBackStackTrace(EcmaVM *vm)
{
    std::string result = ecmascript::base::ErrorHelper::BuildNativeAndJsBackStackTrace(vm->GetJSThread());
    return result;
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
    const_cast<ecmascript::RegionFactory *>(heap->GetRegionFactory())->Delete(heapProfile);
    return result;
}
}


11111111111111111111111111