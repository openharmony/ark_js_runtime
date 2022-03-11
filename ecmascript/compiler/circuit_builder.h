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
#include "ecmascript/compiler/call_signature.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/js_hclass.h"

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
    GateRef NewPtrConstant(int64_t val);
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
    GateRef NewCallGate(const CallSignature *descriptor, GateRef glue, GateRef target,
                                 std::initializer_list<GateRef> args);
    GateRef NewCallGate(const CallSignature *descriptor, GateRef glue, GateRef target,
                                 GateRef depend, std::initializer_list<GateRef> args);
    GateRef NewRuntimeCallGate(GateRef glue, GateRef target, GateRef depend, std::initializer_list<GateRef> args);
    GateRef CallRuntimeVariadic(GateRef glue, GateRef target, GateRef depend, const std::vector<GateRef> &args);
    GateRef NewBytecodeCallGate(const CallSignature *descriptor, GateRef glue, GateRef target,
                                GateRef depend, std::initializer_list<GateRef> args);
    static MachineType GetLoadMachineTypeFromVariableType(VariableType type);
    static MachineType GetStoreMachineTypeFromVariableType(VariableType type);
    static MachineType GetMachineTypeFromVariableType(VariableType type);
    static MachineType GetCallMachineTypeFromVariableType(VariableType type);

    static GateType VariableType2GateType(VariableType type)
    {
        return type.GetGateType();
    }
    // constant
    inline GateRef GetInt8Constant(int8_t value);
    inline GateRef GetInt16Constant(int16_t value);
    inline GateRef GetInt32Constant(int32_t value);
    inline GateRef GetInt64Constant(uint64_t value);
    inline GateRef GetIntPtrConstant(int64_t value);
    inline GateRef GetRelocatableData(uint64_t value);
    inline GateRef TrueConstant();
    inline GateRef FalseConstant();
    inline GateRef GetBooleanConstant(bool value);
    inline GateRef GetArchRelateConstant(uint64_t value);
    inline GateRef GetDoubleConstant(double value);
    inline GateRef GetUndefinedConstant(VariableType type = VariableType::JS_ANY());
    inline GateRef GetHoleConstant(VariableType type = VariableType::JS_ANY());
    inline GateRef GetNullConstant(VariableType type = VariableType::JS_ANY());
    inline GateRef GetExceptionConstant(VariableType type = VariableType::JS_ANY());    
    // call operation
    inline GateRef CallRuntime(const CallSignature *descriptor, GateRef glue, GateRef target,
                               std::initializer_list<GateRef> args);
    inline GateRef CallRuntime(const CallSignature *descriptor, GateRef glue, GateRef target, GateRef depend,
                               std::initializer_list<GateRef> args);
    inline GateRef CallRuntimeTrampoline(GateRef glue, GateRef target,
                               std::initializer_list<GateRef> args);
    inline GateRef CallRuntimeTrampoline(GateRef glue, GateRef target, GateRef depend,
                               std::initializer_list<GateRef> args);
    // memory
    inline GateRef Load(VariableType type, GateRef base, GateRef offset);
    inline GateRef Load(VariableType type, GateRef base);
    // arithmetic
    inline GateRef Int16Add(GateRef x, GateRef y);
    inline GateRef Int32Add(GateRef x, GateRef y);
    inline GateRef Int64Add(GateRef x, GateRef y);
    inline GateRef DoubleAdd(GateRef x, GateRef y);
    inline GateRef IntPtrAdd(GateRef x, GateRef y);
    inline GateRef Int16Sub(GateRef x, GateRef y);
    inline GateRef Int32Sub(GateRef x, GateRef y);
    inline GateRef Int64Sub(GateRef x, GateRef y);
    inline GateRef DoubleSub(GateRef x, GateRef y);
    inline GateRef IntPtrSub(GateRef x, GateRef y);
    inline GateRef Int32Mul(GateRef x, GateRef y);
    inline GateRef Int64Mul(GateRef x, GateRef y);
    inline GateRef DoubleMul(GateRef x, GateRef y);
    inline GateRef IntPtrMul(GateRef x, GateRef y);
    inline GateRef Int32Div(GateRef x, GateRef y);
    inline GateRef Int64Div(GateRef x, GateRef y);
    inline GateRef UInt32Div(GateRef x, GateRef y);
    inline GateRef UInt64Div(GateRef x, GateRef y);
    inline GateRef DoubleDiv(GateRef x, GateRef y);
    inline GateRef IntPtrDiv(GateRef x, GateRef y);
    inline GateRef Int32Mod(GateRef x, GateRef y);
    inline GateRef DoubleMod(GateRef x, GateRef y);
    // bit operation
    inline GateRef BoolAnd(GateRef x, GateRef y);
    inline GateRef Int8And(GateRef x, GateRef y);
    inline GateRef Int32And(GateRef x, GateRef y);
    inline GateRef Int64And(GateRef x, GateRef y);
    inline GateRef IntPtrAnd(GateRef x, GateRef y);
    inline GateRef BoolOr(GateRef x, GateRef y);
    inline GateRef Int32Or(GateRef x, GateRef y);
    inline GateRef Int64Or(GateRef x, GateRef y);
    inline GateRef IntPtrOr(GateRef x, GateRef y);
    inline GateRef Int32Xor(GateRef x, GateRef y);
    inline GateRef Int64Xor(GateRef x, GateRef y);
    inline GateRef BoolNot(GateRef x);
    inline GateRef Int32Not(GateRef x);
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
    // cast operation
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
    inline GateRef ZExtInt32ToPtr(GateRef x);
    inline GateRef SExtInt32ToPtr(GateRef x);
    inline GateRef ZExtInt16ToInt32(GateRef x);
    inline GateRef ZExtInt16ToInt64(GateRef x);
    inline GateRef ChangeInt64ToInt32(GateRef val);
    inline GateRef ChangeInt32ToIntPtr(GateRef val);
    inline GateRef TruncInt64ToInt32(GateRef x);
    inline GateRef TruncPtrToInt32(GateRef x);
    inline GateRef TruncInt64ToInt1(GateRef x);
    inline GateRef TruncInt32ToInt1(GateRef x);
    // compare operation
    inline GateRef Int8Equal(GateRef x, GateRef y);
    inline GateRef Int32Equal(GateRef x, GateRef y);
    inline GateRef Int64Equal(GateRef x, GateRef y);
    inline GateRef IntPtrEqual(GateRef x, GateRef y);
    inline GateRef DoubleEqual(GateRef x, GateRef y);
    inline GateRef Int32NotEqual(GateRef x, GateRef y);
    inline GateRef Int64NotEqual(GateRef x, GateRef y);
    inline GateRef DoubleLessThan(GateRef x, GateRef y);
    inline GateRef DoubleLessThanOrEqual(GateRef x, GateRef y);
    inline GateRef DoubleGreaterThan(GateRef x, GateRef y);
    inline GateRef DoubleGreaterThanOrEqual(GateRef x, GateRef y);
    inline GateRef Int32LessThan(GateRef x, GateRef y);
    inline GateRef Int32LessThanOrEqual(GateRef x, GateRef y);
    inline GateRef Int32GreaterThan(GateRef x, GateRef y);
    inline GateRef Int32GreaterThanOrEqual(GateRef x, GateRef y);
    inline GateRef UInt32LessThan(GateRef x, GateRef y);
    inline GateRef UInt32LessThanOrEqual(GateRef x, GateRef y);
    inline GateRef UInt32GreaterThan(GateRef x, GateRef y);
    inline GateRef UInt32GreaterThanOrEqual(GateRef x, GateRef y);
    inline GateRef Int64LessThan(GateRef x, GateRef y);
    inline GateRef Int64LessThanOrEqual(GateRef x, GateRef y);
    inline GateRef Int64GreaterThan(GateRef x, GateRef y);
    inline GateRef Int64GreaterThanOrEqual(GateRef x, GateRef y);
    inline GateRef UInt64LessThan(GateRef x, GateRef y);
    inline GateRef UInt64LessThanOrEqual(GateRef x, GateRef y);
    inline GateRef UInt6464GreaterThan(GateRef x, GateRef y);
    inline GateRef UInt6464GreaterThanOrEqual(GateRef x, GateRef y);
private:
    Circuit *circuit_;
    LabelManager *lm_ {nullptr};
};

class JsCircuitBuilder {
public:
    explicit JsCircuitBuilder(Circuit *circuit) : circuit_(circuit) {}
    explicit JsCircuitBuilder(Circuit *circuit, LabelManager *lm, CircuitBuilder *builder)
        : circuit_(circuit), lm_(lm), builder_(builder) {}
    ~JsCircuitBuilder() = default;
    NO_MOVE_SEMANTIC(JsCircuitBuilder);
    NO_COPY_SEMANTIC(JsCircuitBuilder);
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
    inline GateRef TaggedIsGeneratorObject(GateRef x);
    inline GateRef TaggedIsPropertyBox(GateRef x);
    inline GateRef TaggedIsWeak(GateRef x);
    inline GateRef TaggedIsPrototypeHandler(GateRef x);
    inline GateRef TaggedIsTransitionHandler(GateRef x);
    GateRef TaggedIsString(GateRef obj);
    GateRef TaggedIsStringOrSymbol(GateRef obj);
    inline GateRef LoadHClass(GateRef object);
    inline GateRef HclassIsPrototypeHandler(GateRef hclass);
    inline GateRef HclassIsTransitionHandler(GateRef hclass);
    inline GateRef HclassIsPropertyBox(GateRef hclass);
    inline GateRef GetObjectType(GateRef hClass);
    inline GateRef IsString(GateRef obj);
    inline GateRef BothIsString(GateRef x, GateRef y);
private:
    [[maybe_unused]]Circuit *circuit_;
    LabelManager *lm_ {nullptr};
    CircuitBuilder *builder_ {nullptr};
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
    JsCircuitBuilder *GetJsCircuitBuilder()
    {
        return &jsBuilder_;
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
    inline void HandleException(GateRef result, Label *success, Label *exception, Label *exit, VariableType type);
    inline void HandleException(GateRef result, Label *success, Label *fail, Label *exit, GateRef exceptionVal);
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
    JsCircuitBuilder jsBuilder_;
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
