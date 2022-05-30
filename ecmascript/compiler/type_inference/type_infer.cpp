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
    GateAccessor acc(circuit_);
    std::vector<bool> visited(gateCount);
    while (!worklist.empty()) {
        std::queue<GateRef> pendingQueue;
        std::fill(visited.begin(), visited.end(), false);
        auto frontGate = worklist.front();
        worklist.pop();
        pendingQueue.push(frontGate);
        visited[acc.GetId(frontGate)] = true;
        while (!pendingQueue.empty()) {
            auto curGate = pendingQueue.front();
            pendingQueue.pop();
            auto uses = acc.ConstUses(curGate);
            for (auto useIt = uses.begin(); useIt != uses.end(); useIt++) {
                if (!visited[acc.GetId(*useIt)]) {
                    visited[acc.GetId(*useIt)] = true;
                    if (Infer(*useIt)) {
                        worklist.push(*useIt);
                    }
                    pendingQueue.push(*useIt);
                }
            }
        }
    }

    if (IsLogEnabled()) {
        COMPILER_LOG(INFO) << "TypeInfer:======================================================";
        circuit_->PrintAllGates(*builder_);
    }
}

bool TypeInfer::Infer(GateRef gate)
{
    // infer constant、js bytecode gate
    if (!ShouldInfer(gateAccessor_.GetOpCode(gate))) {
        return false;
    }

    auto gateToBytecode = builder_->GetGateToBytecode();
    if (gateToBytecode.find(gate) == gateToBytecode.end()) {
        return false;
    }

    auto pc = builder_->GetJSBytecode(gate);
    EcmaOpcode op = static_cast<EcmaOpcode>(*pc);
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
            return SetNumberType(gate);
            break;
        case EcmaOpcode::LDUNDEFINED_PREF:
            return InferLdUndefined(gate);
            break;
        case EcmaOpcode::LDNULL_PREF:
            return InferLdNull(gate);
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
            return SetBooleanType(gate);
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
        default:
            break;
    }
    return false;
}

bool TypeInfer::SetNumberType(GateRef gateRef)
{
    auto numberType = GateTypeCoder::GetNumberType();
    return gateAccessor_.SetGateType(gateRef, numberType);
}

bool TypeInfer::SetBooleanType(GateRef gateRef)
{
    auto booleanType = GateTypeCoder::GetBooleanType();
    return gateAccessor_.SetGateType(gateRef, booleanType);
}

bool TypeInfer::InferLdUndefined(GateRef gateRef)
{
    auto undefinedType = GateTypeCoder::GetUndefinedType();
    return gateAccessor_.SetGateType(gateRef, undefinedType);
}

bool TypeInfer::InferLdNull(GateRef gateRef)
{
    auto nullType = GateTypeCoder::GetNullType();
    return gateAccessor_.SetGateType(gateRef, nullType);
}

bool TypeInfer::InferLdaiDyn(GateRef gateRef)
{
    auto numberType = GateTypeCoder::GetNumberType();
    return gateAccessor_.SetGateType(gateRef, numberType);
}

bool TypeInfer::InferFLdaiDyn(GateRef gateRef)
{
    auto numberType = GateTypeCoder::GetNumberType();
    return gateAccessor_.SetGateType(gateRef, numberType);
}

bool TypeInfer::InferLdSymbol(GateRef gateRef)
{
    auto symbolType = GateTypeCoder::GetSymbolType();
    return gateAccessor_.SetGateType(gateRef, symbolType);
}

bool TypeInfer::InferThrowDyn(GateRef gateRef)
{
    ASSERT(gateAccessor_.GetNumValueIn(gateRef) == 1);
    auto gateType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gateRef, 0));
    return gateAccessor_.SetGateType(gateRef, gateType);
}

bool TypeInfer::InferTypeOfDyn(GateRef gateRef)
{
    ASSERT(gateAccessor_.GetNumValueIn(gateRef) == 1);
    auto gateType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gateRef, 0));
    return gateAccessor_.SetGateType(gateRef, gateType);
}

bool TypeInfer::InferAdd2Dyn(GateRef gateRef)
{
    ASSERT(gateAccessor_.GetNumValueIn(gateRef) == 2);
    auto firInType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gateRef, 0));
    auto secInType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gateRef, 1));
    auto stringType = GateTypeCoder::GetStringType();
    auto numberType = GateTypeCoder::GetNumberType();
    if (GateTypeCoder::IsString(firInType) || GateTypeCoder::IsString(secInType)) {
        return gateAccessor_.SetGateType(gateRef, stringType);
    }
    if ((firInType == numberType) && (secInType == numberType)) {
        return gateAccessor_.SetGateType(gateRef, numberType);
    }
    auto anyType = GateTypeCoder::GetAnyType();
    return gateAccessor_.SetGateType(gateRef, anyType);
}

bool TypeInfer::InferLdObjByIndex(GateRef gateRef)
{
    auto tsLoader = builder_->GetEcmaVM()->GetTSLoader();
    ASSERT(gateAccessor_.GetNumValueIn(gateRef) == 2);
    auto inValueType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gateRef, 0));
    if (TSLoader::GetTypeKind(GlobalTSTypeRef(inValueType)) == TSTypeKind::TS_ARRAY) {
        auto type = GateTypeCoder::GetGateTypeByTypeRef(tsLoader->GetArrayParameterTypeGT(inValueType));
        return gateAccessor_.SetGateType(gateRef, type);
    }
    return false;
}

struct BytecodeInfo TypeInfer::GetByteCodeInfoByGate(GateRef gateRef) const
{
    auto pc = builder_->GetGateToBytecode().at(gateRef).second;
    auto byteCodeInfo = builder_->GetBytecodeInfo(pc);
    return byteCodeInfo;
}

bool TypeInfer::SetStGlobalBcType(GateRef gateRef)
{
    auto byteCodeInfo = GetByteCodeInfoByGate(gateRef);
    ASSERT(byteCodeInfo.inputs.size() == 1);
    auto stringId = std::get<StringId>(byteCodeInfo.inputs[0]).GetId();
    ASSERT(gateAccessor_.GetNumValueIn(gateRef) == 2);
    auto inValueType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gateRef, 1));
    if (stringIdToGateType_.find(stringId) != stringIdToGateType_.end()) {
        stringIdToGateType_[stringId] = inValueType;
    } else {
        stringIdToGateType_.insert(std::pair<uint32_t, GateType>(stringId, inValueType));
    }
    return gateAccessor_.SetGateType(gateRef, inValueType);
}

bool TypeInfer::InferLdGlobalVar(GateRef gateRef)
{
    auto byteCodeInfo = GetByteCodeInfoByGate(gateRef);
    ASSERT(byteCodeInfo.inputs.size() == 1);
    auto stringId = std::get<StringId>(byteCodeInfo.inputs[0]).GetId();
    auto iter = stringIdToGateType_.find(stringId);
    if (iter != stringIdToGateType_.end()) {
        return gateAccessor_.SetGateType(gateRef, iter->second);
    }
    return false;
}

bool TypeInfer::InferReturnUndefined(GateRef gateRef)
{
    auto undefinedType = GateTypeCoder::GetUndefinedType();
    return gateAccessor_.SetGateType(gateRef, undefinedType);
}

bool TypeInfer::InferReturnDyn(GateRef gateRef)
{
    ASSERT(gateAccessor_.GetNumValueIn(gateRef) == 1);
    auto gateType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gateRef, 0));
    return gateAccessor_.SetGateType(gateRef, gateType);
}

bool TypeInfer::InferLdObjByName(GateRef gateRef)
{
    auto tsLoader = builder_->GetEcmaVM()->GetTSLoader();
    ASSERT(gateAccessor_.GetNumValueIn(gateRef) == 2);
    auto objType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gateRef, 1));
    // If this object has no gt type, we cannot get its internal property type
    auto objTypeKind = TSLoader::GetTypeKind(GlobalTSTypeRef(objType));
    if (objTypeKind == TSTypeKind::TS_CLASS || objTypeKind == TSTypeKind::TS_CLASS_INSTANCE ||
        objTypeKind == TSTypeKind::TS_OBJECT) {
        auto index = circuit_->GetBitField(gateAccessor_.GetValueIn(gateRef, 0));
        auto name = tsLoader->GetStringById(index);
        auto type = GateTypeCoder::GetGateTypeByTypeRef(tsLoader->GetPropType(objType, name));
        return gateAccessor_.SetGateType(gateRef, type);
    }
    return false;
}

bool TypeInfer::InferLdNewObjDynRange(GateRef gateRef)
{
    // If the instance does not allocate a local register, there is no type.
    // We assign the type of the class to it.
    if (GateTypeCoder::IsAny(gateAccessor_.GetGateType(gateRef))) {
        ASSERT(gateAccessor_.GetNumValueIn(gateRef) > 0);
        auto objType = gateAccessor_.GetGateType(gateAccessor_.GetValueIn(gateRef, 0));
        return gateAccessor_.SetGateType(gateRef, objType);
    }
    return false;
}

bool TypeInfer::InferLdStr(GateRef gateRef)
{
    auto stringType = GateTypeCoder::GetStringType();
    return gateAccessor_.SetGateType(gateRef, stringType);
}
} // namespace panda::ecmascript