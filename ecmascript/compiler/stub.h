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

#include "ecmascript/accessor_data.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/machine_type.h"
#include "ecmascript/compiler/stub_descriptor.h"
#include "ecmascript/global_dictionary.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/layout_info.h"
#include "ecmascript/message_string.h"
#include "ecmascript/tagged_dictionary.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEFVARIABLE(varname, type, val) Stub::Variable varname(GetEnvironment(), type, NextVariableId(), val)

class CompilationConfig {
public:
    enum class Triple {
        TRIPLE_AMD64,
        TRIPLE_AARCH64,
        TRIPLE_ARM32,
    };

    explicit CompilationConfig(const std::string &triple)
        : triple_(GetTripleFromString(triple)),
          glueTable_(triple_)
    {
    }
    ~CompilationConfig() = default;

    inline bool Is32Bit() const
    {
        return triple_ == Triple::TRIPLE_ARM32;
    }

    inline bool IsAArch64() const
    {
        return triple_ == Triple::TRIPLE_AARCH64;
    }

    inline bool IsAmd64() const
    {
        return triple_ == Triple::TRIPLE_AMD64;
    }

    inline bool Is64Bit() const
    {
        return IsAArch64() || IsAmd64();
    }

    Triple GetTriple() const
    {
        return triple_;
    }

    uint32_t GetGlueOffset(JSThread::GlueID id) const
    {
        return glueTable_.GetOffset(id);
    }

    class GlueTable {
    public:
        explicit GlueTable(Triple triple)
        {
            switch (triple) {
                case Triple::TRIPLE_AMD64:
                case Triple::TRIPLE_AARCH64:
                    offsetTable_ = {
                        GLUE_EXCEPTION_OFFSET_64,
#define GLUE_OFFSET_MACRO(name, camelName, lastName, lastSize32, lastSize64) \
                        GLUE_##name##_OFFSET_64,
                        GLUE_OFFSET_LIST(GLUE_OFFSET_MACRO)
#undef GLUE_OFFSET_MACRO
                    };
                    break;
                case Triple::TRIPLE_ARM32:
                    offsetTable_ = {
                        GLUE_EXCEPTION_OFFSET_32,
#define GLUE_OFFSET_MACRO(name, camelName, lastName, lastSize32, lastSize64) \
                        GLUE_##name##_OFFSET_32,
                        GLUE_OFFSET_LIST(GLUE_OFFSET_MACRO)
#undef GLUE_OFFSET_MACRO
                    };
                    break;
                default:
                    UNREACHABLE();
            }
        }
        ~GlueTable() = default;

        uint32_t GetOffset(JSThread::GlueID id) const
        {
            return offsetTable_[static_cast<size_t>(id)];
        }

    private:
        std::array<uint32_t, static_cast<size_t>(JSThread::GlueID::NUMBER_OF_GLUE)> offsetTable_ {};
    };

private:
    inline Triple GetTripleFromString(const std::string &triple)
    {
        if (triple.compare("x86_64-unknown-linux-gnu") == 0) {
            return Triple::TRIPLE_AMD64;
        }

        if (triple.compare("aarch64-unknown-linux-gnu") == 0) {
            return Triple::TRIPLE_AARCH64;
        }

        if (triple.compare("arm-unknown-linux-gnu") == 0) {
            return Triple::TRIPLE_ARM32;
        }
        UNREACHABLE();
    }
    Triple triple_;
    GlueTable glueTable_;
};

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
        inline void Seal();
        inline void WriteVariable(Variable *var, GateRef value);
        inline GateRef ReadVariable(Variable *var);
        inline void Bind();
        inline void MergeAllControl();
        inline void MergeAllDepend();
        inline void AppendPredecessor(const Label *predecessor);
        inline std::vector<Label> GetPredecessors() const;
        inline void SetControl(GateRef control);
        inline void SetPreControl(GateRef control);
        inline void MergeControl(GateRef control);
        inline GateRef GetControl() const;
        inline GateRef GetDepend() const;
        inline void SetDepend(GateRef depend);
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
        CircuitBuilder &GetCircuitBuilder()
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

        uint32_t GetGlueOffset(JSThread::GlueID id) const
        {
            return compCfg_->GetGlueOffset(id);
        }

        inline GateType GetGateType(GateRef gate) const;
        inline Label GetLabelFromSelector(GateRef sel);
        inline void AddSelectorToLabel(GateRef sel, Label label);
        inline LabelImpl *NewLabel(Environment *env, GateRef control = -1);
        inline void PushCurrentLabel(Label *entry);
        inline void PopCurrentLabel();
        inline void SetFrameType(FrameType type);
        inline GateRef GetArgument(size_t index) const;
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
        Variable(Environment *env, StubMachineType type, uint32_t id, GateRef value) : id_(id), type_(type), env_(env)
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
        StubMachineType Type() const
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
        StubMachineType type_;
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
    inline GateRef GetInt8Constant(int8_t value);
    inline GateRef GetInt16Constant(int16_t value);
    inline GateRef GetInt32Constant(int32_t value);
    inline GateRef GetInt64Constant(int64_t value);
    inline GateRef GetIntPtrConstant(int64_t value);
    inline GateRef GetIntPtrSize();
    inline uint64_t GetIntPtrSize() const;
    inline GateRef GetRelocatableData(uint64_t value);
    inline GateRef TrueConstant();
    inline GateRef FalseConstant();
    inline GateRef GetBooleanConstant(bool value);
    inline GateRef GetDoubleConstant(double value);
    inline GateRef GetUndefinedConstant(StubMachineType type = StubMachineType::TAGGED);
    inline GateRef GetHoleConstant(StubMachineType type = StubMachineType::TAGGED);
    inline GateRef GetNullConstant(StubMachineType type = StubMachineType::TAGGED);
    inline GateRef GetExceptionConstant(StubMachineType type = StubMachineType::TAGGED);
    inline GateRef IntPtrMul(GateRef x, GateRef y);
    // parameter
    inline GateRef Argument(size_t index);
    inline GateRef Int1Argument(size_t index);
    inline GateRef Int32Argument(size_t index);
    inline GateRef Int64Argument(size_t index);
    inline GateRef TaggedArgument(size_t index);
    inline GateRef TaggedPointerArgument(size_t index, GateType type = GateType::TAGGED_POINTER);
    inline GateRef PtrArgument(size_t index, GateType type = GateType::C_VALUE);
    inline GateRef Float32Argument(size_t index);
    inline GateRef Float64Argument(size_t index);
    inline GateRef Alloca(int size);
    // control flow
    inline GateRef Return(GateRef value);
    inline GateRef Return();
    inline void Bind(Label *label);
    void Jump(Label *label);
    void Branch(GateRef condition, Label *trueLabel, Label *falseLabel);
    void Switch(GateRef index, Label *defaultLabel, int64_t *keysValue, Label *keysLabel, int numberOfKeys);
    void Seal(Label *label)
    {
        label->Seal();
    }
    void LoopBegin(Label *loopHead);
    void LoopEnd(Label *loopHead);
    // call operation
    inline GateRef CallStub(StubDescriptor *descriptor, GateRef glue, GateRef target,
                              std::initializer_list<GateRef> args);
    inline GateRef CallStub(StubDescriptor *descriptor, GateRef glue, GateRef target, GateRef depend,
                              std::initializer_list<GateRef> args);
    inline GateRef CallRuntime(StubDescriptor *descriptor, GateRef glue, GateRef target,
                                 std::initializer_list<GateRef> args);
    inline GateRef CallRuntime(StubDescriptor *descriptor, GateRef glue, GateRef target, GateRef depend,
                                 std::initializer_list<GateRef> args);
    inline GateRef CallRuntimeTrampoline(GateRef glue, GateRef target,
                               std::initializer_list<GateRef> args);
    inline GateRef CallRuntimeTrampoline(GateRef glue, GateRef target, GateRef depend,
                               std::initializer_list<GateRef> args);
    inline void DebugPrint(GateRef thread, std::initializer_list<GateRef> args);
    // memory
    inline GateRef Load(StubMachineType type, GateRef base, GateRef offset);
    inline GateRef Load(StubMachineType type, GateRef base);
    GateRef Store(StubMachineType type, GateRef glue, GateRef base, GateRef offset, GateRef value);
    // arithmetic
    inline GateRef TaggedCastToIntPtr(GateRef x);
    inline GateRef Int16Add(GateRef x, GateRef y);
    inline GateRef Int32Add(GateRef x, GateRef y);
    inline GateRef Int64Add(GateRef x, GateRef y);
    inline GateRef DoubleAdd(GateRef x, GateRef y);
    inline GateRef IntPtrAdd(GateRef x, GateRef y);
    inline GateRef IntPtrSub(GateRef x, GateRef y);
    inline GateRef IntPtrEqual(GateRef x, GateRef y);
    inline GateRef Int32Sub(GateRef x, GateRef y);
    inline GateRef Int64Sub(GateRef x, GateRef y);
    inline GateRef DoubleSub(GateRef x, GateRef y);
    inline GateRef Int32Mul(GateRef x, GateRef y);
    inline GateRef Int64Mul(GateRef x, GateRef y);
    inline GateRef DoubleMul(GateRef x, GateRef y);
    inline GateRef DoubleDiv(GateRef x, GateRef y);
    inline GateRef Int32Div(GateRef x, GateRef y);
    inline GateRef UInt32Div(GateRef x, GateRef y);
    inline GateRef UInt64Div(GateRef x, GateRef y);
    inline GateRef Int32Mod(GateRef x, GateRef y);
    inline GateRef DoubleMod(GateRef x, GateRef y);
    inline GateRef Int64Div(GateRef x, GateRef y);
    inline GateRef IntPtrDiv(GateRef x, GateRef y);
    // bit operation
    inline GateRef Int32Or(GateRef x, GateRef y);
    inline GateRef Int8And(GateRef x, GateRef y);
    inline GateRef Int32And(GateRef x, GateRef y);
    inline GateRef IntPtrAnd(GateRef x, GateRef y);
    inline GateRef BoolAnd(GateRef x, GateRef y);
    inline GateRef Int32Not(GateRef x);
    inline GateRef BoolNot(GateRef x);
    inline GateRef Int32Xor(GateRef x, GateRef y);
    inline GateRef FixLoadType(GateRef x);
    inline GateRef Int64Or(GateRef x, GateRef y);
    inline GateRef IntPtrOr(GateRef x, GateRef y);
    inline GateRef Int64And(GateRef x, GateRef y);
    inline GateRef Int64Xor(GateRef x, GateRef y);
    inline GateRef Int64Not(GateRef x);
    inline GateRef Int16LSL(GateRef x, GateRef y);
    inline GateRef Int32LSL(GateRef x, GateRef y);
    inline GateRef Int64LSL(GateRef x, GateRef y);
    inline GateRef UInt64LSL(GateRef x, GateRef y);
    inline GateRef IntPtrLSL(GateRef x, GateRef y);
    inline GateRef Int8LSR(GateRef x, GateRef y);
    inline GateRef UInt32LSR(GateRef x, GateRef y);
    inline GateRef UInt64LSR(GateRef x, GateRef y);
    inline GateRef IntPtrLSR(GateRef x, GateRef y);
    inline GateRef TaggedIsInt(GateRef x);
    inline GateRef TaggedIsDouble(GateRef x);
    inline GateRef TaggedIsObject(GateRef x);
    inline GateRef TaggedIsNumber(GateRef x);
    inline GateRef TaggedIsHole(GateRef x);
    inline GateRef TaggedIsNotHole(GateRef x);
    inline GateRef TaggedIsUndefined(GateRef x);
    inline GateRef TaggedIsException(GateRef x);
    inline GateRef TaggedIsSpecial(GateRef x);
    inline GateRef TaggedIsHeapObject(GateRef x);
    inline GateRef ObjectAddressToRange(GateRef x);
    inline GateRef InYoungGeneration(GateRef glue, GateRef x);
    inline GateRef TaggedIsGeneratorObject(GateRef x);
    inline GateRef TaggedIsPropertyBox(GateRef x);
    inline GateRef TaggedIsWeak(GateRef x);
    inline GateRef TaggedIsPrototypeHandler(GateRef x);
    inline GateRef TaggedIsTransitionHandler(GateRef x);
    GateRef TaggedIsString(GateRef obj);
    GateRef TaggedIsStringOrSymbol(GateRef obj);
    inline GateRef GetNextPositionForHash(GateRef last, GateRef count, GateRef size);
    inline GateRef DoubleIsNAN(GateRef x);
    inline GateRef DoubleIsINF(GateRef x);
    inline GateRef TaggedIsNull(GateRef x);
    inline GateRef TaggedIsUndefinedOrNull(GateRef x);
    inline GateRef TaggedIsTrue(GateRef x);
    inline GateRef TaggedIsFalse(GateRef x);
    inline GateRef TaggedIsBoolean(GateRef x);
    inline GateRef TaggedGetInt(GateRef x);
    inline GateRef Int16BuildTaggedWithNoGC(GateRef x);
    inline GateRef Int16BuildTaggedTypeWithNoGC(GateRef x);
    inline GateRef IntBuildTaggedWithNoGC(GateRef x);
    inline GateRef IntBuildTaggedTypeWithNoGC(GateRef x);
    inline GateRef PtrBuildTaggedWithNoGC(GateRef x);
    inline GateRef DoubleBuildTaggedWithNoGC(GateRef x);
    inline GateRef DoubleBuildTaggedTypeWithNoGC(GateRef x);
    inline GateRef CastDoubleToInt64(GateRef x);
    inline GateRef TaggedTrue();
    inline GateRef TaggedFalse();
    // compare operation
    inline GateRef Int8Equal(GateRef x, GateRef y);
    inline GateRef Int32Equal(GateRef x, GateRef y);
    inline GateRef Int32NotEqual(GateRef x, GateRef y);
    inline GateRef Int64Equal(GateRef x, GateRef y);
    inline GateRef DoubleEqual(GateRef x, GateRef y);
    inline GateRef Int64NotEqual(GateRef x, GateRef y);
    inline GateRef DoubleLessThan(GateRef x, GateRef y);
    inline GateRef DoubleLessThanOrEqual(GateRef x, GateRef y);
    inline GateRef DoubleGreaterThan(GateRef x, GateRef y);
    inline GateRef DoubleGreaterThanOrEqual(GateRef x, GateRef y);
    inline GateRef Int32GreaterThan(GateRef x, GateRef y);
    inline GateRef Int32LessThan(GateRef x, GateRef y);
    inline GateRef Int32GreaterThanOrEqual(GateRef x, GateRef y);
    inline GateRef Int32LessThanOrEqual(GateRef x, GateRef y);
    inline GateRef UInt32GreaterThan(GateRef x, GateRef y);
    inline GateRef UInt32LessThan(GateRef x, GateRef y);
    inline GateRef UInt32LessThanOrEqual(GateRef x, GateRef y);
    inline GateRef UInt32GreaterThanOrEqual(GateRef x, GateRef y);
    inline GateRef Int64GreaterThan(GateRef x, GateRef y);
    inline GateRef Int64LessThan(GateRef x, GateRef y);
    inline GateRef Int64LessThanOrEqual(GateRef x, GateRef y);
    inline GateRef Int64GreaterThanOrEqual(GateRef x, GateRef y);
    inline GateRef UInt6464GreaterThan(GateRef x, GateRef y);
    inline GateRef UInt64LessThan(GateRef x, GateRef y);
    inline GateRef UInt64LessThanOrEqual(GateRef x, GateRef y);
    inline GateRef UInt6464GreaterThanOrEqual(GateRef x, GateRef y);
    // cast operation
    inline GateRef ChangeInt64ToInt32(GateRef val);
    inline GateRef ChangeInt64ToIntPtr(GateRef val);
    inline GateRef ChangeInt32ToIntPtr(GateRef val);
    // math operation
    GateRef Sqrt(GateRef x);
    inline GateRef GetSetterFromAccessor(GateRef accessor);
    inline GateRef GetElementsArray(GateRef object);
    inline void SetElementsArray(GateRef glue, GateRef object, GateRef elementsArray);
    inline GateRef GetPropertiesArray(GateRef object);
    // SetProperties in js_object.h
    inline void SetPropertiesArray(GateRef glue, GateRef object, GateRef propsArray);
    inline GateRef GetLengthOfTaggedArray(GateRef array);
    // object operation
    inline GateRef IsJSHClass(GateRef obj);
    inline GateRef LoadHClass(GateRef object);
    inline void StoreHClass(GateRef glue, GateRef object, GateRef hclass);
    void CopyAllHClass(GateRef glue, GateRef dstHClass, GateRef scrHClass);
    inline GateRef GetObjectType(GateRef hClass);
    inline GateRef GetGeneratorObjectResumeMode(GateRef obj);
    inline GateRef IsDictionaryMode(GateRef object);
    inline GateRef IsDictionaryModeByHClass(GateRef hClass);
    inline GateRef IsDictionaryElement(GateRef hClass);
    inline GateRef NotBuiltinsConstructor(GateRef object);
    inline GateRef IsClassConstructor(GateRef object);
    inline GateRef IsClassPrototype(GateRef object);
    inline GateRef IsExtensible(GateRef object);
    inline GateRef IsEcmaObject(GateRef obj);
    inline GateRef IsSymbol(GateRef obj);
    inline GateRef IsString(GateRef obj);
    inline GateRef IsJsProxy(GateRef obj);
    inline GateRef IsJSFunctionBase(GateRef obj);
    inline GateRef IsJsArray(GateRef obj);
    inline GateRef IsJSObject(GateRef obj);
    inline GateRef IsWritable(GateRef attr);
    inline GateRef IsAccessor(GateRef attr);
    inline GateRef IsInlinedProperty(GateRef attr);
    inline GateRef IsField(GateRef attr);
    inline GateRef IsNonExist(GateRef attr);
    inline GateRef HandlerBaseIsAccessor(GateRef attr);
    inline GateRef HandlerBaseIsJSArray(GateRef attr);
    inline GateRef HandlerBaseIsInlinedProperty(GateRef attr);
    inline GateRef HandlerBaseGetOffset(GateRef attr);
    inline GateRef IsInvalidPropertyBox(GateRef obj);
    inline GateRef GetValueFromPropertyBox(GateRef obj);
    inline void SetValueToPropertyBox(GateRef glue, GateRef obj, GateRef value);
    inline GateRef GetTransitionFromHClass(GateRef obj);
    inline GateRef GetTransitionHandlerInfo(GateRef obj);
    inline GateRef IsInternalAccessor(GateRef attr);
    inline GateRef GetProtoCell(GateRef object);
    inline GateRef GetPrototypeHandlerHolder(GateRef object);
    inline GateRef GetPrototypeHandlerHandlerInfo(GateRef object);
    inline GateRef GetHasChanged(GateRef object);
    inline GateRef HclassIsPrototypeHandler(GateRef hclass);
    inline GateRef HclassIsTransitionHandler(GateRef hclass);
    inline GateRef HclassIsPropertyBox(GateRef hclass);
    inline GateRef PropAttrGetOffset(GateRef attr);
    // SetDictionaryOrder func in property_attribute.h
    inline GateRef SetDictionaryOrderFieldInPropAttr(GateRef attr, GateRef value);
    inline GateRef GetPrototypeFromHClass(GateRef hClass);
    inline GateRef GetLayoutFromHClass(GateRef hClass);
    inline GateRef GetBitFieldFromHClass(GateRef hClass);
    inline GateRef SetBitFieldToHClass(GateRef glue, GateRef hClass, GateRef bitfield);
    inline GateRef SetPrototypeToHClass(StubMachineType type, GateRef glue, GateRef hClass, GateRef proto);
    inline GateRef SetProtoChangeDetailsToHClass(StubMachineType type, GateRef glue, GateRef hClass,
	                                               GateRef protoChange);
    inline GateRef SetLayoutToHClass(StubMachineType type, GateRef glue, GateRef hClass, GateRef attr);
    inline GateRef SetEnumCacheToHClass(StubMachineType type, GateRef glue, GateRef hClass, GateRef key);
    inline GateRef SetTransitionsToHClass(StubMachineType type, GateRef glue, GateRef hClass, GateRef transition);
    inline void SetIsProtoTypeToHClass(GateRef glue, GateRef hClass, GateRef value);
    inline GateRef IsProtoTypeHClass(GateRef hClass);
    inline void SetPropertyInlinedProps(GateRef glue, GateRef obj, GateRef hClass,
                                        GateRef value, GateRef attrOffset);

    inline void IncNumberOfProps(GateRef glue, GateRef hClass);
    inline GateRef GetNumberOfPropsFromHClass(GateRef hClass);
    inline void SetNumberOfPropsToHClass(GateRef glue, GateRef hClass, GateRef value);
    inline GateRef GetObjectSizeFromHClass(GateRef hClass);
    inline GateRef GetInlinedPropsStartFromHClass(GateRef hClass);
    inline GateRef GetInlinedPropertiesFromHClass(GateRef hClass);
    void ThrowTypeAndReturn(GateRef glue, int messageId, GateRef val);
    inline GateRef GetValueFromTaggedArray(StubMachineType returnType, GateRef elements, GateRef index);
    inline void SetValueToTaggedArray(StubMachineType valType, GateRef glue, GateRef array, GateRef index, GateRef val);
    inline void UpdateValueAndAttributes(GateRef glue, GateRef elements, GateRef index, GateRef value, GateRef attr);
    inline GateRef IsSpecialIndexedObj(GateRef jsType);
    inline GateRef IsSpecialContainer(GateRef jsType);
    inline GateRef IsAccessorInternal(GateRef value);
    template<typename DictionaryT = NameDictionary>
    GateRef GetAttributesFromDictionary(GateRef elements, GateRef entry);
    template<typename DictionaryT = NameDictionary>
    GateRef GetValueFromDictionary(StubMachineType returnType, GateRef elements, GateRef entry);
    template<typename DictionaryT = NameDictionary>
    GateRef GetKeyFromDictionary(StubMachineType returnType, GateRef elements, GateRef entry);
    inline GateRef GetPropAttrFromLayoutInfo(GateRef layout, GateRef entry);
    inline GateRef GetPropertiesAddrFromLayoutInfo(GateRef layout);
    inline GateRef GetPropertyMetaDataFromAttr(GateRef attr);
    inline GateRef GetKeyFromLayoutInfo(GateRef layout, GateRef entry);
    GateRef FindElementWithCache(GateRef glue, GateRef layoutInfo, GateRef hClass,
        GateRef key, GateRef propsNum);
    GateRef FindElementFromNumberDictionary(GateRef glue, GateRef elements, GateRef index);
    GateRef FindEntryFromNameDictionary(GateRef glue, GateRef elements, GateRef key);
    GateRef IsMatchInTransitionDictionary(GateRef element, GateRef key, GateRef metaData, GateRef attr);
    GateRef FindEntryFromTransitionDictionary(GateRef glue, GateRef elements, GateRef key, GateRef metaData);
    GateRef JSObjectGetProperty(StubMachineType returnType, GateRef obj, GateRef hClass, GateRef propAttr);
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
    void StoreField(GateRef glue, GateRef receiver, GateRef value, GateRef handler);
    void StoreWithTransition(GateRef glue, GateRef receiver, GateRef value, GateRef handler);
    GateRef StoreGlobal(GateRef glue, GateRef value, GateRef cell);
    void JSHClassAddProperty(GateRef glue, GateRef receiver, GateRef key, GateRef attr);
    void NotifyHClassChanged(GateRef glue, GateRef oldHClass, GateRef newHClass);
    inline GateRef TaggedCastToInt64(GateRef x);
    inline GateRef TaggedCastToInt32(GateRef x);
    inline GateRef TaggedCastToDouble(GateRef x);
    inline GateRef TaggedCastToWeakReferentUnChecked(GateRef x);
    inline GateRef ChangeInt32ToFloat64(GateRef x);
    inline GateRef ChangeFloat64ToInt32(GateRef x);
    inline GateRef ChangeTaggedPointerToInt64(GateRef x);
    inline GateRef ChangeInt64ToTagged(GateRef x);
    inline GateRef CastInt64ToFloat64(GateRef x);
    inline GateRef SExtInt32ToInt64(GateRef x);
    inline GateRef SExtInt1ToInt64(GateRef x);
    inline GateRef SExtInt1ToInt32(GateRef x);
    inline GateRef ZExtInt8ToInt16(GateRef x);
    inline GateRef ZExtInt32ToInt64(GateRef x);
    inline GateRef ZExtInt1ToInt64(GateRef x);
    inline GateRef ZExtInt1ToInt32(GateRef x);
    inline GateRef ZExtInt8ToInt32(GateRef x);
    inline GateRef ZExtInt8ToInt64(GateRef x);
    inline GateRef ZExtInt8ToPtr(GateRef x);
    inline GateRef ZExtInt16ToPtr(GateRef x);
    inline GateRef SExtInt32ToPtr(GateRef x);
    inline GateRef ZExtInt16ToInt32(GateRef x);
    inline GateRef ZExtInt16ToInt64(GateRef x);
    inline GateRef TruncInt64ToInt32(GateRef x);
    inline GateRef TruncPtrToInt32(GateRef x);
    inline GateRef TruncInt64ToInt1(GateRef x);
    inline GateRef TruncInt32ToInt1(GateRef x);
    inline GateRef GetGlobalConstantAddr(GateRef index);
    inline GateRef GetGlobalConstantString(ConstantIndex index);
    inline GateRef IsCallable(GateRef obj);
    inline GateRef GetOffsetFieldInPropAttr(GateRef attr);
    inline GateRef SetOffsetFieldInPropAttr(GateRef attr, GateRef value);
    inline GateRef SetIsInlinePropsFieldInPropAttr(GateRef attr, GateRef value);
    inline void SetHasConstructorToHClass(GateRef glue, GateRef hClass, GateRef value);
    inline void UpdateValueInDict(GateRef glue, GateRef elements, GateRef index, GateRef value);
    inline GateRef GetBitMask(GateRef bitoffset);
    inline GateRef AddrToBitOffset(GateRef memberset, GateRef addr);
    inline GateRef IntptrEuqal(GateRef x, GateRef y);
    void SetValueWithBarrier(GateRef glue, GateRef obj, GateRef offset, GateRef value);
    GateRef GetPropertyByIndex(GateRef glue, GateRef receiver, GateRef index);
    GateRef GetPropertyByName(GateRef glue, GateRef receiver, GateRef key);
    GateRef SetPropertyByIndex(GateRef glue, GateRef receiver, GateRef index, GateRef value);
    GateRef SetPropertyByName(GateRef glue, GateRef receiver, GateRef key,
                               GateRef value); // Crawl prototype chain

    GateRef SetPropertyByNameWithOwn(GateRef glue, GateRef receiver, GateRef key,
                               GateRef value); // Do not crawl the prototype chain
    inline GateRef GetParentEnv(GateRef object);
    inline GateRef GetPropertiesFromLexicalEnv(GateRef object, GateRef index);
    inline void SetPropertiesToLexicalEnv(GateRef glue, GateRef object, GateRef index, GateRef value);
    inline GateRef GetFunctionBitFieldFromJSFunction(GateRef object);
    inline GateRef GetHomeObjectFromJSFunction(GateRef object);
    inline void SetLexicalEnvToFunction(GateRef glue, GateRef object, GateRef lexicalEnv);
    inline GateRef GetGlobalObject(GateRef glue);
    inline GateRef GetEntryIndexOfGlobalDictionary(GateRef entry);
    inline GateRef GetBoxFromGlobalDictionary(GateRef object, GateRef entry);
    inline GateRef GetValueFromGlobalDictionary(GateRef object, GateRef entry);
    inline GateRef GetPropertiesFromJSObject(GateRef object);
    GateRef GetGlobalOwnProperty(GateRef glue, GateRef receiver, GateRef key);

    // fast path
    GateRef FastEqual(GateRef left, GateRef right);
    GateRef FastMul(GateRef left, GateRef right);
    GateRef FastDiv(GateRef left, GateRef right);
    GateRef FastMod(GateRef glue, GateRef left, GateRef right);

    // Add SpecialContainer
    GateRef GetContainerProperty(GateRef glue, GateRef receiver, GateRef index, GateRef jsType);
    GateRef JSArrayListGet(GateRef glue, GateRef receiver, GateRef index);

private:
    Environment env_;
    std::string methodName_;
    int nextVariableId_ {0};
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_STUB_H
