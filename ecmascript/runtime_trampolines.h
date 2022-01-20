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
    static uintptr_t JumpToCInterpreter(uintptr_t argGlue, uintptr_t pc, uintptr_t sp,
                                           uint64_t constpool, uint64_t profileTypeInfo, uint64_t acc,
                                           int32_t hotnessCounter);
    static JSTaggedType IncDyn(uintptr_t argGlue, JSTaggedType value);
    static JSTaggedType DecDyn(uintptr_t argGlue, JSTaggedType value);
    static JSTaggedType ExpDyn(uintptr_t argGlue, JSTaggedType base, JSTaggedType exponent);
    static JSTaggedType IsInDyn(uintptr_t argGlue, JSTaggedType prop, JSTaggedType obj);
    static JSTaggedType InstanceOfDyn(uintptr_t argGlue, JSTaggedType obj, JSTaggedType target);
    static JSTaggedType FastStrictNotEqual(JSTaggedType left, JSTaggedType right);
    static JSTaggedType FastStrictEqual(JSTaggedType left, JSTaggedType right);
    static JSTaggedType CreateGeneratorObj(uintptr_t argGlue, JSTaggedType genFunc);
    static void ThrowConstAssignment(uintptr_t argGlue, JSTaggedType value);
    static JSTaggedType GetTemplateObject(uintptr_t argGlue, JSTaggedType literal);
    static JSTaggedType GetNextPropName(uintptr_t argGlue, JSTaggedType iter);
    static void ThrowIfNotObject(uintptr_t argGlue);
    static JSTaggedType IterNext(uintptr_t argGlue, JSTaggedType iter);
    static JSTaggedType CloseIterator(uintptr_t argGlue, JSTaggedType iter);
    static void CopyModule(uintptr_t argGlue, JSTaggedType srcModule);
    static JSTaggedType SuperCallSpread(uintptr_t argGlue, JSTaggedType func, uintptr_t sp, JSTaggedType array);
    static JSTaggedType DelObjProp(uintptr_t argGlue, JSTaggedType obj, JSTaggedType prop);
    static JSTaggedType NewObjSpreadDyn(uintptr_t argGlue,
                                        JSTaggedType func, JSTaggedType newTarget, JSTaggedType array);
    static JSTaggedType CreateIterResultObj(uintptr_t argGlue, JSTaggedType value, JSTaggedType flag);
    static JSTaggedType AsyncFunctionAwaitUncaught(uintptr_t argGlue, JSTaggedType asyncFuncObj, JSTaggedType value);
    static void ThrowUndefinedIfHole(uintptr_t argGlue, JSTaggedType obj);
    static JSTaggedType CopyDataProperties(uintptr_t argGlue, JSTaggedType dst, JSTaggedType src);
    static JSTaggedType StArraySpread(uintptr_t argGlue, JSTaggedType dst, JSTaggedType index, JSTaggedType src);
    static JSTaggedType GetIteratorNext(uintptr_t argGlue, JSTaggedType obj, JSTaggedType method);
    static JSTaggedType SetObjectWithProto(uintptr_t argGlue, JSTaggedType proto, JSTaggedType obj);
    static JSTaggedType LoadICByValue(uintptr_t argGlue, JSTaggedType profileTypeInfo,
                                      JSTaggedType receiver, JSTaggedType propKey, int32_t slotId);
    static JSTaggedType StoreICByValue(uintptr_t argGlue, JSTaggedType profileTypeInfo,
                                       JSTaggedType receiver, JSTaggedType propKey, JSTaggedType value,
                                       int32_t slotId);
    static JSTaggedType StOwnByValue(uintptr_t argGlue, JSTaggedType obj, JSTaggedType key, JSTaggedType value);
    static JSTaggedType LdSuperByValue(uintptr_t argGlue, JSTaggedType obj, JSTaggedType key, uintptr_t sp);
    static JSTaggedType StSuperByValue(uintptr_t argGlue,
                                       JSTaggedType obj, JSTaggedType key, JSTaggedType value, uintptr_t sp);
    static JSTaggedType LdObjByIndex(uintptr_t argGlue, JSTaggedType obj, uint32_t idx,
                                     bool callGetter, JSTaggedType receiver);
    static JSTaggedType StObjByIndex(uintptr_t argGlue, JSTaggedType obj, uint32_t idx, JSTaggedType value);
    static JSTaggedType StOwnByIndex(uintptr_t argGlue, JSTaggedType obj, uint32_t idx, JSTaggedType value);
    static JSTaggedType StGlobalRecord(uintptr_t argGlue, JSTaggedType prop, JSTaggedType value, bool isConst);
    static JSTaggedType NegDyn(uintptr_t argGlue, JSTaggedType value);
    static JSTaggedType NotDyn(uintptr_t argGlue, JSTaggedType value);
    static JSTaggedType ChangeUintAndIntShrToJSTaggedValue(uintptr_t argGlue, JSTaggedType leftInt, JSTaggedType rightUint);
    static JSTaggedType ChangeUintAndIntShlToJSTaggedValue(uintptr_t argGlue, JSTaggedType leftInt, JSTaggedType rightUint);
    static JSTaggedType ChangeTwoInt32AndToJSTaggedValue(uintptr_t argGlue, JSTaggedType left, JSTaggedType right);
    static JSTaggedType ChangeTwoInt32XorToJSTaggedValue(uintptr_t argGlue, JSTaggedType left, JSTaggedType right);
    static JSTaggedType ChangeTwoInt32OrToJSTaggedValue(uintptr_t argGlue, JSTaggedType left, JSTaggedType right);
    static JSTaggedType ChangeTwoUint32AndToJSTaggedValue(uintptr_t argGlue, JSTaggedType left, JSTaggedType right);
    static JSTaggedType ResolveClass(uintptr_t argGlue, JSTaggedType ctor, JSTaggedType literal, JSTaggedType base,
                                     JSTaggedType lexenv, JSTaggedType constpool);
    static JSTaggedType CloneClassFromTemplate(uintptr_t argGlue, JSTaggedType ctor, JSTaggedType base,
                                               JSTaggedType lexenv, JSTaggedType constpool);
    static JSTaggedType SetClassConstructorLength(uintptr_t argGlue, JSTaggedType ctor, JSTaggedType length);
    static JSTaggedType UpdateHotnessCounter(uintptr_t argGlue, uintptr_t sp);
    static JSTaggedType LoadICByName(uintptr_t argGlue, JSTaggedType profileTypeInfo,
                                     JSTaggedType receiver, JSTaggedType propKey, int32_t slotId);
    static void SetFunctionNameNoPrefix(uintptr_t argGlue, JSTaggedType argFunc, JSTaggedType argName);
    static JSTaggedType StOwnByValueWithNameSet(uintptr_t argGlue, JSTaggedType obj, JSTaggedType key, JSTaggedType value);
    static JSTaggedType StOwnByNameWithNameSet(uintptr_t argGlue, JSTaggedType obj, JSTaggedType key, JSTaggedType value);
    static JSTaggedType ImportModule(uintptr_t argGlue, JSTaggedType moduleName);
    static void StModuleVar(uintptr_t argGlue, JSTaggedType exportName, JSTaggedType exportObj);
    static JSTaggedType LdModvarByName(uintptr_t argGlue, JSTaggedType moduleObj, JSTaggedType itemName);
    static void ThrowDyn(uintptr_t argGlue, JSTaggedType value);
    static JSTaggedType GetPropIterator(uintptr_t argGlue, JSTaggedType value);
    static JSTaggedType AsyncFunctionEnter(uintptr_t argGlue);
    static JSTaggedType GetIterator(uintptr_t argGlue, JSTaggedType obj);
    static void ThrowThrowNotExists(uintptr_t argGlue);
    static void ThrowPatternNonCoercible(uintptr_t argGlue);
    static void ThrowDeleteSuperProperty(uintptr_t argGlue);
    static JSTaggedType EqDyn(uintptr_t argGlue, JSTaggedType left, JSTaggedType right);
    static JSTaggedType LdGlobalRecord(uintptr_t argGlue, JSTaggedType key);
    static JSTaggedType GetGlobalOwnProperty(uintptr_t argGlue, JSTaggedType key);
    static JSTaggedType TryLdGlobalByName(uintptr_t argGlue, JSTaggedType prop);
    static JSTaggedType LoadMiss(uintptr_t argGlue, JSTaggedType profileTypeInfo, JSTaggedType receiver,
                                 JSTaggedType key, uint32_t slotId, uint32_t kind);
    static JSTaggedType StoreMiss(uintptr_t argGlue, JSTaggedType profileTypeInfo, JSTaggedType receiver,
                                  JSTaggedType key, JSTaggedType value, uint32_t slotId, uint32_t kind);
    static JSTaggedType TryUpdateGlobalRecord(uintptr_t argGlue, JSTaggedType prop, JSTaggedType value);
    static JSTaggedType ThrowReferenceError(uintptr_t argGlue, JSTaggedType prop);
    static JSTaggedType StGlobalVar(uintptr_t argGlue, JSTaggedType prop, JSTaggedType value);
    static JSTaggedType ToNumber(uintptr_t argGlue, JSTaggedType value);
};
}  // namespace panda::ecmascript
#endif
