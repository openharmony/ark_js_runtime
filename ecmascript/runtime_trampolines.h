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

#ifndef ECMASCRIPT_RUNTIME_TRAMPOLINES_NEW_H
#define ECMASCRIPT_RUNTIME_TRAMPOLINES_NEW_H
#include "ecmascript/compiler/fast_stub_define.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
extern "C" JSTaggedType CallRuntimeTrampoline(uintptr_t glue, uint64_t runtime_id, uint64_t patch_id,
                                              uint32_t argc, ...);
using JSTaggedType = panda::ecmascript::JSTaggedType;
class RuntimeTrampolines {
public:
    enum RuntimeTrampolineId {
#define DEF_RUNTIME_STUB(name, counter) RUNTIME_ID_##name,
        EXTERNAL_RUNTIMESTUB_LIST(DEF_RUNTIME_STUB, DEF_RUNTIME_STUB)
#undef DEF_RUNTIME_STUB
        EXTERNAL_RUNTIME_STUB_MAXID
    };
    static void InitializeRuntimeTrampolines(JSThread *thread)
    {
    #define DEF_RUNTIME_STUB(name, counter) RuntimeTrampolineId::RUNTIME_ID_##name
    #define INITIAL_RUNTIME_FUNCTIONS(name, count) \
        thread->SetRuntimeFunction(DEF_RUNTIME_STUB(name, count), reinterpret_cast<uintptr_t>(name));
        EXTERNAL_RUNTIMESTUB_LIST(INITIAL_RUNTIME_FUNCTIONS, INITIAL_RUNTIME_FUNCTIONS)
    #undef INITIAL_RUNTIME_FUNCTIONS
    #undef DEF_RUNTIME_STUB
    }

#define IGNORE_DECLARE_RUNTIME_TRAMPOLINES(...)
#define DECLARE_RUNTIME_TRAMPOLINES(name, counter) \
    static JSTaggedType name(uintptr_t argGlue, uint32_t argc, uintptr_t argv);
    EXTERNAL_RUNTIMESTUB_LIST(DECLARE_RUNTIME_TRAMPOLINES, IGNORE_DECLARE_RUNTIME_TRAMPOLINES)
#undef DECLARE_RUNTIME_TRAMPOLINES

    static void DebugPrint(int fmtMessageId, ...);

private:
    static void PrintHeapReginInfo(uintptr_t argGlue);
};
}  // namespace panda::ecmascript
#endif
