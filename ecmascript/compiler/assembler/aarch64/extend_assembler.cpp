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
#include "extend_assembler.h"
#include "ecmascript/frames.h"

namespace panda::ecmascript::aarch64 {
Register ExtendedAssembler::ghcJSCallDispacherArgs_[JS_CALL_DISPATCHER_ARGS_COUNT] =
    { X19, FP, X20, X21, X22, X23, X24, X25 };
Register ExtendedAssembler::cppJSCallDispacherArgs_[JS_CALL_DISPATCHER_ARGS_COUNT] =
    { X0, FP, X1, X2, X3, X4, X5, INVALID_REG};

void ExtendedAssembler::CalleeSave()
{
    Register sp(SP);
    Stp(Register(X27), Register(X28), MemoryOperand(sp, -16, PREINDEX));
    Stp(Register(X25), Register(X26), MemoryOperand(sp, -16, PREINDEX));
    Stp(Register(X23), Register(X24), MemoryOperand(sp, -16, PREINDEX));
    Stp(Register(X21), Register(X22), MemoryOperand(sp, -16, PREINDEX));
    Stp(Register(X19), Register(X20), MemoryOperand(sp, -16, PREINDEX));

    Stp(VectorRegister(v14), VectorRegister(v15), MemoryOperand(sp, -16, PREINDEX));
    Stp(VectorRegister(v12), VectorRegister(v13), MemoryOperand(sp, -16, PREINDEX));
    Stp(VectorRegister(v10), VectorRegister(v11), MemoryOperand(sp, -16, PREINDEX));
    Stp(VectorRegister(v8), VectorRegister(v9), MemoryOperand(sp, -16, PREINDEX));
}

void ExtendedAssembler::CalleeRestore()
{
    Register sp(SP);
    Ldp(VectorRegister(v8), VectorRegister(v9), MemoryOperand(sp, 16, POSTINDEX));
    Ldp(VectorRegister(v10), VectorRegister(v11), MemoryOperand(sp, 16, POSTINDEX));
    Ldp(VectorRegister(v12), VectorRegister(v13), MemoryOperand(sp, 16, POSTINDEX));
    Ldp(VectorRegister(v14), VectorRegister(v15), MemoryOperand(sp, 16, POSTINDEX));
    Ldp(Register(X19), Register(X20), MemoryOperand(sp, 16, POSTINDEX));
    Ldp(Register(X21), Register(X22), MemoryOperand(sp, 16, POSTINDEX));
    Ldp(Register(X23), Register(X24), MemoryOperand(sp, 16, POSTINDEX));
    Ldp(Register(X25), Register(X26), MemoryOperand(sp, 16, POSTINDEX));
    Ldp(Register(X27), Register(X28), MemoryOperand(sp, 16, POSTINDEX));
}

void ExtendedAssembler::CallAssemblerStub(int id, bool isTail)
{
    Label *target = module_->GetFunctionLabel(id);
    isTail ? B(target) : Bl(target);
}

void ExtendedAssembler::BindAssemblerStub(int id)
{
    Label *target = module_->GetFunctionLabel(id);
    Bind(target);
    auto callSigns = module_->GetCSigns();
    auto cs = callSigns[id];
    isGhcCallingConv_ = cs->GetCallConv() == CallSignature::CallConv::GHCCallConv;
}

void ExtendedAssembler::PushFpAndLr()
{
    Register sp(SP);
    Stp(Register(X29), Register(X30), MemoryOperand(sp, -16, PREINDEX));
}

void ExtendedAssembler::SaveFpAndLr()
{
    Register sp(SP);
    Stp(Register(X29), Register(X30), MemoryOperand(sp, -16, PREINDEX));
    Mov(Register(X29), Register(SP));
}

void ExtendedAssembler::RestoreFpAndLr()
{
    Register sp(SP);
    Ldp(Register(X29), Register(X30), MemoryOperand(sp, 16, POSTINDEX));
}

void ExtendedAssembler::PushArgsWithArgv(Register argc, Register argv, Register op, panda::ecmascript::Label *next)
{
    Label loopBeginning;
    Register sp(SP);
    if (next != nullptr) {
        Cmp(argc.W(), Immediate(0));
        B(Condition::LS, next);
    }
    Add(argv, argv, Operand(argc.W(), UXTW, 3));  // 3: argc * 8
    Bind(&loopBeginning);
    Ldr(op, MemoryOperand(argv, -8, PREINDEX));  // -8: 8 bytes
    Str(op, MemoryOperand(sp, -8, PREINDEX));  // -8: 8 bytes
    Sub(argc.W(), argc.W(), Immediate(1));
    Cbnz(argc.W(), &loopBeginning);
}

void ExtendedAssembler::PushArgc(int32_t argc, Register op)
{
    Mov(op, Immediate(JSTaggedValue(argc).GetRawData()));
    Str(op, MemoryOperand(Register(SP), -8, PREINDEX));  // -8: 8 bytes
}

void ExtendedAssembler::PushArgc(Register argc, Register op)
{
    Orr(op, argc, LogicalImmediate::Create(JSTaggedValue::TAG_INT, RegXSize));
    Str(op, MemoryOperand(Register(SP), -8, PREINDEX));  // -8: 8 bytes
}
}  // namespace panda::ecmascript::aarch64