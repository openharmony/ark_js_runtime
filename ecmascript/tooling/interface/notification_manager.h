/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ECMASCRIPT_TOOLING_INTERFACE_NOTIFICATION_MANAGER_H
#define ECMASCRIPT_TOOLING_INTERFACE_NOTIFICATION_MANAGER_H

#include <string_view>

#include "ecmascript/js_thread.h"

namespace panda::ecmascript::tooling {
class RuntimeListener {
public:
    RuntimeListener() = default;
    virtual ~RuntimeListener() = default;
    DEFAULT_COPY_SEMANTIC(RuntimeListener);
    DEFAULT_MOVE_SEMANTIC(RuntimeListener);

    virtual void LoadModule(std::string_view name) = 0;

    virtual void BytecodePcChanged(JSThread *thread, JSMethod *method,
                                   uint32_t bc_offset) = 0;

    virtual void VmStart() = 0;
    virtual void VmDeath() = 0;
};

class NotificationManager {
public:
    NotificationManager() = default;
    ~NotificationManager() = default;
    NO_COPY_SEMANTIC(NotificationManager);
    NO_MOVE_SEMANTIC(NotificationManager);

    void AddListener(RuntimeListener *listener)
    {
        listener_ = listener;
    }
    void RemoveListener()
    {
        listener_ = nullptr;
    }

    void LoadModuleEvent(std::string_view name) const
    {
        if (UNLIKELY(listener_ != nullptr)) {
            listener_->LoadModule(name);
        }
    }

    void BytecodePcChangedEvent(JSThread *thread, JSMethod *method, uint32_t bcOffset) const
    {
        if (UNLIKELY(listener_ != nullptr)) {
            listener_->BytecodePcChanged(thread, method, bcOffset);
        }
    }

    void VmStartEvent() const
    {
        if (UNLIKELY(listener_ != nullptr)) {
            listener_->VmStart();
        }
    }
    void VmDeathEvent() const
    {
        if (UNLIKELY(listener_ != nullptr)) {
            listener_->VmDeath();
        }
    }

private:
    RuntimeListener *listener_ {nullptr};
};
}  // panda::ecmascript::tooling

#endif  // ECMASCRIPT_TOOLING_INTERFACE_NOTIFICATION_MANAGER_H