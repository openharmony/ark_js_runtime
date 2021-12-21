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

#ifndef ECMASCRIPT_IC_IC_BINARY_OP_H_
#define ECMASCRIPT_IC_IC_BINARY_OP_H_

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/property_attributes.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
enum class BinaryType : uint8_t {
    NUMBER,
    NUMBER_GEN,
    STRING,
    STRING_GEN,
    GENERIC,
};

class ICBinaryOP {
public:
    static inline JSTaggedValue AddWithTSType(JSThread *thread, EcmaVM *ecma_vm, JSTaggedValue left,
                                             JSTaggedValue right, JSTaggedValue argType);
    static inline JSTaggedValue SubWithTSType(JSThread *thread, EcmaVM *ecma_vm, JSTaggedValue left,
                                             JSTaggedValue right, JSTaggedValue argType);
    static inline JSTaggedValue MulWithTSType(JSThread *thread, EcmaVM *ecma_vm, JSTaggedValue left,
                                             JSTaggedValue right, JSTaggedValue argType);
    static inline JSTaggedValue DivWithTSType(JSThread *thread, EcmaVM *ecma_vm, JSTaggedValue left,
                                             JSTaggedValue right, JSTaggedValue argType);
    static inline JSTaggedValue ModWithTSType(JSThread *thread, EcmaVM *ecma_vm, JSTaggedValue left,
                                              JSTaggedValue right, JSTaggedValue argType);
    static inline void GetBitOPDate(JSThread *thread, JSTaggedValue left, JSTaggedValue right,
                                    int32_t &opNumber0, int32_t &opNumber1, BinaryType opType);
    static inline JSTaggedValue ShlWithTSType(JSThread *thread, EcmaVM *ecma_vm, JSTaggedValue left,
                                              JSTaggedValue right, JSTaggedValue argType);
    static inline JSTaggedValue ShrWithTSType(JSThread *thread, EcmaVM *ecma_vm, JSTaggedValue left,
                                              JSTaggedValue right, JSTaggedValue argType);
    static inline JSTaggedValue AshrWithTSType(JSThread *thread, EcmaVM *ecma_vm, JSTaggedValue left,
                                               JSTaggedValue right, JSTaggedValue argType);
    static inline JSTaggedValue AndWithTSType(JSThread *thread, EcmaVM *ecma_vm, JSTaggedValue left,
                                              JSTaggedValue right, JSTaggedValue argType);
    static inline JSTaggedValue OrWithTSType(JSThread *thread, EcmaVM *ecma_vm, JSTaggedValue left,
                                             JSTaggedValue right, JSTaggedValue argType);
    static inline JSTaggedValue XorWithTSType(JSThread *thread, EcmaVM *ecma_vm, JSTaggedValue left,
                                              JSTaggedValue right, JSTaggedValue argType);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_IC_IC_BINARY_OP_H_