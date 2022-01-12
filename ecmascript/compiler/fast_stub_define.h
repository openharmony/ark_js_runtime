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

#include "interpreter_stub_define.h"

namespace panda::ecmascript::kungfu {
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EXTERNAL_RUNTIMESTUB_LIST(V)        \
    V(AddElementInternal, 5)                \
    V(CallSetter, 5)                        \
    V(CallSetter2, 4)                       \
    V(CallGetter, 3)                        \
    V(CallGetter2, 4)                       \
    V(CallInternalGetter, 3)                \
    V(ThrowTypeError, 2)                    \
    V(JSProxySetProperty, 6)                \
    V(GetHash32, 2)                         \
    V(FindElementWithCache, 4)              \
    V(Execute, 5)                           \
    V(StringGetHashCode, 1)                 \
    V(FloatMod, 2)                          \
    V(GetTaggedArrayPtrTest, 1)             \
    V(NewInternalString, 2)                 \
    V(NewTaggedArray, 2)                    \
    V(CopyArray, 3)                         \
    V(NameDictPutIfAbsent, 7)               \
    V(SetValueWithBarrier, 4)               \
    V(PropertiesSetValue, 6)                \
    V(TaggedArraySetValue, 6)               \
    V(NewEcmaDynClass, 4)                   \
    V(UpdateLayOutAndAddTransition, 5)      \
    V(NoticeThroughChainAndRefreshUser, 3)  \
    V(JumpToCInterpreter, 7)                \
    V(DebugPrint, 1)                        \
    V(StGlobalRecord, 4)                    \
    V(IncDyn, 2)                            \
    V(DecDyn, 2)                            \
    V(ExpDyn, 3)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define FAST_RUNTIME_STUB_LIST(V)   \
    V(FastAdd, 2)                   \
    V(FastSub, 2)                   \
    V(FastMul, 2)                   \
    V(FastDiv, 2)                   \
    V(FastMod, 3)                   \
    V(FastEqual, 2)                 \
    V(FastTypeOf, 2)                \
    V(GetPropertyByName, 3)         \
    V(SetPropertyByName, 4)         \
    V(SetPropertyByNameWithOwn, 4)  \
    V(GetPropertyByIndex, 3)        \
    V(SetPropertyByIndex, 4)        \
    V(GetPropertyByValue, 3)        \
    V(TryLoadICByName, 4)           \
    V(TryLoadICByValue, 5)          \
    V(TryStoreICByName, 5)          \
    V(TryStoreICByValue, 6)         \
    INTERPRETER_STUB_HELPER_LIST(V) 

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TEST_FUNC_LIST(V)           \
    V(FastMulGCTest, 3)             \
    V(PhiGateTest, 1)               \
    V(LoopTest, 1)                  \
    V(LoopTest1, 1)

#define CALL_STUB_LIST(V)        \
    FAST_RUNTIME_STUB_LIST(V)    \
    EXTERNAL_RUNTIMESTUB_LIST(V) \
    V(BytecodeHandler, 7)

enum FastStubId {
#define DEF_STUB(name, counter) name##Id,
    FAST_RUNTIME_STUB_LIST(DEF_STUB) FAST_STUB_MAXCOUNT,
#undef DEF_STUB
};

enum ExternalRuntimeStubId {
#define DEF_STUB(name, counter) name##Id,
    EXTERNAL_RUNTIMESTUB_LIST(DEF_STUB) EXTERNAL_RUNTIME_STUB_MAXCOUNT,
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
    FAST_RUNTIME_STUB_LIST(DEF_STUB)
    INTERPRETER_STUB_LIST(DEF_STUB)
#ifndef NDEBUG
    TEST_FUNC_LIST(DEF_STUB)
#endif
#undef DEF_STUB
    ALL_STUB_MAXCOUNT
};

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define TEST_STUB_ID(name) panda::ecmascript::kungfu::TestFuncStubId::NAME_##name
#define FAST_STUB_ID(name) panda::ecmascript::kungfu::CallStubId::NAME_##name
#define STUB_ID(name) panda::ecmascript::kungfu::StubId::STUB_##name
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_FASTSTUB_DEFINE_H
