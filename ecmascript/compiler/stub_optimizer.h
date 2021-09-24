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

#ifndef ECMASCRIPT_COMPILER_STUBOPTIMIZER_H
#define ECMASCRIPT_COMPILER_STUBOPTIMIZER_H

#include "ecmascript/accessor_data.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/stub_interface.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tagged_dictionary.h"

namespace kungfu {
class CompilerOptions;
class Environment;
class StubOptimizerLabel;
class StubVariable;
class CallerDescriptor;
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEFVARIABLE(varname, type, val) StubVariable varname(GetEnvironment(), type, NextVariableId(), val)

class StubOptimizerLabel {
public:
    class StubOptimizerLabelImplement {
    public:
        StubOptimizerLabelImplement(Environment *env, AddrShift control)
            : env_(env), control_(control), predeControl_(-1), isSealed_(false)
        {
        }

        ~StubOptimizerLabelImplement() = default;

        NO_MOVE_SEMANTIC(StubOptimizerLabelImplement);
        NO_COPY_SEMANTIC(StubOptimizerLabelImplement);
        using Variable = StubVariable;
        void Seal();
        void WriteVariable(Variable *var, AddrShift value);
        AddrShift ReadVariable(Variable *var);
        void Bind();
        void MergeAllControl();
        void AppendPredecessor(StubOptimizerLabelImplement *predecessor);
        std::vector<StubOptimizerLabelImplement *> GetPredecessors() const
        {
            return predecessors;
        }

        void SetControl(AddrShift control)
        {
            control_ = control;
        }

        void SetPreControl(AddrShift control)
        {
            predeControl_ = control;
        }

        void MergeControl(AddrShift control)
        {
            if (predeControl_ == -1) {
                predeControl_ = control;
                control_ = predeControl_;
            } else {
                otherPredeControls_.push_back(control);
            }
        }

        AddrShift GetControl() const
        {
            return control_;
        }

    private:
        bool IsNeedSeal() const;
        bool IsSealed() const
        {
            return isSealed_;
        }
        bool IsLoopHead() const;
        AddrShift ReadVariableRecursive(Variable *var);
        Environment *env_;
        AddrShift control_;
        AddrShift predeControl_;
        std::vector<AddrShift> otherPredeControls_;
        bool isSealed_;
        std::map<StubVariable *, AddrShift> valueMap_;
        std::vector<AddrShift> phi;
        std::vector<StubOptimizerLabelImplement *> predecessors;
        std::map<StubVariable *, AddrShift> incompletePhis_;
    };
    explicit StubOptimizerLabel() = default;
    explicit StubOptimizerLabel(Environment *env);
    explicit StubOptimizerLabel(StubOptimizerLabelImplement *impl) : impl_(impl) {}
    ~StubOptimizerLabel() = default;
    using Variable = StubVariable;
    StubOptimizerLabel(StubOptimizerLabel const &label) = default;
    StubOptimizerLabel &operator=(StubOptimizerLabel const &label) = default;
    StubOptimizerLabel(StubOptimizerLabel &&label) = default;
    StubOptimizerLabel &operator=(StubOptimizerLabel &&label) = default;

    void Seal()
    {
        return impl_->Seal();
    }

    void WriteVariable(Variable *var, AddrShift value)
    {
        impl_->WriteVariable(var, value);
    }

    AddrShift ReadVariable(Variable *var)
    {
        return impl_->ReadVariable(var);
    }

    void Bind()
    {
        impl_->Bind();
    }

    void MergeAllControl()
    {
        impl_->MergeAllControl();
    }

    void AppendPredecessor(const StubOptimizerLabel *predecessor)
    {
        impl_->AppendPredecessor(predecessor->GetRawLabel());
    }

    std::vector<StubOptimizerLabel> GetPredecessors() const
    {
        std::vector<StubOptimizerLabel> labels;
        for (auto rawlabel : impl_->GetPredecessors()) {
            labels.emplace_back(StubOptimizerLabel(rawlabel));
        }
        return labels;
    }

    void SetControl(AddrShift control)
    {
        impl_->SetControl(control);
    }

    void SetPreControl(AddrShift control)
    {
        impl_->SetPreControl(control);
    }

    void MergeControl(AddrShift control)
    {
        impl_->MergeControl(control);
    }

    AddrShift GetControl() const
    {
        return impl_->GetControl();
    }

private:
    friend class Environment;
    StubOptimizerLabelImplement *GetRawLabel() const
    {
        return impl_;
    }
    StubOptimizerLabelImplement *impl_{nullptr};
};

class Environment {
public:
    using StubOptimizerLabelImplement = StubOptimizerLabel::StubOptimizerLabelImplement;
    Environment(const char *name, size_t arguments);
    ~Environment();
    Environment(Environment const &env);             // copy constructor
    Environment &operator=(Environment const &env);  // copy constructor

    NO_MOVE_SEMANTIC(Environment);
    StubOptimizerLabel *GetCurrentLabel() const
    {
        return currentLabel_;
    }
    void SetCurrentLabel(StubOptimizerLabel *label)
    {
        currentLabel_ = label;
    }
    CircuitBuilder &GetCircuitBuilder()
    {
        return builder_;
    }
    AddrShift GetArgument(size_t index) const
    {
        return arguments_.at(index);
    }
    Circuit &GetCircuit()
    {
        return circuit_;
    }

    StubOptimizerLabel GetLabelFromSelector(AddrShift sel)
    {
        StubOptimizerLabelImplement *rawlabel = phi_to_labels[sel];
        return StubOptimizerLabel(rawlabel);
    }

    void AddSelectorToLabel(AddrShift sel, StubOptimizerLabel label)
    {
        phi_to_labels[sel] = label.GetRawLabel();
    }

    StubOptimizerLabelImplement *NewStubOptimizerLabel(Environment *env, AddrShift control = -1)
    {
        auto impl = new StubOptimizerLabelImplement(env, control);
        rawlabels_.push_back(impl);
        return impl;
    }

private:
    StubOptimizerLabel *currentLabel_{nullptr};
    Circuit circuit_;
    CircuitBuilder builder_;
    std::map<AddrShift, StubOptimizerLabelImplement *> phi_to_labels;
    std::vector<AddrShift> arguments_;
    std::string method_name_;
    StubOptimizerLabel entry_;
    std::vector<StubOptimizerLabelImplement *> rawlabels_;
};

class StubVariable {
public:
    StubVariable(Environment *env, MachineType type, uint32_t id, AddrShift value) : id_(id), type_(type), env_(env)
    {
        Bind(value);
        env_->GetCurrentLabel()->WriteVariable(this, value);
    }
    ~StubVariable() = default;
    NO_MOVE_SEMANTIC(StubVariable);
    NO_COPY_SEMANTIC(StubVariable);
    void Bind(AddrShift value)
    {
        currentValue_ = value;
    }
    AddrShift Value() const
    {
        return currentValue_;
    }
    MachineType Type() const
    {
        return type_;
    }
    bool IsBound() const
    {
        return currentValue_ != 0;
    }
    StubVariable &operator=(const AddrShift value)
    {
        env_->GetCurrentLabel()->WriteVariable(this, value);
        Bind(value);
        return *this;
    }

    AddrShift operator*()
    {
        return env_->GetCurrentLabel()->ReadVariable(this);
    }

    AddrShift AddPhiOperand(AddrShift val);
    AddrShift AddOperandToSelector(AddrShift val, size_t idx, AddrShift in);
    AddrShift TryRemoveTrivialPhi(AddrShift phi);
    void RerouteOuts(const std::vector<Out *> &outs, Gate *newGate);

    bool IsSelector(AddrShift gate) const
    {
        return env_->GetCircuit().IsSelector(gate);
    }

    bool IsSelector(const Gate *gate) const
    {
        return gate->GetOpCode() >= OpCode::VALUE_SELECTOR_JS && gate->GetOpCode() <= OpCode::VALUE_SELECTOR_FLOAT64;
    }

    uint32_t GetId() const
    {
        return id_;
    }

private:
    uint32_t id_;
    MachineType type_;
    AddrShift currentValue_{0};
    Environment *env_;
};

class StubOptimizer {
public:
    using Label = StubOptimizerLabel;
    using Variable = StubVariable;

    explicit StubOptimizer(Environment *env) : env_(env) {}
    virtual ~StubOptimizer() = default;
    NO_MOVE_SEMANTIC(StubOptimizer);
    NO_COPY_SEMANTIC(StubOptimizer);
    virtual void GenerateCircuit() = 0;

    Environment *GetEnvironment() const
    {
        return env_;
    }
    // constant
    AddrShift GetInteger32Constant(int32_t value)
    {
        return env_->GetCircuitBuilder().NewIntegerConstant(value);
    };
    AddrShift GetWord64Constant(uint64_t value)
    {
        return env_->GetCircuitBuilder().NewInteger64Constant(value);
    };

    AddrShift GetPtrConstant(uint32_t value)
    {
#ifdef PANDA_TARGET_AMD64
        return GetWord64Constant(value);
#endif
#ifdef PANDA_TARGET_X86
        return GetInteger32Constant(value);
#endif
#ifdef PANDA_TARGET_ARM64
        return GetWord64Constant(value);
#endif
#ifdef PANDA_TARGET_ARM32
        return GetInteger32Constant(value);
#endif
    }

    AddrShift PtrMul(AddrShift x, AddrShift y)
    {
#ifdef PANDA_TARGET_AMD64
        return Int64Mul(x, y);
#endif
#ifdef PANDA_TARGET_X86
        return Int32Mul(x, y);
#endif
#ifdef PANDA_TARGET_ARM64
        return Int64Mul(x, y);
#endif
#ifdef PANDA_TARGET_ARM32
        return Int32Mul(x, y);
#endif
    }

    AddrShift TrueConstant()
    {
        return TruncInt32ToInt1(GetInteger32Constant(1));
    }

    AddrShift FalseConstant()
    {
        return TruncInt32ToInt1(GetInteger32Constant(0));
    }

    AddrShift GetBooleanConstant(bool value)
    {
        return env_->GetCircuitBuilder().NewBooleanConstant(value);
    }

    AddrShift GetDoubleConstant(double value)
    {
        return env_->GetCircuitBuilder().NewDoubleConstant(value);
    }

    AddrShift GetUndefinedConstant()
    {
        return env_->GetCircuitBuilder().UndefineConstant();
    }

    AddrShift GetHoleConstant()
    {
        return env_->GetCircuitBuilder().HoleConstant();
    }
    // parameter
    AddrShift Argument(size_t index)
    {
        return env_->GetArgument(index);
    }

    AddrShift Int1Argument(size_t index)
    {
        AddrShift argument = Argument(index);
        env_->GetCircuit().SetOpCode(argument, OpCode(OpCode::INT1_ARG));
        return argument;
    }

    AddrShift Int32Argument(size_t index)
    {
        AddrShift argument = Argument(index);
        env_->GetCircuit().SetOpCode(argument, OpCode(OpCode::INT32_ARG));
        return argument;
    }

    AddrShift Int64Argument(size_t index)
    {
        AddrShift argument = Argument(index);
        env_->GetCircuit().SetOpCode(argument, OpCode(OpCode::INT64_ARG));
        return argument;
    }

    AddrShift PtrArgument(size_t index)
    {
        AddrShift argument = Argument(index);
        if (PtrValueCode() == INT64) {
            env_->GetCircuit().SetOpCode(argument, OpCode(OpCode::INT64_ARG));
        } else if (PtrValueCode() == INT32) {
            env_->GetCircuit().SetOpCode(argument, OpCode(OpCode::INT32_ARG));
        } else {
            UNREACHABLE();
        }
        return argument;
    }

    AddrShift Float32Argument(size_t index)
    {
        AddrShift argument = Argument(index);
        env_->GetCircuit().SetOpCode(argument, OpCode(OpCode::FLOAT32_ARG));
        return argument;
    }

    AddrShift Float64Argument(size_t index)
    {
        AddrShift argument = Argument(index);
        env_->GetCircuit().SetOpCode(argument, OpCode(OpCode::FLOAT64_ARG));
        return argument;
    }

    AddrShift Alloca(int size)
    {
        return env_->GetCircuitBuilder().Alloca(size);
    }

    // control flow
    AddrShift Return(AddrShift value)
    {
        auto control = env_->GetCurrentLabel()->GetControl();
        return env_->GetCircuitBuilder().Return(control, value);
    }

    void Bind(Label *label)
    {
        label->Bind();
        env_->SetCurrentLabel(label);
    }
    void Jump(Label *label);
    void Branch(AddrShift condition, Label *trueLabel, Label *falseLabel);
    void Switch(AddrShift index, Label *defaultLabel, int32_t *keysValue, Label *keysLabel, int numberOfKeys);
    void Seal(Label *label)
    {
        label->Seal();
    }

    void LoopBegin(Label *loopHead);
    void LoopEnd(Label *loopHead);

    // call operation
    AddrShift CallStub(StubInterfaceDescriptor *descriptor, AddrShift target, std::initializer_list<AddrShift> args)
    {
        return env_->GetCircuitBuilder().NewCallGate(descriptor, target, args);
    }
    AddrShift CallStub(StubInterfaceDescriptor *descriptor, AddrShift target, AddrShift depend,
                       std::initializer_list<AddrShift> args)
    {
        return env_->GetCircuitBuilder().NewCallGate(descriptor, target, depend, args);
    }

    AddrShift CallRuntime(StubInterfaceDescriptor *descriptor, AddrShift thread, AddrShift target,
                          std::initializer_list<AddrShift> args)
    {
        return env_->GetCircuitBuilder().NewCallRuntimeGate(descriptor, thread, target, args);
    }

    AddrShift CallRuntime(StubInterfaceDescriptor *descriptor, AddrShift thread, AddrShift target, AddrShift depend,
                          std::initializer_list<AddrShift> args)
    {
        return env_->GetCircuitBuilder().NewCallRuntimeGate(descriptor, thread, target, depend, args);
    }

    // memory
    AddrShift Load(MachineType type, AddrShift base, AddrShift offset)
    {
        if (PtrValueCode() == ValueCode::INT64) {
            AddrShift val = Int64Add(base, offset);
            return env_->GetCircuitBuilder().NewLoadGate(type, val);
        }
        if (PtrValueCode() == ValueCode::INT32) {
            AddrShift val = Int32Add(base, offset);
            return env_->GetCircuitBuilder().NewLoadGate(type, val);
        }
        UNREACHABLE();
    }

    AddrShift Load(MachineType type, AddrShift base)
    {
        return env_->GetCircuitBuilder().NewLoadGate(type, base);
    }

    AddrShift LoadFromObject(MachineType type, AddrShift object, AddrShift offset);

    AddrShift Store(MachineType type, AddrShift base, AddrShift offset, AddrShift value)
    {
        if (PtrValueCode() == ValueCode::INT64) {
            AddrShift ptr = Int64Add(base, offset);
            return env_->GetCircuitBuilder().NewStoreGate(type, ptr, value);
        }
        if (PtrValueCode() == ValueCode::INT32) {
            AddrShift ptr = Int32Add(base, offset);
            return env_->GetCircuitBuilder().NewStoreGate(type, ptr, value);
        }
        UNREACHABLE();
    }

    // arithmetic
    AddrShift Int32Add(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_ADD), x, y);
    }

    AddrShift Int64Add(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_ADD), x, y);
    }

    AddrShift DoubleAdd(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::FLOAT64_ADD), x, y);
    }

    AddrShift PtrAdd(AddrShift x, AddrShift y)
    {
#ifdef PANDA_TARGET_AMD64
        return Int64Add(x, y);
#endif
#ifdef PANDA_TARGET_X86
        return Int32Add(x, y);
#endif
#ifdef PANDA_TARGET_ARM64
        return Int64Add(x, y);
#endif
#ifdef PANDA_TARGET_ARM32
        return Int32Add(x, y);
#endif
    }

    AddrShift Int32Sub(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_SUB), x, y);
    }

    AddrShift Int64Sub(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_SUB), x, y);
    }

    AddrShift DoubleSub(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::FLOAT64_SUB), x, y);
    }

    AddrShift Int32Mul(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_MUL), x, y);
    }

    AddrShift Int64Mul(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_MUL), x, y);
    }

    AddrShift DoubleMul(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::FLOAT64_MUL), x, y);
    }

    AddrShift DoubleDiv(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::FLOAT64_DIV), x, y);
    }
    AddrShift Int32Div(AddrShift x, AddrShift y);
    AddrShift Int64Div(AddrShift x, AddrShift y);

    // bit operation
    AddrShift Word32Or(AddrShift x, AddrShift y);
    AddrShift Word32And(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_AND), x, y);
    }
    AddrShift Word32Xor(AddrShift x, AddrShift y);

    AddrShift FixLoadType(AddrShift x);

    AddrShift Word64Or(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_OR), x, y);
    }

    AddrShift Word64And(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_AND), x, y);
    }

    AddrShift Word64Xor(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_XOR), x, y);
    }

    AddrShift Word32Not(AddrShift x);
    AddrShift Word64Not(AddrShift x)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_REV), x);
    }

    AddrShift WordLogicOr(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_OR), x, y);
    }

    AddrShift WordLogicAnd(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_AND), x, y);
    }

    AddrShift WordLogicNot(AddrShift x)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_REV), x);
    }

    AddrShift Word32LSL(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_LSL), x, y);
    }
    AddrShift Word64LSL(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_LSL), x, y);
    }
    AddrShift Word32LSR(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_LSR), x, y);
    }
    AddrShift Word64LSR(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_LSR), x, y);
    }
    AddrShift Word32Sar(AddrShift x, AddrShift y);
    AddrShift Word64Sar(AddrShift x, AddrShift y);
    AddrShift TaggedIsInt(AddrShift x)
    {
        return Word64Equal(Word64And(x, GetWord64Constant(panda::ecmascript::JSTaggedValue::TAG_MASK)),
                           GetWord64Constant(panda::ecmascript::JSTaggedValue::TAG_INT));
    }

    AddrShift TaggedIsDouble(AddrShift x)
    {
        return Word32Equal(WordLogicOr(SExtInt1ToInt32(TaggedIsInt(x)), SExtInt1ToInt32(TaggedIsObject(x))),
                           GetInteger32Constant(0));
    }

    AddrShift TaggedIsObject(AddrShift x)
    {
        return Word64Equal(Word64And(x, GetWord64Constant(panda::ecmascript::JSTaggedValue::TAG_MASK)),
                           GetWord64Constant(panda::ecmascript::JSTaggedValue::TAG_OBJECT));
    }

    AddrShift TaggedIsNumber(AddrShift x)
    {
        return TruncInt32ToInt1(WordLogicOr(SExtInt1ToInt32(TaggedIsInt(x)), SExtInt1ToInt32(TaggedIsDouble(x))));
    }

    AddrShift TaggedIsHole(AddrShift x)
    {
        return Word64Equal(x, GetWord64Constant(panda::ecmascript::JSTaggedValue::VALUE_HOLE));
    }

    AddrShift TaggedIsNotHole(AddrShift x)
    {
        return Word64NotEqual(x, GetWord64Constant(panda::ecmascript::JSTaggedValue::VALUE_HOLE));
    }

    AddrShift TaggedIsUndefined(AddrShift x)
    {
        return Word64Equal(x, GetWord64Constant(panda::ecmascript::JSTaggedValue::VALUE_UNDEFINED));
    }

    AddrShift TaggedIsSpecial(AddrShift x)
    {
        return TruncInt32ToInt1(WordLogicAnd(
            SExtInt1ToInt32(
                Word64Equal(Word64And(x, GetWord64Constant(~panda::ecmascript::JSTaggedValue::TAG_SPECIAL_MASK)),
                            GetWord64Constant(0))), 
            WordLogicOr(SExtInt1ToInt32(Word64NotEqual(
                Word64And(x, GetWord64Constant(panda::ecmascript::JSTaggedValue::TAG_SPECIAL_MASK)),
                GetWord64Constant(0))), SExtInt1ToInt32(TaggedIsHole(x)))));
    }

    AddrShift TaggedIsHeapObject(AddrShift x)
    {
        return TruncInt32ToInt1(
            WordLogicAnd(SExtInt1ToInt32(TaggedIsObject(x)), WordLogicNot(SExtInt1ToInt32(TaggedIsSpecial(x)))));
    }

    AddrShift DoubleIsNAN(AddrShift x)
    {
        AddrShift diff = DoubleEqual(x, x);
        return Word32NotEqual(SExtInt1ToInt32(diff), GetInteger32Constant(0));
    }

    AddrShift IntBuildTagged(AddrShift x)
    {
        AddrShift val = ZExtInt32ToInt64(x);
        return Word64Or(val, GetWord64Constant(panda::ecmascript::JSTaggedValue::TAG_INT));
    }

    AddrShift DoubleBuildTagged(AddrShift x)
    {
        AddrShift val = CastDoubleToInt64(x);
        return Int64Add(val, GetWord64Constant(panda::ecmascript::JSTaggedValue::DOUBLE_ENCODE_OFFSET));
    }

    AddrShift CastDoubleToInt64(AddrShift x)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::FLOAT64_TO_INT64), x);
    }

    // compare operation
    AddrShift Word32Equal(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_EQ), x, y);
    }

    AddrShift Word32NotEqual(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_NE), x, y);
    }

    AddrShift Word64Equal(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_EQ), x, y);
    }

    AddrShift DoubleEqual(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::FLOAT64_EQ), x, y);
    }

    AddrShift Word64NotEqual(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_NE), x, y);
    }

    AddrShift Int32GreaterThan(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_SGT), x, y);
    }

    AddrShift Int32LessThan(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_SLT), x, y);
    }

    AddrShift Int32GreaterThanOrEqual(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_SGE), x, y);
    }

    AddrShift Int32LessThanOrEqual(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_SLE), x, y);
    }

    AddrShift Word32GreaterThan(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_UGT), x, y);
    }

    AddrShift Word32LessThan(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_ULT), x, y);
    }

    AddrShift Word32LessThanOrEqual(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_ULE), x, y);
    }

    AddrShift Word32GreaterThanOrEqual(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT32_UGE), x, y);
    }

    AddrShift Int64GreaterThan(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_SGT), x, y);
    }

    AddrShift Int64LessThan(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_SLT), x, y);
    }

    AddrShift Int64LessThanOrEqual(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_SLE), x, y);
    }

    AddrShift Int64GreaterThanOrEqual(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_SGE), x, y);
    }

    AddrShift Word64GreaterThan(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_UGT), x, y);
    }

    AddrShift Word64LessThan(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_ULT), x, y);
    }

    AddrShift Word64LessThanOrEqual(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_ULE), x, y);
    }

    AddrShift Word64GreaterThanOrEqual(AddrShift x, AddrShift y)
    {
        return env_->GetCircuitBuilder().NewLogicGate(OpCode(OpCode::INT64_UGE), x, y);
    }

    // cast operation
    AddrShift ChangeInt64ToInt32(AddrShift val)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::TRUNC_INT64_TO_INT32), val);
    }

    AddrShift ChangeInt64ToPointer(AddrShift val)
    {
        if (PtrValueCode() == ValueCode::INT32) {
            return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::TRUNC_INT64_TO_INT32), val);
        }
        return val;
    }

    AddrShift ChangeInt32ToPointer(AddrShift val)
    {
        if (PtrValueCode() == ValueCode::INT32) {
            return val;
        }
        return ZExtInt32ToInt64(val);
    }

    // math operation
    AddrShift Sqrt(AddrShift x);

    AddrShift GetSetterFromAccessor(AddrShift accessor)
    {
        AddrShift setterOffset = GetPtrConstant(panda::ecmascript::AccessorData::SETTER_OFFSET);
        return Load(MachineType::UINT64_TYPE, accessor, setterOffset);
    }

    AddrShift GetElements(AddrShift object)
    {
        AddrShift elementsOffset = GetInteger32Constant(panda::ecmascript::JSObject::ELEMENTS_OFFSET);
        if (PtrValueCode() == ValueCode::INT64) {
            elementsOffset = SExtInt32ToInt64(elementsOffset);
        }
        // load elements in object
        return Load(MachineType::UINT64_TYPE, object, elementsOffset);
    }

    // object operation
    AddrShift LoadHClass(AddrShift object)
    {
        return ChangeInt32ToPointer(Load(UINT32_TYPE, object));
    }

    AddrShift GetObjectType(AddrShift object)
    {
        AddrShift hclass = LoadHClass(object);
        AddrShift bitfieldOffset = GetPtrConstant(panda::ecmascript::JSHClass::BIT_FIELD_OFFSET);

        AddrShift bitfield = Load(INT64_TYPE, hclass, bitfieldOffset);

        return ChangeInt64ToInt32(
            Word64And(Word64LSR(bitfield, GetWord64Constant(panda::ecmascript::JSHClass::ExtensibleBit::START_BIT)),
                      GetWord64Constant((1LLU << panda::ecmascript::JSHClass::ExtensibleBit::SIZE) - 1)));
    }

    AddrShift IsDictionaryMode(AddrShift object)
    {
        return Word32NotEqual(Word32And(Load(UINT32_TYPE, LoadHClass(object), GetPtrConstant(0)),
                                        GetInteger32Constant(panda::HClass::IS_DICTIONARY_ARRAY)),
                              GetInteger32Constant(0));
    }

    AddrShift IsExtensible(AddrShift object)
    {
        AddrShift hclass = LoadHClass(object);
        AddrShift bitfieldOffset = GetPtrConstant(panda::ecmascript::JSHClass::BIT_FIELD_OFFSET);

        AddrShift bitfield = Load(INT64_TYPE, hclass, bitfieldOffset);
        // decode
        return Word64NotEqual(
            Word64And(Word64LSR(bitfield, GetWord64Constant(panda::ecmascript::JSHClass::ExtensibleBit::START_BIT)),
                      GetWord64Constant((1LLU << panda::ecmascript::JSHClass::ExtensibleBit::SIZE) - 1)),
            GetWord64Constant(0));
    }

    AddrShift IsJsProxy(AddrShift obj)
    {
        AddrShift objectType = GetObjectType(obj);
        return Word32Equal(objectType, GetInteger32Constant(static_cast<int32_t>(panda::ecmascript::JSType::JS_PROXY)));
    }

    AddrShift IsWritable(AddrShift attr)
    {
        return Word32NotEqual(
            Word32And(
                Word32LSR(attr, GetInteger32Constant(panda::ecmascript::PropertyAttributes::WritableField::START_BIT)),
                GetInteger32Constant((1LLU << panda::ecmascript::PropertyAttributes::WritableField::SIZE) - 1)),
            GetInteger32Constant(0));
    }

    AddrShift IsAcesscor(AddrShift attr)
    {
        return Word32NotEqual(
            Word32And(
                Word32LSR(attr,
                          GetInteger32Constant(panda::ecmascript::PropertyAttributes::IsAccessorField::START_BIT)),
                GetInteger32Constant((1LLU << panda::ecmascript::PropertyAttributes::IsAccessorField::SIZE) - 1)),
            GetInteger32Constant(0));
    }

    AddrShift GetPrototypeFromHClass(AddrShift hclass)
    {
        AddrShift protoOffset = GetPtrConstant(panda::ecmascript::JSHClass::PROTOTYPE_OFFSET);
        return Load(TAGGED_TYPE, hclass, protoOffset);
    }

    void StoreElement(AddrShift elements, AddrShift index, AddrShift value)
    {
        AddrShift offset =
            PtrMul(ChangeInt32ToPointer(index), GetPtrConstant(panda::ecmascript::JSTaggedValue::TaggedTypeSize()));
        AddrShift dataOffset = PtrAdd(offset, GetPtrConstant(panda::coretypes::Array::GetDataOffset()));
        Store(TAGGED_TYPE, elements, dataOffset, value);
    }

    void ThrowTypeAndReturn(AddrShift thread, int messageId, AddrShift val);

    AddrShift GetValueFromTaggedArray(AddrShift elements, AddrShift index)
    {
        AddrShift offset =
            PtrMul(ChangeInt32ToPointer(index), GetPtrConstant(panda::ecmascript::JSTaggedValue::TaggedTypeSize()));
        AddrShift dataOffset = PtrAdd(offset, GetPtrConstant(panda::coretypes::Array::GetDataOffset()));
        return Load(TAGGED_TYPE, elements, dataOffset);
    }

    AddrShift TaggedToRepresentation(AddrShift value, Label *next);

    AddrShift GetElementRepresentation(AddrShift hclass)
    {
        AddrShift bitfieldOffset = GetPtrConstant(panda::ecmascript::JSHClass::BIT_FIELD_OFFSET);
        AddrShift bitfield = Load(INT64_TYPE, hclass, bitfieldOffset);
        return Word64And(
            Word64LSR(bitfield, GetWord64Constant(panda::ecmascript::JSHClass::ElementRepresentationBits::START_BIT)),
            GetWord64Constant(((1LLU << panda::ecmascript::JSHClass::ElementRepresentationBits::SIZE) - 1)));
    }

    void SetElementRepresentation(AddrShift hclass, AddrShift value)
    {
        AddrShift bitfieldOffset = GetPtrConstant(panda::ecmascript::JSHClass::BIT_FIELD_OFFSET);
        AddrShift oldValue = Load(INT64_TYPE, hclass, bitfieldOffset);
        Store(INT64_TYPE, hclass, bitfieldOffset,
              Word64Or(Word64And(oldValue,
                                 GetWord64Constant(~panda::ecmascript::JSHClass::ElementRepresentationBits::Mask())),
                       Word64LSR(value, GetWord64Constant(
                           panda::ecmascript::JSHClass::ElementRepresentationBits::START_BIT))));
    }

    void UpdateValueAndDetails(AddrShift elements, AddrShift index, AddrShift value, AddrShift attr)
    {
        AddrShift arrayIndex =
            Int32Add(GetInteger32Constant(panda::ecmascript::NameDictionary::TABLE_HEADER_SIZE),
                     Int32Mul(index, GetInteger32Constant(panda::ecmascript::NameDictionary::ENTRY_SIZE)));
        AddrShift valueIndex =
            Int32Add(arrayIndex, GetInteger32Constant(panda::ecmascript::NameDictionary::ENTRY_VALUE_INDEX));
        AddrShift attributesIndex =
            Int32Add(arrayIndex, GetInteger32Constant(panda::ecmascript::NameDictionary::ENTRY_DETAILS_INDEX));
        StoreElement(elements, valueIndex, value);
        StoreElement(elements, attributesIndex, IntBuildTagged(attr));
    }

    void UpdateAndStoreRepresention(AddrShift hclass, AddrShift value, Label *next);

    AddrShift UpdateRepresention(AddrShift oldRep, AddrShift value, Label *next);

    AddrShift GetDetailsFromDictionary(AddrShift elements, AddrShift entry)
    {
        AddrShift arrayIndex =
            Int32Add(GetInteger32Constant(panda::ecmascript::NumberDictionary::TABLE_HEADER_SIZE),
                     Int32Mul(entry, GetInteger32Constant(panda::ecmascript::NumberDictionary::ENTRY_SIZE)));
        AddrShift attributesIndex =
            Int32Add(arrayIndex, GetInteger32Constant(panda::ecmascript::NameDictionary::ENTRY_DETAILS_INDEX));
        return GetValueFromTaggedArray(elements, attributesIndex);
    }

    AddrShift GetValueFromDictionary(AddrShift elements, AddrShift entry)
    {
        AddrShift arrayIndex =
            Int32Add(GetInteger32Constant(panda::ecmascript::NumberDictionary::TABLE_HEADER_SIZE),
                     Int32Mul(entry, GetInteger32Constant(panda::ecmascript::NumberDictionary::ENTRY_SIZE)));
        AddrShift valueIndex =
            Int32Add(arrayIndex, GetInteger32Constant(panda::ecmascript::NameDictionary::ENTRY_VALUE_INDEX));
        return GetValueFromTaggedArray(elements, valueIndex);
    }

    AddrShift GetKeyFromNumberDictionary(AddrShift elements, AddrShift entry, Label *next);

    AddrShift IsMatchInNumberDictionary(AddrShift key, AddrShift other, Label *next);

    AddrShift FindElementFromNumberDictionary(AddrShift thread, AddrShift elements, AddrShift key, Label *next);

    AddrShift TaggedCastToInt32(AddrShift x)
    {
        AddrShift val = Word64And(x, GetWord64Constant(~panda::ecmascript::JSTaggedValue::TAG_MASK));
        return ChangeInt64ToInt32(val);
    }

    AddrShift TaggedCastToDouble(AddrShift x)
    {
        AddrShift val = Int64Sub(x, GetWord64Constant(panda::ecmascript::JSTaggedValue::DOUBLE_ENCODE_OFFSET));
        return CastInt64ToFloat64(val);
    }

    AddrShift CastInt32ToFloat64(AddrShift x)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT32_TO_FLOAT64), x);
    }

    AddrShift CastInt64ToFloat64(AddrShift x)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::INT64_TO_FLOAT64), x);
    }

    AddrShift SExtInt32ToInt64(AddrShift x)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::SEXT_INT32_TO_INT64), x);
    }

    AddrShift SExtInt1ToInt64(AddrShift x)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::SEXT_INT1_TO_INT64), x);
    }

    AddrShift SExtInt1ToInt32(AddrShift x)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::SEXT_INT1_TO_INT32), x);
    }

    AddrShift ZExtInt32ToInt64(AddrShift x)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::ZEXT_INT32_TO_INT64), x);
    }

    AddrShift ZExtInt1ToInt64(AddrShift x)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::ZEXT_INT1_TO_INT64), x);
    }

    AddrShift ZExtInt1ToInt32(AddrShift x)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::ZEXT_INT1_TO_INT32), x);
    }

    AddrShift TruncInt64ToInt32(AddrShift x)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::TRUNC_INT64_TO_INT32), x);
    }

    AddrShift TruncInt64ToInt1(AddrShift x)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::TRUNC_INT64_TO_INT1), x);
    }

    AddrShift TruncInt32ToInt1(AddrShift x)
    {
        return env_->GetCircuitBuilder().NewArithMeticGate(OpCode(OpCode::TRUNC_INT32_TO_INT1), x);
    }

    uint32_t NextVariableId()
    {
        return varibial_id_++;
    }

private:
    Environment *env_;
    uint32_t varibial_id_{0};
};
}  // namespace kungfu
#endif  // ECMASCRIPT_COMPILER_STUBOPTIMIZER_H
