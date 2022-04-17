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
#ifndef ECMASCRIPT_COMPILER_TEST_STUBS_SIGNATURE_H
#define ECMASCRIPT_COMPILER_TEST_STUBS_SIGNATURE_H

namespace panda::ecmascript::kungfu {
#ifdef ECMASCRIPT_ENABLE_TEST_STUB
#define TEST_STUB_SIGNATRUE_LIST(V)         \
    V(FooAOT)                               \
    V(BarAOT)                               \
    V(Foo1AOT)                              \
    V(Foo2AOT)                              \
    V(FooNativeAOT)                         \
    V(FooBoundAOT)                          \
    V(Bar1AOT)                              \
    V(FooProxyAOT)                          \
    V(FooProxy2AOT)                         \
    V(Bar2AOT)
#else
    #define TEST_STUB_SIGNATRUE_LIST(V)
#endif
}  // namespace panda::ecmascript::kungfu
#endif
