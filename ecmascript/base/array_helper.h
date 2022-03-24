/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BASE_ARRAY_HELPER_H
#define ECMASCRIPT_BASE_ARRAY_HELPER_H

#include <limits>
#include <string>

#include "ecmascript/base/builtins_base.h"

namespace panda::ecmascript::base {
struct FlattenArgs {
    double sourceLen = 0;
    double start = 0;
    double depth = 0;
};
class ArrayHelper {
public:
    static bool IsConcatSpreadable(JSThread *thread, const JSHandle<JSTaggedValue> &obj);
    static int32_t SortCompare(JSThread *thread, const JSHandle<JSTaggedValue> &callbackfnHandle,
                               const JSHandle<JSTaggedValue> &valueX, const JSHandle<JSTaggedValue> &valueY);
    static double GetLength(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle);
    static double GetArrayLength(JSThread *thread, const JSHandle<JSTaggedValue> &thisHandle);
    static JSTaggedValue FlattenIntoArray(JSThread *thread, const JSHandle<JSObject> &newArrayHandle,
                                          const JSHandle<JSTaggedValue> &thisObjVal, const FlattenArgs &args,
                                          const JSHandle<JSTaggedValue> &mapperFunctionHandle,
                                          const JSHandle<JSTaggedValue> &thisArg);
};
}  // namespace panda::ecmascript::base

#endif  // ECMASCRIPT_BASE_ARRAY_HELPER_H
