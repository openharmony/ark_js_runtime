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

// slot bit
static constexpr int OBJECT_INDEX_BIT_NUMBER = 13;          // object index
static constexpr int OBJECT_TO_STRING_FLAG_NUMBER = 1;      // 1 : reference to string
static constexpr int OBJECT_IN_CONSTANTS_INDEX_NUMBER = 8;  // index
static constexpr int OBJECT_TYPE_BIT_NUMBER = 7;            // js_type
static constexpr int OBJECT_SIZE_BIT_NUMBER = 18;           // 4M
static constexpr int OBJECT_SPECIAL = 1;                    // special
static constexpr int IS_REFERENCE_SLOT_BIT_NUMBER = 16;     // [0x0000] is reference

static constexpr int MAX_C_POINTER_INDEX = 1024 * 8 - 1;
static constexpr int MAX_OBJECT_INDEX = 8192;
static constexpr int MAX_OBJECT_SIZE_INDEX = 1024 * 256 - 1;

// object or space align up
static constexpr size_t PAGE_SIZE_ALIGN_UP = 4096;
static constexpr size_t MAX_UINT_32 = 0xFFFFFFFF;

// builtins native method encode
// builtins deserialize: nativeMehtods_ + getter/setter; program deserialize: getter/setter + programFunctionMethods
static constexpr size_t PROGRAM_NATIVE_METHOD_BEGIN = 6;

// address slot size
static constexpr int ADDRESS_SIZE = sizeof(uintptr_t);

// serialize use constants
static constexpr uint8_t MASK_METHOD_SPACE_BEGIN = 0x7F;
static constexpr int OBJECT_SIZE_EXTEND_PAGE = 512;
static constexpr int NATIVE_METHOD_SIZE = 427;

static constexpr size_t MANAGED_OBJECT_OFFSET = 16;

#endif  // ECMASCRIPT_SNAPSHOT_MEM_CONSTANTS_H
