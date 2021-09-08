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

#ifndef ECMASCRIPT_THREAD_THREAD_POOL_H
#define ECMASCRIPT_THREAD_THREAD_POOL_H

#include <functional>
#include <thread>
#include <vector>
#include <queue>

#include "os/mutex.h"
#include "os/thread.h"
#include "ecmascript/thread/thread_safe_queue.h"
#include "libpandabase/utils/logger.h"

namespace panda::ecmascript {
using ThreadPoolTask = std::function<void(uint32_t threadId)>;

class ThreadPool {
public:
    explicit ThreadPool(size_t threadNum) : stop_(false), threadNum_(threadNum)
    {
        for (size_t i = 1; i < threadNum; i++) {
            workers_.emplace_back([this, i]() {
                for (;;) {
                    std::function<void(uint32_t)> task;
                    {
                        os::memory::LockHolder lock(mtx_);
                        // NOLINTNEXTLINE(readability-implicit-bool-conversion)
                        while (!stop_ && tasks_.empty()) {
                            cv_.Wait(&mtx_);
                        }

                        if (stop_ && tasks_.empty()) {
                            return;
                        }

                        tasks_.dequeue(task);
                    }
                    task(i);
                    {
                        os::memory::LockHolder lock(mtx_);
                        taskingCount_--;
                        if (taskingCount_ == 0) {
                            barrierCv_.Signal();
                        }
                    }
                }
            });
            os::thread::SetThreadName(workers_.at(i - 1).native_handle(), ("js-GcTask" + std::to_string(i)).c_str());
        }
    }

    ~ThreadPool()
    {
        {
            os::memory::LockHolder lock(mtx_);
            stop_ = true;
        }
        cv_.SignalAll();
        for (auto &worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    void Submit(const ThreadPoolTask &taskPtr)
    {
        if (stop_) {
            LOG(FATAL, RUNTIME) << "submit on stopped thread_pool";
        }
        std::function<void(uint32_t)> wrapperFunc = [taskPtr](uint32_t threadId) { taskPtr(threadId); };
        volatile auto atomicField = reinterpret_cast<volatile std::atomic<uint32_t> *>(&taskingCount_);
        uint32_t oldTaskCount;
        do {
            oldTaskCount = atomicField->load(std::memory_order_acquire);
        } while (!std::atomic_compare_exchange_strong_explicit(atomicField, &oldTaskCount, oldTaskCount + 1,
                                                               std::memory_order_release, std::memory_order_relaxed));
        tasks_.enqueue(wrapperFunc);
        cv_.Signal();
    }

    void WaitTaskFinish()
    {
        os::memory::LockHolder lock(mtx_);
        while (taskingCount_ != 0) {
            barrierCv_.Wait(&mtx_);
        }
    }

    uint32_t GetTaskCount()
    {
        return reinterpret_cast<volatile std::atomic<uint32_t> *>(&taskingCount_)->load(std::memory_order_acquire);
    }

    uint32_t GetThreadNum() const
    {
        return threadNum_;
    }

private:
    NO_COPY_SEMANTIC(ThreadPool);
    NO_MOVE_SEMANTIC(ThreadPool);

    bool stop_;
    uint32_t threadNum_{0};
    std::atomic<uint32_t> taskingCount_{0};
    std::vector<std::thread> workers_;
    ThreadSafeQueue<std::function<void(uint32_t)>> tasks_;
    os::memory::Mutex mtx_;
    os::memory::ConditionVariable cv_;
    os::memory::ConditionVariable barrierCv_;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_THREAD_THREAD_POOL_H
