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

#ifndef ECMASCRIPT_CLASS_LINKER_PROGRAM_H
#define ECMASCRIPT_CLASS_LINKER_PROGRAM_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda {
namespace ecmascript {
class JSThread;
class RegionFactory;

class Program : public ECMAObject {
public:
    DECL_CAST(Program)

    static constexpr size_t LOCATION_OFFSET = ECMAObject::SIZE;
    ACCESSORS(Location, LOCATION_OFFSET, CONSTANT_POOL_OFFSET)
    ACCESSORS(ConstantPool, CONSTANT_POOL_OFFSET, MAIN_FUNCTION_OFFSET)
    ACCESSORS(MainFunction, MAIN_FUNCTION_OFFSET, METHODS_DATA_OFFSET)
    SET_GET_NATIVE_FIELD(MethodsData, JSMethod, METHODS_DATA_OFFSET, NUMBER_METHODS_OFFSET)
    SET_GET_PRIMITIVE_FIELD(NumberMethods, uint32_t, NUMBER_METHODS_OFFSET, SIZE);

    inline void FreeMethodData(RegionFactory *factory);

    DECL_DUMP()
    DECL_VISIT_OBJECT(LOCATION_OFFSET, METHODS_DATA_OFFSET)
};

class LexicalFunction : public ECMAObject {
public:
    DECL_CAST(LexicalFunction)

    uint32_t GetNumVregs() const
    {
        return static_cast<uint32_t>(GetNumberVRegs().GetInt());
    }

    uint32_t GetNumICSlots() const
    {
        return static_cast<uint32_t>(GetNumberICSlots().GetInt());
    }

    inline const uint8_t *GetInstructions() const;

    static constexpr size_t NAME_OFFSET = ECMAObject::SIZE;
    ACCESSORS(Name, NAME_OFFSET, NUMBER_VREGS_OFFSET)
    ACCESSORS(NumberVRegs, NUMBER_VREGS_OFFSET, NUMBER_IC_SLOTS_OFFSET)
    ACCESSORS(NumberICSlots, NUMBER_IC_SLOTS_OFFSET, BYTECODE_OFFSET)
    ACCESSORS(Bytecode, BYTECODE_OFFSET, PROGRAM_OFFSET)
    ACCESSORS(Program, PROGRAM_OFFSET, SIZE)

    DECL_VISIT_OBJECT(NAME_OFFSET, SIZE)
};

class ConstantPool : public TaggedArray {
public:
    static ConstantPool *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsTaggedArray());
        return static_cast<ConstantPool *>(object);
    }

    inline JSTaggedValue GetObjectFromCache(uint32_t index) const;
    DECL_DUMP()
};
}  // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_CLASS_LINKER_PROGRAM_H
