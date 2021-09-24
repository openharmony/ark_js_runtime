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

#ifndef ECMASCRIPT_PLATFORM_RUNNER_H
#define ECMASCRIPT_PLATFORM_RUNNER_H

#include <memory>
#include <thread>
#include <vector>

#include "ecmascript/platform/task_queue.h"

namespace panda::ecmascript {
class Runner {
public:
    explicit Runner(int threadNum);
    ~Runner() = default;

    NO_COPY_SEMANTIC(Runner);
    NO_MOVE_SEMANTIC(Runner);

    void PostTask(std::unique_ptr<Task> task)
    {
        taskQueue_.PostTask(std::move(task));
    }

    void Terminate();

private:
    void Run();

    std::vector<std::unique_ptr<std::thread>> threadPool_ {};
    TaskQueue taskQueue_ {};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_RUNNER_H
