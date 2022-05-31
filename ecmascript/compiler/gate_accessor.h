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

#ifndef ECMASCRIPT_COMPILER_GATE_ACCESSOR_H
#define ECMASCRIPT_COMPILER_GATE_ACCESSOR_H

#include "circuit.h"
#include "gate.h"

namespace panda::ecmascript::kungfu {
class GateAccessor {
public:
    // do not create new gate or modify self during iteration
    struct ConstUsesIterator {
        explicit ConstUsesIterator(const Circuit* circuit, const Out* out) : circuit_(circuit), out_(out)
        {
        }

        GateRef operator*() const
        {
            return circuit_->SaveGatePtr(out_->GetGateConst());
        }

        const ConstUsesIterator operator++()
        {
            if (!out_->IsNextOutNull()) {
                out_ = out_->GetNextOutConst();
                return *this;
            }
            out_ = nullptr;
            return *this;
        }
        const ConstUsesIterator operator++(int)
        {
            ConstUsesIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        size_t GetIndex() const
        {
            ASSERT(out_ != nullptr);
            return out_->GetIndex();
        }

        OpCode GetOpCode() const
        {
            ASSERT(out_ != nullptr);
            return out_->GetGateConst()->GetOpCode();
        }

        friend bool operator== (const ConstUsesIterator& a, const ConstUsesIterator& b)
        {
            return a.out_ == b.out_;
        };
        friend bool operator!= (const ConstUsesIterator& a, const ConstUsesIterator& b)
        {
            return a.out_ != b.out_;
        };

    private:
        const Circuit* circuit_;
        const Out* out_;
    };

    // do not create new gate or modify self during iteration
    struct UsesIterator {
        explicit UsesIterator(Circuit* circuit, Out* out, GateRef gate) : circuit_(circuit), out_(out), gate_(gate)
        {
        }

        void SetChanged()
        {
            changed_ = true;
        }

        GateRef operator*()
        {
            return circuit_->SaveGatePtr(out_->GetGate());
        }

        const UsesIterator& operator++()
        {
            if (changed_) {
                if (circuit_->LoadGatePtrConst(gate_)->IsFirstOutNull()) {
                    out_ = nullptr;
                    gate_ = 0;
                } else {
                    out_ = circuit_->LoadGatePtr(gate_)->GetFirstOut();
                }
                changed_ = false;
                return *this;
            }
            if (out_->IsNextOutNull()) {
                out_ = nullptr;
                return *this;
            }
            out_ = out_->GetNextOut();
            return *this;
        }

        UsesIterator operator++(int)
        {
            UsesIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        size_t GetIndex() const
        {
            ASSERT(out_ != nullptr);
            return out_->GetIndex();
        }

        OpCode GetOpCode() const
        {
            ASSERT(out_ != nullptr);
            return out_->GetGateConst()->GetOpCode();
        }

        friend bool operator== (const UsesIterator& a, const UsesIterator& b)
        {
            return a.out_ == b.out_;
        };
        friend bool operator!= (const UsesIterator& a, const UsesIterator& b)
        {
            return a.out_ != b.out_;
        };

    private:
        Circuit* circuit_;
        Out* out_;
        GateRef gate_;
        bool changed_ {false};
    };

    struct ConstInsIterator {
        explicit ConstInsIterator(const Circuit* circuit, const In* in) : circuit_(circuit), in_(in)
        {
        }

        GateRef operator*() const
        {
            return circuit_->SaveGatePtr(in_->GetGateConst());
        }

        const ConstInsIterator& operator++()
        {
            in_++;
            return *this;
        }
        ConstInsIterator operator++(int)
        {
            ConstInsIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        OpCode GetOpCode() const
        {
            ASSERT(in_ != nullptr);
            return in_->GetGateConst()->GetOpCode();
        }

        friend bool operator== (const ConstInsIterator& a, const ConstInsIterator& b)
        {
            return a.in_ == b.in_;
        };
        friend bool operator!= (const ConstInsIterator& a, const ConstInsIterator& b)
        {
            return a.in_ != b.in_;
        };

    private:
        const Circuit* circuit_;
        const In* in_;
    };

    struct InsIterator {
        explicit InsIterator(const Circuit* circuit, In* in) : circuit_(circuit), in_(in)
        {
        }

        GateRef operator*()
        {
            return circuit_->SaveGatePtr(in_->GetGate());
        }

        const InsIterator& operator++()
        {
            in_++;
            return *this;
        }
        InsIterator operator++(int)
        {
            InsIterator tmp = *this;
            ++(*this);
            return tmp;
        }

        OpCode GetOpCode() const
        {
            ASSERT(in_ != nullptr);
            return in_->GetGateConst()->GetOpCode();
        }

        friend bool operator== (const InsIterator& a, const InsIterator& b)
        {
            return a.in_ == b.in_;
        };
        friend bool operator!= (const InsIterator& a, const InsIterator& b)
        {
            return a.in_ != b.in_;
        };

    private:
        const Circuit* circuit_;
        In* in_;
    };

    struct ConstUseWrapper {
        const GateAccessor& iterable;
        GateRef gate;
        auto begin()
        {
            return iterable.ConstUseBegin(gate);
        }
        auto end()
        {
            return iterable.ConstUseEnd();
        }
    };

    struct UseWrapper {
        GateAccessor& iterable;
        GateRef gate;
        auto begin()
        {
            return iterable.UseBegin(gate);
        }
        auto end()
        {
            return iterable.UseEnd();
        }
    };

    struct ConstInWrapper {
        const GateAccessor& iterable;
        const GateRef gate;
        auto begin()
        {
            return iterable.ConstInBegin(gate);
        }
        auto end()
        {
            return iterable.ConstInEnd(gate);
        }
    };

    struct InWrapper {
        GateAccessor& iterable;
        GateRef gate;
        auto begin()
        {
            return iterable.InBegin(gate);
        }
        auto end()
        {
            return iterable.InEnd(gate);
        }
    };

    [[nodiscard]] ConstInWrapper ConstIns(GateRef gate)
    {
        return { *this, gate };
    }

    [[nodiscard]] InWrapper Ins(GateRef gate)
    {
        return { *this, gate };
    }

    [[nodiscard]] ConstUseWrapper ConstUses(GateRef gate)
    {
        return { *this, gate };
    }

    [[nodiscard]] UseWrapper Uses(GateRef gate)
    {
        return { *this, gate };
    }

    explicit GateAccessor(Circuit *circuit) : circuit_(circuit)
    {
    }
    Circuit *GetCircuit() const
    {
        return circuit_;
    }

    ~GateAccessor() = default;

    [[nodiscard]] size_t GetNumIns(GateRef gate) const;
    [[nodiscard]] OpCode GetOpCode(GateRef gate) const;
    void SetOpCode(GateRef gate, OpCode::Op opcode);
    [[nodiscard]] BitField GetBitField(GateRef gate) const;
    void SetBitField(GateRef gate, BitField bitField);
    void Print(GateRef gate) const;
    [[nodiscard]] GateId GetId(GateRef gate) const;
    [[nodiscard]] GateRef GetValueIn(GateRef gate, size_t idx) const;
    [[nodiscard]] size_t GetNumValueIn(GateRef gate) const;
    [[nodiscard]] GateRef GetIn(GateRef gate, size_t idx) const;
    [[nodiscard]] GateRef GetState(GateRef gate, size_t idx = 0) const;
    [[nodiscard]] GateRef GetDep(GateRef gate, size_t idx = 0) const;
    [[nodiscard]] size_t GetImmediateId(GateRef gate) const;
    void SetDep(GateRef gate, GateRef depGate, size_t idx = 0);
    void ReplaceIn(UsesIterator &useIt, GateRef replaceGate);
    // Add for lowering
    [[nodiscard]] GateType GetGateType(GateRef gate) const;
    void SetGateType(GateRef gate, GateType gt);
    void DeleteExceptionDep(UsesIterator &useIt);
    void DeleteIn(UsesIterator &useIt);
    void DeleteGate(UsesIterator &useIt);
    void DecreaseIn(UsesIterator &useIt);

private:
    [[nodiscard]] ConstUsesIterator ConstUseBegin(GateRef gate) const
    {
        if (circuit_->LoadGatePtrConst(gate)->IsFirstOutNull()) {
            return ConstUsesIterator(circuit_, nullptr);
        }
        auto use = circuit_->LoadGatePtrConst(gate)->GetFirstOutConst();
        return ConstUsesIterator(circuit_, use);
    }

    [[nodiscard]] ConstUsesIterator ConstUseEnd() const
    {
        return ConstUsesIterator(circuit_, nullptr);
    }

    [[nodiscard]] UsesIterator UseBegin(GateRef gate) const
    {
        if (circuit_->LoadGatePtrConst(gate)->IsFirstOutNull()) {
            return UsesIterator(circuit_, nullptr, 0);
        }
        auto use = circuit_->LoadGatePtr(gate)->GetFirstOut();
        return UsesIterator(circuit_, use, gate);
    }

    [[nodiscard]] UsesIterator UseEnd() const
    {
        return UsesIterator(circuit_, nullptr, 0);
    }

    [[nodiscard]] ConstInsIterator ConstInBegin(GateRef gate) const
    {
        return ConstInsIterator(circuit_, &reinterpret_cast<const In *>(circuit_->LoadGatePtrConst(gate) + 1)[0]);
    }

    [[nodiscard]] ConstInsIterator ConstInEnd(GateRef gate) const
    {
        auto endIndex = circuit_->LoadGatePtrConst(gate)->GetNumIns();
        return ConstInsIterator(circuit_,
                                &reinterpret_cast<const In *>(circuit_->LoadGatePtrConst(gate) + 1)[endIndex]);
    }

    [[nodiscard]] InsIterator InBegin(GateRef gate)
    {
        return InsIterator(circuit_, &reinterpret_cast<In *>(circuit_->LoadGatePtr(gate) + 1)[0]);
    }

    [[nodiscard]] InsIterator InEnd(GateRef gate)
    {
        auto endIndex = circuit_->LoadGatePtrConst(gate)->GetNumIns();
        return InsIterator(circuit_, &reinterpret_cast<In *>(circuit_->LoadGatePtr(gate) + 1)[endIndex]);
    }

    Circuit *circuit_;
};
}
#endif  // ECMASCRIPT_COMPILER_GATE_ACCESSOR_H
