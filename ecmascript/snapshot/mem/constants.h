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

#ifndef ECMASCRIPT_SNAPSHOT_MEM_CONSTANTS_H
#define ECMASCRIPT_SNAPSHOT_MEM_CONSTANTS_H

namespace panda::ecmascript {
class Constants final {
public:
    explicit Constants() = default;
    ~Constants() = default;
    NO_COPY_SEMANTIC(Constants);
    NO_MOVE_SEMANTIC(Constants);

    static constexpr int MAX_C_POINTER_INDEX = 1024 - 1;
    static constexpr int MAX_REGION_INDEX = 1024 - 1;
    static constexpr int MAX_OBJECT_OFFSET = 1024 * 256 - 1;

    // object or space align up
    static constexpr size_t PAGE_SIZE_ALIGN_UP = 4096;
    static constexpr size_t MAX_UINT_16 = 0xFFFF;
    static constexpr size_t MAX_STRING_SIZE = 0x0FFFFFFF;

    // builtins native method encode
    // builtins deserialize: nativeMehtods_ + getter/setter; program deserialize: getter/setter + programFunctionMethods
    static constexpr size_t PROGRAM_NATIVE_METHOD_BEGIN = 7;

    // serialize use constants
    static constexpr uint8_t MASK_METHOD_SPACE_BEGIN = 0x7F;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_SNAPSHOT_MEM_CONSTANTS_H
