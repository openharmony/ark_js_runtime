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

#ifndef ECMASCRIPT_IC_IC_COMPARE_H
#define ECMASCRIPT_IC_IC_COMPARE_H

#include "ecmascript/js_function.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
enum class CompareOpType {
    NUMBER_NUMBER,
    NUMBER_STRING,
    NUMBER_BOOLEAN,
    NUMBER_OBJ,
    STRING_STRING,
    STRING_NUMBER,
    STRING_BOOLEAN,
    STRING_OBJ,
    BOOLEAN_BOOLEAN,
    BOOLEAN_NUMBER,
    BOOLEAN_STRING,
    BOOLEAN_OBJ,
    OBJ_OBJ,
    OBJ_NUMBER,
    OBJ_STRING,
    OBJ_BOOLEAN,
    SYMBOL_SYMBOL,
    NULL_NULL,
    NULL_UNDEFINED,
    UNDEFINED_UNDEFINED,
    UNDEFINED_NULL,
    UNDEFINED_BLLEAN,
    OTHER,
};

class CompareOp {
public:

    CompareOp() = default;
    ~CompareOp() = default;

    static JSTaggedValue EqualWithIC(JSThread* thread, JSTaggedValue left,
        JSTaggedValue right, CompareOpType operationType);

    static JSTaggedValue NotEqualWithIC(JSThread *thread, JSTaggedValue left,
        JSTaggedValue right, CompareOpType operationType);

    static ComparisonResult Compare(JSThread *thread, JSTaggedValue left,
        JSTaggedValue right, CompareOpType operationType);

    static JSTaggedValue LessDynWithIC(JSThread *thread, JSTaggedValue left,
        JSTaggedValue right, CompareOpType operationType);

    static JSTaggedValue LessEqDynWithIC(JSThread *thread, JSTaggedValue left,
        JSTaggedValue right, CompareOpType operationType);

    static JSTaggedValue GreaterDynWithIC(JSThread *thread, JSTaggedValue left,
        JSTaggedValue right, CompareOpType operationType);

    static JSTaggedValue GreaterEqDynWithIC(JSThread *thread, JSTaggedValue left,
        JSTaggedValue right, CompareOpType operationType);
};
}   // namespace panda::ecmascript
#endif  // ECMASCRIPT_IC_IC_COMPAREOP_H