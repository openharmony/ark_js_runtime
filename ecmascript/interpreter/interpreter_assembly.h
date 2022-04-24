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

#ifndef ECMASCRIPT_INTERPRETER_INTERPRETER_ASSEMBLY_64BIT_H
#define ECMASCRIPT_INTERPRETER_INTERPRETER_ASSEMBLY_64BIT_H

#include "ecmascript/base/config.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_method.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/frames.h"

namespace panda::ecmascript {
using TaggedType = coretypes::TaggedType;
using DispatchEntryPoint =
    void (*)(JSThread *, const uint8_t *, JSTaggedType *, JSTaggedValue, JSTaggedValue, JSTaggedValue, int32_t);
class ConstantPool;
class ECMAObject;
class GeneratorContext;
struct CallParams;

class InterpreterAssembly {
public:
    static const uint32_t METHOD_HOTNESS_THRESHOLD = 512;
    enum ActualNumArgsOfCall : uint8_t { CALLARG0 = 0, CALLARG1, CALLARGS2, CALLARGS3 };
    static void RunInternal(JSThread *thread, ConstantPool *constpool, const uint8_t *pc, JSTaggedType *sp);
    static void InitStackFrame(JSThread *thread);
    static JSTaggedValue Execute(EcmaRuntimeCallInfo *info);
    static JSTaggedValue ExecuteNative(EcmaRuntimeCallInfo *info);
    static JSTaggedValue GeneratorReEnterInterpreter(JSThread *thread, JSHandle<GeneratorContext> context);
    static void ChangeGenContext(JSThread *thread, JSHandle<GeneratorContext> context);
    static void ResumeContext(JSThread *thread);
    static uint32_t FindCatchBlock(JSMethod *caller, uint32_t pc);
    static inline size_t GetJumpSizeAfterCall(const uint8_t *prevPc);

    static inline JSTaggedValue UpdateHotnessCounter(JSThread* thread, TaggedType *sp);
    static inline void InterpreterFrameCopyArgs(JSTaggedType *newSp, uint32_t numVregs, uint32_t numActualArgs,
                                                uint32_t numDeclaredArgs, bool haveExtraArgs = true);
    static JSTaggedValue GetThisFunction(JSTaggedType *sp);
    static JSTaggedValue GetNewTarget(JSTaggedType *sp);
    static uint32_t GetNumArgs(JSTaggedType *sp, uint32_t restIdx, uint32_t &startIdx);
    static inline JSTaggedValue GetThisObjectFromFastNewFrame(JSTaggedType *sp);
    static inline bool IsFastNewFrameEnter(JSMethod *method);
    static inline bool IsFastNewFrameExit(JSTaggedType *sp);

    static void HandleOverflow(JSThread *thread, const uint8_t *pc, JSTaggedType *sp,
                               JSTaggedValue constpool, JSTaggedValue profileTypeInfo,
                               JSTaggedValue acc, int32_t hotnessCounter);

#define DEF_HANDLER(name, counter)                                           \
    static void name(JSThread *thread, const uint8_t *pc, JSTaggedType *sp,  \
                     JSTaggedValue constpool, JSTaggedValue profileTypeInfo, \
                     JSTaggedValue acc, int32_t hotnessCounter);
    ASM_INTERPRETER_BC_STUB_ID_LIST(DEF_HANDLER)
#undef DEF_HANDLER
};

static std::array<DispatchEntryPoint, BCStubEntries::BC_HANDLER_STUB_ENTRIES_COUNT> asmDispatchTable {
#define DEF_HANDLER(name, counter) InterpreterAssembly::name,
    ASM_INTERPRETER_BC_STUB_ID_LIST(DEF_HANDLER)
#undef DEF_HANDLER
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
    InterpreterAssembly::HandleOverflow,
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_INTERPRETER_INTERPRETER_ASSEMBLY_64BIT_H
