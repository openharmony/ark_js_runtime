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

#include "caller_stat.h"

namespace panda::ecmascript {
void PandaRuntimeTimer::Start(PandaRuntimeCallerStat *callerStat, PandaRuntimeTimer *parent)
{
    parent_ = parent;
    callerStat_ = callerStat;
    uint64_t nowTime = Now();
    if (parent != nullptr) {
        parent_->Pause(nowTime);
    }
    Resume(nowTime);
}

PandaRuntimeTimer *PandaRuntimeTimer::Stop()
{
    uint64_t nowTime = Now();
    Pause(nowTime);
    if (parent_ != nullptr) {
        parent_->Resume(nowTime);
    }
    UpdateCallerState();
    elapsed_ = 0;
    return parent_;
}

void PandaRuntimeTimer::Pause(uint64_t now)
{
    if (!IsStarted()) {
        return;
    }
    elapsed_ += (now - start_);
    start_ = 0;
}

void PandaRuntimeTimer::Resume(uint64_t now)
{
    if (IsStarted()) {
        return;
    }
    start_ = now;
}

void PandaRuntimeTimer::Snapshot()
{
    uint64_t nowTime = Now();
    Pause(nowTime);

    PandaRuntimeTimer *timer = this;
    while (timer != nullptr) {
        timer->UpdateCallerState();
        timer = timer->parent_;
    }
    Resume(nowTime);
}
}  // namespace panda::ecmascript