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

#ifndef ECMASCRIPT_TASKPOOL_RUNNER_H
#define ECMASCRIPT_TASKPOOL_RUNNER_H

#include <array>
#include <memory>
#include <thread>
#include <vector>

#include "ecmascript/common.h"
#include "ecmascript/taskpool/task_queue.h"

namespace panda::ecmascript {
static constexpr uint32_t MAX_TASKPOOL_THREAD_NUM = 7;
static constexpr uint32_t DEFAULT_TASKPOOL_THREAD_NUM = 0;

class Runner {
public:
    explicit Runner(uint32_t threadNum);
    ~Runner() = default;

    NO_COPY_SEMANTIC(Runner);
    NO_MOVE_SEMANTIC(Runner);

    void PostTask(std::unique_ptr<Task> task)
    {
        taskQueue_.PostTask(std::move(task));
    }

    void PUBLIC_API TerminateThread();
    void TerminateTask();

    uint32_t GetTotalThreadNum() const
    {
        return totalThreadNum_;
    }

    bool IsInThreadPool(std::thread::id id) const
    {
        for (auto &thread : threadPool_) {
            if (thread->get_id() == id) {
                return true;
            }
        }
        return false;
    }

private:
    void Run(uint32_t threadId);

    std::vector<std::unique_ptr<std::thread>> threadPool_ {};
    TaskQueue taskQueue_ {};
    std::array<Task*, MAX_TASKPOOL_THREAD_NUM + 1> runningTask_;
    uint32_t totalThreadNum_ {0};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TASKPOOL_RUNNER_H
