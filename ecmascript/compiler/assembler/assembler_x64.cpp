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
    if (InRange8(x.Value())) {
        // 6A: Push imm8
        EmitU8(0x6A);
        EmitI8(static_cast<int8_t>(x.Value()));
    } else {
        // 68: Push imm32
        EmitU8(0x68);
        EmitI32(x.Value());
    }
}

void AssemblerX64::Push(Register x)
{
    Pushq(x);
}

void AssemblerX64::Popq(Register x)
{
    EmitRexPrefix(x);
    // 0x58: Pop r16
    EmitU8(0x58 | LowBits(x));
}

void AssemblerX64::Pop(Register x)
{
    Popq(x);
}


void AssemblerX64::Addq(Immediate src, Register dst)
{
    EmitRexPrefixW(dst);
    if (InRange8(src.Value())) {
        // 83: Add r/m16, imm8
        EmitU8(0x83);
        // 0: 83 /0 ib
        EmitModrm(0, dst);
        EmitI8(static_cast<int8_t>(src.Value()));
    } else if (dst == rax) {
        // 0x5: Add rax, imm32
        EmitU8(0x5);
        EmitI32(src.Value());
    } else {
        // 81: Add r/m32, imm32
        EmitU8(0x81);
        // 0: 81 /0 id
        EmitModrm(0, dst);
        EmitI32(src.Value());
    }
}

void AssemblerX64::Addq(Register src, Register dst)
{
    EmitRexPrefix(dst, src);
    // 03 : add r64, r/m64
    EmitU8(0x03);
    EmitModrm(dst, src);
}

void AssemblerX64::Addl(Immediate src, Register dst)
{
    EmitRexPrefix(dst);
    if (InRange8(src.Value())) {
        // 83: Add r/m16, imm8
        EmitU8(0x83);
        // 0: 83 /0 ib
        EmitModrm(0, dst);
        EmitI8(static_cast<int8_t>(src.Value()));
    } else if (dst == rax) {
        // 0x5: Add rax, imm32
        EmitU8(0x5);
        EmitI32(src.Value());
    } else {
        // 81: Add r/m32, imm32
        EmitU8(0x81);
        // 0: 81 /0 id
        EmitModrm(0, dst);
        EmitI32(src.Value());
    }
}

void AssemblerX64::Subq(Immediate src, Register dst)
{
    EmitRexPrefixW(dst);
    if (InRange8(src.Value())) {
        // 83: Sub r/m16, imm8
        EmitU8(0x83);
        // 5: 83 /5 ib
        EmitModrm(5, dst);
        EmitI8(static_cast<int8_t>(src.Value()));
    } else if (dst == rax) {
        // 0x2D: Sub rax, imm32
        EmitU8(0x2D);
        EmitI32(src.Value());
    } else {
        // 81: sub r/m32, imm32
        EmitU8(0x81);
        // 5: 81 /5 id
        EmitModrm(5, dst);
        EmitI32(src.Value());
    }
}

void AssemblerX64::Subq(Register src, Register dst)
{
    EmitRexPrefix(src, dst);
    // 29: sub r/m64, r64
    EmitU8(0x29);
    EmitModrm(src, dst);
}

void AssemblerX64::Subl(Immediate src, Register dst)
{
    EmitRexPrefix(dst);
    if (InRange8(src.Value())) {
        // 83: Sub r/m16, imm8
        EmitU8(0x83);
        // 5: 83 /5 ib
        EmitModrm(5, dst);
        EmitI8(static_cast<int8_t>(src.Value()));
    } else if (dst == rax) {
        // 0x2D: Sub eax, imm32
        EmitU8(0x2D);
        EmitI32(src.Value());
    } else {
        // 81: sub r/m32, imm32
        EmitU8(0x81);
        // 5: 81 /5 id
        EmitModrm(5, dst);
        EmitI32(src.Value());
    }
}

void AssemblerX64::Cmpq(Immediate src, Register dst)
{
    EmitRexPrefixW(dst);
    if (InRange8(src.Value())) {
        // 83: cmp r/m64, imm8
        EmitU8(0x83);
        // 7: 83 /7 ib
        EmitModrm(7, dst);
        EmitI8(static_cast<int8_t>(src.Value()));
    } else if (dst == rax) {
        // 0x3D: cmp rax, imm32
        EmitU8(0x3D);
        EmitI32(src.Value());
    } else {
        // 81: cmp r/m32, imm32
        EmitU8(0x81);
        // 7: 81 /7 id
        EmitModrm(7, dst);
        EmitI32(src.Value());
    }
}

void AssemblerX64::Cmpb(Immediate src, Register dst)
{
    EmitRexPrefix(dst);
    if (InRange8(src.Value())) {
        // 80: cmp r/m8, imm8
        EmitU8(0x80);
        // 7: /7 ib
        EmitModrm(7, dst);
        EmitI8(static_cast<int8_t>(src.Value()));
    } else if (dst == rax) {
        // 0x3C: cmp al, imm8
        EmitU8(0x3C);
        EmitI8(src.Value());
    } else {
        UNREACHABLE();
    }
}

void AssemblerX64::Cmpq(Register src, Register dst)
{
    EmitRexPrefix(src, dst);
    // 39: Cmp r/m64, r64
    EmitU8(0x39);
    EmitModrm(src, dst);
}

void AssemblerX64::Cmpl(Immediate src, Register dst)
{
    EmitRexPrefix(dst);
    if (InRange8(src.Value())) {
        // 83: cmp r/m32, imm8
        EmitU8(0x83);
        // 7: 83 /7 ib
        EmitModrm(7, dst);
        EmitI8(static_cast<int8_t>(src.Value()));
    } else if (dst == rax) {
        // 0x3D: cmp rax, imm32
        EmitU8(0x3D);
        EmitI32(src.Value());
    } else {
        // 81: cmp r/m32, imm32
        EmitU8(0x81);
        // 7: 81 /7 id
        EmitModrm(7, dst);
        EmitI32(src.Value());
    }
}

void AssemblerX64::Cmp(Immediate src, Register dst)
{
    Cmpq(src, dst);
}

void AssemblerX64::Movq(Register src, Register dst)
{
    EmitRexPrefix(src, dst);
    // 0x89: Move r16 to r/m16
    EmitU8(0x89);
    EmitModrm(src, dst);
}

void AssemblerX64::Mov(Register src, Register dst)
{
    EmitRexPrefixl(dst, src);
    // 0x89: Move r16 to r/m16
    EmitU8(0x8B);
    EmitModrm(dst, src);
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
    EmitI32(src.Value());
}

void AssemblerX64::Movq(Immediate src, Register dst)
{
    EmitRexPrefix(dst);
    // B8 : mov r32, imm32
    EmitU8(0xB8 | LowBits(dst));
    EmitI32(src.Value());
}

void AssemblerX64::Mov(Operand src, Register dst)
{
    Movq(src, dst);
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

void AssemblerX64::EmitJa(int32_t offset)
{
    offset--;
    if (InRange8(offset - sizeof(int8_t))) {
        // 77 : Ja rel8
        EmitU8(0x77);
        EmitI8(offset - sizeof(int8_t));
    } else {
        // 0F 87 : ja rel32
        EmitU8(0x0F);
        EmitU8(0x87);
        EmitI32(offset - sizeof(int32_t));
    }
}

void AssemblerX64::EmitJb(int32_t offset)
{
    offset--;
    if (InRange8(offset - sizeof(int8_t))) {
        // 72 : Jb rel8
        EmitU8(0x72);
        EmitI8(offset - sizeof(int8_t));
    } else {
        // 0F 82 : Jb rel32
        EmitU8(0x0F);
        EmitU8(0x82);
        EmitI32(offset - sizeof(int32_t));
    }
}

void AssemblerX64::EmitJz(int32_t offset)
{
    offset--;
    if (InRange8(offset - sizeof(int8_t))) {
        // 74 : Jz rel8
        EmitU8(0x74);
        EmitI8(offset - sizeof(int8_t));
    } else {
        // 0F 84 : Jz rel32
        EmitU8(0x0F);
        EmitU8(0x84);
        EmitI32(offset - sizeof(int32_t));
    }
}

void AssemblerX64::EmitJne(int32_t offset)
{
    offset--;
    if (InRange8(offset - sizeof(int8_t))) {
        // 75 : Jne rel8;
        EmitU8(0x75);
        EmitI8(offset - sizeof(int8_t));
    } else {
        // 0F 85 : Jne rel32
        EmitU8(0x0F);
        EmitU8(0x85);
        EmitI32(offset - sizeof(int32_t));
    }
}

void AssemblerX64::EmitJbe(int32_t offset)
{
    offset--;
    if (InRange8(offset - sizeof(int8_t))) {
        // 76 : Jbe rel8;
        EmitU8(0x76);
        EmitI8(offset - sizeof(int8_t));
    } else {
        // 0F 86 : Jne rel32
        EmitU8(0x0F);
        EmitU8(0x86);
        EmitI32(offset - sizeof(int32_t));
    }
}

void AssemblerX64::EmitJnz(int32_t offset)
{
    offset--;
    if (InRange8(offset)) {
        // 75 : Jnz rel8
        EmitU8(0x75);
        EmitI8(offset - sizeof(int8_t));
    } else {
        // 0F 85: Jnz rel32
        EmitU8(0x0F);
        EmitU8(0x85);
        EmitI32(offset - sizeof(int32_t));
    }
}

void AssemblerX64::EmitJle(int32_t offset)
{
    offset--;
    if (InRange8(offset)) {
        // 7E : Jle rel8
        EmitU8(0x7E);
        EmitI8(offset - sizeof(int8_t));
    } else {
        // 0F 8E: Jle rel32
        EmitU8(0x0F);
        EmitU8(0x8E);
        EmitI32(offset - sizeof(int32_t));
    }
}

void AssemblerX64::EmitJae(int32_t offset)
{
    offset--;
    if (InRange8(offset)) {
        // 73 : Jae rel8
        EmitU8(0x73);
        EmitI8(offset - sizeof(int8_t));
    } else {
        // 0F 83: Jae rel32
        EmitU8(0x0F);
        EmitU8(0x83);
        EmitI32(offset - sizeof(int32_t));
    }
}

void AssemblerX64::EmitJg(int32_t offset)
{
    offset--;
    if (InRange8(offset)) {
        // 7F : Jg rel8
        EmitU8(0x7F);
        EmitI8(offset - sizeof(int8_t));
    } else {
        // 0F 8F: Jg rel32
        EmitU8(0x0F);
        EmitU8(0x8F);
        EmitI32(offset - sizeof(int32_t));
    }
}

void AssemblerX64::EmitJe(int32_t offset)
{
    offset--;
    if (InRange8(offset)) {
        // 74 : Je rel8
        EmitU8(0x74);
        EmitI8(offset - sizeof(int8_t));
    } else {
        // 0F 84: Je rel32
        EmitU8(0x0F);
        EmitU8(0x84);
        EmitI32(offset - sizeof(int32_t));
    }
}

void AssemblerX64::EmitCall(int32_t offset)
{
    offset--;
    // E8: call rel32
    EmitU8(0xE8);
    EmitI32(offset - sizeof(int32_t));
}

void AssemblerX64::EmitJnb(int32_t offset)
{
    offset--;
    if (InRange8(offset)) {
        // 73 : Jnb rel8
        EmitU8(0x73);
        EmitI8(offset - sizeof(int8_t));
    } else {
        // 0F 83: Jnb rel32
        EmitU8(0x0F);
        EmitU8(0x83);
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

void AssemblerX64::Callq(Label *target)
{
    if (target->IsBound()) {
        int32_t offset = static_cast<int32_t>(target->GetPos() - GetCurrentPosition());
        EmitCall(offset);
        return;
    }

    auto pos = GetCurrentPosition();
    int32_t emitPos = 0;
    if (target->IsLinked()) {
        emitPos = static_cast<int32_t>(target->GetLinkedPos());
    }
    // +1: skip opcode
    target->LinkTo(pos + 1);
    // E8: call rel32
    EmitU8(0xE8);
    EmitI32(emitPos);
}

void AssemblerX64::Ret()
{
    // C3: RET Near return to calling procedure
    EmitU8(0xC3);
}

void AssemblerX64::Jmp(Label *target, Distance distance)
{
    if (target->IsBound()) {
        int32_t offset = static_cast<int32_t>(target->GetPos() - GetCurrentPosition());
        EmitJmp(offset);
        return;
    }

    auto pos = GetCurrentPosition();
    int32_t emitPos = 0;
    if (distance == Distance::NEAR) {
        if (target->IsLinkedNear()) {
            emitPos = static_cast<int32_t>(target->GetLinkedNearPos() - pos);
        }
        // +1: skip opcode
        target->LinkNearPos(pos + 1);
        ASSERT(InRange8(emitPos));
        // EB: Jmp rel8
        EmitU8(0xEB);
        EmitI8(static_cast<int8_t>(emitPos));
    } else {
        if (target->IsLinked()) {
            emitPos = static_cast<int32_t>(target->GetLinkedPos());
        }
        // +1: skip opcode
        target->LinkTo(pos + 1);
        // E9: Jmp rel32
        EmitU8(0xE9);
        EmitI32(emitPos);
    }
}

void AssemblerX64::Jmp(Register dst)
{
    EmitRexPrefix(dst);
    // opcode FF/4 : jmp r/m64
    EmitU8(0xFF);
    EmitModrm(4, dst);
}

void AssemblerX64::Jmp(Immediate offset)
{
    if (InRange8(offset.Value())) {
        // opcode EB : jmp rel8
        EmitU8(0xEB);
        EmitI8(static_cast<int8_t>(offset.Value()));
    } else {
        // opcode E9 : jmp rel32
        EmitU8(0xE9);
        EmitI32(offset.Value());
    }
}

void AssemblerX64::Ja(Label *target, Distance distance)
{
    if (target->IsBound()) {
        int32_t offset = static_cast<int32_t>(target->GetPos() - GetCurrentPosition());
        EmitJa(offset);
        return;
    }
    auto pos = GetCurrentPosition();
    int32_t emitPos = 0;
    if (distance == Distance::NEAR) {
        if (target->IsLinkedNear()) {
            emitPos = static_cast<int32_t>(target->GetLinkedNearPos() - pos);
        }
        // +1: skip opcode
        target->LinkNearPos(pos + 1);
        ASSERT(InRange8(emitPos));
        // 77: ja rel8
        EmitU8(0x77);
        EmitI8(static_cast<int8_t>(emitPos));
    } else {
        if (target->IsLinked()) {
            emitPos = static_cast<int32_t>(target->GetLinkedPos());
        }
        // 2: skip opcode
        target->LinkTo(pos + 2);
        // 0F 87: ja rel32
        EmitU8(0X0F);
        EmitU8(0x87);
        EmitI32(emitPos);
    }
}

void AssemblerX64::Jb(Label *target, Distance distance)
{
    if (target->IsBound()) {
        int32_t offset = static_cast<int32_t>(target->GetPos() - GetCurrentPosition());
        EmitJb(offset);
        return;
    }
    auto pos = GetCurrentPosition();
    int32_t emitPos = 0;
    if (distance == Distance::NEAR) {
        if (target->IsLinkedNear()) {
            emitPos = static_cast<int32_t>(target->GetLinkedNearPos() - pos);
        }
        // +1: skip opcode
        target->LinkNearPos(pos + 1);
        ASSERT(InRange8(emitPos));
        // 72 : Jb rel8
        EmitU8(0x72);
        EmitI8(static_cast<int8_t>(emitPos));
    } else {
        if (target->IsLinked()) {
            emitPos = static_cast<int32_t>(target->GetLinkedPos());
        }
        // 2: skip opcode
        target->LinkTo(pos + 2);
        // 0F 82: jb rel32
        EmitU8(0X0F);
        EmitU8(0x82);
        EmitI32(emitPos);
    }
}
void AssemblerX64::Jz(Label *target, Distance distance)
{
    if (target->IsBound()) {
        int32_t offset = static_cast<int32_t>(target->GetPos() - GetCurrentPosition());
        EmitJz(offset);
        return;
    }
    auto pos = GetCurrentPosition();
    int32_t emitPos = 0;
    if (distance == Distance::NEAR) {
        if (target->IsLinkedNear()) {
            emitPos = static_cast<int32_t>(target->GetLinkedNearPos() - pos);
        }
        // +1: skip opcode
        target->LinkNearPos(pos + 1);
        ASSERT(InRange8(emitPos));
        // 74 : Jz rel8
        EmitU8(0x74);
        EmitI8(static_cast<int8_t>(emitPos));
    } else {
        if (target->IsLinked()) {
            emitPos = static_cast<int32_t>(target->GetLinkedPos());
        }
        // 2: skip opcode
        target->LinkTo(pos + 2);
        // 0F 84: Jz rel32
        EmitU8(0X0F);
        EmitU8(0x84);
        EmitI32(emitPos);
    }
}

void AssemblerX64::Je(Label *target, Distance distance)
{
    if (target->IsBound()) {
        int32_t offset = static_cast<int32_t>(target->GetPos() - GetCurrentPosition());
        EmitJe(offset);
        return;
    }
    auto pos = GetCurrentPosition();
    int32_t emitPos = 0;
    if (distance == Distance::NEAR) {
        if (target->IsLinkedNear()) {
            emitPos = static_cast<int32_t>(target->GetLinkedNearPos() - pos);
        }
        // +1: skip opcode
        target->LinkNearPos(pos + 1);
        ASSERT(InRange8(emitPos));
        // 74 : Je rel8
        EmitU8(0x74);
        EmitI8(static_cast<int8_t>(emitPos));
    } else {
        if (target->IsLinked()) {
            emitPos = static_cast<int32_t>(target->GetLinkedPos());
        }
        // 2: skip opcode
        target->LinkTo(pos + 2);
        // 0F 84: Je rel32
        EmitU8(0X0F);
        EmitU8(0x84);
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
            uint32_t preLinkPos = GetU32(linkPos);
            int32_t disp = static_cast<int32_t>(pos - linkPos - sizeof(int32_t));
            PutI32(linkPos, disp);
            linkPos = preLinkPos;
        }
    }

    if (target->IsLinkedNear()) {
        uint32_t linkPos = target->GetLinkedNearPos();
        while (linkPos != 0) {
            int8_t offsetToNext = GetI8(static_cast<size_t>(linkPos));
            int32_t disp = static_cast<int32_t>(pos - linkPos - sizeof(int8_t));
            ASSERT(InRange8(disp));
            PutI8(linkPos, static_cast<int8_t>(disp));
            if (offsetToNext == 0) {
                break;
            }
            linkPos += static_cast<uint32_t>(offsetToNext);
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

// [index * scale + disp]
Operand::Operand(Register index, Scale scale, int32_t disp)
{
    ASSERT(index != rsp);
    BuildModerm(0, rsp);
    BuildSIB(scale, index, rbp);
    BuildDisp32(disp);
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
    EmitU8(rm.moderm_ | (static_cast<uint32_t>(reg) << LOW_BITS_SIZE));
    if (rm.hasSIB_) {
        EmitU8(rm.sib_);
    }

    if (rm.hasDisp8_) {
        EmitI8(static_cast<int8_t>(rm.disp_));
    } else if (rm.hasDisp32_) {
        EmitI32(rm.disp_);
    }
}

void AssemblerX64::Movl(Register src, Register dst)
{
    EmitRexPrefixl(src, dst);
    // 0x89: Move r16 to r/m16
    EmitU8(0x89);
    EmitModrm(src, dst);
}

void AssemblerX64::Movl(Operand src, Register dst)
{
    EmitRexPrefixl(dst, src);
    // 0x8B: Move r/m64 to r64
    EmitU8(0x8B);
    EmitOperand(dst, src);
}

void AssemblerX64::Testq(Immediate src, Register dst) {
    if (InRange8(src.Value())) {
        Testb(src, dst);
    } else if (dst == rax) {
        // A9: Test rax, imm32
        EmitU8(0xA9);
        EmitI32(src.Value());
    } else {
        EmitRexPrefixW(dst);
        // F7: Test r/m64, imm32
        EmitU8(0xF7);
        // 0: F7 0 id
        EmitModrm(0, dst);
        EmitI32(src.Value());
    }
}

void AssemblerX64::Testb(Immediate src, Register dst)
{
    ASSERT(InRange8(src.Value()));
    if (dst == rax) {
        // A8: Test al, imm8
        EmitU8(0xA8);
    } else {
        // AH BH CG DH can not be encoded to access if REX prefix used.
        if (dst >= rsp) {
            EmitRexPrefixL(dst);
        }
        // F6: Test r/m8, imm8
        // REX F6: Test r/m8*, imm8
        EmitU8(0xF6);
        // 0: F6 /0 ib
        EmitModrm(0, dst);
    }
    EmitI8(static_cast<int8_t>(src.Value()));
}

void AssemblerX64::Jne(Label *target, Distance distance)
{
    if (target->IsBound()) {
        int32_t offset = static_cast<int32_t>(target->GetPos() - GetCurrentPosition());
        EmitJne(offset);
        return;
    }
    auto pos = GetCurrentPosition();
    int32_t emitPos = 0;
    if (distance == Distance::NEAR) {
        if (target->IsLinkedNear()) {
            emitPos = static_cast<int32_t>(target->GetLinkedNearPos() - pos);
        }
        // +1: skip opcode
        target->LinkNearPos(pos + 1);
        ASSERT(InRange8(emitPos));
        // 75 : Jne rel8;
        EmitU8(0x75);
        EmitI8(static_cast<int8_t>(emitPos));
    } else {
        if (target->IsLinked()) {
            emitPos = static_cast<int32_t>(target->GetLinkedPos());
        }
        // 2: skip opcode
        target->LinkTo(pos + 2);
        // 0F 85 : Jne rel32
        EmitU8(0x0F);
        EmitU8(0x85);
        EmitI32(emitPos);
    }
}

void AssemblerX64::Cmpl(Register src, Register dst)
{
    EmitRexPrefixl(src, dst);
    // 39: Cmp r16 to r/m16
    EmitU8(0x39);
    EmitModrm(src, dst);
}

void AssemblerX64::Jbe(Label *target, Distance distance)
{
    if (target->IsBound()) {
        int32_t offset = static_cast<int32_t>(target->GetPos() - GetCurrentPosition());
        EmitJbe(offset);
        return;
    }
    auto pos = GetCurrentPosition();
    int32_t emitPos = 0;
    if (distance == Distance::NEAR) {
        if (target->IsLinkedNear()) {
            emitPos = static_cast<int32_t>(target->GetLinkedNearPos() - pos);
        }
        // +1: skip opcode
        target->LinkNearPos(pos + 1);
        ASSERT(InRange8(emitPos));
        // 76 : Jbe rel8;
        EmitU8(0x76);
        EmitI8(static_cast<int8_t>(emitPos));
    } else {
        if (target->IsLinked()) {
            emitPos = static_cast<int32_t>(target->GetLinkedPos());
        }
        // 2: skip opcode
        target->LinkTo(pos + 2);
        // 0F 86 : Jbe rel32
        EmitU8(0x0F);
        EmitU8(0x86);
        EmitI32(emitPos);
    }
}

void AssemblerX64::CMovbe(Register src, Register dst)
{
    EmitRexPrefixl(dst, src);
    // 0f 46: CMovbe r32, r/m32
    EmitU8(0x0F);
    EmitU8(0x46);
    EmitModrm(dst, src);
}

void AssemblerX64::Leaq(Operand src, Register dst)
{
    EmitRexPrefix(dst, src);
    // 8D : lea r64, m
    EmitU8(0x8D);
    EmitOperand(dst, src);
}

void AssemblerX64::Leal(Operand src, Register dst)
{
    EmitRexPrefixl(dst, src);
    // 8D : lea r64, m
    EmitU8(0x8D);
    EmitOperand(dst, src);
}

void AssemblerX64::Shrq(Immediate src, Register dst)
{
    EmitRexPrefixW(dst);
    // C1 : Shr r/m64, imm8;
    EmitU8(0xc1);
    // 5: C1 /5 id
    EmitModrm(5, dst);
    EmitI8(static_cast<int8_t>(src.Value()));
}

void AssemblerX64::Shr(Immediate src, Register dst)
{
    Shrq(src, dst);
}

void AssemblerX64::Andq(Immediate src, Register dst)
{
    EmitRexPrefixW(dst);
    if (InRange8(src.Value())) {
        // 83: and r/m64, imm8
        EmitU8(0x83);
        // 4: 83 /4 ib
        EmitModrm(4, dst);
        EmitI8(static_cast<int8_t>(src.Value()));
    } else if (dst == rax) {
        // 0x25: and rax, imm32
        EmitU8(0x25);
        EmitI32(src.Value());
    } else {
        // 81: and r/m64, imm32
        EmitU8(0x81);
        // 4: 81 /4 id
        EmitModrm(4, dst);
        EmitI32(src.Value());
    }
}

void AssemblerX64::Andl(Immediate src, Register dst)
{
    EmitRexPrefix(dst);
    if (InRange8(src.Value())) {
        // 83: and r/m64, imm8
        EmitU8(0x83);
        // 4: 83 /4 ib
        EmitModrm(4, dst);
        EmitI8(static_cast<int8_t>(src.Value()));
    } else if (dst == rax) {
        // 0x25: and rax, imm32
        EmitU8(0x25);
        EmitI32(src.Value());
    } else {
        // 81: and r/m64, imm32
        EmitU8(0x81);
        // 4: 81 /4 id
        EmitModrm(4, dst);
        EmitI32(src.Value());
    }
}

void AssemblerX64::And(Register src, Register dst)
{
    EmitRexPrefix(src, dst);
    // 21 : And r/m64, r64
    EmitU8(0x21);
    EmitModrm(src, dst);
}

void AssemblerX64::Jnz(Label *target, Distance distance)
{
    if (target->IsBound()) {
        int32_t offset = static_cast<int32_t>(target->GetPos() - GetCurrentPosition());
        EmitJnz(offset);
        return;
    }
    auto pos = GetCurrentPosition();
    int32_t emitPos = 0;
    if (distance == Distance::NEAR) {
        if (target->IsLinkedNear()) {
            emitPos = static_cast<int32_t>(target->GetLinkedNearPos() - pos);
        }
        // +1: skip opcode
        target->LinkNearPos(pos + 1);
        ASSERT(InRange8(emitPos));
        // 75 : Jnz rel8;
        EmitU8(0x75);
        EmitI8(static_cast<int8_t>(emitPos));
    } else {
        if (target->IsLinked()) {
            emitPos = static_cast<int32_t>(target->GetLinkedPos());
        }
        // 2: skip opcode
        target->LinkTo(pos + 2);
        // 0F 85 : Jnz rel32
        EmitU8(0x0F);
        EmitU8(0x85);
        EmitI32(emitPos);
    }
}

void AssemblerX64::Jle(Label *target, Distance distance)
{
    if (target->IsBound()) {
        int32_t offset = static_cast<int32_t>(target->GetPos() - GetCurrentPosition());
        EmitJle(offset);
        return;
    }
    auto pos = GetCurrentPosition();
    int32_t emitPos = 0;
    if (distance == Distance::NEAR) {
        if (target->IsLinkedNear()) {
            emitPos = static_cast<int32_t>(target->GetLinkedNearPos() - pos);
        }
        // +1: skip opcode
        target->LinkNearPos(pos + 1);
        ASSERT(InRange8(emitPos));
        // 7E : Jle rel8;
        EmitU8(0x7E);
        EmitI8(static_cast<int8_t>(emitPos));
    } else {
        if (target->IsLinked()) {
            emitPos = static_cast<int32_t>(target->GetLinkedPos());
        }
        // 2: skip opcode
        target->LinkTo(pos + 2);
        // 0F 8E: Jle rel32
        EmitU8(0x0F);
        EmitU8(0x8E);
        EmitI32(emitPos);
    }
}

void AssemblerX64::Jae(Label *target, Distance distance)
{
    if (target->IsBound()) {
        int32_t offset = static_cast<int32_t>(target->GetPos() - GetCurrentPosition());
        EmitJae(offset);
        return;
    }
    auto pos = GetCurrentPosition();
    int32_t emitPos = 0;
    if (distance == Distance::NEAR) {
        if (target->IsLinkedNear()) {
            emitPos = static_cast<int32_t>(target->GetLinkedNearPos() - pos);
        }
        // +1: skip opcode
        target->LinkNearPos(pos + 1);
        ASSERT(InRange8(emitPos));
        // 73 : Jae rel8
        EmitU8(0x73);
        EmitI8(static_cast<int8_t>(emitPos));
    } else {
        if (target->IsLinked()) {
            emitPos = static_cast<int32_t>(target->GetLinkedPos());
        }
        // 2: skip opcode
        target->LinkTo(pos + 2);
        // 0F 83: Jae rel32
        EmitU8(0x0F);
        EmitU8(0x83);
        EmitI32(emitPos);
    }
}

void AssemblerX64::Jg(Label *target, Distance distance)
{
    if (target->IsBound()) {
        int32_t offset = static_cast<int32_t>(target->GetPos() - GetCurrentPosition());
        EmitJg(offset);
        return;
    }
    auto pos = GetCurrentPosition();
    int32_t emitPos = 0;
    if (distance == Distance::NEAR) {
        if (target->IsLinkedNear()) {
            emitPos = static_cast<int32_t>(target->GetLinkedNearPos() - pos);
        }
        // +1: skip opcode
        target->LinkNearPos(pos + 1);
        ASSERT(InRange8(emitPos));
        // 7F : Jg rel8
        EmitU8(0x7F);
        EmitI8(static_cast<int8_t>(emitPos));
    } else {
        if (target->IsLinked()) {
            emitPos = static_cast<int32_t>(target->GetLinkedPos());
        }
        // 2: skip opcode
        target->LinkTo(pos + 2);
        // 0F 8F: Jae rel32
        EmitU8(0x0F);
        EmitU8(0x8F);
        EmitI32(emitPos);
    }
}

void AssemblerX64::Movzbq(Operand src, Register dst)
{
    EmitRexPrefix(dst, src);
    // 0F B6 : Movzx r64, r/m16
    EmitU8(0x0F);
    EmitU8(0xB6);
    // 0F B6 /r: Movzx r64, r/m16
    EmitOperand(dst, src);
}

void AssemblerX64::Btq(Immediate src, Register dst)
{
    EmitRexPrefixW(dst);
    // 0F BA: bt r/m32, imm8;
    EmitU8(0x0F);
    EmitU8(0xBA);
    // /4: 0F BA bt r/m32, imm8
    EmitModrm(4, dst);
    EmitI8(static_cast<int8_t>(src.Value()));
}
void AssemblerX64::Btl(Immediate src, Register dst)
{
    EmitRexPrefix(dst);
    // 0F BA: bt r/m32, imm8;
    EmitU8(0x0F);
    EmitU8(0xBA);
    // /4: 0F BA bt r/m32, imm8
    EmitModrm(4, dst);
    EmitI8(static_cast<int8_t>(src.Value()));
}

void AssemblerX64::Movabs(uint64_t src, Register dst)
{
    // REX.W + B8 + rd io : Mov r64, imm64
    EmitRexPrefixW(dst);
    EmitU8(0xB8 | LowBits(dst));
    EmitU64(src);
}

void AssemblerX64::Shll(Immediate src, Register dst)
{
    EmitRexPrefix(dst);
    // C1 : shl r/m32, imm8
    EmitU8(0xC1);
    // C1 /4
    EmitModrm(4, dst);
    EmitI8(static_cast<int8_t>(src.Value()));
}

void AssemblerX64::Int3()
{
    // CC :: INT3
    EmitU8(0xCC);
}

void AssemblerX64::Movzwq(Operand src, Register dst)
{
    EmitRexPrefixW(dst);
    EmitU8(0x0F);
    EmitU8(0xB7);
    EmitOperand(dst, src);
}

void AssemblerX64::Jnb(Label *target, Distance distance)
{
    if (target->IsBound()) {
        int32_t offset = static_cast<int32_t>(target->GetPos() - GetCurrentPosition());
        EmitJnb(offset);
        return;
    }
    auto pos = GetCurrentPosition();
    int32_t emitPos = 0;
    if (distance == Distance::NEAR) {
        if (target->IsLinkedNear()) {
            emitPos = static_cast<int32_t>(target->GetLinkedNearPos() - pos);
        }
        // +1: skip opcode
        target->LinkNearPos(pos + 1);
        ASSERT(InRange8(emitPos));
        // 73 : Jnb rel8
        EmitU8(0x73);
        EmitI8(static_cast<int8_t>(emitPos));
    } else {
        if (target->IsLinked()) {
            emitPos = static_cast<int32_t>(target->GetLinkedPos());
        }
        // 2: skip opcode
        target->LinkTo(pos + 2);
        // 0F 83: Jnb rel32
        EmitU8(0x0F);
        EmitU8(0x83);
        EmitI32(emitPos);
    }
}
}  // panda::ecmascript::x64
