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

#include "ecmascript/compiler/circuit_builder.h"
#include "ecmascript/compiler/circuit_builder-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_function.h"
#include "ecmascript/compiler/common_stubs.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "utils/bit_utils.h"

namespace panda::ecmascript::kungfu {
GateRef CircuitBuilder::Merge(GateRef *inList, size_t controlCount)
{
    return circuit_->NewGate(OpCode(OpCode::MERGE), controlCount, controlCount, inList, GateType::Empty());
}

GateRef CircuitBuilder::Selector(OpCode opcode, MachineType machineType, GateRef control,
    const std::vector<GateRef> &values, int valueCounts, VariableType type)
{
    std::vector<GateRef> inList;
    inList.push_back(control);
    if (values.size() == 0) {
        for (int i = 0; i < valueCounts; i++) {
            inList.push_back(Circuit::NullGate());
        }
    } else {
        for (int i = 0; i < valueCounts; i++) {
            inList.push_back(values[i]);
        }
    }
    return circuit_->NewGate(opcode, machineType, valueCounts, inList, type.GetGateType());
}

GateRef CircuitBuilder::Selector(OpCode opcode, GateRef control,
    const std::vector<GateRef> &values, int valueCounts, VariableType type)
{
    std::vector<GateRef> inList;
    inList.push_back(control);
    if (values.size() == 0) {
        for (int i = 0; i < valueCounts; i++) {
            inList.push_back(Circuit::NullGate());
        }
    } else {
        for (int i = 0; i < valueCounts; i++) {
            inList.push_back(values[i]);
        }
    }
    return circuit_->NewGate(opcode, valueCounts, inList, type.GetGateType());
}

GateRef CircuitBuilder::UndefineConstant(GateType type)
{
    return circuit_->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_UNDEFINED, type);
}

GateRef CircuitBuilder::Branch(GateRef state, GateRef condition)
{
    return circuit_->NewGate(OpCode(OpCode::IF_BRANCH), 0, { state, condition }, GateType::Empty());
}

GateRef CircuitBuilder::SwitchBranch(GateRef state, GateRef index, int caseCounts)
{
    return circuit_->NewGate(OpCode(OpCode::SWITCH_BRANCH), caseCounts, { state, index }, GateType::Empty());
}

GateRef CircuitBuilder::Return(GateRef state, GateRef depend, GateRef value)
{
    auto returnList = Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST));
    return circuit_->NewGate(OpCode(OpCode::RETURN), 0, { state, depend, value, returnList }, GateType::Empty());
}

GateRef CircuitBuilder::ReturnVoid(GateRef state, GateRef depend)
{
    auto returnList = Circuit::GetCircuitRoot(OpCode(OpCode::RETURN_LIST));
    return circuit_->NewGate(OpCode(OpCode::RETURN_VOID), 0, { state, depend, returnList }, GateType::Empty());
}

GateRef CircuitBuilder::Goto(GateRef state)
{
    return circuit_->NewGate(OpCode(OpCode::ORDINARY_BLOCK), 0, { state }, GateType::Empty());
}

GateRef CircuitBuilder::LoopBegin(GateRef state)
{
    auto nullGate = Circuit::NullGate();
    return circuit_->NewGate(OpCode(OpCode::LOOP_BEGIN), 0, { state, nullGate }, GateType::Empty());
}

GateRef CircuitBuilder::LoopEnd(GateRef state)
{
    return circuit_->NewGate(OpCode(OpCode::LOOP_BACK), 0, { state }, GateType::Empty());
}

GateRef CircuitBuilder::IfTrue(GateRef ifBranch)
{
    return circuit_->NewGate(OpCode(OpCode::IF_TRUE), 0, { ifBranch }, GateType::Empty());
}

GateRef CircuitBuilder::IfFalse(GateRef ifBranch)
{
    return circuit_->NewGate(OpCode(OpCode::IF_FALSE), 0, { ifBranch }, GateType::Empty());
}

GateRef CircuitBuilder::SwitchCase(GateRef switchBranch, int64_t value)
{
    return circuit_->NewGate(OpCode(OpCode::SWITCH_CASE), value, { switchBranch }, GateType::Empty());
}

GateRef CircuitBuilder::DefaultCase(GateRef switchBranch)
{
    return circuit_->NewGate(OpCode(OpCode::DEFAULT_CASE), 0, { switchBranch }, GateType::Empty());
}

GateRef CircuitBuilder::DependRelay(GateRef state, GateRef depend)
{
    return circuit_->NewGate(OpCode(OpCode::DEPEND_RELAY), 0, { state, depend }, GateType::Empty());
}

GateRef CircuitBuilder::DependAnd(std::initializer_list<GateRef> args)
{
    std::vector<GateRef> inputs;
    for (auto arg : args) {
        inputs.push_back(arg);
    }
    return circuit_->NewGate(OpCode(OpCode::DEPEND_AND), args.size(), inputs, GateType::Empty());
}

GateRef CircuitBuilder::Arguments(size_t index)
{
    auto argListOfCircuit = Circuit::GetCircuitRoot(OpCode(OpCode::ARG_LIST));
    return GetCircuit()->NewGate(OpCode(OpCode::ARG), MachineType::I64, index, {argListOfCircuit},
                                 GateType::NJSValue());
}

GateRef CircuitBuilder::Int8(int8_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::I8, val, GateType::NJSValue());
}

GateRef CircuitBuilder::Int16(int16_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::I16, val, GateType::NJSValue());
}

GateRef CircuitBuilder::Int32(int32_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::I32, static_cast<BitField>(val), GateType::NJSValue());
}

GateRef CircuitBuilder::Int64(int64_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::I64, val, GateType::NJSValue());
}

GateRef CircuitBuilder::IntPtr(int64_t val)
{
    return GetCircuit()->GetConstantGate(MachineType::ARCH, val, GateType::NJSValue());
}

GateRef CircuitBuilder::RelocatableData(uint64_t val)
{
    auto constantList = Circuit::GetCircuitRoot(OpCode(OpCode::CONSTANT_LIST));
    return GetCircuit()->NewGate(OpCode(OpCode::RELOCATABLE_DATA), val, {constantList}, GateType::Empty());
}

GateRef CircuitBuilder::Boolean(bool val)
{
    return GetCircuit()->GetConstantGate(MachineType::I1, val ? 1 : 0, GateType::NJSValue());
}

GateRef CircuitBuilder::Double(double val)
{
    return GetCircuit()->GetConstantGate(MachineType::F64, bit_cast<int64_t>(val), GateType::NJSValue());
}

GateRef CircuitBuilder::HoleConstant(GateType type)
{
    return GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_HOLE, type);
}

GateRef CircuitBuilder::NullConstant(GateType type)
{
    return GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_NULL, type);
}

GateRef CircuitBuilder::ExceptionConstant(GateType type)
{
    return GetCircuit()->GetConstantGate(MachineType::I64, JSTaggedValue::VALUE_EXCEPTION, type);
}

MachineType CircuitBuilder::GetMachineTypeFromVariableType(VariableType type)
{
    return type.GetMachineType();
}

GateRef CircuitBuilder::BinaryArithmetic(OpCode opcode, MachineType machineType, GateRef left, GateRef right)
{
    auto circuit = GetCircuit();
    GateType type = circuit->LoadGatePtr(left)->GetGateType();
    return circuit->NewGate(opcode, machineType, 0, { left, right }, type);
}

GateRef CircuitBuilder::TaggedNumber(OpCode opcode, GateRef value)
{
    return GetCircuit()->NewGate(opcode, 0, { value }, GateType::TaggedValue());
}

GateRef CircuitBuilder::UnaryArithmetic(OpCode opcode, MachineType machineType, GateRef value)
{
    return GetCircuit()->NewGate(opcode, machineType, 0, { value }, GateType::NJSValue());
}

GateRef CircuitBuilder::UnaryArithmetic(OpCode opcode, GateRef value)
{
    return GetCircuit()->NewGate(opcode, 0, { value }, GateType::NJSValue());
}

GateRef CircuitBuilder::BinaryLogic(OpCode opcode, GateRef left, GateRef right)
{
    return GetCircuit()->NewGate(opcode, 0, { left, right }, GateType::NJSValue());
}

GateRef CircuitBuilder::CallBCHandler(GateRef glue, GateRef target, const std::vector<GateRef> &args)
{
    const CallSignature *cs = BytecodeStubCSigns::BCHandler();
    assert(cs->IsBCStub());
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = Call(cs, glue, target, depend, args);
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::CallBCDebugger(GateRef glue, GateRef target, const std::vector<GateRef> &args)
{
    const CallSignature *cs = BytecodeStubCSigns::BCDebuggerHandler();
    assert(cs->IsBCDebuggerStub());
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = Call(cs, glue, target, depend, args);
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::CallRuntime(GateRef glue, int index, GateRef depend, const std::vector<GateRef> &args)
{
    GateRef target = IntPtr(index);
    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
    assert(cs->IsRuntimeStub());
    auto label = GetCurrentLabel();
    if (depend == Gate::InvalidGateRef) {
        depend = label->GetDepend();
    }
    GateRef result = Call(cs, glue, target, depend, args);
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::CallRuntimeVarargs(GateRef glue, int index, GateRef argc, GateRef argv)
{
    const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntimeWithArgv));
    GateRef target = IntPtr(index);
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    assert(cs->IsRuntimeVAStub());
    GateRef result = Call(cs, glue, target, depend, {argc, argv});
    label->SetDepend(result);
    return result;
}

// call operation
GateRef CircuitBuilder::CallNGCRuntime(GateRef glue, int index, GateRef depend, const std::vector<GateRef> &args)
{
    const CallSignature *cs = RuntimeStubCSigns::Get(index);
    assert(cs->IsRuntimeNGCStub());
    GateRef target = IntPtr(index);
    auto label = GetCurrentLabel();
    if (depend == Gate::InvalidGateRef) {
        depend = label->GetDepend();
    }
    GateRef result = Call(cs, glue, target, depend, args);
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::CallStub(GateRef glue, int index, const std::vector<GateRef> &args)
{
    const CallSignature *cs = CommonStubCSigns::Get(index);
    assert(cs->IsCommonStub());
    GateRef target = IntPtr(index);
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef result = Call(cs, glue, target, depend, args);
    label->SetDepend(result);
    return result;
}

GateRef CircuitBuilder::Call(const CallSignature* cs, GateRef glue, GateRef target, GateRef depend,
                             const std::vector<GateRef> &args)
{
    std::vector<GateRef> inputs { depend, target, glue };
    inputs.insert(inputs.end(), args.begin(), args.end());
    OpCode op(OpCode::NOP);
    if (cs->IsCommonStub()) {
        op = OpCode(OpCode::CALL);
    } else if (cs->IsRuntimeVAStub()) {
        op = OpCode(OpCode::RUNTIME_CALL_WITH_ARGV);
    } else if (cs->IsRuntimeStub()) {
        op = OpCode(OpCode::RUNTIME_CALL);
    } else if (cs->IsBCDebuggerStub()) {
        op = OpCode(OpCode::DEBUGGER_BYTECODE_CALL);
    } else if (cs->IsBCHandlerStub()) {
        op = OpCode(OpCode::BYTECODE_CALL);
    } else if (cs->IsRuntimeNGCStub()) {
        op = OpCode(OpCode::NOGC_RUNTIME_CALL);
    } else {
        UNREACHABLE();
    }
    MachineType machineType = cs->GetReturnType().GetMachineType();
    GateType type = cs->GetReturnType().GetGateType();
    GateRef result = GetCircuit()->NewGate(op, machineType, args.size() + 2, inputs, type);
    return result;
}

// memory
void CircuitBuilder::Store(VariableType type, GateRef glue, GateRef base, GateRef offset, GateRef value)
{
    auto label = GetCurrentLabel();
    auto depend = label->GetDepend();
    GateRef ptr = PtrAdd(base, offset);
    GateRef result = GetCircuit()->NewGate(OpCode(OpCode::STORE), 0, { depend, value, ptr }, type.GetGateType());
    label->SetDepend(result);
    if (type == VariableType::JS_POINTER() || type == VariableType::JS_ANY()) {
        CallStub(glue, CommonStubCSigns::SetValueWithBarrier, {glue, base, offset, value});
    }
    return;
}

GateRef CircuitBuilder::Alloca(int size)
{
    auto allocaList = Circuit::GetCircuitRoot(OpCode(OpCode::ALLOCA_LIST));
    return GetCircuit()->NewGate(OpCode(OpCode::ALLOCA), size, { allocaList }, GateType::NJSValue());
}

GateRef CircuitBuilder::TaggedIsString(GateRef obj)
{
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);
    DEFVAlUE(result, env_, VariableType::BOOL(), False());
    Label isHeapObject(env_);
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        result = Equal(GetObjectType(LoadHClass(obj)),
            Int32(static_cast<int32_t>(JSType::STRING)));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::TaggedIsStringOrSymbol(GateRef obj)
{
    Label entry(env_);
    SubCfgEntry(&entry);
    Label exit(env_);
    DEFVAlUE(result, env_, VariableType::BOOL(), False());
    Label isHeapObject(env_);
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        GateRef objType = GetObjectType(LoadHClass(obj));
        result = Equal(objType, Int32(static_cast<int32_t>(JSType::STRING)));
        Label isString(env_);
        Label notString(env_);
        Branch(*result, &exit, &notString);
        Bind(&notString);
        {
            result = Equal(objType, Int32(static_cast<int32_t>(JSType::SYMBOL)));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

GateRef CircuitBuilder::GetGlobalObject(GateRef glue)
{
    GateRef offset = IntPtr(JSThread::GlueData::GetGlobalObjOffset(cmpCfg_->Is32Bit()));
    return Load(VariableType::JS_ANY(), glue, offset);
}

GateRef CircuitBuilder::GetFunctionBitFieldFromJSFunction(GateRef function)
{
    GateRef offset = IntPtr(JSFunction::BIT_FIELD_OFFSET);
    return Load(VariableType::INT32(), function, offset);
}

GateRef CircuitBuilder::GetModuleFromFunction(GateRef function)
{
    GateRef offset = IntPtr(JSFunction::ECMA_MODULE_OFFSET);
    return Load(VariableType::JS_POINTER(), function, offset);
}

GateRef CircuitBuilder::FunctionIsResolved(GateRef function)
{
    Label subentry(env_);
    SubCfgEntry(&subentry);
    Label exit(env_);
    Label isUndefined(env_);
    Label notUndefined(env_);
    DEFVAlUE(result, env_, VariableType::BOOL(), True());
    Branch(TaggedIsUndefined(function), &isUndefined, &notUndefined);
    Bind(&isUndefined);
    {
        Jump(&exit);
    }
    Bind(&notUndefined);
    {
        GateRef bitfield = GetFunctionBitFieldFromJSFunction(function);
        result = NotEqual(Int32And(Int32LSR(bitfield,
                                            Int32(JSFunction::ResolvedBits::START_BIT)),
                                   Int32((1LU << JSFunction::ResolvedBits::SIZE) - 1)),
                          Int32(0));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    SubCfgExit();
    return ret;
}

void CircuitBuilder::SetResolvedToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef bitfield = GetFunctionBitFieldFromJSFunction(function);
    GateRef mask = Int32(~(((1<<JSFunction::ResolvedBits::SIZE) - 1) << JSFunction::ResolvedBits::START_BIT));
    GateRef result = Int32Or(Int32And(bitfield, mask),
        Int32LSL(ZExtInt1ToInt32(value), Int32(JSFunction::ResolvedBits::START_BIT)));
    Store(VariableType::INT32(), glue, function, IntPtr(JSFunction::BIT_FIELD_OFFSET), result);
}

void CircuitBuilder::SetConstPoolToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::CONSTANT_POOL_OFFSET);
    Store(VariableType::INT64(), glue, function, offset, value);
}

void CircuitBuilder::SetLexicalEnvToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::LEXICAL_ENV_OFFSET);
    Store(VariableType::JS_ANY(), glue, function, offset, value);
}

GateRef CircuitBuilder::GetLexicalEnv(GateRef function)
{
    return Load(VariableType::JS_POINTER(), function, IntPtr(JSFunction::LEXICAL_ENV_OFFSET));
}

void CircuitBuilder::SetModuleToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::ECMA_MODULE_OFFSET);
    Store(VariableType::JS_POINTER(), glue, function, offset, value);
}

void CircuitBuilder::SetPropertyInlinedProps(GateRef glue, GateRef obj, GateRef hClass,
    GateRef value, GateRef attrOffset, VariableType type)
{
    GateRef bitfield = Load(VariableType::INT32(), hClass, IntPtr(JSHClass::BIT_FIELD1_OFFSET));
    GateRef inlinedPropsStart = Int32And(Int32LSR(bitfield,
        Int32(JSHClass::InlinedPropsStartBits::START_BIT)),
        Int32((1LU << JSHClass::InlinedPropsStartBits::SIZE) - 1));
    GateRef propOffset = Int32Mul(Int32Add(inlinedPropsStart, attrOffset),
        Int32(JSTaggedValue::TaggedTypeSize()));
    Store(type, glue, obj, ChangeInt32ToIntPtr(propOffset), value);
}

void CircuitBuilder::SetHomeObjectToFunction(GateRef glue, GateRef function, GateRef value)
{
    GateRef offset = IntPtr(JSFunction::HOME_OBJECT_OFFSET);
    Store(VariableType::INT64(), glue, function, offset, value);
}

Environment::Environment(size_t arguments, CircuitBuilder *builder)
    : circuit_(builder->GetCircuit()), circuitBuilder_(builder), arguments_(arguments)
{
    circuitBuilder_->SetEnvironment(this);
    for (size_t i = 0; i < arguments; i++) {
        arguments_[i] = circuitBuilder_->Arguments(i);
    }
    entry_ = Label(NewLabel(this, Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY))));
    currentLabel_ = &entry_;
    currentLabel_->Seal();
    auto depend_entry = Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY));
    currentLabel_->SetDepend(depend_entry);
}

Environment::Environment(GateRef hir, Circuit *circuit, CircuitBuilder *builder)
    : circuit_(circuit), circuitBuilder_(builder)
{
    circuitBuilder_->SetEnvironment(this);
    auto hirGate = circuit_->LoadGatePtr(hir);
    entry_ = Label(NewLabel(this, circuit_->SaveGatePtr(hirGate->GetInGate(0))));
    currentLabel_ = &entry_;
    currentLabel_->Seal();
    auto dependEntry = circuit_->SaveGatePtr(hirGate->GetInGate(1));
    currentLabel_->SetDepend(dependEntry);
    for (size_t i = 2; i < hirGate->GetNumIns(); i++) {
        inputList_.emplace_back(circuit_->SaveGatePtr(hirGate->GetInGate(i)));
    }
}

Environment::Environment(GateRef stateEntry, GateRef dependEntry, std::vector<GateRef>& inlist,
    Circuit *circuit, CircuitBuilder *builder) : circuit_(circuit), circuitBuilder_(builder)
{
    circuitBuilder_->SetEnvironment(this);
    entry_ = Label(NewLabel(this, stateEntry));
    currentLabel_ = &entry_;
    currentLabel_->Seal();
    currentLabel_->SetDepend(dependEntry);
    for (auto in : inlist) {
        inputList_.emplace_back(in);
    }
}

Environment::~Environment()
{
    circuitBuilder_->SetEnvironment(nullptr);
    for (auto label : rawLabels_) {
        delete label;
    }
}

void CircuitBuilder::Jump(Label *label)
{
    ASSERT(label);
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto jump = Goto(currentControl);
    currentLabel->SetControl(jump);
    label->AppendPredecessor(currentLabel);
    label->MergeControl(currentLabel->GetControl());
    env_->SetCurrentLabel(nullptr);
}

void CircuitBuilder::Branch(GateRef condition, Label *trueLabel, Label *falseLabel)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef ifBranch = Branch(currentControl, condition);
    currentLabel->SetControl(ifBranch);
    GateRef ifTrue = IfTrue(ifBranch);
    trueLabel->AppendPredecessor(GetCurrentLabel());
    trueLabel->MergeControl(ifTrue);
    GateRef ifFalse = IfFalse(ifBranch);
    falseLabel->AppendPredecessor(GetCurrentLabel());
    falseLabel->MergeControl(ifFalse);
    env_->SetCurrentLabel(nullptr);
}

void CircuitBuilder::Switch(GateRef index, Label *defaultLabel, int64_t *keysValue, Label *keysLabel, int numberOfKeys)
{
    auto currentLabel = env_->GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef switchBranch = SwitchBranch(currentControl, index, numberOfKeys);
    currentLabel->SetControl(switchBranch);
    for (int i = 0; i < numberOfKeys; i++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        GateRef switchCase = SwitchCase(switchBranch, keysValue[i]);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        keysLabel[i].AppendPredecessor(currentLabel);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        keysLabel[i].MergeControl(switchCase);
    }

    GateRef defaultCase = DefaultCase(switchBranch);
    defaultLabel->AppendPredecessor(currentLabel);
    defaultLabel->MergeControl(defaultCase);
    env_->SetCurrentLabel(nullptr);
}

void CircuitBuilder::LoopBegin(Label *loopHead)
{
    ASSERT(loopHead);
    auto loopControl = LoopBegin(loopHead->GetControl());
    loopHead->SetControl(loopControl);
    loopHead->SetPreControl(loopControl);
    loopHead->Bind();
    env_->SetCurrentLabel(loopHead);
}

void CircuitBuilder::LoopEnd(Label *loopHead)
{
    ASSERT(loopHead);
    auto currentLabel = GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto loopend = LoopEnd(currentControl);
    currentLabel->SetControl(loopend);
    loopHead->AppendPredecessor(currentLabel);
    loopHead->MergeControl(loopend);
    loopHead->Seal();
    loopHead->MergeAllControl();
    loopHead->MergeAllDepend();
    env_->SetCurrentLabel(nullptr);
}

Label::Label(Environment *env)
{
    impl_ = env->NewLabel(env);
}

Label::Label(CircuitBuilder *cirBuilder)
{
    auto env = cirBuilder->GetCurrentEnvironment();
    impl_ = env->NewLabel(env);
}

void Label::LabelImpl::Seal()
{
    for (auto &[variable, gate] : incompletePhis_) {
        variable->AddPhiOperand(gate);
    }
    isSealed_ = true;
}

void Label::LabelImpl::WriteVariable(Variable *var, GateRef value)
{
    valueMap_[var] = value;
}

GateRef Label::LabelImpl::ReadVariable(Variable *var)
{
    if (valueMap_.find(var) != valueMap_.end()) {
        auto result = valueMap_.at(var);
        if (!env_->GetCircuit()->GetOpCode(result).IsNop()) {
            return result;
        }
    }
    return ReadVariableRecursive(var);
}

GateRef Label::LabelImpl::ReadVariableRecursive(Variable *var)
{
    GateRef val;
    MachineType MachineType = CircuitBuilder::GetMachineTypeFromVariableType(var->Type());
    if (!IsSealed()) {
        // only loopheader gate will be not sealed
        int valueCounts = static_cast<int>(this->predecessors_.size()) + 1;
        if (MachineType == MachineType::NOVALUE) {
            val = env_->GetBulder()->Selector(OpCode(OpCode::DEPEND_SELECTOR),
                predeControl_, {}, valueCounts, var->Type());
        } else {
            val = env_->GetBulder()->Selector(OpCode(OpCode::VALUE_SELECTOR),
                MachineType, predeControl_, {}, valueCounts, var->Type());
        }
        env_->AddSelectorToLabel(val, Label(this));
        incompletePhis_[var] = val;
    } else if (predecessors_.size() == 1) {
        val = predecessors_[0]->ReadVariable(var);
    } else {
        if (MachineType == MachineType::NOVALUE) {
            val = env_->GetBulder()->Selector(OpCode(OpCode::DEPEND_SELECTOR),
                predeControl_, {}, this->predecessors_.size(), var->Type());
        } else {
            val = env_->GetBulder()->Selector(OpCode(OpCode::VALUE_SELECTOR), MachineType,
                predeControl_, {}, this->predecessors_.size(), var->Type());
        }
        env_->AddSelectorToLabel(val, Label(this));
        WriteVariable(var, val);
        val = var->AddPhiOperand(val);
    }
    WriteVariable(var, val);
    return val;
}

void Label::LabelImpl::Bind()
{
    ASSERT(!predecessors_.empty());
    if (IsLoopHead()) {
        // 2 means input number of depend selector gate
        loopDepend_ = env_->GetBulder()->Selector(OpCode(OpCode::DEPEND_SELECTOR), predeControl_, {}, 2);
        env_->GetCircuit()->NewIn(loopDepend_, 1, predecessors_[0]->GetDepend());
        depend_ = loopDepend_;
    }
    if (IsNeedSeal()) {
        Seal();
        MergeAllControl();
        MergeAllDepend();
    }
}

void Label::LabelImpl::MergeAllControl()
{
    if (predecessors_.size() < 2) {  // 2 : Loop Head only support two predecessors_
        return;
    }

    if (IsLoopHead()) {
        ASSERT(predecessors_.size() == 2);  // 2 : Loop Head only support two predecessors_
        ASSERT(otherPredeControls_.size() == 1);
        env_->GetCircuit()->NewIn(predeControl_, 1, otherPredeControls_[0]);
        return;
    }

    // merge all control of predecessors_
    std::vector<GateRef> inGates(predecessors_.size());
    size_t i = 0;
    ASSERT(predeControl_ != -1);
    ASSERT((otherPredeControls_.size() + 1) == predecessors_.size());
    inGates[i++] = predeControl_;
    for (auto in : otherPredeControls_) {
        inGates[i++] = in;
    }

    GateRef merge = env_->GetBulder()->Merge(inGates.data(), inGates.size());
    predeControl_ = merge;
    control_ = merge;
}

void Label::LabelImpl::MergeAllDepend()
{
    if (IsControlCase()) {
        // Add depend_relay to current label
        auto denpendEntry = Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY));
        dependRelay_ = env_->GetBulder()->DependRelay(predeControl_, denpendEntry);
    }

    if (predecessors_.size() < 2) {  // 2 : Loop Head only support two predecessors_
        depend_ = predecessors_[0]->GetDepend();
        if (dependRelay_ != -1) {
            depend_ = env_->GetBulder()->DependAnd({depend_, dependRelay_});
        }
        return;
    }
    if (IsLoopHead()) {
        ASSERT(predecessors_.size() == 2);  // 2 : Loop Head only support two predecessors_
        // Add loop depend to in of depend_seclector
        ASSERT(loopDepend_ != -1);
        // 2 mean 3rd input gate for loopDepend_(depend_selector)
        env_->GetCircuit()->NewIn(loopDepend_, 2, predecessors_[1]->GetDepend());
        return;
    }

    //  Merge all depends to depend_seclector
    std::vector<GateRef> dependsList;
    for (auto prede : this->GetPredecessors()) {
        dependsList.push_back(prede->GetDepend());
    }
    depend_ = env_->GetBulder()->Selector(OpCode(OpCode::DEPEND_SELECTOR),
        predeControl_, dependsList, dependsList.size());
}

void Label::LabelImpl::AppendPredecessor(Label::LabelImpl *predecessor)
{
    if (predecessor != nullptr) {
        predecessors_.push_back(predecessor);
    }
}

bool Label::LabelImpl::IsNeedSeal() const
{
    auto control = env_->GetCircuit()->LoadGatePtr(predeControl_);
    auto stateCount = control->GetOpCode().GetStateCount(control->GetBitField());
    return predecessors_.size() >= stateCount;
}

bool Label::LabelImpl::IsLoopHead() const
{
    return env_->GetCircuit()->IsLoopHead(predeControl_);
}

bool Label::LabelImpl::IsControlCase() const
{
    return env_->GetCircuit()->IsControlCase(predeControl_);
}

GateRef Variable::AddPhiOperand(GateRef val)
{
    ASSERT(IsSelector(val));
    Label label = env_->GetLabelFromSelector(val);
    size_t idx = 0;
    for (auto pred : label.GetPredecessors()) {
        auto preVal = pred.ReadVariable(this);
        ASSERT(!env_->GetCircuit()->GetOpCode(preVal).IsNop());
        idx++;
        val = AddOperandToSelector(val, idx, preVal);
    }
    return TryRemoveTrivialPhi(val);
}

GateRef Variable::AddOperandToSelector(GateRef val, size_t idx, GateRef in)
{
    env_->GetCircuit()->NewIn(val, idx, in);
    return val;
}

GateRef Variable::TryRemoveTrivialPhi(GateRef phiVal)
{
    Gate *phi = env_->GetCircuit()->LoadGatePtr(phiVal);
    Gate *same = nullptr;
    for (size_t i = 1; i < phi->GetNumIns(); ++i) {
        In *phiIn = phi->GetIn(i);
        Gate *op = (!phiIn->IsGateNull()) ? phiIn->GetGate() : nullptr;
        if (op == same || op == phi) {
            continue;  // unique value or self-reference
        }
        if (same != nullptr) {
            return phiVal;  // the phi merges at least two valusses: not trivial
        }
        same = op;
    }
    if (same == nullptr) {
        // the phi is unreachable or in the start block
        GateType type = env_->GetCircuit()->GetGateType(phiVal);
        same = env_->GetCircuit()->LoadGatePtr(env_->GetBulder()->UndefineConstant(type));
    }
    auto same_addr_shift = env_->GetCircuit()->SaveGatePtr(same);

    // remove the trivial phi
    // get all users of phi except self
    std::vector<Out *> outs;
    if (!phi->IsFirstOutNull()) {
        Out *phiOut = phi->GetFirstOut();
        while (!phiOut->IsNextOutNull()) {
            if (phiOut->GetGate() != phi) {
                // remove phi
                outs.push_back(phiOut);
            }
            phiOut = phiOut->GetNextOut();
        }
        // save last phi out
        if (phiOut->GetGate() != phi) {
            outs.push_back(phiOut);
        }
    }
    // reroute all outs of phi to same and remove phi
    RerouteOuts(outs, same);
    phi->DeleteGate();

    // try to recursiveby remove all phi users, which might have vecome trivial
    for (auto out : outs) {
        if (IsSelector(out->GetGate())) {
            auto out_addr_shift = env_->GetCircuit()->SaveGatePtr(out->GetGate());
            auto result = TryRemoveTrivialPhi(out_addr_shift);
            if (same_addr_shift == out_addr_shift) {
                same_addr_shift = result;
            }
        }
    }
    return same_addr_shift;
}

void Variable::RerouteOuts(const std::vector<Out *> &outs, Gate *newGate)
{
    // reroute all outs to new node
    for (auto out : outs) {
        size_t idx = out->GetIndex();
        out->GetGate()->ModifyIn(idx, newGate);
    }
}
}  // namespace panda::ecmascript::kungfu
