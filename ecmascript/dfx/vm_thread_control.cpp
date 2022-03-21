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

#include "ecmascript/dfx/vm_thread_control.h"

namespace panda::ecmascript {
bool VmThreadControl::NotifyVMThreadSuspension() // block caller thread
{
    if (VMNeedSuspension()) { // only enable one thread to post suspension
        return false;
    }
    SetVMNeedSuspension(true);
    os::memory::LockHolder lock(vmThreadSuspensionMutex_);
    while (!IsSuspended()) {
        vmThreadNeedSuspensionCV_.Wait(&vmThreadSuspensionMutex_);
    }
    return true;
}

void VmThreadControl::SuspendVM() // block vm thread
{
    os::memory::LockHolder lock(vmThreadSuspensionMutex_);
    SetVMSuspended(true);
    vmThreadNeedSuspensionCV_.Signal(); // wake up the thread who needs suspend vmthread
    while (VMNeedSuspension()) {
        vmThreadHasSuspendedCV_.Wait(&vmThreadSuspensionMutex_);
    }
    SetVMSuspended(false);
}

void VmThreadControl::ResumeVM()
{
    os::memory::LockHolder lock(vmThreadSuspensionMutex_);
    SetVMNeedSuspension(false);
    vmThreadHasSuspendedCV_.Signal();
}
}  // namespace panda::ecmascript