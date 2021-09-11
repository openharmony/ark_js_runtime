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

#ifndef ECMASCRIPT_BASE_CONFIG_H
#define ECMASCRIPT_BASE_CONFIG_H

namespace panda::ecmascript {
#define ARK_INLINE __attribute__((always_inline))
#define ARK_NOINLINE __attribute__((noinline))

#define ECMASCRIPT_ENABLE_RUNTIME_STAT 0 // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ECMASCRIPT_ENABLE_IC 1  // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_BASE_CONFIG_H
