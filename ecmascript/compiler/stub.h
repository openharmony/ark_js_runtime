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
#define DEFVARIABLE(varname, type, val) Stub::Variable varname(GetEnvironment(), type, NextVariableId(), val)
#define NOGC_RTSTUB_CSIGNS_BEGIN (CommonStubCSigns::NUM_OF_STUBS + BytecodeStubCSigns::NUM_OF_VALID_STUBS)
class Stub {
public:
    class Environment;
    class Label;
    class Variable;

    class Label {
    public:
        class LabelImpl {
        public:
            LabelImpl(Environment *env, GateRef control)
                : env_(env), control_(control), predeControl_(-1), isSealed_(false)
            {
            }
            ~LabelImpl() = default;
            NO_MOVE_SEMANTIC(LabelImpl);
            NO_COPY_SEMANTIC(LabelImpl);
            void Seal();
            void WriteVariable(Variable *var, GateRef value);
            GateRef ReadVariable(Variable *var);
            void Bind();
            void MergeAllControl();
            void MergeAllDepend();
            void AppendPredecessor(LabelImpl *predecessor);
            std::vector<LabelImpl *> GetPredecessors() const
            {
                return predecessors_;
            }
            void SetControl(GateRef control)
            {
                control_ = control;
            }
            void SetPreControl(GateRef control)
            {
                predeControl_ = control;
            }
            void MergeControl(GateRef control)
            {
                if (predeControl_ == -1) {
                    predeControl_ = control;
                    control_ = predeControl_;
                } else {
                    otherPredeControls_.push_back(control);
                }
            }
            GateRef GetControl() const
            {
                return control_;
            }
            void SetDepend(GateRef depend)
            {
                depend_ = depend;
            }
            GateRef GetDepend() const
            {
                return depend_;
            }

        private:
            bool IsNeedSeal() const;
            bool IsSealed() const
            {
                return isSealed_;
            }
            bool IsLoopHead() const;
            bool IsControlCase() const;
            GateRef ReadVariableRecursive(Variable *var);
            Environment *env_;
            GateRef control_;
            GateRef predeControl_ {-1};
            GateRef dependRelay_ {-1};
            GateRef depend_ {-1};
            GateRef loopDepend_ {-1};
            std::vector<GateRef> otherPredeControls_;
            bool isSealed_ {false};
            std::map<Variable *, GateRef> valueMap_;
            std::vector<GateRef> phi;
            std::vector<LabelImpl *> predecessors_;
            std::map<Variable *, GateRef> incompletePhis_;
        };
        explicit Label() = default;
        explicit Label(Environment *env);
        explicit Label(LabelImpl *impl) : impl_(impl) {}
        ~Label() = default;
        Label(Label const &label) = default;
        Label &operator=(Label const &label) = default;
        Label(Label &&label) = default;
        Label &operator=(Label &&label) = default;
        void Seal();
        void WriteVariable(Variable *var, GateRef value);
        GateRef ReadVariable(Variable *var);
        void Bind();
        void MergeAllControl();
        void MergeAllDepend();
        void AppendPredecessor(const Label *predecessor);
        std::vector<Label> GetPredecessors() const;
        void SetControl(GateRef control);
        void SetPreControl(GateRef control);
        void MergeControl(GateRef control);
        GateRef GetControl() const;
        GateRef GetDepend() const;
        void SetDepend(GateRef depend);

    private:
        friend class Environment;
        LabelImpl *GetRawLabel() const
        {
            return impl_;
        }
        LabelImpl *impl_ {nullptr};
    };

    class Environment {
    public:
        using LabelImpl = Label::LabelImpl;
        explicit Environment(size_t arguments, Circuit *circuit);
        ~Environment();
        NO_COPY_SEMANTIC(Environment);
        NO_MOVE_SEMANTIC(Environment);
        Label *GetCurrentLabel() const
        {
            return currentLabel_;
        }
        void SetCurrentLabel(Label *label)
        {
            currentLabel_ = label;
        }
        CircuitBuilder &GetBuilder()
        {
            return builder_;
        }
        Circuit *GetCircuit()
        {
            return circuit_;
        }
        void SetCompilationConfig(const CompilationConfig *cfg)
        {
            compCfg_ = cfg;
        }

        const CompilationConfig *GetCompilationConfig() const
        {
            return compCfg_;
        }

        inline bool Is32Bit() const
        {
            return compCfg_->Is32Bit();
        }

        inline bool IsAArch64() const
        {
            return compCfg_->IsAArch64();
        }

        inline bool IsAmd64() const
        {
            return compCfg_->IsAmd64();
        }

        inline bool IsArch64Bit() const
        {
            return compCfg_->IsAmd64() ||  compCfg_->IsAArch64();
        }

        inline bool IsArch32Bit() const
        {
            return compCfg_->Is32Bit();
        }

        GateType GetGateType(GateRef gate) const;
        Label GetLabelFromSelector(GateRef sel);
        void AddSelectorToLabel(GateRef sel, Label label);
        LabelImpl *NewLabel(Environment *env, GateRef control = -1);
        void PushCurrentLabel(Label *entry);
        void PopCurrentLabel();
        void SetFrameType(FrameType type);
        GateRef GetArgument(size_t index) const;

    private:
        const CompilationConfig *compCfg_;
        Label *currentLabel_ {nullptr};
        Circuit *circuit_;
        CircuitBuilder builder_;
        std::unordered_map<GateRef, LabelImpl *> phiToLabels_;
        std::vector<GateRef> arguments_;
        Label entry_;
        std::vector<LabelImpl *> rawLabels_;
        std::stack<Label *> stack_;
    };

    class Variable {
    public:
        Variable(Environment *env, VariableType type, uint32_t id, GateRef value) : id_(id), type_(type), env_(env)
        {
            Bind(value);
            env_->GetCurrentLabel()->WriteVariable(this, value);
        }
        ~Variable() = default;
        NO_MOVE_SEMANTIC(Variable);
        NO_COPY_SEMANTIC(Variable);
        void Bind(GateRef value)
        {
            currentValue_ = value;
        }
        GateRef Value() const
        {
            return currentValue_;
        }
        VariableType Type() const
        {
            return type_;
        }
        bool IsBound() const
        {
            return currentValue_ != 0;
        }
        Variable &operator=(const GateRef value)
        {
            env_->GetCurrentLabel()->WriteVariable(this, value);
            Bind(value);
            return *this;
        }
        GateRef operator*()
        {
            return env_->GetCurrentLabel()->ReadVariable(this);
        }
        GateRef AddPhiOperand(GateRef val);
        GateRef AddOperandToSelector(GateRef val, size_t idx, GateRef in);
        GateRef TryRemoveTrivialPhi(GateRef phi);
        void RerouteOuts(const std::vector<Out *> &outs, Gate *newGate);
        bool IsSelector(GateRef gate) const
        {
            return env_->GetCircuit()->IsSelector(gate);
        }
        bool IsSelector(const Gate *gate) const
        {
            return gate->GetOpCode() == OpCode::VALUE_SELECTOR;
        }
        uint32_t GetId() const
        {
            return id_;
        }

    private:
        uint32_t id_;
        VariableType type_;
        GateRef currentValue_ {0};
        Environment *env_;
    };

    class SubCircuitScope {
    public:
        explicit SubCircuitScope(Environment *env, Label *entry) : env_(env)
        {
            env_->PushCurrentLabel(entry);
        }
        ~SubCircuitScope()
        {
            env_->PopCurrentLabel();
        }

    private:
        Environment *env_;
    };

    explicit Stub(const char *name, int argCount, Circuit *circuit)
        : env_(argCount, circuit), methodName_(name)
    {
    }
    virtual ~Stub() = default;
    NO_MOVE_SEMANTIC(Stub);
    NO_COPY_SEMANTIC(Stub);
    virtual void GenerateCircuit(const CompilationConfig *cfg)
    {
        env_.SetCompilationConfig(cfg);
    }
    Environment *GetEnvironment()
    {
        return &env_;
    }
    int NextVariableId()
    {
        return nextVariableId_++;
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
    GateRef IntPtrMul(GateRef x, GateRef y);
    // parameter
    GateRef Argument(size_t index);
    GateRef Int1Argument(size_t index);
    GateRef Int32Argument(size_t index);
    GateRef Int64Argument(size_t index);
    GateRef TaggedArgument(size_t index);
    GateRef TaggedPointerArgument(size_t index, GateType type = GateType::TAGGED_POINTER);
    GateRef PtrArgument(size_t index, GateType type = GateType::C_VALUE);
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
    GateRef CallRuntime(GateRef glue, int index, std::initializer_list<GateRef> args);
    GateRef CallNGCRuntime(GateRef glue, size_t index, std::initializer_list<GateRef> args);
    GateRef CallStub(GateRef glue, size_t index, std::initializer_list<GateRef> args);
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
    GateRef IntPtrAdd(GateRef x, GateRef y);
    GateRef IntPtrSub(GateRef x, GateRef y);
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
    GateRef UInt32Div(GateRef x, GateRef y);
    GateRef UInt64Div(GateRef x, GateRef y);
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
    GateRef UInt64LSL(GateRef x, GateRef y);
    GateRef IntPtrLSL(GateRef x, GateRef y);
    GateRef Int8LSR(GateRef x, GateRef y);
    GateRef UInt32LSR(GateRef x, GateRef y);
    GateRef UInt64LSR(GateRef x, GateRef y);
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
    GateRef UInt32GreaterThan(GateRef x, GateRef y);
    GateRef UInt32LessThan(GateRef x, GateRef y);
    GateRef UInt32LessThanOrEqual(GateRef x, GateRef y);
    GateRef UInt32GreaterThanOrEqual(GateRef x, GateRef y);
    GateRef Int64GreaterThan(GateRef x, GateRef y);
    GateRef Int64LessThan(GateRef x, GateRef y);
    GateRef Int64LessThanOrEqual(GateRef x, GateRef y);
    GateRef Int64GreaterThanOrEqual(GateRef x, GateRef y);
    GateRef UInt6464GreaterThan(GateRef x, GateRef y);
    GateRef UInt64LessThan(GateRef x, GateRef y);
    GateRef UInt64LessThanOrEqual(GateRef x, GateRef y);
    GateRef UInt6464GreaterThanOrEqual(GateRef x, GateRef y);
    // cast operation
    GateRef ChangeInt64ToInt32(GateRef val);
    GateRef ChangeInt64ToIntPtr(GateRef val);
    GateRef ChangeInt32ToIntPtr(GateRef val);
    // math operation
    GateRef Sqrt(GateRef x);
    GateRef GetSetterFromAccessor(GateRef accessor);
    GateRef GetElementsArray(GateRef object);
    void SetElementsArray(GateRef glue, GateRef object, GateRef elementsArray);
    GateRef GetPropertiesArray(GateRef object);
    // SetProperties in js_object.h
    void SetPropertiesArray(GateRef glue, GateRef object, GateRef propsArray);
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
    GateRef NotBuiltinsConstructor(GateRef object);
    GateRef IsClassConstructor(GateRef object);
    GateRef IsClassPrototype(GateRef object);
    GateRef IsExtensible(GateRef object);
    GateRef IsEcmaObject(GateRef obj);
    GateRef IsSymbol(GateRef obj);
    GateRef IsString(GateRef obj);
    GateRef IsBigInt(GateRef obj);
    GateRef IsJsProxy(GateRef obj);
    GateRef IsJSFunctionBase(GateRef obj);
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
    GateRef CallSetterUtil(GateRef glue, GateRef holder, GateRef accessor,  GateRef value);
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
    GateRef IsCallable(GateRef obj);
    GateRef GetOffsetFieldInPropAttr(GateRef attr);
    GateRef SetOffsetFieldInPropAttr(GateRef attr, GateRef value);
    GateRef SetIsInlinePropsFieldInPropAttr(GateRef attr, GateRef value);
    void SetHasConstructorToHClass(GateRef glue, GateRef hClass, GateRef value);
    void UpdateValueInDict(GateRef glue, GateRef elements, GateRef index, GateRef value);
    GateRef GetBitMask(GateRef bitoffset);
    GateRef IntptrEuqal(GateRef x, GateRef y);
    void SetValueWithBarrier(GateRef glue, GateRef obj, GateRef offset, GateRef value);
    GateRef GetPropertyByIndex(GateRef glue, GateRef receiver, GateRef index);
    GateRef GetPropertyByName(GateRef glue, GateRef receiver, GateRef key);
    GateRef SetPropertyByIndex(GateRef glue, GateRef receiver, GateRef index, GateRef value);
    GateRef SetPropertyByName(GateRef glue, GateRef receiver, GateRef key,
                               GateRef value); // Crawl prototype chain

    GateRef SetPropertyByNameWithOwn(GateRef glue, GateRef receiver, GateRef key,
                               GateRef value); // Do not crawl the prototype chain
    GateRef GetParentEnv(GateRef object);
    GateRef GetPropertiesFromLexicalEnv(GateRef object, GateRef index);
    void SetPropertiesToLexicalEnv(GateRef glue, GateRef object, GateRef index, GateRef value);
    GateRef GetFunctionBitFieldFromJSFunction(GateRef object);
    GateRef GetHomeObjectFromJSFunction(GateRef object);
    GateRef GetMethodFromJSFunction(GateRef object);
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
    GateRef FastMod(GateRef gule, GateRef left, GateRef right);
    GateRef FastTypeOf(GateRef left, GateRef right);
    GateRef FastMul(GateRef left, GateRef right);
    GateRef FastDiv(GateRef left, GateRef right);
    GateRef FastAdd(GateRef left, GateRef right);
    GateRef FastSub(GateRef left, GateRef right);
    GateRef FastToBoolean(GateRef value);

    // Add SpecialContainer
    GateRef GetContainerProperty(GateRef glue, GateRef receiver, GateRef index, GateRef jsType);
    GateRef JSArrayListGet(GateRef glue, GateRef receiver, GateRef index);
private:
    using BinaryOperation = std::function<GateRef(Environment*, GateRef, GateRef)>;
    template<OpCode::Op Op>
    GateRef FastAddSubAndMul(GateRef left, GateRef right);
    GateRef FastBinaryOp(GateRef left, GateRef right,
                         const BinaryOperation& intOp, const BinaryOperation& floatOp);
    Environment env_;
    std::string methodName_;
    int nextVariableId_ {0};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_STUB_H
