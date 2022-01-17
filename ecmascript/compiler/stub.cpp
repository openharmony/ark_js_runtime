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
#include "ecmascript/compiler/stub-inl.h"
#include "ecmascript/js_arraylist.h"
#include "ecmascript/js_object.h"
#include "ecmascript/tagged_hash_table-inl.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::kungfu {
Stub::Label::Label(Environment *env)
{
    impl_ = env->NewLabel(env);
}

GateRef Stub::Variable::AddPhiOperand(GateRef val)
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

GateRef Stub::Variable::AddOperandToSelector(GateRef val, size_t idx, GateRef in)
{
    env_->GetCircuit()->NewIn(val, idx, in);
    return val;
}

GateRef Stub::Variable::TryRemoveTrivialPhi(GateRef phiVal)
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
        TypeCode type = env_->GetCircuit()->GetTypeCode(phiVal);
        same = env_->GetCircuit()->LoadGatePtr(env_->GetCircuitBuilder().UndefineConstant(type));
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

void LabelImpl::WriteVariable(Variable *var, GateRef value)
{
    valueMap_[var] = value;
}

GateRef LabelImpl::ReadVariable(Variable *var)
{
    if (valueMap_.find(var) != valueMap_.end()) {
        auto result = valueMap_.at(var);
        if (!env_->GetCircuit()->GetOpCode(result).IsNop()) {
            return result;
        }
    }
    return ReadVariableRecursive(var);
}

GateRef LabelImpl::ReadVariableRecursive(Variable *var)
{
    GateRef val;
    OpCode opcode = CircuitBuilder::GetSelectOpCodeFromMachineType(var->Type());
    if (!IsSealed()) {
        // only loopheader gate will be not sealed
        int valueCounts = static_cast<int>(this->predecessors_.size()) + 1;

        val = env_->GetCircuitBuilder().NewSelectorGate(opcode, predeControl_, valueCounts, var->Type());
        env_->AddSelectorToLabel(val, Label(this));
        incompletePhis_[var] = val;
    } else if (predecessors_.size() == 1) {
        val = predecessors_[0]->ReadVariable(var);
    } else {
        val = env_->GetCircuitBuilder().NewSelectorGate(opcode, predeControl_, this->predecessors_.size(), var->Type());
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
    std::vector<GateRef> inGates(predecessors_.size());
    size_t i = 0;
    ASSERT(predeControl_ != -1);
    ASSERT((otherPredeControls_.size() + 1) == predecessors_.size());
    inGates[i++] = predeControl_;
    for (auto in : otherPredeControls_) {
        inGates[i++] = in;
    }

    GateRef merge = env_->GetCircuitBuilder().NewMerge(inGates.data(), inGates.size());
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
            depend_ = env_->GetCircuitBuilder().NewDependAnd({ depend_, dependRelay_ });
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
    for (auto label : rawLabels_) {
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

void Stub::Branch(GateRef condition, Label *trueLabel, Label *falseLabel)
{
    auto currentLabel = env_.GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef ifBranch = env_.GetCircuitBuilder().Branch(currentControl, condition);
    currentLabel->SetControl(ifBranch);
    GateRef ifTrue = env_.GetCircuitBuilder().NewIfTrue(ifBranch);
    trueLabel->AppendPredecessor(env_.GetCurrentLabel());
    trueLabel->MergeControl(ifTrue);
    GateRef ifFalse = env_.GetCircuitBuilder().NewIfFalse(ifBranch);
    falseLabel->AppendPredecessor(env_.GetCurrentLabel());
    falseLabel->MergeControl(ifFalse);
    env_.SetCurrentLabel(nullptr);
}

void Stub::Switch(GateRef index, Label *defaultLabel, int64_t *keysValue, Label *keysLabel, int numberOfKeys)
{
    auto currentLabel = env_.GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef switchBranch = env_.GetCircuitBuilder().SwitchBranch(currentControl, index, numberOfKeys);
    currentLabel->SetControl(switchBranch);
    for (int i = 0; i < numberOfKeys; i++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        GateRef switchCase = env_.GetCircuitBuilder().NewSwitchCase(switchBranch, keysValue[i]);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        keysLabel[i].AppendPredecessor(currentLabel);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        keysLabel[i].MergeControl(switchCase);
    }

    GateRef defaultCase = env_.GetCircuitBuilder().NewDefaultCase(switchBranch);
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

GateRef Stub::FixLoadType(GateRef x)
{
    if (PtrValueCode() == ValueCode::INT64) {
        return SExtInt32ToInt64(x);
    }
    if (PtrValueCode() == ValueCode::INT32) {
        return TruncInt64ToInt32(x);
    }
    UNREACHABLE();
}

// FindElementWithCache in ecmascript/layout_info-inl.h
GateRef Stub::FindElementWithCache(GateRef glue, GateRef layoutInfo, GateRef hClass,
    GateRef key, GateRef propsNum)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->PushCurrentLabel(&subEntry);
    DEFVARIABLE(result, MachineType::INT32, GetInt32Constant(-1));
    DEFVARIABLE(i, MachineType::INT32, GetInt32Constant(0));
    Label exit(env);
    Label notExceedUpper(env);
    Label exceedUpper(env);
    Label afterExceedCon(env);
    // 9 : Builtins Object properties number is nine
    Branch(Int32LessThanOrEqual(propsNum, GetInt32Constant(9)), &notExceedUpper, &exceedUpper);
    {
        Bind(&notExceedUpper);
            Label loopHead(env);
            Label loopEnd(env);
            Label afterLoop(env);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                Label propsNumIsZero(env);
                Label propsNumNotZero(env);
                Branch(Word32Equal(propsNum, GetInt32Constant(0)), &propsNumIsZero, &propsNumNotZero);
                Bind(&propsNumIsZero);
                Jump(&afterLoop);
                Bind(&propsNumNotZero);
                GateRef elementAddr = GetPropertiesAddrFromLayoutInfo(layoutInfo);
                GateRef keyInProperty = Load(MachineType::UINT64, elementAddr,
                    ArchRelatePtrMul(ChangeInt32ToUintPtr(*i),
                        GetArchRelateConstant(sizeof(panda::ecmascript::Properties))));
                Label equal(env);
                Label notEqual(env);
                Label afterEqualCon(env);
                Branch(Word64Equal(keyInProperty, key), &equal, &notEqual);
                Bind(&equal);
                result = *i;
                Jump(&exit);
                Bind(&notEqual);
                Jump(&afterEqualCon);
                Bind(&afterEqualCon);
                i = Int32Add(*i, GetInt32Constant(1));
                Branch(Int32LessThan(*i, propsNum), &loopEnd, &afterLoop);
                Bind(&loopEnd);
                LoopEnd(&loopHead);
            }
            Bind(&afterLoop);
            result = GetInt32Constant(-1);
            Jump(&exit);
        Bind(&exceedUpper);
        Jump(&afterExceedCon);
    }
    Bind(&afterExceedCon);
    StubDescriptor *findElemWithCache = GET_STUBDESCRIPTOR(FindElementWithCache);
    result = CallRuntime(findElemWithCache, glue, GetWord64Constant(FAST_STUB_ID(FindElementWithCache)), {
            glue, hClass, key, propsNum
        });
    Jump(&exit);
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::FindElementFromNumberDictionary(GateRef glue, GateRef elements, GateRef key)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->PushCurrentLabel(&subentry);
    DEFVARIABLE(result, MachineType::INT32, GetInt32Constant(-1));
    Label exit(env);
    GateRef capcityoffset =
        ArchRelatePtrMul(GetArchRelateConstant(JSTaggedValue::TaggedTypeSize()),
            GetArchRelateConstant(TaggedHashTable<NumberDictionary>::SIZE_INDEX));
    GateRef dataoffset = GetArchRelateConstant(TaggedArray::DATA_OFFSET);
    GateRef capacity = TaggedCastToInt32(Load(MachineType::UINT64, elements, ArchRelateAdd(dataoffset, capcityoffset)));
    DEFVARIABLE(count, MachineType::INT32, GetInt32Constant(1));

    GateRef pKey = Alloca(static_cast<int>(MachineRep::K_WORD32));
    GateRef keyStore = Store(MachineType::INT32, glue, pKey, GetArchRelateConstant(0), TaggedCastToInt32(key));
    StubDescriptor *getHash32Descriptor = GET_STUBDESCRIPTOR(GetHash32);
    GateRef len = GetInt32Constant(sizeof(int) / sizeof(uint8_t));
    GateRef hash =
        CallRuntime(getHash32Descriptor, glue, GetWord64Constant(FAST_STUB_ID(GetHash32)), keyStore, { pKey, len });
    DEFVARIABLE(entry, MachineType::INT32, Word32And(hash, Int32Sub(capacity, GetInt32Constant(1))));
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    GateRef element = GetKeyFromDictionary<NumberDictionary>(MachineType::TAGGED, elements, *entry);
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
    result = GetInt32Constant(-1);
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
    count = Int32Add(*count, GetInt32Constant(1));
    LoopEnd(&loopHead);
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::IsMatchInNumberDictionary(GateRef key, GateRef other)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    DEFVARIABLE(result, MachineType::BOOL, FalseConstant());
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

// int TaggedHashTable<Derived>::FindEntry(const JSTaggedValue &key) in tagged_hash_table-inl.h
GateRef Stub::FindEntryFromNameDictionary(GateRef glue, GateRef elements, GateRef key)
{
    auto env = GetEnvironment();
    Label funcEntry(env);
    env->PushCurrentLabel(&funcEntry);
    Label exit(env);
    DEFVARIABLE(result, MachineType::INT32, GetInt32Constant(-1));
    GateRef capcityoffset =
        ArchRelatePtrMul(GetArchRelateConstant(JSTaggedValue::TaggedTypeSize()),
            GetArchRelateConstant(TaggedHashTable<NumberDictionary>::SIZE_INDEX));
    GateRef dataoffset = GetArchRelateConstant(TaggedArray::DATA_OFFSET);
    GateRef capacity = TaggedCastToInt32(Load(MachineType::UINT64, elements, PtrAdd(dataoffset, capcityoffset)));
    DEFVARIABLE(count, MachineType::INT32, GetInt32Constant(1));
    DEFVARIABLE(hash, MachineType::INT32, GetInt32Constant(0));
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
        hash = TaggedCastToInt32(Load(MachineType::UINT64, key,
            GetArchRelateConstant(JSSymbol::HASHFIELD_OFFSET)));
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
            hash = CallRuntime(stringGetHashCode, glue, GetWord64Constant(FAST_STUB_ID(StringGetHashCode)), { key });
            Jump(&beforeDefineHash);
        }
        Bind(&notString);
        {
            Jump(&beforeDefineHash);
        }
    }
    Bind(&beforeDefineHash);
    // GetFirstPosition(hash, size)
    DEFVARIABLE(entry, MachineType::INT32, Word32And(*hash, Int32Sub(capacity, GetInt32Constant(1))));
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        GateRef element = GetKeyFromDictionary(MachineType::TAGGED, elements, *entry);
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
                        result = GetInt32Constant(-1);
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
            count = Int32Add(*count, GetInt32Constant(1));
            LoopEnd(&loopHead);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::IsMatchInTransitionDictionary(GateRef element, GateRef key, GateRef metaData, GateRef attr)
{
    return TruncInt32ToInt1(Word32And(ZExtInt1ToInt32(Word64Equal(element, key)),
        ZExtInt1ToInt32(Word32Equal(metaData, attr))));
}

// metaData is int32 type
GateRef Stub::FindEntryFromTransitionDictionary(GateRef glue, GateRef elements, GateRef key, GateRef metaData)
{
    auto env = GetEnvironment();
    Label funcEntry(env);
    env->PushCurrentLabel(&funcEntry);
    Label exit(env);
    DEFVARIABLE(result, MachineType::INT32, GetInt32Constant(-1));
    GateRef capcityoffset =
        ArchRelatePtrMul(GetArchRelateConstant(JSTaggedValue::TaggedTypeSize()),
            GetArchRelateConstant(TaggedHashTable<NumberDictionary>::SIZE_INDEX));
    GateRef dataoffset = GetArchRelateConstant(TaggedArray::DATA_OFFSET);
    GateRef capacity = TaggedCastToInt32(Load(MachineType::UINT64, elements, PtrAdd(dataoffset, capcityoffset)));
    DEFVARIABLE(count, MachineType::INT32, GetInt32Constant(1));
    DEFVARIABLE(hash, MachineType::INT32, GetInt32Constant(0));
    // TransitionDictionary::hash
    Label isSymbol(env);
    Label notSymbol(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label beforeDefineHash(env);
    Branch(IsSymbol(key), &isSymbol, &notSymbol);
    Bind(&isSymbol);
    {
        hash = TaggedCastToInt32(Load(MachineType::UINT64, key,
            GetArchRelateConstant(panda::ecmascript::JSSymbol::HASHFIELD_OFFSET)));
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
            hash = CallRuntime(stringGetHashCode, glue, GetWord64Constant(FAST_STUB_ID(StringGetHashCode)), { key });
            Jump(&beforeDefineHash);
        }
        Bind(&notString);
        {
            Jump(&beforeDefineHash);
        }
    }
    Bind(&beforeDefineHash);
    hash = Int32Add(*hash, metaData);
    // GetFirstPosition(hash, size)
    DEFVARIABLE(entry, MachineType::INT32, Word32And(*hash, Int32Sub(capacity, GetInt32Constant(1))));
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        GateRef element = GetKeyFromDictionary<TransitionsDictionary>(MachineType::TAGGED, elements, *entry);
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
                        result = GetInt32Constant(-1);
                        Jump(&exit);
                    }
                    Bind(&notUndefined);
                    {
                        Label isMatch(env);
                        Label notMatch(env);
                        Branch(
                            IsMatchInTransitionDictionary(element, key, metaData,
                                GetAttributesFromDictionary<TransitionsDictionary>(elements, *entry)),
                            &isMatch, &notMatch);
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
            count = Int32Add(*count, GetInt32Constant(1));
            LoopEnd(&loopHead);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::JSObjectGetProperty(MachineType returnType, GateRef obj, GateRef hClass, GateRef attr)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    DEFVARIABLE(result, returnType, GetUndefinedConstant(returnType));
    Label inlinedProp(env);
    Label notInlinedProp(env);
    GateRef attrOffset = GetOffsetFieldInPropAttr(attr);
    Branch(IsInlinedProperty(attr), &inlinedProp, &notInlinedProp);
    {
        Bind(&inlinedProp);
        {
            // GetPropertyInlinedProps
            GateRef inlinedPropsStart = GetInlinedPropsStartFromHClass(hClass);
            GateRef propOffset = Int32Mul(
                Int32Add(inlinedPropsStart, attrOffset),
                GetInt32Constant(JSTaggedValue::TaggedTypeSize()));
            result = Load(returnType, obj, ZExtInt32ToInt64(propOffset));
            Jump(&exit);
        }
        Bind(&notInlinedProp);
        {
            // compute outOfLineProp offset, get it and return
            GateRef array =
                Load(MachineType::UINT64, obj, GetArchRelateConstant(JSObject::PROPERTIES_OFFSET));
            result = GetValueFromTaggedArray(returnType, array, Int32Sub(attrOffset,
                GetInlinedPropertiesFromHClass(hClass)));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

void Stub::JSObjectSetProperty(GateRef glue, GateRef obj, GateRef hClass, GateRef attr, GateRef value)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->PushCurrentLabel(&subEntry);
    Label exit(env);
    Label inlinedProp(env);
    Label notInlinedProp(env);
    GateRef attrOffset = GetOffsetFieldInPropAttr(attr);
    Branch(IsInlinedProperty(attr), &inlinedProp, &notInlinedProp);
    {
        Bind(&inlinedProp);
        {
            SetPropertyInlinedProps(glue, obj, hClass, value, attrOffset);
            Jump(&exit);
        }
        Bind(&notInlinedProp);
        {
            // compute outOfLineProp offset, get it and return
            GateRef array = Load(MachineType::TAGGED_POINTER, obj, GetArchRelateConstant(JSObject::PROPERTIES_OFFSET));
            SetValueToTaggedArray(MachineType::TAGGED, glue, array, Int32Sub(attrOffset,
                GetInlinedPropertiesFromHClass(hClass)), value);
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->PopCurrentLabel();
    return;
}

GateRef Stub::ComputePropertyCapacityInJSObj(GateRef oldLength)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->PushCurrentLabel(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, MachineType::UINT32, GetInt32Constant(0));
    GateRef newL = Int32Add(oldLength, GetInt32Constant(JSObject::PROPERTIES_GROW_SIZE));
    Label reachMax(env);
    Label notReachMax(env);
    Branch(Int32GreaterThan(newL, GetInt32Constant(PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES)),
        &reachMax, &notReachMax);
    {
        Bind(&reachMax);
        result = GetInt32Constant(PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES);
        Jump(&exit);
        Bind(&notReachMax);
        result = newL;
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::CallSetterUtil(GateRef glue, GateRef holder, GateRef accessor, GateRef value)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->PushCurrentLabel(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, MachineType::UINT64, GetUndefinedConstant(MachineType::UINT64));
    GateRef callRes = CallRuntime(GET_STUBDESCRIPTOR(CallSetter), glue, GetWord64Constant(FAST_STUB_ID(CallSetter)), {
            glue, accessor, holder, value, TrueConstant()
        });
    Label callSuccess(env);
    Label callFail(env);
    Branch(Word32Equal(ZExtInt1ToInt32(callRes), GetInt32Constant(1)), &callSuccess, &callFail);
    {
        Bind(&callSuccess);
        result = GetUndefinedConstant(MachineType::UINT64);
        Jump(&exit);
        Bind(&callFail);
        result = GetExceptionConstant(MachineType::UINT64);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::ShouldCallSetter(GateRef receiver, GateRef holder, GateRef accessor, GateRef attr)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->PushCurrentLabel(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, MachineType::BOOL, TrueConstant());
    Label isInternal(env);
    Label notInternal(env);
    Branch(IsAccessorInternal(accessor), &isInternal, &notInternal);
    Bind(&isInternal);
    {
        Label receiverEqualsHolder(env);
        Label receiverNotEqualsHolder(env);
        Branch(Word64Equal(receiver, holder), &receiverEqualsHolder, &receiverNotEqualsHolder);
        Bind(&receiverEqualsHolder);
        {
            result = IsWritable(attr);
            Jump(&exit);
        }
        Bind(&receiverNotEqualsHolder);
        {
            result = FalseConstant();
            Jump(&exit);
        }
    }
    Bind(&notInternal);
    {
        result = TrueConstant();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

void Stub::JSHClassAddProperty(GateRef glue, GateRef receiver, GateRef key, GateRef attr)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->PushCurrentLabel(&subEntry);
    Label exit(env);
    GateRef hclass = LoadHClass(receiver);
    GateRef metaData = GetPropertyMetaDataFromAttr(attr);
    GateRef newDyn = FindTransitions(glue, receiver, hclass, key, metaData);
    Label findHClass(env);
    Label notFindHClass(env);
    Branch(Word64Equal(newDyn, GetWord64Constant(JSTaggedValue::VALUE_NULL)), &notFindHClass, &findHClass);
    Bind(&findHClass);
    {
        Jump(&exit);
    }
    Bind(&notFindHClass);
    {
        GateRef type = GetObjectType(hclass);
        GateRef size = Int32Mul(GetInlinedPropsStartFromHClass(hclass),
                                GetInt32Constant(JSTaggedValue::TaggedTypeSize()));
        GateRef inlineProps = GetInlinedPropertiesFromHClass(hclass);
        StubDescriptor *newEcmaDynClass = GET_STUBDESCRIPTOR(NewEcmaDynClass);
        GateRef newJshclass = CallRuntime(newEcmaDynClass, glue, GetWord64Constant(FAST_STUB_ID(NewEcmaDynClass)), {
                glue, size, type, inlineProps
            });
        CopyAllHClass(glue, newJshclass, hclass);
        StubDescriptor *updateLayout = GET_STUBDESCRIPTOR(UpdateLayOutAndAddTransition);
        CallRuntime(updateLayout, glue, GetWord64Constant(FAST_STUB_ID(UpdateLayOutAndAddTransition)), {
                    glue, hclass, newJshclass, key, attr
                    });
#if ECMASCRIPT_ENABLE_IC
        NotifyHClassChanged(glue, hclass, newJshclass);
#endif
        StoreHClass(glue, receiver, newJshclass);
        Jump(&exit);
    }
    Bind(&exit);
    env->PopCurrentLabel();
    return;
}

// if condition:objHandle->IsJSArray() &&
//      keyHandle.GetTaggedValue() == thread->GlobalConstants()->GetConstructorString()
GateRef Stub::SetHasConstructorCondition(GateRef glue, GateRef receiver, GateRef key)
{
    GateRef gConstOffset = PtrAdd(glue, GetArchRelateConstant(env_.GetGlueOffset(JSThread::GlueID::GLOBAL_CONST)));
    GateRef gCtorStr = Load(MachineType::TAGGED,
        gConstOffset,
        Int64Mul(GetWord64Constant(sizeof(JSTaggedValue)),
            GetWord64Constant(static_cast<uint64_t>(ConstantIndex::CONSTRUCTOR_STRING_INDEX))));
    GateRef isCtorStr = Word64Equal(key, gCtorStr);
    return Word32NotEqual(
        Word32And(SExtInt1ToInt32(IsJsArray(receiver)), SExtInt1ToInt32(isCtorStr)), GetInt32Constant(0));
}

// Note: set return exit node
GateRef Stub::AddPropertyByName(GateRef glue, GateRef receiver, GateRef key, GateRef value,
                                GateRef propertyAttributes)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->PushCurrentLabel(&subentry);
    Label exit(env);
    DEFVARIABLE(result, MachineType::UINT64, GetUndefinedConstant(MachineType::UINT64));
    Label setHasCtor(env);
    Label notSetHasCtor(env);
    Label afterCtorCon(env);
    GateRef hClass = LoadHClass(receiver);
    Branch(SetHasConstructorCondition(glue, receiver, key), &setHasCtor, &notSetHasCtor);
    {
        Bind(&setHasCtor);
        SetHasConstructorToHClass(glue, hClass, GetInt32Constant(1));
        Jump(&afterCtorCon);
        Bind(&notSetHasCtor);
        Jump(&afterCtorCon);
    }
    Bind(&afterCtorCon);
    // 0x111 : default attribute for property: writable, enumerable, configurable
    DEFVARIABLE(attr, MachineType::UINT32, propertyAttributes);
    GateRef numberOfProps = GetNumberOfPropsFromHClass(hClass);
    GateRef inlinedProperties = GetInlinedPropertiesFromHClass(hClass);
    Label hasUnusedInProps(env);
    Label noUnusedInProps(env);
    Label afterInPropsCon(env);
    Branch(Word32LessThan(numberOfProps, inlinedProperties), &hasUnusedInProps, &noUnusedInProps);
    {
        Bind(&noUnusedInProps);
        Jump(&afterInPropsCon);
        Bind(&hasUnusedInProps);
        {
            SetPropertyInlinedProps(glue, receiver, hClass, value, numberOfProps);
            attr = SetOffsetFieldInPropAttr(*attr, numberOfProps);
            attr = SetIsInlinePropsFieldInPropAttr(*attr, GetInt32Constant(1)); // 1: set inInlineProps true
            JSHClassAddProperty(glue, receiver, key, *attr);
            result = GetUndefinedConstant(MachineType::UINT64);
            Jump(&exit);
        }
    }
    Bind(&afterInPropsCon);
    DEFVARIABLE(array, MachineType::TAGGED_POINTER, GetPropertiesArray(receiver));
    DEFVARIABLE(length, MachineType::UINT32, GetLengthofTaggedArray(*array));
    Label lenIsZero(env);
    Label lenNotZero(env);
    Label afterLenCon(env);
    Branch(Word32Equal(*length, GetInt32Constant(0)), &lenIsZero, &lenNotZero);
    {
        Bind(&lenIsZero);
        {
            length = GetInt32Constant(JSObject::MIN_PROPERTIES_LENGTH);
            array = CallRuntime(GET_STUBDESCRIPTOR(NewTaggedArray), glue,
                GetWord64Constant(FAST_STUB_ID(NewTaggedArray)), { glue, *length });
            SetPropertiesArray(glue, receiver, *array);
            Jump(&afterLenCon);
        }
        Bind(&lenNotZero);
        Jump(&afterLenCon);
    }
    Bind(&afterLenCon);
    Label isDictMode(env);
    Label notDictMode(env);
    Branch(IsDictionaryMode(*array), &isDictMode, &notDictMode);
    {
        Bind(&isDictMode);
        {
            GateRef res = CallRuntime(GET_STUBDESCRIPTOR(NameDictPutIfAbsent), glue,
                GetWord64Constant(FAST_STUB_ID(NameDictPutIfAbsent)), {
                    glue, receiver, *array, key, value, *attr, FalseConstant()
                });
            SetPropertiesArray(glue, receiver, res);
            Jump(&exit);
        }
        Bind(&notDictMode);
        {
            attr = SetIsInlinePropsFieldInPropAttr(*attr, GetInt32Constant(0));
            GateRef outProps = Int32Sub(numberOfProps, inlinedProperties);
            Label isArrayFull(env);
            Label arrayNotFull(env);
            Label afterArrLenCon(env);
            Branch(Word32Equal(*length, outProps), &isArrayFull, &arrayNotFull);
            {
                Bind(&isArrayFull);
                {
                    Label ChangeToDict(env);
                    Label notChangeToDict(env);
                    Label afterDictChangeCon(env);
                    Branch(Word32Equal(*length, GetInt32Constant(JSHClass::MAX_CAPACITY_OF_OUT_OBJECTS)),
                        &ChangeToDict, &notChangeToDict);
                    {
                        Bind(&ChangeToDict);
                        {
                            attr = SetDictionaryOrderFieldInPropAttr(*attr,
                                GetInt32Constant(PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES));
                            GateRef res = CallRuntime(GET_STUBDESCRIPTOR(NameDictPutIfAbsent), glue,
                                GetWord64Constant(FAST_STUB_ID(NameDictPutIfAbsent)), {
                                    glue, receiver, *array, key, value, *attr, TrueConstant()
                                });
                            SetPropertiesArray(glue, receiver, res);
                            result = GetUndefinedConstant(MachineType::UINT64);
                            Jump(&exit);
                        }
                        Bind(&notChangeToDict);
                        Jump(&afterDictChangeCon);
                    }
                    Bind(&afterDictChangeCon);
                    GateRef capacity = ComputePropertyCapacityInJSObj(*length);
                    array = CallRuntime(GET_STUBDESCRIPTOR(CopyArray), glue,
                        GetWord64Constant(FAST_STUB_ID(CopyArray)), { glue, *array, *length, capacity });
                    SetPropertiesArray(glue, receiver, *array);
                    Jump(&afterArrLenCon);
                }
                Bind(&arrayNotFull);
                Jump(&afterArrLenCon);
            }
            Bind(&afterArrLenCon);
            {
                attr = SetOffsetFieldInPropAttr(*attr, numberOfProps);
                JSHClassAddProperty(glue, receiver, key, *attr);
                SetValueToTaggedArray(MachineType::TAGGED, glue, *array, outProps, value);
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

void Stub::ThrowTypeAndReturn(GateRef glue, int messageId, GateRef val)
{
    StubDescriptor *throwTypeError = GET_STUBDESCRIPTOR(ThrowTypeError);
    GateRef msgIntId = GetInt32Constant(messageId);
    CallRuntime(throwTypeError, glue, GetWord64Constant(FAST_STUB_ID(ThrowTypeError)), { glue, msgIntId });
    Return(val);
}

GateRef Stub::TaggedToRepresentation(GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    DEFVARIABLE(resultRep, MachineType::INT64,
                GetWord64Constant(static_cast<int32_t>(Representation::OBJECT)));
    Label isInt(env);
    Label notInt(env);

    Branch(TaggedIsInt(value), &isInt, &notInt);
    Bind(&isInt);
    {
        resultRep = GetWord64Constant(static_cast<int32_t>(Representation::INT));
        Jump(&exit);
    }
    Bind(&notInt);
    {
        Label isDouble(env);
        Label notDouble(env);
        Branch(TaggedIsDouble(value), &isDouble, &notDouble);
        Bind(&isDouble);
        {
            resultRep = GetWord64Constant(static_cast<int32_t>(Representation::DOUBLE));
            Jump(&exit);
        }
        Bind(&notDouble);
        {
            resultRep = GetWord64Constant(static_cast<int32_t>(Representation::OBJECT));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *resultRep;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::Store(MachineType type, GateRef glue, GateRef base, GateRef offset, GateRef value)
{
    auto depend = env_.GetCurrentLabel()->GetDepend();
    GateRef result;
    if (env_.IsArch64Bit()) {
        GateRef ptr = Int64Add(base, offset);
        if (type == MachineType::NATIVE_POINTER) {
            type = MachineType::INT64;
        }
        result = env_.GetCircuitBuilder().NewStoreGate(type, ptr, value, depend);
        env_.GetCurrentLabel()->SetDepend(result);
    } else if (env_.IsArch32Bit()) {
        if (type == MachineType::NATIVE_POINTER) {
            type = MachineType::INT32;
        }
        GateRef ptr = Int32Add(base, offset);
        result = env_.GetCircuitBuilder().NewStoreGate(type, ptr, value, depend);
        env_.GetCurrentLabel()->SetDepend(result);
    } else {
        UNREACHABLE();
    }
    // write barrier will implemented in IR later
    if (type == MachineType::TAGGED_POINTER || type == MachineType::TAGGED) {
        SetValueWithBarrier(glue, base, offset, value);
    }

    return result;
}

void Stub::SetValueWithBarrier(GateRef glue, GateRef obj, GateRef offset, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);

    Label isHeapObject(env);
    Branch(TaggedIsHeapObject(value), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        StubDescriptor *setValueWithBarrier = GET_STUBDESCRIPTOR(SetValueWithBarrier);
        CallRuntime(setValueWithBarrier, glue, GetWord64Constant(FAST_STUB_ID(SetValueWithBarrier)), {
                glue, obj, offset, value
            });
        Jump(&exit);
    }
    Bind(&exit);
    env->PopCurrentLabel();
    return;
}

GateRef Stub::TaggedIsString(GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    DEFVARIABLE(result, MachineType::BOOL, FalseConstant());
    Label isHeapObject(env);
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        result = Word32Equal(GetObjectType(LoadHClass(obj)),
                             GetInt32Constant(static_cast<int32_t>(JSType::STRING)));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::TaggedIsStringOrSymbol(GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    DEFVARIABLE(result, MachineType::BOOL, FalseConstant());
    Label isHeapObject(env);
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        GateRef objType = GetObjectType(LoadHClass(obj));
        result = Word32Equal(objType, GetInt32Constant(static_cast<int32_t>(JSType::STRING)));
        Label isString(env);
        Label notString(env);
        Branch(*result, &exit, &notString);
        Bind(&notString);
        {
            result = Word32Equal(objType, GetInt32Constant(static_cast<int32_t>(JSType::SYMBOL)));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::IsUtf16String(GateRef string)
{
    // compressedStringsEnabled fixed to true constant
    GateRef len = Load(MachineType::UINT32, string, GetArchRelateConstant(EcmaString::MIX_LENGTH_OFFSET));
    return Word32Equal(
        Word32And(len, GetInt32Constant(EcmaString::STRING_COMPRESSED_BIT)),
        GetInt32Constant(EcmaString::STRING_UNCOMPRESSED));
}

GateRef Stub::IsUtf8String(GateRef string)
{
    // compressedStringsEnabled fixed to true constant
    GateRef len = Load(MachineType::UINT32, string, GetArchRelateConstant(EcmaString::MIX_LENGTH_OFFSET));
    return Word32Equal(
        Word32And(len, GetInt32Constant(EcmaString::STRING_COMPRESSED_BIT)),
        GetInt32Constant(EcmaString::STRING_COMPRESSED));
}

GateRef Stub::IsInternalString(GateRef string)
{
    // compressedStringsEnabled fixed to true constant
    GateRef len = Load(MachineType::UINT32, string, GetArchRelateConstant(EcmaString::MIX_LENGTH_OFFSET));
    return Word32NotEqual(
        Word32And(len, GetInt32Constant(EcmaString::STRING_INTERN_BIT)),
        GetInt32Constant(0));
}

GateRef Stub::IsDigit(GateRef ch)
{
    return TruncInt32ToInt1(
        Word32And(SExtInt1ToInt32(Int32LessThanOrEqual(ch, GetInt32Constant('9'))),
                  SExtInt1ToInt32(Int32GreaterThanOrEqual(ch, GetInt32Constant('0')))));
}

GateRef Stub::StringToElementIndex(GateRef string)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    DEFVARIABLE(result, MachineType::INT32, GetInt32Constant(-1));
    Label greatThanZero(env);
    Label inRange(env);
    GateRef len = Load(MachineType::UINT32, string, GetArchRelateConstant(EcmaString::MIX_LENGTH_OFFSET));
    len = Word32LSR(len, GetInt32Constant(2));  // 2 : 2 means len must be right shift 2 bits
    Branch(Word32Equal(len, GetInt32Constant(0)), &exit, &greatThanZero);
    Bind(&greatThanZero);
    Branch(Int32GreaterThan(len, GetInt32Constant(MAX_INDEX_LEN)), &exit, &inRange);
    Bind(&inRange);
    {
        GateRef dataUtf16 = PtrAdd(string, GetArchRelateConstant(EcmaString::DATA_OFFSET));
        DEFVARIABLE(c, MachineType::UINT32, GetInt32Constant(0));
        Label isUtf16(env);
        Label isUtf8(env);
        Label getChar1(env);
        GateRef isUtf16String = IsUtf16String(string);
        Branch(isUtf16String, &isUtf16, &isUtf8);
        Bind(&isUtf16);
        {
            c = ZExtInt16ToInt32(Load(MachineType::INT16, dataUtf16));
            Jump(&getChar1);
        }
        Bind(&isUtf8);
        {
            c = ZExtInt8ToInt32(Load(MachineType::INT8, dataUtf16));
            Jump(&getChar1);
        }
        Bind(&getChar1);
        {
            Label isDigitZero(env);
            Label notDigitZero(env);
            Branch(Word32Equal(*c, GetInt32Constant('0')), &isDigitZero, &notDigitZero);
            Bind(&isDigitZero);
            {
                Label lengthIsOne(env);
                Branch(Word32Equal(len, GetInt32Constant(1)), &lengthIsOne, &exit);
                Bind(&lengthIsOne);
                {
                    result = GetInt32Constant(0);
                    Jump(&exit);
                }
            }
            Bind(&notDigitZero);
            {
                Label isDigit(env);
                DEFVARIABLE(i, MachineType::UINT32, GetInt32Constant(1));
                DEFVARIABLE(n, MachineType::UINT32, Int32Sub(*c, GetInt32Constant('0')));
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
                        // 2 : 2 means utf16 char width is two bytes
                        auto charOffset = ArchRelatePtrMul(ChangeInt32ToUintPtr(*i),  GetArchRelateConstant(2));
                        c = ZExtInt16ToInt32(Load(MachineType::INT16, dataUtf16, charOffset));
                        Jump(&getChar2);
                    }
                    Bind(&notUtf16);
                    {
                        c = ZExtInt8ToInt32(Load(MachineType::INT8, dataUtf16, ChangeInt32ToUintPtr(*i)));
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
                            n = Int32Add(Int32Mul(*n, GetInt32Constant(10)),
                                         Int32Sub(*c, GetInt32Constant('0')));
                            i = Int32Add(*i, GetInt32Constant(1));
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
                    Branch(Word32LessThan(*n, GetInt32Constant(JSObject::MAX_ELEMENT_INDEX)),
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

GateRef Stub::TryToElementsIndex(GateRef key)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    Label isKeyInt(env);
    Label notKeyInt(env);

    DEFVARIABLE(resultKey, MachineType::INT32, GetInt32Constant(-1));
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
                GateRef number = TaggedCastToDouble(key);
                GateRef integer = ChangeFloat64ToInt32(number);
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

GateRef Stub::LoadFromField(GateRef receiver, GateRef handlerInfo)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    Label handlerInfoIsInlinedProps(env);
    Label handlerInfoNotInlinedProps(env);
    DEFVARIABLE(result, MachineType::TAGGED, GetUndefinedConstant());
    GateRef index = HandlerBaseGetOffset(handlerInfo);
    Branch(HandlerBaseIsInlinedProperty(handlerInfo), &handlerInfoIsInlinedProps, &handlerInfoNotInlinedProps);
    Bind(&handlerInfoIsInlinedProps);
    {
        result = Load(MachineType::TAGGED, receiver, ArchRelatePtrMul(ChangeInt32ToUintPtr(index),
            GetArchRelateConstant(JSTaggedValue::TaggedTypeSize())));
        Jump(&exit);
    }
    Bind(&handlerInfoNotInlinedProps);
    {
        result = GetValueFromTaggedArray(MachineType::TAGGED, GetPropertiesArray(receiver), index);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::LoadGlobal(GateRef cell)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    Label cellIsInvalid(env);
    Label cellNotInvalid(env);
    DEFVARIABLE(result, MachineType::TAGGED, GetHoleConstant());
    Branch(IsInvalidPropertyBox(cell), &cellIsInvalid, &cellNotInvalid);
    Bind(&cellIsInvalid);
    {
        Jump(&exit);
    }
    Bind(&cellNotInvalid);
    {
        result = GetValueFromPropertyBox(cell);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::CheckPolyHClass(GateRef cachedValue, GateRef hclass)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label iLessLength(env);
    Label hasHclass(env);
    Label cachedValueNotWeak(env);
    DEFVARIABLE(i, MachineType::UINT32, GetInt32Constant(0));
    DEFVARIABLE(result, MachineType::TAGGED, GetHoleConstant());
    Branch(TaggedIsWeak(cachedValue), &exit, &cachedValueNotWeak);
    Bind(&cachedValueNotWeak);
    {
        GateRef length = GetLengthofTaggedArray(cachedValue);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Branch(Int32LessThan(*i, length), &iLessLength, &exit);
            Bind(&iLessLength);
            {
                GateRef element = GetValueFromTaggedArray(MachineType::TAGGED, cachedValue, *i);
                Branch(Word64Equal(element, hclass), &hasHclass, &loopEnd);
                Bind(&hasHclass);
                result = GetValueFromTaggedArray(MachineType::TAGGED, cachedValue, Int32Add(*i, GetInt32Constant(1)));
                Jump(&exit);
            }
            Bind(&loopEnd);
            i = Int32Add(*i, GetInt32Constant(2));  // 2 means one ic, two slot
            LoopEnd(&loopHead);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::LoadICWithHandler(GateRef glue, GateRef receiver, GateRef argHolder, GateRef argHandler)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    Label handlerIsInt(env);
    Label handlerNotInt(env);
    Label handlerInfoIsField(env);
    Label handlerInfoNotField(env);
    Label handlerInfoIsNonExist(env);
    Label handlerInfoNotNonExist(env);
    Label handlerNotPrototypeHandler(env);
    Label cellHasChanged(env);
    Label cellNotChanged(env);
    Label loopHead(env);
    Label loopEnd(env);
    DEFVARIABLE(result, MachineType::TAGGED, GetUndefinedConstant());
    DEFVARIABLE(holder, MachineType::TAGGED, argHolder);
    DEFVARIABLE(handler, MachineType::TAGGED, argHandler);

    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(TaggedIsInt(*handler), &handlerIsInt, &handlerNotInt);
        Bind(&handlerIsInt);
        {
            GateRef handlerInfo = TaggedCastToInt32(*handler);
            Branch(IsField(handlerInfo), &handlerInfoIsField, &handlerInfoNotField);
            Bind(&handlerInfoIsField);
            {
                result = LoadFromField(*holder, handlerInfo);
                Jump(&exit);
            }
            Bind(&handlerInfoNotField);
            {
                Branch(IsNonExist(handlerInfo), &handlerInfoIsNonExist, &handlerInfoNotNonExist);
                Bind(&handlerInfoIsNonExist);
                Jump(&exit);
                Bind(&handlerInfoNotNonExist);
                GateRef accessor = LoadFromField(*holder, handlerInfo);
                StubDescriptor *callGetter2 = GET_STUBDESCRIPTOR(CallGetter2);
                result = CallRuntime(callGetter2, glue, GetWord64Constant(FAST_STUB_ID(CallGetter2)), {
                        glue, receiver, *holder, accessor
                    });
                Jump(&exit);
            }
        }
        Bind(&handlerNotInt);
        Branch(TaggedIsPrototypeHandler(*handler), &loopEnd, &handlerNotPrototypeHandler);
        Bind(&loopEnd);
        {
            GateRef cellValue = GetProtoCell(*handler);
            Branch(GetHasChanged(cellValue), &cellHasChanged, &cellNotChanged);
            Bind(&cellHasChanged);
            {
                result = GetHoleConstant();
                Jump(&exit);
            }
            Bind(&cellNotChanged);
            holder = GetPrototypeHandlerHolder(*handler);
            handler = GetPrototypeHandlerHandlerInfo(*handler);
            LoopEnd(&loopHead);
        }
    }
    Bind(&handlerNotPrototypeHandler);
    result = LoadGlobal(*handler);
    Jump(&exit);

    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::LoadElement(GateRef receiver, GateRef key)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    Label indexLessZero(env);
    Label indexNotLessZero(env);
    Label lengthLessIndex(env);
    Label lengthNotLessIndex(env);
    DEFVARIABLE(result, MachineType::TAGGED, GetHoleConstant());
    GateRef index = TryToElementsIndex(key);
    Branch(Int32LessThan(index, GetInt32Constant(0)), &indexLessZero, &indexNotLessZero);
    Bind(&indexLessZero);
    {
        Jump(&exit);
    }
    Bind(&indexNotLessZero);
    {
        GateRef elements = GetPropertiesArray(receiver);
        Branch(Int32LessThanOrEqual(GetLengthofTaggedArray(elements), index), &lengthLessIndex, &lengthNotLessIndex);
        Bind(&lengthLessIndex);
        Jump(&exit);
        Bind(&lengthNotLessIndex);
        result = GetValueFromTaggedArray(MachineType::TAGGED, elements, index);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::ICStoreElement(GateRef glue, GateRef receiver, GateRef key, GateRef value, GateRef handlerInfo)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    Label indexLessZero(env);
    Label indexNotLessZero(env);
    Label handerInfoIsJSArray(env);
    Label handerInfoNotJSArray(env);
    Label indexGreaterLength(env);
    Label indexGreaterCapacity(env);
    DEFVARIABLE(result, MachineType::UINT64, GetHoleConstant(MachineType::UINT64));
    GateRef index = TryToElementsIndex(key);
    Branch(Int32LessThan(index, GetInt32Constant(0)), &indexLessZero, &indexNotLessZero);
    Bind(&indexLessZero);
    {
        Jump(&exit);
    }
    Bind(&indexNotLessZero);
    {
        Branch(HandlerBaseIsJSArray(handlerInfo), &handerInfoIsJSArray, &handerInfoNotJSArray);
        Bind(&handerInfoIsJSArray);
        {
            GateRef oldLength = GetArrayLength(receiver);
            Branch(Int32GreaterThanOrEqual(index, oldLength), &indexGreaterLength, &handerInfoNotJSArray);
            Bind(&indexGreaterLength);
            Store(MachineType::UINT64, glue, receiver, GetArchRelateConstant(panda::ecmascript::JSArray::LENGTH_OFFSET),
                  IntBuildTaggedWithNoGC(Int32Add(index, GetInt32Constant(1))));
            Jump(&handerInfoNotJSArray);
        }
        Bind(&handerInfoNotJSArray);
        {
            GateRef elements = GetElementsArray(receiver);
            GateRef capacity = GetLengthofTaggedArray(elements);
            StubDescriptor *taggedArraySetValue = GET_STUBDESCRIPTOR(TaggedArraySetValue);
            result = CallRuntime(taggedArraySetValue, glue, GetWord64Constant(FAST_STUB_ID(TaggedArraySetValue)), {
                    glue, receiver, value, elements, index, capacity
                });
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::GetArrayLength(GateRef object)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    Label lengthIsInt(env);
    Label lengthNotInt(env);
    DEFVARIABLE(result, MachineType::UINT32, GetInt32Constant(0));
    GateRef lengthOffset = GetArchRelateConstant(panda::ecmascript::JSArray::LENGTH_OFFSET);
    GateRef length = Load(MachineType::UINT64, object, lengthOffset);
    Branch(TaggedIsInt(length), &lengthIsInt, &lengthNotInt);
    Bind(&lengthIsInt);
    {
        result = TaggedCastToInt32(length);
        Jump(&exit);
    }
    Bind(&lengthNotInt);
    {
        result = ChangeFloat64ToInt32(TaggedCastToDouble(length));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::StoreICWithHandler(GateRef glue, GateRef receiver, GateRef argHolder, GateRef value, GateRef argHandler)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    Label handlerIsInt(env);
    Label handlerNotInt(env);
    Label handlerInfoIsField(env);
    Label handlerInfoNotField(env);
    Label handlerIsTransitionHandler(env);
    Label handlerNotTransitionHandler(env);
    Label handlerIsPrototypeHandler(env);
    Label handlerNotPrototypeHandler(env);
    Label handlerIsPropertyBox(env);
    Label handlerNotPropertyBox(env);
    Label cellHasChanged(env);
    Label cellHasNotChanged(env);
    Label loopHead(env);
    Label loopEnd(env);
    DEFVARIABLE(result, MachineType::UINT64, GetUndefinedConstant(MachineType::UINT64));
    DEFVARIABLE(holder, MachineType::TAGGED, argHolder);
    DEFVARIABLE(handler, MachineType::TAGGED, argHandler);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(TaggedIsInt(*handler), &handlerIsInt, &handlerNotInt);
        Bind(&handlerIsInt);
        {
            GateRef handlerInfo = TaggedCastToInt32(*handler);
            Branch(IsField(handlerInfo), &handlerInfoIsField, &handlerInfoNotField);
            Bind(&handlerInfoIsField);
            {
                StoreField(glue, receiver, value, handlerInfo);
                Jump(&exit);
            }
            Bind(&handlerInfoNotField);
            {
                GateRef accessor = LoadFromField(*holder, handlerInfo);
                StubDescriptor *callsetter2 = GET_STUBDESCRIPTOR(CallSetter2);
                result = CallRuntime(callsetter2, glue, GetWord64Constant(FAST_STUB_ID(CallSetter2)), {
                        glue, receiver, value, accessor
                    });
                Jump(&exit);
            }
        }
        Bind(&handlerNotInt);
        {
            Branch(TaggedIsTransitionHandler(*handler), &handlerIsTransitionHandler, &handlerNotTransitionHandler);
            Bind(&handlerIsTransitionHandler);
            {
                StoreWithTransition(glue, receiver, value, *handler);
                Jump(&exit);
            }
            Bind(&handlerNotTransitionHandler);
            {
                Branch(TaggedIsPrototypeHandler(*handler), &loopEnd, &handlerNotPrototypeHandler);
                Bind(&handlerNotPrototypeHandler);
                {
                    Branch(TaggedIsPropertyBox(*handler), &handlerIsPropertyBox, &handlerNotPropertyBox);
                    Bind(&handlerIsPropertyBox);
                    StoreGlobal(glue, value, *handler);
                    Jump(&exit);
                    Bind(&handlerNotPropertyBox);
                    Jump(&exit);
                }
            }
        }
        Bind(&loopEnd);
        {
            GateRef cellValue = GetProtoCell(*handler);
            Branch(GetHasChanged(cellValue), &cellHasChanged, &cellHasNotChanged);
            Bind(&cellHasChanged);
            {
                result = GetHoleConstant(MachineType::UINT64);
                Jump(&exit);
            }
            Bind(&cellHasNotChanged);
            {
                holder = GetPrototypeHandlerHolder(*handler);
                handler = GetPrototypeHandlerHandlerInfo(*handler);
                LoopEnd(&loopHead);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

void Stub::StoreField(GateRef glue, GateRef receiver, GateRef value, GateRef handler)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    Label handlerIsInlinedProperty(env);
    Label handlerNotInlinedProperty(env);
    GateRef index = HandlerBaseGetOffset(handler);
    Branch(HandlerBaseIsInlinedProperty(handler), &handlerIsInlinedProperty, &handlerNotInlinedProperty);
    Bind(&handlerIsInlinedProperty);
    {
        Store(MachineType::TAGGED, glue, receiver,
            ArchRelatePtrMul(ChangeInt32ToUintPtr(index), GetArchRelateConstant(JSTaggedValue::TaggedTypeSize())),
            value);
        Jump(&exit);
    }
    Bind(&handlerNotInlinedProperty);
    {
        GateRef array = GetPropertiesArray(receiver);
        SetValueToTaggedArray(MachineType::TAGGED, glue, array, index, value);
        Jump(&exit);
    }
    Bind(&exit);
    env->PopCurrentLabel();
}

void Stub::StoreWithTransition(GateRef glue, GateRef receiver, GateRef value, GateRef handler)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);

    Label handlerInfoIsInlinedProps(env);
    Label handlerInfoNotInlinedProps(env);
    Label indexMoreCapacity(env);
    Label indexLessCapacity(env);
    GateRef newHClass = GetTransitionFromHClass(handler);
    StoreHClass(glue, receiver, newHClass);
    GateRef handlerInfo = TaggedCastToInt32(GetTransitionHandlerInfo(handler));
    Branch(HandlerBaseIsInlinedProperty(handlerInfo), &handlerInfoIsInlinedProps, &handlerInfoNotInlinedProps);
    Bind(&handlerInfoNotInlinedProps);
    {
        GateRef array = GetPropertiesArray(receiver);
        GateRef capacity = GetLengthofTaggedArray(array);
        GateRef index = HandlerBaseGetOffset(handlerInfo);
        Branch(Int32GreaterThanOrEqual(index, capacity), &indexMoreCapacity, &indexLessCapacity);
        Bind(&indexMoreCapacity);
        {
            StubDescriptor *propertiesSetValue = GET_STUBDESCRIPTOR(PropertiesSetValue);
            CallRuntime(propertiesSetValue, glue, GetWord64Constant(FAST_STUB_ID(PropertiesSetValue)), {
                    glue, receiver, value, array, capacity, index
                });
            Jump(&exit);
        }
        Bind(&indexLessCapacity);
        {
            Store(MachineType::UINT64, glue, ArchRelateAdd(array, GetArchRelateConstant(TaggedArray::DATA_OFFSET)),
                ArchRelatePtrMul(ChangeInt32ToUintPtr(index), GetArchRelateConstant(JSTaggedValue::TaggedTypeSize())),
                value);
            Jump(&exit);
        }
    }
    Bind(&handlerInfoIsInlinedProps);
    {
        StoreField(glue, receiver, value, handlerInfo);
        Jump(&exit);
    }
    Bind(&exit);
    env->PopCurrentLabel();
}

GateRef Stub::StoreGlobal(GateRef glue, GateRef value, GateRef cell)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    Label cellIsInvalid(env);
    Label cellNotInvalid(env);
    DEFVARIABLE(result, MachineType::UINT64, GetHoleConstant(MachineType::UINT64));
    Branch(IsInvalidPropertyBox(cell), &cellIsInvalid, &cellNotInvalid);
    Bind(&cellIsInvalid);
    {
        Jump(&exit);
    }
    Bind(&cellNotInvalid);
    {
        Store(MachineType::TAGGED, glue, cell, GetArchRelateConstant(PropertyBox::VALUE_OFFSET), value);
        result = GetUndefinedConstant(MachineType::UINT64);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::GetPropertyByIndex(GateRef glue, GateRef receiver, GateRef index)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    DEFVARIABLE(result, MachineType::TAGGED, GetHoleConstant());
    DEFVARIABLE(holder, MachineType::TAGGED, receiver);
    Label exit(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label afterLoop(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        GateRef hclass = LoadHClass(*holder);
        GateRef jsType = GetObjectType(hclass);
        Label isSpecialIndexed(env);
        Label notSpecialIndexed(env);
        Branch(IsSpecialIndexedObj(jsType), &isSpecialIndexed, &notSpecialIndexed);
        Bind(&isSpecialIndexed);
        {
            Label isSpecialContainer(env);
            Label notSpecialContainer(env);
            // Add SpecialContainer
            Branch(IsSpecialContainer(jsType), &isSpecialContainer, &notSpecialContainer);
            Bind(&isSpecialContainer);
            {
                result = GetContainerProperty(glue, *holder, index, jsType);
                Jump(&exit);
            }
            Bind(&notSpecialContainer);
            {
                result = GetHoleConstant();
                Jump(&exit);
            }
        }
        Bind(&notSpecialIndexed);
        {
            GateRef elements = GetElementsArray(*holder);
            Label isDictionaryElement(env);
            Label notDictionaryElement(env);
            Branch(IsDictionaryElement(hclass), &isDictionaryElement, &notDictionaryElement);
            Bind(&notDictionaryElement);
            {
                Label lessThanLength(env);
                Label notLessThanLength(env);
                Branch(Word32LessThan(index, GetLengthofTaggedArray(elements)), &lessThanLength, &notLessThanLength);
                Bind(&lessThanLength);
                {
                    Label notHole(env);
                    Label isHole(env);
                    GateRef value = GetValueFromTaggedArray(MachineType::TAGGED, elements, index);
                    Branch(TaggedIsNotHole(value), &notHole, &isHole);
                    Bind(&notHole);
                    {
                        result = value;
                        Jump(&exit);
                    }
                    Bind(&isHole);
                    {
                        Jump(&loopExit);
                    }
                }
                Bind(&notLessThanLength);
                {
                    result = GetHoleConstant();
                    Jump(&exit);
                }
            }
            Bind(&isDictionaryElement);
            {
                GateRef entry = FindElementFromNumberDictionary(glue, elements, IntBuildTaggedWithNoGC(index));
                Label notNegtiveOne(env);
                Label negtiveOne(env);
                Branch(Word32NotEqual(entry, GetInt32Constant(-1)), &notNegtiveOne, &negtiveOne);
                Bind(&notNegtiveOne);
                {
                    GateRef attr = GetAttributesFromDictionary<NumberDictionary>(elements, entry);
                    GateRef value = GetValueFromDictionary<NumberDictionary>(MachineType::TAGGED, elements, entry);
                    Label isAccessor(env);
                    Label notAccessor(env);
                    Branch(IsAccessor(attr), &isAccessor, &notAccessor);
                    Bind(&isAccessor);
                    {
                        Label isInternal(env);
                        Label notInternal(env);
                        Branch(IsAccessorInternal(value), &isInternal, &notInternal);
                        Bind(&isInternal);
                        {
                            StubDescriptor *callInternalGetter = GET_STUBDESCRIPTOR(CallInternalGetter);
                            result = CallRuntime(callInternalGetter, glue,
                                GetWord64Constant(FAST_STUB_ID(CallInternalGetter)), { glue, value, *holder });
                            Jump(&exit);
                        }
                        Bind(&notInternal);
                        {
                            StubDescriptor *callGetter = GET_STUBDESCRIPTOR(CallGetter);
                            result = CallRuntime(callGetter, glue,
                                GetWord64Constant(FAST_STUB_ID(CallGetter)), { glue, value, receiver });
                            Jump(&exit);
                        }
                    }
                    Bind(&notAccessor);
                    {
                        result = value;
                        Jump(&exit);
                    }
                }
                Bind(&negtiveOne);
                Jump(&loopExit);
            }
            Bind(&loopExit);
            {
                holder = GetPrototypeFromHClass(LoadHClass(*holder));
                Branch(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
            }
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&afterLoop);
        {
            result = GetUndefinedConstant();
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::GetPropertyByName(GateRef glue, GateRef receiver, GateRef key)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    DEFVARIABLE(result, MachineType::TAGGED, GetHoleConstant());
    DEFVARIABLE(holder, MachineType::TAGGED, receiver);
    Label exit(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label afterLoop(env);
    // a do-while loop
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        // auto *hclass = holder.GetTaggedObject()->GetClass()
        // JSType jsType = hclass->GetObjectType()
        GateRef hClass = LoadHClass(*holder);
        GateRef jsType = GetObjectType(hClass);
        Label isSIndexObj(env);
        Label notSIndexObj(env);
        // if branch condition : IsSpecialIndexedObj(jsType)
        Branch(IsSpecialIndexedObj(jsType), &isSIndexObj, &notSIndexObj);
        Bind(&isSIndexObj);
        {
            result = GetHoleConstant();
            Jump(&exit);
        }
        Bind(&notSIndexObj);
        {
            Label isDicMode(env);
            Label notDicMode(env);
            // if branch condition : LIKELY(!hclass->IsDictionaryMode())
            Branch(IsDictionaryModeByHClass(hClass), &isDicMode, &notDicMode);
            Bind(&notDicMode);
            {
                // LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetAttributes().GetTaggedObject())
                GateRef layOutInfo = GetLayoutFromHClass(hClass);
                // int propsNumber = hclass->NumberOfPropsFromHClass()
                GateRef propsNum = GetNumberOfPropsFromHClass(hClass);
                // int entry = layoutInfo->FindElementWithCache(thread, hclass, key, propsNumber)
                GateRef entry = FindElementWithCache(glue, layOutInfo, hClass, key, propsNum);
                Label hasEntry(env);
                Label noEntry(env);
                // if branch condition : entry != -1
                Branch(Word32NotEqual(entry, GetInt32Constant(-1)), &hasEntry, &noEntry);
                Bind(&hasEntry);
                {
                    // PropertyAttributes attr(layoutInfo->GetAttr(entry))
                    GateRef propAttr = GetPropAttrFromLayoutInfo(layOutInfo, entry);
                    GateRef attr = TaggedCastToInt32(propAttr);
                    // auto value = JSObject::Cast(holder)->GetProperty(hclass, attr)
                    GateRef value = JSObjectGetProperty(MachineType::TAGGED, *holder, hClass, attr);
                    Label isAccessor(env);
                    Label notAccessor(env);
                    Branch(IsAccessor(attr), &isAccessor, &notAccessor);
                    Bind(&isAccessor);
                    {
                        Label isInternal(env);
                        Label notInternal(env);
                        Branch(IsAccessorInternal(value), &isInternal, &notInternal);
                        Bind(&isInternal);
                        {
                            StubDescriptor *callInternalGetter = GET_STUBDESCRIPTOR(CallInternalGetter);
                            result = CallRuntime(callInternalGetter, glue,
                                GetWord64Constant(FAST_STUB_ID(CallInternalGetter)), { glue, value, *holder });
                            Jump(&exit);
                        }
                        Bind(&notInternal);
                        {
                            StubDescriptor *callGetter = GET_STUBDESCRIPTOR(CallGetter);
                            result = CallRuntime(callGetter, glue,
                                GetWord64Constant(FAST_STUB_ID(CallGetter)), { glue, value, receiver });
                            Jump(&exit);
                        }
                    }
                    Bind(&notAccessor);
                    {
                        result = value;
                        Jump(&exit);
                    }
                }
                Bind(&noEntry);
                {
                    Jump(&loopExit);
                }
            }
            Bind(&isDicMode);
            {
                GateRef array = GetPropertiesArray(*holder);
                // int entry = dict->FindEntry(key)
                GateRef entry = FindEntryFromNameDictionary(glue, array, key);
                Label notNegtiveOne(env);
                Label negtiveOne(env);
                // if branch condition : entry != -1
                Branch(Word32NotEqual(entry, GetInt32Constant(-1)), &notNegtiveOne, &negtiveOne);
                Bind(&notNegtiveOne);
                {
                    // auto value = dict->GetValue(entry)
                    GateRef attr = GetAttributesFromDictionary(array, entry);
                    // auto attr = dict->GetAttributes(entry)
                    GateRef value = GetValueFromDictionary(MachineType::TAGGED, array, entry);
                    Label isAccessor1(env);
                    Label notAccessor1(env);
                    // if branch condition : UNLIKELY(attr.IsAccessor())
                    Branch(IsAccessor(attr), &isAccessor1, &notAccessor1);
                    Bind(&isAccessor1);
                    {
                        Label isInternal1(env);
                        Label notInternal1(env);
                        Branch(IsAccessorInternal(value), &isInternal1, &notInternal1);
                        Bind(&isInternal1);
                        {
                            StubDescriptor *callAccessorGetter1 = GET_STUBDESCRIPTOR(CallInternalGetter);
                            result = CallRuntime(callAccessorGetter1, glue,
                                GetWord64Constant(FAST_STUB_ID(CallInternalGetter)), { glue, value, *holder });
                            Jump(&exit);
                        }
                        Bind(&notInternal1);
                        {
                            StubDescriptor *callGetter1 = GET_STUBDESCRIPTOR(CallGetter);
                            result = CallRuntime(callGetter1, glue,
                                GetWord64Constant(FAST_STUB_ID(CallGetter)), { glue, value, receiver });
                            Jump(&exit);
                        }
                    }
                    Bind(&notAccessor1);
                    {
                        result = value;
                        Jump(&exit);
                    }
                }
                Bind(&negtiveOne);
                Jump(&loopExit);
            }
            Bind(&loopExit);
            {
                // holder = hclass->GetPrototype()
                holder = GetPrototypeFromHClass(LoadHClass(*holder));
                // loop condition for a do-while loop
                Branch(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
            }
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&afterLoop);
        {
            result = GetUndefinedConstant();
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

void Stub::CopyAllHClass(GateRef glue, GateRef dstHClass, GateRef srcHClass)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    Label isEcmaObject(env);
    Label notEcmaObject(env);
    auto proto = GetPrototypeFromHClass(srcHClass);
    Branch(IsEcmaObject(proto), &isEcmaObject, &exit);
    Bind(&isEcmaObject);
    {
        SetIsProtoTypeToHClass(glue, LoadHClass(proto), TrueConstant());
        Jump(&exit);
    }
    Bind(&exit);
    SetPrototypeToHClass(MachineType::TAGGED_POINTER, glue, dstHClass, proto);
    SetBitFieldToHClass(glue, dstHClass, GetBitFieldFromHClass(srcHClass));
    SetNumberOfPropsToHClass(glue, dstHClass, GetNumberOfPropsFromHClass(srcHClass));
    SetParentToHClass(MachineType::INT64, glue, dstHClass, GetWord64Constant(JSTaggedValue::VALUE_NULL));
    SetTransitionsToHClass(MachineType::INT64, glue, dstHClass, GetWord64Constant(JSTaggedValue::VALUE_NULL));
    SetProtoChangeDetailsToHClass(MachineType::INT64, glue, dstHClass, GetWord64Constant(JSTaggedValue::VALUE_NULL));
    SetEnumCacheToHClass(MachineType::INT64, glue, dstHClass, GetWord64Constant(JSTaggedValue::VALUE_NULL));
    SetLayoutToHClass(glue, dstHClass, GetLayoutFromHClass(srcHClass));
    env->PopCurrentLabel();
    return;
}

GateRef Stub::FindTransitions(GateRef glue, GateRef receiver, GateRef hclass, GateRef key, GateRef metaData)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    GateRef transitionOffset = GetArchRelateConstant(JSHClass::TRANSTIONS_OFFSET);
    GateRef transition = Load(MachineType::TAGGED_POINTER, hclass, transitionOffset);
    DEFVARIABLE(result, MachineType::TAGGED, transition);

    Label notNull(env);
    Branch(Word64Equal(transition, GetWord64Constant(JSTaggedValue::VALUE_NULL)), &exit, &notNull);
    Bind(&notNull);
    {
        Label isJSHClass(env);
        Label notJSHClass(env);
        Branch(IsJSHClass(transition), &isJSHClass, &notJSHClass);
        Bind(&isJSHClass);
        {
            GateRef propNums = GetNumberOfPropsFromHClass(transition);
            GateRef last = Int32Sub(propNums, GetInt32Constant(-1));
            GateRef layoutInfo = GetLayoutFromHClass(transition);
            GateRef cachedKey = GetKeyFromLayoutInfo(layoutInfo, last);
            GateRef cachedAttr = TaggedCastToInt32(GetPropAttrFromLayoutInfo(layoutInfo, last));
            GateRef cachedMetaData = GetPropertyMetaDataFromAttr(cachedAttr);
            Label keyMatch(env);
            Label isMatch(env);
            Label notMatch(env);
            Branch(Word64Equal(cachedKey, key), &keyMatch, &notMatch);
            Bind(&keyMatch);
            {
                Branch(Word64Equal(metaData, cachedMetaData), &isMatch, &notMatch);
                Bind(&isMatch);
                {
#if ECMASCRIPT_ENABLE_IC
                    NotifyHClassChanged(glue, hclass, transition);
#endif
                    StoreHClass(glue, receiver, transition);
                    Jump(&exit);
                }
            }
            Bind(&notMatch);
            {
                result = GetNullConstant();
                Jump(&exit);
            }
        }
        Bind(&notJSHClass);
        {
            // need to find from dictionary
            GateRef entry = FindEntryFromTransitionDictionary(glue, transition, key, metaData);
            Label isFound(env);
            Label notFound(env);
            Branch(Word32NotEqual(entry, GetInt32Constant(-1)), &isFound, &notFound);
            Bind(&isFound);
            auto newHClass = GetValueFromDictionary<TransitionsDictionary>(
                MachineType::TAGGED_POINTER, transition, entry);
            result = newHClass;
#if ECMASCRIPT_ENABLE_IC
            NotifyHClassChanged(glue, hclass, newHClass);
#endif
            StoreHClass(glue, receiver, newHClass);
            Jump(&exit);
            Bind(&notFound);
            result = GetNullConstant();
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::SetPropertyByIndex(GateRef glue, GateRef receiver, GateRef index, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    DEFVARIABLE(returnValue, MachineType::UINT64, GetHoleConstant(MachineType::UINT64));
    DEFVARIABLE(holder, MachineType::TAGGED, receiver);
    Label exit(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label afterLoop(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        GateRef hclass = LoadHClass(*holder);
        GateRef jsType = GetObjectType(hclass);
        Label isSpecialIndex(env);
        Label notSpecialIndex(env);
        Branch(IsSpecialIndexedObj(jsType), &isSpecialIndex, &notSpecialIndex);
        Bind(&isSpecialIndex);
        {
            Label isSpecialContainer(env);
            Label notSpecialContainer(env);
            // Add SpecialContainer
            Branch(IsSpecialContainer(jsType), &isSpecialContainer, &notSpecialContainer);
            Bind(&isSpecialContainer);
            {
                returnValue = SetContainerProperty(glue, *holder, index, value, jsType);
                Jump(&exit);
            }
            Bind(&notSpecialContainer);
            {
                returnValue = GetHoleConstant(MachineType::UINT64);
                Jump(&exit);
            }
        }
        Bind(&notSpecialIndex);
        {
            GateRef elements = GetElementsArray(*holder);
            Label isDictionaryElement(env);
            Label notDictionaryElement(env);
            Branch(IsDictionaryElement(hclass), &isDictionaryElement, &notDictionaryElement);
            Bind(&notDictionaryElement);
            {
                Label isReceiver(env);
                Label notReceiver(env);
                Branch(Word64Equal(*holder, receiver), &isReceiver, &notReceiver);
                Bind(&isReceiver);
                {
                    GateRef length = GetLengthofTaggedArray(elements);
                    Label inRange(env);
                    Branch(Word64LessThan(index, length), &inRange, &loopExit);
                    Bind(&inRange);
                    {
                        GateRef value1 = GetValueFromTaggedArray(MachineType::TAGGED, elements, index);
                        Label notHole(env);
                        Branch(Word64NotEqual(value1, GetHoleConstant()), &notHole, &loopExit);
                        Bind(&notHole);
                        {
                            SetValueToTaggedArray(MachineType::TAGGED, glue, elements, index, value);
                            returnValue = GetUndefinedConstant(MachineType::UINT64);
                            Jump(&exit);
                        }
                    }
                }
                Bind(&notReceiver);
                Jump(&afterLoop);
            }
            Bind(&isDictionaryElement);
            returnValue = GetHoleConstant(MachineType::UINT64);
            Jump(&exit);
        }
        Bind(&loopExit);
        {
            holder = GetPrototypeFromHClass(LoadHClass(*holder));
            Branch(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
        }
    }
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    {
        Label isExtensible(env);
        Label notExtensible(env);
        Branch(IsExtensible(receiver), &isExtensible, &notExtensible);
        Bind(&isExtensible);
        {
            StubDescriptor *addElementInternal = GET_STUBDESCRIPTOR(AddElementInternal);
            GateRef result =
                CallRuntime(addElementInternal, glue, GetWord64Constant(FAST_STUB_ID(AddElementInternal)), {
                        glue, receiver, index, value, GetInt32Constant(PropertyAttributes::GetDefaultAttributes())
                    });
            Label success(env);
            Label failed(env);
            Branch(result, &success, &failed);
            Bind(&success);
            returnValue = GetUndefinedConstant(MachineType::UINT64);
            Jump(&exit);
            Bind(&failed);
            returnValue = GetExceptionConstant(MachineType::UINT64);
            Jump(&exit);
        }
        Bind(&notExtensible);
        {
            GateRef taggedId = GetInt32Constant(GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensible));
            CallRuntime(GET_STUBDESCRIPTOR(ThrowTypeError), glue,
                GetWord64Constant(FAST_STUB_ID(ThrowTypeError)), { glue, taggedId });
            returnValue = GetExceptionConstant(MachineType::UINT64);
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *returnValue;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::SetPropertyByName(GateRef glue, GateRef receiver, GateRef key, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    DEFVARIABLE(result, MachineType::UINT64, GetHoleConstant(MachineType::UINT64));
    DEFVARIABLE(holder, MachineType::TAGGED_POINTER, receiver);
    Label exit(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label afterLoop(env);
    // a do-while loop
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        // auto *hclass = holder.GetTaggedObject()->GetClass()
        // JSType jsType = hclass->GetObjectType()
        GateRef hClass = LoadHClass(*holder);
        GateRef jsType = GetObjectType(hClass);
        Label isSIndexObj(env);
        Label notSIndexObj(env);
        // if branch condition : IsSpecialIndexedObj(jsType)
        Branch(IsSpecialIndexedObj(jsType), &isSIndexObj, &notSIndexObj);
        Bind(&isSIndexObj);
        {
            Label isSpecialContainer(env);
            Label notSpecialContainer(env);
            // Add SpecialContainer
            Branch(IsSpecialContainer(jsType), &isSpecialContainer, &notSpecialContainer);
            Bind(&isSpecialContainer);
            {
                GateRef taggedId = GetInt32Constant(GET_MESSAGE_STRING_ID(CanNotSetPropertyOnContainer));
                CallRuntime(GET_STUBDESCRIPTOR(ThrowTypeError), glue, GetWord64Constant(FAST_STUB_ID(ThrowTypeError)),
                            {glue, taggedId});
                result = GetExceptionConstant(MachineType::UINT64);
                Jump(&exit);
            }
            Bind(&notSpecialContainer);
            {
                result = GetHoleConstant(MachineType::UINT64);
                Jump(&exit);
            }
        }
        Bind(&notSIndexObj);
        {
            Label isDicMode(env);
            Label notDicMode(env);
            // if branch condition : LIKELY(!hclass->IsDictionaryMode())
            Branch(IsDictionaryModeByHClass(hClass), &isDicMode, &notDicMode);
            Bind(&notDicMode);
            {
                // LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetAttributes().GetTaggedObject())
                GateRef layOutInfo = GetLayoutFromHClass(hClass);
                // int propsNumber = hclass->NumberOfPropsFromHClass()
                GateRef propsNum = GetNumberOfPropsFromHClass(hClass);
                // int entry = layoutInfo->FindElementWithCache(thread, hclass, key, propsNumber)
                GateRef entry = FindElementWithCache(glue, layOutInfo, hClass, key, propsNum);
                Label hasEntry(env);
                Label noEntry(env);
                // if branch condition : entry != -1
                Branch(Word32NotEqual(entry, GetInt32Constant(-1)), &hasEntry, &noEntry);
                Bind(&hasEntry);
                {
                    // PropertyAttributes attr(layoutInfo->GetAttr(entry))
                    GateRef propAttr = GetPropAttrFromLayoutInfo(layOutInfo, entry);
                    GateRef attr = TaggedCastToInt32(propAttr);
                    Label isAccessor(env);
                    Label notAccessor(env);
                    Label afterIsAccessor(env);
                    Branch(IsAccessor(attr), &isAccessor, &notAccessor);
                    Bind(&isAccessor);
                    {
                        // auto accessor = JSObject::Cast(holder)->GetProperty(hclass, attr)
                        GateRef accessor = JSObjectGetProperty(MachineType::TAGGED, *holder, hClass, attr);
                        Label shouldCall(env);
                        Label shouldNotCall(env);
                        // ShouldCallSetter(receiver, *holder, accessor, attr)
                        Branch(ShouldCallSetter(receiver, *holder, accessor, attr), &shouldCall, &shouldNotCall);
                        {
                            Bind(&shouldCall);
                            result = CallSetterUtil(glue, receiver, accessor, value);
                            Jump(&exit);
                            Bind(&shouldNotCall);
                            Jump(&afterIsAccessor);
                        }
                    }
                    Bind(&notAccessor);
                    Jump(&afterIsAccessor);
                    Bind(&afterIsAccessor);
                    {
                        Label writable(env);
                        Label notWritable(env);
                        Label afterIsWritable(env);
                        Branch(IsWritable(attr), &writable, &notWritable);
                        {
                            Bind(&writable);
                            Jump(&afterIsWritable);
                            Bind(&notWritable);
                            GateRef taggedId = GetInt32Constant(GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensible));
                            CallRuntime(GET_STUBDESCRIPTOR(ThrowTypeError), glue,
                                GetWord64Constant(FAST_STUB_ID(ThrowTypeError)), { glue, taggedId });
                            result = GetExceptionConstant(MachineType::UINT64);
                            Jump(&exit);
                        }
                        Bind(&afterIsWritable);
                        {
                            Label holdEqualsRecv(env);
                            Label holdNotEqualsRecv(env);
                            Label afterComp(env);
                            Branch(Word64Equal(*holder, receiver), &holdEqualsRecv, &holdNotEqualsRecv);
                            {
                                Bind(&holdEqualsRecv);
                                Jump(&afterComp);
                                Bind(&holdNotEqualsRecv);
                                Jump(&afterLoop);
                            }
                            Bind(&afterComp);
                            // JSObject::Cast(holder)->SetProperty(thread, hclass, attr, value)
                            // return JSTaggedValue::Undefined()
                            JSObjectSetProperty(glue, *holder, hClass, attr, value);
                            result = GetUndefinedConstant(MachineType::UINT64);
                            Jump(&exit);
                        }
                    }
                }
                Bind(&noEntry);
                {
                    Jump(&loopExit);
                }
            }
            Bind(&isDicMode);
            {
                GateRef array = GetPropertiesArray(*holder);
                // int entry = dict->FindEntry(key)
                GateRef entry1 = FindEntryFromNameDictionary(glue, array, key);
                Label notNegtiveOne(env);
                Label negtiveOne(env);
                // if branch condition : entry != -1
                Branch(Word32NotEqual(entry1, GetInt32Constant(-1)), &notNegtiveOne, &negtiveOne);
                Bind(&notNegtiveOne);
                {
                    // auto attr = dict->GetAttributes(entry)
                    GateRef attr1 = GetAttributesFromDictionary(array, entry1);
                    Label isAccessor1(env);
                    Label notAccessor1(env);
                    Label afterIsAccessor1(env);
                    // if branch condition : UNLIKELY(attr.IsAccessor())
                    Branch(IsAccessor(attr1), &isAccessor1, &notAccessor1);
                    Bind(&isAccessor1);
                    {
                        Label isInternal1(env);
                        Label notInternal1(env);
                        // auto accessor = dict->GetValue(entry)
                        GateRef accessor1 = GetValueFromDictionary(MachineType::TAGGED, array, entry1);
                        Label shouldCall1(env);
                        Label shouldNotCall1(env);
                        Branch(ShouldCallSetter(receiver, *holder, accessor1, attr1), &shouldCall1, &shouldNotCall1);
                        Bind(&shouldCall1);
                        result = CallSetterUtil(glue, receiver, accessor1, value);
                        Jump(&exit);
                        Bind(&shouldNotCall1);
                        Jump(&afterIsAccessor1);
                    }
                    Bind(&notAccessor1);
                    Jump(&afterIsAccessor1);
                    Bind(&afterIsAccessor1);
                    {
                        Label writable1(env);
                        Label notWritable1(env);
                        Label afterIsWritable1(env);
                        Branch(IsWritable(attr1), &writable1, &notWritable1);
                        {
                            Bind(&writable1);
                            Jump(&afterIsWritable1);
                            Bind(&notWritable1);
                            GateRef taggedId = GetInt32Constant(GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensible));
                            CallRuntime(GET_STUBDESCRIPTOR(ThrowTypeError), glue,
                                GetWord64Constant(FAST_STUB_ID(ThrowTypeError)), { glue, taggedId });
                            result = GetExceptionConstant(MachineType::UINT64);
                            Jump(&exit);
                        }
                        Bind(&afterIsWritable1);
                        {
                            Label holdEqualsRecv1(env);
                            Label holdNotEqualsRecv1(env);
                            Label afterComp1(env);
                            Branch(Word64Equal(*holder, receiver), &holdEqualsRecv1, &holdNotEqualsRecv1);
                            {
                                Bind(&holdEqualsRecv1);
                                Jump(&afterComp1);
                                Bind(&holdNotEqualsRecv1);
                                Jump(&afterLoop);
                            }
                            Bind(&afterComp1);
                            // dict->UpdateValue(thread, entry, value)
                            // return JSTaggedValue::Undefined()
                            UpdateValueInDict(glue, array, entry1, value);
                            result = GetUndefinedConstant(MachineType::UINT64);
                            Jump(&exit);
                        }
                    }
                }
                Bind(&negtiveOne);
                Jump(&loopExit);
            }
            Bind(&loopExit);
            {
                // holder = hclass->GetPrototype()
                holder = GetPrototypeFromHClass(LoadHClass(*holder));
                // loop condition for a do-while loop
                Branch(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
            }
        }
    }
    Bind(&loopEnd);
    LoopEnd(&loopHead);
    Bind(&afterLoop);
    {
        Label extensible(env);
        Label inextensible(env);
        Label afterExtenCon(env);
        Branch(IsExtensible(receiver), &extensible, &inextensible);
        {
            Bind(&extensible);
            Jump(&afterExtenCon);
            Bind(&inextensible);
            GateRef taggedId = GetInt32Constant(GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensible));
            CallRuntime(GET_STUBDESCRIPTOR(ThrowTypeError), glue, GetWord64Constant(FAST_STUB_ID(ThrowTypeError)),
                {glue, taggedId});
            result = GetExceptionConstant(MachineType::UINT64);
            Jump(&exit);
        }
        Bind(&afterExtenCon);
        result = AddPropertyByName(glue, receiver, key, value,
                                   GetInt32Constant(PropertyAttributes::GetDefaultAttributes()));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::SetPropertyByNameWithOwn(GateRef glue, GateRef receiver, GateRef key, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    DEFVARIABLE(result, MachineType::UINT64, GetHoleConstant(MachineType::UINT64));
    DEFVARIABLE(holder, MachineType::TAGGED_POINTER, receiver);
    Label exit(env);
    Label ifEnd(env);
    // auto *hclass = holder.GetTaggedObject()->GetClass()
    // JSType jsType = hclass->GetObjectType()
    GateRef hClass = LoadHClass(*holder);
    GateRef jsType = GetObjectType(hClass);
    Label isSIndexObj(env);
    Label notSIndexObj(env);
    // if branch condition : IsSpecialIndexedObj(jsType)
    Branch(IsSpecialIndexedObj(jsType), &isSIndexObj, &notSIndexObj);
    Bind(&isSIndexObj);
    {
        Label isSpecialContainer(env);
        Label notSpecialContainer(env);
        // Add SpecialContainer
        Branch(IsSpecialContainer(jsType), &isSpecialContainer, &notSpecialContainer);
        Bind(&isSpecialContainer);
        {
            GateRef taggedId = GetInt32Constant(GET_MESSAGE_STRING_ID(CanNotSetPropertyOnContainer));
            CallRuntime(GET_STUBDESCRIPTOR(ThrowTypeError), glue, GetWord64Constant(FAST_STUB_ID(ThrowTypeError)),
                        {glue, taggedId});
            result = GetExceptionConstant(MachineType::UINT64);
            Jump(&exit);
        }
        Bind(&notSpecialContainer);
        {
            result = GetHoleConstant(MachineType::UINT64);
            Jump(&exit);
        }
    }
    Bind(&notSIndexObj);
    {
        Label isDicMode(env);
        Label notDicMode(env);
        // if branch condition : LIKELY(!hclass->IsDictionaryMode())
        Branch(IsDictionaryModeByHClass(hClass), &isDicMode, &notDicMode);
        Bind(&notDicMode);
        {
            // LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetAttributes().GetTaggedObject())
            GateRef layOutInfo = GetLayoutFromHClass(hClass);
            // int propsNumber = hclass->NumberOfPropsFromHClass()
            GateRef propsNum = GetNumberOfPropsFromHClass(hClass);
            // int entry = layoutInfo->FindElementWithCache(thread, hclass, key, propsNumber)
            GateRef entry = FindElementWithCache(glue, layOutInfo, hClass, key, propsNum);
            Label hasEntry(env);
            Label noEntry(env);
            // if branch condition : entry != -1
            Branch(Word32NotEqual(entry, GetInt32Constant(-1)), &hasEntry, &noEntry);
            Bind(&hasEntry);
            {
                // PropertyAttributes attr(layoutInfo->GetAttr(entry))
                GateRef propAttr = GetPropAttrFromLayoutInfo(layOutInfo, entry);
                GateRef attr = TaggedCastToInt32(propAttr);
                Label isAccessor(env);
                Label notAccessor(env);
                Label afterIsAccessor(env);
                Branch(IsAccessor(attr), &isAccessor, &notAccessor);
                Bind(&isAccessor);
                {
                    // auto accessor = JSObject::Cast(holder)->GetProperty(hclass, attr)
                    GateRef accessor = JSObjectGetProperty(MachineType::TAGGED, *holder, hClass, attr);
                    Label shouldCall(env);
                    Label shouldNotCall(env);
                    // ShouldCallSetter(receiver, *holder, accessor, attr)
                    Branch(ShouldCallSetter(receiver, *holder, accessor, attr), &shouldCall, &shouldNotCall);
                    {
                        Bind(&shouldCall);
                        result = CallSetterUtil(glue, receiver, accessor, value);
                        Jump(&exit);
                        Bind(&shouldNotCall);
                        Jump(&afterIsAccessor);
                    }
                }
                Bind(&notAccessor);
                Jump(&afterIsAccessor);
                Bind(&afterIsAccessor);
                {
                    Label writable(env);
                    Label notWritable(env);
                    Label afterIsWritable(env);
                    Branch(IsWritable(attr), &writable, &notWritable);
                    {
                        Bind(&writable);
                        Jump(&afterIsWritable);
                        Bind(&notWritable);
                        GateRef taggedId = GetInt32Constant(GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensible));
                        CallRuntime(GET_STUBDESCRIPTOR(ThrowTypeError), glue,
                            GetWord64Constant(FAST_STUB_ID(ThrowTypeError)), { glue, taggedId });
                        result = GetExceptionConstant(MachineType::UINT64);
                        Jump(&exit);
                    }
                    Bind(&afterIsWritable);
                    {
                        Label holdEqualsRecv(env);
                        Label holdNotEqualsRecv(env);
                        Label afterComp(env);
                        Branch(Word64Equal(*holder, receiver), &holdEqualsRecv, &holdNotEqualsRecv);
                        {
                            Bind(&holdEqualsRecv);
                            Jump(&afterComp);
                            Bind(&holdNotEqualsRecv);
                            Jump(&ifEnd);
                        }
                        Bind(&afterComp);
                        // JSObject::Cast(holder)->SetProperty(thread, hclass, attr, value)
                        // return JSTaggedValue::Undefined()
                        JSObjectSetProperty(glue, *holder, hClass, attr, value);
                        result = GetUndefinedConstant(MachineType::UINT64);
                        Jump(&exit);
                    }
                }
            }
            Bind(&noEntry);
            {
                Jump(&ifEnd);
            }
        }
        Bind(&isDicMode);
        {
            // TaggedArray *array = TaggedArray::Cast(JSObject::Cast(holder)->GetPropertiesArray().GetTaggedObject())
            GateRef array = GetPropertiesArray(*holder);
            // int entry = dict->FindEntry(key)
            GateRef entry1 = FindEntryFromNameDictionary(glue, array, key);
            Label notNegtiveOne(env);
            Label negtiveOne(env);
            // if branch condition : entry != -1
            Branch(Word32NotEqual(entry1, GetInt32Constant(-1)), &notNegtiveOne, &negtiveOne);
            Bind(&notNegtiveOne);
            {
                // auto attr = dict->GetAttributes(entry)
                GateRef attr1 = GetAttributesFromDictionary(array, entry1);
                Label isAccessor1(env);
                Label notAccessor1(env);
                Label afterIsAccessor1(env);
                // if branch condition : UNLIKELY(attr.IsAccessor())
                Branch(IsAccessor(attr1), &isAccessor1, &notAccessor1);
                Bind(&isAccessor1);
                {
                    Label isInternal1(env);
                    Label notInternal1(env);
                    // auto accessor = dict->GetValue(entry)
                    GateRef accessor1 = GetValueFromDictionary(MachineType::TAGGED, array, entry1);
                    Label shouldCall1(env);
                    Label shouldNotCall1(env);
                    Branch(ShouldCallSetter(receiver, *holder, accessor1, attr1), &shouldCall1, &shouldNotCall1);
                    Bind(&shouldCall1);
                    result = CallSetterUtil(glue, receiver, accessor1, value);
                    Jump(&exit);
                    Bind(&shouldNotCall1);
                    Jump(&afterIsAccessor1);
                }
                Bind(&notAccessor1);
                Jump(&afterIsAccessor1);
                Bind(&afterIsAccessor1);
                {
                    Label writable1(env);
                    Label notWritable1(env);
                    Label afterIsWritable1(env);
                    Branch(IsWritable(attr1), &writable1, &notWritable1);
                    {
                        Bind(&writable1);
                        Jump(&afterIsWritable1);
                        Bind(&notWritable1);
                        GateRef taggedId = GetInt32Constant(GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensible));
                        CallRuntime(GET_STUBDESCRIPTOR(ThrowTypeError), glue,
                            GetWord64Constant(FAST_STUB_ID(ThrowTypeError)), { glue, taggedId });
                        result = GetExceptionConstant(MachineType::UINT64);
                        Jump(&exit);
                    }
                    Bind(&afterIsWritable1);
                    {
                        Label holdEqualsRecv1(env);
                        Label holdNotEqualsRecv1(env);
                        Label afterComp1(env);
                        Branch(Word64Equal(*holder, receiver), &holdEqualsRecv1, &holdNotEqualsRecv1);
                        {
                            Bind(&holdEqualsRecv1);
                            Jump(&afterComp1);
                            Bind(&holdNotEqualsRecv1);
                            Jump(&ifEnd);
                        }
                        Bind(&afterComp1);
                        // dict->UpdateValue(thread, entry, value)
                        // return JSTaggedValue::Undefined()
                        UpdateValueInDict(glue, array, entry1, value);
                        result = GetUndefinedConstant(MachineType::UINT64);
                        Jump(&exit);
                    }
                }
            }
            Bind(&negtiveOne);
            Jump(&ifEnd);
        }
    }
    Bind(&ifEnd);
    {
        Label extensible(env);
        Label inextensible(env);
        Label afterExtenCon(env);
        Branch(IsExtensible(receiver), &extensible, &inextensible);
        {
            Bind(&extensible);
            Jump(&afterExtenCon);
            Bind(&inextensible);
            GateRef taggedId = GetInt32Constant(GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensible));
            CallRuntime(GET_STUBDESCRIPTOR(ThrowTypeError), glue, GetWord64Constant(FAST_STUB_ID(ThrowTypeError)),
                {glue, taggedId});
            result = GetExceptionConstant(MachineType::UINT64);
            Jump(&exit);
        }
        Bind(&afterExtenCon);
        result = AddPropertyByName(glue, receiver, key, value,
                                   GetInt32Constant(PropertyAttributes::GetDefaultAttributes()));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

void Stub::NotifyHClassChanged(GateRef glue, GateRef oldHClass, GateRef newHClass)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    Label isProtoType(env);
    Branch(IsProtoTypeHClass(oldHClass), &isProtoType, &exit);
    Bind(&isProtoType);
    {
        Label notEqualHClass(env);
        Branch(Word64Equal(oldHClass, newHClass), &exit, &notEqualHClass);
        Bind(&notEqualHClass);
        {
            SetIsProtoTypeToHClass(glue, newHClass, TrueConstant());
            auto stubDescriptor = GET_STUBDESCRIPTOR(NoticeThroughChainAndRefreshUser);
            CallRuntime(stubDescriptor, glue, GetWord64Constant(FAST_STUB_ID(NoticeThroughChainAndRefreshUser)), {
                    glue, oldHClass, newHClass
                });
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->PopCurrentLabel();
    return;
}

GateRef Stub::GetContainerProperty(GateRef glue, GateRef receiver, GateRef index, GateRef jsType)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    DEFVARIABLE(result, MachineType::TAGGED, GetUndefinedConstant());

    Label arrayListLabel(env);
    Label queueLabel(env);
    Label defaultLabel(env);
    std::array<Label, 2> repCaseLabels = { // 2 : 2 means that there are 2 args in total.
        arrayListLabel,
        queueLabel,
    };
    std::array<int64_t, 2> keyValues = { // 2 : 2 means that there are 2 args in total.
        static_cast<int64_t>(JSType::JS_ARRAY_LIST),
        static_cast<int64_t>(JSType::JS_QUEUE),
    };
    // 2 : 2 means that there are 2 cases.
    Switch(ZExtInt32ToInt64(jsType), &defaultLabel, keyValues.data(), repCaseLabels.data(), 2);
    Bind(&arrayListLabel);
    {
        result = JSArrayListGet(glue, receiver, index);
        Jump(&exit);
    }
    Bind(&queueLabel);
    {
        Jump(&exit);
    }
    Bind(&defaultLabel);
    {
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::JSArrayListGet(GateRef glue, GateRef receiver, GateRef index)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    DEFVARIABLE(result, MachineType::TAGGED, GetUndefinedConstant());

    GateRef lengthOffset = GetArchRelateConstant(panda::ecmascript::JSArrayList::LENGTH_OFFSET);
    GateRef length = TaggedCastToInt32(Load(MachineType::UINT64, receiver, lengthOffset));
    Label isVailedIndex(env);
    Label notValidIndex(env);
    Branch(TruncInt32ToInt1(Word32And(ZExtInt1ToInt32(Int32GreaterThanOrEqual(index, GetInt32Constant(0))),
                            ZExtInt1ToInt32(Int32LessThan(index, length)))), &isVailedIndex, &notValidIndex);
    Bind(&isVailedIndex);
    {
        GateRef elements = GetElementsArray(receiver);
        result = GetValueFromTaggedArray(MachineType::TAGGED, elements, index);
        Jump(&exit);
    }
    Bind(&notValidIndex);
    {
        GateRef taggedId = GetInt32Constant(GET_MESSAGE_STRING_ID(GetPropertyOutOfBounds));
        CallRuntime(GET_STUBDESCRIPTOR(ThrowTypeError), glue, GetWord64Constant(FAST_STUB_ID(ThrowTypeError)),
                    {glue, taggedId});
        result = GetExceptionConstant();
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}

GateRef Stub::SetContainerProperty(GateRef glue, GateRef receiver, GateRef index, GateRef value, GateRef jsType)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->PushCurrentLabel(&entry);
    Label exit(env);
    DEFVARIABLE(result, MachineType::UINT64, GetUndefinedConstant(MachineType::UINT64));
    Label arrayListLabel(env);
    Label queueLabel(env);
    Label defaultLabel(env);
    std::array<Label, 2> repCaseLabels = { // 2 : 2 means that there are 2 args in total.
        arrayListLabel,
        queueLabel,
    };
    std::array<int64_t, 2> keyValues = { // 2 : 2 means that there are 2 args in total.
        static_cast<int64_t>(JSType::JS_ARRAY_LIST),
        static_cast<int64_t>(JSType::JS_QUEUE),
    };
    // 2 : 2 means that there are 2 cases.
    Switch(ZExtInt32ToInt64(jsType), &defaultLabel, keyValues.data(), repCaseLabels.data(), 2);
    Bind(&arrayListLabel);
    {
        StubDescriptor *jsarraylistSetByIndex = GET_STUBDESCRIPTOR(JSArrayListSetByIndex);
        CallRuntime(jsarraylistSetByIndex, glue, GetWord64Constant(FAST_STUB_ID(JSArrayListSetByIndex)),
                    { glue, receiver, index, value });
        Jump(&exit);
    }
    Bind(&queueLabel);
    {
        Jump(&exit);
    }
    Bind(&defaultLabel);
    {
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->PopCurrentLabel();
    return ret;
}
}  // namespace panda::ecmascript::kungfu
