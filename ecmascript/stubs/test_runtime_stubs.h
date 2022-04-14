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

#ifndef ECMASCRIPT_TEST_RUNTIME_STUBS_H
#define ECMASCRIPT_TEST_RUNTIME_STUBS_H
namespace panda::ecmascript {
#ifdef ECMASCRIPT_ENABLE_TEST_STUB
    #define TEST_RUNTIME_STUB_GC_LIST(V)         \
        V(DefineAotFunc, 3)                      \
        V(GetPrintFunc, 0)                       \
        V(GetBindFunc, 1)                        \
        V(DefineProxyFunc, 3)                    \
        V(DefineProxyFunc2, 3)                   \
        V(DefineProxyHandler, 3)                 \
        V(DumpTaggedType, 1)
#else
    #define TEST_RUNTIME_STUB_GC_LIST(V)
#endif
}  // namespace panda::ecmascript
#endif