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
enum FunctionKind : uint8_t {
    // BEGIN constructable functions
    NORMAL_FUNCTION = 0,
    BUILTIN_PROXY_CONSTRUCTOR,
    BUILTIN_CONSTRUCTOR,
    // BEGIN class constructors
    // BEGIN base constructors
    BASE_CONSTRUCTOR,
    // BEGIN default constructors
    DEFAULT_BASE_CONSTRUCTOR,
    CLASS_CONSTRUCTOR,
    // END base constructors
    // BEGIN derived constructors
    DEFAULT_DERIVED_CONSTRUCTOR,
    // END default constructors
    DERIVED_CONSTRUCTOR,
    // END derived constructors
    // END class constructors
    // END constructable functions.
    GENERATOR_FUNCTION,
    // END generators

    // BEGIN accessors
    GETTER_FUNCTION,
    SETTER_FUNCTION,
    // END accessors
    // BEGIN arrow functions
    ARROW_FUNCTION,
    // BEGIN async functions
    ASYNC_ARROW_FUNCTION,
    // END arrow functions
    ASYNC_FUNCTION,
    // BEGIN concise methods 1
    ASYNC_CONCISE_METHOD,
    // BEGIN generators
    ASYNC_CONCISE_GENERATOR_METHOD,
    // END concise methods 1
    ASYNC_GENERATOR_FUNCTION,
    // END async functions
    CONSIZE_METHDOD,

    CLASS_MEMBERS_INITIALIZER_FUNCTION,
    // END concise methods 2

    LAST_FUNCTION_KIND = CLASS_MEMBERS_INITIALIZER_FUNCTION,
};

enum FunctionMode : uint8_t {
    LEXICAL,
    STRICT,
    GLOBAL,
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_FUNCTION_KIND_H
