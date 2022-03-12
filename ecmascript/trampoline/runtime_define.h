/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_TRAMPOLINE_RUNTIME_DEFINE_H
#define ECMASCRIPT_TRAMPOLINE_RUNTIME_DEFINE_H

#include "ecmascript/base/config.h"

namespace panda::ecmascript {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RUNTIME_CALL_LIST_BASE(V, I)         \
    I(DebugPrint, 1)                         \
    I(InsertOldToNewRememberedSet, 3)        \
    I(MarkingBarrier, 5)                     \
    I(DoubleToInt, 1)                        \
    I(RuntimeCallTrampolineAot, 3)           \
    I(RuntimeCallTrampolineInterpreterAsm, 3)\
    V(AddElementInternal, 5)                 \
    V(CallSetter, 5)                         \
    V(CallSetter2, 3)                        \
    V(CallGetter, 3)                         \
    V(CallGetter2, 4)                        \
    V(CallInternalGetter, 3)                 \
    V(ThrowTypeError, 2)                     \
    V(JSProxySetProperty, 6)                 \
    V(GetHash32, 2)                          \
    V(FindElementWithCache, 4)               \
    V(StringGetHashCode, 1)                  \
    V(FloatMod, 2)                           \
    V(GetTaggedArrayPtrTest, 2)              \
    V(NewInternalString, 2)                  \
    V(NewTaggedArray, 2)                     \
    V(CopyArray, 3)                          \
    V(NameDictPutIfAbsent, 7)                \
    V(PropertiesSetValue, 6)                 \
    V(TaggedArraySetValue, 6)                \
    V(NewEcmaDynClass, 4)                    \
    V(UpdateLayOutAndAddTransition, 5)       \
    V(NoticeThroughChainAndRefreshUser, 3)   \
    V(JumpToCInterpreter, 7)                 \
    V(StGlobalRecord, 4)                     \
    V(SetFunctionNameNoPrefix, 3)            \
    V(StOwnByValueWithNameSet, 4)            \
    V(StOwnByName, 4)                        \
    V(StOwnByNameWithNameSet, 7)             \
    V(SuspendGenerator, 7)                   \
    V(UpFrame, 1)                            \
    V(NegDyn, 2)                             \
    V(NotDyn, 2)                             \
    V(IncDyn, 2)                             \
    V(DecDyn, 2)                             \
    V(ChangeUintAndIntShrToJSTaggedValue, 3) \
    V(ChangeUintAndIntShlToJSTaggedValue, 3) \
    V(ChangeTwoInt32AndToJSTaggedValue, 3)   \
    V(ChangeTwoInt32XorToJSTaggedValue, 3)   \
    V(ChangeTwoInt32OrToJSTaggedValue, 3)    \
    V(ChangeTwoUint32AndToJSTaggedValue, 3)  \
    V(ExpDyn, 3)                             \
    V(IsInDyn, 3)                            \
    V(InstanceOfDyn, 3)                      \
    V(FastStrictEqual, 2)                    \
    V(FastStrictNotEqual, 2)                 \
    V(CreateGeneratorObj, 2)                 \
    V(ThrowConstAssignment, 2)               \
    V(GetTemplateObject, 2)                  \
    V(GetNextPropName, 2)                    \
    V(ThrowIfNotObject, 1)                   \
    V(IterNext, 2)                           \
    V(CloseIterator, 2)                      \
    V(CopyModule, 2)                         \
    V(SuperCallSpread, 4)                    \
    V(DelObjProp, 3)                         \
    V(NewObjSpreadDyn, 4)                    \
    V(CreateIterResultObj, 3)                \
    V(AsyncFunctionAwaitUncaught, 3)         \
    V(AsyncFunctionResolveOrReject, 4)       \
    V(ThrowUndefinedIfHole, 2)               \
    V(CopyDataProperties, 3)                 \
    V(StArraySpread, 4)                      \
    V(GetIteratorNext, 3)                    \
    V(SetObjectWithProto, 3)                 \
    V(LoadICByValue, 5)                      \
    V(StoreICByValue, 6)                     \
    V(StOwnByValue, 4)                       \
    V(LdSuperByValue, 4)                     \
    V(StSuperByValue, 5)                     \
    V(LdObjByIndex, 5)                       \
    V(StObjByIndex, 4)                       \
    V(StOwnByIndex, 4)                       \
    V(ResolveClass, 6)                       \
    V(CloneClassFromTemplate, 5)             \
    V(SetClassConstructorLength, 3)          \
    V(LoadICByName, 5)                       \
    V(StoreICByName, 6)                      \
    V(UpdateHotnessCounter, 2)               \
    V(GetModuleNamespace, 2)                 \
    V(StModuleVar, 3)                        \
    V(LdModuleVar, 3)                        \
    V(ThrowDyn, 2)                           \
    V(GetPropIterator, 2)                    \
    V(AsyncFunctionEnter, 1)                 \
    V(GetIterator, 2)                        \
    V(ThrowThrowNotExists, 1)                \
    V(ThrowPatternNonCoercible, 1)           \
    V(ThrowDeleteSuperProperty, 1)           \
    V(EqDyn, 3)                              \
    V(LdGlobalRecord, 2)                     \
    V(GetGlobalOwnProperty, 2)               \
    V(TryLdGlobalByName, 2)                  \
    V(LoadMiss, 6)                           \
    V(StoreMiss, 7)                          \
    V(TryUpdateGlobalRecord, 3)              \
    V(ThrowReferenceError, 2)                \
    V(StGlobalVar, 3)                        \
    V(LdGlobalVar, 3)                        \
    V(ToNumber, 2)                           \
    V(ToBoolean, 1)                          \
    V(NotEqDyn, 3)                           \
    V(LessDyn, 3)                            \
    V(LessEqDyn, 3)                          \
    V(GreaterDyn, 3)                         \
    V(GreaterEqDyn, 3)                       \
    V(Add2Dyn, 3)                            \
    V(Sub2Dyn, 3)                            \
    V(Mul2Dyn, 3)                            \
    V(Div2Dyn, 3)                            \
    V(Mod2Dyn, 3)                            \
    V(GetLexicalEnv, 1)                      \
    V(LoadValueFromConstantPool, 3)          \
    V(CreateEmptyObject, 1)                  \
    V(CreateEmptyArray, 1)                   \
    V(GetSymbolFunction, 1)                  \
    V(GetUnmapedArgs, 2)                     \
    V(CopyRestArgs, 3)                       \
    V(CreateArrayWithBuffer, 2)              \
    V(CreateObjectWithBuffer, 2)             \
    V(NewLexicalEnvDyn, 2)                   \
    V(NewObjDynRange, 5)                     \
    V(DefinefuncDyn, 2)                      \
    V(CreateRegExpWithLiteral, 3)            \
    V(ThrowIfSuperNotCorrectCall, 3)         \
    V(CreateObjectHavingMethod, 4)           \
    V(CreateObjectWithExcludedKeys, 4)       \
    V(DefineNCFuncDyn, 2)                    \
    V(DefineGeneratorFunc, 2)                \
    V(DefineAsyncFunc, 2)                    \
    V(DefineMethod, 3)                       \
    V(SetNotCallableException, 0)            \
    V(SetCallConstructorException, 0)        \
    V(SetStackOverflowException, 0)          \
    V(CallNative, 3)                         \
    V(CallSpreadDyn, 4)                      \
    V(DefineGetterSetterByValue, 6)          \
    V(SuperCall, 5)

#define IGNORE_RUNTIME(...)

#define RUNTIME_CALL_LIST(V)      \
    RUNTIME_CALL_LIST_BASE(V, IGNORE_RUNTIME)

#define ALL_RUNTIME_CALL_LIST(V)      \
    RUNTIME_CALL_LIST_BASE(V, V)

#define NO_GC_RUNTIME_CALL_LIST(V)      \
    RUNTIME_CALL_LIST_BASE(IGNORE_RUNTIME, V)

enum RuntimeTrampolineId {
#define DEF_RUNTIME_STUB(name, counter) RUNTIME_ID_##name,
    ALL_RUNTIME_CALL_LIST(DEF_RUNTIME_STUB)
#undef DEF_RUNTIME_STUB
    RUNTIME_CALL_MAX_ID
};

#define RUNTIME_CALL_ID(name) RuntimeTrampolineId::RUNTIME_ID_##name 
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TRAMPOLINE_RUNTIME_DEFINE_H
