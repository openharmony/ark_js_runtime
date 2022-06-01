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

#include "extended_assembler_x64.h"
#include "ecmascript/frames.h"

namespace panda::ecmascript::x64 {
Register ExtendedAssembler::ghcJSCallDispacherArgs_[JS_CALL_DISPATCHER_ARGS_COUNT] =
    { r13, rbp, r12, rbx, r14, rsi, rdi, r8 };
Register ExtendedAssembler::cppJSCallDispacherArgs_[JS_CALL_DISPATCHER_ARGS_COUNT] =
    { rdi, rbp, rsi, rdx, rcx, r8, r9, rInvalid };

void ExtendedAssembler::PushAlignBytes()
{
    Subq(8, rsp);  // 8: 8 bytes
}

void ExtendedAssembler::PopAlignBytes()
{
    Addq(8, rsp);  // 8: 8 bytes
}

// c++ calling convention
void ExtendedAssembler::PushCppCalleeSaveRegisters()
{
    Pushq(r12);
    Pushq(r13);
    Pushq(r14);
    Pushq(r15);
    Pushq(rbx);
}

void ExtendedAssembler::PopCppCalleeSaveRegisters()
{
    Popq(rbx);
    Popq(r15);
    Popq(r14);
    Popq(r13);
    Popq(r12);
}

void ExtendedAssembler::PushGhcCalleeSaveRegisters()
{
    Pushq(r10);
    Pushq(r11);
    Pushq(r12);
    Pushq(r13);
    Pushq(r15);
}

void ExtendedAssembler::PopGhcCalleeSaveRegisters()
{
    Popq(r15);
    Popq(r13);
    Popq(r12);
    Popq(r11);
    Popq(r10);
}

void ExtendedAssembler::PushArgsWithArgv(Register argc, Register argv, Register operatorRegister)
{
    Label loopBeginning;
    Bind(&loopBeginning);
    Movq(Operand(argv, argc, Times8, -8), operatorRegister);  // 8: 8 bytes
    Pushq(operatorRegister);
    Subq(1, argc);
    Ja(&loopBeginning);
}

void ExtendedAssembler::CallAssemblerStub(int id, bool isTail)
{
    Label *target = module_->GetFunctionLabel(id);
    isTail ? Jmp(target) : Callq(target);
}

void ExtendedAssembler::BindAssemblerStub(int id)
{
    Label *target = module_->GetFunctionLabel(id);
    Bind(target);
    auto callSigns = module_->GetCSigns();
    auto cs = callSigns[id];
    isGhcCallingConv_ = cs->GetCallConv() ==
        CallSignature::CallConv::GHCCallConv;
}

void ExtendedAssembler::PushArgc(int32_t argc, Register tempArgcRegister)
{
    Movabs(JSTaggedValue(argc).GetRawData(), tempArgcRegister);
    Pushq(tempArgcRegister);
}

void ExtendedAssembler::PushArgc(Register argcRegister, Register tempArgcRegister)
{
    Movabs(JSTaggedValue::TAG_INT, tempArgcRegister);
    Orq(argcRegister, tempArgcRegister);
    Pushq(tempArgcRegister);
}
}  // panda::ecmascript::x64