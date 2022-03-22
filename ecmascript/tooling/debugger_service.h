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

#ifndef ECMASCRIPT_TOOLING_DEBUGGER_SERVICE_H
#define ECMASCRIPT_TOOLING_DEBUGGER_SERVICE_H

#include <functional>
#include <string>

#include "ecmascript/common.h"

namespace panda::ecmascript {
class EcmaVM;
}  // namespace panda::ecmascript

namespace panda::tooling::ecmascript {
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* End of #ifdef __cplusplus */

PUBLIC_API void InitializeDebugger(const std::function<void(std::string)> &on_response,
    const ::panda::ecmascript::EcmaVM *vm);

PUBLIC_API void UninitializeDebugger();

PUBLIC_API void DispatchProtocolMessage(const std::string &message);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */
}  // panda::tooling::ecmascript

#endif