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
#include "ecmascript/ecma_macros.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/trampoline/runtime_define.h"

namespace panda::ecmascript {
extern "C" JSTaggedType RuntimeCallTrampolineAot(uintptr_t glue, uint64_t runtime_id,
                                                 uint64_t argc, ...);
extern "C" JSTaggedType RuntimeCallTrampolineInterpreterAsm(uintptr_t glue, uint64_t runtime_id,
                                                            uint64_t argc, ...);
class RuntimeTrampolines {
public:
    static void InitializeRuntimeTrampolines(JSThread *thread)
    {
    #define DEF_RUNTIME_STUB(name, counter) RuntimeTrampolineId::RUNTIME_ID_##name
    #define INITIAL_RUNTIME_FUNCTIONS(name, count) \
        thread->SetRuntimeFunction(DEF_RUNTIME_STUB(name, count), reinterpret_cast<uintptr_t>(name));
        ALL_RUNTIME_CALL_LIST(INITIAL_RUNTIME_FUNCTIONS)
    #undef INITIAL_RUNTIME_FUNCTIONS
    #undef DEF_RUNTIME_STUB
    }

#define DECLARE_RUNTIME_TRAMPOLINES(name, counter) \
    static JSTaggedType name(uintptr_t argGlue, uint32_t argc, uintptr_t argv);
    RUNTIME_CALL_LIST(DECLARE_RUNTIME_TRAMPOLINES)
#undef DECLARE_RUNTIME_TRAMPOLINES

    static void DebugPrint(int fmtMessageId, ...);
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
    static inline JSTaggedValue RuntimeAdd2Dyn(JSThread *thread, EcmaVM *ecma_vm, const JSHandle<JSTaggedValue> &left,
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
};
}  // namespace panda::ecmascript
#endif
