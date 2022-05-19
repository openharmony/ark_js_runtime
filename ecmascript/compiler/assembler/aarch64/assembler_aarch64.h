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

#include "ecmascript/compiler/assembler/assembler.h"
#include "assembler_aarch64_constants.h"

namespace panda::ecmascript::aarch64 {
class Register {
public:
    Register(RegisterId reg, RegisterType type = RegisterType::X) : reg_(reg), type_(type) {};
    Register W() const
    {
        return Register(reg_, RegisterType::W);
    }
    Register X() const
    {
        return Register(reg_, RegisterType::X);
    }
    inline bool IsSp() const
    {
        return reg_ == RegisterId::SP;
    }
    inline bool IsW() const
    {
        return type_ == RegisterType::W;
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
    RegisterType type_;
};

class VectorRegister {
public:
    VectorRegister(VectorRegisterId reg, Scale scale = D) : reg_(reg), scale_(scale) {};
    
    inline VectorRegisterId GetId() const
    {
        return reg_;
    }
    inline bool IsValid() const
    {
        return reg_ != VectorRegisterId::INVALID_VREG;
    }
    inline Scale GetScale() const
    {
        return scale_;
    }
    inline int GetRegSize() const
    {
        if (scale_ == B) {
            return 8;
        } else if (scale_ == H) {
            return 16;
        } else if (scale_ == S) {
            return 32;
        } else if (scale_ == D) {
            return 64;
        } else if (scale_ == Q) {
            return 128;
        }
        UNREACHABLE();
    }
private:
    VectorRegisterId reg_;
    Scale scale_;
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
        : reg_(RegisterId::INVALID_REG), extend_(Extend::NO_EXTEND), shift_(Shift::NO_SHIFT),
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
    MemoryOperand(Register base, int64_t offset, AddrMode addrmod = AddrMode::OFFSET)
        : base_(base), offsetReg_(RegisterId::INVALID_REG), offsetImm_(offset), addrmod_(addrmod),
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
    void Ldp(const VectorRegister &vt, const VectorRegister &vt2, const MemoryOperand &operand);
    void Stp(const VectorRegister &vt, const VectorRegister &vt2, const MemoryOperand &operand);
    void Ldr(const Register &rt, const MemoryOperand &operand);
    void Str(const Register &rt, const MemoryOperand &operand);
    void Mov(const Register &rd, const Immediate &imm);
    void Mov(const Register &rd, const Register &rm);
    void Movz(const Register &rd, uint64_t imm, int shift);
    void Movk(const Register &rd, uint64_t imm, int shift);
    void Movn(const Register &rd, uint64_t imm, int shift);
    void Orr(const Register &rd, const Register &rn, const LogicalImmediate &imm);
    void Orr(const Register &rd, const Register &rn, const Operand &operand);
    void And(const Register &rd, const Register &rn, const Operand &operand);
    void And(const Register &rd, const Register &rn, const LogicalImmediate &imm);
    void Lsr(const Register &rd, const Register &rn, unsigned shift);
    void Lsl(const Register &rd, const Register &rn, unsigned shift);
    void Lsl(const Register &rd, const Register &rn, const Register &rm);
    void Lsr(const Register &rd, const Register &rn, const Register &rm);
    void Ubfm(const Register &rd, const Register &rn, unsigned immr, unsigned imms);

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
    void Br(const Register &rn);
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
    // common reg field defines
    inline uint32_t Rd(uint32_t id)
    {
        return (id << COMMON_REG_Rd_LOWBITS) & COMMON_REG_Rd_MASK;
    }

    inline uint32_t Rn(uint32_t id)
    {
        return (id << COMMON_REG_Rn_LOWBITS) & COMMON_REG_Rn_MASK;
    }

    inline uint32_t Rm(uint32_t id)
    {
        return (id << COMMON_REG_Rm_LOWBITS) & COMMON_REG_Rm_MASK;
    }

    inline uint32_t Rt(uint32_t id)
    {
        return (id << COMMON_REG_Rt_LOWBITS) & COMMON_REG_Rt_MASK;
    }

    inline uint32_t Rt2(uint32_t id)
    {
        return (id << COMMON_REG_Rt2_LOWBITS) & COMMON_REG_Rt2_MASK;
    }

    inline uint32_t Sf(uint32_t id)
    {
        return (id << COMMON_REG_Sf_LOWBITS) & COMMON_REG_Sf_MASK;
    }

    inline uint32_t LoadAndStorePairImm(uint32_t imm)
    {
        return (((imm) << LDP_STP_Imm7_LOWBITS) & LDP_STP_Imm7_MASK);
    }

    inline uint32_t LoadAndStoreImm(uint32_t imm, bool isSigned)
    {
        if (isSigned) {
            return (imm << LDR_STR_Imm9_LOWBITS) & LDR_STR_Imm9_MASK;
        } else {
            return (imm << LDR_STR_Imm12_LOWBITS) & LDR_STR_Imm12_MASK;
        }
    }

    inline uint32_t BranchImm19(uint32_t imm)
    {
        return (imm << BRANCH_Imm19_LOWBITS) & BRANCH_Imm19_MASK;
    }

    uint32_t GetOpcFromScale(Scale scale, bool ispair);
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