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
#ifndef ECMASCRIPT_TRAMPOLINE_H
#define ECMASCRIPT_TRAMPOLINE_H

#include "ecmascript/js_method.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
extern "C" JSTaggedType JSFunctionEntry(uintptr_t glue, uintptr_t prevFp, uint32_t expectedNumArgs,
    uint32_t actualNumArgs, const JSTaggedType argV[], uintptr_t codeAddr);
extern "C" JSTaggedType OptimizedCallOptimized(uintptr_t glue, uint32_t expectedNumArgs,
    uint32_t actualNumArgs, uintptr_t codeAddr, ...);

using JSFunctionEntryType = uint64_t (*)(uintptr_t glue, uintptr_t prevFp, uint32_t expectedNumArgs,
                                        uint32_t actualNumArgs, const JSTaggedType argV[], uintptr_t codeAddr);
}  // panda::ecmascript
#endif  // ECMASCRIPT_BRIDGE_H
