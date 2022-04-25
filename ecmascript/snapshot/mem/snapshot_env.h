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

#ifndef ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_ENV_H
#define ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_ENV_H

#include "ecmascript/mem/object_xray.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
class EcmaVM;

class SnapShotEnv final {
public:
    explicit SnapShotEnv(EcmaVM *vm) : vm_(vm) {}
    ~SnapShotEnv() = default;

    void Initialize();
    void Iterate(const RootVisitor &v);
    
    void ClearEnvMap()
    {
        envMap_.clear();
    }

    size_t GetEnvObjectIndex(uintptr_t objectAddr) const
    {
        if (envMap_.find(objectAddr) == envMap_.end()) {
            return MAX_UINT_32;
        }
        return envMap_.find(objectAddr)->second;
    }

    static constexpr size_t MAX_UINT_32 = 0xFFFFFFFF;
    
private:
    NO_MOVE_SEMANTIC(SnapShotEnv);
    NO_COPY_SEMANTIC(SnapShotEnv);

    EcmaVM *vm_;
    std::unordered_map<uintptr_t, size_t> envMap_;  // Cache global object which can reuse when serialize
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_ENV_H