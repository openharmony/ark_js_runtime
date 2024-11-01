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

#include "ecmascript/compiler/type_inference/type_infer.h"

namespace panda::ecmascript::kungfu {
void TypeInfer::TraverseCircuit()
{
    auto circuitRoot = Circuit::GetCircuitRoot(OpCode(OpCode::CIRCUIT_ROOT));
    size_t gateCount = circuit_->GetGateCount();
    // worklist start
    std::queue<GateRef> worklist;
    worklist.push(circuitRoot);
    std::vector<bool> visited(gateCount);
    std::function<void(GateRef)> bfs = [&](GateRef root) -> void {
        std::queue<GateRef> pendingQueue;
        pendingQueue.push(root);
        visited[gateAccessor_.GetId(root)] = true;
        while (!pendingQueue.empty()) {
            auto curGate = pendingQueue.front();
            pendingQueue.pop();
            auto uses = gateAccessor_.ConstUses(curGate);
            for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
                if (!visited[gateAccessor_.GetId(*useIt)]) {
                    visited[gateAccessor_.GetId(*useIt)] = true;
                    if (Infer(*useIt)) {
                        worklist.push(*useIt);
                    }
                    pendingQueue.push(*useIt);
                }
            }
        }
    };
    while (!worklist.empty()) {
        auto frontGate = worklist.front();
        worklist.pop();
        std::fill(visited.begin(), visited.end(), false);
        bfs(frontGate);
    }

    if (IsLogEnabled()) {
        LOG_COMPILER(INFO) << "TypeInfer:======================================================";
        circuit_->PrintAllGates(*builder_);
    }

    if (tsLoader_->GetTypeInferenceLog()) {
        TypeInferPrint();
    }
}

void TypeInfer::TypeInferPrint() const
{
    const auto &gateList = circuit_->GetAllGates();
    std::string log("TestInfer:");
    for (const auto &gate : gateList) {
        auto type = gateAccessor_.GetGateType(gate);
        if (ShouldInfer(gate) && type.IsTSType() && !type.IsAnyType()) {
            auto op = gateAccessor_.GetOpCode(gate);
            if (op == OpCode::VALUE_SELECTOR) {
                log += "&" + op.Str() + ":";
            } else {
                log += "&" + builder_->GetBytecodeStr(gate) + ":";
            }
            log += type.GetTypeStr();
        }
    }
    LOG_COMPILER(INFO) << std::dec << log;
}

bool TypeInfer::UpdateType(GateRef gate, const GateType type)
{
    auto preType = gateAccessor_.GetGateType(gate);
    if (type != preType) {
        gateAccessor_.SetGateType(gate, type);
        return true;
    }
    return false;
}

bool TypeInfer::UpdateType(GateRef gate, const GlobalTSTypeRef &typeRef)
{
    auto type = GateType(typeRef);
    return UpdateType(gate, type);
}

bool TypeInfer::ShouldInfer(const GateRef gate) const
{
    auto op = gateAccessor_.GetOpCode(gate);
    // handle phi gates
    if (op == OpCode::VALUE_SELECTOR) {
        return true;
    }
    if (op == OpCode::JS_BYTECODE ||
        op == OpCode::CONSTANT ||
        op == OpCode::RETURN) {
        auto gateToBytecode = builder_->GetGateToBytecode();
        // handle gates generated by ecma.* bytecodes (not including Jump)
        if (gateToBytecode.find(gate) != gateToBytecode.end()) {
            return !builder_->GetByteCodeInfo(gate).IsJump();
        }
    }
    return false;
}

bool TypeInfer::Infer(GateRef gate)
{
    if (!ShouldInfer(gate)) {
        return false;
    }
    if (gateAccessor_.GetOpCode(gate) == OpCode::VALUE_SELECTOR) {
        return InferPhiGate(gate);
    }
    // infer ecma.* bytecode gates
    EcmaOpcode op = builder_->GetByteCodeOpcode(gate);
    switch (op) {
        case EcmaOpcode::LDNAN_PREF:
        case EcmaOpcode::LDINFINITY_PREF:
        case EcmaOpcode::SUB2DYN_PREF_V8:
        case EcmaOpcode::MUL2DYN_PREF_V8:
        case EcmaOpcode::DIV2DYN_PREF_V8:
        case EcmaOpcode::MOD2DYN_PREF_V8:
        case EcmaOpcode::SHL2DYN_PREF_V8:
        case EcmaOpcode::ASHR2DYN_PREF_V8:
        case EcmaOpcode::SHR2DYN_PREF_V8:
        case EcmaOpcode::AND2DYN_PREF_V8:
        case EcmaOpcode::OR2DYN_PREF_V8:
        case EcmaOpcode::XOR2DYN_PREF_V8:
        case EcmaOpcode::TONUMBER_PREF_V8:
        case EcmaOpcode::NEGDYN_PREF_V8:
        case EcmaOpcode::NOTDYN_PREF_V8:
        case EcmaOpcode::INCDYN_PREF_V8:
        case EcmaOpcode::DECDYN_PREF_V8:
        case EcmaOpcode::EXPDYN_PREF_V8:
        case EcmaOpcode::STARRAYSPREAD_PREF_V8_V8:
            return SetNumberType(gate);
            break;
        case EcmaOpcode::LDTRUE_PREF:
        case EcmaOpcode::LDFALSE_PREF:
        case EcmaOpcode::EQDYN_PREF_V8:
        case EcmaOpcode::NOTEQDYN_PREF_V8:
        case EcmaOpcode::LESSDYN_PREF_V8:
        case EcmaOpcode::LESSEQDYN_PREF_V8:
        case EcmaOpcode::GREATERDYN_PREF_V8:
        case EcmaOpcode::GREATEREQDYN_PREF_V8:
        case EcmaOpcode::ISINDYN_PREF_V8:
        case EcmaOpcode::INSTANCEOFDYN_PREF_V8:
        case EcmaOpcode::STRICTNOTEQDYN_PREF_V8:
        case EcmaOpcode::STRICTEQDYN_PREF_V8:
        case EcmaOpcode::ISTRUE_PREF:
        case EcmaOpcode::ISFALSE_PREF:
        case EcmaOpcode::SETOBJECTWITHPROTO_PREF_V8_V8:
        case EcmaOpcode::DELOBJPROP_PREF_V8_V8:
            return SetBooleanType(gate);
            break;
        case EcmaOpcode::LDUNDEFINED_PREF:
            return InferLdUndefined(gate);
            break;
        case EcmaOpcode::LDNULL_PREF:
            return InferLdNull(gate);
            break;
        case EcmaOpcode::LDAI_DYN_IMM32:
            return InferLdaiDyn(gate);
            break;
        case EcmaOpcode::FLDAI_DYN_IMM64:
            return InferFLdaiDyn(gate);
            break;
        case EcmaOpcode::LDSYMBOL_PREF:
            return InferLdSymbol(gate);
            break;
        case EcmaOpcode::THROWDYN_PREF:
            return InferThrowDyn(gate);
            break;
        case EcmaOpcode::TYPEOFDYN_PREF:
            return InferTypeOfDyn(gate);
            break;
        case EcmaOpcode::ADD2DYN_PREF_V8:
            return InferAdd2Dyn(gate);
            break;
        case EcmaOpcode::LDOBJBYINDEX_PREF_V8_IMM32:
            return InferLdObjByIndex(gate);
            break;
        case EcmaOpcode::STGLOBALVAR_PREF_ID32:
        case EcmaOpcode::STCONSTTOGLOBALRECORD_PREF_ID32:
        case EcmaOpcode::TRYSTGLOBALBYNAME_PREF_ID32:
        case EcmaOpcode::STLETTOGLOBALRECORD_PREF_ID32:
        case EcmaOpcode::STCLASSTOGLOBALRECORD_PREF_ID32:
            return SetStGlobalBcType(gate);
            break;
        case EcmaOpcode::LDGLOBALVAR_PREF_ID32:
            return InferLdGlobalVar(gate);
            break;
        case EcmaOpcode::RETURNUNDEFINED_PREF:
            return InferReturnUndefined(gate);
            break;
        case EcmaOpcode::RETURN_DYN:
            return InferReturnDyn(gate);
            break;
        case EcmaOpcode::LDOBJBYNAME_PREF_ID32_V8:
            return InferLdObjByName(gate);
            break;
        case EcmaOpcode::LDA_STR_ID32:
            return InferLdStr(gate);
            break;
        case EcmaOpcode::CALLARG0DYN_PREF_V8:
        case EcmaOpcode::CALLARG1DYN_PREF_V8_V8:
        case EcmaOpcode::CALLARGS2DYN_PREF_V8_V8_V8:
        case EcmaOpcode::CALLARGS3DYN_PREF_V8_V8_V8_V8:
        case EcmaOpcode::CALLSPREADDYN_PREF_V8_V8_V8:
        case EcmaOpcode::CALLIRANGEDYN_PREF_IMM16_V8:
        case EcmaOpcode::CALLITHISRANGEDYN_PREF_IMM16_V8:
            return InferCallFunction(gate);
            break;
        case EcmaOpcode::LDOBJBYVALUE_PREF_V8_V8:
            return InferLdObjByValue(gate);
            break;
        case EcmaOpcode::GETNEXTPROPNAME_PREF_V8:
            return InferGetNextPropName(gate);
            break;
        case EcmaOpcode::DEFINEGETTERSETTERBYVALUE_PREF_V8_V8_V8_V8:
            return InferDefineGetterSetterByValue(gate);
            break;
        case EcmaOpcode::NEWOBJSPREADDYN_PREF_V8_V8:
            return InferNewObjSpread(gate);
            break;
        case EcmaOpcode::SUPERCALL_PREF_IMM16_V8:
        case EcmaOpcode::SUPERCALLSPREAD_PREF_V8:
            return InferSuperCall(gate);
            break;
        default:
            break;
    }
    return false;
}

bool TypeInfer::InferPhiGate(GateRef gate)
{
    ASSERT(gateAccessor_.GetOpCode(gate) == OpCode::VALUE_SELECTOR);
    CVector<GlobalTSTypeRef> typeList;
    auto ins = gateAccessor_.ConstIns(gate);
    for (auto it =  ins.begin(); it != ins.end(); it++) {
        // assuming that VALUE_SELECTOR is NO_DEPEND and NO_ROOT
        if (gateAccessor_.GetOpCode(*it) == OpCode::MERGE ||
            gateAccessor_.GetOpCode(*it) == OpCode::LOOP_BEGIN) {
            continue;
        }
        auto valueInType = gateAccessor_.GetGateType(*it);
        if (valueInType.IsAnyType()) {
            return UpdateType(gate, valueInType);
        }
        typeList.emplace_back(GlobalTSTypeRef(valueInType.GetType()));
    }
    // deduplicate
    auto deduplicateIndex = std::unique(typeList.begin(), typeList.end());
    typeList.erase(deduplicateIndex, typeList.end());
    if (typeList.size() > 1) {
        auto unionType = tsLoader_->GetOrCreateUnionType(typeList);
        return UpdateType(gate, unionType);
    }
    auto type = typeList.at(0);
    return UpdateType(gate, type);
}

bool TypeInfer::SetNumberType(GateRef gate)
{
    auto numberType = GateType::NumberType();
    return UpdateType(gate, numberType);
}

bool TypeInfer::SetBooleanType(GateRef gate)
{
    auto booleanType = GateType::BooleanType();
    return UpdateType(gate, booleanType);
}

bool TypeInfer::InferLdUndefined(GateRef gate)
{
    auto undefinedType = GateType::UndefinedType();
    return UpdateType(gate, undefinedType);
}

bool TypeInfer::InferLdNull(GateRef gate)
{
    auto nullType = GateType::NullType();
    return UpdateType(gate, nullType);
}

bool TypeInfer::InferLdaiDyn(GateRef gate)
{
    auto numberType = GateType::NumberType();
    return UpdateType(gate, numberType);
}

bool TypeInfer::InferFLdaiDyn(GateRef gate)
{
    auto numberType = GateType::NumberType();
    return UpdateType(gate, numberType);
}

bool TypeInfer::InferLdSymbol(GateRef gate)
{
    auto symbolType = GateType::SymbolType();
    return UpdateType(gate, symbolType);
}

bool TypeInfer::InferThrowDyn(GateRef gate)
{
    ASSERT(gateAccessor_.GetNumValueIn(gate) == 1);
    auto gateType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
    return UpdateType(gate, gateType);
}

bool TypeInfer::InferTypeOfDyn(GateRef gate)
{
    ASSERT(gateAccessor_.GetNumValueIn(gate) == 1);
    auto gateType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
    return UpdateType(gate, gateType);
}

bool TypeInfer::InferAdd2Dyn(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(gateAccessor_.GetNumValueIn(gate) == 2);
    auto firInType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
    auto secInType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 1));
    if (firInType.IsStringType() || secInType.IsStringType()) {
        return UpdateType(gate, GateType::StringType());
    }
    if (firInType.IsNumberType() && secInType.IsNumberType()) {
        return UpdateType(gate, GateType::NumberType());
    }
    return UpdateType(gate, GateType::AnyType());
}

bool TypeInfer::InferLdObjByIndex(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(gateAccessor_.GetNumValueIn(gate) == 2);
    auto inValueType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
    if (inValueType.IsArrayTypeKind()) {
        auto type = tsLoader_->GetArrayParameterTypeGT(inValueType);
        return UpdateType(gate, type);
    }
    return false;
}


bool TypeInfer::SetStGlobalBcType(GateRef gate)
{
    auto byteCodeInfo = builder_->GetByteCodeInfo(gate);
    ASSERT(byteCodeInfo.inputs.size() == 1);
    auto stringId = std::get<StringId>(byteCodeInfo.inputs[0]).GetId();
    // 2: number of value inputs
    ASSERT(gateAccessor_.GetNumValueIn(gate) == 2);
    auto inValueType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 1));
    if (stringIdToGateType_.find(stringId) != stringIdToGateType_.end()) {
        stringIdToGateType_[stringId] = inValueType;
    } else {
        stringIdToGateType_.insert(std::pair<uint32_t, GateType>(stringId, inValueType));
    }
    return UpdateType(gate, inValueType);
}

bool TypeInfer::InferLdGlobalVar(GateRef gate)
{
    auto byteCodeInfo = builder_->GetByteCodeInfo(gate);
    ASSERT(byteCodeInfo.inputs.size() == 1);
    auto stringId = std::get<StringId>(byteCodeInfo.inputs[0]).GetId();
    auto iter = stringIdToGateType_.find(stringId);
    if (iter != stringIdToGateType_.end()) {
        return UpdateType(gate, iter->second);
    }
    return false;
}

bool TypeInfer::InferReturnUndefined(GateRef gate)
{
    auto undefinedType = GateType::UndefinedType();
    return UpdateType(gate, undefinedType);
}

bool TypeInfer::InferReturnDyn(GateRef gate)
{
    ASSERT(gateAccessor_.GetNumValueIn(gate) == 1);
    auto gateType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
    return UpdateType(gate, gateType);
}

bool TypeInfer::InferLdObjByName(GateRef gate)
{
    // 2: number of value inputs
    ASSERT(gateAccessor_.GetNumValueIn(gate) == 2);
    auto objType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 1));
    // If this object has no gt type, we cannot get its internal property type
    if (objType.IsClassTypeKind() || objType.IsClassInstanceTypeKind() || objType.IsObjectTypeKind()) {
        auto index = circuit_->GetBitField(gateAccessor_.GetValueIn(gate, 0));
        auto name = tsLoader_->GetStringById(index);
        auto type = tsLoader_->GetPropType(objType, name);
        return UpdateType(gate, type);
    }
    return false;
}

bool TypeInfer::InferLdNewObjDynRange(GateRef gate)
{
    // If the instance does not allocate a local register, there is no type.
    // We assign the type of the class to it.
    if (gateAccessor_.GetGateType(gate).IsAnyType()) {
        ASSERT(gateAccessor_.GetNumValueIn(gate) > 0);
        auto objType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
        return UpdateType(gate, objType);
    }
    return false;
}

bool TypeInfer::InferLdStr(GateRef gate)
{
    auto stringType = GateType::StringType();
    return UpdateType(gate, stringType);
}

bool TypeInfer::InferCallFunction(GateRef gate)
{
    // 0 : the index of function
    auto funcType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
    if (funcType.IsFunctionTypeKind()) {
        auto returnType = tsLoader_->GetFuncReturnValueTypeGT(funcType);
        return UpdateType(gate, returnType);
    }
    return false;
}

bool TypeInfer::InferLdObjByValue(GateRef gate)
{
    auto objType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
    // handle array
    if (objType.IsArrayTypeKind()) {
        auto elementType = tsLoader_->GetArrayParameterTypeGT(objType);
        return UpdateType(gate, elementType);
    }
    // handle object
    if (objType.IsClassTypeKind() || objType.IsClassInstanceTypeKind() || objType.IsObjectTypeKind()) {
        auto valueGate = gateAccessor_.GetValueIn(gate, 1);
        if (gateAccessor_.GetOpCode(valueGate) == OpCode::CONSTANT) {
            auto value = circuit_->GetBitField(valueGate);
            auto type = tsLoader_->GetPropType(objType, value);
            return UpdateType(gate, type);
        }
    }
    return false;
}

bool TypeInfer::InferGetNextPropName(GateRef gate)
{
    auto stringType = GateType::StringType();
    return UpdateType(gate, stringType);
}

bool TypeInfer::InferDefineGetterSetterByValue(GateRef gate)
{
    // 0 : the index of obj
    auto objType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
    return UpdateType(gate, objType);
}

bool TypeInfer::InferNewObjSpread(GateRef gate)
{
    if (gateAccessor_.GetGateType(gate).IsAnyType()) {
        // 0 : the index of func
        auto funcType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gate, 0));
        return UpdateType(gate, funcType);
    }
    return false;
}

bool TypeInfer::InferSuperCall(GateRef gate)
{
    ArgumentAccessor argAcc(circuit_);
    auto newTarget = argAcc.GetCommonArgGate(CommonArgIdx::NEW_TARGET);
    auto funcType = gateAccessor_.GetGateType(newTarget);
    if (!funcType.IsUndefinedType()) {
        return UpdateType(gate, funcType);
    }
    return false;
}
}  // namespace panda::ecmascript
