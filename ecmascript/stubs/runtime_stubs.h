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

#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_method.h"
#include "ecmascript/mem/region.h"

namespace panda::ecmascript {
using kungfu::CallSignature;
class ConstantPool;
class EcmaVM;
class GlobalEnv;
class JSthread;
class JSFunction;
class ObjectFactory;
extern "C" JSTaggedType OptimizedCallRuntime(uintptr_t glue, uint64_t runtime_id, uint64_t argc, ...);
extern "C" JSTaggedType AsmIntCallRuntime(uintptr_t glue, uint64_t runtime_id, uint64_t argc, ...);
extern "C" JSTaggedType OptimizedCallOptimized(uintptr_t glue, uint32_t expectedNumArgs,
    uint32_t actualNumArgs, uintptr_t codeAddr, ...);
extern "C" void HandleCommonCall(uintptr_t glue, uint64_t callType, uintptr_t sp, uint64_t funcReg,
                                 uint64_t actualArgc, ...);
#define RUNTIME_STUB_WITHOUT_GC_LIST(V)       \
    V(DebugPrint, 1)                          \
    V(FatalPrint, 1)                          \
    V(InsertOldToNewRememberedSet, 3)         \
    V(MarkingBarrier, 5)                      \
    V(DoubleToInt, 1)                         \
    V(OptimizedCallRuntime, 3)                \
    V(OptimizedCallOptimized, 4)              \
    V(AsmIntCallRuntime, 3)                   \
    V(HandleCommonCall, 5)

#define RUNTIME_STUB_WITH_GC_LIST(V)         \
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
    V(Shl2Dyn, 3)                            \
    V(Shr2Dyn, 3)                            \
    V(Ashr2Dyn, 3)                           \
    V(Or2Dyn, 3)                             \
    V(Xor2Dyn, 3)                            \
    V(And2Dyn, 3)                            \
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
    V(LoadValueFromConstantStringTable, 2)   \
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
    V(SuperCall, 5)                          \
    V(CallArg0Dyn, 2)                        \
    V(CallArg1Dyn, 3)                        \
    V(CallArgs2Dyn, 4)                       \
    V(CallArgs3Dyn, 5)                       \
    V(CallIThisRangeDyn, 3)                  \
    V(CallIRangeDyn, 2)                      \
    V(LdBigInt, 2)                           \
    V(NewLexicalEnvWithNameDyn, 3)

#define RUNTIME_EXPROTED_TO_BC_STUB_LIST(V) \
    V(HandleCommonCall)

#define RUNTIME_STUB_LIST(V)                 \
    RUNTIME_STUB_WITHOUT_GC_LIST(V)          \
    RUNTIME_STUB_WITH_GC_LIST(V)

class RuntimeStubs {
public:
    static void Initialize(JSThread *thread);

#define DECLARE_RUNTIME_STUBS(name, counter) \
    static JSTaggedType name(uintptr_t argGlue, uint32_t argc, uintptr_t argv);
    RUNTIME_STUB_WITH_GC_LIST(DECLARE_RUNTIME_STUBS)
#undef DECLARE_RUNTIME_STUBS

    static void DebugPrint(int fmtMessageId, ...);
    static void FatalPrint(int fmtMessageId, ...);
    static void MarkingBarrier([[maybe_unused]]uintptr_t argGlue, uintptr_t slotAddr,
        Region *objectRegion, TaggedObject *value, Region *valueRegion);
    static void InsertOldToNewRememberedSet([[maybe_unused]]uintptr_t argGlue, Region* region, uintptr_t addr);
    static int32_t DoubleToInt(double x);

private:
    static void PrintHeapReginInfo(uintptr_t argGlue);

    static inline JSTaggedValue RuntimeIncDyn(JSThread *thread, const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeDecDyn(JSThread *thread, const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeExpDyn(JSThread *thread, const JSHandle<JSTaggedValue> &base,
                                              const JSHandle<JSTaggedValue> &exponent);
    static inline JSTaggedValue RuntimeIsInDyn(JSThread *thread, const JSHandle<JSTaggedValue> &prop,
                                               const JSHandle<JSTaggedValue> &obj);
    static inline JSTaggedValue RuntimeInstanceofDyn(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                     const JSHandle<JSTaggedValue> &target);
    static inline JSTaggedValue RuntimeCreateGeneratorObj(JSThread *thread, const JSHandle<JSTaggedValue> &genFunc);
    static inline JSTaggedValue RuntimeGetTemplateObject(JSThread *thread, const JSHandle<JSTaggedValue> &literal);
    static inline JSTaggedValue RuntimeGetNextPropName(JSThread *thread, const JSHandle<JSTaggedValue> &iter);
    static inline JSTaggedValue RuntimeIterNext(JSThread *thread, const JSHandle<JSTaggedValue> &iter);
    static inline JSTaggedValue RuntimeCloseIterator(JSThread *thread, const JSHandle<JSTaggedValue> &iter);
    static inline JSTaggedValue RuntimeSuperCallSpread(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                       const JSHandle<JSTaggedValue> &newTarget,
                                                       const JSHandle<JSTaggedValue> &array);
    static inline JSTaggedValue RuntimeDelObjProp(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                  const JSHandle<JSTaggedValue> &prop);
    static inline JSTaggedValue RuntimeNewObjSpreadDyn(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                       const JSHandle<JSTaggedValue> &newTarget,
                                                       const JSHandle<JSTaggedValue> &array);
    static inline JSTaggedValue RuntimeCreateIterResultObj(JSThread *thread, const JSHandle<JSTaggedValue> &value,
                                                           JSTaggedValue flag);
    static inline JSTaggedValue RuntimeAsyncFunctionAwaitUncaught(JSThread *thread,
                                                                  const JSHandle<JSTaggedValue> &asyncFuncObj,
                                                                  const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeAsyncFunctionResolveOrReject(JSThread *thread,
                                                                    const JSHandle<JSTaggedValue> &asyncFuncObj,
                                                                    const JSHandle<JSTaggedValue> &value,
                                                                    bool is_resolve);
    static inline JSTaggedValue RuntimeCopyDataProperties(JSThread *thread, const JSHandle<JSTaggedValue> &dst,
                                                          const JSHandle<JSTaggedValue> &src);
    static inline JSTaggedValue RuntimeStArraySpread(JSThread *thread, const JSHandle<JSTaggedValue> &dst,
                                                     JSTaggedValue index, const JSHandle<JSTaggedValue> &src);
    static inline JSTaggedValue RuntimeGetIteratorNext(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                       const JSHandle<JSTaggedValue> &method);
    static inline JSTaggedValue RuntimeSetObjectWithProto(JSThread *thread, const JSHandle<JSTaggedValue> &proto,
                                                          const JSHandle<JSObject> &obj);
    static inline JSTaggedValue RuntimeLdObjByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                    const JSHandle<JSTaggedValue> &prop, bool callGetter,
                                                    JSTaggedValue receiver);
    static inline JSTaggedValue RuntimeStObjByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                    const JSHandle<JSTaggedValue> &prop,
                                                    const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeStOwnByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                    const JSHandle<JSTaggedValue> &key,
                                                    const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeLdSuperByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                      const JSHandle<JSTaggedValue> &key, JSTaggedValue thisFunc);
    static inline JSTaggedValue RuntimeStSuperByValue(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                      const JSHandle<JSTaggedValue> &key,
                                                      const JSHandle<JSTaggedValue> &value, JSTaggedValue thisFunc);
    static inline JSTaggedValue RuntimeLdObjByIndex(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t idx,
                                                    bool callGetter, JSTaggedValue receiver);
    static inline JSTaggedValue RuntimeStObjByIndex(JSThread *thread, const JSHandle<JSTaggedValue> &obj, uint32_t idx,
                                                    const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeStOwnByIndex(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                    const JSHandle<JSTaggedValue> &idx,
                                                    const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeStGlobalRecord(JSThread *thread, const JSHandle<JSTaggedValue> &prop,
                                                      const JSHandle<JSTaggedValue> &value, bool isConst);
    static inline JSTaggedValue RuntimeNegDyn(JSThread *thread, const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeNotDyn(JSThread *thread, const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeResolveClass(JSThread *thread, const JSHandle<JSFunction> &ctor,
                                                    const JSHandle<TaggedArray> &literal,
                                                    const JSHandle<JSTaggedValue> &base,
                                                    const JSHandle<JSTaggedValue> &lexenv,
                                                    const JSHandle<ConstantPool> &constpool);
    static inline JSTaggedValue RuntimeCloneClassFromTemplate(JSThread *thread, const JSHandle<JSFunction> &ctor,
                                                              const JSHandle<JSTaggedValue> &base,
                                                              const JSHandle<JSTaggedValue> &lexenv,
                                                              const JSHandle<JSTaggedValue> &constpool);
    static inline JSTaggedValue RuntimeSetClassInheritanceRelationship(JSThread *thread,
                                                                       const JSHandle<JSTaggedValue> &ctor,
                                                                       const JSHandle<JSTaggedValue> &base);
    static inline JSTaggedValue RuntimeSetClassConstructorLength(JSThread *thread, JSTaggedValue ctor,
                                                                 JSTaggedValue length);
    static inline JSTaggedValue RuntimeNotifyInlineCache(JSThread *thread, const JSHandle<JSFunction> &func,
                                                         JSMethod *method);
    static inline JSTaggedValue RuntimeStOwnByValueWithNameSet(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                               const JSHandle<JSTaggedValue> &key,
                                                               const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeStOwnByName(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                                  const JSHandle<JSTaggedValue> &prop,
                                                   const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeSuspendGenerator(JSThread *thread, const JSHandle<JSTaggedValue> &genObj,
                                                        const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeGetModuleNamespace(JSThread *thread, JSTaggedValue localName);
    static inline void RuntimeStModuleVar(JSThread *thread, JSTaggedValue key, JSTaggedValue value);
    static inline JSTaggedValue RuntimeLdModuleVar(JSThread *thread, JSTaggedValue key, bool inner);
    static inline JSTaggedValue RuntimeGetPropIterator(JSThread *thread, const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeAsyncFunctionEnter(JSThread *thread);
    static inline JSTaggedValue RuntimeGetIterator(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static inline void RuntimeThrowDyn(JSThread *thread, JSTaggedValue value);
    static inline void RuntimeThrowThrowNotExists(JSThread *thread);
    static inline void RuntimeThrowPatternNonCoercible(JSThread *thread);
    static inline void RuntimeThrowDeleteSuperProperty(JSThread *thread);
    static inline void RuntimeThrowUndefinedIfHole(JSThread *thread, const JSHandle<EcmaString> &obj);
    static inline void RuntimeThrowIfNotObject(JSThread *thread);
    static inline void RuntimeThrowConstAssignment(JSThread *thread, const JSHandle<EcmaString> &value);
    static inline JSTaggedValue RuntimeLdGlobalRecord(JSThread *thread, JSTaggedValue key);
    static inline JSTaggedValue RuntimeTryLdGlobalByName(JSThread *thread, JSTaggedValue global,
                                                         const JSHandle<JSTaggedValue> &prop);
    static inline JSTaggedValue RuntimeTryUpdateGlobalRecord(JSThread *thread, JSTaggedValue prop, JSTaggedValue value);
    static inline JSTaggedValue RuntimeThrowReferenceError(JSThread *thread, const JSHandle<JSTaggedValue> &prop,
                                                           const char *desc);
    static inline JSTaggedValue RuntimeLdGlobalVar(JSThread *thread, JSTaggedValue global,
                                                   const JSHandle<JSTaggedValue> &prop);
    static inline JSTaggedValue RuntimeStGlobalVar(JSThread *thread, const JSHandle<JSTaggedValue> &prop,
                                                   const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeToNumber(JSThread *thread, const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeEqDyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                             const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeNotEqDyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                                const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeLessDyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                               const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeLessEqDyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                                 const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeGreaterDyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                                  const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeGreaterEqDyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                                    const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeAdd2Dyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                               const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeSub2Dyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                               const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeMul2Dyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                               const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeDiv2Dyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                               const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeMod2Dyn(JSThread *thread, const JSHandle<JSTaggedValue> &left,
                                               const JSHandle<JSTaggedValue> &right);
    static inline JSTaggedValue RuntimeCreateEmptyObject(JSThread *thread, ObjectFactory *factory,
                                                         JSHandle<GlobalEnv> globalEnv);
    static inline JSTaggedValue RuntimeCreateEmptyArray(JSThread *thread, ObjectFactory *factory,
                                                        JSHandle<GlobalEnv> globalEnv);
    static inline JSTaggedValue RuntimeGetUnmapedArgs(JSThread *thread, JSTaggedType *sp, uint32_t actualNumArgs,
                                                      uint32_t startIdx);
    static inline JSTaggedValue RuntimeCopyRestArgs(JSThread *thread, JSTaggedType *sp, uint32_t restNumArgs,
                                                    uint32_t startIdx);
    static inline JSTaggedValue RuntimeCreateArrayWithBuffer(JSThread *thread, ObjectFactory *factory,
                                                             const JSHandle<JSTaggedValue> &literal);
    static inline JSTaggedValue RuntimeCreateObjectWithBuffer(JSThread *thread, ObjectFactory *factory,
                                                              const JSHandle<JSObject> &literal);
    static inline JSTaggedValue RuntimeNewLexicalEnvDyn(JSThread *thread, uint16_t numVars);
    static inline JSTaggedValue RuntimeNewObjDynRange(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                      const JSHandle<JSTaggedValue> &newTarget, uint16_t firstArgIdx,
                                                      uint16_t length);
    static inline JSTaggedValue RuntimeDefinefuncDyn(JSThread *thread, JSFunction *func);
    static inline JSTaggedValue RuntimeCreateRegExpWithLiteral(JSThread *thread, const JSHandle<JSTaggedValue> &pattern,
                                                               uint8_t flags);
    static inline JSTaggedValue RuntimeThrowIfSuperNotCorrectCall(JSThread *thread, uint16_t index,
                                                                  JSTaggedValue thisValue);
    static inline JSTaggedValue RuntimeCreateObjectHavingMethod(JSThread *thread, ObjectFactory *factory,
                                                                const JSHandle<JSObject> &literal,
                                                                const JSHandle<JSTaggedValue> &env,
                                                                const JSHandle<JSTaggedValue> &constpool);
    static inline JSTaggedValue RuntimeCreateObjectWithExcludedKeys(JSThread *thread, uint16_t numKeys,
                                                                    const JSHandle<JSTaggedValue> &objVal,
                                                                    uint16_t firstArgRegIdx);
    static inline JSTaggedValue RuntimeDefineNCFuncDyn(JSThread *thread, JSFunction *func);
    static inline JSTaggedValue RuntimeDefineGeneratorFunc(JSThread *thread, JSFunction *func);
    static inline JSTaggedValue RuntimeDefineAsyncFunc(JSThread *thread, JSFunction *func);
    static inline JSTaggedValue RuntimeDefineMethod(JSThread *thread, JSFunction *func,
                                                    const JSHandle<JSTaggedValue> &homeObject);
    static inline JSTaggedValue RuntimeCallSpreadDyn(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                     const JSHandle<JSTaggedValue> &obj,
                                                     const JSHandle<JSTaggedValue> &array);
    static inline JSTaggedValue RuntimeDefineGetterSetterByValue(JSThread *thread, const JSHandle<JSObject> &obj,
                                                                 const JSHandle<JSTaggedValue> &prop,
                                                                 const JSHandle<JSTaggedValue> &getter,
                                                                 const JSHandle<JSTaggedValue> &setter, bool flag);
    static inline JSTaggedValue RuntimeSuperCall(JSThread *thread, const JSHandle<JSTaggedValue> &func,
                                                 const JSHandle<JSTaggedValue> &newTarget, uint16_t firstVRegIdx,
                                                 uint16_t length);
    static inline JSTaggedValue RuntimeThrowTypeError(JSThread *thread, const char *message);
    static inline JSTaggedValue RuntimeGetCallSpreadArgs(JSThread *thread, JSTaggedValue array);
    static inline JSTaggedValue RuntimeThrowReferenceError(JSThread *thread, JSTaggedValue prop, const char *desc);
    static inline JSTaggedValue RuntimeThrowSyntaxError(JSThread *thread, const char *message);
    static inline JSTaggedType RuntimeNativeCall(JSThread *thread, JSTaggedValue func, bool callThis,
                                                 uint32_t actualNumArgs, std::vector<JSTaggedType> &actualArgs);
    static inline JSTaggedValue RuntimeLdBigInt(JSThread *thread, const JSHandle<JSTaggedValue> &numberBigInt);
    static inline JSTaggedValue RuntimeNewLexicalEnvWithNameDyn(JSThread *thread, uint16_t numVars, uint16_t scopeId);
};
}  // namespace panda::ecmascript
#endif
