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

namespace kungfu {
enum MachineType {
    NONE_TYPE,
    BOOL_TYPE,
    INT8_TYPE,
    INT16_TYPE,
    INT32_TYPE,
    INT64_TYPE,
    UINT8_TYPE,
    UINT16_TYPE,
    UINT32_TYPE,
    UINT64_TYPE,
    POINTER_TYPE,
    TAGGED_TYPE,
    TAGGED_POINTER_TYPE,
    FLOAT32_TYPE,
    FLOAT64_TYPE,
};
}  // namespace kungfu
#endif  // ECMASCRIPT_COMPILER_MACHINE_TYPE_H