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

#ifndef ECMASCRIPT_COMMON_H
#define ECMASCRIPT_COMMON_H

#include "libpandabase/macros.h"

namespace panda {
namespace ecmascript {
enum BarrierMode { SKIP_BARRIER, WRITE_BARRIER, READ_BARRIER };

constexpr size_t NUM_MANDATORY_JSFUNC_ARGS = 3;

using Address = uintptr_t;

#define PUBLIC_API PANDA_PUBLIC_API

#ifdef NDEBUG
#define DUMP_API_ATTR __attribute__((unused))
#else
#ifndef PANDA_TARGET_WINDOWS
#define DUMP_API_ATTR __attribute__((visibility ("default"), used))
#else
#define DUMP_API_ATTR __attribute__((unused))
#endif
#endif

#ifdef PANDA_TARGET_32
#define STATIC_ASSERT_EQ_ARCH32(a, b) static_assert(a == b)
#else
#define STATIC_ASSERT_EQ_ARCH32(a, b)
#endif

#ifdef PANDA_TARGET_64
#define STATIC_ASSERT_EQ_ARCH64(a, b) static_assert(a == b)
#else
#define STATIC_ASSERT_EQ_ARCH64(a, b)
#endif

#define STATIC_ASSERT_EQ_ARCH(expect, valueArch32, valueArch64) \
    STATIC_ASSERT_EQ_ARCH32(expect, valueArch32)                \
    STATIC_ASSERT_EQ_ARCH64(expect, valueArch64)
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_COMMON_H
