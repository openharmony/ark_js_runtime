#include "compile_bridge.h"

namespace panda::ecmascript {
uint64_t CallCompiledJSFunction(JSThread *thread, const CallParams &params)
{
    JSMethod* method = params->callTarget->GetCallTarget();
    uint64_t expectedNumArgs = method->GetNumArgs();
    MachineCode *code = MachineCode::Cast(jsFunc->GetCodeEntry());
    uintptr_t glue = thread->GetGlueAddr();
    return CallJSFunctionWithArray(glue, params, expectedNumArgs,
        params.argc, params.argv, code->GetCodeOffset());
}

uint64_t CallJSFunctionWithArray(uintptr_t glue, const CallParams &params, uint32_t expectedNumArgs,
                                 uint32_t actualNumArgs, const uint64_t argV[], uintptr_t codeAddr)

    register uint64_t i asm("r10");
    // copy undefined value on stack
    for (i = expectedNumArgs; i > actualNumArgs; i--) {
        asm("push 0xa\n");
    }
    // copy argument on stack
    uint32_t copyNumArgs = std::min(expectedNumArgs, actualNumArgs);
    for (uint32_t i = copyNumArgs; i > 0; i--) {
        asm("push %0\n"::"r"(argV[i - 1]));
    }
    // copy this new_target, func
    asm("push %0\n"::"r"(params.callTarget));
    asm("push %0\n"::"r"(params.newTarget));
    asm("push %0\n"::"r"(params.thisArg));
    i = InvokeCompiledJSFunction(glue, codeAddr);
    asm("addl %0, %%rsp\n"::"r"((expectedNumArgs + 3) * 8));
    return i;
}
}  // panda::ecmascript
