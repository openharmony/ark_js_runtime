/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_GARBAGE_COLLECTOR_H
#define ECMASCRIPT_MEM_GARBAGE_COLLECTOR_H

#include "libpandabase/macros.h"

namespace panda {
namespace ecmascript {
class GarbageCollector {
public:
    GarbageCollector() = default;
    virtual ~GarbageCollector() = default;
    DEFAULT_COPY_SEMANTIC(GarbageCollector);
    DEFAULT_MOVE_SEMANTIC(GarbageCollector);

    virtual void RunPhases() = 0;

protected:
    virtual void Initialize() = 0;
    virtual void Mark() = 0;
    virtual void Sweep() = 0;
    virtual void Finish() = 0;
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_MEM_GARBAGE_COLLECTOR_H
