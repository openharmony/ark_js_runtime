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

#ifndef ECMASCRIPT_VM_THREAD_CONTROL_H
#define ECMASCRIPT_VM_THREAD_CONTROL_H

#include "libpandabase/utils/bit_field.h"
#include "os/mutex.h"

namespace panda::ecmascript {
class VmThreadControl {
public:
    using VMNeedSuspensionBit = BitField<bool, 0, 1>;
    using VMHasSuspendedBit = VMNeedSuspensionBit::NextFlag;

    void SetVMNeedSuspension(bool flag)
    {
        uint64_t newVal = VMNeedSuspensionBit::Update(threadStateBitField_, flag);
        threadStateBitField_ = newVal;
    }

    bool VMNeedSuspension()
    {
        return VMNeedSuspensionBit::Decode(threadStateBitField_);
    }

    bool CheckSafepoint();

    void SuspendVM();

    void ResumeVM();

    bool NotifyVMThreadSuspension();

    void SetVMSuspended(bool flag)
    {
        uint64_t newVal = VMHasSuspendedBit::Update(threadStateBitField_, flag);
        threadStateBitField_ = newVal;
    }

    bool IsSuspended()
    {
        return VMHasSuspendedBit::Decode(threadStateBitField_);
    }

private:
    std::atomic<uint8_t> threadStateBitField_ {0};
    os::memory::Mutex vmThreadSuspensionMutex_;
    os::memory::ConditionVariable vmThreadNeedSuspensionCV_;
    os::memory::ConditionVariable vmThreadHasSuspendedCV_;
};
} // namespace panda::ecmascript
#endif  // ECMASCRIPT_VM_THREAD_CONTROL_H