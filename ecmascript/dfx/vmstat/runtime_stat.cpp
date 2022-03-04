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

#include "ecmascript/ecma_macros.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
// NOLINTNEXTLINE(modernize-avoid-c-arrays)
EcmaRuntimeStat::EcmaRuntimeStat(const char *const runtimeCallerNames[], int count)
{
    for (int i = 0; i < count; i++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        callerStat_.emplace_back(PandaRuntimeCallerStat(CString(runtimeCallerNames[i])));
    }
}

void EcmaRuntimeStat::StartCount(PandaRuntimeTimer *timer, int callerId)
{
    if (currentTimer_ != nullptr) {
        timer->SetParent(currentTimer_);
    }
    PandaRuntimeTimer *parent = currentTimer_;
    currentTimer_ = timer;
    PandaRuntimeCallerStat *callerStat = &callerStat_[callerId];
    timer->Start(callerStat, parent);
}

void EcmaRuntimeStat::StopCount(const PandaRuntimeTimer *nowTimer)
{
    if (nowTimer != currentTimer_) {
        return;
    }
    PandaRuntimeTimer *parentTimer = currentTimer_->Stop();
    currentTimer_ = parentTimer;
}

void EcmaRuntimeStat::Print() const
{
    if (currentTimer_ != nullptr) {
        currentTimer_->Snapshot();
    }
    LOG_ECMA(ERROR) << GetAllStats();
}

void EcmaRuntimeStat::ResetAllCount()
{
    while (currentTimer_ != nullptr) {
        StopCount(currentTimer_);
    }
    for (auto &runCallerStat : callerStat_) {
        runCallerStat.Reset();
    }
}

CString EcmaRuntimeStat::GetAllStats() const
{
    CStringStream statistic;
    statistic << "panda runtime stat:" << std::endl;
    static constexpr int nameRightAdjustment = 50;
    static constexpr int numberRightAdjustment = 20;
    statistic << std::right << std::setw(nameRightAdjustment) << "InterPreter && GC && C++ Builtin Function"
              << std::setw(numberRightAdjustment) << "Time(ns)" << std::setw(numberRightAdjustment) << "Count"
              << std::setw(numberRightAdjustment) << "MaxTime(ns)"
              << std::setw(numberRightAdjustment) << "AverageTime(ns)" << std::endl;

    statistic << "==========================================================================================="
              << "=======================================" << std::endl;
    for (auto &runCallerStat : callerStat_) {
        if (runCallerStat.TotalCount() != 0) {
            statistic << std::right << std::setw(nameRightAdjustment) << runCallerStat.Name()
                      << std::setw(numberRightAdjustment) << runCallerStat.TotalTime()
                      << std::setw(numberRightAdjustment) << runCallerStat.TotalCount()
                      << std::setw(numberRightAdjustment) << runCallerStat.MaxTime()
                      << std::setw(numberRightAdjustment) << runCallerStat.TotalTime() / runCallerStat.TotalCount()
                      << std::endl;
        }
    }
    return statistic.str();
}
}  // namespace panda::ecmascript
