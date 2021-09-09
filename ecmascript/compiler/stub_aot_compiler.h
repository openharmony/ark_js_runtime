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

#ifndef ECMASCRIPT_COMPILER_STUB_AOT_COMPILER_H
#define ECMASCRIPT_COMPILER_STUB_AOT_COMPILER_H

#include "ecmascript/js_thread.h"
#include "ecmascript/stub_module.h"

namespace kungfu {
class Stub;
class circuit;
using Address = uintptr_t;
class StubAotCompiler {
public:
    StubAotCompiler()
    {
        for (int i = 0; i < FAST_STUB_MAXCOUNT; i++) {
            stubs_[i] = nullptr;
        }
    };
    ~StubAotCompiler() = default;
    void BuildStubModule(panda::ecmascript::StubModule *module);
    void SetStub(int index, Stub *optimizer)
    {
        stubs_[index] = optimizer;
    }

private:
    std::array<Stub *, FAST_STUB_MAXCOUNT> stubs_;
};
}  // namespace kungfu
#endif  // ECMASCRIPT_COMPILER_STUB_AOT_COMPILER_H
