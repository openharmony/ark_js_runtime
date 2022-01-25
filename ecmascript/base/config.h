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

#define ECMASCRIPT_ENABLE_DEBUG_MODE 0
#define ECMASCRIPT_ENABLE_ARK_CONTAINER 0
#define ECMASCRIPT_ENABLE_RUNTIME_STAT 1 // NOLINTNEXTLINE(cppcoreguidelines-macro-usage)

/*
 * 1. close ic
 * 2. close parallel gc
 * 3. enable gc logs
 * 4. enable handle-scope zap, zap reclaimed regions
 * 5. switch gc mode to full gc
 * 6. enable Cast() check
 * 7. enable verify heap
 * 9. enable Proactively interrogating and collecting information in the call stack
 */
#if ECMASCRIPT_ENABLE_DEBUG_MODE
    #define ECMASCRIPT_ENABLE_IC 0
    #define ECMASCRIPT_DISABLE_PARALLEL_GC 1
    #define ECMASCRIPT_ENABLE_GC_LOG 1
    #define ECMASCRIPT_ENABLE_ZAP_MEM 1
    #define ECMASCRIPT_SWITCH_GC_MODE_TO_COMPRESS_GC 1
    #define ECMASCRIPT_ENABLE_CAST_CHECK 1
    #define ECMASCRIPT_ENABLE_HEAP_VERIFY 1
    #define ECMASCRIPT_ENABLE_THREAD_CHECK 1
    #define ECMASCRIPT_ENABLE_ACTIVE_CPUPROFILER 0
#else
    #define ECMASCRIPT_ENABLE_IC 1
    #define ECMASCRIPT_DISABLE_PARALLEL_GC 0
    #define ECMASCRIPT_ENABLE_GC_LOG 0
    #define ECMASCRIPT_ENABLE_ZAP_MEM 0
    #define ECMASCRIPT_SWITCH_GC_MODE_TO_COMPRESS_GC 0
    #define ECMASCRIPT_ENABLE_CAST_CHECK 0
    #define ECMASCRIPT_ENABLE_HEAP_VERIFY 0
    #define ECMASCRIPT_ENABLE_THREAD_CHECK 0
    #define ECMASCRIPT_ENABLE_ACTIVE_CPUPROFILER 0
#endif
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_BASE_CONFIG_H
