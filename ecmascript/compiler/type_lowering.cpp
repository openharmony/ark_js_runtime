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

#include "ecmascript/compiler/type_lowering.h"

namespace panda::ecmascript::kungfu {
void TypeLowering::RunTypeLowering()
{
    const auto &gateList = circuit_->GetAllGates();
    for (const auto &gate : gateList) {
        auto op = circuit_->GetOpCode(gate);
        if (op == OpCode::JS_BYTECODE) {
            Lower(gate);
        }
    }

    if (IsLogEnabled()) {
        COMPILER_LOG(INFO) << "================== type lowering print all gates ==================";
        circuit_->PrintAllGates(*bcBuilder_);
    }
}

void TypeLowering::Lower(GateRef gate)
{
    ArgumentAccessor argAcc(circuit_);
    auto glue = argAcc.GetCommonArgGate(CommonArgIdx::GLUE);

    auto pc = bcBuilder_->GetJSBytecode(gate);
    EcmaOpcode op = static_cast<EcmaOpcode>(*pc);
    // initialize label manager
    Environment env(gate, circuit_, &builder_);
    switch (op) {
        case NEWOBJDYNRANGE_PREF_IMM16_V8:
            LowerTypeNewObjDynRange(gate, glue);
            break;
        default:
            break;
    }
}

void TypeLowering::ReplaceHirToCall(GateRef hirGate, GateRef callGate, bool noThrow)
{
    GateRef stateInGate = acc_.GetState(hirGate);
    // copy depend-wire of hirGate to callGate
    GateRef dependInGate = acc_.GetDep(hirGate);
    acc_.SetDep(callGate, dependInGate);

    GateRef ifBranch;
    if (!noThrow) {
        // exception value
        GateRef exceptionVal = builder_.ExceptionConstant(GateType::TaggedNPointer());
        // compare with trampolines result
        GateRef equal = builder_.BinaryLogic(OpCode(OpCode::EQ), callGate, exceptionVal);
        ifBranch = builder_.Branch(stateInGate, equal);
    } else {
        ifBranch = builder_.Branch(stateInGate, builder_.Boolean(false));
    }

    auto uses = acc_.Uses(hirGate);
    for (auto it = uses.begin(); it != uses.end(); it++) {
        if (acc_.GetOpCode(*it) == OpCode::IF_SUCCESS) {
            acc_.SetOpCode(*it, OpCode::IF_FALSE);
            acc_.ReplaceIn(it, ifBranch);
        } else {
            if (acc_.GetOpCode(*it) == OpCode::IF_EXCEPTION) {
                acc_.SetOpCode(*it, OpCode::IF_TRUE);
                acc_.ReplaceIn(it, ifBranch);
            } else {
                acc_.ReplaceIn(it, callGate);
            }
        }
    }

    // delete old gate
    circuit_->DeleteGate(hirGate);
}

GateRef TypeLowering::LowerCallRuntime(GateRef glue, int index, const std::vector<GateRef> &args, bool useLabel)
{
    if (useLabel) {
        GateRef result = builder_.CallRuntime(glue, index, Gate::InvalidGateRef, args);
        return result;
    } else {
        const CallSignature *cs = RuntimeStubCSigns::Get(RTSTUB_ID(CallRuntime));
        GateRef target = builder_.IntPtr(index);
        GateRef result = builder_.Call(cs, glue, target, dependEntry_, args);
        return result;
    }
}

void TypeLowering::LowerTypeNewObjDynRange(GateRef gate, GateRef glue)
{
    GateRef ctor = acc_.GetValueIn(gate, 0);
    GateType ctorType = acc_.GetGateType(ctor);
    ASSERT(ctorType.IsTSType());
    if (!ctorType.IsClassTypeKind()) {
        return;
    }

    GlobalTSTypeRef gt = GlobalTSTypeRef(ctorType.GetType());
    std::map<GlobalTSTypeRef, uint32_t> gtHClassIndexMap = tsLoader_->GetGtHClassIndexMap();
    int64_t index = gtHClassIndexMap[gt];
    GateRef ihcIndex = builder_.TaggedTypeNGC(builder_.Int64(index));

    size_t range = acc_.GetNumValueIn(gate);
    std::vector<GateRef> args(range + 1);

    for (size_t i = 0; i < range; ++i) {
        args[i] = acc_.GetValueIn(gate, i);
    }
    args[range] = ihcIndex;

    const int id = RTSTUB_ID(AotNewObjWithIHClass);
    GateRef newGate = LowerCallRuntime(glue, id, args);
    ReplaceHirToCall(gate, newGate);
}
}  // namespace panda::ecmascript
