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
#include "ecmascript/ts_types/ts_loader.h"

namespace panda::ecmascript::kungfu {
class TypeInfer {
public:
    TypeInfer(BytecodeCircuitBuilder *builder, Circuit *circuit, bool enableLog)
        : builder_(builder), circuit_(circuit), gateAccessor_(circuit), enableLog_(enableLog) {}

    bool IsLogEnabled() const
    {
        return enableLog_;
    }

    void TraverseCircuit();

private:
    bool Infer(GateRef gate);
    struct BytecodeInfo GetByteCodeInfoByGate(GateRef gate) const;
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
    bool SetStGlobalBcType(GateRef gateRef);
    bool InferLdStr(GateRef gateRef);
    static bool ShouldInfer(OpCode opCode)
    {
        switch (opCode) {
            case OpCode::JS_BYTECODE:
            case OpCode::CONSTANT:
            case OpCode::RETURN:
                return true;
            default:
                return false;
        }
    }

    BytecodeCircuitBuilder *builder_;
    Circuit *circuit_;
    GateAccessor gateAccessor_;
    bool enableLog_ {false};
    std::map<uint32_t, GateType> stringIdToGateType_;
};
}
#endif