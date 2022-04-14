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
#ifndef ECMASCRIPT_COMPILER_ASSEMBLER_X64_H
#define ECMASCRIPT_COMPILER_ASSEMBLER_X64_H

#include "assembler.h"

namespace panda::ecmascript::x64 {
using Immediate = int32_t;

enum Register : uint8_t {
    rax = 0,
    rcx,
    rdx,
    rbx,
    rsp,
    rbp,
    rsi,
    rdi,
    r8,
    r9,
    r10,
    r11,
    r12,
    r13,
    r14,
    r15
};

enum Scale : uint8_t {
    Times1 = 0,
    Times2,
    Times4,
    Times8
};

class Operand {
public:
    Operand(Register base, int32_t disp);
    Operand(Register base, Register index, Scale scale, int32_t disp);

    void BuildSIB(Scale scale, Register index, Register base);
    void BuildModerm(int32_t mode, Register rm);
    void BuildDisp8(int32_t disp);
    void BuildDisp32(int32_t disp);
    int32_t disp_ = 0;
    uint8_t rex_ = 0;
    uint8_t sib_ = 0;
    uint8_t moderm_ = 0;
    bool hasSIB_ = false;
    bool hasDisp8_ = false;
    bool hasDisp32_ = false;
};

// The Intel 64 instruction format is:
// | prefixs| opcode| modR/M| SIB| Displacement| Immediate|
class AssemblerX64 : public Assembler {
public:
    explicit AssemblerX64(Chunk *chunk)
        : Assembler(chunk)
    {
    }
    void Pushq(Register x);
    void Pushq(Immediate x);
    void Popq(Register x);
    void Movq(Register src, Register dst);
    void Movq(Operand src, Register dst);
    void Movq(Register src, Operand dst);
    void Movq(Immediate src, Operand dst);
    void Addq(Immediate src, Register dst);
    void Subq(Immediate src, Register dst);
    void Cmpq(Immediate src, Register dst);
    void Callq(Register addr);
    void Ret();
    void Jmp(Label *target, Distance distance = Distance::FAR);
    void Bind(Label* target);
    void Align16();

private:
    void EmitRexPrefix(Register x)
    {
        if (HighBit(x) != 0) {
            EmitU8(REX_PREFIX_B);
        }
    }

    void EmitRexPrefixW(Register rm)
    {
        EmitU8(REX_PREFIX_W | HighBit(rm));
    }

    void EmitRexPrefix(Operand rm)
    {
        // 0: Extension to the MODRM.rm field B
        EmitU8(REX_PREFIX_W | rm.rex_);
    }

    void EmitRexPrefix(Register reg, Register rm)
    {
        // 0: Extension to the MODRM.rm field B
        // 2: Extension to the MODRM.reg field R
        EmitU8(REX_PREFIX_W | (HighBit(reg) << 2) | HighBit(rm));
    }

    void EmitRexPrefix(Register reg, Operand rm)
    {
        // 0: Extension to the MODRM.rm field B
        // 2: Extension to the MODRM.reg field R
        EmitU8(REX_PREFIX_W | (HighBit(reg) << 2) | rm.rex_);
    }

    // +---+---+---+---+---+---+---+---+
    // |  mod  |    reg    |     rm    |
    // +---+---+---+---+---+---+---+---+
    void EmitModrm(int32_t reg, Register rm)
    {
        EmitU8(MODE_RM | (reg << LOW_BITS_SIZE) | LowBits(rm));
    }

    void EmitModrm(Register reg, Register rm)
    {
        EmitModrm(LowBits(reg), rm);
    }

    void EmitOperand(Register reg, Operand rm)
    {
        EmitOperand(LowBits(reg), rm);
    }
    void EmitOperand(int32_t reg, Operand rm);
    void EmitJmp(int32_t offset);

    // +---+---+---+---+---+---+---+---+
    // | 0   1   0   0 | W | R | X | B |
    // +---+---+---+---+---+---+---+---+
    static constexpr uint8_t REX_PREFIX_FIXED_BITS = 0x40;
    static constexpr uint8_t REX_PREFIX_B = 0x41;
    static constexpr uint8_t REX_PREFIX_W = 0x48;
    // b11
    static constexpr uint8_t MODE_RM = 0xC0;
    // low bits: 3, high bit 1
    static constexpr size_t LOW_BITS_SIZE = 3;
    static constexpr size_t LOW_BITS_MASK = (1 << LOW_BITS_SIZE) - 1;

    static uint8_t GetModrm(int32_t mode, Register rm)
    {
        // [r/m]
        // [r/m + disp8]
        // [r/m + disp32]
        // 6: offset of mode
        return (mode << 6) | LowBits(rm);
    }
    static uint8_t GetModrmRex(Register rm)
    {
        return HighBit(rm);
    }
    // +---+---+---+---+---+---+---+---+
    // | scale |   index   |    base   |
    // +---+---+---+---+---+---+---+---+
    static uint8_t GetSIB(Scale scale, Register index, Register base)
    {
        // 6: offset of scale
        return (scale << 6) | (LowBits(index) << LOW_BITS_SIZE) |
            LowBits(base);
    }
    static uint8_t GetSIBRex(Register index, Register base)
    {
        return (HighBit(index) << 1) | HighBit(base);
    }
    static uint32_t LowBits(Register x)
    {
        return x & LOW_BITS_MASK;
    }
    static uint32_t HighBit(Register x)
    {
        return x >> LOW_BITS_SIZE;
    }
    friend class Operand;
};
}  // panda::ecmascript::x64
#endif  // ECMASCRIPT_COMPILER_ASSEMBLER_X64_H
