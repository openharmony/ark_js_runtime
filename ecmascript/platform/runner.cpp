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

#include "ecmascript/platform/runner.h"

#include "os/thread.h"

namespace panda::ecmascript {
Runner::Runner(int threadNum)
{
    for (int i = 0; i < threadNum; i++) {
        std::unique_ptr<std::thread> thread = std::make_unique<std::thread>(&Runner::Run, this);
        os::thread::SetThreadName(thread->native_handle(), "GC_WorkerThread");
        threadPool_.emplace_back(std::move(thread));
    }
}

void Runner::Terminate()
{
    taskQueue_.Terminate();
    int threadNum = threadPool_.size();
    for (int i = 0; i < threadNum; i++) {
        threadPool_.at(i)->join();
    }
    threadPool_.clear();
}

void Runner::Run()
{
    while (std::unique_ptr<Task> task = taskQueue_.PopTask()) {
        task->Run();
    }
}
}  // namespace panda::ecmascript
