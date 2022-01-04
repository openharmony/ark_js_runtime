#include "interpreter_stub.h"

#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/compiler/machine_type.h"
#include "ecmascript/js_array.h"
#include "ecmascript/message_string.h"
#include "ecmascript/tagged_hash_table-inl.h"

namespace panda::ecmascript::kungfu {

void HandleLdnanPrefStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedArgument(4); /* 4 : 5rd parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    
    // acc = DoubleBuildTaggedWithNoGC(GetDoubleConstant(base::NAN_VALUE));
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc.Value(), hotnessCounter);
}

void SingleStepDebuggingStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedArgument(4); /* 4 : 5rd parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc.Value(), hotnessCounter);
}
}  // namespace panda::ecmascript::kungfu
