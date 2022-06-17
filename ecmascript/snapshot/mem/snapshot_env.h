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

#include <unordered_map>

#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/visitor.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
class EcmaVM;

class SnapshotEnv final {
public:
    explicit SnapshotEnv(EcmaVM *vm) : vm_(vm), objXRay_(vm) {}
    ~SnapshotEnv() = default;

    void Initialize();

    void Iterate(const RootVisitor &v);
    
    void ClearEnvMap()
    {
        objectVector_.clear();
    }

    size_t GetEnvObjectIndex(uintptr_t objectAddr) const
    {
        auto it = std::find(objectVector_.begin(), objectVector_.end(), objectAddr);
        if (it == objectVector_.end()) {
            return MAX_UINT_32;
        } else {
            return std::distance(objectVector_.begin(), it);
        }
    }

    uintptr_t FindEnvObjectByIndex(size_t index)
    {
        ASSERT(index < objectVector_.size());
        return objectVector_.at(index);
    }

    static constexpr size_t MAX_UINT_32 = 0xFFFFFFFF;
    
private:
    NO_MOVE_SEMANTIC(SnapshotEnv);
    NO_COPY_SEMANTIC(SnapshotEnv);

    void HandleObjectField(TaggedObject *objectHeader, CQueue<TaggedObject *> *objectQueue,
                           std::set<TaggedObject *> *objectSet);
    void InitGlobalConst();
    void InitGlobalEnv();

    EcmaVM *vm_;
    ObjectXRay objXRay_;
    CVector<uintptr_t> objectVector_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_SNAPSHOT_MEM_SNAPSHOT_ENV_H