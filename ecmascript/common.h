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

#ifndef PANDA_RUNTIME_ECMASCRIPT_COMMON_H
#define PANDA_RUNTIME_ECMASCRIPT_COMMON_H

#include "libpandabase/macros.h"

namespace panda {
namespace ecmascript {
enum BarrierMode { SKIP_BARRIER, WRITE_BARRIER, READ_BARRIER };

constexpr size_t NUM_MANDATORY_JSFUNC_ARGS = 3;

#define PUBLIC_API PANDA_PUBLIC_API

#ifdef NDEBUG
#define DUMP_API_ATTR
#else
#define DUMP_API_ATTR __attribute__((visibility ("default"), used))
#endif
}  // namespace ecmascript
}  // namespace panda

#endif  // PANDA_RUNTIME_ECMASCRIPT_COMMON_H
