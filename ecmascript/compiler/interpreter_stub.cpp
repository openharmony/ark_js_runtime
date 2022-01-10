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
    GateRef constpool = TaggedPointerArgument(3); /* 3 : 4th parameter is value */
    GateRef profileTypeInfo = TaggedPointerArgument(4); /* 4 : 5th parameter is value */
    DEFVARIABLE(acc, MachineType::TAGGED, TaggedArgument(5)); /* 5: 6th parameter is value */
    GateRef hotnessCounter = Int32Argument(6); /* 6 : 7th parameter is value */
    
    acc = DoubleBuildTaggedWithNoGC(GetDoubleConstant(base::NAN_VALUE));
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(2));
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
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(2));
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

void HandleLdaDynStub::GenerateCircuit(const CompilationConfig *cfg)
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

    GateRef vsrc = ReadInst8(pc, GetInt32Constant(1));
    acc = GetVregValue(sp, ZExtInt8ToPtr(vsrc));
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(2));
}

void HandleStaDynStub::GenerateCircuit(const CompilationConfig *cfg)
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

    GateRef vdst = ReadInst8(pc, GetInt32Constant(1));
    SetVregValue(glue, sp, ZExtInt8ToPtr(vdst), acc);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, acc, hotnessCounter, GetArchRelateConstant(2));
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
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(3));
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

    GateRef level = ZExtInt8ToInt32(ReadInst8(pc, GetArchRelateConstant(2)));  // 2 : skip opcode and prefix
    GateRef slot = ZExtInt8ToInt32(ReadInst8(pc, GetArchRelateConstant(3)));  // 3: the same as above

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
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(4));
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
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(6));
}

void HandleIncdynPrefV8Stub::GenerateCircuit(const CompilationConfig *cfg)
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

    GateRef v0 = ReadInst8(pc, GetInt32Constant(2));
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
        acc = CallRuntime(incDyn, glue, GetWord64Constant(FAST_STUB_ID(IncDyn)),
                            {glue, value});
        Jump(&accDispatch);
    }

    Bind(&accDispatch);
    Dispatch(glue, pc, sp, constpool, profileTypeInfo, *acc, hotnessCounter, GetArchRelateConstant(3));
}
}  // namespace panda::ecmascript::kungfu
