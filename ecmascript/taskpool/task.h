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

#ifndef ECMASCRIPT_TASKPOOL_TASK_H
#define ECMASCRIPT_TASKPOOL_TASK_H

#include "macros.h"

namespace panda::ecmascript {
class Task {
public:
    Task() = default;
    virtual ~Task() = default;
    virtual bool Run(uint32_t threadIndex) = 0;

    NO_COPY_SEMANTIC(Task);
    NO_MOVE_SEMANTIC(Task);

    void Terminated()
    {
        terminate_ = true;
    }

    bool IsTerminate()
    {
        return terminate_;
    }

private:
    volatile bool terminate_ {false};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TASKPOOL_TASK_H
