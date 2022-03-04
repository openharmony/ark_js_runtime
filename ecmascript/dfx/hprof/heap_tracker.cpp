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

#include "ecmascript/dfx/hprof/heap_tracker.h"
#include "ecmascript/dfx/hprof/heap_snapshot.h"
#include "ecmascript/mem/space.h"

namespace panda::ecmascript {
static constexpr int32_t MILLI_TO_MICRO = 1000;

void HeapTrackerSample::Run()
{
    while (!isInterrupt_) {
        snapShot_->RecordSampleTime();
        usleep(timeInterval_ * MILLI_TO_MICRO);
    }
}

void HeapTracker::AllocationEvent(uintptr_t address)
{
    if (snapShot_ != nullptr) {
        snapShot_->AddNode(address);
    }
}

void HeapTracker::MoveEvent(uintptr_t address, uintptr_t forward_address)
{
    if (snapShot_ != nullptr) {
        snapShot_->MoveNode(address, forward_address);
    }
}
}  // namespace panda::ecmascript
