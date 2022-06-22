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

#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/tooling/interface/notification_manager.h"

namespace panda::ecmascript::tooling {
class ProtocolHandler;
class JsDebuggerManager {
public:
    using LibraryHandle = os::library_loader::LibraryHandle;
    using ObjectUpdaterFunc =
        std::function<void(const InterpretedFrameHandler *, std::string_view, Local<JSValueRef>)>;

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

    void SetDebuggerHandler(ProtocolHandler *debuggerHandler)
    {
        debuggerHandler_ = debuggerHandler;
    }

    ProtocolHandler *GetDebuggerHandler() const
    {
        return debuggerHandler_;
    }

    void SetDebugLibraryHandle(LibraryHandle handle)
    {
        debuggerLibraryHandle_ = std::move(handle);
    }

    const LibraryHandle &GetDebugLibraryHandle() const
    {
        return debuggerLibraryHandle_;
    }

    void SetEvalFrameHandler(std::shared_ptr<InterpretedFrameHandler> frameHandler)
    {
        frameHandler_ = frameHandler;
    }

    const std::shared_ptr<InterpretedFrameHandler> &GetEvalFrameHandler() const
    {
        return frameHandler_;
    }

    void SetLocalScopeUpdater(ObjectUpdaterFunc *updaterFunc)
    {
        updaterFunc_ = updaterFunc;
    }

    void NotifyLocalScopeUpdated(std::string_view varName, Local<JSValueRef> value)
    {
        if (updaterFunc_ != nullptr) {
            (*updaterFunc_)(frameHandler_.get(), varName, value);
        }
    }

private:
    bool isDebugMode_ {false};
    ProtocolHandler *debuggerHandler_ {nullptr};
    LibraryHandle debuggerLibraryHandle_ {nullptr};
    NotificationManager *notificationManager_ {nullptr};
    ObjectUpdaterFunc *updaterFunc_ {nullptr};

    std::shared_ptr<InterpretedFrameHandler> frameHandler_;
};
}  // panda::ecmascript::tooling

#endif  // ECMASCRIPT_TOOLING_INTERFACE_JS_DEBUGGER_MANAGER_H