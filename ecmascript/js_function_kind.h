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

#ifndef ECMASCRIPT_JS_FUNCTION_KIND_H
#define ECMASCRIPT_JS_FUNCTION_KIND_H

#include <cstdint>

namespace panda::ecmascript {
enum class FunctionKind : uint8_t {
    NORMAL_FUNCTION = 0,
    // BEGIN arrow functions
    ARROW_FUNCTION,
    // BEGIN async functions
    ASYNC_ARROW_FUNCTION,
    // END arrow functions
    ASYNC_FUNCTION,
    // END async functions
    // BEGIN constructable functions
    BUILTIN_PROXY_CONSTRUCTOR,
    BUILTIN_CONSTRUCTOR,
    // BEGIN base constructors
    BASE_CONSTRUCTOR,
    // BEGIN default constructors
    DEFAULT_BASE_CONSTRUCTOR,
    // BEGIN class constructors
    CLASS_CONSTRUCTOR,
    // END base constructors
    // END default constructors
    DERIVED_CONSTRUCTOR,
    // END class constructors
    GENERATOR_FUNCTION,
    // END generators
    // END constructable functions.

    // BEGIN accessors
    GETTER_FUNCTION,
    SETTER_FUNCTION,
    // END accessors

    LAST_FUNCTION_KIND,
};

enum class FunctionMode : uint8_t {
    LEXICAL,
    STRICT,
    GLOBAL,
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_FUNCTION_KIND_H
