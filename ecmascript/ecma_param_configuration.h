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

#ifndef ECMASCRIPT_ECMA_PARAM_CONFIGURATION_H
#define ECMASCRIPT_ECMA_PARAM_CONFIGURATION_H

#include "libpandabase/mem/mem.h"

namespace panda::ecmascript {
    // MEMEORY SIZE SHOULD ROUND UP TO 256KB
    static constexpr size_t MAX_HEAP_SIZE = 256_MB;                 // Recommended range: 128-256MB
    static constexpr size_t DEFAULT_SEMI_SPACE_SIZE = 2_MB;         // Recommended range: 2-4MB
    static constexpr size_t MAX_SEMI_SPACE_SIZE = 16_MB;            // Recommended range: 2-16MB
    static constexpr size_t DEFAULT_NONMOVABLE_SPACE_SIZE = 4_MB;   // Recommended range: 4-8MB
    static constexpr size_t DEFAULT_SNAPSHOT_SPACE_SIZE = 256_KB;
    static constexpr size_t MAX_SNAPSHOT_SPACE_SIZE = 8_MB;
    static constexpr size_t DEFAULT_MACHINECODE_SPACE_SIZE = 8_MB;

    static constexpr size_t MIN_AllOC_LIMIT_GROWING_STEP = 8_MB;
    static constexpr uint32_t MAX_STACK_SIZE = 512_KB;
} // namespace panda::ecmascript

#endif // ECMASCRIPT_ECMA_PARAM_CONFIGURATION_H
