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

#ifndef ECMASCRIPT_COMPILER_TYPE_INFERENCE_TYPE_INFER_H
#define ECMASCRIPT_COMPILER_TYPE_INFERENCE_TYPE_INFER_H

#include "ecmascript/compiler/bytecode_circuit_builder.h"
#include "ecmascript/compiler/circuit.h"
#include "ecmascript/compiler/gate_accessor.h"
#include "ecmascript/ts_types/ts_loader.h"

namespace panda::ecmascript::kungfu {
class TypeInfer {
public:
    TypeInfer(BytecodeCircuitBuilder *builder, Circuit *circuit, TSLoader *tsLoader, bool enableLog)
        : builder_(builder), circuit_(circuit), gateAccessor_(circuit),
          tsLoader_(tsLoader), enableLog_(enableLog) {}
    ~TypeInfer() = default;
    NO_COPY_SEMANTIC(TypeInfer);
    NO_MOVE_SEMANTIC(TypeInfer);
    void TraverseCircuit();

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

private:
    bool UpdateType(GateRef gate, const GateType type);
    bool UpdateType(GateRef gate, const GlobalTSTypeRef &typeRef);
    bool ShouldInfer(const GateRef gate) const;
    bool Infer(GateRef gate);
    bool InferPhiGate(GateRef gate);
    bool SetNumberType(GateRef gate);
    bool SetBooleanType(GateRef gate);
    bool InferLdUndefined(GateRef gate);
    bool InferLdNull(GateRef gate);
    bool InferLdaiDyn(GateRef gate);
    bool InferFLdaiDyn(GateRef gate);
    bool InferLdSymbol(GateRef gate);
    bool InferThrowDyn(GateRef gate);
    bool InferTypeOfDyn(GateRef gate);
    bool InferAdd2Dyn(GateRef gate);
    bool InferLdObjByIndex(GateRef gate);
    bool InferLdGlobalVar(GateRef gate);
    bool InferReturnUndefined(GateRef gate);
    bool InferReturnDyn(GateRef gate);
    bool InferLdObjByName(GateRef gate);
    bool InferLdNewObjDynRange(GateRef gate);
    bool SetStGlobalBcType(GateRef gate);
    bool InferLdStr(GateRef gate);
    bool InferCallFunction(GateRef gate);
    bool InferLdObjByValue(GateRef gate);
    bool InferGetNextPropName(GateRef gate);
    bool InferDefineGetterSetterByValue(GateRef gate);
    bool InferNewObjSpread(GateRef gate);
    bool InferSuperCall(GateRef gate);
    void TypeInferPrint() const;

    BytecodeCircuitBuilder *builder_ {nullptr};
    Circuit *circuit_ {nullptr};
    GateAccessor gateAccessor_;
    TSLoader *tsLoader_ {nullptr};
    bool enableLog_ {false};
    std::map<uint32_t, GateType> stringIdToGateType_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_TYPE_INFERENCE_TYPE_INFER_H
