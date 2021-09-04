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

#ifndef PANDA_ECMASCRIPT_THREAD_THREAD_SAFE_QUEUE_H
#define PANDA_ECMASCRIPT_THREAD_THREAD_SAFE_QUEUE_H

#include "os/mutex.h"
#include <queue>

namespace panda::ecmascript {
template <typename T>
class ThreadSafeQueue {
public:
    explicit ThreadSafeQueue() = default;
    ~ThreadSafeQueue() = default;

    bool empty()
    {
        os::memory::LockHolder lock(mutex_);
        return queue_.empty();
    }

    int size()
    {
        os::memory::LockHolder lock(mutex_);
        return queue_.size();
    }

    void enqueue(T &t)
    {
        os::memory::LockHolder lock(mutex_);
        queue_.push(t);
    }

    bool dequeue(T &t)
    {
        os::memory::LockHolder lock(mutex_);

        if (queue_.empty()) {
            return false;
        }
        t = std::move(queue_.front());
        queue_.pop();
        return true;
    }

private:
    NO_COPY_SEMANTIC(ThreadSafeQueue);
    NO_MOVE_SEMANTIC(ThreadSafeQueue);

    std::queue<T> queue_;
    os::memory::Mutex mutex_;
};
}  // namespace panda::ecmascript

#endif  // PANDA_ECMASCRIPT_THREAD_THREAD_SAFE_QUEUE_H
