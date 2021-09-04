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

#ifndef PANDA_RUNTIME_ECMASCRIPT_GLOBAL_HANDLE_COLLECTION_H
#define PANDA_RUNTIME_ECMASCRIPT_GLOBAL_HANDLE_COLLECTION_H

#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
template <typename T>
class JSHandle;
class GlobalHandleCollection {
public:
    explicit GlobalHandleCollection(JSThread *thread) : thread_(thread) {}

    ~GlobalHandleCollection() = default;

    DEFAULT_MOVE_SEMANTIC(GlobalHandleCollection);
    DEFAULT_COPY_SEMANTIC(GlobalHandleCollection);

    template <typename T>
    JSHandle<T> NewHandle(JSTaggedType value)
    {
        uintptr_t addr = thread_->GetGlobalHandleStorage()->NewGlobalHandle(value);
        return JSHandle<T>(addr);
    }

    void Dispose(HandleBase handle)
    {
        thread_->GetGlobalHandleStorage()->DisposeGlobalHandle(handle.GetAddress());
    }

private:
    JSThread *thread_{nullptr};
};
}  // namespace panda::ecmascript

#endif  // PANDA_RUNTIME_ECMASCRIPT_GLOBAL_HANDLE_COLLECTION_H
