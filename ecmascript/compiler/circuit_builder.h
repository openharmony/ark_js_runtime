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

#ifndef ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_H
#define ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_H

#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/compiler/variable_type.h"
#include "ecmascript/compiler/stub_descriptor.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;
#define DEFVAlUE(varname, labelmanager, type, val) \
        Variable varname(labelmanager, type, labelmanager->NextVariableId(), val)

class LabelManager;
class Label;
class Variable;
class CircuitBuilder {
public:
    explicit CircuitBuilder(Circuit *circuit) : circuit_(circuit) {}
    explicit CircuitBuilder(Circuit *circuit, LabelManager *lm) : circuit_(circuit), lm_(lm) {}
    ~CircuitBuilder() = default;
    NO_MOVE_SEMANTIC(CircuitBuilder);
    NO_COPY_SEMANTIC(CircuitBuilder);
    GateRef NewArguments(size_t index);
    GateRef NewMerge(GateRef *in, size_t controlCount);
    GateRef NewSelectorGate(OpCode opcode, GateRef control, int valueCounts,
                            VariableType type = VariableType::VOID());
    GateRef NewSelectorGate(OpCode opcode, GateRef control, std::vector<GateRef> &values, int valueCounts,
                            VariableType type = VariableType::VOID());
    GateRef NewSelectorGate(OpCode opcode, MachineType machineType, GateRef control, int valueCounts,
                            VariableType type = VariableType::VOID());
    GateRef NewSelectorGate(OpCode opcode, MachineType machineType, GateRef control, std::vector<GateRef> &values,
                            int valueCounts, VariableType type = VariableType::VOID());
    GateRef NewInt8Constant(int8_t val);
    GateRef NewInt16Constant(int16_t val);
    GateRef NewIntegerConstant(int32_t value);
    GateRef NewInteger64Constant(int64_t value);
    GateRef NewBooleanConstant(bool value);
    GateRef NewDoubleConstant(double value);
    GateRef UndefineConstant(GateType type);
    GateRef HoleConstant(GateType type);
    GateRef NullConstant(GateType type);
    GateRef ExceptionConstant(GateType type);
    GateRef NewRelocatableData(uint64_t val);
    GateRef Alloca(int size);
    GateRef Branch(GateRef state, GateRef condition);
    GateRef SwitchBranch(GateRef state, GateRef index, int caseCounts);
    GateRef Return(GateRef state, GateRef depend, GateRef value);
    GateRef ReturnVoid(GateRef state, GateRef depend);
    GateRef Goto(GateRef state);
    GateRef LoopBegin(GateRef state);
    GateRef LoopEnd(GateRef state);
    GateRef NewIfTrue(GateRef ifBranch);
    GateRef NewIfFalse(GateRef ifBranch);
    GateRef NewSwitchCase(GateRef switchBranch, int64_t value);
    GateRef NewDefaultCase(GateRef switchBranch);
    GateRef NewLoadGate(VariableType type, GateRef val, GateRef depend);
    GateRef NewStoreGate(VariableType type, GateRef ptr, GateRef val, GateRef depend);
    GateRef NewDependRelay(GateRef state, GateRef depend);
    GateRef NewDependAnd(std::initializer_list<GateRef> args);
    GateRef NewNumberGate(OpCode opcode, GateRef value);
    GateRef NewArithmeticGate(OpCode opcode, MachineType machineType, GateRef left, GateRef right);
    GateRef NewArithmeticGate(OpCode opcode, MachineType machineType, GateRef value);
    GateRef NewArithmeticGate(OpCode opcode, GateRef value);
    GateRef NewLogicGate(OpCode opcode, MachineType machineType, GateRef left, GateRef right);
    GateRef NewLogicGate(OpCode opcode, GateRef left, GateRef right);
    GateRef NewLogicGate(OpCode opcode, MachineType machineType, GateRef value);
    GateRef NewCallGate(StubDescriptor *descriptor, GateRef glue, GateRef target,
                                 std::initializer_list<GateRef> args);
    GateRef NewCallGate(StubDescriptor *descriptor, GateRef glue, GateRef target,
                                 GateRef depend, std::initializer_list<GateRef> args);
    GateRef NewRuntimeCallGate(GateRef glue, GateRef target, GateRef depend, std::initializer_list<GateRef> args);
    GateRef NewBytecodeCallGate(StubDescriptor *descriptor, GateRef glue, GateRef target,
                                GateRef depend, std::initializer_list<GateRef> args);
    static MachineType GetLoadMachineTypeFromVariableType(VariableType type);
    static MachineType GetStoreMachineTypeFromVariableType(VariableType type);
    static MachineType GetMachineTypeFromVariableType(VariableType type);
    static MachineType GetCallMachineTypeFromVariableType(VariableType type);

    static GateType VariableType2GateType(VariableType type)
    {
        return type.GetGateType();
    }
    // call operation
    inline GateRef CallRuntime(StubDescriptor *descriptor, GateRef glue, GateRef target,
                               std::initializer_list<GateRef> args);
    inline GateRef CallRuntime(StubDescriptor *descriptor, GateRef glue, GateRef target, GateRef depend,
                               std::initializer_list<GateRef> args);
    inline GateRef CallRuntimeTrampoline(GateRef glue, GateRef target,
                               std::initializer_list<GateRef> args);
    inline GateRef CallRuntimeTrampoline(GateRef glue, GateRef target, GateRef depend,
                               std::initializer_list<GateRef> args);
    // memory
    inline GateRef Load(VariableType type, GateRef base, GateRef offset);
    inline GateRef Load(VariableType type, GateRef base);
    // arithmetic
    inline GateRef IntPtrAdd(GateRef x, GateRef y);

private:
    Circuit *circuit_;
    LabelManager *lm_ {nullptr};
};

class Label {
public:
    explicit Label() = default;
    explicit Label(LabelManager *lm);
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
    class LabelImpl {
    public:
        LabelImpl(LabelManager *lm, GateRef control)
            : lm_(lm), control_(control), predeControl_(-1), isSealed_(false)
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
        LabelManager *lm_;
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
    explicit Label(LabelImpl *impl) : impl_(impl) {}
    friend class LabelManager;
    LabelImpl *GetRawLabel() const
    {
        return impl_;
    }
    LabelImpl *impl_ {nullptr};
};

class LabelManager {
public:
    using LabelImpl = Label::LabelImpl;
    LabelManager(GateRef hir, Circuit *circuit);
    LabelManager(GateRef stateEntry, GateRef dependEntry, std::vector<GateRef>& inlist, Circuit *circuit);
    ~LabelManager();
    Label *GetCurrentLabel() const
    {
        return currentLabel_;
    }
    void SetCurrentLabel(Label *label)
    {
        currentLabel_ = label;
    }
    CircuitBuilder *GetCircuitBuilder()
    {
        return &builder_;
    }
    Circuit *GetCircuit()
    {
        return circuit_;
    }
    int NextVariableId()
    {
        return nextVariableId_++;
    }
    inline GateType GetGateType(GateRef gate) const;
    inline Label GetLabelFromSelector(GateRef sel);
    inline void AddSelectorToLabel(GateRef sel, Label label);
    inline LabelImpl *NewLabel(LabelManager *lm, GateRef control = -1);
    inline void PushCurrentLabel(Label *entry);
    inline void PopCurrentLabel();
    inline GateRef GetInput(size_t index) const;
    template<bool noThrow = false>
    inline void MergeMirCircuit(GateRef hir, GateRef outir,
                                const std::vector<GateRef> &successControl,
                                const std::vector<GateRef> &exceptionControl);
    inline GateRef Return(GateRef value);
    inline GateRef Return();
    inline void Bind(Label *label);
    inline void Bind(Label *label, bool justSlowPath);
    void Jump(Label *label);
    void Branch(GateRef condition, Label *trueLabel, Label *falseLabel);
    void Switch(GateRef index, Label *defaultLabel, int64_t *keysValue, Label *keysLabel, int numberOfKeys);
    void Seal(Label *label)
    {
        label->Seal();
    }
    void LoopBegin(Label *loopHead);
    void LoopEnd(Label *loopHead);
private:
    Label *currentLabel_ {nullptr};
    Circuit *circuit_;
    CircuitBuilder builder_;
    std::unordered_map<GateRef, LabelImpl *> phiToLabels_;
    std::vector<GateRef> inputList_;
    Label entry_;
    std::vector<LabelImpl *> rawLabels_;
    std::stack<Label *> stack_;
    int nextVariableId_ {0};
};

class Variable {
public:
    Variable(LabelManager *lm, VariableType type, uint32_t id, GateRef value) : id_(id), type_(type), lm_(lm)
    {
        Bind(value);
        lm_->GetCurrentLabel()->WriteVariable(this, value);
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
        lm_->GetCurrentLabel()->WriteVariable(this, value);
        Bind(value);
        return *this;
    }
    GateRef operator*()
    {
        return lm_->GetCurrentLabel()->ReadVariable(this);
    }
    GateRef AddPhiOperand(GateRef val);
    GateRef AddOperandToSelector(GateRef val, size_t idx, GateRef in);
    GateRef TryRemoveTrivialPhi(GateRef phi);
    void RerouteOuts(const std::vector<Out *> &outs, Gate *newGate);
    bool IsSelector(GateRef gate) const
    {
        return lm_->GetCircuit()->IsSelector(gate);
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
    LabelManager *lm_;
};
}  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_H
