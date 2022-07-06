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
#ifndef ECMASCRIPT_COMPILER_EXTENDED_ASSEMBLER_X64_H
#define ECMASCRIPT_COMPILER_EXTENDED_ASSEMBLER_X64_H

#include "assembler_x64.h"
#include "ecmascript/compiler/assembler_module.h"
#include "ecmascript/compiler/bc_call_signature.h"

namespace panda::ecmascript::x64 {
// ExtendedAssembler implements frequently-used assembler macros with some extended usages.
class ExtendedAssembler : public AssemblerX64 {
public:
    explicit ExtendedAssembler(Chunk *chunk, kungfu::AssemblerModule *module)
        : AssemblerX64(chunk), module_(module)
    {
    }
    void CallAssemblerStub(int id, bool isTail);
    void BindAssemblerStub(int id);
    void PushAlignBytes();
    void PopAlignBytes();
    void PushCppCalleeSaveRegisters();
    void PopCppCalleeSaveRegisters();
    void PushGhcCalleeSaveRegisters();
    void PopGhcCalleeSaveRegisters();
    void PushArgsWithArgv(Register argc, Register argv, Register operatorRegister);
    void PushArgc(int32_t argc, Register tempArgcRegister);
    void PushArgc(Register argcRegister, Register tempArgcRegister);

    Register TempRegister()
    {
        if (tempInUse_) {
            LOG_COMPILER(ERROR) << "temp register inuse.";
            UNREACHABLE();
        }
        tempInUse_ = true;
        return rax;
    }
    Register AvailableRegister1() const
    {
        // r10 is neither callee saved reegister nor argument register
        return r10;
    }
    Register AvailableRegister2() const
    {
        // r11 is neither callee saved reegister nor argument register
        return r11;
    }
    Register CallDispatcherArgument(kungfu::CallDispatchInputs index)
    {
        size_t i = static_cast<size_t>(index);
        return isGhcCallingConv_ ? ghcJSCallDispacherArgs_[i] : cppJSCallDispacherArgs_[i];
    }
    Register GlueRegister()
    {
        return isGhcCallingConv_ ? r13 : rdi;
    }

    bool FromInterpreterHandler() const
    {
        return isGhcCallingConv_;
    }

private:
    kungfu::AssemblerModule *module_;
    bool isGhcCallingConv_ {false};
    bool tempInUse_ {false};
    friend class TempRegisterScope;

    static constexpr size_t JS_CALL_DISPATCHER_ARGS_COUNT =
        static_cast<size_t>(kungfu::CallDispatchInputs::NUM_OF_INPUTS);
    static Register ghcJSCallDispacherArgs_[JS_CALL_DISPATCHER_ARGS_COUNT];
    static Register cppJSCallDispacherArgs_[JS_CALL_DISPATCHER_ARGS_COUNT];
};

class TempRegisterScope {
public:
    explicit TempRegisterScope(ExtendedAssembler *assembler) : assembler_(assembler) {}
    ~TempRegisterScope()
    {
        assembler_->tempInUse_ = false;
    }

    NO_COPY_SEMANTIC(TempRegisterScope);
    NO_MOVE_SEMANTIC(TempRegisterScope);
private:
    ExtendedAssembler *assembler_;
};
}  // panda::ecmascript::x64
#endif  // ECMASCRIPT_COMPILER_EXTENDED_ASSEMBLER_X64_H