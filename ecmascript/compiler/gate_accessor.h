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

#ifndef ECMASCRIPT_COMPILER_GATE_ACCESSOR_H
#define ECMASCRIPT_COMPILER_GATE_ACCESSOR_H

#include "circuit.h"

namespace panda::ecmascript::kungfu {
class GateAccessor {
public:
    explicit GateAccessor(Circuit *circuit) : circuit_(circuit) {}
    ~GateAccessor() = default;

    [[nodiscard]] GateRef GetUseList(GateRef gate);
    [[nodiscard]] bool hasUseList(GateRef gate);
    [[nodiscard]] size_t GetNumIns(GateRef gate);
    [[nodiscard]] OpCode GetOpCode(GateRef gate);
    void SetOpCode(GateRef gate, OpCode::Op opcode);
    [[nodiscard]] GateId GetId(GateRef gate);
    void ModifyIn(GateRef gate, size_t idx, GateRef inGate);
    [[nodiscard]] GateRef GetValueIn(GateRef gate, size_t idx);
    [[nodiscard]] GateRef GetIn(GateRef gate, size_t idx);

private:
    Circuit *circuit_;
};
}
#endif  // ECMASCRIPT_COMPILER_GATE_ACCESSOR_H
