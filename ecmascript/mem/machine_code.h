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

#ifndef PANDA_RUNTIME_ECMASCRIPT_MEM_MACHINE_CODE_H
#define PANDA_RUNTIME_ECMASCRIPT_MEM_MACHINE_CODE_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/tagged_object.h"

namespace panda {
namespace ecmascript {
class MachineCode : public TaggedObject {
public:
    NO_COPY_SEMANTIC(MachineCode);
    NO_MOVE_SEMANTIC(MachineCode);
    static MachineCode *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsMachineCodeObject());
        return static_cast<MachineCode *>(object);
    }

    static constexpr size_t INS_SIZE_OFFSET = TaggedObjectSize();
    ACCESSORS(InstructionSizeInBytes, INS_SIZE_OFFSET, DATA_OFFSET);
    static constexpr size_t SIZE = DATA_OFFSET;

    uintptr_t GetDataOffsetAddress(void)
    {
        return reinterpret_cast<uintptr_t>(this) + DATA_OFFSET;
    }

    void SetData(const uint8_t *codeData, size_t codeLength)
    {
        if (codeData == nullptr) {
            LOG_ECMA_MEM(ERROR) << "data is null in creating new code object";
            return;
        }
        if (memcpy_s(reinterpret_cast<void *>(this->GetDataOffsetAddress()),
            this->GetInstructionSizeInBytes().GetInt(), codeData, codeLength) != EOK) {
            LOG_ECMA_MEM(ERROR) << "memcpy fail in creating new code object ";
            return;
        }
    }
    void VisitRangeSlot(const EcmaObjectRangeVisitor &v)
    {
        // no need to visit
    }

    void VisitObjects([[maybe_unused]] const EcmaObjectRangeVisitor &visitor) const
    {
        // no field in this object
    }

    size_t GetMachineCodeObjectSize(void)
    {
        return SIZE + this->GetInstructionSizeInBytes().GetInt();
    }
};
}  // namespace ecmascript
}  // namespace panda

#endif  // PANDA_RUNTIME_ECMASCRIPT_MEM_MACHINE_CODE_H
