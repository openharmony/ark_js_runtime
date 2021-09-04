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

#ifndef PANDA_RUNTIME_CALLER_STAT_H
#define PANDA_RUNTIME_CALLER_STAT_H

#include <cstdint>
#include <string>
#include <time.h>  // NOLINTNEXTLINE(modernize-deprecated-headers)

#include "ecmascript/mem/c_string.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
class EcmaRuntimeStat;
class PandaRuntimeCallerStat {
public:
    // NOLINTNEXTLINE(modernize-pass-by-value)
    explicit PandaRuntimeCallerStat(const CString &name) : name_(name) {}
    PandaRuntimeCallerStat() = default;
    virtual ~PandaRuntimeCallerStat() = default;

    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(PandaRuntimeCallerStat);
    DEFAULT_COPY_SEMANTIC(PandaRuntimeCallerStat);

    void UpdateState(uint64_t elapsed)
    {
        totalCount_++;
        totalTime_ += elapsed;
        maxTime_ = elapsed < maxTime_ ? maxTime_ : elapsed;
    }
    const char *Name() const
    {
        return name_.c_str();
    }
    uint64_t TotalCount() const
    {
        return totalCount_;
    }
    uint64_t TotalTime() const
    {
        return totalTime_;
    }
    uint64_t MaxTime() const
    {
        return maxTime_;
    }

    void Reset()
    {
        totalCount_ = 0;
        totalTime_ = 0;
        maxTime_ = 0;
    }

private:
    CString name_{};
    uint64_t totalCount_{0};
    uint64_t totalTime_{0};
    uint64_t maxTime_{0};
};

class PandaRuntimeTimer {
public:
    void Start(PandaRuntimeCallerStat *callerStat, PandaRuntimeTimer *parent);
    inline static uint64_t Now()
    {
        struct timespec timeNow = {0, 0};
        clock_gettime(CLOCK_REALTIME, &timeNow);
        return timeNow.tv_sec * NANOSECONDSINSECOND + timeNow.tv_nsec;
    }

    uint64_t Elapsed() const
    {
        return elapsed_;
    }

    inline bool IsStarted() const
    {
        return start_ != 0;
    }

    inline void SetParent(PandaRuntimeTimer *parent)
    {
        parent_ = parent;
    }

    void Snapshot();

    inline void UpdateCallerState()
    {
        callerStat_->UpdateState(elapsed_);
    }

private:
    static constexpr uint64_t NANOSECONDSINSECOND = 1000000000;
    PandaRuntimeTimer *Stop();
    void Pause(uint64_t now);
    void Resume(uint64_t now);
    PandaRuntimeCallerStat *callerStat_{nullptr};
    PandaRuntimeTimer *parent_{nullptr};
    uint64_t start_{0};
    uint64_t elapsed_{0};

    friend class EcmaRuntimeStat;
};
}  // namespace panda::ecmascript
#endif
