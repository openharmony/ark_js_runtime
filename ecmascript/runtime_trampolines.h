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
#include "ecmascript/ecma_macros.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
using JSTaggedType = panda::ecmascript::JSTaggedType;
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
    static bool AddElementInternal(uintptr_t argGlue, JSTaggedType argReceiver, uint32_t argIndex,
                                   JSTaggedType argValue, uint32_t argAttr);
    static bool CallSetter(uintptr_t argGlue, JSTaggedType argSetter, JSTaggedType argReceiver,
                           JSTaggedType argValue, bool argMayThrow);
    static JSTaggedType CallSetter2(uintptr_t argGlue, JSTaggedType argReceiver, JSTaggedType argValue,
                                JSTaggedType argAccessor);
    static JSTaggedType CallGetter(uintptr_t argGlue, JSTaggedType argGetter, JSTaggedType argReceiver);
    static JSTaggedType CallGetter2(uintptr_t argGlue, JSTaggedType argReceiver, JSTaggedType argHolder,
                                JSTaggedType argAccessor);
    static JSTaggedType CallInternalGetter(uintptr_t argGlue, JSTaggedType argGetter, JSTaggedType argReceiver);
    static void ThrowTypeError(uintptr_t argGlue, int argMessageStringId);
    static bool JSProxySetProperty(uintptr_t argGlue, JSTaggedType argProxy, JSTaggedType argKey,
                                   JSTaggedType argValue, JSTaggedType argReceiver, bool argMayThrow);
    static uint32_t GetHash32(uintptr_t key, uint32_t len);
    static int32_t FindElementWithCache(uintptr_t argGlue, JSTaggedType hClass, JSTaggedType key, int32_t num);
    static uint32_t StringGetHashCode(JSTaggedType ecmaString);
    static JSTaggedType GetTaggedArrayPtrTest(uintptr_t argGlue);
    static JSTaggedType Execute(uintptr_t argGlue, JSTaggedType argFunc, JSTaggedType thisArg, uint32_t argc,
                                uintptr_t argArgv);
    static void SetValueWithBarrier(uintptr_t argGlue, JSTaggedType argAddr, size_t argOffset,
                                   JSTaggedType argValue);
    static double FloatMod(double left, double right);
    static JSTaggedType NewInternalString(uintptr_t argGlue, JSTaggedType argKey);
    static JSTaggedType NewEcmaDynClass(uintptr_t argGlue, uint32_t size, uint32_t type, uint32_t inlinedProps);
    static void UpdateLayOutAndAddTransition(uintptr_t argGlue, JSTaggedType oldHClass, JSTaggedType newHClass,
                                            JSTaggedType key, uint32_t attr);
    static void PrintHeapReginInfo(uintptr_t argGlue);
    static JSTaggedType NewTaggedArray(uintptr_t argGlue, uint32_t length);
    static JSTaggedType CopyArray(uintptr_t argGlue, JSTaggedType array, uint32_t length, uint32_t capacity);
    static JSTaggedType NameDictPutIfAbsent(uintptr_t argGlue, JSTaggedType receiver, JSTaggedType array,
        JSTaggedType key, JSTaggedType value, uint32_t attr, bool needTransToDict);
    static void PropertiesSetValue(uintptr_t argGlue, JSTaggedType argReceiver, JSTaggedType argValue,
                                   JSTaggedType argArray, uint32_t capacity, uint32_t index);
    static JSTaggedType TaggedArraySetValue(uintptr_t argGlue, JSTaggedType argReceiver, JSTaggedType argValue,
                                        JSTaggedType argElement, uint32_t elementIndex, uint32_t capacity);
    static void DebugPrint(int fmtMessageId, ...);
    static void NoticeThroughChainAndRefreshUser(uintptr_t argGlue, uint64_t argoldHClass, uint64_t argnewHClass);
    static void JSArrayListSetByIndex(uintptr_t argGlue, JSTaggedValue obj, int32_t index, JSTaggedValue value);
};
}  // namespace panda::ecmascript
#endif
