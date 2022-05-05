/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_TOOLING_INTERFACE_JS_DEBUGGER_MANAGER_H
#define ECMASCRIPT_TOOLING_INTERFACE_JS_DEBUGGER_MANAGER_H

#include "libpandabase/os/library_loader.h"

#include "ecmascript/tooling/interface/notification_manager.h"

#include "ecmascript/interpreter/frame_handler.h"

namespace panda::ecmascript::tooling {
class JsDebuggerManager {
public:
    using LibraryHandle = os::library_loader::LibraryHandle;

    JsDebuggerManager() = default;
    ~JsDebuggerManager()
    {
        delete notificationManager_;
    }

    NO_COPY_SEMANTIC(JsDebuggerManager);
    NO_MOVE_SEMANTIC(JsDebuggerManager);

    void Initialize()
    {
        notificationManager_ = new NotificationManager();
    }

    NotificationManager *GetNotificationManager() const
    {
        return notificationManager_;
    }

    void SetDebugMode(bool isDebugMode)
    {
        isDebugMode_ = isDebugMode;
    }

    bool IsDebugMode() const
    {
        return isDebugMode_;
    }

    void SetDebugLibraryHandle(LibraryHandle handle)
    {
        debuggerLibraryHandle_ = std::move(handle);
    }

    const LibraryHandle &GetDebugLibraryHandle() const
    {
        return debuggerLibraryHandle_;
    }

    void SetEvalFrameHandler(const JSThread *thread)
    {
        if (thread != nullptr) {
            frameHandler_ = std::make_unique<FrameHandler>(thread);
        } else {
            frameHandler_ = nullptr;
        }
    }

    const std::unique_ptr<FrameHandler> &GetEvalFrameHandler() const
    {
        return frameHandler_;
    }

private:
    bool isDebugMode_ {false};
    LibraryHandle debuggerLibraryHandle_ {nullptr};
    NotificationManager *notificationManager_ {nullptr};

    std::unique_ptr<FrameHandler> frameHandler_;
};
}  // panda::ecmascript::tooling

#endif  // ECMASCRIPT_TOOLING_INTERFACE_JS_DEBUGGER_MANAGER_H