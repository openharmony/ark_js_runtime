#include "interpreter_stub.h"

#include "ecmascript/base/number_helper.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/compiler/machine_type.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/message_string.h"
#include "ecmascript/tagged_hash_table-inl.h"

namespace panda::ecmascript::kungfu {

void HandleLdNanPrefStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    
    acc = DoubleBuildTaggedWithNoGC(GetDoubleConstant(base::NAN_VALUE));
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_NONE)));
}

void HandleLdInfinityPrefStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5rd parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    acc = DoubleBuildTaggedWithNoGC(GetDoubleConstant(base::POSITIVE_INFINITY));
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_NONE)));
}

void HandleLdUndefinedPrefStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5rd parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    
    acc = GetUndefinedConstant();
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_NONE)));
}

void HandleLdNullPrefStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5rd parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    
    acc = GetNullConstant();
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_NONE)));
}

void HandleLdTruePrefStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5rd parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    
    acc = ChangeInt64ToTagged(GetWord64Constant(JSTaggedValue::VALUE_TRUE));
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_NONE)));
}

void HandleLdFalsePrefStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5rd parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    
    acc = ChangeInt64ToTagged(GetWord64Constant(JSTaggedValue::VALUE_FALSE));
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_NONE)));
}

void HandleThrowDynPrefStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5rd parameter is value */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    
    StubDescriptor *throwDyn = GET_STUBDESCRIPTOR(ThrowDyn);
    CallRuntime(throwDyn, glue, GetWord64Constant(FAST_STUB_ID(ThrowDyn)),
                {glue, acc});
    DispatchLast(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter, GetArchRelateConstant(0));
}

void HandleLdLexEnvDynPrefStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5rd parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    
    GateRef state = GetFrame(sp);
    acc = GetEnvFromFrame(state);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_NONE)));
}

void HandlePopLexEnvDynPrefStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5rd parameter is value */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    
    GateRef state = GetFrame(sp);
    GateRef currentLexEnv = GetEnvFromFrame(state);
    GateRef parentLexEnv = GetParentEnv(currentLexEnv);
    SetEnvToFrame(glue, state, parentLexEnv);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_NONE)));
}

void HandleLdHolePrefStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5rd parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    
    acc = GetHoleConstant();
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_NONE)));
}

void HandleLdHomeObjectPrefStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5rd parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    
    GateRef thisFunc = GetFunctionFromFrame(GetFrame(sp));
    acc = GetHomeObjectFromJSFunction(thisFunc);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_NONE)));
}

void HandleDebuggerPrefStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5rd parameter is value */
    GateRef acc = TaggedArgument(5);
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_NONE)));
}

void SingleStepDebuggingStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    DEFVARIABLE(pc, MachineType::NATIVE_POINTER, PtrArgument(1));
    DEFVARIABLE(sp, MachineType::NATIVE_POINTER, PtrArgument(2)); /* 2 : 3rd parameter is value */
    DEFVARIABLE(constpool, MachineType::TAGGED_POINTER, TaggedPointerArgument(3)); /* 3: 4th parameter is value */
    DEFVARIABLE(profileTypeInfo, MachineType::TAGGED_POINTER, TaggedPointerArgument(4)); /* 4: 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    auto stubDescriptor = GET_STUBDESCRIPTOR(JumpToCInterpreter);
    pc = CallRuntime(stubDescriptor, glue, GetWord64Constant(FAST_STUB_ID(JumpToCInterpreter)), {
        glue, *pc, *sp, *constpool, *profileTypeInfo, *acc, hotnessCounter
    });
    Label shouldReturn(env);
    Label shouldContinue(env);

    // if (pc == nullptr) return
    Branch(PtrEqual(*pc, GetArchRelateConstant(0)), &shouldReturn, &shouldContinue);
    Bind(&shouldReturn);
    {
        Return();
    }
    // constpool = frame->constpool
    // sp = glue->currentFrame
    // profileTypeInfo = frame->profileTypeInfo
    Bind(&shouldContinue);
    {
        GateRef spOffset = GetArchRelateConstant(cfg->GetGlueOffset(JSThread::GlueID::CURRENT_FRAME));
        sp = Load(MachineType::NATIVE_POINTER, glue, spOffset);

        GateRef frameSize = GetArchRelateConstant(cfg->GetGlueOffset(JSThread::GlueID::FRAME_STATE_SIZE));
        GateRef frame = PtrSub(*sp, frameSize);

        GateRef profileOffset = GetArchRelateConstant(cfg->GetGlueOffset(JSThread::GlueID::GLUE_FRAME_PROFILE));
        GateRef constpoolOffset = GetArchRelateConstant(cfg->GetGlueOffset(JSThread::GlueID::GLUE_FRAME_CONSTPOOL));
        GateRef accOffset = GetArchRelateConstant(cfg->GetGlueOffset(JSThread::GlueID::GLUE_FRAME_ACC));
        profileTypeInfo = Load(MachineType::TAGGED, frame, profileOffset);
        constpool = Load(MachineType::TAGGED, frame, constpoolOffset);
        acc = Load(MachineType::TAGGED, frame, accOffset);
    }
    // do not update hotnessCounter now
    Dispatch(glue, *pc, *sp, *constpool, *profileTypeInfo, *acc,
             hotnessCounter, GetArchRelateConstant(0));
}

void HandleLdaDynV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef vsrc = ReadInst8_0(pc);
    acc = GetVregValue(sp, ZExtInt8ToPtr(vsrc));
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::V8)));
}

void HandleStaDynV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef vdst = ReadInst8_0(pc);
    SetVregValue(glue, sp, ZExtInt8ToPtr(vdst), acc);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::V8)));
}

void HandleJmpImm8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    DEFVARIABLE(profileTypeInfo, MachineType::TAGGED_POINTER, TaggedPointerArgument(4)); /* 4: 5th parameter is value */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is value */
    DEFVARIABLE(hotnessCounter, MachineType::INT32, TaggedArgument(6)); /* 6: 7th parameter is value */
    Label slowPath(env);
    Label dispatch(env);

    GateRef offset = ReadInstSigned8_0(pc);
    hotnessCounter = Int32Add(offset, *hotnessCounter);
    Branch(Int32LessThan(*hotnessCounter, GetInt32Constant(0)), &slowPath, &dispatch);

    Bind(&slowPath);
    {
        StubDescriptor *setClassConstructorLength = GET_STUBDESCRIPTOR(UpdateHotnessCounter);
        profileTypeInfo = CallRuntime(setClassConstructorLength, glue,
            GetWord64Constant(FAST_STUB_ID(UpdateHotnessCounter)), { glue, sp });
        Jump(&dispatch);
    }
    Bind(&dispatch);
    Dispatch(glue, pc, sp, constpool, *profileTypeInfo, acc, *hotnessCounter, SExtInt32ToPtr(offset));
}

void HandleJmpImm16Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    DEFVARIABLE(profileTypeInfo, MachineType::TAGGED_POINTER, TaggedPointerArgument(4)); /* 4: 5th parameter is value */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is value */
    DEFVARIABLE(hotnessCounter, MachineType::INT32, TaggedArgument(6)); /* 6: 7th parameter is value */

    Label slowPath(env);
    Label dispatch(env);

    GateRef offset = ReadInstSigned16_0(pc);
    hotnessCounter = Int32Add(offset, *hotnessCounter);
    Branch(Int32LessThan(*hotnessCounter, GetInt32Constant(0)), &slowPath, &dispatch);

    Bind(&slowPath);
    {
        StubDescriptor *setClassConstructorLength = GET_STUBDESCRIPTOR(UpdateHotnessCounter);
        profileTypeInfo = CallRuntime(setClassConstructorLength, glue,
            GetWord64Constant(FAST_STUB_ID(UpdateHotnessCounter)), { glue, sp });
        Jump(&dispatch);
    }
    Bind(&dispatch);
    Dispatch(glue, pc, sp, constpool, *profileTypeInfo, acc, *hotnessCounter, SExtInt32ToPtr(offset));
}

void HandleJmpImm32Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    DEFVARIABLE(profileTypeInfo, MachineType::TAGGED_POINTER, TaggedPointerArgument(4)); /* 4: 5th parameter is value */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is value */
    DEFVARIABLE(hotnessCounter, MachineType::INT32, TaggedArgument(6)); /* 6: 7th parameter is value */

    Label slowPath(env);
    Label dispatch(env);

    GateRef offset = ReadInstSigned32_0(pc);
    hotnessCounter = Int32Add(offset, *hotnessCounter);
    Branch(Int32LessThan(*hotnessCounter, GetInt32Constant(0)), &slowPath, &dispatch);

    Bind(&slowPath);
    {
        StubDescriptor *setClassConstructorLength = GET_STUBDESCRIPTOR(UpdateHotnessCounter);
        profileTypeInfo = CallRuntime(setClassConstructorLength, glue,
            GetWord64Constant(FAST_STUB_ID(UpdateHotnessCounter)), { glue, sp });
        Jump(&dispatch);
    }
    Bind(&dispatch);
    Dispatch(glue, pc, sp, constpool, *profileTypeInfo, acc, *hotnessCounter, SExtInt32ToPtr(offset));
}

void HandleLdLexVarDynPrefImm4Imm4Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is sp */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is constpool */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is profileTypeInfo */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is acc */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is hotnessCounter */

    GateRef level = ZExtInt8ToInt32(ReadInst4_2(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst4_3(pc));

    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, MachineType::TAGGED, GetEnvFromFrame(state));
    DEFVARIABLE(i, MachineType::INT32, GetInt32Constant(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, GetInt32Constant(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    GateRef variable = GetPropertiesFromLexicalEnv(*currentEnv, slot);
    acc = variable;
    // do not update hotnessCounter now
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_IMM4_IMM4)));
}

void HandleLdLexVarDynPrefImm8Imm8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is sp */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is constpool */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is profileTypeInfo */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is acc */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is hotnessCounter */

    GateRef level = ZExtInt8ToInt32(ReadInst8_1(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst8_2(pc));

    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, MachineType::TAGGED, GetEnvFromFrame(state));
    DEFVARIABLE(i, MachineType::INT32, GetInt32Constant(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, GetInt32Constant(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    GateRef variable = GetPropertiesFromLexicalEnv(*currentEnv, slot);
    acc = variable;
    // do not update hotnessCounter now
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_IMM8_IMM8)));
}

void HandleLdLexVarDynPrefImm16Imm16Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is sp */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is constpool */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is profileTypeInfo */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is acc */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is hotnessCounter */

    GateRef level = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef slot = ZExtInt16ToInt32(ReadInst16_3(pc));

    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, MachineType::TAGGED, GetEnvFromFrame(state));
    DEFVARIABLE(i, MachineType::INT32, GetInt32Constant(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, GetInt32Constant(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    GateRef variable = GetPropertiesFromLexicalEnv(*currentEnv, slot);
    acc = variable;
    // do not update hotnessCounter now
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_IMM16_IMM16)));
}

void HandleStLexVarDynPrefImm4Imm4V8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is sp */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is constpool */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is profileTypeInfo */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is acc */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is hotnessCounter */

    GateRef level = ZExtInt8ToInt32(ReadInst4_2(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst4_3(pc));
    GateRef v0 = ReadInst8_2(pc);

    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, MachineType::TAGGED, GetEnvFromFrame(state));
    DEFVARIABLE(i, MachineType::INT32, GetInt32Constant(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, GetInt32Constant(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, value);
    // do not update hotnessCounter now
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_IMM4_IMM4_V8)));
}

void HandleStLexVarDynPrefImm8Imm8V8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is sp */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is constpool */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is profileTypeInfo */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is acc */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is hotnessCounter */

    GateRef level = ZExtInt8ToInt32(ReadInst8_1(pc));
    GateRef slot = ZExtInt8ToInt32(ReadInst8_2(pc));
    GateRef v0 = ReadInst8_3(pc);

    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, MachineType::TAGGED, GetEnvFromFrame(state));
    DEFVARIABLE(i, MachineType::INT32, GetInt32Constant(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, GetInt32Constant(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, value);
    // do not update hotnessCounter now
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_IMM8_IMM8_V8)));
}

void HandleStLexVarDynPrefImm16Imm16V8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is sp */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is constpool */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is profileTypeInfo */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is acc */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is hotnessCounter */

    GateRef level = ZExtInt16ToInt32(ReadInst16_1(pc));
    GateRef slot = ZExtInt16ToInt32(ReadInst16_3(pc));
    GateRef v0 = ReadInst8_5(pc);

    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef state = GetFrame(sp);
    DEFVARIABLE(currentEnv, MachineType::TAGGED, GetEnvFromFrame(state));
    DEFVARIABLE(i, MachineType::INT32, GetInt32Constant(0));

    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Branch(Int32LessThan(*i, level), &loopHead, &afterLoop);
    LoopBegin(&loopHead);
    currentEnv = GetParentEnv(*currentEnv);
    i = Int32Add(*i, GetInt32Constant(1));
    Branch(Int32LessThan(*i, level), &loopEnd, &afterLoop);
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    SetPropertiesToLexicalEnv(glue, *currentEnv, slot, value);
    // do not update hotnessCounter now
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_IMM16_IMM16_V8)));
}

void HandleIncDynPrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    Label valueIsInt(env);
    Label valueNotInt(env);
    Label accDispatch(env);
    Branch(TaggedIsInt(value), &valueIsInt, &valueNotInt);
    Bind(&valueIsInt);
    {
        GateRef valueInt = TaggedCastToInt32(value);
        Label valueOverflow(env);
        Label valueNoOverflow(env);
        Branch(Word32Equal(valueInt, GetInt32Constant(INT32_MAX)), &valueOverflow, &valueNoOverflow);
        Bind(&valueOverflow);
        {
            GateRef valueDoubleFromInt = ChangeInt32ToFloat64(valueInt);
            acc = DoubleBuildTaggedWithNoGC(DoubleAdd(valueDoubleFromInt, GetDoubleConstant(1.0)));
            Jump(&accDispatch);
        }
        Bind(&valueNoOverflow);
        {
            acc = IntBuildTaggedWithNoGC(Int32Add(valueInt, GetInt32Constant(1)));
            Jump(&accDispatch);
        }
    }
    Bind(&valueNotInt);
    Label valueIsDouble(env);
    Label valueNotDouble(env);
    Branch(TaggedIsDouble(value), &valueIsDouble, &valueNotDouble);
    Bind(&valueIsDouble);
    {
        GateRef valueDouble = TaggedCastToDouble(value);
        acc = DoubleBuildTaggedWithNoGC(DoubleAdd(valueDouble, GetDoubleConstant(1.0)));
        Jump(&accDispatch);
    }
    Bind(&valueNotDouble);
    {
        // slow path
        StubDescriptor *incDyn = GET_STUBDESCRIPTOR(IncDyn);
        GateRef result = CallRuntime(incDyn, glue, GetWord64Constant(FAST_STUB_ID(IncDyn)),
                                     {glue, value});
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(result), &isException, &notException);
        Bind(&isException);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(0));
        }
        Bind(&notException);
        acc = result;
        Jump(&accDispatch);
    }

    Bind(&accDispatch);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
}

void HandleDecDynPrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    Label valueIsInt(env);
    Label valueNotInt(env);
    Label accDispatch(env);
    Branch(TaggedIsInt(value), &valueIsInt, &valueNotInt);
    Bind(&valueIsInt);
    {
        GateRef valueInt = TaggedCastToInt32(value);
        Label valueOverflow(env);
        Label valueNoOverflow(env);
        Branch(Word32Equal(valueInt, GetInt32Constant(INT32_MIN)), &valueOverflow, &valueNoOverflow);
        Bind(&valueOverflow);
        {
            GateRef valueDoubleFromInt = ChangeInt32ToFloat64(valueInt);
            acc = DoubleBuildTaggedWithNoGC(DoubleSub(valueDoubleFromInt, GetDoubleConstant(1.0)));
            Jump(&accDispatch);
        }
        Bind(&valueNoOverflow);
        {
            acc = IntBuildTaggedWithNoGC(Int32Sub(valueInt, GetInt32Constant(1)));
            Jump(&accDispatch);
        }
    }
    Bind(&valueNotInt);
    Label valueIsDouble(env);
    Label valueNotDouble(env);
    Branch(TaggedIsDouble(value), &valueIsDouble, &valueNotDouble);
    Bind(&valueIsDouble);
    {
        GateRef valueDouble = TaggedCastToDouble(value);
        acc = DoubleBuildTaggedWithNoGC(DoubleSub(valueDouble, GetDoubleConstant(1.0)));
        Jump(&accDispatch);
    }
    Bind(&valueNotDouble);
    {
        // slow path
        StubDescriptor *decDyn = GET_STUBDESCRIPTOR(DecDyn);
        GateRef result = CallRuntime(decDyn, glue, GetWord64Constant(FAST_STUB_ID(DecDyn)),
                                     {glue, value});
        Label isException(env);
        Label notException(env);
        Branch(TaggedIsException(result), &isException, &notException);
        Bind(&isException);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(0));
        }
        Bind(&notException);
        acc = result;
        Jump(&accDispatch);
    }

    Bind(&accDispatch);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
}

void HandleExpDynPrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef base = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *expDyn = GET_STUBDESCRIPTOR(ExpDyn);
    GateRef result = CallRuntime(expDyn, glue, GetWord64Constant(FAST_STUB_ID(ExpDyn)),
                                 {glue, base, *acc}); // acc is exponent
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(0));
    }
    Bind(&notException);
    acc = result;
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
}

void HandleIsInDynPrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef prop = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *isInDyn = GET_STUBDESCRIPTOR(IsInDyn);
    GateRef result = CallRuntime(isInDyn, glue, GetWord64Constant(FAST_STUB_ID(IsInDyn)),
                                 {glue, prop, *acc}); // acc is obj
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(0));
    }
    Bind(&notException);
    acc = result;
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
}

void HandleInstanceOfDynPrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *instanceOfDyn = GET_STUBDESCRIPTOR(InstanceOfDyn);
    GateRef result = CallRuntime(instanceOfDyn, glue, GetWord64Constant(FAST_STUB_ID(InstanceOfDyn)),
                                 {glue, obj, *acc}); // acc is target
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(0));
    }
    Bind(&notException);
    acc = result;
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
}

void HandleStrictNotEqDynPrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *fastStrictNotEqual = GET_STUBDESCRIPTOR(FastStrictNotEqual);
    GateRef result = CallRuntime(fastStrictNotEqual, glue, GetWord64Constant(FAST_STUB_ID(FastStrictNotEqual)),
                                 {left, *acc}); // acc is right
    acc = result;
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
}

void HandleStrictEqDynPrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef left = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *fastStrictEqual = GET_STUBDESCRIPTOR(FastStrictEqual);
    GateRef result = CallRuntime(fastStrictEqual, glue, GetWord64Constant(FAST_STUB_ID(FastStrictEqual)),
                                 {left, *acc}); // acc is right
    acc = result;
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
}

void HandleResumeGeneratorPrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef vs = ReadInst8_1(pc);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(vs));
    GateRef resumeResultOffset = GetArchRelateConstant(JSGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET);
    acc = Load(MachineType::TAGGED, obj, resumeResultOffset);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
}

void HandleGetResumeModePrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef vs = ReadInst8_1(pc);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(vs));
    GateRef resumeModeOffset = GetArchRelateConstant(JSGeneratorObject::GENERATOR_RESUME_MODE_OFFSET);
    acc = Load(MachineType::TAGGED, obj, resumeModeOffset);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
}

void HandleCreateGeneratorObjPrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef genFunc = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *createGeneratorObj = GET_STUBDESCRIPTOR(CreateGeneratorObj);
    GateRef result = CallRuntime(createGeneratorObj, glue, GetWord64Constant(FAST_STUB_ID(CreateGeneratorObj)),
                                 {glue, genFunc});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(0));
    }
    Bind(&notException);
    acc = result;
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
}

void HandleThrowConstAssignmentPrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *throwConstAssignment = GET_STUBDESCRIPTOR(ThrowConstAssignment);
    CallRuntime(throwConstAssignment, glue, GetWord64Constant(FAST_STUB_ID(ThrowConstAssignment)),
                {glue, value});
    DispatchLast(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter, GetArchRelateConstant(0));
}

void HandleGetTemplateObjectPrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef literal = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *getTemplateObject = GET_STUBDESCRIPTOR(GetTemplateObject);
    GateRef result = CallRuntime(getTemplateObject, glue, GetWord64Constant(FAST_STUB_ID(GetTemplateObject)),
                                 {glue, literal});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(0));
    }
    Bind(&notException);
    acc = result;
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
}

void HandleGetNextPropNamePrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef iter = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *getNextPropName = GET_STUBDESCRIPTOR(GetNextPropName);
    GateRef result = CallRuntime(getNextPropName, glue, GetWord64Constant(FAST_STUB_ID(GetNextPropName)),
                                 {glue, iter});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(0));
    }
    Bind(&notException);
    acc = result;
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
}

void HandleThrowIfNotObjectPrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    Label isEcmaObject(env);
    Label notEcmaObject(env);
    Branch(IsEcmaObject(value), &isEcmaObject, &notEcmaObject);
    Bind(&isEcmaObject);
    {
        Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,
                 GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
    }
    Bind(&notEcmaObject);
    StubDescriptor *throwIfNotObject = GET_STUBDESCRIPTOR(ThrowIfNotObject);
    CallRuntime(throwIfNotObject, glue, GetWord64Constant(FAST_STUB_ID(ThrowIfNotObject)),
                {glue});
    DispatchLast(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter, GetArchRelateConstant(0));
}

void HandleIterNextPrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef iter = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *iterNext = GET_STUBDESCRIPTOR(IterNext);
    GateRef result = CallRuntime(iterNext, glue, GetWord64Constant(FAST_STUB_ID(IterNext)),
                                 {glue, iter});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(0));
    }
    Bind(&notException);
    acc = result;
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
}

void HandleCloseIteratorPrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef iter = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *closeIterator = GET_STUBDESCRIPTOR(CloseIterator);
    GateRef result = CallRuntime(closeIterator, glue, GetWord64Constant(FAST_STUB_ID(CloseIterator)),
                                 {glue, iter});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(0));
    }
    Bind(&notException);
    acc = result;
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
}

void HandleCopyModulePrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef srcModule = GetVregValue(sp, ZExtInt8ToPtr(v0));
    StubDescriptor *copyModule = GET_STUBDESCRIPTOR(CopyModule);
    CallRuntime(copyModule, glue, GetWord64Constant(FAST_STUB_ID(CopyModule)),
                {glue, srcModule});
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
}

void HandleDelObjPropPrefV8V8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef prop = GetVregValue(sp, ZExtInt8ToPtr(v1));
    StubDescriptor *delObjProp = GET_STUBDESCRIPTOR(DelObjProp);
    GateRef result = CallRuntime(delObjProp, glue, GetWord64Constant(FAST_STUB_ID(DelObjProp)),
                                 {glue, obj, prop});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(0));
    }
    Bind(&notException);
    acc = result;
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8)));
}

void HandleNewObjSpreadDynPrefV8V8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef func = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef newTarget = GetVregValue(sp, ZExtInt8ToPtr(v1));
    StubDescriptor *newObjSpreadDyn = GET_STUBDESCRIPTOR(NewObjSpreadDyn);
    GateRef result = CallRuntime(newObjSpreadDyn, glue, GetWord64Constant(FAST_STUB_ID(NewObjSpreadDyn)),
                                 {glue, func, newTarget, *acc}); // acc is array
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(0));
    }
    Bind(&notException);
    acc = result;
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8)));
}

void HandleCreateIterResultObjPrefV8V8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef flag = GetVregValue(sp, ZExtInt8ToPtr(v1));
    StubDescriptor *createIterResultObj = GET_STUBDESCRIPTOR(CreateIterResultObj);
    GateRef result = CallRuntime(createIterResultObj, glue, GetWord64Constant(FAST_STUB_ID(CreateIterResultObj)),
                                 {glue, value, flag});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(0));
    }
    Bind(&notException);
    acc = result;
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8)));
}

void HandleAsyncFunctionAwaitUncaughtPrefV8V8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef asyncFuncObj = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef flag = GetVregValue(sp, ZExtInt8ToPtr(v1));
    StubDescriptor *asyncFunctionAwaitUncaught = GET_STUBDESCRIPTOR(AsyncFunctionAwaitUncaught);
    GateRef result = CallRuntime(asyncFunctionAwaitUncaught, glue,
                                 GetWord64Constant(FAST_STUB_ID(AsyncFunctionAwaitUncaught)),
                                 {glue, asyncFuncObj, flag});
    Label isException(env);
    Label notException(env);
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(0));
    }
    Bind(&notException);
    acc = result;
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8)));
}

void HandleThrowUndefinedIfHolePrefV8V8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    GateRef v0 = ReadInst8_1(pc);
    GateRef v1 = ReadInst8_2(pc);
    GateRef hole = GetVregValue(sp, ZExtInt8ToPtr(v0));
    Label isHole(env);
    Label notHole(env);
    Branch(TaggedIsHole(hole), &isHole, &notHole);
    Bind(&notHole);
    {
        Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,
                 GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8)));
    }
    Bind(&isHole);
    GateRef obj = GetVregValue(sp, ZExtInt8ToPtr(v1));
    // assert obj.IsString()
    StubDescriptor *throwUndefinedIfHole = GET_STUBDESCRIPTOR(ThrowUndefinedIfHole);
    CallRuntime(throwUndefinedIfHole, glue, GetWord64Constant(FAST_STUB_ID(ThrowUndefinedIfHole)),
                {glue, obj});
    DispatchLast(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter, GetArchRelateConstant(0));
}

void HandleStConstToGlobalRecordPrefId32Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    Label isException(env);
    Label notException(env);
    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetValueFromTaggedArray(MachineType::TAGGED, constpool, stringId);
    StubDescriptor *stGlobalRecord = GET_STUBDESCRIPTOR(StGlobalRecord);
    GateRef result = CallRuntime(stGlobalRecord, glue, GetWord64Constant(FAST_STUB_ID(StGlobalRecord)),
                                 { glue, propKey, *acc, TrueConstant() });
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(6));
    }
    Bind(&notException);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_ID32)));
}

void HandleStLetToGlobalRecordPrefId32Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    Label isException(env);
    Label notException(env);
    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetValueFromTaggedArray(MachineType::TAGGED, constpool, stringId);
    StubDescriptor *stGlobalRecord = GET_STUBDESCRIPTOR(StGlobalRecord);
    GateRef result = CallRuntime(stGlobalRecord, glue, GetWord64Constant(FAST_STUB_ID(StGlobalRecord)),
                                 { glue, propKey, *acc, FalseConstant() });
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(6));
    }
    Bind(&notException);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_ID32)));
}

void HandleStClassToGlobalRecordPrefId32Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    Label isException(env);
    Label notException(env);
    GateRef stringId = ReadInst32_1(pc);
    GateRef propKey = GetValueFromTaggedArray(MachineType::TAGGED, constpool, stringId);
    StubDescriptor *stGlobalRecord = GET_STUBDESCRIPTOR(StGlobalRecord);
    GateRef result = CallRuntime(stGlobalRecord, glue, GetWord64Constant(FAST_STUB_ID(StGlobalRecord)),
                                 { glue, propKey, *acc, FalseConstant() });
    Branch(TaggedIsException(result), &isException, &notException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(6));
    }
    Bind(&notException);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_ID32)));
}

void HandleNegDynPrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2);
    GateRef constpool = TaggedPointerArgument(3);
    GateRef profileTypeInfo = TaggedPointerArgument(4);
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5));
    GateRef hotnessCounter = Int32Argument(6);

    GateRef vsrc = ReadInst8_1(pc);
    GateRef value = GetVregValue(sp, ZExtInt8ToPtr(vsrc));

    Label valueIsInt(env);
    Label valueNotInt(env);
    Label accDispatch(env);
    Branch(TaggedIsInt(value), &valueIsInt, &valueNotInt);
    Bind(&valueIsInt);
    {
        GateRef valueInt = TaggedCastToInt32(value);
        Label valueIsZero(env);
        Label valueNotZero(env);
        Branch(Word32Equal(valueInt, GetInt32Constant(0)), &valueIsZero, &valueNotZero);
        Bind(&valueIsZero);
        {
            // Format::PREF_V8： size = 3
            acc = IntBuildTaggedWithNoGC(GetInt32Constant(0));
            Jump(&accDispatch);
        }
        Bind(&valueNotZero);
        {
            acc = IntBuildTaggedWithNoGC(Int32Sub(GetInt32Constant(0), valueInt));
            Jump(&accDispatch);
        }
    }
    Bind(&valueNotInt);
    {
        Label valueIsDouble(env);
        Label valueNotDouble(env);
        Branch(TaggedIsDouble(value), &valueIsDouble, &valueNotDouble);
        Bind(&valueIsDouble);
        {
            GateRef valueDouble = TaggedCastToDouble(value);
            acc = DoubleBuildTaggedWithNoGC(DoubleSub(GetDoubleConstant(0), valueDouble));
            Jump(&accDispatch);
        }
        Label isException(env);
        Label notException(env);
        Bind(&valueNotDouble);
        {
            // slow path
            StubDescriptor *NegDyn = GET_STUBDESCRIPTOR(NegDyn);
            GateRef result = CallRuntime(NegDyn, glue, GetWord64Constant(FAST_STUB_ID(NegDyn)),
                                         {glue, value});
            Branch(TaggedIsException(result), &isException, &notException);
            Bind(&isException);
            {
                DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(0));
            }
            Bind(&notException);
            acc = result;
            Jump(&accDispatch);
        }
    }

    Bind(&accDispatch);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8)));
}
void HandleDefineClassWithBufferPrefId16Imm16Imm16V8V8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is sp */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is constpool */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is profileTypeInfo */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is hotnessCounter */

    GateRef methodId = ReadInst16_1(pc);
    GateRef literalId = ReadInst16_3(pc);
    GateRef length = ReadInst16_5(pc);
    GateRef v0 = ReadInst8_7(pc);
    GateRef v1 = ReadInst8_8(pc);

    GateRef classTemplate = GetObjectFromConstPool(constpool, ZExtInt16ToInt32(methodId));
    GateRef literalBuffer = GetObjectFromConstPool(constpool, ZExtInt16ToInt32(literalId));
    GateRef lexicalEnv = GetVregValue(sp, ZExtInt8ToPtr(v0));
    GateRef proto = GetVregValue(sp, ZExtInt8ToPtr(v1));

    DEFVARIABLE(res, MachineType::TAGGED, GetUndefinedConstant());

    Label isResolved(env);
    Label isNotResolved(env);
    Label afterCheckResolved(env);
    Branch(FunctionIsResolved(classTemplate), &isResolved, &isNotResolved);
    Bind(&isResolved);
    {
        StubDescriptor *cloneClassFromTemplate = GET_STUBDESCRIPTOR(CloneClassFromTemplate);
        res = CallRuntime(cloneClassFromTemplate, glue, GetWord64Constant(FAST_STUB_ID(CloneClassFromTemplate)),
                          { glue, classTemplate, proto, lexicalEnv, constpool });
        Jump(&afterCheckResolved);
    }
    Bind(&isNotResolved);
    {
        StubDescriptor *resolveClass = GET_STUBDESCRIPTOR(ResolveClass);
        res = CallRuntime(resolveClass, glue, GetWord64Constant(FAST_STUB_ID(ResolveClass)),
                          { glue, classTemplate, literalBuffer, proto, lexicalEnv, constpool });
        Jump(&afterCheckResolved);
    }
    Bind(&afterCheckResolved);
    Label isException(env);
    Label isNotException(env);
    Branch(TaggedIsException(*res), &isException, &isNotException);
    Bind(&isException);
    {
        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
            GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_ID16_IMM16_IMM16_V8_V8)));
    }
    Bind(&isNotException);
    GateRef newLexicalEnv = GetVregValue(sp, ZExtInt8ToPtr(v0));  // slow runtime may gc
    SetLexicalEnvToFunction(glue, *res, newLexicalEnv);
    StubDescriptor *setClassConstructorLength = GET_STUBDESCRIPTOR(SetClassConstructorLength);
    CallRuntime(setClassConstructorLength, glue, GetWord64Constant(FAST_STUB_ID(SetClassConstructorLength)),
                { glue, *res, Int16BuildTaggedWithNoGC(length) });
    acc = *res;
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
        GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_ID16_IMM16_IMM16_V8_V8)));
}

void HandleLdObjByNamePrefId32V8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */

    // uint16_t slotId = READ_INST_8_0();
    GateRef slotId = ZExtInt8ToInt32(ReadInst8_0(pc));
    GateRef receiver = GetVregValue(sp, ZExtInt8ToPtr(ReadInst8_5(pc)));
    Label receiverIsHeapObject(env);
    Label dispatch(env);
    Label slowPath(env);
    Branch(TaggedIsHeapObject(receiver), &receiverIsHeapObject, &slowPath);
    Bind(&receiverIsHeapObject);
    {
        Label tryIC(env);
        Label tryFastPath(env);
        Branch(Word64Equal(profileTypeInfo, GetUndefinedConstant()), &tryFastPath, &tryIC);
        Bind(&tryIC);
        {
            Label isHeapObject(env);
            // JSTaggedValue firstValue = profileTypeArray->Get(slotId);
            GateRef firstValue = GetValueFromTaggedArray(MachineType::TAGGED, profileTypeInfo, slotId);
            // if (LIKELY(firstValue.IsHeapObject())) {
            Branch(TaggedIsHeapObject(firstValue), &isHeapObject, &slowPath);
            Bind(&isHeapObject);
            {
                Label tryPoly(env);
                Label loadWithHandler(env);
                GateRef secondValue = GetValueFromTaggedArray(MachineType::TAGGED, profileTypeInfo,
                        Int32Add(slotId, GetInt32Constant(1)));
                DEFVARIABLE(cachedHandler, MachineType::TAGGED, secondValue);
                GateRef hclass = LoadHClass(receiver);
                Branch(Word64Equal(TaggedCastToWeakReferentUnChecked(firstValue), hclass),
                       &loadWithHandler, &tryPoly);

                Bind(&tryPoly);
                {
                    cachedHandler = CheckPolyHClass(firstValue, hclass);
                    Jump(&loadWithHandler);
                }

                Bind(&loadWithHandler);
                {
                    Label notHole(env);

                    GateRef result = LoadICWithHandler(glue, receiver, receiver, *cachedHandler);
                    Branch(Word64Equal(result, GetHoleConstant()), &slowPath, &notHole);
                    Bind(&notHole);
                    acc = result;
                    Jump(&dispatch);
                }
            }
        }
        Bind(&tryFastPath);
        {
            Label notHole(env);
            GateRef stringId = ZExtInt8ToInt32(ReadInst32_1(pc));
            GateRef propKey = GetValueFromTaggedArray(MachineType::TAGGED, constpool, stringId);
            auto stubDescriptor = GET_STUBDESCRIPTOR(GetPropertyByName);
            GateRef result = CallRuntime(stubDescriptor, glue, GetWord64Constant(FAST_STUB_ID(GetPropertyByName)), {
                glue, receiver, propKey
            });
            Branch(Word64Equal(result, GetHoleConstant()), &slowPath, &notHole);

            Bind(&notHole);
            acc = result;
            Jump(&dispatch);
        }
    }
    Bind(&slowPath);
    {
        Label isException(env);
        Label noException(env);
        GateRef stringId = ZExtInt8ToInt32(ReadInst32_1(pc));
        GateRef propKey = GetValueFromTaggedArray(MachineType::TAGGED, constpool, stringId);
        auto stubDescriptor = GET_STUBDESCRIPTOR(LoadICByName);
        GateRef result = CallRuntime(stubDescriptor, glue, GetWord64Constant(FAST_STUB_ID(LoadICByName)), {
            glue, profileTypeInfo, receiver, propKey, slotId
        });
        // INTERPRETER_RETURN_IF_ABRUPT(res);
        Branch(TaggedIsException(result), &isException, &noException);
        Bind(&isException);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(0));
        }
        Bind(&noException);
        acc = result;
        Jump(&dispatch);
    }
    Bind(&dispatch);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_ID32_V8)));
}

void HandleStOwnByValueWithNameSetPrefV8V8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    GateRef v0 = ZExtInt8ToPtr(ReadInst8_1(pc));
    GateRef v1 = ZExtInt8ToPtr(ReadInst8_2(pc));
    DEFVARIABLE(receiver, MachineType::TAGGED, GetVregValue(sp, v0));
    Label isHeapObject(env);
    Label notHeapObject(env);
    Label notClassConstructor(env);
    Label notClassPrototype(env);
    Label notHole(env);
    Label isException(env);
    Label notException(env);
    Label isException1(env);
    Label notException1(env);
    Branch(TaggedIsHeapObject(*receiver), &isHeapObject, &notHeapObject);
    Bind(&isHeapObject);
    {
        Branch(IsClassConstructor(*receiver), &notHeapObject, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            Branch(IsClassPrototype(*receiver), &notHeapObject, &notClassPrototype);
            Bind(&notClassPrototype);
            {
                DEFVARIABLE(propKey, MachineType::TAGGED, GetVregValue(sp, v1));
                StubDescriptor *setPropertyByValue = GET_STUBDESCRIPTOR(SetPropertyByValue);
                GateRef res = CallRuntime(setPropertyByValue, glue, GetWord64Constant(FAST_STUB_ID(SetPropertyByValue)),
                                 { glue, *receiver, *propKey, acc });
                propKey = GetVregValue(sp, v1);
                Branch(TaggedIsHole(res), &notHeapObject, &notHole);
                Bind(&notHole);
                {
                    Branch(TaggedIsException(res), &isException, &notException);
                    Bind(&isException);
                    {
                        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter, GetArchRelateConstant(0));
                    }
                    Bind(&notException);
                    StubDescriptor *setFunctionNameNoPrefix = GET_STUBDESCRIPTOR(SetFunctionNameNoPrefix);
                    CallRuntime(setFunctionNameNoPrefix, glue, GetWord64Constant(FAST_STUB_ID(SetFunctionNameNoPrefix)),
                                    { glue, acc, *propKey });
                    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,
                        GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8)));
                }
            }
        }
    }
    Bind(&notHeapObject);
    {
        receiver = GetVregValue(sp, v0);
        GateRef propKey1 = GetVregValue(sp, v1);
        StubDescriptor *stOwnByValueWithNameSet = GET_STUBDESCRIPTOR(StOwnByValueWithNameSet);
        GateRef res = CallRuntime(stOwnByValueWithNameSet, glue, GetWord64Constant(FAST_STUB_ID(StOwnByValueWithNameSet)),
                                    { glue, *receiver, propKey1, acc });
        Branch(TaggedIsException(res), &isException1, &notException1);
        Bind(&isException1);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter, GetArchRelateConstant(0));
        }
        Bind(&notException1);
        Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,
            GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_V8_V8)));
        Return();
    }
}

void HandleStOwnByNameWithNameSetPrefId32V8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    GateRef stringId = ReadInst32_1(pc);
    GateRef v0 = ZExtInt8ToPtr(ReadInst8_5(pc));
    DEFVARIABLE(receiver, MachineType::TAGGED, GetVregValue(sp, v0));
    Label isJSObject(env);
    Label notJSObject(env);
    Label notClassConstructor(env);
    Label notClassPrototype(env);
    Label notHole(env);
    Label isException(env);
    Label notException(env);
    Label isException1(env);
    Label notException1(env);
    Branch(IsJSObject(*receiver), &isJSObject, &notJSObject);
    Bind(&isJSObject);
    {
        Branch(IsClassConstructor(*receiver), &notJSObject, &notClassConstructor);
        Bind(&notClassConstructor);
        {
            Branch(IsClassPrototype(*receiver), &notJSObject, &notClassPrototype);
            Bind(&notClassPrototype);
            {
                GateRef propKey = GetValueFromTaggedArray(MachineType::TAGGED, constpool, stringId);
                GateRef res = SetPropertyByNameWithOwn(glue, *receiver, propKey, acc);
                Branch(TaggedIsHole(res), &notJSObject, &notHole);
                Bind(&notHole);
                {
                    Branch(TaggedIsException(res), &isException, &notException);
                    Bind(&isException);
                    {
                        DispatchLast(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter, GetArchRelateConstant(0));
                    }
                    Bind(&notException);
                    StubDescriptor *setFunctionNameNoPrefix = GET_STUBDESCRIPTOR(SetFunctionNameNoPrefix);
                    CallRuntime(setFunctionNameNoPrefix, glue, GetWord64Constant(FAST_STUB_ID(SetFunctionNameNoPrefix)),
                                    { glue, acc, propKey });
                    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,
                        GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_ID32_V8)));
                }
            }
        }
    }
    Bind(&notJSObject);
    {
        receiver = GetVregValue(sp, v0);
        GateRef propKey1 = GetValueFromTaggedArray(MachineType::TAGGED, constpool, stringId);
        StubDescriptor *stOwnByNameWithNameSet = GET_STUBDESCRIPTOR(StOwnByNameWithNameSet);
        GateRef res = CallRuntime(stOwnByNameWithNameSet, glue, GetWord64Constant(FAST_STUB_ID(StOwnByNameWithNameSet)),
                                    { glue, *receiver, propKey1, acc });
        Branch(TaggedIsException(res), &isException1, &notException1);
        Bind(&isException1);
        {
            DispatchLast(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter, GetArchRelateConstant(0));
        }
        Bind(&notException1);
        Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,
            GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_ID32_V8)));
        Return();
    }
}

void HandleLdFunctionPrefStub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    acc = GetFunctionFromFrame(GetFrame(sp));
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_NONE)));
}

void HandleMovV4V4Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    GateRef vdst = ZExtInt8ToPtr(ReadInst4_0(pc));
    GateRef vsrc = ZExtInt8ToPtr(ReadInst4_1(pc));
    GateRef value = GetVregValue(sp, vsrc);
    SetVregValue(glue, sp, vdst, value);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::V4_V4)));
}

void HandleMovDynV8V8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    GateRef vdst = ZExtInt8ToPtr(ReadInst8_0(pc));
    GateRef vsrc = ZExtInt8ToPtr(ReadInst8_1(pc));
    GateRef value = GetVregValue(sp, vsrc);
    SetVregValue(glue, sp, vdst, value);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::V8_V8)));
}

void HandleMovDynV16V16Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    GateRef vdst = ZExtInt16ToPtr(ReadInst16_0(pc));
    GateRef vsrc = ZExtInt16ToPtr(ReadInst16_2(pc));
    GateRef value = GetVregValue(sp, vsrc);
    SetVregValue(glue, sp, vdst, value);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::V16_V16)));
}

void HandleLdaStrId32Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    GateRef stringId = ReadInst32_0(pc);
    acc = GetValueFromTaggedArray(MachineType::TAGGED, constpool, stringId);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::ID32)));
}

void HandleLdaiDynImm32Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    GateRef imm = ReadInst32_0(pc);
    acc = IntBuildTaggedWithNoGC(imm);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::IMM32)));
}

void HandleFldaiDynImm64Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    GateRef imm = CastInt64ToFloat64(ReadInst64_0(pc));
    acc = DoubleBuildTaggedWithNoGC(imm);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::IMM64)));
}

void HandleJeqzImm8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    DEFVARIABLE(profileTypeInfo, MachineType::TAGGED_POINTER, TaggedPointerArgument(4)); /* 4: 5th parameter is value */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is value */
    DEFVARIABLE(hotnessCounter, MachineType::INT32, TaggedArgument(6)); /* 6: 7th parameter is value */
    GateRef offset = ReadInstSigned8_0(pc);
    Label accEqualFalse(env);
    Label accNotEqualFalse(env);
    Label accIsInt(env);
    Label accNotInt(env);
    Label accEqualZero(env);
    Label accIsDouble(env);
    Label last(env);
    Branch(Word64Equal(ChangeTaggedPointerToInt64(acc), TaggedFalse()), &accEqualFalse, &accNotEqualFalse);
    Bind(&accNotEqualFalse);
    {
        Branch(TaggedIsInt(acc), &accIsInt, &accNotInt);
        Bind(&accIsInt);
        {
            Branch(Word32Equal(TaggedGetInt(acc), GetInt32Constant(0)), &accEqualFalse, &accNotInt);
        }
        Bind(&accNotInt);
        {
            Branch(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                Branch(DoubleEqual(TaggedCastToDouble(acc), GetDoubleConstant(0)), &accEqualFalse, &last);
            }
        }
    }
    Bind(&accEqualFalse);
    {
        hotnessCounter = Int32Add(offset, *hotnessCounter);
        StubDescriptor *updateHotnessCounter = GET_STUBDESCRIPTOR(UpdateHotnessCounter);
        profileTypeInfo = CallRuntime(updateHotnessCounter, glue,
            GetWord64Constant(FAST_STUB_ID(UpdateHotnessCounter)), { glue, sp });
        Dispatch(glue, pc, sp, constpool, *profileTypeInfo, acc, *hotnessCounter, SExtInt32ToPtr(offset));
    }
    Bind(&last);
    Dispatch(glue, pc, sp, constpool, *profileTypeInfo, acc, *hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_NONE)));
}

void HandleJeqzImm16Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is value */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    DEFVARIABLE(profileTypeInfo, MachineType::TAGGED_POINTER, TaggedPointerArgument(4)); /* 4: 5th parameter is value */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is value */
    DEFVARIABLE(hotnessCounter, MachineType::INT32, TaggedArgument(6)); /* 6: 7th parameter is value */
    GateRef offset = ReadInstSigned16_0(pc);
    Label accEqualFalse(env);
    Label accNotEqualFalse(env);
    Label accIsInt(env);
    Label accNotInt(env);
    Label accEqualZero(env);
    Label accIsDouble(env);
    Label last(env);
    Branch(Word64Equal(ChangeTaggedPointerToInt64(acc), TaggedFalse()), &accEqualFalse, &accNotEqualFalse);
    Bind(&accNotEqualFalse);
    {
        Branch(TaggedIsInt(acc), &accIsInt, &accNotInt);
        Bind(&accIsInt);
        {
            Branch(Word32Equal(TaggedGetInt(acc), GetInt32Constant(0)), &accEqualFalse, &accNotInt);
        }
        Bind(&accNotInt);
        {
            Branch(TaggedIsDouble(acc), &accIsDouble, &last);
            Bind(&accIsDouble);
            {
                Branch(DoubleEqual(TaggedCastToDouble(acc), GetDoubleConstant(0)), &accEqualFalse, &last);
            }
        }
    }
    Bind(&accEqualFalse);
    {
        hotnessCounter = Int32Add(offset, *hotnessCounter);
        StubDescriptor *updateHotnessCounter = GET_STUBDESCRIPTOR(UpdateHotnessCounter);
        profileTypeInfo = CallRuntime(updateHotnessCounter, glue,
            GetWord64Constant(FAST_STUB_ID(UpdateHotnessCounter)), { glue, sp });
        Dispatch(glue, pc, sp, constpool, *profileTypeInfo, acc, *hotnessCounter, SExtInt32ToPtr(offset));
    }
    Bind(&last);
    Dispatch(glue, pc, sp, constpool, *profileTypeInfo, acc, *hotnessCounter,
             GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::IMM16)));
}

void HandleImportModulePrefId32Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is sp */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is constpool */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is profileTypeInfo */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is hotnessCounter */

    GateRef stringId = ReadInst32_1(pc);
    GateRef prop = GetObjectFromConstPool(constpool, stringId);
    StubDescriptor *importModule = GET_STUBDESCRIPTOR(ImportModule);
    GateRef moduleRef = CallRuntime(importModule, glue, GetWord64Constant(FAST_STUB_ID(ImportModule)), { glue, prop });
    acc = moduleRef;
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
        GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_ID32)));
}

void HandleStModuleVarPrefId32Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is sp */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is constpool */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is profileTypeInfo */
    GateRef acc = TaggedArgument(5); /* 5: 6th parameter is acc */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is hotnessCounter */

    GateRef stringId = ReadInst32_1(pc);
    GateRef prop = GetObjectFromConstPool(constpool, stringId);
    GateRef value = acc;

    StubDescriptor *stModuleVar = GET_STUBDESCRIPTOR(StModuleVar);
    CallRuntime(stModuleVar, glue, GetWord64Constant(FAST_STUB_ID(StModuleVar)), { glue, prop, value });
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter,
        GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_ID32)));
}

void HandleLdModVarByNamePrefId32V8Stub::GenerateCircuit(const CompilationConfig *cfg)
{
    Stub::GenerateCircuit(cfg);
    // auto env = GetEnvironment();
    GateRef glue = PtrArgument(0);
    GateRef pc = PtrArgument(1);
    GateRef sp = PtrArgument(2); /* 2 : 3rd parameter is sp */
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is constpool */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is profileTypeInfo */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is hotnessCounter */

    GateRef stringId = ReadInst32_1(pc);
    GateRef v0 = ReadInst8_5(pc);
    GateRef itemName = GetObjectFromConstPool(constpool, stringId);
    GateRef moduleObj = GetVregValue(sp, ZExtInt8ToPtr(v0));

    StubDescriptor *ldModvarByName = GET_STUBDESCRIPTOR(LdModvarByName);
    GateRef moduleVar = CallRuntime(ldModvarByName, glue, GetWord64Constant(FAST_STUB_ID(LdModvarByName)),
                                    { glue, moduleObj, itemName });
    acc = moduleVar;
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter,
        GetArchRelateConstant(BytecodeInstruction::Size(BytecodeInstruction::Format::PREF_ID32_V8)));
}
}  // namespace panda::ecmascript::kungfu
