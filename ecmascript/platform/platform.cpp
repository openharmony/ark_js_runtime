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

#include "ecmascript/platform/platform.h"

#include "sys/sysinfo.h"

namespace panda::ecmascript {
void Platform::Initialize(int threadNum)
{
    os::memory::LockHolder lock(mutex_);
    if (isInitialized_++ <= 0) {
        runner_ = std::make_unique<Runner>(TheMostSuitableThreadNum(threadNum));
    }
}

void Platform::Destory()
{
    os::memory::LockHolder lock(mutex_);
    if (--isInitialized_ <= 0) {
        runner_->Terminate();
    }
}

int Platform::TheMostSuitableThreadNum(int threadNum) const
{
    if (threadNum > 0) {
        return std::min<int>(threadNum, MAX_PLATFORM_THREAD_NUM);
    }
    int numOfCpuCore = get_nprocs() - 1;
    return std::min<int>(numOfCpuCore, MAX_PLATFORM_THREAD_NUM);
}
}  // namespace panda::ecmascript
