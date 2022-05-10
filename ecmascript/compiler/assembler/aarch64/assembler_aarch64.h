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
#ifndef ECMASCRIPT_COMPILER_ASSEMBLER_AARCH64_H
#define ECMASCRIPT_COMPILER_ASSEMBLER_AARCH64_H

#include "assembler.h"
#include "assembler_aarch64_constants.h"

namespace panda::ecmascript::aarch64 {
class Register {
public:
    Register(RegisterId reg, bool isWReg = false) : reg_(reg), isWReg_(isWReg) {};
    Register W() const
    {
        return Register(reg_, true);
    }
    Register X() const
    {
        return Register(reg_, false);
    }
    inline bool IsSp() const
    {
        return reg_ == RegisterId::SP;
    }
    inline bool IsW() const
    {
        return isWReg_;
    }
    inline RegisterId GetId() const
    {
        return reg_;
    }
    inline bool IsValid() const
    {
        return reg_ != RegisterId::INVALID_REG;
    }
private:
    RegisterId reg_;
    bool isWReg_;
};

class Immediate {
public:
    Immediate(int64_t value) : value_(value)
    {
    }

    int64_t Value() const
    {
        return value_;
    }
private:
    int64_t value_;
};

class LogicalImmediate {
public:
    static LogicalImmediate Create(uint64_t imm, int width);
    int Value() const
    {
        ASSERT(IsValid());
        return imm_;
    }

    bool IsValid() const
    {
        return imm_ != InvalidLogicalImmediate;
    }

    bool Is64bit() const
    {
        return imm_ & BITWISE_OP_N_MASK;
    }
private:
    explicit LogicalImmediate(int value)
        : imm_(value)
    {
    }
    static const int InvalidLogicalImmediate = -1;
    int imm_;
};

class Operand {
public:
    Operand(Immediate imm)
        : reg_(RegisterId::INVALID_REG, false), extend_(Extend::NO_EXTEND), shift_(Shift::NO_SHIFT),
          shiftAmount_(0), immediate_(imm)
    {
    }
    Operand(Register reg, Shift shift = Shift::LSL, uint8_t shift_amount = 0)
        : reg_(reg), extend_(Extend::NO_EXTEND), shift_(shift), shiftAmount_(shift_amount), immediate_(0)
    {
    }
    Operand(Register reg, Extend extend, uint8_t shiftAmount = 0)
        : reg_(reg), extend_(extend), shift_(Shift::NO_SHIFT), shiftAmount_(shiftAmount), immediate_(0)
    {
    }

    inline bool IsImmediate() const
    {
        return !reg_.IsValid();
    }

    inline bool IsShifted() const
    {
        return reg_.IsValid() && shift_ != Shift::NO_SHIFT;
    }

    inline bool IsExtended() const
    {
        return reg_.IsValid() && extend_ != Extend::NO_EXTEND;
    }

    inline Register Reg() const
    {
        return reg_;
    }

    inline Shift GetShiftOption() const
    {
        return shift_;
    }

    inline Extend GetExtendOption() const
    {
        return extend_;
    }

    inline uint8_t GetShiftAmount() const
    {
        return shiftAmount_;
    }

    inline int64_t ImmediateValue() const
    {
        return immediate_.Value();
    }

    inline Immediate GetImmediate() const
    {
        return immediate_;
    }
private:
    Register reg_;
    Extend  extend_;
    Shift  shift_;
    uint8_t  shiftAmount_;
    Immediate immediate_;
};

class MemoryOperand {
public:
    enum class AddrMode {
        OFFSET,
        PREINDEX,
        POSTINDEX
    };
    MemoryOperand(Register base, Register offset, Extend extend, uint8_t  shiftAmount = 0)
        : base_(base), offsetReg_(offset), offsetImm_(0), addrmod_(AddrMode::OFFSET),
          extend_(extend), shift_(Shift::NO_SHIFT), shiftAmount_(shiftAmount)
    {
    }
    MemoryOperand(Register base, Register offset, Shift shift = Shift::NO_SHIFT, uint8_t  shiftAmount = 0)
        : base_(base), offsetReg_(offset), offsetImm_(0), addrmod_(AddrMode::OFFSET),
          extend_(Extend::NO_EXTEND), shift_(shift), shiftAmount_(shiftAmount)
    {
    }
    MemoryOperand(Register base, int64_t offset, AddrMode addrmod = OFFSET)
        : base_(base), offsetReg_(RegisterId::INVALID_REG, false), offsetImm_(offset), addrmod_(addrmod),
          extend_(Extend::NO_EXTEND), shift_(Shift::NO_SHIFT), shiftAmount_(0)
    {
    }

    Register GetRegBase() const
    {
        return base_;
    }

    bool IsImmediateOffset() const
    {
        return !offsetReg_.IsValid();
    }

    Immediate GetImmediate() const
    {
        return offsetImm_;
    }

    AddrMode GetAddrMode() const
    {
        return addrmod_;
    }

    Extend GetExtendOption() const
    {
        return extend_;
    }

    Shift GetShiftOption() const
    {
        return shift_;
    }

    uint8_t GetShiftAmount() const
    {
        return shiftAmount_;
    }
private:
    Register base_;
    Register offsetReg_;
    Immediate offsetImm_;
    AddrMode addrmod_;
    Extend extend_;
    Shift shift_;
    uint8_t shiftAmount_;
};

class AssemblerAarch64 : public Assembler {
public:
    explicit AssemblerAarch64(Chunk *chunk)
        : Assembler(chunk)
    {
    }
    void Ldp(const Register &rt, const Register &rt2, const MemoryOperand &operand);
    void Stp(const Register &rt, const Register &rt2, const MemoryOperand &operand);
    void Ldr(const Register &rt, const MemoryOperand &operand);
    void Str(const Register &rt, const MemoryOperand &operand);
    void Mov(const Register &rd, const Immediate &imm);
    void Mov(const Register &rd, const Register &rm);
    void Movz(const Register &rd, uint64_t imm, int shift);
    void Movk(const Register &rd, uint64_t imm, int shift);
    void Movn(const Register &rd, uint64_t imm, int shift);
    void Orr(const Register &rd, const Register &rn, const LogicalImmediate &imm);
    void Orr(const Register &rd, const Register &rn, const Operand &operand);

    void Add(const Register &rd, const Register &rn, const Operand &operand);
    void Adds(const Register &rd, const Register &rn, const Operand &operand);
    void Sub(const Register &rd, const Register &rn, const Operand &operand);
    void Subs(const Register &rd, const Register &rn, const Operand &operand);
    void Cmp(const Register &rd, const Operand &operand);
    void CMov(const Register &rd, const Register &rn, const Operand &operand, Condition cond);
    void B(Label *label);
    void B(int32_t imm);
    void B(Condition cond, Label *label);
    void B(Condition cond, int32_t imm);
    void Blr(const Register &rn);
    void Bl(Label *label);
    void Bl(int32_t imm);
    void Cbz(const Register &rt, int32_t imm);
    void Cbz(const Register &rt, Label *label);
    void Cbnz(const Register &rt, int32_t imm);
    void Cbnz(const Register &rt, Label *label);
    void Tbz(const Register &rt, int32_t bitPos, Label *label);
    void Tbz(const Register &rt, int32_t bitPos, int32_t imm);
    void Tbnz(const Register &rt, int32_t bitPos, Label *label);
    void Tbnz(const Register &rt, int32_t bitPos, int32_t imm);
    void Ret();
    void Ret(const Register &rn);
    void Brk(const Immediate &imm);
    void Bind(Label *target);
private:
    bool IsAddSubImm(uint64_t imm);
    void AddSubImm(AddSubOpCode op, const Register &rd, const Register &rn, bool setFlags, uint64_t imm);
    void AddSubReg(AddSubOpCode op, const Register &rd, const Register &rn, bool setFlags, const Operand &operand);
    void MovWide(uint32_t op, const Register &rd, uint64_t imm, int shift);
    void BitWiseOpImm(BitwiseOpCode op, const Register &rd, const Register &rn, uint64_t imm);
    void BitWiseOpShift(BitwiseOpCode op, const Register &rd, const Register &rn, const Operand &operand);
    bool TrySequenceOfOnes(const Register &rd, uint64_t imm);
    bool TryReplicateHWords(const Register &rd, uint64_t imm);
    void EmitMovInstruct(const Register &rd, uint64_t imm,
                         unsigned int allOneHWords, unsigned int allZeroHWords);
    int32_t GetLinkOffsetFromBranchInst(int32_t pos);
    int32_t LinkAndGetInstOffsetToLabel(Label *label);
    int32_t ImmBranch(uint32_t branchCode);
    void SetRealOffsetToBranchInst(uint32_t linkPos, int32_t disp);
};
}  // namespace panda::ecmascript::aarch64
#endif