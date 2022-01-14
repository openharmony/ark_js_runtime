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

#ifndef ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_HELPER_H
#define ECMASCRIPT_COMPILER_CIRCUIT_BUILDER_HELPER_H

#include "ecmascript/compiler/gate.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/machine_type.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;
#define DEFVAlUE(varname, labelmanager, type, val) \
        Variable varname(labelmanager, type, labelmanager->NextVariableId(), val)

class LabelManager;
class Label;
class Variable;

class Label {
public:
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
    explicit Label() = default;
    explicit Label(LabelManager *lm);
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
    ~LabelManager() = default;
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
    int NextVariableId()
    {
        return nextVariableId_++;
    }
    inline TypeCode GetTypeCode(GateRef gate) const;
    inline Label GetLabelFromSelector(GateRef sel);
    inline void AddSelectorToLabel(GateRef sel, Label label);
    inline LabelImpl *NewLabel(LabelManager *lm, GateRef control = -1);
    inline void PushCurrentLabel(Label *entry);
    inline void PopCurrentLabel();
    inline GateRef GetInput(size_t index) const;
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
    Variable(LabelManager *lm, MachineType type, uint32_t id, GateRef value) : id_(id), type_(type), lm_(lm)
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
    MachineType Type() const
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
        return gate->GetOpCode() >= OpCode::VALUE_SELECTOR_JS
                && gate->GetOpCode() <= OpCode::VALUE_SELECTOR_FLOAT64;
    }
    uint32_t GetId() const
    {
        return id_;
    }
private:
    uint32_t id_;
    MachineType type_;
    GateRef currentValue_ {0};
    LabelManager *lm_;
};
} // panda::ecmascript::kungfu
#endif