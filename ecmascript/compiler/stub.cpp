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

#include "ecmascript/compiler/stub.h"

#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/js_object.h"
#include "ecmascript/tagged_hash_table-inl.h"
#include "libpandabase/macros.h"

namespace kungfu {
using LabelImpl = Stub::Label::LabelImpl;

Stub::Label::Label(Environment *env)
{
    impl_ = env->NewLabel(env);
}

AddrShift Stub::Variable::AddPhiOperand(AddrShift val)
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

AddrShift Stub::Variable::AddOperandToSelector(AddrShift val, size_t idx, AddrShift in)
{
    env_->GetCircuit()->NewIn(val, idx, in);
    return val;
}

AddrShift Stub::Variable::TryRemoveTrivialPhi(AddrShift phiVal)
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
            return phiVal;  // the phi merges at least two values: not trivial
        }
        same = op;
    }
    if (same == nullptr) {
        // the phi is unreachable or in the start block
        same = env_->GetCircuit()->LoadGatePtr(env_->GetCircuitBuilder().UndefineConstant());
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

void Stub::Variable::RerouteOuts(const std::vector<Out *> &outs, Gate *newGate)
{
    // reroute all outs to new node
    for (auto out : outs) {
        size_t idx = out->GetIndex();
        out->GetGate()->ModifyIn(idx, newGate);
    }
}

void LabelImpl::Seal()
{
    for (auto &[variable, gate] : incompletePhis_) {
        variable->AddPhiOperand(gate);
    }
    isSealed_ = true;
}

void LabelImpl::WriteVariable(Variable *var, AddrShift value)
{
    valueMap_[var] = value;
}

AddrShift LabelImpl::ReadVariable(Variable *var)
{
    if (valueMap_.find(var) != valueMap_.end()) {
        auto result = valueMap_.at(var);
        if (!env_->GetCircuit()->GetOpCode(result).IsNop()) {
            return result;
        }
    }
    return ReadVariableRecursive(var);
}

AddrShift LabelImpl::ReadVariableRecursive(Variable *var)
{
    AddrShift val;
    OpCode opcode = CircuitBuilder::GetSelectOpCodeFromMachineType(var->Type());
    if (!IsSealed()) {
        // only loopheader gate will be not sealed
        int valueCounts = static_cast<int>(this->predecessors_.size()) + 1;

        val = env_->GetCircuitBuilder().NewSelectorGate(opcode, predeControl_, valueCounts);
        env_->AddSelectorToLabel(val, Label(this));
        incompletePhis_[var] = val;
    } else if (predecessors_.size() == 1) {
        val = predecessors_[0]->ReadVariable(var);
    } else {
        val = env_->GetCircuitBuilder().NewSelectorGate(opcode, predeControl_, this->predecessors_.size());
        env_->AddSelectorToLabel(val, Label(this));
        WriteVariable(var, val);
        val = var->AddPhiOperand(val);
    }
    WriteVariable(var, val);
    return val;
}

void LabelImpl::Bind()
{
    ASSERT(!predecessors_.empty());
    if (IsLoopHead()) {
        // 2 means input number of depend selector gate
        loopDepend_ = env_->GetCircuitBuilder().NewSelectorGate(OpCode(OpCode::DEPEND_SELECTOR), predeControl_, 2);
        env_->GetCircuit()->NewIn(loopDepend_, 1, predecessors_[0]->GetDepend());
        depend_ = loopDepend_;
    }
    if (IsNeedSeal()) {
        Seal();
        MergeAllControl();
        MergeAllDepend();
    }
}

void LabelImpl::MergeAllControl()
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
    std::vector<AddrShift> inGates(predecessors_.size());
    size_t i = 0;
    ASSERT(predeControl_ != -1);
    ASSERT((otherPredeControls_.size() + 1) == predecessors_.size());
    inGates[i++] = predeControl_;
    for (auto in : otherPredeControls_) {
        inGates[i++] = in;
    }

    AddrShift merge = env_->GetCircuitBuilder().NewMerge(inGates.data(), inGates.size());
    predeControl_ = merge;
    control_ = merge;
}

void LabelImpl::MergeAllDepend()
{
    if (IsControlCase()) {
        // Add depend_relay to current label
        auto denpendEntry = Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY));
        dependRelay_ = env_->GetCircuitBuilder().NewDependRelay(predeControl_, denpendEntry);
    }

    if (predecessors_.size() < 2) {  // 2 : Loop Head only support two predecessors_
        depend_ = predecessors_[0]->GetDepend();
        if (dependRelay_ != -1) {
            depend_ = env_->GetCircuitBuilder().NewDependAnd({depend_, dependRelay_});
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
    std::vector<AddrShift> dependsList;
    for (auto prede : this->GetPredecessors()) {
        dependsList.push_back(prede->GetDepend());
    }
    depend_ = env_->GetCircuitBuilder().NewSelectorGate(OpCode(OpCode::DEPEND_SELECTOR), predeControl_, dependsList,
                                                        dependsList.size());
}

void LabelImpl::AppendPredecessor(LabelImpl *predecessor)
{
    if (predecessor != nullptr) {
        predecessors_.push_back(predecessor);
    }
}

bool LabelImpl::IsNeedSeal() const
{
    auto control = env_->GetCircuit()->LoadGatePtr(predeControl_);
    auto numsInList = control->GetOpCode().GetOpCodeNumInsArray(control->GetBitField());
    return predecessors_.size() >= numsInList[0];
}

bool LabelImpl::IsLoopHead() const
{
    return env_->GetCircuit()->IsLoopHead(predeControl_);
}

bool LabelImpl::IsControlCase() const
{
    return env_->GetCircuit()->IsControlCase(predeControl_);
}

Stub::Environment::Environment(size_t arguments, Circuit *circuit)
    : circuit_(circuit), builder_(circuit), arguments_(arguments)
{
    for (size_t i = 0; i < arguments; i++) {
        arguments_[i] = builder_.NewArguments(i);
    }
    entry_ = Label(NewLabel(this, Circuit::GetCircuitRoot(OpCode(OpCode::STATE_ENTRY))));
    currentLabel_ = &entry_;
    currentLabel_->Seal();
    auto depend_entry = Circuit::GetCircuitRoot(OpCode(OpCode::DEPEND_ENTRY));
    currentLabel_->SetDepend(depend_entry);
}

Stub::Environment::~Environment()
{
    for (auto label : rawlabels_) {
        delete label;
    }
}

void Stub::Jump(Label *label)
{
    ASSERT(label);
    auto currentLabel = env_.GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto jump = env_.GetCircuitBuilder().Goto(currentControl);
    currentLabel->SetControl(jump);
    label->AppendPredecessor(currentLabel);
    label->MergeControl(currentLabel->GetControl());
    env_.SetCurrentLabel(nullptr);
}

void Stub::Branch(AddrShift condition, Label *trueLabel, Label *falseLabel)
{
    auto currentLabel = env_.GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    AddrShift ifBranch = env_.GetCircuitBuilder().Branch(currentControl, condition);
    currentLabel->SetControl(ifBranch);
    AddrShift ifTrue = env_.GetCircuitBuilder().NewIfTrue(ifBranch);
    trueLabel->AppendPredecessor(env_.GetCurrentLabel());
    trueLabel->MergeControl(ifTrue);
    AddrShift ifFalse = env_.GetCircuitBuilder().NewIfFalse(ifBranch);
    falseLabel->AppendPredecessor(env_.GetCurrentLabel());
    falseLabel->MergeControl(ifFalse);
    env_.SetCurrentLabel(nullptr);
}

void Stub::Switch(AddrShift index, Label *defaultLabel, int32_t *keysValue, Label *keysLabel, int numberOfKeys)
{
    auto currentLabel = env_.GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    AddrShift switchBranch = env_.GetCircuitBuilder().SwitchBranch(currentControl, index, numberOfKeys);
    currentLabel->SetControl(switchBranch);
    for (int i = 0; i < numberOfKeys; i++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        AddrShift switchCase = env_.GetCircuitBuilder().NewSwitchCase(switchBranch, keysValue[i]);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        keysLabel[i].AppendPredecessor(currentLabel);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        keysLabel[i].MergeControl(switchCase);
    }

    AddrShift defaultCase = env_.GetCircuitBuilder().NewDefaultCase(switchBranch);
    defaultLabel->AppendPredecessor(currentLabel);
    defaultLabel->MergeControl(defaultCase);
    env_.SetCurrentLabel(nullptr);
}

void Stub::LoopBegin(Label *loopHead)
{
    ASSERT(loopHead);
    auto loopControl = env_.GetCircuitBuilder().LoopBegin(loopHead->GetControl());
    loopHead->SetControl(loopControl);
    loopHead->SetPreControl(loopControl);
    loopHead->Bind();
    env_.SetCurrentLabel(loopHead);
}

void Stub::LoopEnd(Label *loopHead)
{
    ASSERT(loopHead);
    auto currentLabel = env_.GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto loopend = env_.GetCircuitBuilder().LoopEnd(currentControl);
    currentLabel->SetControl(loopend);
    loopHead->AppendPredecessor(currentLabel);
    loopHead->MergeControl(loopend);
    loopHead->Seal();
    loopHead->MergeAllControl();
    loopHead->MergeAllDepend();
    env_.SetCurrentLabel(nullptr);
}

AddrShift Stub::FixLoadType(AddrShift x)
{
    if (PtrValueCode() == ValueCode::INT64) {
        return SExtInt32ToInt64(x);
    }
    if (PtrValueCode() == ValueCode::INT32) {
        return TruncInt64ToInt32(x);
    }
    UNREACHABLE();
}

AddrShift Stub::LoadFromObject(MachineType type, AddrShift object, AddrShift offset)
{
    AddrShift elementsOffset = GetInteger32Constant(panda::ecmascript::JSObject::ELEMENTS_OFFSET);
    if (PtrValueCode() == ValueCode::INT64) {
        elementsOffset = SExtInt32ToInt64(elementsOffset);
    }
    // load elements in object
    AddrShift elements = Load(MachineType::UINT64_TYPE, object, elementsOffset);
    // load index in tagged array
    AddrShift dataOffset =
        Int32Add(GetInteger32Constant(panda::coretypes::Array::GetDataOffset()),
                 Int32Mul(offset, GetInteger32Constant(panda::ecmascript::JSTaggedValue::TaggedTypeSize())));
    if (PtrValueCode() == ValueCode::INT64) {
        dataOffset = SExtInt32ToInt64(dataOffset);
    }
    return Load(type, ChangeInt64ToPointer(elements), dataOffset);
}

AddrShift Stub::FindElementFromNumberDictionary(AddrShift thread, AddrShift elements, AddrShift key)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->PushCurrentLabel(&subentry);
    DEFVARIABLE(result, INT32_TYPE, GetInteger32Constant(-1));
    Label exit(env);
    AddrShift capcityoffset =
        PtrMul(GetPtrConstant(panda::ecmascript::JSTaggedValue::TaggedTypeSize()),
               GetPtrConstant(panda::ecmascript::TaggedHashTable<panda::ecmascript::NumberDictionary>::SIZE_INDEX));
    AddrShift dataoffset = GetPtrConstant(panda::coretypes::Array::GetDataOffset());
    AddrShift capacity = TaggedCastToInt32(Load(TAGGED_TYPE, elements, PtrAdd(dataoffset, capcityoffset)));
    DEFVARIABLE(count, INT32_TYPE, GetInteger32Constant(1));

    AddrShift pKey = Alloca(static_cast<int>(MachineRep::K_WORD32));
    AddrShift keyStore = Store(INT32_TYPE, pKey, GetPtrConstant(0), TaggedCastToInt32(key));
    StubDescriptor *getHash32Descriptor = GET_STUBDESCRIPTOR(GetHash32);
    AddrShift len = GetInteger32Constant(sizeof(int) / sizeof(uint8_t));
    AddrShift hash =
        CallRuntime(getHash32Descriptor, thread, GetWord64Constant(FAST_STUB_ID(GetHash32)), keyStore, {pKey, len});
    DEFVARIABLE(entry, INT32_TYPE, Word32And(hash, Int32Sub(capacity, GetInteger32Constant(1))));
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    AddrShift element = GetKeyFromNumberDictionary(elements, *entry);
    Label isHole(env);
    Label notHole(env);
    Branch(TaggedIsHole(element), &isHole, &notHole);
    Bind(&isHole);
    Jump(&loopEnd);
    Bind(&notHole);
    Label isUndefined(env);
    Label notUndefined(env);
    Branch(TaggedIsUndefined(element), &isUndefined, &notUndefined);
    Bind(&isUndefined);
    result = GetInteger32Constant(-1);
    Jump(&exit);
    Bind(&notUndefined);
    Label isMatch(env);
    Label notMatch(env);
    Branch(IsMatchInNumberDictionary(key, element), &isMatch, &notMatch);
    Bind(&isMatch);
    result = *entry;
    Jump(&exit);
    Bind(&notMatch);
    Jump(&loopEnd);
    Bind(&loopEnd);
    entry = GetNextPositionForHash(*entry, *count, capacity);
    count = Int32Add(*count, GetInteger32Constant(1));
    LoopEnd(&loopHead);
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

AddrShift Stub::IsMatchInNumberDictionary(AddrShift key, AddrShift other)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    DEFVARIABLE(result, BOOL_TYPE, FalseConstant());
    Label isHole(env);
    Label notHole(env);
    Label isUndefined(env);
    Label notUndefined(env);
    Branch(TaggedIsHole(key), &isHole, &notHole);
    Bind(&isHole);
    Jump(&exit);
    Bind(&notHole);
    Branch(TaggedIsUndefined(key), &isUndefined, &notUndefined);
    Bind(&isUndefined);
    Jump(&exit);
    Bind(&notUndefined);
    Label keyIsInt(env);
    Label keyNotInt(env);
    Label otherIsInt(env);
    Label otherNotInt(env);
    Branch(TaggedIsInt(key), &keyIsInt, &keyNotInt);
    Bind(&keyIsInt);
    Branch(TaggedIsInt(other), &otherIsInt, &otherNotInt);
    Bind(&otherIsInt);
    result = Word32Equal(TaggedCastToInt32(key), TaggedCastToInt32(other));
    Jump(&exit);
    Bind(&otherNotInt);
    Jump(&exit);
    Bind(&keyNotInt);
    Jump(&exit);
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

AddrShift Stub::GetKeyFromNumberDictionary(AddrShift elements, AddrShift entry)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->PushCurrentLabel(&subentry);
    Label exit(env);
    DEFVARIABLE(result, TAGGED_TYPE, GetUndefinedConstant());
    Label ltZero(env);
    Label notLtZero(env);
    Label gtLength(env);
    Label notGtLength(env);
    AddrShift dictionaryLength = Load(INT32_TYPE, elements, GetPtrConstant(panda::coretypes::Array::GetLengthOffset()));
    AddrShift arrayIndex =
        Int32Add(GetInteger32Constant(panda::ecmascript::NumberDictionary::TABLE_HEADER_SIZE),
                 Int32Mul(entry, GetInteger32Constant(panda::ecmascript::NumberDictionary::ENTRY_SIZE)));
    Branch(Int32LessThan(arrayIndex, GetInteger32Constant(0)), &ltZero, &notLtZero);
    Bind(&ltZero);
    Jump(&exit);
    Bind(&notLtZero);
    Branch(Int32GreaterThan(arrayIndex, dictionaryLength), &gtLength, &notGtLength);
    Bind(&gtLength);
    Jump(&exit);
    Bind(&notGtLength);
    result = GetValueFromTaggedArray(elements, arrayIndex);
    Jump(&exit);
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

// int TaggedHashTable<Derived>::FindEntry(const JSTaggedValue &key) in tagged_hash_table-inl.h
AddrShift Stub::FindEntryFromNameDictionary(AddrShift thread, AddrShift elements, AddrShift key)
{
    auto env = GetEnvironment();
    Label funcEntry(env);
    env->PushCurrentLabel(&funcEntry);
    Label exit(env);
    DEFVARIABLE(result, INT32_TYPE, GetInteger32Constant(-1));
    AddrShift capcityoffset =
        PtrMul(GetPtrConstant(panda::ecmascript::JSTaggedValue::TaggedTypeSize()),
               GetPtrConstant(panda::ecmascript::TaggedHashTable<panda::ecmascript::NumberDictionary>::SIZE_INDEX));
    AddrShift dataoffset = GetPtrConstant(panda::coretypes::Array::GetDataOffset());
    AddrShift capacity = TaggedCastToInt32(Load(TAGGED_TYPE, elements, PtrAdd(dataoffset, capcityoffset)));
    DEFVARIABLE(count, INT32_TYPE, GetInteger32Constant(1));
    DEFVARIABLE(hash, INT32_TYPE, GetInteger32Constant(0));
    // NameDictionary::hash
    Label isSymbol(env);
    Label notSymbol(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label beforeDefineHash(env);
    Branch(IsSymbol(key), &isSymbol, &notSymbol);
    Bind(&isSymbol);
    {
        hash = TaggedCastToInt32(Load(TAGGED_TYPE, key, GetPtrConstant(panda::ecmascript::JSSymbol::HASHFIELD_OFFSET)));
        Jump(&beforeDefineHash);
    }
    Bind(&notSymbol);
    {
        Label isString(env);
        Label notString(env);
        Branch(IsString(key), &isString, &notString);
        Bind(&isString);
        {
            StubDescriptor *stringGetHashCode = GET_STUBDESCRIPTOR(StringGetHashCode);
            hash = CallRuntime(stringGetHashCode, thread, GetWord64Constant(FAST_STUB_ID(StringGetHashCode)), {key});
            Jump(&beforeDefineHash);
        }
        Bind(&notString);
        {
            Jump(&beforeDefineHash);
        }
    }
    Bind(&beforeDefineHash);
    // GetFirstPosition(hash, size)
    DEFVARIABLE(entry, INT32_TYPE, Word32And(*hash, Int32Sub(capacity, GetInteger32Constant(1))));
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        AddrShift element = GetKeyFromNumberDictionary(elements, *entry);
        Label isHole(env);
        Label notHole(env);
        Branch(TaggedIsHole(element), &isHole, &notHole);
        {
            Bind(&isHole);
            {
                Jump(&loopEnd);
            }
            Bind(&notHole);
            {
                Label isUndefined(env);
                Label notUndefined(env);
                Branch(TaggedIsUndefined(element), &isUndefined, &notUndefined);
                {
                    Bind(&isUndefined);
                    {
                        result = GetInteger32Constant(-1);
                        Jump(&exit);
                    }
                    Bind(&notUndefined);
                    {
                        Label isMatch(env);
                        Label notMatch(env);
                        Branch(Word64Equal(key, element), &isMatch, &notMatch);
                        {
                            Bind(&isMatch);
                            {
                                result = *entry;
                                Jump(&exit);
                            }
                            Bind(&notMatch);
                            {
                                Jump(&loopEnd);
                            }
                        }
                    }
                }
            }
        }
        Bind(&loopEnd);
        {
            entry = GetNextPositionForHash(*entry, *count, capacity);
            count = Int32Add(*count, GetInteger32Constant(1));
            LoopEnd(&loopHead);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

AddrShift Stub::JSObjectGetProperty(AddrShift obj, AddrShift hClass, AddrShift attr)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    DEFVARIABLE(result, TAGGED_TYPE, GetUndefinedConstant());
    Label inlinedProp(env);
    Label notInlinedProp(env);
    AddrShift attrOffset = PropAttrGetOffset(attr);
    Branch(IsInlinedProperty(attr), &inlinedProp, &notInlinedProp);
    {
        Bind(&inlinedProp);
        {
            // GetPropertyInlinedProps
            AddrShift hClassObjectSize = GetObjectSizeFromHClass(hClass);
            AddrShift propOffset = Int32Sub(
                ChangeInt64ToInt32(hClassObjectSize),
                Int32Mul(Int32Sub(
                    GetInteger32Constant(panda::ecmascript::JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS), attrOffset),
                    GetInteger32Constant(panda::ecmascript::JSTaggedValue::TaggedTypeSize())));
            result = Load(UINT64_TYPE, obj, ZExtInt32ToInt64(propOffset));
            Jump(&exit);
        }
        Bind(&notInlinedProp);
        {
            // compute outOfLineProp offset, get it and return
            AddrShift array = Load(UINT64_TYPE, obj, GetPtrConstant(panda::ecmascript::JSObject::PROPERTIES_OFFSET));
            result = GetValueFromTaggedArray(array, Int32Sub(attrOffset,
                GetInteger32Constant(panda::ecmascript::JSHClass::DEFAULT_CAPACITY_OF_IN_OBJECTS)));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

void Stub::ThrowTypeAndReturn(AddrShift thread, int messageId, AddrShift val)
{
    StubDescriptor *throwTypeError = GET_STUBDESCRIPTOR(ThrowTypeError);
    AddrShift msgIntId = GetInteger32Constant(messageId);
    CallRuntime(throwTypeError, thread, GetWord64Constant(FAST_STUB_ID(ThrowTypeError)), {thread, msgIntId});
    Return(val);
}

AddrShift Stub::TaggedToRepresentation(AddrShift value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    DEFVARIABLE(resultRep, INT64_TYPE,
                GetWord64Constant(static_cast<int32_t>(panda::ecmascript::Representation::OBJECT)));
    Label isInt(env);
    Label notInt(env);

    Branch(TaggedIsInt(value), &isInt, &notInt);
    Bind(&isInt);
    {
        resultRep = GetWord64Constant(static_cast<int32_t>(panda::ecmascript::Representation::INT));
        Jump(&exit);
    }
    Bind(&notInt);
    {
        Label isDouble(env);
        Label notDouble(env);
        Branch(TaggedIsDouble(value), &isDouble, &notDouble);
        Bind(&isDouble);
        {
            resultRep = GetWord64Constant(static_cast<int32_t>(panda::ecmascript::Representation::DOUBLE));
            Jump(&exit);
        }
        Bind(&notDouble);
        {
            resultRep = GetWord64Constant(static_cast<int32_t>(panda::ecmascript::Representation::OBJECT));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *resultRep;
    env->PopCurrentLabel();
    return ret;
}

AddrShift Stub::UpdateRepresention(AddrShift oldRep, AddrShift value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    DEFVARIABLE(resultRep, INT64_TYPE, oldRep);
    Label isMixedRep(env);
    Label notMiexedRep(env);
    Branch(Word64Equal(oldRep, GetWord64Constant(static_cast<int64_t>(panda::ecmascript::Representation::MIXED))),
           &isMixedRep, &notMiexedRep);
    Bind(&isMixedRep);
    Jump(&exit);
    Bind(&notMiexedRep);
    {
        AddrShift newRep = TaggedToRepresentation(value);
        Label isNoneRep(env);
        Label notNoneRep(env);
        Branch(Word64Equal(oldRep, GetWord64Constant(static_cast<int64_t>(panda::ecmascript::Representation::NONE))),
               &isNoneRep, &notNoneRep);
        Bind(&isNoneRep);
        {
            resultRep = newRep;
            Jump(&exit);
        }
        Bind(&notNoneRep);
        {
            Label isEqaulNewRep(env);
            Label notEqaualNewRep(env);
            Branch(Word64NotEqual(oldRep, newRep), &isEqaulNewRep, &notEqaualNewRep);
            Bind(&isEqaulNewRep);
            {
                resultRep = newRep;
                Jump(&exit);
            }
            Bind(&notEqaualNewRep);
            {
                Label defaultLabel(env);
                Label intLabel(env);
                Label doubleLabel(env);
                Label numberLabel(env);
                Label objectLabel(env);
                // 4 : 4 means that there are 4 args in total.
                std::array<Label, 4> repCaseLabels = {
                    intLabel,
                    doubleLabel,
                    numberLabel,
                    objectLabel,
                };
                // 4 : 4 means that there are 4 args in total.
                std::array<int32_t, 4> keyValues = {
                    static_cast<int32_t>(panda::ecmascript::Representation::INT),
                    static_cast<int32_t>(panda::ecmascript::Representation::DOUBLE),
                    static_cast<int32_t>(panda::ecmascript::Representation::NUMBER),
                    static_cast<int32_t>(panda::ecmascript::Representation::OBJECT),
                };
                // 4 : 4 means that there are 4 cases in total.
                Switch(oldRep, &defaultLabel, keyValues.data(), repCaseLabels.data(), 4);
                Bind(&intLabel);
                Jump(&numberLabel);
                Bind(&doubleLabel);
                Jump(&numberLabel);
                Bind(&numberLabel);
                {
                    Label isObjectNewRep(env);
                    Label notObjectNewRep(env);
                    Branch(Word64NotEqual(newRep, GetWord64Constant(
                        static_cast<int32_t>(panda::ecmascript::Representation::OBJECT))),
                        &notObjectNewRep, &isObjectNewRep);
                    Bind(&notObjectNewRep);
                    {
                        resultRep = GetWord64Constant(static_cast<int32_t>(panda::ecmascript::Representation::NUMBER));
                        Jump(&exit);
                    }
                    Bind(&isObjectNewRep);
                    {
                        resultRep = GetWord64Constant(static_cast<int32_t>(panda::ecmascript::Representation::MIXED));
                        Jump(&exit);
                    }
                }
                Bind(&objectLabel);
                {
                    resultRep = GetWord64Constant(static_cast<int32_t>(panda::ecmascript::Representation::MIXED));
                    Jump(&exit);
                }
                Bind(&defaultLabel);
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *resultRep;
    env->PopCurrentLabel();
    return ret;
}

void Stub::UpdateAndStoreRepresention(AddrShift hclass, AddrShift value)
{
    AddrShift newRep = UpdateRepresention(GetElementRepresentation(hclass), value);
    SetElementRepresentation(hclass, newRep);
}

AddrShift Stub::TaggedIsString(AddrShift obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    DEFVARIABLE(result, BOOL_TYPE, FalseConstant());
    Label isHeapObject(env);
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        result = Word32Equal(GetObjectType(LoadHClass(obj)),
                             GetInteger32Constant(static_cast<int32_t>(panda::ecmascript::JSType::STRING)));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

AddrShift Stub::TaggedIsStringOrSymbol(AddrShift obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    DEFVARIABLE(result, BOOL_TYPE, FalseConstant());
    Label isHeapObject(env);
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        AddrShift objType = GetObjectType(LoadHClass(obj));
        result = Word32Equal(objType,
                             GetInteger32Constant(static_cast<int32_t>(panda::ecmascript::JSType::STRING)));
        Label isString(env);
        Label notString(env);
        Branch(*result, &exit, &notString);
        Bind(&notString);
        {
            result = Word32Equal(objType,
                                 GetInteger32Constant(static_cast<int32_t>(panda::ecmascript::JSType::SYMBOL)));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

AddrShift Stub::IsUtf16String(AddrShift string)
{
    // compressedStringsEnabled fixed to true constant
    AddrShift len = Load(MachineType::UINT32_TYPE, string,
                         GetPtrConstant(panda::ecmascript::EcmaString::GetLengthOffset()));
    return Word32Equal(
        Word32And(len, GetInteger32Constant(panda::ecmascript::EcmaString::STRING_COMPRESSED_BIT)),
        GetInteger32Constant(panda::ecmascript::EcmaString::STRING_UNCOMPRESSED));
}

AddrShift Stub::IsUtf8String(AddrShift string)
{
    // compressedStringsEnabled fixed to true constant
    AddrShift len = Load(MachineType::UINT32_TYPE, string,
                         GetPtrConstant(panda::ecmascript::EcmaString::GetLengthOffset()));
    return Word32Equal(
        Word32And(len, GetInteger32Constant(panda::ecmascript::EcmaString::STRING_COMPRESSED_BIT)),
        GetInteger32Constant(panda::ecmascript::EcmaString::STRING_COMPRESSED));
}

AddrShift Stub::IsInternalString(AddrShift string)
{
    // compressedStringsEnabled fixed to true constant
    AddrShift len = Load(MachineType::UINT32_TYPE, string,
                         GetPtrConstant(panda::ecmascript::EcmaString::GetLengthOffset()));
    return Word32NotEqual(
        Word32And(len, GetInteger32Constant(panda::ecmascript::EcmaString::STRING_INTERN_BIT)),
        GetInteger32Constant(0));
}

AddrShift Stub::IsDigit(AddrShift ch)
{
    return TruncInt32ToInt1(
        Word32And(SExtInt1ToInt32(Int32LessThanOrEqual(ch, GetInteger32Constant('9'))),
                  SExtInt1ToInt32(Int32GreaterThanOrEqual(ch, GetInteger32Constant('0')))));
}

AddrShift Stub::StringToElementIndex(AddrShift string)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    DEFVARIABLE(result, INT32_TYPE, GetInteger32Constant(-1));
    Label greatThanZero(env);
    Label inRange(env);
    AddrShift len = Load(MachineType::UINT32_TYPE, string,
                         GetPtrConstant(panda::ecmascript::EcmaString::GetLengthOffset()));
    len = Word32LSR(len, GetInteger32Constant(2));  // 2 : 2 means len must be right shift 2 bits
    Branch(Word32Equal(len, GetInteger32Constant(0)), &exit, &greatThanZero);
    Bind(&greatThanZero);
    Branch(Int32GreaterThan(len, GetInteger32Constant(panda::ecmascript::MAX_INDEX_LEN)), &exit, &inRange);
    Bind(&inRange);
    {
        AddrShift dataUtf16 = PtrAdd(string, GetPtrConstant(panda::ecmascript::EcmaString::GetDataOffset()));
        DEFVARIABLE(c, UINT32_TYPE, GetInteger32Constant(0));
        Label isUtf16(env);
        Label isUtf8(env);
        Label getChar1(env);
        AddrShift isUtf16String = IsUtf16String(string);
        Branch(isUtf16String, &isUtf16, &isUtf8);
        Bind(&isUtf16);
        {
            c = ZExtInt16ToInt32(Load(INT16_TYPE, dataUtf16));
            Jump(&getChar1);
        }
        Bind(&isUtf8);
        {
            c = ZExtInt8ToInt32(Load(INT8_TYPE, dataUtf16));
            Jump(&getChar1);
        }
        Bind(&getChar1);
        {
            Label isDigitZero(env);
            Label notDigitZero(env);
            Branch(Word32Equal(*c, GetInteger32Constant('0')), &isDigitZero, &notDigitZero);
            Bind(&isDigitZero);
            {
                Label lengthIsOne(env);
                Branch(Word32Equal(len, GetInteger32Constant(1)), &lengthIsOne, &exit);
                Bind(&lengthIsOne);
                {
                    result = GetInteger32Constant(0);
                    Jump(&exit);
                }
            }
            Bind(&notDigitZero);
            {
                Label isDigit(env);
                DEFVARIABLE(i, UINT32_TYPE, GetInteger32Constant(1));
                DEFVARIABLE(n, UINT32_TYPE, Int32Sub(*c, GetInteger32Constant('0')));
                Branch(IsDigit(*c), &isDigit, &exit);
                Label loopHead(env);
                Label loopEnd(env);
                Label afterLoop(env);
                Bind(&isDigit);
                Branch(Int32LessThan(*i, len), &loopHead, &afterLoop);
                LoopBegin(&loopHead);
                {
                    Label isUtf16(env);
                    Label notUtf16(env);
                    Label getChar2(env);
                    Branch(isUtf16String, &isUtf16, &notUtf16);
                    Bind(&isUtf16);
                    {
                        c = ZExtInt16ToInt32(Load(INT16_TYPE, dataUtf16,
                                PtrMul(ChangeInt32ToPointer(*i),
                                       GetPtrConstant(2))));  // 2 : 2 means utf16 char width is two bytes
                        Jump(&getChar2);
                    }
                    Bind(&notUtf16);
                    {
                        c = ZExtInt8ToInt32(Load(INT8_TYPE, dataUtf16, ChangeInt32ToPointer(*i)));
                        Jump(&getChar2);
                    }
                    Bind(&getChar2);
                    {
                        Label isDigit2(env);
                        Label notDigit2(env);
                        Branch(IsDigit(*c), &isDigit2, &notDigit2);
                        Bind(&isDigit2);
                        {
                            // 10 means the base of digit is 10.
                            n = Int32Add(Int32Mul(*n, GetInteger32Constant(10)),
                                         Int32Sub(*c, GetInteger32Constant('0')));
                            i = Int32Add(*i, GetInteger32Constant(1));
                            Branch(Int32LessThan(*i, len), &loopEnd, &afterLoop);
                        }
                        Bind(&notDigit2);
                        Jump(&exit);
                    }
                }
                Bind(&loopEnd);
                LoopEnd(&loopHead);
                Bind(&afterLoop);
                {
                    Label lessThanMaxIndex(env);
                    Branch(Word32LessThan(*n, GetInteger32Constant(panda::ecmascript::JSObject::MAX_ELEMENT_INDEX)),
                           &lessThanMaxIndex, &exit);
                    Bind(&lessThanMaxIndex);
                    {
                        result = *n;
                        Jump(&exit);
                    }
                }
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

AddrShift Stub::TryToElementsIndex(AddrShift key)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    Label isKeyInt(env);
    Label notKeyInt(env);

    DEFVARIABLE(resultKey, INT32_TYPE, GetInteger32Constant(-1));
    Branch(TaggedIsInt(key), &isKeyInt, &notKeyInt);
    Bind(&isKeyInt);
    {
        resultKey = TaggedCastToInt32(key);
        Jump(&exit);
    }
    Bind(&notKeyInt);
    {
        Label isString(env);
        Label notString(env);
        Branch(TaggedIsString(key), &isString, &notString);
        Bind(&isString);
        {
            resultKey = StringToElementIndex(key);
            Jump(&exit);
        }
        Bind(&notString);
        {
            Label isDouble(env);
            Branch(TaggedIsDouble(key), &isDouble, &exit);
            Bind(&isDouble);
            {
                AddrShift number = TaggedCastToDouble(key);
                AddrShift integer = ChangeFloat64ToInt32(number);
                Label isEqual(env);
                Branch(DoubleEqual(number, ChangeInt32ToFloat64(integer)), &isEqual, &exit);
                Bind(&isEqual);
                {
                    resultKey = integer;
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&exit);
    auto ret = *resultKey;
    env->PopCurrentLabel();
    return ret;
}
}  // namespace kungfu