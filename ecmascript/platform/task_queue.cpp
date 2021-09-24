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

#include "ecmascript/platform/task_queue.h"

namespace panda::ecmascript {
void TaskQueue::PostTask(std::unique_ptr<Task> task)
{
    os::memory::LockHolder holder(mtx_);
    ASSERT(!terminate_);
    tasks_.push(std::move(task));
    cv_.Signal();
}

std::unique_ptr<Task> TaskQueue::PopTask()
{
    os::memory::LockHolder holder(mtx_);
    while (true) {
        if (!tasks_.empty()) {
            std::unique_ptr<Task> task = std::move(tasks_.front());
            tasks_.pop();
            return task;
        }
        if (terminate_) {
            cv_.SignalAll();
            return nullptr;
        }
        cv_.Wait(&mtx_);
    }
}

void TaskQueue::Terminate()
{
    os::memory::LockHolder holder(mtx_);
    terminate_ = true;
    cv_.SignalAll();
}
}  // namespace panda::ecmascript
