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

#ifndef ECMASCRIPT_RUNTIME_STUBS_H
#define ECMASCRIPT_RUNTIME_STUBS_H

#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/frames.h"
#include "ecmascript/stubs/test_runtime_stubs.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_method.h"
#include "ecmascript/mem/region.h"

namespace panda::ecmascript {
using kungfu::CallSignature;
class ConstantPool;
class EcmaVM;
class GlobalEnv;
class JSThread;
class JSFunction;
class ObjectFactory;

using JSFunctionEntryType = uint64_t (*)(uintptr_t glue, uintptr_t prevFp, uint32_t expectedNumArgs,
                                         uint32_t actualNumArgs, const JSTaggedType argV[], uintptr_t codeAddr);

#define RUNTIME_ASM_STUB_LIST(V)             \
    V(CallRuntime)                           \
    V(AsmInterpreterEntry)                   \
    V(GeneratorReEnterAsmInterp)             \
    V(PushCallArgsAndDispatchNative)         \
    V(PushCallArgs0AndDispatch)              \
    V(PushCallArgs1AndDispatch)              \
    V(PushCallArgs2AndDispatch)              \
    V(PushCallArgs3AndDispatch)              \
    V(PushCallIRangeAndDispatch)             \
    V(PushCallNewAndDispatch)                \
    V(PushCallNewAndDispatchNative)          \
    V(PushCallIRangeAndDispatchNative)       \
    V(PushCallIThisRangeAndDispatch)         \
    V(CallOptimizedJSFunction)               \
    V(JSCallWithArgV)                        \
    V(ResumeRspAndDispatch)                  \
    V(ResumeRspAndReturn)                    \
    V(ResumeCaughtFrameAndDispatch)          \
    V(ResumeUncaughtFrameAndReturn)          \
    V(CallSetter)                            \
    V(CallGetter)                            \
    V(CallRuntimeWithArgv)                   \
    V(JSCall)                                \
    V(JSProxyCallInternalWithArgV)           \
    V(JSFunctionEntry)                       \
    V(CallBuiltinTrampoline)                 \
    V(OptimizedCallOptimized)

#define RUNTIME_STUB_WITHOUT_GC_LIST(V)        \
    V(DebugPrint)                              \
    V(FatalPrint)                              \
    V(InsertOldToNewRSet)                      \
    V(MarkingBarrier)                          \
    V(DoubleToInt)                             \
    V(FloatMod)                                \
    V(FindElementWithCache)                    \
    V(JSObjectGetMethod)                       \
    V(CreateArrayFromList)                     \
    V(StringsAreEquals)                        \
    V(BigIntEquals)                            \

#define RUNTIME_STUB_WITH_GC_LIST(V)      \
    V(AddElementInternal)                 \
    V(AllocateInYoung)                    \
    V(CallInternalGetter)                 \
    V(CallInternalSetter)                 \
    V(ThrowTypeError)                     \
    V(JSProxySetProperty)                 \
    V(GetHash32)                          \
    V(ComputeHashcode)                    \
    V(GetTaggedArrayPtrTest)              \
    V(NewInternalString)                  \
    V(NewTaggedArray)                     \
    V(CopyArray)                          \
    V(NameDictPutIfAbsent)                \
    V(PropertiesSetValue)                 \
    V(TaggedArraySetValue)                \
    V(NewEcmaDynClass)                    \
    V(UpdateLayOutAndAddTransition)       \
    V(NoticeThroughChainAndRefreshUser)   \
    V(JumpToCInterpreter)                 \
    V(StGlobalRecord)                     \
    V(SetFunctionNameNoPrefix)            \
    V(StOwnByValueWithNameSet)            \
    V(StOwnByName)                        \
    V(StOwnByNameWithNameSet)             \
    V(SuspendGenerator)                   \
    V(UpFrame)                            \
    V(NegDyn)                             \
    V(NotDyn)                             \
    V(IncDyn)                             \
    V(DecDyn)                             \
    V(Shl2Dyn)                            \
    V(Shr2Dyn)                            \
    V(Ashr2Dyn)                           \
    V(Or2Dyn)                             \
    V(Xor2Dyn)                            \
    V(And2Dyn)                            \
    V(ExpDyn)                             \
    V(IsInDyn)                            \
    V(InstanceOfDyn)                      \
    V(FastStrictEqual)                    \
    V(FastStrictNotEqual)                 \
    V(CreateGeneratorObj)                 \
    V(ThrowConstAssignment)               \
    V(GetTemplateObject)                  \
    V(GetNextPropName)                    \
    V(ThrowIfNotObject)                   \
    V(IterNext)                           \
    V(CloseIterator)                      \
    V(CopyModule)                         \
    V(SuperCallSpread)                    \
    V(DelObjProp)                         \
    V(NewObjSpreadDyn)                    \
    V(CreateIterResultObj)                \
    V(AsyncFunctionAwaitUncaught)         \
    V(AsyncFunctionResolveOrReject)       \
    V(ThrowUndefinedIfHole)               \
    V(CopyDataProperties)                 \
    V(StArraySpread)                      \
    V(GetIteratorNext)                    \
    V(SetObjectWithProto)                 \
    V(LoadICByValue)                      \
    V(StoreICByValue)                     \
    V(StOwnByValue)                       \
    V(LdSuperByValue)                     \
    V(StSuperByValue)                     \
    V(LdObjByIndex)                       \
    V(StObjByIndex)                       \
    V(StOwnByIndex)                       \
    V(ResolveClass)                       \
    V(CloneClassFromTemplate)             \
    V(SetClassConstructorLength)          \
    V(LoadICByName)                       \
    V(StoreICByName)                      \
    V(UpdateHotnessCounter)               \
    V(GetModuleNamespace)                 \
    V(StModuleVar)                        \
    V(LdModuleVar)                        \
    V(ThrowDyn)                           \
    V(GetPropIterator)                    \
    V(AsyncFunctionEnter)                 \
    V(GetIterator)                        \
    V(ThrowThrowNotExists)                \
    V(ThrowPatternNonCoercible)           \
    V(ThrowDeleteSuperProperty)           \
    V(EqDyn)                              \
    V(LdGlobalRecord)                     \
    V(GetGlobalOwnProperty)               \
    V(TryLdGlobalByName)                  \
    V(LoadMiss)                           \
    V(StoreMiss)                          \
    V(TryUpdateGlobalRecord)              \
    V(ThrowReferenceError)                \
    V(StGlobalVar)                        \
    V(LdGlobalVar)                        \
    V(ToNumber)                           \
    V(ToBoolean)                          \
    V(NotEqDyn)                           \
    V(LessDyn)                            \
    V(LessEqDyn)                          \
    V(GreaterDyn)                         \
    V(GreaterEqDyn)                       \
    V(Add2Dyn)                            \
    V(Sub2Dyn)                            \
    V(Mul2Dyn)                            \
    V(Div2Dyn)                            \
    V(Mod2Dyn)                            \
    V(LoadValueFromConstantStringTable)   \
    V(CreateEmptyObject)                  \
    V(CreateEmptyArray)                   \
    V(GetSymbolFunction)                  \
    V(GetUnmapedArgs)                     \
    V(CopyRestArgs)                       \
    V(CreateArrayWithBuffer)              \
    V(CreateObjectWithBuffer)             \
    V(NewLexicalEnvDyn)                   \
    V(NewThisObject)                      \
    V(NewObjDynRange)                     \
    V(DefinefuncDyn)                      \
    V(CreateRegExpWithLiteral)            \
    V(ThrowIfSuperNotCorrectCall)         \
    V(CreateObjectHavingMethod)           \
    V(CreateObjectWithExcludedKeys)       \
    V(DefineNCFuncDyn)                    \
    V(DefineGeneratorFunc)                \
    V(DefineAsyncFunc)                    \
    V(DefineMethod)                       \
    V(ThrowSetterIsUndefinedException)    \
    V(ThrowNotCallableException)          \
    V(ThrowCallConstructorException)      \
    V(ThrowStackOverflowException)        \
    V(ThrowDerivedMustReturnException)    \
    V(CallNative)                         \
    V(CallSpreadDyn)                      \
    V(DefineGetterSetterByValue)          \
    V(SuperCall)                          \
    V(LdBigInt)                           \
    V(NewLexicalEnvWithNameDyn)           \
    V(GetAotUnmapedArgs)                  \
    V(GetAotUnmapedArgsWithRestArgs)      \
    V(CopyAotRestArgs)                    \
    V(NotifyBytecodePcChanged)            \
    V(GetAotLexicalEnv)                   \
    V(NewAotLexicalEnvDyn)                \
    V(NewAotLexicalEnvWithNameDyn)        \
    V(SuspendAotGenerator)                \
    V(NewAotObjDynRange)                  \
    V(GetTypeArrayPropertyByIndex)        \
    V(SetTypeArrayPropertyByIndex)        \
    V(AotNewObjWithIHClass)               \
    V(PopAotLexicalEnv)                   \
    V(LdAotLexVarDyn)                     \
    V(StAotLexVarDyn)

#define RUNTIME_STUB_LIST(V)                     \
    RUNTIME_ASM_STUB_LIST(V)                     \
    RUNTIME_STUB_WITHOUT_GC_LIST(V)              \
    RUNTIME_STUB_WITH_GC_LIST(V)                 \
    TEST_RUNTIME_STUB_GC_LIST(V)

class RuntimeStubs {
public:
    static void Initialize(JSThread *thread);

#define DECLARE_RUNTIME_STUBS(name) \
    static JSTaggedType name(uintptr_t argGlue, uint32_t argc, uintptr_t argv);
    RUNTIME_STUB_WITH_GC_LIST(DECLARE_RUNTIME_STUBS)
    TEST_RUNTIME_STUB_GC_LIST(DECLARE_RUNTIME_STUBS)
#undef DECLARE_RUNTIME_STUBS

    inline static JSTaggedType GetTArg(uintptr_t argv, [[maybe_unused]] uint32_t argc, uint32_t index)
    {
        ASSERT(index < argc);
        return *(reinterpret_cast<JSTaggedType *>(argv) + (index));
    }

    inline static JSTaggedValue GetArg(uintptr_t argv, [[maybe_unused]] uint32_t argc, uint32_t index)
    {
        ASSERT(index < argc);
        return JSTaggedValue(*(reinterpret_cast<JSTaggedType *>(argv) + (index)));
    }

    template<typename T>
    inline static JSHandle<T> GetHArg(uintptr_t argv, [[maybe_unused]] uint32_t argc, uint32_t index)
    {
        ASSERT(index < argc);
        return JSHandle<T>(&(reinterpret_cast<JSTaggedType *>(argv)[index]));
    }

    template<typename T>
    inline static T GetPtrArg(uintptr_t argv, [[maybe_unused]] uint32_t argc, uint32_t index)
    {
        ASSERT(index < argc);
        return reinterpret_cast<T>(*(reinterpret_cast<JSTaggedType *>(argv) + (index)));
    }

    static void DebugPrint(int fmtMessageId, ...);
    static void FatalPrint(int fmtMessageId, ...);
    static void MarkingBarrier([[maybe_unused]]uintptr_t argGlue, uintptr_t slotAddr,
        Region *objectRegion, TaggedObject *value, Region *valueRegion);
    static JSTaggedType JSObjectGetMethod([[maybe_unused]]uintptr_t argGlue, JSTaggedValue handler, JSTaggedValue key);
    static JSTaggedType CreateArrayFromList([[maybe_unused]]uintptr_t argGlue, int32_t argc, JSTaggedValue *argv);
    static void InsertOldToNewRSet([[maybe_unused]]uintptr_t argGlue, Region* region, uintptr_t addr);
    static int32_t DoubleToInt(double x);
    static JSTaggedType FloatMod(double x, double y);
    static int32_t FindElementWithCache(uintptr_t argGlue, JSTaggedType hClass,
                                        JSTaggedType key, int32_t num);
    static bool StringsAreEquals(EcmaString *str1, EcmaString *str2);
    static bool BigIntEquals(JSTaggedType left, JSTaggedType right);
private:
    static void PrintHeapReginInfo(uintptr_t argGlue);

    static inline JSTaggedValue RuntimeIncDyn(JSThread *thread, const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeDecDyn(JSThread *thread, const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeExpDyn(JSThread *thread, JSTaggedValue base, JSTaggedValue exponent);
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
    static inline JSTaggedValue RuntimeLdBigInt(JSThread *thread, const JSHandle<JSTaggedValue> &numberBigInt);
    static inline JSTaggedValue RuntimeNewLexicalEnvWithNameDyn(JSThread *thread, uint16_t numVars, uint16_t scopeId);
    static inline JSTaggedValue RuntimeGetAotUnmapedArgs(JSThread *thread, uint32_t actualNumArgs);
    static inline JSTaggedValue RuntimeGetAotUnmapedArgsWithRestArgs(JSThread *thread, uint32_t actualNumArgs);
    static inline JSTaggedValue RuntimeGetUnmapedJSArgumentObj(JSThread *thread,
                                                               const JSHandle<TaggedArray> &argumentsList);
    static inline JSTaggedValue RuntimeNewAotLexicalEnvDyn(JSThread *thread, uint16_t numVars,
                                                           JSHandle<JSTaggedValue> &currentLexEnv);
    static inline JSTaggedValue RuntimeNewAotLexicalEnvWithNameDyn(JSThread *thread, uint16_t numVars, uint16_t scopeId,
                                                                   JSHandle<JSTaggedValue> &currentLexEnv,
                                                                   JSHandle<JSTaggedValue> &func);
    static inline JSTaggedValue RuntimeCopyAotRestArgs(JSThread *thread, uint32_t actualArgc, uint32_t restIndex);
    static inline JSTaggedValue RuntimeSuspendAotGenerator(JSThread *thread, const JSHandle<JSTaggedValue> &genObj,
                                                           const JSHandle<JSTaggedValue> &value);
    static inline JSTaggedValue RuntimeNewAotObjDynRange(JSThread *thread, uintptr_t argv, uint32_t argc);

    static inline JSTaggedValue RuntimeAotNewObjWithIHClass(JSThread *thread, uintptr_t argv, uint32_t argc);
    static inline JSTaggedValue RuntimeGetAotLexEnv(JSThread *thread);
    static inline void RuntimeSetAotLexEnv(JSThread *thread, JSTaggedValue lexEnv);
    static inline JSTaggedValue RuntimeGenerateAotScopeInfo(JSThread *thread, uint16_t scopeId, JSTaggedValue func);
    static inline JSTaggedType *GetActualArgv(JSThread *thread);
    static inline OptimizedJSFunctionFrame *GetOptimizedJSFunctionFrame(JSThread *thread);
};
}  // namespace panda::ecmascript
#endif
