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
#include "assembler_x64.h"

namespace panda::ecmascript::x64 {
void AssemblerX64::Pushq(Register x)
{
    EmitRexPrefix(x);
    // 0x50: Push r16
    EmitU8(0x50 | LowBits(x));
}

void AssemblerX64::Pushq(Immediate x)
{
    if (InRange8(x)) {
        // 6A: Push imm8
        EmitU8(0x6A);
        EmitI8(static_cast<int8_t>(x));
    } else {
        // 68: Push imm32
        EmitU8(0x68);
        EmitI32(x);
    }
}

void AssemblerX64::Popq(Register x)
{
    EmitRexPrefix(x);
    // 0x58: Pop r16
    EmitU8(0x58 | LowBits(x));
}

void AssemblerX64::Addq(Immediate src, Register dst)
{
    EmitRexPrefixW(dst);
    if (InRange8(src)) {
        // 83: Add r/m16, imm8
        EmitU8(0x83);
        // 0: 83 /0 ib
        EmitModrm(0, dst);
        EmitI8(static_cast<int8_t>(src));
    } else if (dst == rax) {
        // 0x5: Add rax, imm32
        EmitU8(0x5);
        EmitI32(src);
    } else {
        // 81: Add r/m32, imm32
        EmitU8(0x81);
        // 0: 81 /0 id
        EmitModrm(0, dst);
        EmitI32(src);
    }
}

void AssemblerX64::Subq(Immediate src, Register dst)
{
    EmitRexPrefixW(dst);
    if (InRange8(src)) {
        // 83: Sub r/m16, imm8
        EmitU8(0x83);
        // 5: 83 /5 ib
        EmitModrm(5, dst);
        EmitI8(static_cast<int8_t>(src));
    } else if (dst == rax) {
        // 0x2D: Sub rax, imm32
        EmitU8(0x2D);
        EmitI32(src);
    } else {
        // 81: sub r/m32, imm32
        EmitU8(0x81);
        // 5: 81 /5 id
        EmitModrm(5, dst);
        EmitI32(src);
    }
}

void AssemblerX64::Cmpq(Immediate src, Register dst)
{
    EmitRexPrefixW(dst);
    if (InRange8(src)) {
        // 83: cmp r/m16, imm8
        EmitU8(0x83);
        // 7: 83 /7 ib
        EmitModrm(7, dst);
        EmitI8(static_cast<int8_t>(src));
    } else if (dst == rax) {
        // 0x3D: cmp rax, imm32
        EmitU8(0x3D);
        EmitI32(src);
    } else {
        // 81: cmp r/m32, imm32
        EmitU8(0x81);
        // 7: 81 /7 id
        EmitModrm(7, dst);
        EmitI32(src);
    }
}

void AssemblerX64::Movq(Register src, Register dst)
{
    EmitRexPrefix(src, dst);
    // 0x89: Move r16 to r/m16
    EmitU8(0x89);
    EmitModrm(src, dst);
}

void AssemblerX64::Align16()
{
    auto pos = GetCurrentPosition();
    // 16: align16
    size_t x = 16;
    size_t delta = static_cast<size_t>(x - (pos & (x - 1)));
    for (size_t i = 0; i < delta; i++) {
        // 0x90: NOP
        EmitU8(0x90);
    }
}

void AssemblerX64::Movq(Operand src, Register dst)
{
    EmitRexPrefix(dst, src);
    // 0x8B: Move r/m64 to r64
    EmitU8(0x8B);
    EmitOperand(dst, src);
}

void AssemblerX64::Movq(Register src, Operand dst)
{
    EmitRexPrefix(src, dst);
    // 0x89: Move r64 to r/m64
    EmitU8(0x89);
    EmitOperand(src, dst);
}

void AssemblerX64::Movq(Immediate src, Operand dst)
{
    EmitRexPrefix(dst);
    // 0xC7: Move imm32 to r/m32
    EmitU8(0xC7);
    // 0: C7 /0 id
    EmitOperand(0, dst);
    EmitI32(src);
}

void AssemblerX64::EmitJmp(int32_t offset)
{
    offset--;
    if (InRange8(offset - sizeof(int8_t))) {
        // EB: Jmp rel8
        EmitU8(0xEB);
        EmitI8(offset - sizeof(int8_t));
    } else {
        // E9: Jmp rel32
        EmitU8(0xE9);
        EmitI32(offset - sizeof(int32_t));
    }
}

void AssemblerX64::Callq(Register addr)
{
    // C3: RET Near return to calling procedure
    EmitRexPrefix(addr);
    // FF: Call r/m16
    EmitU8(0xFF);
    // 0x2: FF /2
    EmitModrm(0x2, addr);
}

void AssemblerX64::Ret()
{
    // C3: RET Near return to calling procedure
    EmitU8(0xC3);
}

void AssemblerX64::Jmp(Label *target, Distance distance)
{
    if (target->IsBound()) {
        int32_t offset = target->GetPos() - GetCurrentPosition();
        EmitJmp(offset);
        return;
    }

    auto pos = GetCurrentPosition();
    int32_t emitPos = 0;
    if (distance == Distance::NEAR) {
        // EB: Jmp rel8
        EmitU8(0xEB);
        if (target->IsLinkedNear()) {
            emitPos = target->GetLinkedNearPos() - pos;
        }
        target->LinkNearPos(pos);
        ASSERT(InRange8(emitPos));
        EmitI8(static_cast<int8_t>(emitPos));
    } else {
        if (target->IsLinked()) {
            emitPos = target->GetLinkedPos();
        }
        // +1: skip opcode
        target->LinkTo(pos + 1);
        // E9: Jmp rel32
        EmitU8(0xE9);
        EmitI32(emitPos);
    }
}

void AssemblerX64::Bind(Label *target)
{
    size_t pos = GetCurrentPosition();
    ASSERT(!target->IsBound());

    if (target->IsLinked()) {
        uint32_t linkPos = target->GetLinkedPos();
        while (linkPos != 0) {
            linkPos = GetU32(linkPos);
            int32_t disp = (pos - linkPos - sizeof(int32_t));
            PutI32(linkPos, disp);
        }
    }

    if (target->IsLinkedNear()) {
        uint32_t linkPos = target->GetLinkedNearPos();
        while (linkPos != 0) {
            int8_t offsetToNext = GetI8(static_cast<size_t>(linkPos));
            linkPos += offsetToNext;
            int32_t disp = (pos - linkPos - sizeof(int8_t));
            ASSERT(InRange8(disp));
            PutI8(linkPos, static_cast<int8_t>(disp));
        }
        target->UnlinkNearPos();
    }

    target->BindTo(pos);
}

Operand::Operand(Register base, int32_t disp)
{
    if (base == rsp || base == r12) {
        BuildSIB(Times1, rsp, base);
    }
    if (disp == 0 && base != rbp && base != r13) {
        // 0: mode 00 [r/m]
        BuildModerm(0, base);
    } else if (AssemblerX64::InRange8(disp)) {
        // 1: mode 01 [r/m + disp8]
        BuildModerm(1, base);
        BuildDisp8(disp);
    } else {
        // 2: mode 10 [r/m + disp32]
        BuildModerm(2, base);
        BuildDisp32(disp);
    }
}

Operand::Operand(Register base, Register index, Scale scale, int32_t disp)
{
    BuildSIB(scale, index, base);
    if (disp == 0 && base != rbp && base != r13) {
        // 0: mode 00 [r/m]
        BuildModerm(0, rsp);
    } else if (AssemblerX64::InRange8(disp)) {
        // 1: mode 01 [r/m + disp8]
        BuildModerm(1, rsp);
        BuildDisp8(disp);
    } else {
        // 2: mode 10 [r/m + disp32]
        BuildModerm(2, rsp);
        BuildDisp32(disp);
    }
}

void Operand::BuildSIB(Scale scale, Register index, Register base)
{
    sib_ = AssemblerX64::GetSIB(scale, index, base);
    rex_ |= AssemblerX64::GetSIBRex(index, base);
    hasSIB_ = true;
}

void Operand::BuildModerm(int32_t mode, Register rm)
{
    rex_ |= AssemblerX64::GetModrmRex(rm);
    moderm_ = AssemblerX64::GetModrm(mode, rm);
}

void Operand::BuildDisp8(int32_t disp)
{
    disp_ = disp;
    hasDisp8_ = true;
}

void Operand::BuildDisp32(int32_t disp)
{
    disp_ = disp;
    hasDisp32_ = true;
}

void AssemblerX64::EmitOperand(int32_t reg, Operand rm)
{
    // moderm
    EmitU8(rm.moderm_ | (reg << LOW_BITS_SIZE));
    if (rm.hasSIB_) {
        EmitU8(rm.sib_);
    }

    if (rm.hasDisp8_) {
        EmitI8(static_cast<int8_t>(rm.disp_));
    } else if (rm.hasDisp32_) {
        EmitI32(rm.disp_);
    }
}
}  // panda::ecmascript::x64
