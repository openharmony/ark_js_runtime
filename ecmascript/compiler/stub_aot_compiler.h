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

namespace panda::ecmascript::kungfu {
class Stub;
class circuit;
class StubAotCompiler {
public:
    StubAotCompiler()
    {
        for (int i = 0; i < ALL_STUB_MAXCOUNT; i++) {
            stubs_[i] = nullptr;
        }
    }
    ~StubAotCompiler()
    {
        for (int i = 0; i < ALL_STUB_MAXCOUNT; i++) {
            stubs_[i] = nullptr;
        }
    }
    void BuildStubModuleAndSave(const std::string &triple, panda::ecmascript::StubModule *module,
                                const std::string &filename);
    void SetAllStubToModule();
    void SetStub(int index, Stub *stub)
    {
        stubs_[index] = stub;
    }

    std::vector<int> GetStubIndices()
    {
        std::vector<int> result;
        uint32_t i;
        for (i = 0; i < FAST_STUB_MAXCOUNT; i++) {
            if (stubs_[i] != nullptr) {
                result.push_back(i);
            }
        }
        return result;
    }

private:
    std::array<Stub *, ALL_STUB_MAXCOUNT> stubs_;
};
}  // namespace panda::ecmascript::kungfu
#endif  // ECMASCRIPT_COMPILER_STUB_AOT_COMPILER_H
