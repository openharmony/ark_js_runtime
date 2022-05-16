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
#include <ostream>
#include <sstream>
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/compiler/assembler/aarch64/assembler_aarch64.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/dyn_chunk.h"
#include "llvm-c/Analysis.h"
#include "llvm-c/Core.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Target.h"
#include "llvm-c/Disassembler.h"

namespace panda::test {
using namespace panda::ecmascript;
using namespace panda::ecmascript::aarch64;
class AssemblerAarch64Test : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        InitializeLLVM("aarch64-unknown-linux-gnu");
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
        chunk_ = thread->GetEcmaVM()->GetChunk();
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    static const char *SymbolLookupCallback([[maybe_unused]] void *disInfo, [[maybe_unused]] uint64_t referenceValue,
                                            uint64_t *referenceType, [[maybe_unused]]uint64_t referencePC,
                                            [[maybe_unused]] const char **referenceName)
    {
        *referenceType = LLVMDisassembler_ReferenceType_InOut_None;
        return nullptr;
    }

    void InitializeLLVM(std::string triple)
    {
        if (triple.compare("x86_64-unknown-linux-gnu") == 0) {
            LLVMInitializeX86TargetInfo();
            LLVMInitializeX86TargetMC();
            LLVMInitializeX86Disassembler();
            /* this method must be called, ohterwise "Target does not support MC emission" */
            LLVMInitializeX86AsmPrinter();
            LLVMInitializeX86AsmParser();
            LLVMInitializeX86Target();
        } else if (triple.compare("aarch64-unknown-linux-gnu") == 0) {
            LLVMInitializeAArch64TargetInfo();
            LLVMInitializeAArch64TargetMC();
            LLVMInitializeAArch64Disassembler();
            LLVMInitializeAArch64AsmPrinter();
            LLVMInitializeAArch64AsmParser();
            LLVMInitializeAArch64Target();
        } else if (triple.compare("arm-unknown-linux-gnu") == 0) {
            LLVMInitializeARMTargetInfo();
            LLVMInitializeARMTargetMC();
            LLVMInitializeARMDisassembler();
            LLVMInitializeARMAsmPrinter();
            LLVMInitializeARMAsmParser();
            LLVMInitializeARMTarget();
        } else {
            UNREACHABLE();
        }
    }

    void DisassembleChunk(const char *triple, Assembler *assemlber, std::ostream &os)
    {
        LLVMDisasmContextRef dcr = LLVMCreateDisasm(triple, nullptr, 0, nullptr, SymbolLookupCallback);
        uint8_t *byteSp = assemlber->GetBegin();
        size_t numBytes = assemlber->GetCurrentPosition();
        unsigned pc = 0;
        const char outStringSize = 100;
        char outString[outStringSize];
        while (numBytes > 0) {
            size_t InstSize = LLVMDisasmInstruction(dcr, byteSp, numBytes, pc, outString, outStringSize);
            if (InstSize == 0) {
                // 8 : 8 means width of the pc offset and instruction code
                os << std::setw(8) << std::setfill('0') << std::hex << pc << ":" << std::setw(8)
                   << *reinterpret_cast<uint32_t *>(byteSp) << "maybe constant" << std::endl;
                pc += 4; // 4 pc length
                byteSp += 4; // 4 sp offset
                numBytes -= 4; // 4 num bytes
            }
            // 8 : 8 means width of the pc offset and instruction code
            os << std::setw(8) << std::setfill('0') << std::hex << pc << ":" << std::setw(8)
               << *reinterpret_cast<uint32_t *>(byteSp) << " " << outString << std::endl;
            pc += InstSize;
            byteSp += InstSize;
            numBytes -= InstSize;
        }
        LLVMDisasmDispose(dcr);
    }
    EcmaVM *instance {nullptr};
    JSThread *thread {nullptr};
    EcmaHandleScope *scope {nullptr};
    Chunk *chunk_ {nullptr};
};

#define __ masm.
HWTEST_F_L0(AssemblerAarch64Test, Mov)
{
    std::string expectResult("00000000:d28acf01 \tmov\tx1, #22136\n"
                             "00000004:f2a24681 \tmovk\tx1, #4660, lsl #16\n"
                             "00000008:f2ffffe1 \tmovk\tx1, #65535, lsl #48\n"
                             "0000000c:d2801de2 \tmov\tx2, #239\n"
                             "00000010:f2b579a2 \tmovk\tx2, #43981, lsl #16\n"
                             "00000014:f2cacf02 \tmovk\tx2, #22136, lsl #32\n"
                             "00000018:f2e24682 \tmovk\tx2, #4660, lsl #48\n"
                             "0000001c:b2683be3 \tmov\tx3, #549739036672\n"
                             "00000020:f2824683 \tmovk\tx3, #4660\n"
                             "00000024:32083fe4 \tmov\tw4, #-16776961\n");
    AssemblerAarch64 masm(chunk_);
    __ Mov(Register(X1),  Immediate(0xffff000012345678));
    __ Mov(Register(X2),  Immediate(0x12345678abcd00ef));
    __ Mov(Register(X3),  Immediate(0x7fff001234));
    __ Mov(Register(X4).W(),  Immediate(0xff0000ff));
    std::ostringstream oss;
    DisassembleChunk("aarch64-unknown-linux-gnu", &masm, oss);
    ASSERT_EQ(oss.str(), expectResult);
}

HWTEST_F_L0(AssemblerAarch64Test, MovReg)
{
    std::string expectResult("00000000:aa0203e1 \tmov\tx1, x2\n"
                             "00000004:910003e2 \tmov\tx2, sp\n"
                             "00000008:2a0203e1 \tmov\tw1, w2\n");
    AssemblerAarch64 masm(chunk_);
    __ Mov(Register(X1),  Register(X2));
    __ Mov(Register(X2),  Register(SP));
    __ Mov(Register(X1, true),  Register(X2, true));
    std::ostringstream oss;
    DisassembleChunk("aarch64-unknown-linux-gnu", &masm, oss);
    ASSERT_EQ(oss.str(), expectResult);
}

HWTEST_F_L0(AssemblerAarch64Test, LdpStp)
{
    std::string expectResult("00000000:a8808be1 \tstp\tx1, x2, [sp], #8\n"
                             "00000004:a9c08be1 \tldp\tx1, x2, [sp, #8]!\n"
                             "00000008:a94093e3 \tldp\tx3, x4, [sp, #8]\n"
                             "0000000c:294113e3 \tldp\tw3, w4, [sp, #8]\n");

    AssemblerAarch64 masm(chunk_);
    __ Stp(Register(X1),  Register(X2), MemoryOperand(Register(SP), 8, MemoryOperand::AddrMode::POSTINDEX));
    __ Ldp(Register(X1),  Register(X2), MemoryOperand(Register(SP), 8, MemoryOperand::AddrMode::PREINDEX));
    __ Ldp(Register(X3),  Register(X4), MemoryOperand(Register(SP), 8, MemoryOperand::AddrMode::OFFSET));
    __ Ldp(Register(X3).W(),  Register(X4).W(), MemoryOperand(Register(SP), 8, MemoryOperand::AddrMode::OFFSET));
    std::ostringstream oss;
    DisassembleChunk("aarch64-unknown-linux-gnu", &masm, oss);
    ASSERT_EQ(oss.str(), expectResult);
}

HWTEST_F_L0(AssemblerAarch64Test, LdrStr)
{
    std::string expectResult("00000000:f80087e1 \tstr\tx1, [sp], #8\n"
                             "00000004:f81f87e1 \tstr\tx1, [sp], #-8\n"
                             "00000008:f8408fe1 \tldr\tx1, [sp, #8]!\n"
                             "0000000c:f94007e3 \tldr\tx3, [sp, #8]\n"
                             "00000010:b9400be3 \tldr\tw3, [sp, #8]\n");

    AssemblerAarch64 masm(chunk_);
    __ Str(Register(X1), MemoryOperand(Register(SP), 8, MemoryOperand::AddrMode::POSTINDEX));
    __ Str(Register(X1), MemoryOperand(Register(SP), -8, MemoryOperand::AddrMode::POSTINDEX));
    __ Ldr(Register(X1), MemoryOperand(Register(SP), 8, MemoryOperand::AddrMode::PREINDEX));
    __ Ldr(Register(X3), MemoryOperand(Register(SP), 8, MemoryOperand::AddrMode::OFFSET));
    __ Ldr(Register(X3).W(), MemoryOperand(Register(SP), 8, MemoryOperand::AddrMode::OFFSET));
    std::ostringstream oss;
    DisassembleChunk("aarch64-unknown-linux-gnu", &masm, oss);
    ASSERT_EQ(oss.str(), expectResult);
}

HWTEST_F_L0(AssemblerAarch64Test, AddSub)
{
    std::string expectResult("00000000:910023ff \tadd\tsp, sp, #8\n"
                             "00000004:d10023ff \tsub\tsp, sp, #8\n"
                             "00000008:8b020021 \tadd\tx1, x1, x2\n"
                             "0000000c:8b030c41 \tadd\tx1, x2, x3, lsl #3\n"
                             "00000010:8b234c41 \tadd\tx1, x2, w3, uxtw #3\n"
                             "00000014:8b224fff \tadd\tsp, sp, w2, uxtw #3\n");
    AssemblerAarch64 masm(chunk_);
    __ Add(Register(SP), Register(SP), Immediate(8));
    __ Add(Register(SP), Register(SP), Immediate(-8));
    __ Add(Register(X1), Register(X1), Operand(Register(X2)));
    __ Add(Register(X1), Register(X2), Operand(Register(X3), LSL, 3));
    __ Add(Register(X1), Register(X2), Operand(Register(X3), UXTW, 3));
    __ Add(Register(SP), Register(SP), Operand(Register(X2), UXTW, 3));

    std::ostringstream oss;
    DisassembleChunk("aarch64-unknown-linux-gnu", &masm, oss);
    ASSERT_EQ(oss.str(), expectResult);
}

HWTEST_F_L0(AssemblerAarch64Test, CMP)
{
    std::string expectResult("00000000:eb02003f \tcmp\tx1, x2\n"
                             "00000004:f100203f \tcmp\tx1, #8\n");
    AssemblerAarch64 masm(chunk_);
    __ Cmp(Register(X1), Register(X2));
    __ Cmp(Register(X1), Immediate(8));

    std::ostringstream oss;
    DisassembleChunk("aarch64-unknown-linux-gnu", &masm, oss);
    ASSERT_EQ(oss.str(), expectResult);
}

HWTEST_F_L0(AssemblerAarch64Test, Branch)
{
    std::string expectResult("00000000:eb02003f \tcmp\tx1, x2\n"
                             "00000004:54000080 \tb.eq\t0x14\n"
                             "00000008:f100203f \tcmp\tx1, #8\n"
                             "0000000c:5400004c \tb.gt\t0x14\n"
                             "00000010:14000002 \tb\t0x18\n"
                             "00000014:d2800140 \tmov\tx0, #10\n"
                             "00000018:b27f03e0 \torr\tx0, xzr, #0x2\n");
    AssemblerAarch64 masm(chunk_);
    Label label1;
    Label label2;
    __ Cmp(Register(X1), Register(X2));
    __ B(Condition::EQ, &label1);
    __ Cmp(Register(X1), Immediate(8));
    __ B(Condition::GT, &label1);
    __ B(&label2);
    __ Bind(&label1);
    {
        __ Mov(Register(X0), Immediate(0xa));
    }
    __ Bind(&label2);
    {
        __ Mov(Register(X0), Immediate(0x2));
    }

    std::ostringstream oss;
    DisassembleChunk("aarch64-unknown-linux-gnu", &masm, oss);
    ASSERT_EQ(oss.str(), expectResult);
}

HWTEST_F_L0(AssemblerAarch64Test, Loop)
{
    std::string expectResult("00000000:f100005f \tcmp\tx2, #0\n"
                             "00000004:540000e0 \tb.eq\t0x20\n"
                             "00000008:51000442 \tsub\tw2, w2, #1\n"
                             "0000000c:8b224c84 \tadd\tx4, x4, w2, uxtw #3\n"
                             "00000010:f85f8485 \tldr\tx5, [x4], #-8\n"
                             "00000014:f81f8fe5 \tstr\tx5, [sp, #-8]!\n"
                             "00000018:51000442 \tsub\tw2, w2, #1\n"
                             "0000001c:54ffffa5 \tb.pl\t0x10\n"
                             "00000020:d2800140 \tmov\tx0, #10\n");
    AssemblerAarch64 masm(chunk_);
    Label label1;
    Label labelLoop;
    Register count(X2, true);
    Register base(X4);
    Register temp(X5, false);
    __ Cmp(count, Immediate(0));
    __ B(Condition::EQ, &label1);
    __ Add(count, count, Immediate(-1));
    __ Add(base, base, Operand(count, UXTW, 3));
    __ Bind(&labelLoop);
    {
        __ Ldr(temp, MemoryOperand(base, -8, MemoryOperand::AddrMode::POSTINDEX));
        __ Str(temp, MemoryOperand(Register(SP), -8, MemoryOperand::AddrMode::PREINDEX));
        __ Add(count, count, Immediate(-1));
        __ B(Condition::PL, &labelLoop);
    }
    __ Bind(&label1);
    {
        __ Mov(Register(X0), Immediate(0xa));
    }
    std::ostringstream oss;
    DisassembleChunk("aarch64-unknown-linux-gnu", &masm, oss);
    ASSERT_EQ(oss.str(), expectResult);
}

HWTEST_F_L0(AssemblerAarch64Test, TbzAndCbz)
{
    std::string expectResult("00000000:36780001 \ttbz\tw1, #15, 0x0\n"
                             "00000004:b60000c2 \ttbz\tx2, #32, 0x1c\n"
                             "00000008:372800c2 \ttbnz\tw2, #5, 0x20\n"
                             "0000000c:34000063 \tcbz\tw3, 0x18\n"
                             "00000010:b5000064 \tcbnz\tx4, 0x1c\n"
                             "00000014:b4000065 \tcbz\tx5, 0x20\n"
                             "00000018:b24003e0 \torr\tx0, xzr, #0x1\n"
                             "0000001c:b27f03e0 \torr\tx0, xzr, #0x2\n"
                             "00000020:b24007e0 \torr\tx0, xzr, #0x3\n");
    AssemblerAarch64 masm(chunk_);
    Label label1;
    Label label2;
    Label label3;
    __ Tbz(Register(X1), 15, &label1);
    __ Tbz(Register(X2), 32,  &label2);
    __ Tbnz(Register(X2), 5,  &label3);
    __ Cbz(Register(X3).W(), &label1);
    __ Cbnz(Register(X4), &label2);
    __ Cbz(Register(X5), &label3);
    __ Bind(&label1);
    {
        __ Mov(Register(X0), Immediate(0x1));
    }
    __ Bind(&label2);
    {
        __ Mov(Register(X0), Immediate(0x2));
    }
    __ Bind(&label3);
    {
        __ Mov(Register(X0), Immediate(0x3));
    }
    std::ostringstream oss;
    DisassembleChunk("aarch64-unknown-linux-gnu", &masm, oss);
    ASSERT_EQ(oss.str(), expectResult);
}

HWTEST_F_L0(AssemblerAarch64Test, Call)
{
    std::string expectResult("00000000:b24003e0 \torr\tx0, xzr, #0x1\n"
                             "00000004:b27f03e1 \torr\tx1, xzr, #0x2\n"
                             "00000008:b24007e2 \torr\tx2, xzr, #0x3\n"
                             "0000000c:97fffffd \tbl\t0x0\n"
                             "00000010:d63f0040 \tblr\tx2\n");
    AssemblerAarch64 masm(chunk_);
    Label label1;
    __ Bind(&label1);
    {
        __ Mov(Register(X0), Immediate(0x1));
        __ Mov(Register(X1), Immediate(0x2));
        __ Mov(Register(X2), Immediate(0x3));
        __ Bl(&label1);
        __ Blr(Register(X2));
    }
    std::ostringstream oss;
    DisassembleChunk("aarch64-unknown-linux-gnu", &masm, oss);
    ASSERT_EQ(oss.str(), expectResult);
}

HWTEST_F_L0(AssemblerAarch64Test, RetAndBrk)
{
    std::string expectResult("00000000:d65f03c0 \tret\n"
                             "00000004:d65f0280 \tret\tx20\n"
                             "00000008:d4200000 \tbrk\t#0\n");
    AssemblerAarch64 masm(chunk_);
    __ Ret();
    __ Ret(Register(X20));
    __ Brk(Immediate(0));

    std::ostringstream oss;
    DisassembleChunk("aarch64-unknown-linux-gnu", &masm, oss);
    ASSERT_EQ(oss.str(), expectResult);
}
#undef __
}