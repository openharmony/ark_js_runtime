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

#ifndef ECMASCRIPT_COMPILER_FASTSTUB_DEFINE_H
#define ECMASCRIPT_COMPILER_FASTSTUB_DEFINE_H

#include "ecmascript/base/config.h"
#include "ecmascript/trampoline/runtime_define.h"
#include "interpreter_stub_define.h"

namespace panda::ecmascript::kungfu {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define FAST_STUB_LIST_BASE(V)          \
    V(FastAdd, 3)                       \
    V(FastSub, 3)                       \
    V(FastMul, 3)                       \
    V(FastDiv, 3)                       \
    V(FastMod, 3)                       \
    V(FastEqual, 3)                     \
    V(FastTypeOf, 2)                    \
    V(GetPropertyByName, 3)             \
    V(SetPropertyByName, 4)             \
    V(SetPropertyByNameWithOwn, 4)      \
    V(GetPropertyByIndex, 3)            \
    V(SetPropertyByIndex, 4)            \
    V(GetPropertyByValue, 3)            \
    V(SetPropertyByValue, 4)            \
    V(TryLoadICByName, 4)               \
    V(TryLoadICByValue, 5)              \
    V(TryStoreICByName, 5)              \
    V(TryStoreICByValue, 6)

#if ECMASCRIPT_COMPILE_INTERPRETER_ASM
#define FAST_STUB_LIST(V)              \
    FAST_STUB_LIST_BASE(V)             \
    INTERPRETER_STUB_HELPER_LIST(V)
#else
#define FAST_STUB_LIST(V)       \
    FAST_STUB_LIST_BASE(V)
#endif

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TEST_FUNC_LIST(V)           \
    V(FastMulGCTest, 3)             \
    V(TestAbsoluteAddressRelocation, 2)

#define CALL_STUB_LIST(V)           \
    FAST_STUB_LIST(V)               \
    NO_GC_RUNTIME_CALL_LIST(V)      \
    V(BytecodeHandler, 7)

enum FastStubId {
#define DEF_STUB(name, counter) name##Id,
    FAST_STUB_LIST(DEF_STUB) FAST_STUB_MAXCOUNT,
#undef DEF_STUB
};

#ifndef NDEBUG
enum TestFuncStubId {
#define DEF_STUB(name, counter) name##Id,
    TEST_FUNC_LIST(DEF_STUB) TEST_FUNC_MAXCOUNT,
#undef DEF_STUB
};
#endif

enum CallStubId {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEF_STUB(name, counter) NAME_##name,
    CALL_STUB_LIST(DEF_STUB)
#undef DEF_STUB
#ifndef NDEBUG
    TEST_FUNC_OFFSET,
    TEST_FUNC_BEGIN = TEST_FUNC_OFFSET - 1,
#define DEF_STUB(name, counter) NAME_##name,
    TEST_FUNC_LIST(DEF_STUB)
#undef DEF_STUB
#endif
    CALL_STUB_MAXCOUNT,
};

enum StubId {
#define DEF_STUB(name, counter) STUB_##name,
    FAST_STUB_LIST(DEF_STUB)
#if ECMASCRIPT_COMPILE_INTERPRETER_ASM
    INTERPRETER_STUB_LIST(DEF_STUB)
#endif
#undef DEF_STUB
    ALL_STUB_MAXCOUNT
};

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TEST_STUB_ID(name) panda::ecmascript::kungfu::TestFuncStubId::name##Id
#define FAST_STUB_ID(name) panda::ecmascript::kungfu::CallStubId::NAME_##name
#define STUB_ID(name) panda::ecmascript::kungfu::StubId::STUB_##name
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_FASTSTUB_DEFINE_H
