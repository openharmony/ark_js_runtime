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

#ifndef ECMASCRIPT_RUNTIME_TRAMPOLINES_H
#define ECMASCRIPT_RUNTIME_TRAMPOLINES_H
#include "ecmascript/compiler/fast_stub_define.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
class RuntimeTrampolines {
public:
    enum RuntimeTrampolineId {
#define DEF_RUNTIME_STUB(name, counter) RUNTIME_ID_##name,
        EXTERNAL_RUNTIMESTUB_LIST(DEF_RUNTIME_STUB)
#undef DEF_RUNTIME_STUB
        EXTERNAL_RUNTIME_STUB_MAXID
    };
    static void InitializeRuntimeTrampolines(JSThread *thread)
    {
    #define DEF_RUNTIME_STUB(name, counter) RuntimeTrampolineId::RUNTIME_ID_##name
    #define INITIAL_RUNTIME_FUNCTIONS(name, count) \
        thread->SetRuntimeFunction(DEF_RUNTIME_STUB(name, count), reinterpret_cast<uintptr_t>(name));
        EXTERNAL_RUNTIMESTUB_LIST(INITIAL_RUNTIME_FUNCTIONS)
    #undef INITIAL_RUNTIME_FUNCTIONS
    #undef DEF_RUNTIME_STUB
    }
    static bool AddElementInternal(uint64_t argThread, uint64_t argReceiver, uint32_t argIndex,
                                   uint64_t argValue, uint32_t argAttr);
    static bool CallSetter(uint64_t argThread, uint64_t argSetter, uint64_t argReceiver, uint64_t argValue,
                           bool argMayThrow);
    static void ThrowTypeError(uint64_t argThread, int argMessageStringId);
    static bool JSProxySetProperty(uint64_t argThread, uint64_t argProxy, uint64_t argKey, uint64_t argValue,
                                   uint64_t argReceiver, bool argMayThrow);
    static uint32_t GetHash32(uint64_t key, uint64_t len);
};
}  // namespace panda::ecmascript
#endif