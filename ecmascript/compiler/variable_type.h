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

#ifndef ECMASCRIPT_COMPILER_MACHINE_TYPE_H
#define ECMASCRIPT_COMPILER_MACHINE_TYPE_H

#include "gate.h"

namespace panda::ecmascript::kungfu {
class VariableType {
public:
    explicit VariableType()
        : machineType_(MachineType::NOVALUE), gateType_(GateType::EMPTY)
    {
    }

    explicit VariableType(MachineType machine_type, GateType gate_type)
        : machineType_(machine_type), gateType_(gate_type)
    {
    }

    [[nodiscard]] MachineType GetMachineType() const
    {
        return machineType_;
    }

    [[nodiscard]] GateType GetGateType() const
    {
        return gateType_;
    }

    static VariableType VOID()
    {
        return VariableType(MachineType::NOVALUE, GateType::EMPTY);
    }

    static VariableType BOOL()
    {
        return VariableType(MachineType::I1, GateType::NJS_VALUE);
    }

    static VariableType INT8()
    {
        return VariableType(MachineType::I8, GateType::NJS_VALUE);
    }

    static VariableType INT16()
    {
        return VariableType(MachineType::I16, GateType::NJS_VALUE);
    }

    static VariableType INT32()
    {
        return VariableType(MachineType::I32, GateType::NJS_VALUE);
    }

    static VariableType INT64()
    {
        return VariableType(MachineType::I64, GateType::NJS_VALUE);
    }

    static VariableType FLOAT32()
    {
        return VariableType(MachineType::F32, GateType::NJS_VALUE);
    }

    static VariableType FLOAT64()
    {
        return VariableType(MachineType::F64, GateType::NJS_VALUE);
    }

    static VariableType POINTER()
    {
        return VariableType(MachineType::ARCH, GateType::NJS_VALUE);
    }

    static VariableType JS_ANY()
    {
        return VariableType(MachineType::I64, GateType::TAGGED_VALUE);
    }

    static VariableType JS_POINTER()
    {
        return VariableType(MachineType::I64, GateType::TAGGED_POINTER);
    }

    static VariableType JS_NOT_POINTER()
    {
        return VariableType(MachineType::I64, GateType::TAGGED_NPOINTER);
    }

    bool operator==(const VariableType &rhs) const
    {
        return (machineType_ == rhs.machineType_) && (gateType_ == rhs.gateType_);
    }

    bool operator!=(const VariableType &rhs) const
    {
        return (machineType_ != rhs.machineType_) || (gateType_ != rhs.gateType_);
    }

    bool operator<(const VariableType &rhs) const
    {
        return (machineType_ < rhs.machineType_)
            || (machineType_ == rhs.machineType_ && gateType_ < rhs.gateType_);
    }

    bool operator>(const VariableType &rhs) const
    {
        return (machineType_ > rhs.machineType_)
            || (machineType_ == rhs.machineType_ && gateType_ > rhs.gateType_);
    }

    bool operator<=(const VariableType &rhs) const
    {
        return (machineType_ < rhs.machineType_)
            || (machineType_ == rhs.machineType_ && gateType_ <= rhs.gateType_);
    }

    bool operator>=(const VariableType &rhs) const
    {
        return (machineType_ > rhs.machineType_)
            || (machineType_ == rhs.machineType_ && gateType_ >= rhs.gateType_);
    }

private:
    MachineType machineType_;
    GateType gateType_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_MACHINE_TYPE_H