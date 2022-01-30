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

#ifndef ECMASCRIPT_INTERPRETER_SLOW_RUNTIME_STUB_H
#define ECMASCRIPT_INTERPRETER_SLOW_RUNTIME_STUB_H

#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
class GlobalEnv;
class JSArray;

class SlowRuntimeStub {
public:
    /* -------------- Common API Begin, Don't change those interface!!! ----------------- */
    static JSTaggedValue CallSpreadDyn(JSThread *thread, JSTaggedValue func, JSTaggedValue obj, JSTaggedValue array);
    static JSTaggedValue NegDyn(JSThread *thread, JSTaggedValue value);
    static JSTaggedValue AsyncFunctionEnter(JSThread *thread);
    static JSTaggedValue ToNumber(JSThread *thread, JSTaggedValue value);
    static JSTaggedValue NotDyn(JSThread *thread, JSTaggedValue value);
    static JSTaggedValue IncDyn(JSThread *thread, JSTaggedValue value);
    static JSTaggedValue DecDyn(JSThread *thread, JSTaggedValue value);
    static void ThrowDyn(JSThread *thread, JSTaggedValue value);
    static JSTaggedValue GetPropIterator(JSThread *thread, JSTaggedValue value);
    static void ThrowConstAssignment(JSThread *thread, JSTaggedValue value);
    static JSTaggedValue Add2Dyn(JSThread *thread, EcmaVM *ecma_vm, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue Sub2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue Mul2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue Div2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue Mod2Dyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue EqDyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue NotEqDyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue LessDyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue LessEqDyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue GreaterDyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right);
    static JSTaggedValue GreaterEqDyn(JSThread *thread, JSTaggedValue left, JSTaggedValue right);

    static JSTaggedValue ToJSTaggedValueWithInt32(JSThread *thread, JSTaggedValue value);
    static JSTaggedValue ToJSTaggedValueWithUint32(JSThread *thread, JSTaggedValue value);

    static JSTaggedValue DelObjProp(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop);
    static JSTaggedValue NewObjDynRange(JSThread *thread, JSTaggedValue func, JSTaggedValue newTarget,
                                        uint16_t firstArgIdx, uint16_t length);
    static JSTaggedValue CreateObjectWithExcludedKeys(JSThread *thread, uint16_t numKeys, JSTaggedValue objVal,
                                                      uint16_t firstArgRegIdx);
    static JSTaggedValue ExpDyn(JSThread *thread, JSTaggedValue base, JSTaggedValue exponent);
    static JSTaggedValue IsInDyn(JSThread *thread, JSTaggedValue prop, JSTaggedValue obj);
    static JSTaggedValue InstanceofDyn(JSThread *thread, JSTaggedValue obj, JSTaggedValue target);

    static JSTaggedValue NewLexicalEnvDyn(JSThread *thread, uint16_t numVars);
    static JSTaggedValue NewLexicalEnvWithNameDyn(JSThread *thread, uint16_t numVars, uint16_t scopeId);
    static JSTaggedValue CreateIterResultObj(JSThread *thread, JSTaggedValue value, JSTaggedValue flag);

    static JSTaggedValue CreateGeneratorObj(JSThread *thread, JSTaggedValue genFunc);
    static JSTaggedValue SuspendGenerator(JSThread *thread, JSTaggedValue genObj, JSTaggedValue value);
    static JSTaggedValue AsyncFunctionAwaitUncaught(JSThread *thread, JSTaggedValue asyncFuncObj, JSTaggedValue value);
    static JSTaggedValue AsyncFunctionResolveOrReject(JSThread *thread, JSTaggedValue asyncFuncObj, JSTaggedValue value,
                                                      bool is_resolve);

    static JSTaggedValue NewObjSpreadDyn(JSThread *thread, JSTaggedValue func, JSTaggedValue newTarget,
                                         JSTaggedValue array);
    static void ThrowUndefinedIfHole(JSThread *thread, JSTaggedValue obj);
    static void ThrowIfNotObject(JSThread *thread);
    static void ThrowThrowNotExists(JSThread *thread);
    static void ThrowPatternNonCoercible(JSThread *thread);
    static JSTaggedValue ThrowIfSuperNotCorrectCall(JSThread *thread, uint16_t index, JSTaggedValue thisValue);
    static void ThrowDeleteSuperProperty(JSThread *thread);

    static JSTaggedValue StOwnByName(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop, JSTaggedValue value);
    static JSTaggedValue StOwnByNameWithNameSet(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop,
                                                JSTaggedValue value);
    static JSTaggedValue StOwnByIndex(JSThread *thread, JSTaggedValue obj, uint32_t idx, JSTaggedValue value);
    static JSTaggedValue StOwnByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue key, JSTaggedValue value);
    static JSTaggedValue StOwnByValueWithNameSet(JSThread *thread, JSTaggedValue obj, JSTaggedValue key,
                                                 JSTaggedValue value);
    static JSTaggedValue CreateEmptyArray(JSThread *thread, ObjectFactory *factory, JSHandle<GlobalEnv> globalEnv);
    static JSTaggedValue CreateEmptyObject(JSThread *thread, ObjectFactory *factory, JSHandle<GlobalEnv> globalEnv);
    static JSTaggedValue CreateObjectWithBuffer(JSThread *thread, ObjectFactory *factory, JSObject *literal);
    static JSTaggedValue CreateObjectHavingMethod(JSThread *thread, ObjectFactory *factory, JSObject *literal,
                                                  JSTaggedValue env, ConstantPool *constpool);
    static JSTaggedValue SetObjectWithProto(JSThread *thread, JSTaggedValue proto, JSTaggedValue obj);
    static JSTaggedValue CreateArrayWithBuffer(JSThread *thread, ObjectFactory *factory, JSArray *literal);

    static JSTaggedValue GetTemplateObject(JSThread *thread, JSTaggedValue literal);
    static JSTaggedValue GetNextPropName(JSThread *thread, JSTaggedValue iter);
    static JSTaggedValue CopyDataProperties(JSThread *thread, JSTaggedValue dst, JSTaggedValue src);

    static JSTaggedValue GetUnmapedArgs(JSThread *thread, JSTaggedType *sp, uint32_t actualNumArgs, uint32_t startIdx);
    static JSTaggedValue CopyRestArgs(JSThread *thread, JSTaggedType *sp, uint32_t restNumArgs, uint32_t startIdx);
    static JSTaggedValue GetIterator(JSThread *thread, JSTaggedValue obj);
    static JSTaggedValue IterNext(JSThread *thread, JSTaggedValue iter);
    static JSTaggedValue CloseIterator(JSThread *thread, JSTaggedValue iter);
    static void StModuleVar(JSThread *thread, JSTaggedValue key, JSTaggedValue value);
    static JSTaggedValue LdModuleVar(JSThread *thread, JSTaggedValue key, bool inner);
    static JSTaggedValue CreateRegExpWithLiteral(JSThread *thread, JSTaggedValue pattern, uint8_t flags);
    static JSTaggedValue GetIteratorNext(JSThread *thread, JSTaggedValue obj, JSTaggedValue method);

    static JSTaggedValue DefineGetterSetterByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop,
                                                   JSTaggedValue getter, JSTaggedValue setter, bool flag);

    static JSTaggedValue LdObjByIndex(JSThread *thread, JSTaggedValue obj, uint32_t idx, bool callGetter,
                                      JSTaggedValue receiver);
    static JSTaggedValue StObjByIndex(JSThread *thread, JSTaggedValue obj, uint32_t idx, JSTaggedValue value);
    static JSTaggedValue LdObjByName(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop, bool callGetter,
                                     JSTaggedValue receiver);
    static JSTaggedValue StObjByName(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop, JSTaggedValue value);
    static JSTaggedValue LdObjByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop, bool callGetter,
                                      JSTaggedValue receiver);
    static JSTaggedValue StObjByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue prop, JSTaggedValue value);
    static JSTaggedValue TryLdGlobalByName(JSThread *thread, JSTaggedValue global, JSTaggedValue prop);
    static JSTaggedValue TryStGlobalByName(JSThread *thread, JSTaggedValue prop);
    static JSTaggedValue LdGlobalVar(JSThread *thread, JSTaggedValue global, JSTaggedValue prop);
    static JSTaggedValue StGlobalVar(JSThread *thread, JSTaggedValue prop, JSTaggedValue value);
    static JSTaggedValue StGlobalRecord(JSThread *thread, JSTaggedValue prop, JSTaggedValue value, bool isConst);
    static JSTaggedValue LdGlobalRecord(JSThread *thread, JSTaggedValue key);
    static JSTaggedValue TryUpdateGlobalRecord(JSThread *thread, JSTaggedValue prop, JSTaggedValue value);
    static JSTaggedValue StArraySpread(JSThread *thread, JSTaggedValue dst, JSTaggedValue index, JSTaggedValue src);

    static JSTaggedValue DefineGeneratorFunc(JSThread *thread, JSFunction *func);
    static JSTaggedValue DefineAsyncFunc(JSThread *thread, JSFunction *func);
    static JSTaggedValue DefineNCFuncDyn(JSThread *thread, JSFunction *func);
    static JSTaggedValue DefinefuncDyn(JSThread *thread, JSFunction *func);

    static JSTaggedValue SuperCall(JSThread *thread, JSTaggedValue func, JSTaggedValue newTarget, uint16_t firstVRegIdx,
                                   uint16_t length);
    static JSTaggedValue SuperCallSpread(JSThread *thread, JSTaggedValue func, JSTaggedValue newTarget,
                                         JSTaggedValue array);
    static JSTaggedValue DefineMethod(JSThread *thread, JSFunction *func, JSTaggedValue homeObject);
    static JSTaggedValue LdSuperByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue key, JSTaggedValue thisFunc);
    static JSTaggedValue StSuperByValue(JSThread *thread, JSTaggedValue obj, JSTaggedValue key, JSTaggedValue value,
                                        JSTaggedValue thisFunc);
    static JSTaggedValue NotifyInlineCache(JSThread *thread, JSFunction *func, JSMethod *method);
    static JSTaggedValue ThrowReferenceError(JSThread *thread, JSTaggedValue prop, const char *desc);

    static JSTaggedValue ResolveClass(JSThread *thread, JSTaggedValue ctor, TaggedArray *literal, JSTaggedValue base,
                                      JSTaggedValue lexenv, ConstantPool *constpool);
    static JSTaggedValue CloneClassFromTemplate(JSThread *thread, JSTaggedValue ctor, JSTaggedValue base,
                                                JSTaggedValue lexenv, ConstantPool *constpool);
    static JSTaggedValue SetClassConstructorLength(JSThread *thread, JSTaggedValue ctor, JSTaggedValue length);
    static JSTaggedValue GetModuleNamespace(JSThread *thread, JSTaggedValue localName);
    /* -------------- Common API End, Don't change those interface!!! ----------------- */

private:
    static JSTaggedValue ThrowTypeError(JSThread *thread, const char *message);
    static JSTaggedValue ThrowSyntaxError(JSThread *thread, const char *message);
    static JSTaggedValue GetCallSpreadArgs(JSThread *thread, JSTaggedValue array);
    static JSTaggedValue SetClassInheritanceRelationship(JSThread *thread, JSTaggedValue ctor, JSTaggedValue base);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_INTERPRETER_SLOW_RUNTIME_STUB_H
