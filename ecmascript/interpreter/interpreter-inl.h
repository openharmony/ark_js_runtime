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

#ifndef ECMASCRIPT_INTERPRETER_INTERPRETER_INL_H
#define ECMASCRIPT_INTERPRETER_INTERPRETER_INL_H

#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/ic_runtime_stub-inl.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/interpreter/interpreter_assembly.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/jspandafile/literal_data_extractor.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/module/js_module_manager.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/template_string.h"
#include "ecmascript/tooling/interface/js_debugger_manager.h"
#include "libpandafile/code_data_accessor.h"
#include "libpandafile/file.h"
#include "libpandafile/method_data_accessor-inl.h"

namespace panda::ecmascript {
using CommonStubCSigns = kungfu::CommonStubCSigns;
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wvoid-ptr-dereference"
#pragma clang diagnostic ignored "-Wgnu-label-as-value"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_INST() LOG(DEBUG, INTERPRETER) << ": "

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define HANDLE_OPCODE(handle_opcode) \
    handle_opcode:  // NOLINT(clang-diagnostic-gnu-label-as-value, cppcoreguidelines-macro-usage)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ADVANCE_PC(offset) \
    pc += (offset);  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic, cppcoreguidelines-macro-usage)

#define GOTO_NEXT()  // NOLINT(clang-diagnostic-gnu-label-as-value, cppcoreguidelines-macro-usage)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISPATCH(format)                                          \
    do {                                                          \
        ADVANCE_PC(BytecodeInstruction::Size(format))             \
        opcode = READ_INST_OP(); goto * (*dispatchTable)[opcode]; \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DISPATCH_OFFSET(offset)                                   \
    do {                                                          \
        ADVANCE_PC(offset)                                        \
        opcode = READ_INST_OP(); goto * (*dispatchTable)[opcode]; \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_FRAME(CurrentSp) \
    (reinterpret_cast<InterpretedFrame *>(CurrentSp) - 1)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_ENTRY_FRAME(sp) \
    (reinterpret_cast<InterpretedEntryFrame *>(sp) - 1)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_ENTRY_FRAME_WITH_ARGS_SIZE(actualNumArgs) \
    (static_cast<uint32_t>(INTERPRETER_ENTRY_FRAME_STATE_SIZE + 1U + (actualNumArgs) + RESERVED_CALL_ARGCOUNT))
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SAVE_PC() (GET_FRAME(sp)->pc = pc)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SAVE_ACC() (GET_FRAME(sp)->acc = acc)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RESTORE_ACC() (acc = GET_FRAME(sp)->acc)  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_VREG(idx) (sp[idx])  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_VREG_VALUE(idx) (JSTaggedValue(sp[idx]))  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SET_VREG(idx, val) (sp[idx] = (val));  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
#define GET_ACC() (acc)                        // NOLINT(cppcoreguidelines-macro-usage)
#define SET_ACC(val) (acc = val);              // NOLINT(cppcoreguidelines-macro-usage)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTERPRETER_GOTO_EXCEPTION_HANDLER()          \
    do {                                              \
        SAVE_PC();                                    \
        goto *(*dispatchTable)[EcmaOpcode::LAST_OPCODE]; \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTERPRETER_HANDLE_RETURN()                                                     \
    do {                                                                                \
        size_t jumpSize = GetJumpSizeAfterCall(pc);                                     \
        DISPATCH_OFFSET(jumpSize);                                                      \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CHECK_SWITCH_TO_DEBUGGER_TABLE()    \
    if (ecmaVm->GetJsDebuggerManager()->IsDebugMode()) { \
        dispatchTable = &debugDispatchTable; \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define REAL_GOTO_DISPATCH_OPCODE(opcode) \
    do {                                  \
        goto *instDispatchTable[opcode];  \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define REAL_GOTO_EXCEPTION_HANDLER()                     \
    do {                                                  \
        SAVE_PC();                                        \
        goto *instDispatchTable[EcmaOpcode::LAST_OPCODE]; \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define INTERPRETER_RETURN_IF_ABRUPT(result)      \
    do {                                          \
        if (result.IsException()) {               \
            INTERPRETER_GOTO_EXCEPTION_HANDLER(); \
        }                                         \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NOTIFY_DEBUGGER_EVENT()          \
    do {                                 \
        SAVE_ACC();                      \
        SAVE_PC();                       \
        NotifyBytecodePcChanged(thread); \
        RESTORE_ACC();                   \
    } while (false)

/*
 * reasons of set acc with hole:
 * 1. acc will become illegal when new error
 * 2. debugger logic will save acc, so illegal acc will set to frame
 * 3. when debugger trigger gc, will mark an invalid acc and crash
 * 4. acc will set to exception later, so it can set to hole template
 */
#define NOTIFY_DEBUGGER_EXCEPTION_EVENT() \
    do {                                  \
        SET_ACC(JSTaggedValue::Hole());   \
        NOTIFY_DEBUGGER_EVENT();          \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_INITIALIZE()                                             \
    do {                                                              \
        SAVE_PC();                                                    \
        thread->CheckSafepoint();                                     \
        funcTagged = sp[funcReg];                                     \
        JSTaggedValue funcValue(funcTagged);                          \
        if (!funcValue.IsCallable()) {                                \
            {                                                         \
                [[maybe_unused]] EcmaHandleScope handleScope(thread); \
                JSHandle<JSObject> error = factory->GetJSError(       \
                    ErrorType::TYPE_ERROR, "is not callable");        \
                thread->SetException(error.GetTaggedValue());         \
            }                                                         \
            INTERPRETER_GOTO_EXCEPTION_HANDLER();                     \
        }                                                             \
        funcObject = ECMAObject::Cast(funcValue.GetTaggedObject());   \
        method = funcObject->GetCallTarget();                         \
        newSp = sp - INTERPRETER_FRAME_STATE_SIZE;                    \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_UNDEFINED(n)                           \
    do {                                                 \
        for (int i = 0; i < (n); i++) {                  \
            *(--newSp) = JSTaggedValue::VALUE_UNDEFINED; \
        }                                                \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_0()          \
    do {                            \
        /* do nothing when 0 arg */ \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_1()   \
    do {                     \
        *(--newSp) = sp[a0]; \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_2()   \
    do {                     \
        *(--newSp) = sp[a1]; \
        CALL_PUSH_ARGS_1();  \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_3()   \
    do {                     \
        *(--newSp) = sp[a2]; \
        CALL_PUSH_ARGS_2();  \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_I()                                                   \
    do {                                                                     \
        if (UNLIKELY(thread->DoStackOverflowCheck(newSp - actualNumArgs))) { \
            INTERPRETER_GOTO_EXCEPTION_HANDLER();                            \
        }                                                                    \
        for (int i = actualNumArgs; i > 0; i--) {                            \
            *(--newSp) = sp[funcReg + static_cast<uint32_t>(i)];             \
        }                                                                    \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_I_THIS()                                              \
    do {                                                                     \
        if (UNLIKELY(thread->DoStackOverflowCheck(newSp - actualNumArgs))) { \
            INTERPRETER_GOTO_EXCEPTION_HANDLER();                            \
        }                                                                    \
        /* 1: skip this */                                                   \
        for (int i = actualNumArgs + 1; i > 1; i--) {                        \
            *(--newSp) = sp[funcReg + static_cast<uint32_t>(i)];             \
        }                                                                    \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_0_NO_EXTRA() \
    do {                            \
        /* do nothing when 0 arg */ \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_1_NO_EXTRA()                             \
    do {                                                        \
        if (declaredNumArgs >= ActualNumArgsOfCall::CALLARG1) { \
            *(--newSp) = sp[a0];                                \
        }                                                       \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_2_NO_EXTRA()                              \
    do {                                                         \
        if (declaredNumArgs >= ActualNumArgsOfCall::CALLARGS2) { \
            *(--newSp) = sp[a1];                                 \
        }                                                        \
        CALL_PUSH_ARGS_1_NO_EXTRA();                             \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_3_NO_EXTRA()                              \
    do {                                                         \
        if (declaredNumArgs >= ActualNumArgsOfCall::CALLARGS3) { \
            *(--newSp) = sp[a2];                                 \
        }                                                        \
        CALL_PUSH_ARGS_2_NO_EXTRA();                             \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_I_NO_EXTRA()                                          \
    do {                                                                     \
        int num = std::min(actualNumArgs, declaredNumArgs);                  \
        if (UNLIKELY(thread->DoStackOverflowCheck(newSp - num))) {           \
            INTERPRETER_GOTO_EXCEPTION_HANDLER();                            \
        }                                                                    \
        for (int i = num; i > 0; i--) {                                      \
            *(--newSp) = sp[funcReg + static_cast<uint32_t>(i)];             \
        }                                                                    \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS_I_THIS_NO_EXTRA()                                         \
    do {                                                                         \
        int num = std::min(actualNumArgs, declaredNumArgs);                      \
        if (UNLIKELY(thread->DoStackOverflowCheck(newSp - num))) {               \
            INTERPRETER_GOTO_EXCEPTION_HANDLER();                                \
        }                                                                        \
        /* 1: skip this */                                                       \
        for (int i = num + 1; i > 1; i--) {                                      \
            *(--newSp) = sp[funcReg + static_cast<uint32_t>(i)];                 \
        }                                                                        \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CALL_PUSH_ARGS(ARG_TYPE)                                                   \
    do {                                                                           \
        if (method->IsNativeWithCallField()) {                                     \
            /* native, just push all args directly */                              \
            CALL_PUSH_ARGS_##ARG_TYPE();                                           \
            goto setVregsAndFrameNative;                                           \
        }                                                                          \
        int32_t declaredNumArgs = static_cast<int32_t>(                            \
            method->GetNumArgsWithCallField());                                    \
        if (actualNumArgs == declaredNumArgs) {                                    \
            /* fast path, just push all args directly */                           \
            CALL_PUSH_ARGS_##ARG_TYPE();                                           \
            goto setVregsAndFrameNotNative;                                        \
        }                                                                          \
        /* slow path */                                                            \
        if (!method->HaveExtraWithCallField()) {                                   \
            /* push length = declaredNumArgs, may push undefined */                \
            CALL_PUSH_UNDEFINED(declaredNumArgs - actualNumArgs);                  \
            CALL_PUSH_ARGS_##ARG_TYPE##_NO_EXTRA();                                \
        } else {                                                                   \
            /* push actualNumArgs in the end, then all args, may push undefined */ \
            *(--newSp) = JSTaggedValue(actualNumArgs).GetRawData();                \
            CALL_PUSH_UNDEFINED(declaredNumArgs - actualNumArgs);                  \
            CALL_PUSH_ARGS_##ARG_TYPE();                                           \
        }                                                                          \
        goto setVregsAndFrameNotNative;                                            \
    } while (false)

#if ECMASCRIPT_ENABLE_IC
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define UPDATE_HOTNESS_COUNTER_NON_ACC(offset)   (UpdateHotnessCounter(thread, sp, acc, offset))

#define UPDATE_HOTNESS_COUNTER(offset)                       \
    do {                                                     \
        if (UpdateHotnessCounter(thread, sp, acc, offset)) { \
            RESTORE_ACC();                                   \
        }                                                    \
    } while (false)
#else
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define UPDATE_HOTNESS_COUNTER(offset) static_cast<void>(0)
#define UPDATE_HOTNESS_COUNTER_NON_ACC(offset) static_cast<void>(0)
#endif

#define READ_INST_OP() READ_INST_8(0)               // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_4_0() (READ_INST_8(1) & 0xf)      // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_4_1() (READ_INST_8(1) >> 4 & 0xf) // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_4_2() (READ_INST_8(2) & 0xf)      // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_4_3() (READ_INST_8(2) >> 4 & 0xf) // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_0() READ_INST_8(1)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_1() READ_INST_8(2)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_2() READ_INST_8(3)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_3() READ_INST_8(4)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_4() READ_INST_8(5)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_5() READ_INST_8(6)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_6() READ_INST_8(7)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_7() READ_INST_8(8)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8_8() READ_INST_8(9)              // NOLINT(hicpp-signed-bitwise, cppcoreguidelines-macro-usage)
#define READ_INST_8(offset) (*(pc + (offset)))
#define MOVE_AND_READ_INST_8(currentInst, offset) \
    currentInst <<= 8;                            \
    currentInst += READ_INST_8(offset);           \

#define READ_INST_16_0() READ_INST_16(2)
#define READ_INST_16_1() READ_INST_16(3)
#define READ_INST_16_2() READ_INST_16(4)
#define READ_INST_16_3() READ_INST_16(5)
#define READ_INST_16_5() READ_INST_16(7)
#define READ_INST_16(offset)                          \
    ({                                                \
        uint16_t currentInst = READ_INST_8(offset);   \
        MOVE_AND_READ_INST_8(currentInst, offset - 1) \
    })

#define READ_INST_32_0() READ_INST_32(4)
#define READ_INST_32_1() READ_INST_32(5)
#define READ_INST_32_2() READ_INST_32(6)
#define READ_INST_32(offset)                          \
    ({                                                \
        uint32_t currentInst = READ_INST_8(offset);   \
        MOVE_AND_READ_INST_8(currentInst, offset - 1) \
        MOVE_AND_READ_INST_8(currentInst, offset - 2) \
        MOVE_AND_READ_INST_8(currentInst, offset - 3) \
    })

#define READ_INST_64_0()                       \
    ({                                         \
        uint64_t currentInst = READ_INST_8(8); \
        MOVE_AND_READ_INST_8(currentInst, 7)   \
        MOVE_AND_READ_INST_8(currentInst, 6)   \
        MOVE_AND_READ_INST_8(currentInst, 5)   \
        MOVE_AND_READ_INST_8(currentInst, 4)   \
        MOVE_AND_READ_INST_8(currentInst, 3)   \
        MOVE_AND_READ_INST_8(currentInst, 2)   \
        MOVE_AND_READ_INST_8(currentInst, 1)   \
    })

JSTaggedValue EcmaInterpreter::ExecuteNative(EcmaRuntimeCallInfo *info)
{
    JSThread *thread = info->GetThread();
    INTERPRETER_TRACE(thread, ExecuteNative);

    // current is entry frame.
    JSTaggedType *sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    int32_t actualNumArgs = static_cast<int32_t>(info->GetArgsNumber());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    JSTaggedType *newSp = sp - GET_ENTRY_FRAME_WITH_ARGS_SIZE(static_cast<uint32_t>(actualNumArgs));

    InterpretedFrame *state = GET_FRAME(newSp);
    state->base.prev = sp;
    state->base.type = FrameType::INTERPRETER_FRAME;
    state->pc = nullptr;
    state->function = info->GetFunctionValue();
    thread->SetCurrentSPFrame(newSp);

#if ECMASCRIPT_ENABLE_ACTIVE_CPUPROFILER
    CpuProfiler::IsNeedAndGetStack(thread);
#endif
    thread->CheckSafepoint();
    ECMAObject *callTarget = reinterpret_cast<ECMAObject*>(info->GetFunctionValue().GetTaggedObject());
    JSMethod *method = callTarget->GetCallTarget();
    LOG(DEBUG, INTERPRETER) << "Entry: Runtime Call.";
    JSTaggedValue tagged =
        reinterpret_cast<EcmaEntrypoint>(const_cast<void *>(method->GetNativePointer()))(info);
    LOG(DEBUG, INTERPRETER) << "Exit: Runtime Call.";

    InterpretedEntryFrame *entryState = GET_ENTRY_FRAME(sp);
    JSTaggedType *prevSp = entryState->base.prev;
    thread->SetCurrentSPFrame(prevSp);
#if ECMASCRIPT_ENABLE_ACTIVE_CPUPROFILER
    CpuProfiler::IsNeedAndGetStack(thread);
#endif
    return tagged;
}

JSTaggedValue EcmaInterpreter::Execute(EcmaRuntimeCallInfo *info)
{
    if (info == nullptr || (info->GetArgsNumber() == INVALID_ARGS_NUMBER)) {
        return JSTaggedValue::Exception();
    }

    JSThread *thread = info->GetThread();
    INTERPRETER_TRACE(thread, Execute);
    if (thread->IsAsmInterpreter()) {
        return InterpreterAssembly::Execute(info);
    }
    JSHandle<JSTaggedValue> func = info->GetFunction();
    ECMAObject *callTarget = reinterpret_cast<ECMAObject*>(func.GetTaggedValue().GetTaggedObject());
    ASSERT(callTarget != nullptr);
    JSMethod *method = callTarget->GetCallTarget();
    if (method->IsNativeWithCallField()) {
        return EcmaInterpreter::ExecuteNative(info);
    }

    // current is entry frame.
    JSTaggedType *sp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());
    int32_t actualNumArgs = static_cast<int32_t>(info->GetArgsNumber());
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    JSTaggedType *newSp = sp - GET_ENTRY_FRAME_WITH_ARGS_SIZE(static_cast<uint32_t>(actualNumArgs));
    if (UNLIKELY(thread->DoStackOverflowCheck(newSp - actualNumArgs - RESERVED_CALL_ARGCOUNT))) {
        return JSTaggedValue::Undefined();
    }

    int32_t declaredNumArgs = static_cast<int32_t>(method->GetNumArgsWithCallField());
    // push args
    if (actualNumArgs == declaredNumArgs) {
        // fast path, just push all args directly
        for (int i = actualNumArgs - 1; i >= 0; i--) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            *(--newSp) = info->GetCallArgValue(i).GetRawData();
        }
    } else {
        // slow path
        if (!method->HaveExtraWithCallField()) {
            // push length = declaredNumArgs, may push undefined
            if (declaredNumArgs > actualNumArgs) {
                CALL_PUSH_UNDEFINED(declaredNumArgs - actualNumArgs);
            }
            for (int32_t i = std::min(actualNumArgs, declaredNumArgs) - 1; i >= 0; i--) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                *(--newSp) = info->GetCallArgValue(i).GetRawData();
            }
        } else {
            // push actualNumArgs in the end, then all args, may push undefined
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            *(--newSp) = JSTaggedValue(actualNumArgs).GetRawData();
            if (declaredNumArgs > actualNumArgs) {
                CALL_PUSH_UNDEFINED(declaredNumArgs - actualNumArgs);
            }
            for (int32_t i = actualNumArgs - 1; i >= 0; i--) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                *(--newSp) = info->GetCallArgValue(i).GetRawData();
            }
        }
    }
    uint64_t callField = method->GetCallField();
    if ((callField & CALL_TYPE_MASK) != 0) {
        // not normal call type, setting func/newTarget/this cannot be skipped
        if (method->HaveThisWithCallField()) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            *(--newSp) = info->GetThisValue().GetRawData();  // push this
        }
        if (method->HaveNewTargetWithCallField()) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            *(--newSp) = info->GetNewTargetValue().GetRawData();  // push new target
        }
        if (method->HaveFuncWithCallField()) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            *(--newSp) = info->GetFunctionValue().GetRawData();  // push func
        }
    }
    int32_t numVregs = static_cast<int32_t>(method->GetNumVregsWithCallField());
    if (UNLIKELY(thread->DoStackOverflowCheck(newSp - numVregs))) {
        return JSTaggedValue::Undefined();
    }
    // push vregs
    CALL_PUSH_UNDEFINED(numVregs);

    const uint8_t *pc = method->GetBytecodeArray();
    InterpretedFrame *state = GET_FRAME(newSp);
    state->pc = pc;
    state->function = info->GetFunctionValue();
    state->acc = JSTaggedValue::Hole();
    JSHandle<JSFunction> thisFunc = JSHandle<JSFunction>::Cast(func);
    JSTaggedValue constpool = thisFunc->GetConstantPool();
    state->constpool = constpool;
    state->profileTypeInfo = thisFunc->GetProfileTypeInfo();
    state->base.prev = sp;
    state->base.type = FrameType::INTERPRETER_FRAME;
    state->env = thisFunc->GetLexicalEnv();
    thread->SetCurrentSPFrame(newSp);
#if ECMASCRIPT_ENABLE_ACTIVE_CPUPROFILER
    CpuProfiler::IsNeedAndGetStack(thread);
#endif
    thread->CheckSafepoint();
    LOG(DEBUG, INTERPRETER) << "Entry: Runtime Call " << std::hex << reinterpret_cast<uintptr_t>(newSp) << " "
                            << std::hex << reinterpret_cast<uintptr_t>(pc);

    EcmaInterpreter::RunInternal(thread, ConstantPool::Cast(constpool.GetTaggedObject()), pc, newSp);

    // NOLINTNEXTLINE(readability-identifier-naming)
    const JSTaggedValue resAcc = state->acc;
    // pop frame
    InterpretedEntryFrame *entryState = GET_ENTRY_FRAME(sp);
    JSTaggedType *prevSp = entryState->base.prev;
    thread->SetCurrentSPFrame(prevSp);
#if ECMASCRIPT_ENABLE_ACTIVE_CPUPROFILER
    CpuProfiler::IsNeedAndGetStack(thread);
#endif
    return resAcc;
}

JSTaggedValue EcmaInterpreter::GeneratorReEnterInterpreter(JSThread *thread, JSHandle<GeneratorContext> context)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    if (thread->IsAsmInterpreter()) {
        return InterpreterAssembly::GeneratorReEnterInterpreter(thread, context);
    }
    JSHandle<JSFunction> func = JSHandle<JSFunction>::Cast(JSHandle<JSTaggedValue>(thread, context->GetMethod()));
    JSMethod *method = func->GetCallTarget();

    JSTaggedType *currentSp = const_cast<JSTaggedType *>(thread->GetCurrentSPFrame());

    // push break frame
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    JSTaggedType *breakSp = currentSp - INTERPRETER_FRAME_STATE_SIZE;
    if (UNLIKELY(thread->DoStackOverflowCheck(breakSp))) {
        return JSTaggedValue::Exception();
    }

    InterpretedFrame *breakState = GET_FRAME(breakSp);
    breakState->pc = nullptr;
    breakState->function = JSTaggedValue::Hole();
    breakState->base.prev = currentSp;
    breakState->base.type = FrameType::INTERPRETER_FRAME;

    // create new frame and resume sp and pc
    uint32_t nregs = context->GetNRegs();
    size_t newFrameSize = INTERPRETER_FRAME_STATE_SIZE + nregs;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic
    JSTaggedType *newSp = breakSp - newFrameSize;
    if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
        return JSTaggedValue::Exception();
    }
    JSHandle<TaggedArray> regsArray(thread, context->GetRegsArray());
    for (size_t i = 0; i < nregs; i++) {
        newSp[i] = regsArray->Get(i).GetRawData();  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    JSTaggedValue constpool = func->GetConstantPool();
    uint32_t pcOffset = context->GetBCOffset();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    const uint8_t *resumePc = method->GetBytecodeArray() + pcOffset +
                              BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8);

    InterpretedFrame *state = GET_FRAME(newSp);
    state->pc = resumePc;
    state->function = func.GetTaggedValue();
    state->constpool = constpool;
    state->profileTypeInfo = func->GetProfileTypeInfo();
    state->acc = context->GetAcc();
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    state->base.prev = breakSp;
    state->base.type = FrameType::INTERPRETER_FRAME;
    JSTaggedValue env = context->GetLexicalEnv();
    state->env = env;
    // execute interpreter
    thread->SetCurrentSPFrame(newSp);

    EcmaInterpreter::RunInternal(thread, ConstantPool::Cast(constpool.GetTaggedObject()), resumePc, newSp);

    JSTaggedValue res = state->acc;
    // pop frame
    thread->SetCurrentSPFrame(currentSp);
    return res;
}

void EcmaInterpreter::NotifyBytecodePcChanged(JSThread *thread)
{
    FrameHandler frameHandler(thread);
    for (; frameHandler.HasFrame(); frameHandler.PrevInterpretedFrame()) {
        if (frameHandler.IsEntryFrame()) {
            continue;
        }
        JSMethod *method = frameHandler.GetMethod();
        // Skip builtins method
        if (method->IsNativeWithCallField()) {
            continue;
        }
        auto bcOffset = frameHandler.GetBytecodeOffset();
        auto *debuggerMgr = thread->GetEcmaVM()->GetJsDebuggerManager();
        debuggerMgr->GetNotificationManager()->BytecodePcChangedEvent(thread, method, bcOffset);
        return;
    }
}

const JSPandaFile *EcmaInterpreter::GetNativeCallPandafile(JSThread *thread)
{
    FrameHandler frameHandler(thread);
    for (; frameHandler.HasFrame(); frameHandler.PrevInterpretedFrame()) {
        if (frameHandler.IsEntryFrame()) {
            continue;
        }
        JSMethod *method = frameHandler.GetMethod();
        // Skip builtins method
        if (method->IsNativeWithCallField()) {
            continue;
        }
        const JSPandaFile *jsPandaFile = method->GetJSPandaFile();
        return jsPandaFile;
    }
    UNREACHABLE();
}

// NOLINTNEXTLINE(readability-function-size)
NO_UB_SANITIZE void EcmaInterpreter::RunInternal(JSThread *thread, ConstantPool *constpool, const uint8_t *pc,
                                                 JSTaggedType *sp)
{
    INTERPRETER_TRACE(thread, RunInternal);
    uint8_t opcode = READ_INST_OP();
    JSTaggedValue acc = JSTaggedValue::Hole();
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVm->GetGlobalEnv();
    JSTaggedValue globalObj = globalEnv->GetGlobalObject();
    ObjectFactory *factory = ecmaVm->GetFactory();

    constexpr size_t numOps = 0x100;
    static std::array<const void *, numOps> instDispatchTable {
#include "templates/instruction_dispatch.inl"
    };

    static std::array<const void *, numOps> debugDispatchTable {
#include "templates/debugger_instruction_dispatch.inl"
    };

    std::array<const void *, numOps> *dispatchTable = &instDispatchTable;
    CHECK_SWITCH_TO_DEBUGGER_TABLE();
    goto *(*dispatchTable)[opcode];

    HANDLE_OPCODE(HANDLE_MOV_V4_V4) {
        uint16_t vdst = READ_INST_4_0();
        uint16_t vsrc = READ_INST_4_1();
        LOG_INST() << "mov v" << vdst << ", v" << vsrc;
        uint64_t value = GET_VREG(vsrc);
        SET_VREG(vdst, value)
        DISPATCH(BytecodeInstruction::Format::V4_V4);
    }

    HANDLE_OPCODE(HANDLE_MOV_DYN_V8_V8) {
        uint16_t vdst = READ_INST_8_0();
        uint16_t vsrc = READ_INST_8_1();
        LOG_INST() << "mov.dyn v" << vdst << ", v" << vsrc;
        uint64_t value = GET_VREG(vsrc);
        SET_VREG(vdst, value)
        DISPATCH(BytecodeInstruction::Format::V8_V8);
    }
    HANDLE_OPCODE(HANDLE_MOV_DYN_V16_V16) {
        uint16_t vdst = READ_INST_16_0();
        uint16_t vsrc = READ_INST_16_2();
        LOG_INST() << "mov.dyn v" << vdst << ", v" << vsrc;
        uint64_t value = GET_VREG(vsrc);
        SET_VREG(vdst, value)
        DISPATCH(BytecodeInstruction::Format::V16_V16);
    }
    HANDLE_OPCODE(HANDLE_LDA_STR_ID32) {
        uint32_t stringId = READ_INST_32_0();
        LOG_INST() << "lda.str " << std::hex << stringId;
        SET_ACC(constpool->GetObjectFromCache(stringId));
        DISPATCH(BytecodeInstruction::Format::ID32);
    }
    HANDLE_OPCODE(HANDLE_JMP_IMM8) {
        int8_t offset = static_cast<int8_t>(READ_INST_8_0());
        UPDATE_HOTNESS_COUNTER(offset);
        LOG_INST() << "jmp " << std::hex << static_cast<int32_t>(offset);
        DISPATCH_OFFSET(offset);
    }
    HANDLE_OPCODE(HANDLE_JMP_IMM16) {
        int16_t offset = static_cast<int16_t>(READ_INST_16_0());
        UPDATE_HOTNESS_COUNTER(offset);
        LOG_INST() << "jmp " << std::hex << static_cast<int32_t>(offset);
        DISPATCH_OFFSET(offset);
    }
    HANDLE_OPCODE(HANDLE_JMP_IMM32) {
        int32_t offset = static_cast<int32_t>(READ_INST_32_0());
        UPDATE_HOTNESS_COUNTER(offset);
        LOG_INST() << "jmp " << std::hex << offset;
        DISPATCH_OFFSET(offset);
    }
    HANDLE_OPCODE(HANDLE_JEQZ_IMM8) {
        int8_t offset = static_cast<int8_t>(READ_INST_8_0());
        LOG_INST() << "jeqz ->\t"
                   << "cond jmpz " << std::hex << static_cast<int32_t>(offset);
        if (GET_ACC() == JSTaggedValue::False() || (GET_ACC().IsInt() && GET_ACC().GetInt() == 0) ||
            (GET_ACC().IsDouble() && GET_ACC().GetDouble() == 0)) {
            UPDATE_HOTNESS_COUNTER(offset);
            DISPATCH_OFFSET(offset);
        } else {
            DISPATCH(BytecodeInstruction::Format::PREF_NONE);
        }
    }
    HANDLE_OPCODE(HANDLE_JEQZ_IMM16) {
        int16_t offset = static_cast<int16_t>(READ_INST_16_0());
        LOG_INST() << "jeqz ->\t"
                   << "cond jmpz " << std::hex << static_cast<int32_t>(offset);
        if (GET_ACC() == JSTaggedValue::False() || (GET_ACC().IsInt() && GET_ACC().GetInt() == 0) ||
            (GET_ACC().IsDouble() && GET_ACC().GetDouble() == 0)) {
            UPDATE_HOTNESS_COUNTER(offset);
            DISPATCH_OFFSET(offset);
        } else {
            DISPATCH(BytecodeInstruction::Format::IMM16);
        }
    }
    HANDLE_OPCODE(HANDLE_JNEZ_IMM8) {
        int8_t offset = static_cast<int8_t>(READ_INST_8_0());
        LOG_INST() << "jnez ->\t"
                   << "cond jmpz " << std::hex << static_cast<int32_t>(offset);
        if (GET_ACC() == JSTaggedValue::True() || (GET_ACC().IsInt() && GET_ACC().GetInt() != 0) ||
            (GET_ACC().IsDouble() && GET_ACC().GetDouble() != 0)) {
            UPDATE_HOTNESS_COUNTER(offset);
            DISPATCH_OFFSET(offset);
        } else {
            DISPATCH(BytecodeInstruction::Format::PREF_NONE);
        }
    }
    HANDLE_OPCODE(HANDLE_JNEZ_IMM16) {
        int16_t offset = static_cast<int16_t>(READ_INST_16_0());
        LOG_INST() << "jnez ->\t"
                   << "cond jmpz " << std::hex << static_cast<int32_t>(offset);
        if (GET_ACC() == JSTaggedValue::True() || (GET_ACC().IsInt() && GET_ACC().GetInt() != 0) ||
            (GET_ACC().IsDouble() && GET_ACC().GetDouble() != 0)) {
            UPDATE_HOTNESS_COUNTER(offset);
            DISPATCH_OFFSET(offset);
        } else {
            DISPATCH(BytecodeInstruction::Format::IMM16);
        }
    }
    HANDLE_OPCODE(HANDLE_LDA_DYN_V8) {
        uint16_t vsrc = READ_INST_8_0();
        LOG_INST() << "lda.dyn v" << vsrc;
        uint64_t value = GET_VREG(vsrc);
        SET_ACC(JSTaggedValue(value))
        DISPATCH(BytecodeInstruction::Format::V8);
    }
    HANDLE_OPCODE(HANDLE_STA_DYN_V8) {
        uint16_t vdst = READ_INST_8_0();
        LOG_INST() << "sta.dyn v" << vdst;
        SET_VREG(vdst, GET_ACC().GetRawData())
        DISPATCH(BytecodeInstruction::Format::V8);
    }
    HANDLE_OPCODE(HANDLE_LDAI_DYN_IMM32) {
        int32_t imm = static_cast<int32_t>(READ_INST_32_0());
        LOG_INST() << "ldai.dyn " << std::hex << imm;
        SET_ACC(JSTaggedValue(imm))
        DISPATCH(BytecodeInstruction::Format::IMM32);
    }

    HANDLE_OPCODE(HANDLE_FLDAI_DYN_IMM64) {
        auto imm = bit_cast<double>(READ_INST_64_0());
        LOG_INST() << "fldai.dyn " << imm;
        SET_ACC(JSTaggedValue(imm))
        DISPATCH(BytecodeInstruction::Format::IMM64);
    }
    {
        int32_t actualNumArgs;
        uint32_t funcReg;
        JSTaggedType funcTagged;
        ECMAObject *funcObject;
        JSMethod *method;
        JSTaggedType *newSp;
        bool callThis;

        HANDLE_OPCODE(HANDLE_CALLARG0DYN_PREF_V8) {
            actualNumArgs = ActualNumArgsOfCall::CALLARG0;
            funcReg = READ_INST_8_1();
            LOG_INST() << "callarg0.dyn "
                       << "v" << funcReg;
            CALL_INITIALIZE();
            callThis = false;
            CALL_PUSH_ARGS(0);
        }
        HANDLE_OPCODE(HANDLE_CALLARG1DYN_PREF_V8_V8) {
            actualNumArgs = ActualNumArgsOfCall::CALLARG1;
            funcReg = READ_INST_8_1();
            uint8_t a0 = READ_INST_8_2();
            LOG_INST() << "callarg1.dyn "
                       << "v" << funcReg << ", v" << a0;
            CALL_INITIALIZE();
            callThis = false;
            CALL_PUSH_ARGS(1);
        }
        HANDLE_OPCODE(HANDLE_CALLARGS2DYN_PREF_V8_V8_V8) {
            actualNumArgs = ActualNumArgsOfCall::CALLARGS2;
            funcReg = READ_INST_8_1();
            uint8_t a0 = READ_INST_8_2();
            uint8_t a1 = READ_INST_8_3();
            LOG_INST() << "callargs2.dyn "
                       << "v" << funcReg << ", v" << a0 << ", v" << a1;
            CALL_INITIALIZE();
            callThis = false;
            CALL_PUSH_ARGS(2);
        }
        HANDLE_OPCODE(HANDLE_CALLARGS3DYN_PREF_V8_V8_V8_V8) {
            actualNumArgs = ActualNumArgsOfCall::CALLARGS3;
            funcReg = READ_INST_8_1();
            uint8_t a0 = READ_INST_8_2();
            uint8_t a1 = READ_INST_8_3();
            uint8_t a2 = READ_INST_8_4();
            LOG_INST() << "callargs3.dyn "
                       << "v" << funcReg << ", v" << a0 << ", v" << a1 << ", v" << a2;
            CALL_INITIALIZE();
            callThis = false;
            CALL_PUSH_ARGS(3);
        }
        HANDLE_OPCODE(HANDLE_CALLITHISRANGEDYN_PREF_IMM16_V8) {
            actualNumArgs = static_cast<int32_t>(READ_INST_16_1() - 1);  // 1: exclude this
            funcReg = READ_INST_8_3();
            LOG_INST() << "calli.dyn.this.range " << actualNumArgs << ", v" << funcReg;
            CALL_INITIALIZE();
            callThis = true;
            CALL_PUSH_ARGS(I_THIS);
        }
        HANDLE_OPCODE(HANDLE_CALLSPREADDYN_PREF_V8_V8_V8) {
            uint16_t v0 = READ_INST_8_1();
            uint16_t v1 = READ_INST_8_2();
            uint16_t v2 = READ_INST_8_3();
            LOG_INST() << "intrinsics::callspreaddyn"
                       << " v" << v0 << " v" << v1 << " v" << v2;
            JSTaggedValue func = GET_VREG_VALUE(v0);
            JSTaggedValue obj = GET_VREG_VALUE(v1);
            JSTaggedValue array = GET_VREG_VALUE(v2);

            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::CallSpreadDyn(thread, func, obj, array);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);

            DISPATCH(BytecodeInstruction::Format::PREF_V8_V8_V8);
        }
        HANDLE_OPCODE(HANDLE_CALLIRANGEDYN_PREF_IMM16_V8) {
            actualNumArgs = static_cast<int32_t>(READ_INST_16_1());
            funcReg = READ_INST_8_3();
            LOG_INST() << "calli.rangedyn " << actualNumArgs << ", v" << funcReg;
            CALL_INITIALIZE();
            callThis = false;
            CALL_PUSH_ARGS(I);
        }
        setVregsAndFrameNative: {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            *(--newSp) = (callThis ? sp[funcReg + callThis] : JSTaggedValue::VALUE_UNDEFINED);  // push this
            *(--newSp) = JSTaggedValue::VALUE_UNDEFINED;  // push new target
            *(--newSp) = static_cast<JSTaggedType>(ToUintPtr(funcObject));  // push func
            ASSERT(method->GetNumVregsWithCallField() == 0);  // no need to push vregs
            EcmaRuntimeCallInfo ecmaRuntimeCallInfo(thread, actualNumArgs, newSp);

            InterpretedFrame *state = GET_FRAME(newSp);
            state->base.prev = sp;
            state->base.type = FrameType::INTERPRETER_FRAME;
            state->pc = nullptr;
            state->function = JSTaggedValue(funcTagged);
            thread->SetCurrentSPFrame(newSp);
            LOG(DEBUG, INTERPRETER) << "Entry: Runtime Call.";
            JSTaggedValue retValue = reinterpret_cast<EcmaEntrypoint>(
                const_cast<void *>(method->GetNativePointer()))(&ecmaRuntimeCallInfo);
            thread->SetCurrentSPFrame(sp);
            if (UNLIKELY(thread->HasPendingException())) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }
            LOG(DEBUG, INTERPRETER) << "Exit: Runtime Call.";
            SET_ACC(retValue);
            INTERPRETER_HANDLE_RETURN();
        }
        setVregsAndFrameNotNative: {
            if (JSFunction::Cast(funcObject)->IsClassConstructor()) {
                {
                    [[maybe_unused]] EcmaHandleScope handleScope(thread);
                    JSHandle<JSObject> error =
                        factory->GetJSError(ErrorType::TYPE_ERROR, "class constructor cannot called without 'new'");
                    thread->SetException(error.GetTaggedValue());
                }
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }
            uint64_t callField = method->GetCallField();
            if ((callField & CALL_TYPE_MASK) != 0) {
                // not normal call type, setting func/newTarget/this cannot be skipped
                if (method->HaveThisWithCallField()) {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    *(--newSp) = (callThis ? sp[funcReg + callThis] : JSTaggedValue::VALUE_UNDEFINED);  // push this
                }
                if (method->HaveNewTargetWithCallField()) {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    *(--newSp) = JSTaggedValue::VALUE_UNDEFINED;  // push new target
                }
                if (method->HaveFuncWithCallField()) {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    *(--newSp) = static_cast<JSTaggedType>(ToUintPtr(funcObject));  // push func
                }
            }
            int32_t numVregs = static_cast<int32_t>(method->GetNumVregsWithCallField());
            if (UNLIKELY(thread->DoStackOverflowCheck(newSp - numVregs))) {
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }
            // push vregs
            CALL_PUSH_UNDEFINED(numVregs);
            SAVE_PC();

            InterpretedFrame *state = GET_FRAME(newSp);
            state->base.prev = sp;
            state->base.type = FrameType::INTERPRETER_FRAME;
            state->pc = pc = method->GetBytecodeArray();
            sp = newSp;
            state->function = JSTaggedValue(funcTagged);
            state->acc = JSTaggedValue::Hole();
            state->constpool = JSFunction::Cast(funcObject)->GetConstantPool();
            constpool = ConstantPool::Cast(state->constpool.GetTaggedObject());
            state->profileTypeInfo = JSFunction::Cast(funcObject)->GetProfileTypeInfo();
            JSTaggedValue env = JSFunction::Cast(funcObject)->GetLexicalEnv();
            state->env = env;
            thread->SetCurrentSPFrame(newSp);
            LOG(DEBUG, INTERPRETER) << "Entry: Runtime Call " << std::hex << reinterpret_cast<uintptr_t>(sp) << " "
                                    << std::hex << reinterpret_cast<uintptr_t>(pc);
            DISPATCH_OFFSET(0);
        }
    }
    HANDLE_OPCODE(HANDLE_RETURN_DYN) {
        LOG_INST() << "return.dyn";
        InterpretedFrame *state = GET_FRAME(sp);
        LOG(DEBUG, INTERPRETER) << "Exit: Runtime Call " << std::hex << reinterpret_cast<uintptr_t>(sp) << " "
                                << std::hex << reinterpret_cast<uintptr_t>(state->pc);
        JSMethod *method = JSFunction::Cast(state->function.GetTaggedObject())->GetMethod();
        [[maybe_unused]] auto fistPC = method->GetBytecodeArray();
        UPDATE_HOTNESS_COUNTER(-(pc - fistPC));
        JSTaggedType *currentSp = sp;
        sp = state->base.prev;
        ASSERT(sp != nullptr);
        InterpretedFrame *prevState = GET_FRAME(sp);
        pc = prevState->pc;
        // entry frame
        if (FrameHandler::IsEntryFrame(pc)) {
            state->acc = acc;
            return;
        }

        thread->SetCurrentSPFrame(sp);

        constpool = ConstantPool::Cast(prevState->constpool.GetTaggedObject());

        if (IsFastNewFrameExit(currentSp)) {
            JSFunction *func = JSFunction::Cast(GetThisFunction(currentSp).GetTaggedObject());
            if (acc.IsECMAObject()) {
                INTERPRETER_HANDLE_RETURN();
            }

            if (func->IsBase()) {
                JSTaggedValue thisObject = GetThisObjectFromFastNewFrame(currentSp);
                SET_ACC(thisObject);
                INTERPRETER_HANDLE_RETURN();
            }

            if (!acc.IsUndefined()) {
                {
                    [[maybe_unused]] EcmaHandleScope handleScope(thread);
                    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR,
                        "Derived constructor must return object or undefined");
                    thread->SetException(error.GetTaggedValue());
                }
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }

            JSTaggedValue thisObject = GetThisObjectFromFastNewFrame(currentSp);
            SET_ACC(thisObject);
            INTERPRETER_HANDLE_RETURN();
        }

        INTERPRETER_HANDLE_RETURN();
    }
    HANDLE_OPCODE(HANDLE_RETURNUNDEFINED_PREF) {
        LOG_INST() << "return.undefined";
        InterpretedFrame *state = GET_FRAME(sp);
        LOG(DEBUG, INTERPRETER) << "Exit: Runtime Call " << std::hex << reinterpret_cast<uintptr_t>(sp) << " "
                                << std::hex << reinterpret_cast<uintptr_t>(state->pc);
        JSMethod *method = JSFunction::Cast(state->function.GetTaggedObject())->GetMethod();
        [[maybe_unused]] auto fistPC = method->GetBytecodeArray();
        UPDATE_HOTNESS_COUNTER_NON_ACC(-(pc - fistPC));
        JSTaggedType *currentSp = sp;
        sp = state->base.prev;
        ASSERT(sp != nullptr);
        InterpretedFrame *prevState = GET_FRAME(sp);
        pc = prevState->pc;
        // entry frame
        if (FrameHandler::IsEntryFrame(pc)) {
            state->acc = JSTaggedValue::Undefined();
            return;
        }

        thread->SetCurrentSPFrame(sp);

        constpool = ConstantPool::Cast(prevState->constpool.GetTaggedObject());

        if (IsFastNewFrameExit(currentSp)) {
            JSFunction *func = JSFunction::Cast(GetThisFunction(currentSp).GetTaggedObject());
            if (func->IsBase()) {
                JSTaggedValue thisObject = GetThisObjectFromFastNewFrame(currentSp);
                SET_ACC(thisObject);
                INTERPRETER_HANDLE_RETURN();
            }

            if (!acc.IsUndefined()) {
                {
                    [[maybe_unused]] EcmaHandleScope handleScope(thread);
                    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR,
                        "Derived constructor must return object or undefined");
                    thread->SetException(error.GetTaggedValue());
                }
                INTERPRETER_GOTO_EXCEPTION_HANDLER();
            }

            JSTaggedValue thisObject = GetThisObjectFromFastNewFrame(currentSp);
            SET_ACC(thisObject);
            INTERPRETER_HANDLE_RETURN();
        } else {
            SET_ACC(JSTaggedValue::Undefined());
        }
        INTERPRETER_HANDLE_RETURN();
    }
    HANDLE_OPCODE(HANDLE_LDNAN_PREF) {
        LOG_INST() << "intrinsics::ldnan";
        SET_ACC(JSTaggedValue(base::NAN_VALUE));
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_LDINFINITY_PREF) {
        LOG_INST() << "intrinsics::ldinfinity";
        SET_ACC(JSTaggedValue(base::POSITIVE_INFINITY));
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_LDGLOBALTHIS_PREF) {
        LOG_INST() << "intrinsics::ldglobalthis";
        SET_ACC(globalObj)
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_LDUNDEFINED_PREF) {
        LOG_INST() << "intrinsics::ldundefined";
        SET_ACC(JSTaggedValue::Undefined())
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_LDNULL_PREF) {
        LOG_INST() << "intrinsics::ldnull";
        SET_ACC(JSTaggedValue::Null())
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_LDSYMBOL_PREF) {
        LOG_INST() << "intrinsics::ldsymbol";
        SET_ACC(globalEnv->GetSymbolFunction().GetTaggedValue());
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_LDGLOBAL_PREF) {
        LOG_INST() << "intrinsics::ldglobal";
        SET_ACC(globalObj)
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_LDTRUE_PREF) {
        LOG_INST() << "intrinsics::ldtrue";
        SET_ACC(JSTaggedValue::True())
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_LDFALSE_PREF) {
        LOG_INST() << "intrinsics::ldfalse";
        SET_ACC(JSTaggedValue::False())
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_LDLEXENVDYN_PREF) {
        LOG_INST() << "intrinsics::ldlexenvDyn ";
        InterpretedFrame *state = GET_FRAME(sp);
        JSTaggedValue currentLexenv = state->env;
        SET_ACC(currentLexenv);
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_GETUNMAPPEDARGS_PREF) {
        LOG_INST() << "intrinsics::getunmappedargs";

        uint32_t startIdx = 0;
        uint32_t actualNumArgs = GetNumArgs(sp, 0, startIdx);

        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::GetUnmapedArgs(thread, sp, actualNumArgs, startIdx);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_ASYNCFUNCTIONENTER_PREF) {
        LOG_INST() << "intrinsics::asyncfunctionenter";
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::AsyncFunctionEnter(thread);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_TONUMBER_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();

        LOG_INST() << "intrinsics::tonumber"
                   << " v" << v0;
        JSTaggedValue value = GET_VREG_VALUE(v0);
        if (value.IsNumber() || value.IsBigInt()) {
            // fast path
            SET_ACC(value);
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::ToNumber(thread, value);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_NEGDYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();
        LOG_INST() << "intrinsics::negdyn"
                   << " v" << v0;
        JSTaggedValue value = GET_VREG_VALUE(v0);
        // fast path
        if (value.IsInt()) {
            if (value.GetInt() == 0) {
                SET_ACC(JSTaggedValue(-0.0));
            } else {
                SET_ACC(JSTaggedValue(-value.GetInt()));
            }
        } else if (value.IsDouble()) {
            SET_ACC(JSTaggedValue(-value.GetDouble()));
        } else {  // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::NegDyn(thread, value);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_NOTDYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();

        LOG_INST() << "intrinsics::notdyn"
                   << " v" << v0;
        JSTaggedValue value = GET_VREG_VALUE(v0);
        int32_t number;
        // number, fast path
        if (value.IsInt()) {
            number = static_cast<int32_t>(value.GetInt());
            SET_ACC(JSTaggedValue(~number));  // NOLINT(hicpp-signed-bitwise)
        } else if (value.IsDouble()) {
            number = base::NumberHelper::DoubleToInt(value.GetDouble(), base::INT32_BITS);
            SET_ACC(JSTaggedValue(~number));  // NOLINT(hicpp-signed-bitwise)
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::NotDyn(thread, value);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_INCDYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();

        LOG_INST() << "intrinsics::incdyn"
                   << " v" << v0;

        JSTaggedValue value = GET_VREG_VALUE(v0);
        // number fast path
        if (value.IsInt()) {
            int32_t a0 = value.GetInt();
            if (UNLIKELY(a0 == INT32_MAX)) {
                auto ret = static_cast<double>(a0) + 1.0;
                SET_ACC(JSTaggedValue(ret))
            } else {
                SET_ACC(JSTaggedValue(a0 + 1))
            }
        } else if (value.IsDouble()) {
            SET_ACC(JSTaggedValue(value.GetDouble() + 1.0))
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::IncDyn(thread, value);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_DECDYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();
        LOG_INST() << "intrinsics::decdyn"
                   << " v" << v0;

        JSTaggedValue value = GET_VREG_VALUE(v0);
        // number, fast path
        if (value.IsInt()) {
            int32_t a0 = value.GetInt();
            if (UNLIKELY(a0 == INT32_MIN)) {
                auto ret = static_cast<double>(a0) - 1.0;
                SET_ACC(JSTaggedValue(ret))
            } else {
                SET_ACC(JSTaggedValue(a0 - 1))
            }
        } else if (value.IsDouble()) {
            SET_ACC(JSTaggedValue(value.GetDouble() - 1.0))
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::DecDyn(thread, value);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_THROWDYN_PREF) {
        LOG_INST() << "intrinsics::throwdyn";
        SAVE_PC();
        SlowRuntimeStub::ThrowDyn(thread, GET_ACC());
        INTERPRETER_GOTO_EXCEPTION_HANDLER();
    }
    HANDLE_OPCODE(HANDLE_TYPEOFDYN_PREF) {
        LOG_INST() << "intrinsics::typeofdyn";
        JSTaggedValue res = FastRuntimeStub::FastTypeOf(thread, GET_ACC());
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_GETPROPITERATOR_PREF) {
        LOG_INST() << "intrinsics::getpropiterator";
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::GetPropIterator(thread, GET_ACC());
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_RESUMEGENERATOR_PREF_V8) {
        LOG_INST() << "intrinsics::resumegenerator";
        uint16_t vs = READ_INST_8_1();
        JSGeneratorObject *obj = JSGeneratorObject::Cast(GET_VREG_VALUE(vs).GetTaggedObject());
        SET_ACC(obj->GetResumeResult());
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_GETRESUMEMODE_PREF_V8) {
        LOG_INST() << "intrinsics::getresumemode";
        uint16_t vs = READ_INST_8_1();
        JSGeneratorObject *obj = JSGeneratorObject::Cast(GET_VREG_VALUE(vs).GetTaggedObject());
        SET_ACC(JSTaggedValue(static_cast<int>(obj->GetResumeMode())));
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_GETITERATOR_PREF) {
        LOG_INST() << "intrinsics::getiterator";
        JSTaggedValue obj = GET_ACC();

        // fast path: Generator obj is already store in acc
        if (!obj.IsGeneratorObject()) {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::GetIterator(thread, obj);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_THROWCONSTASSIGNMENT_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();
        LOG_INST() << "throwconstassignment"
                   << " v" << v0;
        SAVE_PC();
        SlowRuntimeStub::ThrowConstAssignment(thread, GET_VREG_VALUE(v0));
        INTERPRETER_GOTO_EXCEPTION_HANDLER();
    }
    HANDLE_OPCODE(HANDLE_THROWTHROWNOTEXISTS_PREF) {
        LOG_INST() << "throwthrownotexists";

        SAVE_PC();
        SlowRuntimeStub::ThrowThrowNotExists(thread);
        INTERPRETER_GOTO_EXCEPTION_HANDLER();
    }
    HANDLE_OPCODE(HANDLE_THROWPATTERNNONCOERCIBLE_PREF) {
        LOG_INST() << "throwpatternnoncoercible";

        SAVE_PC();
        SlowRuntimeStub::ThrowPatternNonCoercible(thread);
        INTERPRETER_GOTO_EXCEPTION_HANDLER();
    }
    HANDLE_OPCODE(HANDLE_THROWIFNOTOBJECT_PREF_V8) {
        LOG_INST() << "throwifnotobject";
        uint16_t v0 = READ_INST_8_1();

        JSTaggedValue value = GET_VREG_VALUE(v0);
        // fast path
        if (value.IsECMAObject()) {
            DISPATCH(BytecodeInstruction::Format::PREF_V8);
        }

        // slow path
        SAVE_PC();
        SlowRuntimeStub::ThrowIfNotObject(thread);
        INTERPRETER_GOTO_EXCEPTION_HANDLER();
    }
    HANDLE_OPCODE(HANDLE_ITERNEXT_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();
        LOG_INST() << "intrinsics::iternext"
                   << " v" << v0;
        SAVE_PC();
        JSTaggedValue iter = GET_VREG_VALUE(v0);
        JSTaggedValue res = SlowRuntimeStub::IterNext(thread, iter);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_CLOSEITERATOR_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();
        LOG_INST() << "intrinsics::closeiterator"
                   << " v" << v0;
        SAVE_PC();
        JSTaggedValue iter = GET_VREG_VALUE(v0);
        JSTaggedValue res = SlowRuntimeStub::CloseIterator(thread, iter);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_ADD2DYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();
        LOG_INST() << "intrinsics::add2dyn"
                   << " v" << v0;
        int32_t a0;
        int32_t a1;
        JSTaggedValue left = GET_VREG_VALUE(v0);
        JSTaggedValue right = GET_ACC();
        // number, fast path
        if (left.IsInt() && right.IsInt()) {
            a0 = left.GetInt();
            a1 = right.GetInt();
            if ((a0 > 0 && a1 > INT32_MAX - a0) || (a0 < 0 && a1 < INT32_MIN - a0)) {
                auto ret = static_cast<double>(a0) + static_cast<double>(a1);
                SET_ACC(JSTaggedValue(ret))
            } else {
                SET_ACC(JSTaggedValue(a0 + a1))
            }
        } else if (left.IsNumber() && right.IsNumber()) {
            double a0Double = left.IsInt() ? left.GetInt() : left.GetDouble();
            double a1Double = right.IsInt() ? right.GetInt() : right.GetDouble();
            double ret = a0Double + a1Double;
            SET_ACC(JSTaggedValue(ret))
        } else {
            // one or both are not number, slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::Add2Dyn(thread, left, right);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_SUB2DYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();
        LOG_INST() << "intrinsics::sub2dyn"
                   << " v" << v0;
        int32_t a0;
        int32_t a1;
        JSTaggedValue left = GET_VREG_VALUE(v0);
        JSTaggedValue right = GET_ACC();
        if (left.IsInt() && right.IsInt()) {
            a0 = left.GetInt();
            a1 = -right.GetInt();
            if ((a0 > 0 && a1 > INT32_MAX - a0) || (a0 < 0 && a1 < INT32_MIN - a0)) {
                auto ret = static_cast<double>(a0) + static_cast<double>(a1);
                SET_ACC(JSTaggedValue(ret))
            } else {
                SET_ACC(JSTaggedValue(a0 + a1))
            }
        } else if (left.IsNumber() && right.IsNumber()) {
            double a0Double = left.IsInt() ? left.GetInt() : left.GetDouble();
            double a1Double = right.IsInt() ? right.GetInt() : right.GetDouble();
            double ret = a0Double - a1Double;
            SET_ACC(JSTaggedValue(ret))
        } else {
            // one or both are not number, slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::Sub2Dyn(thread, left, right);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_MUL2DYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();
        LOG_INST() << "intrinsics::mul2dyn"
                   << " v" << v0;
        JSTaggedValue left = GET_VREG_VALUE(v0);
        JSTaggedValue right = acc;
        JSTaggedValue value = FastRuntimeStub::FastMul(left, right);
        if (!value.IsHole()) {
            SET_ACC(value);
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::Mul2Dyn(thread, left, right);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_DIV2DYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();
        LOG_INST() << "intrinsics::div2dyn"
                   << " v" << v0;
        JSTaggedValue left = GET_VREG_VALUE(v0);
        JSTaggedValue right = acc;
        // fast path
        JSTaggedValue res = FastRuntimeStub::FastDiv(left, right);
        if (!res.IsHole()) {
            SET_ACC(res);
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue slowRes = SlowRuntimeStub::Div2Dyn(thread, left, right);
            INTERPRETER_RETURN_IF_ABRUPT(slowRes);
            SET_ACC(slowRes);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_MOD2DYN_PREF_V8) {
        uint16_t vs = READ_INST_8_1();
        LOG_INST() << "intrinsics::mod2dyn"
                   << " v" << vs;
        JSTaggedValue left = GET_VREG_VALUE(vs);
        JSTaggedValue right = GET_ACC();
        JSTaggedValue res = FastRuntimeStub::FastMod(left, right);
        if (!res.IsHole()) {
            SET_ACC(res);
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue slowRes = SlowRuntimeStub::Mod2Dyn(thread, left, right);
            INTERPRETER_RETURN_IF_ABRUPT(slowRes);
            SET_ACC(slowRes);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_EQDYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();

        LOG_INST() << "intrinsics::eqdyn"
                   << " v" << v0;
        JSTaggedValue left = GET_VREG_VALUE(v0);
        JSTaggedValue right = acc;
        JSTaggedValue res = FastRuntimeStub::FastEqual(left, right);
        if (!res.IsHole()) {
            SET_ACC(res);
        } else {
            // slow path
            SAVE_PC();
            res = SlowRuntimeStub::EqDyn(thread, left, right);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }

        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_NOTEQDYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();

        LOG_INST() << "intrinsics::noteqdyn"
                   << " v" << v0;
        JSTaggedValue left = GET_VREG_VALUE(v0);
        JSTaggedValue right = acc;

        JSTaggedValue res = FastRuntimeStub::FastEqual(left, right);
        if (!res.IsHole()) {
            res = res.IsTrue() ? JSTaggedValue::False() : JSTaggedValue::True();
            SET_ACC(res);
        } else {
            // slow path
            SAVE_PC();
            res = SlowRuntimeStub::NotEqDyn(thread, left, right);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_LESSDYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();

        LOG_INST() << "intrinsics::lessdyn"
                   << " v" << v0;
        JSTaggedValue left = GET_VREG_VALUE(v0);
        JSTaggedValue right = GET_ACC();
        if (left.IsNumber() && right.IsNumber()) {
            // fast path
            double valueA = left.IsInt() ? static_cast<double>(left.GetInt()) : left.GetDouble();
            double valueB = right.IsInt() ? static_cast<double>(right.GetInt()) : right.GetDouble();
            bool ret = JSTaggedValue::StrictNumberCompare(valueA, valueB) == ComparisonResult::LESS;
            SET_ACC(ret ? JSTaggedValue::True() : JSTaggedValue::False())
        } else if (left.IsBigInt() && right.IsBigInt()) {
            bool result = BigInt::LessThan(left, right);
            SET_ACC(JSTaggedValue(result));
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::LessDyn(thread, left, right);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_LESSEQDYN_PREF_V8) {
        uint16_t vs = READ_INST_8_1();
        LOG_INST() << "intrinsics::lesseqdyn "
                   << " v" << vs;
        JSTaggedValue left = GET_VREG_VALUE(vs);
        JSTaggedValue right = GET_ACC();
        if (left.IsNumber() && right.IsNumber()) {
            // fast path
            double valueA = left.IsInt() ? static_cast<double>(left.GetInt()) : left.GetDouble();
            double valueB = right.IsInt() ? static_cast<double>(right.GetInt()) : right.GetDouble();
            bool ret = JSTaggedValue::StrictNumberCompare(valueA, valueB) <= ComparisonResult::EQUAL;
            SET_ACC(ret ? JSTaggedValue::True() : JSTaggedValue::False())
        } else if (left.IsBigInt() && right.IsBigInt()) {
            bool result = BigInt::LessThan(left, right) || BigInt::Equal(left, right);
            SET_ACC(JSTaggedValue(result));
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::LessEqDyn(thread, left, right);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_GREATERDYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();

        LOG_INST() << "intrinsics::greaterdyn"
                   << " v" << v0;
        JSTaggedValue left = GET_VREG_VALUE(v0);
        JSTaggedValue right = acc;
        if (left.IsNumber() && right.IsNumber()) {
            // fast path
            double valueA = left.IsInt() ? static_cast<double>(left.GetInt()) : left.GetDouble();
            double valueB = right.IsInt() ? static_cast<double>(right.GetInt()) : right.GetDouble();
            bool ret = JSTaggedValue::StrictNumberCompare(valueA, valueB) == ComparisonResult::GREAT;
            SET_ACC(ret ? JSTaggedValue::True() : JSTaggedValue::False())
        } else if (left.IsBigInt() && right.IsBigInt()) {
            bool result = BigInt::LessThan(right, left);
            SET_ACC(JSTaggedValue(result));
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::GreaterDyn(thread, left, right);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_GREATEREQDYN_PREF_V8) {
        uint16_t vs = READ_INST_8_1();
        LOG_INST() << "intrinsics::greateqdyn "
                   << " v" << vs;
        JSTaggedValue left = GET_VREG_VALUE(vs);
        JSTaggedValue right = GET_ACC();
        if (left.IsNumber() && right.IsNumber()) {
            // fast path
            double valueA = left.IsInt() ? static_cast<double>(left.GetInt()) : left.GetDouble();
            double valueB = right.IsInt() ? static_cast<double>(right.GetInt()) : right.GetDouble();
            ComparisonResult comparison = JSTaggedValue::StrictNumberCompare(valueA, valueB);
            bool ret = (comparison == ComparisonResult::GREAT) || (comparison == ComparisonResult::EQUAL);
            SET_ACC(ret ? JSTaggedValue::True() : JSTaggedValue::False())
        } else if (left.IsBigInt() && right.IsBigInt()) {
            bool result = BigInt::LessThan(right, left) || BigInt::Equal(right, left);
            SET_ACC(JSTaggedValue(result))
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::GreaterEqDyn(thread, left, right);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_SHL2DYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();

        LOG_INST() << "intrinsics::shl2dyn"
                   << " v" << v0;
        JSTaggedValue left = GET_VREG_VALUE(v0);
        JSTaggedValue right = GET_ACC();
        // both number, fast path
        if (left.IsInt() && right.IsInt()) {
            int32_t opNumber0 = left.GetInt();
            int32_t opNumber1 = right.GetInt();
            uint32_t shift =
                static_cast<uint32_t>(opNumber1) & 0x1f; // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
            using unsigned_type = std::make_unsigned_t<int32_t>;
            auto ret =
                static_cast<int32_t>(static_cast<unsigned_type>(opNumber0) << shift); // NOLINT(hicpp-signed-bitwise)
            SET_ACC(JSTaggedValue(ret))
        } else if (left.IsNumber() && right.IsNumber()) {
            int32_t opNumber0 =
                left.IsInt() ? left.GetInt() : base::NumberHelper::DoubleToInt(left.GetDouble(), base::INT32_BITS);
            int32_t opNumber1 =
                right.IsInt() ? right.GetInt() : base::NumberHelper::DoubleToInt(right.GetDouble(), base::INT32_BITS);
            uint32_t shift =
                static_cast<uint32_t>(opNumber1) & 0x1f; // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
            using unsigned_type = std::make_unsigned_t<int32_t>;
            auto ret =
                static_cast<int32_t>(static_cast<unsigned_type>(opNumber0) << shift); // NOLINT(hicpp-signed-bitwise)
            SET_ACC(JSTaggedValue(ret))
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::Shl2Dyn(thread, left, right);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_SHR2DYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();

        LOG_INST() << "intrinsics::shr2dyn"
                   << " v" << v0;
        JSTaggedValue left = GET_VREG_VALUE(v0);
        JSTaggedValue right = GET_ACC();
        // both number, fast path
        if (left.IsInt() && right.IsInt()) {
            int32_t opNumber0 = left.GetInt();
            int32_t opNumber1 = right.GetInt();
            uint32_t shift =
                static_cast<uint32_t>(opNumber1) & 0x1f; // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
            using unsigned_type = std::make_unsigned_t<uint32_t>;
            auto ret =
                static_cast<uint32_t>(static_cast<unsigned_type>(opNumber0) >> shift); // NOLINT(hicpp-signed-bitwise)
            SET_ACC(JSTaggedValue(ret))
        } else if (left.IsNumber() && right.IsNumber()) {
            int32_t opNumber0 =
                left.IsInt() ? left.GetInt() : base::NumberHelper::DoubleToInt(left.GetDouble(), base::INT32_BITS);
            int32_t opNumber1 =
                right.IsInt() ? right.GetInt() : base::NumberHelper::DoubleToInt(right.GetDouble(), base::INT32_BITS);
            uint32_t shift =
                static_cast<uint32_t>(opNumber1) & 0x1f; // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
            using unsigned_type = std::make_unsigned_t<uint32_t>;
            auto ret =
                static_cast<uint32_t>(static_cast<unsigned_type>(opNumber0) >> shift); // NOLINT(hicpp-signed-bitwise)
            SET_ACC(JSTaggedValue(ret))
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::Shr2Dyn(thread, left, right);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_ASHR2DYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();

        LOG_INST() << "intrinsics::ashr2dyn"
                   << " v" << v0;
        JSTaggedValue left = GET_VREG_VALUE(v0);
        JSTaggedValue right = GET_ACC();
        // both number, fast path
        if (left.IsInt() && right.IsInt()) {
            int32_t opNumber0 = left.GetInt();
            int32_t opNumber1 = right.GetInt();
            uint32_t shift =
                static_cast<uint32_t>(opNumber1) & 0x1f; // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
            auto ret = static_cast<int32_t>(opNumber0 >> shift); // NOLINT(hicpp-signed-bitwise)
            SET_ACC(JSTaggedValue(ret))
        } else if (left.IsNumber() && right.IsNumber()) {
            int32_t opNumber0 =
                left.IsInt() ? left.GetInt() : base::NumberHelper::DoubleToInt(left.GetDouble(), base::INT32_BITS);
            int32_t opNumber1 =
                right.IsInt() ? right.GetInt() : base::NumberHelper::DoubleToInt(right.GetDouble(), base::INT32_BITS);
            uint32_t shift =
                static_cast<uint32_t>(opNumber1) & 0x1f; // NOLINT(hicpp-signed-bitwise, readability-magic-numbers)
            auto ret = static_cast<int32_t>(opNumber0 >> shift); // NOLINT(hicpp-signed-bitwise)
            SET_ACC(JSTaggedValue(ret))
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::Ashr2Dyn(thread, left, right);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_AND2DYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();

        LOG_INST() << "intrinsics::and2dyn"
                   << " v" << v0;
        JSTaggedValue left = GET_VREG_VALUE(v0);
        JSTaggedValue right = GET_ACC();
        // both number, fast path
        if (left.IsInt() && right.IsInt()) {
            int32_t opNumber0 = left.GetInt();
            int32_t opNumber1 = right.GetInt();
            // NOLINT(hicpp-signed-bitwise)
            auto ret = static_cast<uint32_t>(opNumber0) & static_cast<uint32_t>(opNumber1);
            SET_ACC(JSTaggedValue(static_cast<int32_t>(ret)))
        } else if (left.IsNumber() && right.IsNumber()) {
            int32_t opNumber0 =
                left.IsInt() ? left.GetInt() : base::NumberHelper::DoubleToInt(left.GetDouble(), base::INT32_BITS);
            int32_t opNumber1 =
                right.IsInt() ? right.GetInt() : base::NumberHelper::DoubleToInt(right.GetDouble(), base::INT32_BITS);
            // NOLINT(hicpp-signed-bitwise)
            auto ret = static_cast<uint32_t>(opNumber0) & static_cast<uint32_t>(opNumber1);
            SET_ACC(JSTaggedValue(static_cast<int32_t>(ret)))
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::And2Dyn(thread, left, right);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_OR2DYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();

        LOG_INST() << "intrinsics::or2dyn"
                   << " v" << v0;
        JSTaggedValue left = GET_VREG_VALUE(v0);
        JSTaggedValue right = GET_ACC();
        // both number, fast path
        if (left.IsInt() && right.IsInt()) {
            int32_t opNumber0 = left.GetInt();
            int32_t opNumber1 = right.GetInt();
            // NOLINT(hicpp-signed-bitwise)
            auto ret = static_cast<uint32_t>(opNumber0) | static_cast<uint32_t>(opNumber1);
            SET_ACC(JSTaggedValue(static_cast<int32_t>(ret)))
        } else if (left.IsNumber() && right.IsNumber()) {
            int32_t opNumber0 =
                left.IsInt() ? left.GetInt() : base::NumberHelper::DoubleToInt(left.GetDouble(), base::INT32_BITS);
            int32_t opNumber1 =
                right.IsInt() ? right.GetInt() : base::NumberHelper::DoubleToInt(right.GetDouble(), base::INT32_BITS);
            // NOLINT(hicpp-signed-bitwise)
            auto ret = static_cast<uint32_t>(opNumber0) | static_cast<uint32_t>(opNumber1);
            SET_ACC(JSTaggedValue(static_cast<int32_t>(ret)))
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::Or2Dyn(thread, left, right);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_XOR2DYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();

        LOG_INST() << "intrinsics::xor2dyn"
                   << " v" << v0;
        JSTaggedValue left = GET_VREG_VALUE(v0);
        JSTaggedValue right = GET_ACC();
        // both number, fast path
        if (left.IsInt() && right.IsInt()) {
            int32_t opNumber0 = left.GetInt();
            int32_t opNumber1 = right.GetInt();
            // NOLINT(hicpp-signed-bitwise)
            auto ret = static_cast<uint32_t>(opNumber0) ^ static_cast<uint32_t>(opNumber1);
            SET_ACC(JSTaggedValue(static_cast<int32_t>(ret)))
        } else if (left.IsNumber() && right.IsNumber()) {
            int32_t opNumber0 =
                left.IsInt() ? left.GetInt() : base::NumberHelper::DoubleToInt(left.GetDouble(), base::INT32_BITS);
            int32_t opNumber1 =
                right.IsInt() ? right.GetInt() : base::NumberHelper::DoubleToInt(right.GetDouble(), base::INT32_BITS);
            // NOLINT(hicpp-signed-bitwise)
            auto ret = static_cast<uint32_t>(opNumber0) ^ static_cast<uint32_t>(opNumber1);
            SET_ACC(JSTaggedValue(static_cast<int32_t>(ret)))
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::Xor2Dyn(thread, left, right);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_DELOBJPROP_PREF_V8_V8) {
        uint16_t v0 = READ_INST_8_1();
        uint16_t v1 = READ_INST_8_2();
        LOG_INST() << "intrinsics::delobjprop"
                   << " v0" << v0 << " v1" << v1;

        JSTaggedValue obj = GET_VREG_VALUE(v0);
        JSTaggedValue prop = GET_VREG_VALUE(v1);
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::DelObjProp(thread, obj, prop);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);

        DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_DEFINEFUNCDYN_PREF_ID16_IMM16_V8) {
        uint16_t methodId = READ_INST_16_1();
        uint16_t length = READ_INST_16_3();
        uint16_t v0 = READ_INST_8_5();
        LOG_INST() << "intrinsics::definefuncDyn length: " << length
                   << " v" << v0;
        JSFunction *result = JSFunction::Cast(constpool->GetObjectFromCache(methodId).GetTaggedObject());
        ASSERT(result != nullptr);
        if (result->GetResolved()) {
            SAVE_PC();
            auto res = SlowRuntimeStub::DefinefuncDyn(thread, result);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            result = JSFunction::Cast(res.GetTaggedObject());
            result->SetConstantPool(thread, JSTaggedValue(constpool));
        } else {
            result->SetResolved(true);
        }

        result->SetPropertyInlinedProps(thread, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, JSTaggedValue(length));
        JSTaggedValue envHandle = GET_VREG_VALUE(v0);
        result->SetLexicalEnv(thread, envHandle);

        JSFunction *currentFunc = JSFunction::Cast((GET_FRAME(sp)->function).GetTaggedObject());
        result->SetModule(thread, currentFunc->GetModule());
        SET_ACC(JSTaggedValue(result))

        DISPATCH(BytecodeInstruction::Format::PREF_ID16_IMM16_V8);
    }
    HANDLE_OPCODE(HANDLE_DEFINENCFUNCDYN_PREF_ID16_IMM16_V8) {
        uint16_t methodId = READ_INST_16_1();
        uint16_t length = READ_INST_16_3();
        uint16_t v0 = READ_INST_8_5();
        JSTaggedValue homeObject = GET_ACC();
        LOG_INST() << "intrinsics::definencfuncDyn length: " << length
                   << " v" << v0;
        JSFunction *result = JSFunction::Cast(constpool->GetObjectFromCache(methodId).GetTaggedObject());
        ASSERT(result != nullptr);
        if (result->GetResolved()) {
            SAVE_ACC();
            SAVE_PC();
            auto res = SlowRuntimeStub::DefineNCFuncDyn(thread, result);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            result = JSFunction::Cast(res.GetTaggedObject());
            result->SetConstantPool(thread, JSTaggedValue(constpool));
            RESTORE_ACC();
            homeObject = GET_ACC();
        } else {
            result->SetResolved(true);
        }

        result->SetPropertyInlinedProps(thread, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, JSTaggedValue(length));
        JSTaggedValue env = GET_VREG_VALUE(v0);
        result->SetLexicalEnv(thread, env);
        result->SetHomeObject(thread, homeObject);

        JSFunction *currentFunc = JSFunction::Cast((GET_FRAME(sp)->function).GetTaggedObject());
        result->SetModule(thread, currentFunc->GetModule());
        SET_ACC(JSTaggedValue(result));

        DISPATCH(BytecodeInstruction::Format::PREF_ID16_IMM16_V8);
    }
    HANDLE_OPCODE(HANDLE_DEFINEMETHOD_PREF_ID16_IMM16_V8) {
        uint16_t methodId = READ_INST_16_1();
        uint16_t length = READ_INST_16_3();
        uint16_t v0 = READ_INST_8_5();
        JSTaggedValue homeObject = GET_ACC();
        LOG_INST() << "intrinsics::definemethod length: " << length
                   << " v" << v0;
        JSFunction *result = JSFunction::Cast(constpool->GetObjectFromCache(methodId).GetTaggedObject());
        ASSERT(result != nullptr);
        if (result->GetResolved()) {
            SAVE_PC();
            auto res = SlowRuntimeStub::DefineMethod(thread, result, homeObject);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            result = JSFunction::Cast(res.GetTaggedObject());
            result->SetConstantPool(thread, JSTaggedValue(constpool));
        } else {
            result->SetHomeObject(thread, homeObject);
            result->SetResolved(true);
        }

        result->SetPropertyInlinedProps(thread, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, JSTaggedValue(length));
        JSTaggedValue taggedCurEnv = GET_VREG_VALUE(v0);
        result->SetLexicalEnv(thread, taggedCurEnv);

        JSFunction *currentFunc = JSFunction::Cast((GET_FRAME(sp)->function).GetTaggedObject());
        result->SetModule(thread, currentFunc->GetModule());
        SET_ACC(JSTaggedValue(result));

        DISPATCH(BytecodeInstruction::Format::PREF_ID16_IMM16_V8);
    }
    HANDLE_OPCODE(HANDLE_NEWOBJDYNRANGE_PREF_IMM16_V8) {
        uint16_t numArgs = READ_INST_16_1();
        uint16_t firstArgRegIdx = READ_INST_8_3();
        LOG_INST() << "intrinsics::newobjDynrange " << numArgs << " v" << firstArgRegIdx;
        JSTaggedValue ctor = GET_VREG_VALUE(firstArgRegIdx);

        if (ctor.IsJSFunction() && ctor.IsConstructor()) {
            JSFunction *ctorFunc = JSFunction::Cast(ctor.GetTaggedObject());
            JSMethod *ctorMethod = ctorFunc->GetMethod();
            if (ctorFunc->IsBuiltinConstructor()) {
                ASSERT(ctorMethod->GetNumVregsWithCallField() == 0);
                size_t frameSize = INTERPRETER_FRAME_STATE_SIZE + numArgs + 1;  // +1 for this
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                JSTaggedType *newSp = sp - frameSize;
                if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
                    INTERPRETER_GOTO_EXCEPTION_HANDLER();
                }
                EcmaRuntimeCallInfo ecmaRuntimeCallInfo(thread, numArgs + 1 - NUM_MANDATORY_JSFUNC_ARGS, newSp);
                // copy args
                uint32_t index = 0;
                // func
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = GET_VREG(firstArgRegIdx);
                // newTarget
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = GET_VREG(firstArgRegIdx + 1);
                // this
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
                for (size_t i = 2; i < numArgs; ++i) {  // 2: func and newTarget
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    newSp[index++] = GET_VREG(firstArgRegIdx + i);
                }

                InterpretedFrame *state = GET_FRAME(newSp);
                state->base.prev = sp;
                state->base.type = FrameType::INTERPRETER_FRAME;
                state->pc = nullptr;
                state->function = ctor;
                thread->SetCurrentSPFrame(newSp);

                LOG(DEBUG, INTERPRETER) << "Entry: Runtime New.";
                JSTaggedValue retValue = reinterpret_cast<EcmaEntrypoint>(
                    const_cast<void *>(ctorMethod->GetNativePointer()))(&ecmaRuntimeCallInfo);
                thread->SetCurrentSPFrame(sp);
                if (UNLIKELY(thread->HasPendingException())) {
                    INTERPRETER_GOTO_EXCEPTION_HANDLER();
                }
                LOG(DEBUG, INTERPRETER) << "Exit: Runtime New.";
                SET_ACC(retValue);
                DISPATCH(BytecodeInstruction::Format::PREF_IMM16_V8);
            }

            if (IsFastNewFrameEnter(ctorFunc, ctorMethod)) {
                SAVE_PC();
                uint32_t numVregs = ctorMethod->GetNumVregsWithCallField();
                uint32_t numDeclaredArgs = ctorFunc->IsBase() ?
                                           ctorMethod->GetNumArgsWithCallField() + 1 :  // +1 for this
                                           ctorMethod->GetNumArgsWithCallField() + 2;   // +2 for newTarget and this
                // +1 for hidden this, explicit this may be overwritten after bc optimizer
                size_t frameSize = INTERPRETER_FRAME_STATE_SIZE + numVregs + numDeclaredArgs + 1;
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                JSTaggedType *newSp = sp - frameSize;
                InterpretedFrame *state = GET_FRAME(newSp);

                if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
                    INTERPRETER_GOTO_EXCEPTION_HANDLER();
                }

                uint32_t index = 0;
                // initialize vregs value
                for (size_t i = 0; i < numVregs; ++i) {
                    newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
                }

                // this
                JSTaggedValue thisObj;
                if (ctorFunc->IsBase()) {
                    JSTaggedValue newTarget = GET_VREG_VALUE(firstArgRegIdx + 1);
                    thisObj = FastRuntimeStub::NewThisObject(thread, ctor, newTarget, state);
                    INTERPRETER_RETURN_IF_ABRUPT(thisObj);
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    newSp[index++] = thisObj.GetRawData();
                } else {
                    ASSERT(ctorFunc->IsDerivedConstructor());
                    JSTaggedValue newTarget = GET_VREG_VALUE(firstArgRegIdx + 1);
                    newSp[index++] = newTarget.GetRawData();
                    thisObj = JSTaggedValue::Undefined();
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    newSp[index++] = thisObj.GetRawData();

                    state->function = ctor;
                    state->constpool = ctorFunc->GetConstantPool();
                    state->profileTypeInfo = ctorFunc->GetProfileTypeInfo();
                    state->env = ctorFunc->GetLexicalEnv();
                }

                // the second condition ensure not push extra args
                for (size_t i = 2; i < numArgs && index < numVregs + numDeclaredArgs; ++i) {  // 2: func and newTarget
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    newSp[index++] = GET_VREG(firstArgRegIdx + i);
                }

                // set undefined to the extra prats of declare
                for (size_t i = index; i < numVregs + numDeclaredArgs; ++i) {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
                }

                // hidden this object
                newSp[index] = thisObj.GetRawData();

                state->base.prev = sp;
                state->base.type = FrameType::INTERPRETER_FAST_NEW_FRAME;
                state->pc = pc = ctorMethod->GetBytecodeArray();
                sp = newSp;
                state->acc = JSTaggedValue::Hole();
                constpool = ConstantPool::Cast(state->constpool.GetTaggedObject());

                thread->SetCurrentSPFrame(newSp);
                LOG(DEBUG, INTERPRETER) << "Entry: Runtime New " << std::hex << reinterpret_cast<uintptr_t>(sp) << " "
                                        << std::hex << reinterpret_cast<uintptr_t>(pc);
                DISPATCH_OFFSET(0);
            }
        }

        // bound function, proxy, other call types, enter slow path
        constexpr uint16_t firstArgOffset = 2;
        JSTaggedValue newTarget = GET_VREG_VALUE(firstArgRegIdx + 1);
        // Exclude func and newTarget
        uint16_t firstArgIdx = firstArgRegIdx + firstArgOffset;
        uint16_t length = numArgs - firstArgOffset;

        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::NewObjDynRange(thread, ctor, newTarget, firstArgIdx, length);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_IMM16_V8);
    }
    HANDLE_OPCODE(HANDLE_EXPDYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();
        LOG_INST() << "intrinsics::expdyn"
                   << " v" << v0;
        JSTaggedValue base = GET_VREG_VALUE(v0);
        JSTaggedValue exponent = GET_ACC();
        if (base.IsNumber() && exponent.IsNumber()) {
            // fast path
            double doubleBase = base.IsInt() ? base.GetInt() : base.GetDouble();
            double doubleExponent = exponent.IsInt() ? exponent.GetInt() : exponent.GetDouble();
            if (std::abs(doubleBase) == 1 && std::isinf(doubleExponent)) {
                SET_ACC(JSTaggedValue(base::NAN_VALUE));
            }
            if ((doubleBase == 0 &&
                ((bit_cast<uint64_t>(doubleBase)) & base::DOUBLE_SIGN_MASK) == base::DOUBLE_SIGN_MASK) &&
                std::isfinite(doubleExponent) && base::NumberHelper::TruncateDouble(doubleExponent) == doubleExponent &&
                base::NumberHelper::TruncateDouble(doubleExponent / 2) + base::HALF ==  // 2 : half
                (doubleExponent / 2)) {  // 2 : half
                if (doubleExponent > 0) {
                    SET_ACC(JSTaggedValue(-0.0));
                }
                if (doubleExponent < 0) {
                    SET_ACC(JSTaggedValue(-base::POSITIVE_INFINITY));
                }
            }
            SET_ACC(JSTaggedValue(std::pow(doubleBase, doubleExponent)));
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::ExpDyn(thread, base, exponent);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_ISINDYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();
        LOG_INST() << "intrinsics::isindyn"
                   << " v" << v0;
        JSTaggedValue prop = GET_VREG_VALUE(v0);
        JSTaggedValue obj = GET_ACC();
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::IsInDyn(thread, prop, obj);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_INSTANCEOFDYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();
        LOG_INST() << "intrinsics::instanceofdyn"
                   << " v" << v0;
        JSTaggedValue obj = GET_VREG_VALUE(v0);
        JSTaggedValue target = GET_ACC();
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::InstanceofDyn(thread, obj, target);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_STRICTNOTEQDYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();
        LOG_INST() << "intrinsics::strictnoteq"
                   << " v" << v0;
        JSTaggedValue left = GET_VREG_VALUE(v0);
        JSTaggedValue right = GET_ACC();
        bool res = FastRuntimeStub::FastStrictEqual(left, right);
        SET_ACC(JSTaggedValue(!res));
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_STRICTEQDYN_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();
        LOG_INST() << "intrinsics::stricteq"
                   << " v" << v0;
        JSTaggedValue left = GET_VREG_VALUE(v0);
        JSTaggedValue right = GET_ACC();
        bool res = FastRuntimeStub::FastStrictEqual(left, right);
        SET_ACC(JSTaggedValue(res));
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_LDLEXVARDYN_PREF_IMM16_IMM16) {
        uint16_t level = READ_INST_16_1();
        uint16_t slot = READ_INST_16_3();

        LOG_INST() << "intrinsics::ldlexvardyn"
                   << " level:" << level << " slot:" << slot;
        InterpretedFrame *state = GET_FRAME(sp);
        JSTaggedValue currentLexenv = state->env;
        JSTaggedValue env(currentLexenv);
        for (uint32_t i = 0; i < level; i++) {
            JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
            ASSERT(!taggedParentEnv.IsUndefined());
            env = taggedParentEnv;
        }
        SET_ACC(LexicalEnv::Cast(env.GetTaggedObject())->GetProperties(slot));
        DISPATCH(BytecodeInstruction::Format::PREF_IMM16_IMM16);
    }
    HANDLE_OPCODE(HANDLE_LDLEXVARDYN_PREF_IMM8_IMM8) {
        uint16_t level = READ_INST_8_1();
        uint16_t slot = READ_INST_8_2();

        LOG_INST() << "intrinsics::ldlexvardyn"
                   << " level:" << level << " slot:" << slot;
        InterpretedFrame *state = GET_FRAME(sp);
        JSTaggedValue currentLexenv = state->env;
        JSTaggedValue env(currentLexenv);
        for (uint32_t i = 0; i < level; i++) {
            JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
            ASSERT(!taggedParentEnv.IsUndefined());
            env = taggedParentEnv;
        }
        SET_ACC(LexicalEnv::Cast(env.GetTaggedObject())->GetProperties(slot));
        DISPATCH(BytecodeInstruction::Format::PREF_IMM8_IMM8);
    }
    HANDLE_OPCODE(HANDLE_LDLEXVARDYN_PREF_IMM4_IMM4) {
        uint16_t level = READ_INST_4_2();
        uint16_t slot = READ_INST_4_3();

        LOG_INST() << "intrinsics::ldlexvardyn"
                   << " level:" << level << " slot:" << slot;
        InterpretedFrame *state = GET_FRAME(sp);
        JSTaggedValue currentLexenv = state->env;
        JSTaggedValue env(currentLexenv);
        for (uint32_t i = 0; i < level; i++) {
            JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
            ASSERT(!taggedParentEnv.IsUndefined());
            env = taggedParentEnv;
        }
        SET_ACC(LexicalEnv::Cast(env.GetTaggedObject())->GetProperties(slot));
        DISPATCH(BytecodeInstruction::Format::PREF_IMM4_IMM4);
    }
    HANDLE_OPCODE(HANDLE_STLEXVARDYN_PREF_IMM16_IMM16_V8) {
        uint16_t level = READ_INST_16_1();
        uint16_t slot = READ_INST_16_3();
        uint16_t v0 = READ_INST_8_5();
        LOG_INST() << "intrinsics::stlexvardyn"
                   << " level:" << level << " slot:" << slot << " v" << v0;

        JSTaggedValue value = GET_VREG_VALUE(v0);
        InterpretedFrame *state = GET_FRAME(sp);
        JSTaggedValue env = state->env;
        for (uint32_t i = 0; i < level; i++) {
            JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
            ASSERT(!taggedParentEnv.IsUndefined());
            env = taggedParentEnv;
        }
        LexicalEnv::Cast(env.GetTaggedObject())->SetProperties(thread, slot, value);

        DISPATCH(BytecodeInstruction::Format::PREF_IMM16_IMM16_V8);
    }
    HANDLE_OPCODE(HANDLE_NEWLEXENVWITHNAMEDYN_PREF_IMM16_IMM16) {
        uint16_t numVars = READ_INST_16_1();
        uint16_t scopeId = READ_INST_16_3();
        LOG_INST() << "intrinsics::newlexenvwithnamedyn"
                   << " numVars " << numVars << " scopeId " << scopeId;

        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::NewLexicalEnvWithNameDyn(thread, numVars, scopeId);
        INTERPRETER_RETURN_IF_ABRUPT(res);

        SET_ACC(res);
        GET_FRAME(sp)->env = res;
        DISPATCH(BytecodeInstruction::Format::PREF_IMM16_IMM16);
    }
    HANDLE_OPCODE(HANDLE_STLEXVARDYN_PREF_IMM8_IMM8_V8) {
        uint16_t level = READ_INST_8_1();
        uint16_t slot = READ_INST_8_2();
        uint16_t v0 = READ_INST_8_3();
        LOG_INST() << "intrinsics::stlexvardyn"
                   << " level:" << level << " slot:" << slot << " v" << v0;

        JSTaggedValue value = GET_VREG_VALUE(v0);
        InterpretedFrame *state = GET_FRAME(sp);
        JSTaggedValue env = state->env;
        for (uint32_t i = 0; i < level; i++) {
            JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
            ASSERT(!taggedParentEnv.IsUndefined());
            env = taggedParentEnv;
        }
        LexicalEnv::Cast(env.GetTaggedObject())->SetProperties(thread, slot, value);

        DISPATCH(BytecodeInstruction::Format::PREF_IMM8_IMM8_V8);
    }
    HANDLE_OPCODE(HANDLE_STLEXVARDYN_PREF_IMM4_IMM4_V8) {
        uint16_t level = READ_INST_4_2();
        uint16_t slot = READ_INST_4_3();
        uint16_t v0 = READ_INST_8_2();
        LOG_INST() << "intrinsics::stlexvardyn"
                   << " level:" << level << " slot:" << slot << " v" << v0;

        JSTaggedValue value = GET_VREG_VALUE(v0);
        InterpretedFrame *state = GET_FRAME(sp);
        JSTaggedValue env = state->env;
        for (uint32_t i = 0; i < level; i++) {
            JSTaggedValue taggedParentEnv = LexicalEnv::Cast(env.GetTaggedObject())->GetParentEnv();
            ASSERT(!taggedParentEnv.IsUndefined());
            env = taggedParentEnv;
        }
        LexicalEnv::Cast(env.GetTaggedObject())->SetProperties(thread, slot, value);

        DISPATCH(BytecodeInstruction::Format::PREF_IMM4_IMM4_V8);
    }
    HANDLE_OPCODE(HANDLE_NEWLEXENVDYN_PREF_IMM16) {
        uint16_t numVars = READ_INST_16_1();
        LOG_INST() << "intrinsics::newlexenvdyn"
                   << " imm " << numVars;

        JSTaggedValue res = FastRuntimeStub::NewLexicalEnvDyn(thread, factory, numVars);
        if (res.IsHole()) {
            SAVE_PC();
            res = SlowRuntimeStub::NewLexicalEnvDyn(thread, numVars);
            INTERPRETER_RETURN_IF_ABRUPT(res);
        }
        SET_ACC(res);
        GET_FRAME(sp)->env = res;
        DISPATCH(BytecodeInstruction::Format::PREF_IMM16);
    }
    HANDLE_OPCODE(HANDLE_POPLEXENVDYN_PREF) {
        InterpretedFrame *state = GET_FRAME(sp);
        JSTaggedValue currentLexenv = state->env;
        JSTaggedValue parentLexenv = LexicalEnv::Cast(currentLexenv.GetTaggedObject())->GetParentEnv();
        GET_FRAME(sp)->env = parentLexenv;
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_CREATEITERRESULTOBJ_PREF_V8_V8) {
        uint16_t v0 = READ_INST_8_1();
        uint16_t v1 = READ_INST_8_2();
        LOG_INST() << "intrinsics::createiterresultobj"
                   << " v" << v0 << " v" << v1;
        JSTaggedValue value = GET_VREG_VALUE(v0);
        JSTaggedValue flag = GET_VREG_VALUE(v1);
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::CreateIterResultObj(thread, value, flag);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_SUSPENDGENERATOR_PREF_V8_V8) {
        uint16_t v0 = READ_INST_8_1();
        uint16_t v1 = READ_INST_8_2();
        LOG_INST() << "intrinsics::suspendgenerator"
                   << " v" << v0 << " v" << v1;
        JSTaggedValue genObj = GET_VREG_VALUE(v0);
        JSTaggedValue value = GET_VREG_VALUE(v1);
        // suspend will record bytecode offset
        SAVE_PC();
        SAVE_ACC();
        JSTaggedValue res = SlowRuntimeStub::SuspendGenerator(thread, genObj, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);

        InterpretedFrame *state = GET_FRAME(sp);
        JSMethod *method = JSFunction::Cast(state->function.GetTaggedObject())->GetMethod();
        [[maybe_unused]] auto fistPC = method->GetBytecodeArray();
        UPDATE_HOTNESS_COUNTER(-(pc - fistPC));
        LOG(DEBUG, INTERPRETER) << "Exit: SuspendGenerator " << std::hex << reinterpret_cast<uintptr_t>(sp) << " "
                                << std::hex << reinterpret_cast<uintptr_t>(state->pc);
        sp = state->base.prev;
        ASSERT(sp != nullptr);
        InterpretedFrame *prevState = GET_FRAME(sp);
        pc = prevState->pc;
        // entry frame
        if (FrameHandler::IsEntryFrame(pc)) {
            state->acc = acc;
            return;
        }

        thread->SetCurrentSPFrame(sp);
        constpool = ConstantPool::Cast(prevState->constpool.GetTaggedObject());

        INTERPRETER_HANDLE_RETURN();
    }
    HANDLE_OPCODE(HANDLE_ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8) {
        uint16_t v0 = READ_INST_8_1();
        uint16_t v1 = READ_INST_8_2();
        LOG_INST() << "intrinsics::asyncfunctionawaituncaught"
                   << " v" << v0 << " v" << v1;
        JSTaggedValue asyncFuncObj = GET_VREG_VALUE(v0);
        JSTaggedValue value = GET_VREG_VALUE(v1);
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::AsyncFunctionAwaitUncaught(thread, asyncFuncObj, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8) {
        uint16_t v0 = READ_INST_8_1();
        [[maybe_unused]] uint16_t v1 = READ_INST_8_2();
        uint16_t v2 = READ_INST_8_3();
        LOG_INST() << "intrinsics::asyncfunctionresolve"
                   << " v" << v0 << " v" << v1 << " v" << v2;

        JSTaggedValue asyncFuncObj = GET_VREG_VALUE(v0);
        JSTaggedValue value = GET_VREG_VALUE(v2);
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::AsyncFunctionResolveOrReject(thread, asyncFuncObj, value, true);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_ASYNCFUNCTIONREJECT_PREF_V8_V8_V8) {
        uint16_t v0 = READ_INST_8_1();
        [[maybe_unused]] uint16_t v1 = READ_INST_8_2();
        uint16_t v2 = READ_INST_8_3();
        LOG_INST() << "intrinsics::asyncfunctionreject"
                   << " v" << v0 << " v" << v1 << " v" << v2;

        JSTaggedValue asyncFuncObj = GET_VREG_VALUE(v0);
        JSTaggedValue value = GET_VREG_VALUE(v2);
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::AsyncFunctionResolveOrReject(thread, asyncFuncObj, value, false);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_NEWOBJSPREADDYN_PREF_V8_V8) {
        uint16_t v0 = READ_INST_8_1();
        uint16_t v1 = READ_INST_8_2();
        LOG_INST() << "intrinsic::newobjspearddyn"
                   << " v" << v0 << " v" << v1;
        JSTaggedValue func = GET_VREG_VALUE(v0);
        JSTaggedValue newTarget = GET_VREG_VALUE(v1);
        JSTaggedValue array = GET_ACC();
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::NewObjSpreadDyn(thread, func, newTarget, array);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_THROWUNDEFINEDIFHOLE_PREF_V8_V8) {
        uint16_t v0 = READ_INST_8_1();
        uint16_t v1 = READ_INST_8_2();
        LOG_INST() << "intrinsic::throwundefinedifhole"
                   << " v" << v0 << " v" << v1;
        JSTaggedValue hole = GET_VREG_VALUE(v0);
        if (!hole.IsHole()) {
            DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
        }
        JSTaggedValue obj = GET_VREG_VALUE(v1);
        ASSERT(obj.IsString());
        SAVE_PC();
        SlowRuntimeStub::ThrowUndefinedIfHole(thread, obj);
        INTERPRETER_GOTO_EXCEPTION_HANDLER();
    }
    HANDLE_OPCODE(HANDLE_STOWNBYNAME_PREF_ID32_V8) {
        uint32_t stringId = READ_INST_32_1();
        uint32_t v0 = READ_INST_8_5();
        LOG_INST() << "intrinsics::stownbyname "
                   << "v" << v0 << " stringId:" << stringId;

        JSTaggedValue receiver = GET_VREG_VALUE(v0);
        if (receiver.IsJSObject() && !receiver.IsClassConstructor() && !receiver.IsClassPrototype()) {
            JSTaggedValue propKey = constpool->GetObjectFromCache(stringId);
            JSTaggedValue value = GET_ACC();
            // fast path
            SAVE_ACC();
            JSTaggedValue res = FastRuntimeStub::SetPropertyByName<true>(thread, receiver, propKey, value);
            if (!res.IsHole()) {
                INTERPRETER_RETURN_IF_ABRUPT(res);
                RESTORE_ACC();
                DISPATCH(BytecodeInstruction::Format::PREF_ID32_V8);
            }
            RESTORE_ACC();
        }

        SAVE_ACC();
        receiver = GET_VREG_VALUE(v0);                           // Maybe moved by GC
        auto propKey = constpool->GetObjectFromCache(stringId);  // Maybe moved by GC
        auto value = GET_ACC();                                  // Maybe moved by GC
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::StOwnByName(thread, receiver, propKey, value);
        RESTORE_ACC();
        INTERPRETER_RETURN_IF_ABRUPT(res);
        DISPATCH(BytecodeInstruction::Format::PREF_ID32_V8);
    }
    HANDLE_OPCODE(HANDLE_CREATEEMPTYARRAY_PREF) {
        LOG_INST() << "intrinsics::createemptyarray";
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::CreateEmptyArray(thread, factory, globalEnv);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_CREATEEMPTYOBJECT_PREF) {
        LOG_INST() << "intrinsics::createemptyobject";
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::CreateEmptyObject(thread, factory, globalEnv);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_CREATEOBJECTWITHBUFFER_PREF_IMM16) {
        uint16_t imm = READ_INST_16_1();
        LOG_INST() << "intrinsics::createobjectwithbuffer"
                   << " imm:" << imm;
        JSObject *result = JSObject::Cast(constpool->GetObjectFromCache(imm).GetTaggedObject());

        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::CreateObjectWithBuffer(thread, factory, result);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_IMM16);
    }
    HANDLE_OPCODE(HANDLE_SETOBJECTWITHPROTO_PREF_V8_V8) {
        uint16_t v0 = READ_INST_8_1();
        uint16_t v1 = READ_INST_8_2();
        LOG_INST() << "intrinsics::setobjectwithproto"
                   << " v" << v0 << " v" << v1;
        JSTaggedValue proto = GET_VREG_VALUE(v0);
        JSTaggedValue obj = GET_VREG_VALUE(v1);
        SAVE_ACC();
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::SetObjectWithProto(thread, proto, obj);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
        DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_CREATEARRAYWITHBUFFER_PREF_IMM16) {
        uint16_t imm = READ_INST_16_1();
        LOG_INST() << "intrinsics::createarraywithbuffer"
                   << " imm:" << imm;
        JSArray *result = JSArray::Cast(constpool->GetObjectFromCache(imm).GetTaggedObject());
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::CreateArrayWithBuffer(thread, factory, result);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_IMM16);
    }
    HANDLE_OPCODE(HANDLE_GETMODULENAMESPACE_PREF_ID32) {
        uint32_t stringId = READ_INST_32_1();
        auto localName = constpool->GetObjectFromCache(stringId);

        LOG_INST() << "intrinsics::getmodulenamespace "
                   << "stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(localName.GetTaggedObject()));

        JSTaggedValue moduleNamespace = SlowRuntimeStub::GetModuleNamespace(thread, localName);
        INTERPRETER_RETURN_IF_ABRUPT(moduleNamespace);
        SET_ACC(moduleNamespace);
        DISPATCH(BytecodeInstruction::Format::PREF_ID32);
    }
    HANDLE_OPCODE(HANDLE_STMODULEVAR_PREF_ID32) {
        uint32_t stringId = READ_INST_32_1();
        auto key = constpool->GetObjectFromCache(stringId);

        LOG_INST() << "intrinsics::stmodulevar "
                   << "stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(key.GetTaggedObject()));

        JSTaggedValue value = GET_ACC();

        SAVE_ACC();
        SlowRuntimeStub::StModuleVar(thread, key, value);
        RESTORE_ACC();
        DISPATCH(BytecodeInstruction::Format::PREF_ID32);
    }
    HANDLE_OPCODE(HANDLE_COPYMODULE_PREF_V8) {
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_LDMODULEVAR_PREF_ID32_IMM8) {
        uint32_t stringId = READ_INST_32_1();
        uint8_t innerFlag = READ_INST_8_5();

        JSTaggedValue key = constpool->GetObjectFromCache(stringId);
        LOG_INST() << "intrinsics::ldmodulevar "
                   << "string_id:" << stringId << ", "
                   << "key: " << ConvertToString(EcmaString::Cast(key.GetTaggedObject()));

        JSTaggedValue moduleVar = SlowRuntimeStub::LdModuleVar(thread, key, innerFlag != 0);
        INTERPRETER_RETURN_IF_ABRUPT(moduleVar);
        SET_ACC(moduleVar);
        DISPATCH(BytecodeInstruction::Format::PREF_ID32_IMM8);
    }
    HANDLE_OPCODE(HANDLE_CREATEREGEXPWITHLITERAL_PREF_ID32_IMM8) {
        uint32_t stringId = READ_INST_32_1();
        JSTaggedValue pattern = constpool->GetObjectFromCache(stringId);
        uint8_t flags = READ_INST_8_5();
        LOG_INST() << "intrinsics::createregexpwithliteral "
                   << "stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(pattern.GetTaggedObject()))
                   << ", flags:" << flags;
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::CreateRegExpWithLiteral(thread, pattern, flags);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_ID32_IMM8);
    }
    HANDLE_OPCODE(HANDLE_GETTEMPLATEOBJECT_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();
        LOG_INST() << "intrinsic::gettemplateobject"
                   << " v" << v0;

        JSTaggedValue literal = GET_VREG_VALUE(v0);
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::GetTemplateObject(thread, literal);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_GETNEXTPROPNAME_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();
        LOG_INST() << "intrinsic::getnextpropname"
                   << " v" << v0;
        JSTaggedValue iter = GET_VREG_VALUE(v0);
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::GetNextPropName(thread, iter);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_COPYDATAPROPERTIES_PREF_V8_V8) {
        uint16_t v0 = READ_INST_8_1();
        uint16_t v1 = READ_INST_8_2();
        LOG_INST() << "intrinsic::copydataproperties"
                   << " v" << v0 << " v" << v1;
        JSTaggedValue dst = GET_VREG_VALUE(v0);
        JSTaggedValue src = GET_VREG_VALUE(v1);
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::CopyDataProperties(thread, dst, src);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_STOWNBYINDEX_PREF_V8_IMM32) {
        uint32_t v0 = READ_INST_8_1();
        uint32_t index = READ_INST_32_2();
        LOG_INST() << "intrinsics::stownbyindex"
                   << " v" << v0 << " imm" << index;
        JSTaggedValue receiver = GET_VREG_VALUE(v0);
        // fast path
        if (receiver.IsHeapObject() && !receiver.IsClassConstructor() && !receiver.IsClassPrototype()) {
            SAVE_ACC();
            JSTaggedValue value = GET_ACC();
            // fast path
            JSTaggedValue res =
                FastRuntimeStub::SetPropertyByIndex<true>(thread, receiver, index, value);
            if (!res.IsHole()) {
                INTERPRETER_RETURN_IF_ABRUPT(res);
                RESTORE_ACC();
                DISPATCH(BytecodeInstruction::Format::PREF_V8_IMM32);
            }
            RESTORE_ACC();
        }
        SAVE_ACC();
        receiver = GET_VREG_VALUE(v0);  // Maybe moved by GC
        auto value = GET_ACC();         // Maybe moved by GC
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::StOwnByIndex(thread, receiver, index, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
        DISPATCH(BytecodeInstruction::Format::PREF_V8_IMM32);
    }
    HANDLE_OPCODE(HANDLE_STOWNBYVALUE_PREF_V8_V8) {
        uint32_t v0 = READ_INST_8_1();
        uint32_t v1 = READ_INST_8_2();
        LOG_INST() << "intrinsics::stownbyvalue"
                   << " v" << v0 << " v" << v1;

        JSTaggedValue receiver = GET_VREG_VALUE(v0);
        if (receiver.IsHeapObject() && !receiver.IsClassConstructor() && !receiver.IsClassPrototype()) {
            SAVE_ACC();
            JSTaggedValue propKey = GET_VREG_VALUE(v1);
            JSTaggedValue value = GET_ACC();
            // fast path
            JSTaggedValue res = FastRuntimeStub::SetPropertyByValue<true>(thread, receiver, propKey, value);

            // SetPropertyByValue maybe gc need update the value
            RESTORE_ACC();
            propKey = GET_VREG_VALUE(v1);
            value = GET_ACC();
            if (!res.IsHole()) {
                INTERPRETER_RETURN_IF_ABRUPT(res);
                RESTORE_ACC();
                DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
            }
        }

        // slow path
        SAVE_ACC();
        receiver = GET_VREG_VALUE(v0);      // Maybe moved by GC
        auto propKey = GET_VREG_VALUE(v1);  // Maybe moved by GC
        auto value = GET_ACC();             // Maybe moved by GC
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::StOwnByValue(thread, receiver, propKey, value);
        RESTORE_ACC();
        INTERPRETER_RETURN_IF_ABRUPT(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8) {
        uint16_t numKeys = READ_INST_16_1();
        uint16_t v0 = READ_INST_8_3();
        uint16_t firstArgRegIdx = READ_INST_8_4();
        LOG_INST() << "intrinsics::createobjectwithexcludedkeys " << numKeys << " v" << firstArgRegIdx;

        JSTaggedValue obj = GET_VREG_VALUE(v0);

        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::CreateObjectWithExcludedKeys(thread, numKeys, obj, firstArgRegIdx);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_IMM16_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_DEFINEGENERATORFUNC_PREF_ID16_IMM16_V8) {
        uint16_t methodId = READ_INST_16_1();
        uint16_t length = READ_INST_16_3();
        uint16_t v0 = READ_INST_8_5();
        LOG_INST() << "define gengerator function length: " << length
                   << " v" << v0;
        JSFunction *result = JSFunction::Cast(constpool->GetObjectFromCache(methodId).GetTaggedObject());
        ASSERT(result != nullptr);
        if (result->GetResolved()) {
            SAVE_PC();
            auto res = SlowRuntimeStub::DefineGeneratorFunc(thread, result);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            result = JSFunction::Cast(res.GetTaggedObject());
            result->SetConstantPool(thread, JSTaggedValue(constpool));
        } else {
            result->SetResolved(true);
        }

        result->SetPropertyInlinedProps(thread, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, JSTaggedValue(length));
        JSTaggedValue env = GET_VREG_VALUE(v0);
        result->SetLexicalEnv(thread, env);

        JSFunction *currentFunc = JSFunction::Cast((GET_FRAME(sp)->function).GetTaggedObject());
        result->SetModule(thread, currentFunc->GetModule());
        SET_ACC(JSTaggedValue(result))
        DISPATCH(BytecodeInstruction::Format::PREF_ID16_IMM16_V8);
    }
    HANDLE_OPCODE(HANDLE_DEFINEASYNCFUNC_PREF_ID16_IMM16_V8) {
        uint16_t methodId = READ_INST_16_1();
        uint16_t length = READ_INST_16_3();
        uint16_t v0 = READ_INST_8_5();
        LOG_INST() << "define async function length: " << length
                   << " v" << v0;
        JSFunction *result = JSFunction::Cast(constpool->GetObjectFromCache(methodId).GetTaggedObject());
        ASSERT(result != nullptr);
        if (result->GetResolved()) {
            SAVE_PC();
            auto res = SlowRuntimeStub::DefineAsyncFunc(thread, result);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            result = JSFunction::Cast(res.GetTaggedObject());
            result->SetConstantPool(thread, JSTaggedValue(constpool));
        } else {
            result->SetResolved(true);
        }

        result->SetPropertyInlinedProps(thread, JSFunction::LENGTH_INLINE_PROPERTY_INDEX, JSTaggedValue(length));
        JSTaggedValue env = GET_VREG_VALUE(v0);
        result->SetLexicalEnv(thread, env);

        JSFunction *currentFunc = JSFunction::Cast((GET_FRAME(sp)->function).GetTaggedObject());
        result->SetModule(thread, currentFunc->GetModule());
        SET_ACC(JSTaggedValue(result))
        DISPATCH(BytecodeInstruction::Format::PREF_ID16_IMM16_V8);
    }
    HANDLE_OPCODE(HANDLE_LDHOLE_PREF) {
        LOG_INST() << "intrinsic::ldhole";
        SET_ACC(JSTaggedValue::Hole());
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_COPYRESTARGS_PREF_IMM16) {
        uint16_t restIdx = READ_INST_16_1();
        LOG_INST() << "intrinsics::copyrestargs"
                   << " index: " << restIdx;

        uint32_t startIdx = 0;
        uint32_t restNumArgs = GetNumArgs(sp, restIdx, startIdx);

        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::CopyRestArgs(thread, sp, restNumArgs, startIdx);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_IMM16);
    }
    HANDLE_OPCODE(HANDLE_DEFINEGETTERSETTERBYVALUE_PREF_V8_V8_V8_V8) {
        uint16_t v0 = READ_INST_8_1();
        uint16_t v1 = READ_INST_8_2();
        uint16_t v2 = READ_INST_8_3();
        uint16_t v3 = READ_INST_8_4();
        LOG_INST() << "intrinsics::definegettersetterbyvalue"
                   << " v" << v0 << " v" << v1 << " v" << v2 << " v" << v3;

        JSTaggedValue obj = GET_VREG_VALUE(v0);
        JSTaggedValue prop = GET_VREG_VALUE(v1);
        JSTaggedValue getter = GET_VREG_VALUE(v2);
        JSTaggedValue setter = GET_VREG_VALUE(v3);
        JSTaggedValue flag = GET_ACC();
        SAVE_PC();
        JSTaggedValue res =
            SlowRuntimeStub::DefineGetterSetterByValue(thread, obj, prop, getter, setter, flag.ToBoolean());
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8_V8_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_LDOBJBYINDEX_PREF_V8_IMM32) {
        uint16_t v0 = READ_INST_8_1();
        uint32_t idx = READ_INST_32_2();
        LOG_INST() << "intrinsics::ldobjbyindex"
                   << " v" << v0 << " imm" << idx;

        JSTaggedValue receiver = GET_VREG_VALUE(v0);
        // fast path
        if (LIKELY(receiver.IsHeapObject())) {
            JSTaggedValue res = FastRuntimeStub::GetPropertyByIndex(thread, receiver, idx);
            if (!res.IsHole()) {
                INTERPRETER_RETURN_IF_ABRUPT(res);
                SET_ACC(res);
                DISPATCH(BytecodeInstruction::Format::PREF_V8_IMM32);
            }
        }
        // not meet fast condition or fast path return hole, walk slow path
        // slow stub not need receiver
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::LdObjByIndex(thread, receiver, idx, false, JSTaggedValue::Undefined());
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8_IMM32);
    }
    HANDLE_OPCODE(HANDLE_STOBJBYINDEX_PREF_V8_IMM32) {
        uint16_t v0 = READ_INST_8_1();
        uint32_t index = READ_INST_32_2();
        LOG_INST() << "intrinsics::stobjbyindex"
                   << " v" << v0 << " imm" << index;

        JSTaggedValue receiver = GET_VREG_VALUE(v0);
        if (receiver.IsHeapObject()) {
            SAVE_ACC();
            JSTaggedValue value = GET_ACC();
            // fast path
            JSTaggedValue res = FastRuntimeStub::SetPropertyByIndex(thread, receiver, index, value);
            if (!res.IsHole()) {
                INTERPRETER_RETURN_IF_ABRUPT(res);
                RESTORE_ACC();
                DISPATCH(BytecodeInstruction::Format::PREF_V8_IMM32);
            }
            RESTORE_ACC();
        }
        // slow path
        SAVE_ACC();
        SAVE_PC();
        receiver = GET_VREG_VALUE(v0);    // Maybe moved by GC
        JSTaggedValue value = GET_ACC();  // Maybe moved by GC
        JSTaggedValue res = SlowRuntimeStub::StObjByIndex(thread, receiver, index, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
        DISPATCH(BytecodeInstruction::Format::PREF_V8_IMM32);
    }
    HANDLE_OPCODE(HANDLE_LDOBJBYVALUE_PREF_V8_V8) {
        uint32_t v0 = READ_INST_8_1();
        uint32_t v1 = READ_INST_8_2();
        LOG_INST() << "intrinsics::Ldobjbyvalue"
                   << " v" << v0 << " v" << v1;

        JSTaggedValue receiver = GET_VREG_VALUE(v0);
        JSTaggedValue propKey = GET_VREG_VALUE(v1);

#if ECMASCRIPT_ENABLE_IC
        auto profileTypeInfo = GetRuntimeProfileTypeInfo(sp);
        if (!profileTypeInfo.IsUndefined()) {
            uint16_t slotId = READ_INST_8_0();
            auto profileTypeArray = ProfileTypeInfo::Cast(profileTypeInfo.GetTaggedObject());
            JSTaggedValue firstValue = profileTypeArray->Get(slotId);
            JSTaggedValue res = JSTaggedValue::Hole();

            if (LIKELY(firstValue.IsHeapObject())) {
                JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
                res = ICRuntimeStub::TryLoadICByValue(thread, receiver, propKey, firstValue, secondValue);
            }
            // IC miss and not enter the megamorphic state, store as polymorphic
            if (res.IsHole() && !firstValue.IsHole()) {
                res = ICRuntimeStub::LoadICByValue(thread,
                                                   profileTypeArray,
                                                   receiver, propKey, slotId);
            }

            if (LIKELY(!res.IsHole())) {
                INTERPRETER_RETURN_IF_ABRUPT(res);
                SET_ACC(res);
                DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
            }
        }
#endif
        // fast path
        if (LIKELY(receiver.IsHeapObject())) {
            JSTaggedValue res = FastRuntimeStub::GetPropertyByValue(thread, receiver, propKey);
            if (!res.IsHole()) {
                ASSERT(!res.IsAccessor());
                INTERPRETER_RETURN_IF_ABRUPT(res);
                SET_ACC(res);
                DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
            }
        }
        // slow path
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::LdObjByValue(thread, receiver, propKey, false, JSTaggedValue::Undefined());
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_STOBJBYVALUE_PREF_V8_V8) {
        uint32_t v0 = READ_INST_8_1();
        uint32_t v1 = READ_INST_8_2();

        LOG_INST() << "intrinsics::stobjbyvalue"
                   << " v" << v0 << " v" << v1;

        JSTaggedValue receiver = GET_VREG_VALUE(v0);
#if ECMASCRIPT_ENABLE_IC
        auto profileTypeInfo = GetRuntimeProfileTypeInfo(sp);
        if (!profileTypeInfo.IsUndefined()) {
            uint16_t slotId = READ_INST_8_0();
            auto profileTypeArray = ProfileTypeInfo::Cast(profileTypeInfo.GetTaggedObject());
            JSTaggedValue firstValue = profileTypeArray->Get(slotId);
            JSTaggedValue propKey = GET_VREG_VALUE(v1);
            JSTaggedValue value = GET_ACC();
            JSTaggedValue res = JSTaggedValue::Hole();
            SAVE_ACC();

            if (LIKELY(firstValue.IsHeapObject())) {
                JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
                res = ICRuntimeStub::TryStoreICByValue(thread, receiver, propKey, firstValue, secondValue, value);
            }
            // IC miss and not enter the megamorphic state, store as polymorphic
            if (res.IsHole() && !firstValue.IsHole()) {
                res = ICRuntimeStub::StoreICByValue(thread,
                                                    profileTypeArray,
                                                    receiver, propKey, value, slotId);
            }

            if (LIKELY(!res.IsHole())) {
                INTERPRETER_RETURN_IF_ABRUPT(res);
                RESTORE_ACC();
                DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
            }
        }
#endif
        if (receiver.IsHeapObject()) {
            SAVE_ACC();
            JSTaggedValue propKey = GET_VREG_VALUE(v1);
            JSTaggedValue value = GET_ACC();
            // fast path
            JSTaggedValue res = FastRuntimeStub::SetPropertyByValue(thread, receiver, propKey, value);
            if (!res.IsHole()) {
                INTERPRETER_RETURN_IF_ABRUPT(res);
                RESTORE_ACC();
                DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
            }
            RESTORE_ACC();
        }
        {
            // slow path
            SAVE_ACC();
            SAVE_PC();
            receiver = GET_VREG_VALUE(v0);  // Maybe moved by GC
            JSTaggedValue propKey = GET_VREG_VALUE(v1);   // Maybe moved by GC
            JSTaggedValue value = GET_ACC();              // Maybe moved by GC
            JSTaggedValue res = SlowRuntimeStub::StObjByValue(thread, receiver, propKey, value);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
        }
        DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_LDSUPERBYVALUE_PREF_V8_V8) {
        uint32_t v0 = READ_INST_8_1();
        uint32_t v1 = READ_INST_8_2();
        LOG_INST() << "intrinsics::Ldsuperbyvalue"
                   << " v" << v0 << " v" << v1;

        JSTaggedValue receiver = GET_VREG_VALUE(v0);
        JSTaggedValue propKey = GET_VREG_VALUE(v1);

        // slow path
        SAVE_PC();
        JSTaggedValue thisFunc = GetThisFunction(sp);
        JSTaggedValue res = SlowRuntimeStub::LdSuperByValue(thread, receiver, propKey, thisFunc);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_STSUPERBYVALUE_PREF_V8_V8) {
        uint32_t v0 = READ_INST_8_1();
        uint32_t v1 = READ_INST_8_2();

        LOG_INST() << "intrinsics::stsuperbyvalue"
                   << " v" << v0 << " v" << v1;
        JSTaggedValue receiver = GET_VREG_VALUE(v0);
        JSTaggedValue propKey = GET_VREG_VALUE(v1);
        JSTaggedValue value = GET_ACC();

        // slow path
        SAVE_ACC();
        SAVE_PC();
        JSTaggedValue thisFunc = GetThisFunction(sp);
        JSTaggedValue res = SlowRuntimeStub::StSuperByValue(thread, receiver, propKey, value, thisFunc);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
        DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_TRYLDGLOBALBYNAME_PREF_ID32) {
        uint32_t stringId = READ_INST_32_1();
        auto prop = constpool->GetObjectFromCache(stringId);

        LOG_INST() << "intrinsics::tryldglobalbyname "
                   << "stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(prop.GetTaggedObject()));

#if ECMASCRIPT_ENABLE_IC
        auto profileTypeInfo = GetRuntimeProfileTypeInfo(sp);
        if (!profileTypeInfo.IsUndefined()) {
            uint16_t slotId = READ_INST_8_0();
            JSTaggedValue res = ICRuntimeStub::LoadGlobalICByName(thread,
                                                                  ProfileTypeInfo::Cast(
                                                                      profileTypeInfo.GetTaggedObject()),
                                                                  globalObj, prop, slotId);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(BytecodeInstruction::Format::PREF_ID32);
        }
#endif

        // order: 1. global record 2. global object
        JSTaggedValue result = SlowRuntimeStub::LdGlobalRecord(thread, prop);
        if (!result.IsUndefined()) {
            SET_ACC(PropertyBox::Cast(result.GetTaggedObject())->GetValue());
        } else {
            JSTaggedValue globalResult = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, prop);
            if (!globalResult.IsHole()) {
                SET_ACC(globalResult);
            } else {
                // slow path
                SAVE_PC();
                JSTaggedValue res = SlowRuntimeStub::TryLdGlobalByName(thread, globalObj, prop);
                INTERPRETER_RETURN_IF_ABRUPT(res);
                SET_ACC(res);
            }
        }

        DISPATCH(BytecodeInstruction::Format::PREF_ID32);
    }
    HANDLE_OPCODE(HANDLE_TRYSTGLOBALBYNAME_PREF_ID32) {
        uint32_t stringId = READ_INST_32_1();
        JSTaggedValue propKey = constpool->GetObjectFromCache(stringId);
        LOG_INST() << "intrinsics::trystglobalbyname"
                   << " stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject()));

#if ECMASCRIPT_ENABLE_IC
        auto profileTypeInfo = GetRuntimeProfileTypeInfo(sp);
        if (!profileTypeInfo.IsUndefined()) {
            uint16_t slotId = READ_INST_8_0();
            JSTaggedValue value = GET_ACC();
            SAVE_ACC();
            JSTaggedValue res = ICRuntimeStub::StoreGlobalICByName(thread,
                                                                   ProfileTypeInfo::Cast(
                                                                       profileTypeInfo.GetTaggedObject()),
                                                                   globalObj, propKey, value, slotId);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(BytecodeInstruction::Format::PREF_ID32);
        }
#endif

        auto recordResult = SlowRuntimeStub::LdGlobalRecord(thread, propKey);
        SAVE_PC();
        // 1. find from global record
        if (!recordResult.IsUndefined()) {
            JSTaggedValue value = GET_ACC();
            SAVE_ACC();
            JSTaggedValue res = SlowRuntimeStub::TryUpdateGlobalRecord(thread, propKey, value);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
        } else {
            // 2. find from global object
            auto globalResult = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, propKey);
            if (globalResult.IsHole()) {
                auto result = SlowRuntimeStub::ThrowReferenceError(thread, propKey, " is not defined");
                INTERPRETER_RETURN_IF_ABRUPT(result);
            }
            JSTaggedValue value = GET_ACC();
            SAVE_ACC();
            JSTaggedValue res = SlowRuntimeStub::StGlobalVar(thread, propKey, value);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
        }
        DISPATCH(BytecodeInstruction::Format::PREF_ID32);
    }

    HANDLE_OPCODE(HANDLE_STCONSTTOGLOBALRECORD_PREF_ID32) {
        uint32_t stringId = READ_INST_32_1();
        JSTaggedValue propKey = constpool->GetObjectFromCache(stringId);
        LOG_INST() << "intrinsics::stconsttoglobalrecord"
                   << " stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject()));

        JSTaggedValue value = GET_ACC();
        SAVE_ACC();
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::StGlobalRecord(thread, propKey, value, true);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
        DISPATCH(BytecodeInstruction::Format::PREF_ID32);
    }

    HANDLE_OPCODE(HANDLE_STLETTOGLOBALRECORD_PREF_ID32) {
        uint32_t stringId = READ_INST_32_1();
        JSTaggedValue propKey = constpool->GetObjectFromCache(stringId);
        LOG_INST() << "intrinsics::stlettoglobalrecord"
                   << " stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject()));

        JSTaggedValue value = GET_ACC();
        SAVE_ACC();
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::StGlobalRecord(thread, propKey, value, false);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
        DISPATCH(BytecodeInstruction::Format::PREF_ID32);
    }

    HANDLE_OPCODE(HANDLE_STCLASSTOGLOBALRECORD_PREF_ID32) {
        uint32_t stringId = READ_INST_32_1();
        JSTaggedValue propKey = constpool->GetObjectFromCache(stringId);
        LOG_INST() << "intrinsics::stclasstoglobalrecord"
                   << " stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject()));

        JSTaggedValue value = GET_ACC();
        SAVE_ACC();
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::StGlobalRecord(thread, propKey, value, false);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
        DISPATCH(BytecodeInstruction::Format::PREF_ID32);
    }

    HANDLE_OPCODE(HANDLE_STOWNBYVALUEWITHNAMESET_PREF_V8_V8) {
        uint32_t v0 = READ_INST_8_1();
        uint32_t v1 = READ_INST_8_2();
        LOG_INST() << "intrinsics::stownbyvaluewithnameset"
                   << " v" << v0 << " v" << v1;
        JSTaggedValue receiver = GET_VREG_VALUE(v0);
        if (receiver.IsHeapObject() && !receiver.IsClassConstructor() && !receiver.IsClassPrototype()) {
            SAVE_ACC();
            JSTaggedValue propKey = GET_VREG_VALUE(v1);
            JSTaggedValue value = GET_ACC();
            // fast path
            JSTaggedValue res = FastRuntimeStub::SetPropertyByValue<true>(thread, receiver, propKey, value);

            // SetPropertyByValue maybe gc need update the value
            RESTORE_ACC();
            propKey = GET_VREG_VALUE(v1);
            value = GET_ACC();
            if (!res.IsHole()) {
                INTERPRETER_RETURN_IF_ABRUPT(res);
                JSFunction::SetFunctionNameNoPrefix(thread, JSFunction::Cast(value.GetTaggedObject()), propKey);
                RESTORE_ACC();
                DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
            }
        }

        // slow path
        SAVE_ACC();
        SAVE_PC();
        receiver = GET_VREG_VALUE(v0);      // Maybe moved by GC
        auto propKey = GET_VREG_VALUE(v1);  // Maybe moved by GC
        auto value = GET_ACC();             // Maybe moved by GC
        JSTaggedValue res = SlowRuntimeStub::StOwnByValueWithNameSet(thread, receiver, propKey, value);
        RESTORE_ACC();
        INTERPRETER_RETURN_IF_ABRUPT(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_STOWNBYNAMEWITHNAMESET_PREF_ID32_V8) {
        uint32_t stringId = READ_INST_32_1();
        uint32_t v0 = READ_INST_8_5();
        LOG_INST() << "intrinsics::stownbynamewithnameset "
                   << "v" << v0 << " stringId:" << stringId;

        JSTaggedValue receiver = GET_VREG_VALUE(v0);
        if (receiver.IsJSObject() && !receiver.IsClassConstructor() && !receiver.IsClassPrototype()) {
            JSTaggedValue propKey = constpool->GetObjectFromCache(stringId);
            JSTaggedValue value = GET_ACC();
            // fast path
            SAVE_ACC();
            JSTaggedValue res = FastRuntimeStub::SetPropertyByName<true>(thread, receiver, propKey, value);
            if (!res.IsHole()) {
                INTERPRETER_RETURN_IF_ABRUPT(res);
                JSFunction::SetFunctionNameNoPrefix(thread, JSFunction::Cast(value.GetTaggedObject()), propKey);
                RESTORE_ACC();
                DISPATCH(BytecodeInstruction::Format::PREF_ID32_V8);
            }
            RESTORE_ACC();
        }

        SAVE_ACC();
        SAVE_PC();
        receiver = GET_VREG_VALUE(v0);                           // Maybe moved by GC
        auto propKey = constpool->GetObjectFromCache(stringId);  // Maybe moved by GC
        auto value = GET_ACC();                                  // Maybe moved by GC
        JSTaggedValue res = SlowRuntimeStub::StOwnByNameWithNameSet(thread, receiver, propKey, value);
        RESTORE_ACC();
        INTERPRETER_RETURN_IF_ABRUPT(res);
        DISPATCH(BytecodeInstruction::Format::PREF_ID32_V8);
    }

    HANDLE_OPCODE(HANDLE_LDGLOBALVAR_PREF_ID32) {
        uint32_t stringId = READ_INST_32_1();
        JSTaggedValue propKey = constpool->GetObjectFromCache(stringId);

#if ECMASCRIPT_ENABLE_IC
        auto profileTypeInfo = GetRuntimeProfileTypeInfo(sp);
        if (!profileTypeInfo.IsUndefined()) {
            uint16_t slotId = READ_INST_8_0();
            JSTaggedValue res = ICRuntimeStub::LoadGlobalICByName(thread,
                                                                  ProfileTypeInfo::Cast(
                                                                      profileTypeInfo.GetTaggedObject()),
                                                                  globalObj, propKey, slotId);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
            DISPATCH(BytecodeInstruction::Format::PREF_ID32);
        }
#endif
        JSTaggedValue result = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, propKey);
        if (!result.IsHole()) {
            SET_ACC(result);
        } else {
            // slow path
            SAVE_PC();
            JSTaggedValue res = SlowRuntimeStub::LdGlobalVar(thread, globalObj, propKey);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            SET_ACC(res);
        }
        DISPATCH(BytecodeInstruction::Format::PREF_ID32);
    }
    HANDLE_OPCODE(HANDLE_LDOBJBYNAME_PREF_ID32_V8) {
        uint32_t v0 = READ_INST_8_5();
        JSTaggedValue receiver = GET_VREG_VALUE(v0);

#if ECMASCRIPT_ENABLE_IC
        auto profileTypeInfo = GetRuntimeProfileTypeInfo(sp);
        if (!profileTypeInfo.IsUndefined()) {
            uint16_t slotId = READ_INST_8_0();
            auto profileTypeArray = ProfileTypeInfo::Cast(profileTypeInfo.GetTaggedObject());
            JSTaggedValue firstValue = profileTypeArray->Get(slotId);
            JSTaggedValue res = JSTaggedValue::Hole();

            if (LIKELY(firstValue.IsHeapObject())) {
                JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
                res = ICRuntimeStub::TryLoadICByName(thread, receiver, firstValue, secondValue);
            }
            if (LIKELY(!res.IsHole())) {
                INTERPRETER_RETURN_IF_ABRUPT(res);
                SET_ACC(res);
                DISPATCH(BytecodeInstruction::Format::PREF_ID32_V8);
            } else if (!firstValue.IsHole()) { // IC miss and not enter the megamorphic state, store as polymorphic
                uint32_t stringId = READ_INST_32_1();
                JSTaggedValue propKey = constpool->GetObjectFromCache(stringId);
                res = ICRuntimeStub::LoadICByName(thread,
                                                  profileTypeArray,
                                                  receiver, propKey, slotId);
                INTERPRETER_RETURN_IF_ABRUPT(res);
                SET_ACC(res);
                DISPATCH(BytecodeInstruction::Format::PREF_ID32_V8);
            }
        }
#endif
        uint32_t stringId = READ_INST_32_1();
        JSTaggedValue propKey = constpool->GetObjectFromCache(stringId);
        LOG_INST() << "intrinsics::ldobjbyname "
                   << "v" << v0 << " stringId:" << stringId << ", "
                   << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject())) << ", obj:" << receiver.GetRawData();

        if (LIKELY(receiver.IsHeapObject())) {
            // fast path
            JSTaggedValue res = FastRuntimeStub::GetPropertyByName(thread, receiver, propKey);
            if (!res.IsHole()) {
                ASSERT(!res.IsAccessor());
                INTERPRETER_RETURN_IF_ABRUPT(res);
                SET_ACC(res);
                DISPATCH(BytecodeInstruction::Format::PREF_ID32_V8);
            }
        }
        // not meet fast condition or fast path return hole, walk slow path
        // slow stub not need receiver
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::LdObjByName(thread, receiver, propKey, false, JSTaggedValue::Undefined());
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_ID32_V8);
    }
    HANDLE_OPCODE(HANDLE_STOBJBYNAME_PREF_ID32_V8) {
        uint32_t v0 = READ_INST_8_5();
        JSTaggedValue receiver = GET_VREG_VALUE(v0);
        JSTaggedValue value = GET_ACC();
#if ECMASCRIPT_ENABLE_IC
        auto profileTypeInfo = GetRuntimeProfileTypeInfo(sp);
        if (!profileTypeInfo.IsUndefined()) {
            uint16_t slotId = READ_INST_8_0();
            auto profileTypeArray = ProfileTypeInfo::Cast(profileTypeInfo.GetTaggedObject());
            JSTaggedValue firstValue = profileTypeArray->Get(slotId);
            JSTaggedValue res = JSTaggedValue::Hole();
            SAVE_ACC();

            if (LIKELY(firstValue.IsHeapObject())) {
                JSTaggedValue secondValue = profileTypeArray->Get(slotId + 1);
                res = ICRuntimeStub::TryStoreICByName(thread, receiver, firstValue, secondValue, value);
            }
            if (LIKELY(!res.IsHole())) {
                INTERPRETER_RETURN_IF_ABRUPT(res);
                RESTORE_ACC();
                DISPATCH(BytecodeInstruction::Format::PREF_ID32_V8);
            } else if (!firstValue.IsHole()) { // IC miss and not enter the megamorphic state, store as polymorphic
                uint32_t stringId = READ_INST_32_1();
                JSTaggedValue propKey = constpool->GetObjectFromCache(stringId);
                res = ICRuntimeStub::StoreICByName(thread,
                                                   profileTypeArray,
                                                   receiver, propKey, value, slotId);
                INTERPRETER_RETURN_IF_ABRUPT(res);
                RESTORE_ACC();
                DISPATCH(BytecodeInstruction::Format::PREF_ID32_V8);
            }
        }
#endif
        uint32_t stringId = READ_INST_32_1();
        LOG_INST() << "intrinsics::stobjbyname "
                   << "v" << v0 << " stringId:" << stringId;
        if (receiver.IsHeapObject()) {
            JSTaggedValue propKey = constpool->GetObjectFromCache(stringId);
            value = GET_ACC();
            // fast path
            SAVE_ACC();
            JSTaggedValue res = FastRuntimeStub::SetPropertyByName(thread, receiver, propKey, value);
            if (!res.IsHole()) {
                INTERPRETER_RETURN_IF_ABRUPT(res);
                RESTORE_ACC();
                DISPATCH(BytecodeInstruction::Format::PREF_ID32_V8);
            }
            RESTORE_ACC();
        }
        // slow path
        SAVE_ACC();
        SAVE_PC();
        receiver = GET_VREG_VALUE(v0);                           // Maybe moved by GC
        auto propKey = constpool->GetObjectFromCache(stringId);  // Maybe moved by GC
        value = GET_ACC();                                  // Maybe moved by GC
        JSTaggedValue res = SlowRuntimeStub::StObjByName(thread, receiver, propKey, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
        DISPATCH(BytecodeInstruction::Format::PREF_ID32_V8);
    }
    HANDLE_OPCODE(HANDLE_LDSUPERBYNAME_PREF_ID32_V8) {
        uint32_t stringId = READ_INST_32_1();
        uint32_t v0 = READ_INST_8_5();
        JSTaggedValue obj = GET_VREG_VALUE(v0);
        JSTaggedValue propKey = constpool->GetObjectFromCache(stringId);

        LOG_INST() << "intrinsics::ldsuperbyname"
                   << "v" << v0 << " stringId:" << stringId << ", "
                   << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject())) << ", obj:" << obj.GetRawData();

        SAVE_PC();
        JSTaggedValue thisFunc = GetThisFunction(sp);
        JSTaggedValue res = SlowRuntimeStub::LdSuperByValue(thread, obj, propKey, thisFunc);

        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_ID32_V8);
    }
    HANDLE_OPCODE(HANDLE_STSUPERBYNAME_PREF_ID32_V8) {
        uint32_t stringId = READ_INST_32_1();
        uint32_t v0 = READ_INST_8_5();

        JSTaggedValue obj = GET_VREG_VALUE(v0);
        JSTaggedValue propKey = constpool->GetObjectFromCache(stringId);
        JSTaggedValue value = GET_ACC();

        LOG_INST() << "intrinsics::stsuperbyname"
                   << "v" << v0 << " stringId:" << stringId << ", "
                   << ConvertToString(EcmaString::Cast(propKey.GetTaggedObject())) << ", obj:" << obj.GetRawData()
                   << ", value:" << value.GetRawData();

        // slow path
        SAVE_ACC();
        SAVE_PC();
        JSTaggedValue thisFunc = GetThisFunction(sp);
        JSTaggedValue res = SlowRuntimeStub::StSuperByValue(thread, obj, propKey, value, thisFunc);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
        DISPATCH(BytecodeInstruction::Format::PREF_ID32_V8);
    }
    HANDLE_OPCODE(HANDLE_STGLOBALVAR_PREF_ID32) {
        uint32_t stringId = READ_INST_32_1();
        JSTaggedValue prop = constpool->GetObjectFromCache(stringId);
        JSTaggedValue value = GET_ACC();

        LOG_INST() << "intrinsics::stglobalvar "
                   << "stringId:" << stringId << ", " << ConvertToString(EcmaString::Cast(prop.GetTaggedObject()))
                   << ", value:" << value.GetRawData();
#if ECMASCRIPT_ENABLE_IC
        auto profileTypeInfo = GetRuntimeProfileTypeInfo(sp);
        if (!profileTypeInfo.IsUndefined()) {
            uint16_t slotId = READ_INST_8_0();
            SAVE_ACC();
            JSTaggedValue res = ICRuntimeStub::StoreGlobalICByName(thread,
                                                                   ProfileTypeInfo::Cast(
                                                                       profileTypeInfo.GetTaggedObject()),
                                                                   globalObj, prop, value, slotId);
            INTERPRETER_RETURN_IF_ABRUPT(res);
            RESTORE_ACC();
            DISPATCH(BytecodeInstruction::Format::PREF_ID32);
        }
#endif
        SAVE_ACC();
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::StGlobalVar(thread, prop, value);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        RESTORE_ACC();
        DISPATCH(BytecodeInstruction::Format::PREF_ID32);
    }
    HANDLE_OPCODE(HANDLE_CREATEGENERATOROBJ_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();
        LOG_INST() << "intrinsics::creategeneratorobj"
                   << " v" << v0;
        SAVE_PC();
        JSTaggedValue genFunc = GET_VREG_VALUE(v0);
        JSTaggedValue res = SlowRuntimeStub::CreateGeneratorObj(thread, genFunc);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_STARRAYSPREAD_PREF_V8_V8) {
        uint16_t v0 = READ_INST_8_1();
        uint16_t v1 = READ_INST_8_2();
        LOG_INST() << "ecmascript::intrinsics::starrayspread"
                   << " v" << v0 << " v" << v1 << "acc";
        JSTaggedValue dst = GET_VREG_VALUE(v0);
        JSTaggedValue index = GET_VREG_VALUE(v1);
        JSTaggedValue src = GET_ACC();
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::StArraySpread(thread, dst, index, src);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_GETITERATORNEXT_PREF_V8_V8) {
        uint16_t v0 = READ_INST_8_1();
        uint16_t v1 = READ_INST_8_2();
        LOG_INST() << "intrinsic::getiteratornext"
                   << " v" << v0 << " v" << v1;
        JSTaggedValue obj = GET_VREG_VALUE(v0);
        JSTaggedValue method = GET_VREG_VALUE(v1);
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::GetIteratorNext(thread, obj, method);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8) {
        uint16_t methodId = READ_INST_16_1();
        uint16_t length = READ_INST_16_5();
        uint16_t v0 = READ_INST_8_7();
        uint16_t v1 = READ_INST_8_8();
        LOG_INST() << "intrinsics::defineclasswithbuffer"
                   << " method id:" << methodId << " lexenv: v" << v0 << " parent: v" << v1;
        JSFunction *classTemplate = JSFunction::Cast(constpool->GetObjectFromCache(methodId).GetTaggedObject());
        ASSERT(classTemplate != nullptr);

        JSTaggedValue lexenv = GET_VREG_VALUE(v0);
        JSTaggedValue proto = GET_VREG_VALUE(v1);

        JSTaggedValue res;
        SAVE_PC();
        res = SlowRuntimeStub::CloneClassFromTemplate(thread, JSTaggedValue(classTemplate),
                                                      proto, lexenv, constpool);

        INTERPRETER_RETURN_IF_ABRUPT(res);
        ASSERT(res.IsClassConstructor());
        JSFunction *cls = JSFunction::Cast(res.GetTaggedObject());

        lexenv = GET_VREG_VALUE(v0);  // slow runtime may gc
        cls->SetLexicalEnv(thread, lexenv);

        JSFunction *currentFunc = JSFunction::Cast((GET_FRAME(sp)->function).GetTaggedObject());
        cls->SetModule(thread, currentFunc->GetModule());

        SlowRuntimeStub::SetClassConstructorLength(thread, res, JSTaggedValue(length));

        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_ID16_IMM16_IMM16_V8_V8);
    }
    HANDLE_OPCODE(HANDLE_LDFUNCTION_PREF) {
        LOG_INST() << "intrinsic::ldfunction";
        SET_ACC(GetThisFunction(sp));
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_LDBIGINT_PREF_ID32) {
        uint32_t stringId = READ_INST_32_1();
        LOG_INST() << "intrinsic::ldbigint";
        JSTaggedValue numberBigInt = constpool->GetObjectFromCache(stringId);
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::LdBigInt(thread, numberBigInt);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_ID32);
    }
    HANDLE_OPCODE(HANDLE_SUPERCALL_PREF_IMM16_V8) {
        uint16_t range = READ_INST_16_1();
        uint16_t v0 = READ_INST_8_3();
        LOG_INST() << "intrinsics::supercall"
                   << " range: " << range << " v" << v0;

        JSTaggedValue thisFunc = GET_ACC();
        JSTaggedValue newTarget = GetNewTarget(sp);

        JSTaggedValue superCtor = SlowRuntimeStub::GetSuperConstructor(thread, thisFunc);
        INTERPRETER_RETURN_IF_ABRUPT(superCtor);

        if (superCtor.IsJSFunction() && superCtor.IsConstructor() && !newTarget.IsUndefined()) {
            JSFunction *superCtorFunc = JSFunction::Cast(superCtor.GetTaggedObject());
            JSMethod *superCtorMethod = superCtorFunc->GetMethod();
            if (superCtorFunc->IsBuiltinConstructor()) {
                ASSERT(superCtorMethod->GetNumVregsWithCallField() == 0);
                size_t frameSize = INTERPRETER_FRAME_STATE_SIZE + range + NUM_MANDATORY_JSFUNC_ARGS;
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                JSTaggedType *newSp = sp - frameSize;
                if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
                    INTERPRETER_GOTO_EXCEPTION_HANDLER();
                }
                EcmaRuntimeCallInfo ecmaRuntimeCallInfo(thread, range, newSp);
                // copy args
                uint32_t index = 0;
                // func
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = superCtor.GetRawData();
                // newTarget
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = newTarget.GetRawData();
                // this
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
                for (size_t i = 0; i < range; ++i) {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    newSp[index++] = GET_VREG(v0 + i);
                }

                InterpretedFrame *state = GET_FRAME(newSp);
                state->base.prev = sp;
                state->base.type = FrameType::INTERPRETER_FRAME;
                state->pc = nullptr;
                state->function = superCtor;
                thread->SetCurrentSPFrame(newSp);
                LOG(DEBUG, INTERPRETER) << "Entry: Runtime SuperCall ";
                JSTaggedValue retValue = reinterpret_cast<EcmaEntrypoint>(
                    const_cast<void *>(superCtorMethod->GetNativePointer()))(&ecmaRuntimeCallInfo);
                thread->SetCurrentSPFrame(sp);

                if (UNLIKELY(thread->HasPendingException())) {
                    INTERPRETER_GOTO_EXCEPTION_HANDLER();
                }
                LOG(DEBUG, INTERPRETER) << "Exit: Runtime SuperCall ";
                SET_ACC(retValue);
                DISPATCH(BytecodeInstruction::Format::PREF_IMM16_V8);
            }

            if (IsFastNewFrameEnter(superCtorFunc, superCtorMethod)) {
                SAVE_PC();
                uint32_t numVregs = superCtorMethod->GetNumVregsWithCallField();
                uint32_t numDeclaredArgs = superCtorFunc->IsBase() ?
                    superCtorMethod->GetNumArgsWithCallField() + 1 :  // +1 for this
                    superCtorMethod->GetNumArgsWithCallField() + 2;   // +2 for newTarget and this
                // +1 for hidden this, explicit this may be overwritten after bc optimizer
                size_t frameSize = INTERPRETER_FRAME_STATE_SIZE + numVregs + numDeclaredArgs + 1;
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                JSTaggedType *newSp = sp - frameSize;
                InterpretedFrame *state = GET_FRAME(newSp);

                if (UNLIKELY(thread->DoStackOverflowCheck(newSp))) {
                    INTERPRETER_GOTO_EXCEPTION_HANDLER();
                }

                uint32_t index = 0;
                // initialize vregs value
                for (size_t i = 0; i < numVregs; ++i) {
                    newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
                }

                // this
                JSTaggedValue thisObj;
                if (superCtorFunc->IsBase()) {
                    thisObj = FastRuntimeStub::NewThisObject(thread, superCtor, newTarget, state);
                    INTERPRETER_RETURN_IF_ABRUPT(thisObj);
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    newSp[index++] = thisObj.GetRawData();
                } else {
                    ASSERT(superCtorFunc->IsDerivedConstructor());
                    newSp[index++] = newTarget.GetRawData();
                    thisObj = JSTaggedValue::Undefined();
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    newSp[index++] = thisObj.GetRawData();

                    state->function = superCtor;
                    state->constpool = superCtorFunc->GetConstantPool();
                    state->profileTypeInfo = superCtorFunc->GetProfileTypeInfo();
                    state->env = superCtorFunc->GetLexicalEnv();
                }

                // the second condition ensure not push extra args
                for (size_t i = 0; i < range && index < numVregs + numDeclaredArgs; ++i) {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    newSp[index++] = GET_VREG(v0 + i);
                }

                // set undefined to the extra prats of declare
                for (size_t i = index; i < numVregs + numDeclaredArgs; ++i) {
                    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                    newSp[index++] = JSTaggedValue::VALUE_UNDEFINED;
                }

                // hidden this object
                newSp[index] = thisObj.GetRawData();

                state->base.prev = sp;
                state->base.type = FrameType::INTERPRETER_FAST_NEW_FRAME;
                state->pc = pc = superCtorMethod->GetBytecodeArray();
                sp = newSp;
                state->acc = JSTaggedValue::Hole();
                constpool = ConstantPool::Cast(state->constpool.GetTaggedObject());

                thread->SetCurrentSPFrame(newSp);
                LOG(DEBUG, INTERPRETER) << "Entry: Runtime SuperCall " << std::hex << reinterpret_cast<uintptr_t>(sp)
                                        << " " << std::hex << reinterpret_cast<uintptr_t>(pc);
                DISPATCH_OFFSET(0);
            }
        }

        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::SuperCall(thread, thisFunc, newTarget, v0, range);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_IMM16_V8);
    }
    HANDLE_OPCODE(HANDLE_SUPERCALLSPREAD_PREF_V8) {
        uint16_t v0 = READ_INST_8_1();
        LOG_INST() << "intrinsic::supercallspread"
                   << " array: v" << v0;

        JSTaggedValue thisFunc = GET_ACC();
        JSTaggedValue newTarget = GetNewTarget(sp);
        JSTaggedValue array = GET_VREG_VALUE(v0);

        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::SuperCallSpread(thread, thisFunc, newTarget, array);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_V8);
    }
    HANDLE_OPCODE(HANDLE_CREATEOBJECTHAVINGMETHOD_PREF_IMM16) {
        uint16_t imm = READ_INST_16_1();
        LOG_INST() << "intrinsics::createobjecthavingmethod"
                   << " imm:" << imm;
        JSObject *result = JSObject::Cast(constpool->GetObjectFromCache(imm).GetTaggedObject());
        JSTaggedValue env = GET_ACC();

        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::CreateObjectHavingMethod(thread, factory, result, env, constpool);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        SET_ACC(res);
        DISPATCH(BytecodeInstruction::Format::PREF_IMM16);
    }
    HANDLE_OPCODE(HANDLE_THROWIFSUPERNOTCORRECTCALL_PREF_IMM16) {
        uint16_t imm = READ_INST_16_1();
        JSTaggedValue thisValue = GET_ACC();
        LOG_INST() << "intrinsic::throwifsupernotcorrectcall"
                   << " imm:" << imm;
        SAVE_PC();
        JSTaggedValue res = SlowRuntimeStub::ThrowIfSuperNotCorrectCall(thread, imm, thisValue);
        INTERPRETER_RETURN_IF_ABRUPT(res);
        DISPATCH(BytecodeInstruction::Format::PREF_IMM16);
    }
    HANDLE_OPCODE(HANDLE_LDHOMEOBJECT_PREF) {
        LOG_INST() << "intrinsics::ldhomeobject";

        JSTaggedValue thisFunc = GetThisFunction(sp);
        JSTaggedValue homeObject = JSFunction::Cast(thisFunc.GetTaggedObject())->GetHomeObject();

        SET_ACC(homeObject);
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_THROWDELETESUPERPROPERTY_PREF) {
        LOG_INST() << "throwdeletesuperproperty";

        SAVE_PC();
        SlowRuntimeStub::ThrowDeleteSuperProperty(thread);
        INTERPRETER_GOTO_EXCEPTION_HANDLER();
    }
    HANDLE_OPCODE(HANDLE_DEBUGGER_PREF) {
        LOG_INST() << "intrinsics::debugger";
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_ISTRUE_PREF) {
        LOG_INST() << "intrinsics::istrue";
        if (GET_ACC().ToBoolean()) {
            SET_ACC(JSTaggedValue::True());
        } else {
            SET_ACC(JSTaggedValue::False());
        }
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(HANDLE_ISFALSE_PREF) {
        LOG_INST() << "intrinsics::isfalse";
        if (!GET_ACC().ToBoolean()) {
            SET_ACC(JSTaggedValue::True());
        } else {
            SET_ACC(JSTaggedValue::False());
        }
        DISPATCH(BytecodeInstruction::Format::PREF_NONE);
    }
    HANDLE_OPCODE(EXCEPTION_HANDLER) {
        FrameHandler frameHandler(thread);
        uint32_t pcOffset = panda_file::INVALID_OFFSET;
        for (; frameHandler.HasFrame(); frameHandler.PrevInterpretedFrame()) {
            if (frameHandler.IsEntryFrame()) {
                return;
            }
            auto method = frameHandler.GetMethod();
            pcOffset = FindCatchBlock(method, frameHandler.GetBytecodeOffset());
            if (pcOffset != panda_file::INVALID_OFFSET) {
                sp = frameHandler.GetSp();
                constpool = frameHandler.GetConstpool();
                // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                pc = method->GetBytecodeArray() + pcOffset;
                break;
            }
        }
        if (pcOffset == panda_file::INVALID_OFFSET) {
            return;
        }

        auto exception = thread->GetException();
        SET_ACC(exception);
        thread->ClearException();
        thread->SetCurrentSPFrame(sp);
        DISPATCH_OFFSET(0);
    }
    HANDLE_OPCODE(HANDLE_OVERFLOW) {
        LOG(FATAL, INTERPRETER) << "opcode overflow";
    }
#include "templates/debugger_instruction_handler.inl"
}

void EcmaInterpreter::InitStackFrame(JSThread *thread)
{
    if (thread->IsAsmInterpreter()) {
        return InterpreterAssembly::InitStackFrame(thread);
    }
    uint64_t *prevSp = const_cast<uint64_t *>(thread->GetCurrentSPFrame());
    InterpretedFrame *state = GET_FRAME(prevSp);
    state->pc = nullptr;
    state->function = JSTaggedValue::Hole();
    state->acc = JSTaggedValue::Hole();
    state->constpool = JSTaggedValue::Hole();
    state->profileTypeInfo = JSTaggedValue::Undefined();
    state->base.type = FrameType::INTERPRETER_FRAME;
    state->base.prev = nullptr;
}

uint32_t EcmaInterpreter::FindCatchBlock(JSMethod *caller, uint32_t pc)
{
    auto *pandaFile = caller->GetPandaFile();
    panda_file::MethodDataAccessor mda(*pandaFile, caller->GetMethodId());
    panda_file::CodeDataAccessor cda(*pandaFile, mda.GetCodeId().value());

    uint32_t pcOffset = panda_file::INVALID_OFFSET;
    cda.EnumerateTryBlocks([&pcOffset, pc](panda_file::CodeDataAccessor::TryBlock &try_block) {
        if ((try_block.GetStartPc() <= pc) && ((try_block.GetStartPc() + try_block.GetLength()) > pc)) {
            try_block.EnumerateCatchBlocks([&](panda_file::CodeDataAccessor::CatchBlock &catch_block) {
                pcOffset = catch_block.GetHandlerPc();
                return false;
            });
        }
        return pcOffset == panda_file::INVALID_OFFSET;
    });
    return pcOffset;
}

JSTaggedValue EcmaInterpreter::GetThisFunction(JSTaggedType *sp)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    return state->function;
}

JSTaggedValue EcmaInterpreter::GetNewTarget(JSTaggedType *sp)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    JSMethod *method = JSFunction::Cast(state->function.GetTaggedObject())->GetMethod();
    ASSERT(method->HaveNewTargetWithCallField());
    uint32_t numVregs = method->GetNumVregsWithCallField();
    bool haveFunc = method->HaveFuncWithCallField();
    return JSTaggedValue(sp[numVregs + haveFunc]);
}

uint32_t EcmaInterpreter::GetNumArgs(JSTaggedType *sp, uint32_t restIdx, uint32_t &startIdx)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    JSMethod *method = JSFunction::Cast(state->function.GetTaggedObject())->GetMethod();
    ASSERT(method->HaveExtraWithCallField());

    uint32_t numVregs = method->GetNumVregsWithCallField();
    bool haveFunc = method->HaveFuncWithCallField();
    bool haveNewTarget = method->HaveNewTargetWithCallField();
    bool haveThis = method->HaveThisWithCallField();
    uint32_t copyArgs = haveFunc + haveNewTarget + haveThis;
    uint32_t numArgs = method->GetNumArgsWithCallField();

    JSTaggedType *lastFrame = state->base.prev;
    // The prev frame of InterpretedFrame may entry frame or interpreter frame.
    if (FrameHandler::GetFrameType(state->base.prev) == FrameType::INTERPRETER_ENTRY_FRAME) {
        lastFrame = FrameHandler::GetInterpretedEntryFrameStart(state->base.prev);
    } else {
        lastFrame = lastFrame - INTERPRETER_FRAME_STATE_SIZE;
    }

    if (static_cast<uint32_t>(lastFrame - sp) > numVregs + copyArgs + numArgs) {
        // In this case, actualNumArgs is in the end
        // If not, then actualNumArgs == declaredNumArgs, therefore do nothing
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        numArgs = static_cast<uint32_t>(JSTaggedValue(*(lastFrame - 1)).GetInt());
    }
    startIdx = numVregs + copyArgs + restIdx;
    return ((numArgs > restIdx) ? (numArgs - restIdx) : 0);
}

size_t EcmaInterpreter::GetJumpSizeAfterCall(const uint8_t *prevPc)
{
    uint8_t op = *prevPc;
    size_t jumpSize;
    switch (op) {
        case (EcmaOpcode::CALLARG0DYN_PREF_V8):
            jumpSize = BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8);
            break;
        case (EcmaOpcode::CALLARG1DYN_PREF_V8_V8):
            jumpSize = BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8);
            break;
        case (EcmaOpcode::CALLARGS2DYN_PREF_V8_V8_V8):
            jumpSize = BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8_V8);
            break;
        case (EcmaOpcode::CALLARGS3DYN_PREF_V8_V8_V8_V8):
            jumpSize = BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8_V8_V8);
            break;
        case (EcmaOpcode::CALLIRANGEDYN_PREF_IMM16_V8):
            jumpSize = BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_IMM16_V8);
            break;
        case (EcmaOpcode::CALLITHISRANGEDYN_PREF_IMM16_V8):
        case (EcmaOpcode::NEWOBJDYNRANGE_PREF_IMM16_V8):
        case (EcmaOpcode::SUPERCALL_PREF_IMM16_V8):
            jumpSize = BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_IMM16_V8);
            break;
        default:
            UNREACHABLE();
    }

    return jumpSize;
}

JSTaggedValue EcmaInterpreter::GetRuntimeProfileTypeInfo(JSTaggedType *sp)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    return state->profileTypeInfo;
}

bool EcmaInterpreter::UpdateHotnessCounter(JSThread* thread, JSTaggedType *sp, JSTaggedValue acc, int32_t offset)
{
    InterpretedFrame *state = GET_FRAME(sp);
    auto method = JSFunction::Cast(state->function.GetTaggedObject())->GetMethod();
    auto hotnessCounter = method->GetHotnessCounter();

    hotnessCounter += offset;
    if (UNLIKELY(hotnessCounter <= 0)) {
        bool needRestoreAcc = false;
        SAVE_ACC();
        needRestoreAcc = thread->CheckSafepoint();
        RESTORE_ACC();
        if (state->profileTypeInfo == JSTaggedValue::Undefined()) {
            state->acc = acc;
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            auto thisFunc = state->function;
            auto res = SlowRuntimeStub::NotifyInlineCache(
                thread, JSFunction::Cast(thisFunc.GetHeapObject()), method);
            state->profileTypeInfo = res;
            method->SetHotnessCounter(EcmaInterpreter::METHOD_HOTNESS_THRESHOLD);
            return true;
        } else {
            method->SetHotnessCounter(EcmaInterpreter::METHOD_HOTNESS_THRESHOLD);
            return needRestoreAcc;
        }
    }
    method->SetHotnessCounter(hotnessCounter);
    return false;
}

// only use for fast new, not universal API
JSTaggedValue EcmaInterpreter::GetThisObjectFromFastNewFrame(JSTaggedType *sp)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    InterpretedFrame *state = reinterpret_cast<InterpretedFrame *>(sp) - 1;
    JSMethod *method = ECMAObject::Cast(state->function.GetTaggedObject())->GetCallTarget();
    ASSERT(method->OnlyHaveThisWithCallField() || method->OnlyHaveNewTagetAndThisWithCallField());
    uint32_t numVregs = method->GetNumVregsWithCallField();
    uint32_t numDeclaredArgs;
    if (method->OnlyHaveThisWithCallField()) {
        numDeclaredArgs = method->GetNumArgsWithCallField() + 1;  // 1: explicit this object
    } else {
        numDeclaredArgs = method->GetNumArgsWithCallField() + 2;  // 2: newTarget and explicit this object
    }
    uint32_t hiddenThisObjectIndex = numVregs + numDeclaredArgs;   // hidden this object in the end of fast new frame
    return JSTaggedValue(sp[hiddenThisObjectIndex]);
}

bool EcmaInterpreter::IsFastNewFrameEnter(JSFunction *ctor, JSMethod *method)
{
    if (method->IsNativeWithCallField()) {
        return false;
    }

    if (ctor->IsBase()) {
        return method->OnlyHaveThisWithCallField();
    }

    if (ctor->IsDerivedConstructor()) {
        return method->OnlyHaveNewTagetAndThisWithCallField();
    }

    return false;
}

bool EcmaInterpreter::IsFastNewFrameExit(JSTaggedType *sp)
{
    return GET_FRAME(sp)->base.type == FrameType::INTERPRETER_FAST_NEW_FRAME;
}

std::string GetEcmaOpcodeStr(EcmaOpcode opcode)
{
    const std::map<EcmaOpcode, const char *> strMap = {
        {LDNAN_PREF, "LDNAN"},
        {LDINFINITY_PREF, "LDINFINITY"},
        {LDGLOBALTHIS_PREF, "LDGLOBALTHIS"},
        {LDUNDEFINED_PREF, "LDUNDEFINED"},
        {LDNULL_PREF, "LDNULL"},
        {LDSYMBOL_PREF, "LDSYMBOL"},
        {LDGLOBAL_PREF, "LDGLOBAL"},
        {LDTRUE_PREF, "LDTRUE"},
        {LDFALSE_PREF, "LDFALSE"},
        {THROWDYN_PREF, "THROWDYN"},
        {TYPEOFDYN_PREF, "TYPEOFDYN"},
        {LDLEXENVDYN_PREF, "LDLEXENVDYN"},
        {POPLEXENVDYN_PREF, "POPLEXENVDYN"},
        {GETUNMAPPEDARGS_PREF, "GETUNMAPPEDARGS"},
        {GETPROPITERATOR_PREF, "GETPROPITERATOR"},
        {ASYNCFUNCTIONENTER_PREF, "ASYNCFUNCTIONENTER"},
        {LDHOLE_PREF, "LDHOLE"},
        {RETURNUNDEFINED_PREF, "RETURNUNDEFINED"},
        {CREATEEMPTYOBJECT_PREF, "CREATEEMPTYOBJECT"},
        {CREATEEMPTYARRAY_PREF, "CREATEEMPTYARRAY"},
        {GETITERATOR_PREF, "GETITERATOR"},
        {THROWTHROWNOTEXISTS_PREF, "THROWTHROWNOTEXISTS"},
        {THROWPATTERNNONCOERCIBLE_PREF, "THROWPATTERNNONCOERCIBLE"},
        {LDHOMEOBJECT_PREF, "LDHOMEOBJECT"},
        {THROWDELETESUPERPROPERTY_PREF, "THROWDELETESUPERPROPERTY"},
        {DEBUGGER_PREF, "DEBUGGER"},
        {ADD2DYN_PREF_V8, "ADD2DYN"},
        {SUB2DYN_PREF_V8, "SUB2DYN"},
        {MUL2DYN_PREF_V8, "MUL2DYN"},
        {DIV2DYN_PREF_V8, "DIV2DYN"},
        {MOD2DYN_PREF_V8, "MOD2DYN"},
        {EQDYN_PREF_V8, "EQDYN"},
        {NOTEQDYN_PREF_V8, "NOTEQDYN"},
        {LESSDYN_PREF_V8, "LESSDYN"},
        {LESSEQDYN_PREF_V8, "LESSEQDYN"},
        {GREATERDYN_PREF_V8, "GREATERDYN"},
        {GREATEREQDYN_PREF_V8, "GREATEREQDYN"},
        {SHL2DYN_PREF_V8, "SHL2DYN"},
        {ASHR2DYN_PREF_V8, "ASHR2DYN"},
        {SHR2DYN_PREF_V8, "SHR2DYN"},
        {AND2DYN_PREF_V8, "AND2DYN"},
        {OR2DYN_PREF_V8, "OR2DYN"},
        {XOR2DYN_PREF_V8, "XOR2DYN"},
        {TONUMBER_PREF_V8, "TONUMBER"},
        {NEGDYN_PREF_V8, "NEGDYN"},
        {NOTDYN_PREF_V8, "NOTDYN"},
        {INCDYN_PREF_V8, "INCDYN"},
        {DECDYN_PREF_V8, "DECDYN"},
        {EXPDYN_PREF_V8, "EXPDYN"},
        {ISINDYN_PREF_V8, "ISINDYN"},
        {INSTANCEOFDYN_PREF_V8, "INSTANCEOFDYN"},
        {STRICTNOTEQDYN_PREF_V8, "STRICTNOTEQDYN"},
        {STRICTEQDYN_PREF_V8, "STRICTEQDYN"},
        {RESUMEGENERATOR_PREF_V8, "RESUMEGENERATOR"},
        {GETRESUMEMODE_PREF_V8, "GETRESUMEMODE"},
        {CREATEGENERATOROBJ_PREF_V8, "CREATEGENERATOROBJ"},
        {THROWCONSTASSIGNMENT_PREF_V8, "THROWCONSTASSIGNMENT"},
        {GETTEMPLATEOBJECT_PREF_V8, "GETTEMPLATEOBJECT"},
        {GETNEXTPROPNAME_PREF_V8, "GETNEXTPROPNAME"},
        {CALLARG0DYN_PREF_V8, "CALLARG0DYN"},
        {THROWIFNOTOBJECT_PREF_V8, "THROWIFNOTOBJECT"},
        {ITERNEXT_PREF_V8, "ITERNEXT"},
        {CLOSEITERATOR_PREF_V8, "CLOSEITERATOR"},
        {COPYMODULE_PREF_V8, "COPYMODULE"},
        {SUPERCALLSPREAD_PREF_V8, "SUPERCALLSPREAD"},
        {DELOBJPROP_PREF_V8_V8, "DELOBJPROP"},
        {NEWOBJSPREADDYN_PREF_V8_V8, "NEWOBJSPREADDYN"},
        {CREATEITERRESULTOBJ_PREF_V8_V8, "CREATEITERRESULTOBJ"},
        {SUSPENDGENERATOR_PREF_V8_V8, "SUSPENDGENERATOR"},
        {ASYNCFUNCTIONAWAITUNCAUGHT_PREF_V8_V8, "ASYNCFUNCTIONAWAITUNCAUGHT"},
        {THROWUNDEFINEDIFHOLE_PREF_V8_V8, "THROWUNDEFINEDIFHOLE"},
        {CALLARG1DYN_PREF_V8_V8, "CALLARG1DYN"},
        {COPYDATAPROPERTIES_PREF_V8_V8, "COPYDATAPROPERTIES"},
        {STARRAYSPREAD_PREF_V8_V8, "STARRAYSPREAD"},
        {GETITERATORNEXT_PREF_V8_V8, "GETITERATORNEXT"},
        {SETOBJECTWITHPROTO_PREF_V8_V8, "SETOBJECTWITHPROTO"},
        {LDOBJBYVALUE_PREF_V8_V8, "LDOBJBYVALUE"},
        {STOBJBYVALUE_PREF_V8_V8, "STOBJBYVALUE"},
        {STOWNBYVALUE_PREF_V8_V8, "STOWNBYVALUE"},
        {LDSUPERBYVALUE_PREF_V8_V8, "LDSUPERBYVALUE"},
        {STSUPERBYVALUE_PREF_V8_V8, "STSUPERBYVALUE"},
        {LDOBJBYINDEX_PREF_V8_IMM32, "LDOBJBYINDEX"},
        {STOBJBYINDEX_PREF_V8_IMM32, "STOBJBYINDEX"},
        {STOWNBYINDEX_PREF_V8_IMM32, "STOWNBYINDEX"},
        {CALLSPREADDYN_PREF_V8_V8_V8, "CALLSPREADDYN"},
        {ASYNCFUNCTIONRESOLVE_PREF_V8_V8_V8, "ASYNCFUNCTIONRESOLVE"},
        {ASYNCFUNCTIONREJECT_PREF_V8_V8_V8, "ASYNCFUNCTIONREJECT"},
        {CALLARGS2DYN_PREF_V8_V8_V8, "CALLARGS2DYN"},
        {CALLARGS3DYN_PREF_V8_V8_V8_V8, "CALLARGS3DYN"},
        {DEFINEGETTERSETTERBYVALUE_PREF_V8_V8_V8_V8, "DEFINEGETTERSETTERBYVALUE"},
        {NEWOBJDYNRANGE_PREF_IMM16_V8, "NEWOBJDYNRANGE"},
        {CALLIRANGEDYN_PREF_IMM16_V8, "CALLIRANGEDYN"},
        {CALLITHISRANGEDYN_PREF_IMM16_V8, "CALLITHISRANGEDYN"},
        {SUPERCALL_PREF_IMM16_V8, "SUPERCALL"},
        {CREATEOBJECTWITHEXCLUDEDKEYS_PREF_IMM16_V8_V8, "CREATEOBJECTWITHEXCLUDEDKEYS"},
        {DEFINEFUNCDYN_PREF_ID16_IMM16_V8, "DEFINEFUNCDYN"},
        {DEFINENCFUNCDYN_PREF_ID16_IMM16_V8, "DEFINENCFUNCDYN"},
        {DEFINEGENERATORFUNC_PREF_ID16_IMM16_V8, "DEFINEGENERATORFUNC"},
        {DEFINEASYNCFUNC_PREF_ID16_IMM16_V8, "DEFINEASYNCFUNC"},
        {DEFINEMETHOD_PREF_ID16_IMM16_V8, "DEFINEMETHOD"},
        {NEWLEXENVDYN_PREF_IMM16, "NEWLEXENVDYN"},
        {COPYRESTARGS_PREF_IMM16, "COPYRESTARGS"},
        {CREATEARRAYWITHBUFFER_PREF_IMM16, "CREATEARRAYWITHBUFFER"},
        {CREATEOBJECTHAVINGMETHOD_PREF_IMM16, "CREATEOBJECTHAVINGMETHOD"},
        {THROWIFSUPERNOTCORRECTCALL_PREF_IMM16, "THROWIFSUPERNOTCORRECTCALL"},
        {CREATEOBJECTWITHBUFFER_PREF_IMM16, "CREATEOBJECTWITHBUFFER"},
        {LDLEXVARDYN_PREF_IMM4_IMM4, "LDLEXVARDYN"},
        {LDLEXVARDYN_PREF_IMM8_IMM8, "LDLEXVARDYN"},
        {LDLEXVARDYN_PREF_IMM16_IMM16, "LDLEXVARDYN"},
        {STLEXVARDYN_PREF_IMM4_IMM4_V8, "STLEXVARDYN"},
        {STLEXVARDYN_PREF_IMM8_IMM8_V8, "STLEXVARDYN"},
        {STLEXVARDYN_PREF_IMM16_IMM16_V8, "STLEXVARDYN"},
        {NEWLEXENVWITHNAMEDYN_PREF_IMM16_IMM16, "NEWLEXENVWITHNAMEDYN"},
        {DEFINECLASSWITHBUFFER_PREF_ID16_IMM16_IMM16_V8_V8, "DEFINECLASSWITHBUFFER"},
        {GETMODULENAMESPACE_PREF_ID32, "GETMODULENAMESPACE"},
        {STMODULEVAR_PREF_ID32, "STMODULEVAR"},
        {TRYLDGLOBALBYNAME_PREF_ID32, "TRYLDGLOBALBYNAME"},
        {TRYSTGLOBALBYNAME_PREF_ID32, "TRYSTGLOBALBYNAME"},
        {LDGLOBALVAR_PREF_ID32, "LDGLOBALVAR"},
        {STGLOBALVAR_PREF_ID32, "STGLOBALVAR"},
        {LDOBJBYNAME_PREF_ID32_V8, "LDOBJBYNAME"},
        {STOBJBYNAME_PREF_ID32_V8, "STOBJBYNAME"},
        {STOWNBYNAME_PREF_ID32_V8, "STOWNBYNAME"},
        {LDSUPERBYNAME_PREF_ID32_V8, "LDSUPERBYNAME"},
        {STSUPERBYNAME_PREF_ID32_V8, "STSUPERBYNAME"},
        {LDMODULEVAR_PREF_ID32_IMM8, "LDMODULEVAR"},
        {CREATEREGEXPWITHLITERAL_PREF_ID32_IMM8, "CREATEREGEXPWITHLITERAL"},
        {ISTRUE_PREF, "ISTRUE"},
        {ISFALSE_PREF, "ISFALSE"},
        {STCONSTTOGLOBALRECORD_PREF_ID32, "STCONSTTOGLOBALRECORD"},
        {STLETTOGLOBALRECORD_PREF_ID32, "STLETTOGLOBALRECORD"},
        {STCLASSTOGLOBALRECORD_PREF_ID32, "STCLASSTOGLOBALRECORD"},
        {STOWNBYVALUEWITHNAMESET_PREF_V8_V8, "STOWNBYVALUEWITHNAMESET"},
        {STOWNBYNAMEWITHNAMESET_PREF_ID32_V8, "STOWNBYNAMEWITHNAMESET"},
        {LDFUNCTION_PREF, "LDFUNCTION"},
        {LDBIGINT_PREF_ID32, "LDBIGINT"},
        {MOV_DYN_V8_V8, "MOV_DYN"},
        {MOV_DYN_V16_V16, "MOV_DYN"},
        {LDA_STR_ID32, "LDA_STR"},
        {LDAI_DYN_IMM32, "LDAI_DYN"},
        {FLDAI_DYN_IMM64, "FLDAI_DYN"},
        {JMP_IMM8, "JMP"},
        {JMP_IMM16, "JMP"},
        {JMP_IMM32, "JMP"},
        {JEQZ_IMM8, "JEQZ"},
        {JEQZ_IMM16, "JEQZ"},
        {LDA_DYN_V8, "LDA_DYN"},
        {STA_DYN_V8, "STA_DYN"},
        {RETURN_DYN, "RETURN_DYN"},
        {MOV_V4_V4, "MOV"},
        {JNEZ_IMM8, "JNEZ"},
        {JNEZ_IMM16, "JNEZ"},
        {LAST_OPCODE, "LAST_OPCODE"},
    };
    if (strMap.count(opcode) > 0) {
        return strMap.at(opcode);
    }
    return "bytecode-" + std::to_string(opcode);
}
#undef LOG_INST
#undef HANDLE_OPCODE
#undef ADVANCE_PC
#undef GOTO_NEXT
#undef DISPATCH
#undef DISPATCH_OFFSET
#undef GET_FRAME
#undef GET_ENTRY_FRAME
#undef GET_ENTRY_FRAME_WITH_ARGS_SIZE
#undef SAVE_PC
#undef SAVE_ACC
#undef RESTORE_ACC
#undef INTERPRETER_GOTO_EXCEPTION_HANDLER
#undef INTERPRETER_HANDLE_RETURN
#undef CHECK_SWITCH_TO_DEBUGGER_TABLE
#undef REAL_GOTO_DISPATCH_OPCODE
#undef REAL_GOTO_EXCEPTION_HANDLER
#undef INTERPRETER_RETURN_IF_ABRUPT
#undef NOTIFY_DEBUGGER_EVENT
#undef CALL_INITIALIZE
#undef CALL_PUSH_UNDEFINED
#undef CALL_PUSH_ARGS_0
#undef CALL_PUSH_ARGS_1
#undef CALL_PUSH_ARGS_2
#undef CALL_PUSH_ARGS_3
#undef CALL_PUSH_ARGS_I
#undef CALL_PUSH_ARGS_I_THIS
#undef CALL_PUSH_ARGS_0_NO_EXTRA
#undef CALL_PUSH_ARGS_1_NO_EXTRA
#undef CALL_PUSH_ARGS_2_NO_EXTRA
#undef CALL_PUSH_ARGS_3_NO_EXTRA
#undef CALL_PUSH_ARGS_I_NO_EXTRA
#undef CALL_PUSH_ARGS_I_THIS_NO_EXTRA
#undef CALL_PUSH_ARGS
#undef UPDATE_HOTNESS_COUNTER_NON_ACC
#undef UPDATE_HOTNESS_COUNTER
#undef GET_VREG
#undef GET_VREG_VALUE
#undef SET_VREG
#undef GET_ACC
#undef SET_ACC
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_INTERPRETER_INTERPRETER_INL_H
