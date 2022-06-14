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

#ifndef ECMASCRIPT_COMPILER_STUB_H
#define ECMASCRIPT_COMPILER_STUB_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/variable_type.h"
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/global_env_constants.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEFVARIABLE(varname, type, val) Variable varname(GetEnvironment(), type, NextVariableId(), val)

class Stub {
public:
    explicit Stub(const char *name, int argCount, Circuit *circuit)
        : builder_(circuit), env_(argCount, &builder_), methodName_(name)
    {
    }
    virtual ~Stub() = default;
    NO_MOVE_SEMANTIC(Stub);
    NO_COPY_SEMANTIC(Stub);
    virtual void GenerateCircuit(const CompilationConfig *cfg)
    {
        env_.SetCompilationConfig(cfg);
    }
    CircuitBuilder* GetBuilder()
    {
        return &builder_;
    }
    Environment *GetEnvironment()
    {
        return &env_;
    }
    int NextVariableId()
    {
        return env_.NextVariableId();
    }
    std::string GetMethodName() const
    {
        return methodName_;
    }
    // constant
    GateRef Int8(int8_t value);
    GateRef Int16(int16_t value);
    GateRef Int32(int32_t value);
    GateRef Int64(int64_t value);
    GateRef IntPtr(int64_t value);
    GateRef IntPtrSize();
    GateRef RelocatableData(uint64_t value);
    GateRef True();
    GateRef False();
    GateRef Boolean(bool value);
    GateRef Double(double value);
    GateRef Undefined(VariableType type = VariableType::JS_ANY());
    GateRef Hole(VariableType type = VariableType::JS_ANY());
    GateRef Null(VariableType type = VariableType::JS_ANY());
    GateRef Exception(VariableType type = VariableType::JS_ANY());
    GateRef PtrMul(GateRef x, GateRef y);
    // parameter
    GateRef Argument(size_t index);
    GateRef Int1Argument(size_t index);
    GateRef Int32Argument(size_t index);
    GateRef Int64Argument(size_t index);
    GateRef TaggedArgument(size_t index);
    GateRef TaggedPointerArgument(size_t index, GateType type = GateType::TaggedPointer());
    GateRef PtrArgument(size_t index, GateType type = GateType::NJSValue());
    GateRef Float32Argument(size_t index);
    GateRef Float64Argument(size_t index);
    GateRef Alloca(int size);
    // control flow
    GateRef Return(GateRef value);
    GateRef Return();
    void Bind(Label *label);
    void Jump(Label *label);
    void Branch(GateRef condition, Label *trueLabel, Label *falseLabel);
    void Switch(GateRef index, Label *defaultLabel, int64_t *keysValue, Label *keysLabel, int numberOfKeys);
    void LoopBegin(Label *loopHead);
    void LoopEnd(Label *loopHead);
    // call operation
    GateRef CallRuntime(GateRef glue, int index, const std::initializer_list<GateRef>& args);
    GateRef CallRuntime(GateRef glue, int index, GateRef argc, GateRef argv);
    GateRef CallNGCRuntime(GateRef glue, int index, const std::initializer_list<GateRef>& args);
    GateRef CallStub(GateRef glue, int index, const std::initializer_list<GateRef>& args);
    void DebugPrint(GateRef thread, std::initializer_list<GateRef> args);
    void FatalPrint(GateRef thread, std::initializer_list<GateRef> args);
    // memory
    GateRef Load(VariableType type, GateRef base, GateRef offset);
    GateRef Load(VariableType type, GateRef base);
    void Store(VariableType type, GateRef glue, GateRef base, GateRef offset, GateRef value);
    // arithmetic
    GateRef TaggedCastToIntPtr(GateRef x);
    GateRef Int16Add(GateRef x, GateRef y);
    GateRef Int32Add(GateRef x, GateRef y);
    GateRef Int64Add(GateRef x, GateRef y);
    GateRef DoubleAdd(GateRef x, GateRef y);
    GateRef PtrAdd(GateRef x, GateRef y);
    GateRef PtrSub(GateRef x, GateRef y);
    GateRef PointerSub(GateRef x, GateRef y);
    GateRef IntPtrEqual(GateRef x, GateRef y);
    GateRef Int16Sub(GateRef x, GateRef y);
    GateRef Int32Sub(GateRef x, GateRef y);
    GateRef Int64Sub(GateRef x, GateRef y);
    GateRef DoubleSub(GateRef x, GateRef y);
    GateRef Int32Mul(GateRef x, GateRef y);
    GateRef Int64Mul(GateRef x, GateRef y);
    GateRef DoubleMul(GateRef x, GateRef y);
    GateRef DoubleDiv(GateRef x, GateRef y);
    GateRef Int32Div(GateRef x, GateRef y);
    GateRef Int32Mod(GateRef x, GateRef y);
    GateRef DoubleMod(GateRef x, GateRef y);
    GateRef Int64Div(GateRef x, GateRef y);
    GateRef IntPtrDiv(GateRef x, GateRef y);
    // bit operation
    GateRef Int32Or(GateRef x, GateRef y);
    GateRef Int8And(GateRef x, GateRef y);
    GateRef Int32And(GateRef x, GateRef y);
    GateRef IntPtrAnd(GateRef x, GateRef y);
    GateRef BoolAnd(GateRef x, GateRef y);
    GateRef BoolOr(GateRef x, GateRef y);
    GateRef Int32Not(GateRef x);
    GateRef BoolNot(GateRef x);
    GateRef Int32Xor(GateRef x, GateRef y);
    GateRef FixLoadType(GateRef x);
    GateRef Int64Or(GateRef x, GateRef y);
    GateRef IntPtrOr(GateRef x, GateRef y);
    GateRef Int64And(GateRef x, GateRef y);
    GateRef Int64Xor(GateRef x, GateRef y);
    GateRef Int64Not(GateRef x);
    GateRef Int16LSL(GateRef x, GateRef y);
    GateRef Int32LSL(GateRef x, GateRef y);
    GateRef Int64LSL(GateRef x, GateRef y);
    GateRef IntPtrLSL(GateRef x, GateRef y);
    GateRef Int8LSR(GateRef x, GateRef y);
    GateRef Int32LSR(GateRef x, GateRef y);
    GateRef Int64LSR(GateRef x, GateRef y);
    GateRef IntPtrLSR(GateRef x, GateRef y);
    GateRef Int32ASR(GateRef x, GateRef y);
    GateRef TaggedIsInt(GateRef x);
    GateRef TaggedIsDouble(GateRef x);
    GateRef TaggedIsObject(GateRef x);
    GateRef TaggedIsNumber(GateRef x);
    GateRef TaggedIsHole(GateRef x);
    GateRef TaggedIsNotHole(GateRef x);
    GateRef TaggedIsUndefined(GateRef x);
    GateRef TaggedIsException(GateRef x);
    GateRef TaggedIsSpecial(GateRef x);
    GateRef TaggedIsHeapObject(GateRef x);
    GateRef ObjectAddressToRange(GateRef x);
    GateRef InYoungGeneration(GateRef x);
    GateRef TaggedIsGeneratorObject(GateRef x);
    GateRef TaggedIsPropertyBox(GateRef x);
    GateRef TaggedIsWeak(GateRef x);
    GateRef TaggedIsPrototypeHandler(GateRef x);
    GateRef TaggedIsTransitionHandler(GateRef x);
    GateRef TaggedIsString(GateRef obj);
    GateRef TaggedIsStringOrSymbol(GateRef obj);
    GateRef GetNextPositionForHash(GateRef last, GateRef count, GateRef size);
    GateRef DoubleIsNAN(GateRef x);
    GateRef DoubleIsINF(GateRef x);
    GateRef TaggedIsNull(GateRef x);
    GateRef TaggedIsUndefinedOrNull(GateRef x);
    GateRef TaggedIsTrue(GateRef x);
    GateRef TaggedIsFalse(GateRef x);
    GateRef TaggedIsBoolean(GateRef x);
    GateRef TaggedGetInt(GateRef x);
    GateRef Int8BuildTaggedTypeWithNoGC(GateRef x);
    GateRef Int16BuildTaggedWithNoGC(GateRef x);
    GateRef Int16BuildTaggedTypeWithNoGC(GateRef x);
    GateRef IntBuildTaggedWithNoGC(GateRef x);
    GateRef IntBuildTaggedTypeWithNoGC(GateRef x);
    GateRef DoubleBuildTaggedWithNoGC(GateRef x);
    GateRef DoubleBuildTaggedTypeWithNoGC(GateRef x);
    GateRef CastDoubleToInt64(GateRef x);
    GateRef TaggedTrue();
    GateRef TaggedFalse();
    // compare operation
    GateRef Int8Equal(GateRef x, GateRef y);
    GateRef Int32Equal(GateRef x, GateRef y);
    GateRef Int32NotEqual(GateRef x, GateRef y);
    GateRef Int64Equal(GateRef x, GateRef y);
    GateRef DoubleEqual(GateRef x, GateRef y);
    GateRef Int64NotEqual(GateRef x, GateRef y);
    GateRef DoubleLessThan(GateRef x, GateRef y);
    GateRef DoubleLessThanOrEqual(GateRef x, GateRef y);
    GateRef DoubleGreaterThan(GateRef x, GateRef y);
    GateRef DoubleGreaterThanOrEqual(GateRef x, GateRef y);
    GateRef Int32GreaterThan(GateRef x, GateRef y);
    GateRef Int32LessThan(GateRef x, GateRef y);
    GateRef Int32GreaterThanOrEqual(GateRef x, GateRef y);
    GateRef Int32LessThanOrEqual(GateRef x, GateRef y);
    GateRef Int32UnsignedGreaterThan(GateRef x, GateRef y);
    GateRef Int32UnsignedLessThan(GateRef x, GateRef y);
    GateRef Int32UnsignedGreaterThanOrEqual(GateRef x, GateRef y);
    GateRef Int64GreaterThan(GateRef x, GateRef y);
    GateRef Int64LessThan(GateRef x, GateRef y);
    GateRef Int64LessThanOrEqual(GateRef x, GateRef y);
    GateRef Int64GreaterThanOrEqual(GateRef x, GateRef y);
    GateRef Int64UnsignedLessThanOrEqual(GateRef x, GateRef y);
    GateRef IntPtrGreaterThan(GateRef x, GateRef y);
    // cast operation
    GateRef ChangeInt64ToInt32(GateRef val);
    GateRef ChangeInt64ToIntPtr(GateRef val);
    GateRef ChangeInt32ToIntPtr(GateRef val);
    GateRef ChangeIntPtrToInt32(GateRef val);
    // math operation
    GateRef Sqrt(GateRef x);
    GateRef GetSetterFromAccessor(GateRef accessor);
    GateRef GetElementsArray(GateRef object);
    void SetElementsArray(VariableType type, GateRef glue, GateRef object, GateRef elementsArray);
    GateRef GetPropertiesArray(GateRef object);
    // SetProperties in js_object.h
    void SetPropertiesArray(VariableType type, GateRef glue, GateRef object, GateRef propsArray);
    void SetHash(GateRef glue, GateRef object, GateRef hash);
    GateRef GetLengthOfTaggedArray(GateRef array);
    // object operation
    GateRef IsJSHClass(GateRef obj);
    GateRef LoadHClass(GateRef object);
    void StoreHClass(GateRef glue, GateRef object, GateRef hclass);
    void CopyAllHClass(GateRef glue, GateRef dstHClass, GateRef scrHClass);
    GateRef GetObjectType(GateRef hClass);
    GateRef IsDictionaryMode(GateRef object);
    GateRef IsDictionaryModeByHClass(GateRef hClass);
    GateRef IsDictionaryElement(GateRef hClass);
    GateRef IsClassConstructorFromBitField(GateRef bitfield);
    GateRef IsClassConstructor(GateRef object);
    GateRef IsClassPrototype(GateRef object);
    GateRef IsExtensible(GateRef object);
    GateRef TaggedObjectIsEcmaObject(GateRef obj);
    GateRef IsSymbol(GateRef obj);
    GateRef IsString(GateRef obj);
    GateRef TaggedIsBigInt(GateRef obj);
    GateRef IsBigInt(GateRef obj);
    GateRef IsJsProxy(GateRef obj);
    GateRef IsJSFunctionBase(GateRef obj);
    GateRef IsConstructor(GateRef object);
    GateRef IsBase(GateRef func);
    GateRef IsJsArray(GateRef obj);
    GateRef IsJSObject(GateRef obj);
    GateRef IsWritable(GateRef attr);
    GateRef IsAccessor(GateRef attr);
    GateRef IsInlinedProperty(GateRef attr);
    GateRef IsField(GateRef attr);
    GateRef IsNonExist(GateRef attr);
    GateRef HandlerBaseIsAccessor(GateRef attr);
    GateRef HandlerBaseIsJSArray(GateRef attr);
    GateRef HandlerBaseIsInlinedProperty(GateRef attr);
    GateRef HandlerBaseGetOffset(GateRef attr);
    GateRef IsInvalidPropertyBox(GateRef obj);
    GateRef GetValueFromPropertyBox(GateRef obj);
    void SetValueToPropertyBox(GateRef glue, GateRef obj, GateRef value);
    GateRef GetTransitionFromHClass(GateRef obj);
    GateRef GetTransitionHandlerInfo(GateRef obj);
    GateRef IsInternalAccessor(GateRef attr);
    GateRef GetProtoCell(GateRef object);
    GateRef GetPrototypeHandlerHolder(GateRef object);
    GateRef GetPrototypeHandlerHandlerInfo(GateRef object);
    GateRef GetHasChanged(GateRef object);
    GateRef HclassIsPrototypeHandler(GateRef hclass);
    GateRef HclassIsTransitionHandler(GateRef hclass);
    GateRef HclassIsPropertyBox(GateRef hclass);
    GateRef PropAttrGetOffset(GateRef attr);
    // SetDictionaryOrder func in property_attribute.h
    GateRef SetDictionaryOrderFieldInPropAttr(GateRef attr, GateRef value);
    GateRef GetPrototypeFromHClass(GateRef hClass);
    GateRef GetLayoutFromHClass(GateRef hClass);
    GateRef GetBitFieldFromHClass(GateRef hClass);
    GateRef GetLengthFromString(GateRef value);
    GateRef GetHashcodeFromString(GateRef glue, GateRef value);
    void SetBitFieldToHClass(GateRef glue, GateRef hClass, GateRef bitfield);
    void SetPrototypeToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef proto);
    void SetProtoChangeDetailsToHClass(VariableType type, GateRef glue, GateRef hClass,
	                                               GateRef protoChange);
    void SetLayoutToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef attr);
    void SetEnumCacheToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef key);
    void SetTransitionsToHClass(VariableType type, GateRef glue, GateRef hClass, GateRef transition);
    void SetIsProtoTypeToHClass(GateRef glue, GateRef hClass, GateRef value);
    GateRef IsProtoTypeHClass(GateRef hClass);
    void SetPropertyInlinedProps(GateRef glue, GateRef obj, GateRef hClass,
        GateRef value, GateRef attrOffset, VariableType type = VariableType::JS_ANY());

    void IncNumberOfProps(GateRef glue, GateRef hClass);
    GateRef GetNumberOfPropsFromHClass(GateRef hClass);
    void SetNumberOfPropsToHClass(GateRef glue, GateRef hClass, GateRef value);
    GateRef GetObjectSizeFromHClass(GateRef hClass);
    GateRef GetInlinedPropsStartFromHClass(GateRef hClass);
    GateRef GetInlinedPropertiesFromHClass(GateRef hClass);
    void ThrowTypeAndReturn(GateRef glue, int messageId, GateRef val);
    GateRef GetValueFromTaggedArray(VariableType returnType, GateRef elements, GateRef index);
    void SetValueToTaggedArray(VariableType valType, GateRef glue, GateRef array, GateRef index, GateRef val);
    void UpdateValueAndAttributes(GateRef glue, GateRef elements, GateRef index, GateRef value, GateRef attr);
    GateRef IsSpecialIndexedObj(GateRef jsType);
    GateRef IsSpecialContainer(GateRef jsType);
    GateRef IsAccessorInternal(GateRef value);
    template<typename DictionaryT>
    GateRef GetAttributesFromDictionary(GateRef elements, GateRef entry);
    template<typename DictionaryT>
    GateRef GetValueFromDictionary(VariableType returnType, GateRef elements, GateRef entry);
    template<typename DictionaryT>
    GateRef GetKeyFromDictionary(VariableType returnType, GateRef elements, GateRef entry);
    GateRef GetPropAttrFromLayoutInfo(GateRef layout, GateRef entry);
    GateRef GetPropertiesAddrFromLayoutInfo(GateRef layout);
    GateRef GetPropertyMetaDataFromAttr(GateRef attr);
    GateRef GetKeyFromLayoutInfo(GateRef layout, GateRef entry);
    GateRef FindElementWithCache(GateRef glue, GateRef layoutInfo, GateRef hClass,
        GateRef key, GateRef propsNum);
    GateRef FindElementFromNumberDictionary(GateRef glue, GateRef elements, GateRef index);
    GateRef FindEntryFromNameDictionary(GateRef glue, GateRef elements, GateRef key);
    GateRef IsMatchInTransitionDictionary(GateRef element, GateRef key, GateRef metaData, GateRef attr);
    GateRef FindEntryFromTransitionDictionary(GateRef glue, GateRef elements, GateRef key, GateRef metaData);
    GateRef JSObjectGetProperty(VariableType returnType, GateRef obj, GateRef hClass, GateRef propAttr);
    void JSObjectSetProperty(GateRef glue, GateRef obj, GateRef hClass, GateRef attr, GateRef value);
    GateRef ShouldCallSetter(GateRef receiver, GateRef holder, GateRef accessor, GateRef attr);
    GateRef CallSetterHelper(GateRef glue, GateRef holder, GateRef accessor,  GateRef value);
    GateRef SetHasConstructorCondition(GateRef glue, GateRef receiver, GateRef key);
    GateRef AddPropertyByName(GateRef glue, GateRef receiver, GateRef key, GateRef value, GateRef propertyAttributes);
    GateRef IsUtf16String(GateRef string);
    GateRef IsUtf8String(GateRef string);
    GateRef IsInternalString(GateRef string);
    GateRef IsDigit(GateRef ch);
    GateRef StringToElementIndex(GateRef string);
    GateRef TryToElementsIndex(GateRef key);
    GateRef ComputePropertyCapacityInJSObj(GateRef oldLength);
    GateRef FindTransitions(GateRef glue, GateRef receiver, GateRef hclass, GateRef key, GateRef attr);
    GateRef TaggedToRepresentation(GateRef value);
    GateRef LoadFromField(GateRef receiver, GateRef handlerInfo);
    GateRef LoadGlobal(GateRef cell);
    GateRef LoadElement(GateRef receiver, GateRef key);
    GateRef TryToElementsIndex(GateRef glue, GateRef key);
    GateRef CheckPolyHClass(GateRef cachedValue, GateRef hclass);
    GateRef LoadICWithHandler(GateRef glue, GateRef receiver, GateRef holder, GateRef handler);
    GateRef StoreICWithHandler(GateRef glue, GateRef receiver, GateRef holder,
                                 GateRef value, GateRef handler);
    GateRef ICStoreElement(GateRef glue, GateRef receiver, GateRef key,
                             GateRef value, GateRef handlerInfo);
    GateRef GetArrayLength(GateRef object);
    GateRef DoubleToInt(GateRef glue, GateRef x);
    void StoreField(GateRef glue, GateRef receiver, GateRef value, GateRef handler);
    void StoreWithTransition(GateRef glue, GateRef receiver, GateRef value, GateRef handler);
    GateRef StoreGlobal(GateRef glue, GateRef value, GateRef cell);
    void JSHClassAddProperty(GateRef glue, GateRef receiver, GateRef key, GateRef attr);
    void NotifyHClassChanged(GateRef glue, GateRef oldHClass, GateRef newHClass);
    GateRef TaggedCastToInt64(GateRef x);
    GateRef TaggedCastToInt32(GateRef x);
    GateRef TaggedCastToDouble(GateRef x);
    GateRef TaggedCastToWeakReferentUnChecked(GateRef x);
    GateRef ChangeInt32ToFloat64(GateRef x);
    GateRef ChangeUInt32ToFloat64(GateRef x);
    GateRef ChangeFloat64ToInt32(GateRef x);
    GateRef ChangeTaggedPointerToInt64(GateRef x);
    GateRef ChangeInt64ToTagged(GateRef x);
    GateRef CastInt64ToFloat64(GateRef x);
    GateRef SExtInt32ToInt64(GateRef x);
    GateRef SExtInt1ToInt64(GateRef x);
    GateRef SExtInt1ToInt32(GateRef x);
    GateRef ZExtInt8ToInt16(GateRef x);
    GateRef ZExtInt32ToInt64(GateRef x);
    GateRef ZExtInt1ToInt64(GateRef x);
    GateRef ZExtInt1ToInt32(GateRef x);
    GateRef ZExtInt8ToInt32(GateRef x);
    GateRef ZExtInt8ToInt64(GateRef x);
    GateRef ZExtInt8ToPtr(GateRef x);
    GateRef ZExtInt16ToPtr(GateRef x);
    GateRef SExtInt32ToPtr(GateRef x);
    GateRef ZExtInt16ToInt32(GateRef x);
    GateRef ZExtInt16ToInt64(GateRef x);
    GateRef TruncInt64ToInt32(GateRef x);
    GateRef TruncPtrToInt32(GateRef x);
    GateRef TruncInt64ToInt1(GateRef x);
    GateRef TruncInt32ToInt1(GateRef x);
    GateRef GetGlobalConstantAddr(GateRef index);
    GateRef GetGlobalConstantString(ConstantIndex index);
    GateRef IsCallableFromBitField(GateRef bitfield);
    GateRef IsCallable(GateRef obj);
    GateRef GetOffsetFieldInPropAttr(GateRef attr);
    GateRef SetOffsetFieldInPropAttr(GateRef attr, GateRef value);
    GateRef SetIsInlinePropsFieldInPropAttr(GateRef attr, GateRef value);
    void SetHasConstructorToHClass(GateRef glue, GateRef hClass, GateRef value);
    void UpdateValueInDict(GateRef glue, GateRef elements, GateRef index, GateRef value);
    GateRef GetBitMask(GateRef bitoffset);
    GateRef IntPtrEuqal(GateRef x, GateRef y);
    void SetValueWithBarrier(GateRef glue, GateRef obj, GateRef offset, GateRef value);
    GateRef GetPropertyByIndex(GateRef glue, GateRef receiver, GateRef index);
    GateRef GetPropertyByName(GateRef glue, GateRef receiver, GateRef key);
    GateRef SetPropertyByIndex(GateRef glue, GateRef receiver, GateRef index, GateRef value, bool useOwn);
    GateRef SetPropertyByName(GateRef glue, GateRef receiver, GateRef key,
        GateRef value, bool useOwn); // Crawl prototype chain
    GateRef SetPropertyByValue(GateRef glue, GateRef receiver, GateRef key, GateRef value, bool useOwn);
    GateRef GetParentEnv(GateRef object);
    GateRef GetPropertiesFromLexicalEnv(GateRef object, GateRef index);
    void SetPropertiesToLexicalEnv(GateRef glue, GateRef object, GateRef index, GateRef value);
    GateRef GetFunctionBitFieldFromJSFunction(GateRef object);
    GateRef GetHomeObjectFromJSFunction(GateRef object);
    GateRef GetCallFieldFromMethod(GateRef method);
    void SetLexicalEnvToFunction(GateRef glue, GateRef object, GateRef lexicalEnv);
    GateRef GetGlobalObject(GateRef glue);
    GateRef GetEntryIndexOfGlobalDictionary(GateRef entry);
    GateRef GetBoxFromGlobalDictionary(GateRef object, GateRef entry);
    GateRef GetValueFromGlobalDictionary(GateRef object, GateRef entry);
    GateRef GetPropertiesFromJSObject(GateRef object);
    template<OpCode::Op Op, MachineType Type>
    GateRef BinaryOp(GateRef x, GateRef y);
    GateRef GetGlobalOwnProperty(GateRef glue, GateRef receiver, GateRef key);
    // fast path
    GateRef FastEqual(GateRef left, GateRef right);
    GateRef FastStrictEqual(GateRef glue, GateRef left, GateRef right);
    GateRef FastMod(GateRef gule, GateRef left, GateRef right);
    GateRef FastTypeOf(GateRef left, GateRef right);
    GateRef FastMul(GateRef left, GateRef right);
    GateRef FastDiv(GateRef left, GateRef right);
    GateRef FastAdd(GateRef left, GateRef right);
    GateRef FastSub(GateRef left, GateRef right);
    GateRef FastToBoolean(GateRef value);

    // Add SpecialContainer
    GateRef GetContainerProperty(GateRef glue, GateRef receiver, GateRef index, GateRef jsType);
    GateRef JSAPIContainerGet(GateRef glue, GateRef receiver, GateRef index);

    // Exception handle
    GateRef HasPendingException(GateRef glue);
    void ReturnExceptionIfAbruptCompletion(GateRef glue);

    // method operator
    GateRef IsJSFunction(GateRef obj);
    GateRef IsBoundFunction(GateRef obj);
    GateRef GetMethodFromJSFunction(GateRef jsfunc);
    GateRef IsNativeMethod(GateRef method);
    GateRef HasAotCode(GateRef method);
    GateRef GetExpectedNumOfArgs(GateRef method);
    // proxy operator
    GateRef GetMethodFromJSProxy(GateRef proxy);
    GateRef GetHandlerFromJSProxy(GateRef proxy);
    GateRef GetTargetFromJSProxy(GateRef proxy);
    inline void SavePcIfNeeded(GateRef glue);
    inline GateRef ComputeTaggedArraySize(GateRef length);
    inline GateRef GetGlobalConstantValue(
        VariableType type, GateRef glue, ConstantIndex index);
    void InitializeTaggedArrayWithSpeicalValue(
        GateRef glue, GateRef array, GateRef value, GateRef start, GateRef length);
    void InitializeWithSpeicalValue(
        GateRef glue, GateRef object, GateRef value, GateRef start, GateRef end);
    GateRef AllocateInYoung(GateRef glue, GateRef size);
    GateRef NewLexicalEnv(GateRef glue, GateRef numSlots, GateRef parent);
    GateRef NewJSObject(GateRef glue, GateRef hclass);
    GateRef CallGetterHelper(GateRef glue, GateRef receiver, GateRef holder, GateRef accessor);
    GateRef JSCallDispatch(GateRef glue, GateRef func, GateRef actualNumArgs,
                           JSCallMode mode, std::initializer_list<GateRef> args);
private:
    using BinaryOperation = std::function<GateRef(Environment*, GateRef, GateRef)>;
    template<OpCode::Op Op>
    GateRef FastAddSubAndMul(GateRef left, GateRef right);
    GateRef FastBinaryOp(GateRef left, GateRef right,
                         const BinaryOperation& intOp, const BinaryOperation& floatOp);
    CircuitBuilder builder_;
    Environment env_;
    std::string methodName_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_STUB_H
