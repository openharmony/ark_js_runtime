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

#ifndef ECMASCRIPT_MEM_CLOCK_SCOPE_H
#define ECMASCRIPT_MEM_CLOCK_SCOPE_H

#include "time.h"
#include "chrono"

namespace panda::ecmascript {
class ClockScope {
using Clock = std::chrono::high_resolution_clock;
using Duration = std::chrono::duration<uint64_t, std::nano>;

public:
    explicit ClockScope()
    {
        start_ = Clock::now();
    }

    Duration GetPauseTime() const
    {
        return Clock::now() - start_;
    }

    float TotalSpentTime() const
    {
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - start_);
        return (float) duration.count() / MILLION_TIME;
    }

private:
    NO_COPY_SEMANTIC(ClockScope);
    NO_MOVE_SEMANTIC(ClockScope);

    Clock::time_point start_;
    static constexpr uint32_t MILLION_TIME = 1000;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_CLOCK_SCOPE_H
