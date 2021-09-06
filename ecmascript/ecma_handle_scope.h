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

#ifndef PANDA_RUNTIME_ECMASCRIPT_HANDLE_SCOPE_H
#define PANDA_RUNTIME_ECMASCRIPT_HANDLE_SCOPE_H

#include "ecmascript/js_tagged_value.h"

namespace panda::ecmascript {
class JSThread;

/*
 * Handles are only valid within a HandleScope. When a handle is created for an object a cell is allocated in the
 * current HandleScope.
 */
class EcmaHandleScope {
public:
    inline explicit EcmaHandleScope(JSThread *thread);

    inline ~EcmaHandleScope();

    static inline uintptr_t NewHandle(JSThread *thread, JSTaggedType value);

    JSThread *GetThread() const
    {
        return thread_;
    }

private:
    JSThread *thread_;
    JSTaggedType *prevNext_;
    JSTaggedType *prevEnd_;

    NO_COPY_SEMANTIC(EcmaHandleScope);
    NO_MOVE_SEMANTIC(EcmaHandleScope);
};
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_HANDLE_SCOPE_H
