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

#ifndef ECMASCRIPT_HANDLE_SCOPE_INL_H
#define ECMASCRIPT_HANDLE_SCOPE_INL_H

#include "ecmascript/ecma_handle_scope.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {
inline EcmaHandleScope::EcmaHandleScope(JSThread *thread)
    : thread_(thread), prevNext_(thread->handleScopeStorageNext_), prevEnd_(thread->handleScopeStorageEnd_),
      prevHandleStorageIndex_(thread->currentHandleStorageIndex_)
{
    thread->HandleScopeCountAdd();
}

inline EcmaHandleScope::~EcmaHandleScope()
{
    thread_->HandleScopeCountDec();
    thread_->handleScopeStorageNext_ = prevNext_;
    if (thread_->handleScopeStorageEnd_ != prevEnd_) {
        thread_->handleScopeStorageEnd_ = prevEnd_;
        thread_->ShrinkHandleStorage(prevHandleStorageIndex_);
    }
}

uintptr_t EcmaHandleScope::NewHandle(JSThread *thread, JSTaggedType value)
{
    // Each Handle must be managed by HandleScope, otherwise it may cause Handle leakage.
    ASSERT(thread->handleScopeCount_ > 0);
    auto result = thread->handleScopeStorageNext_;
    if (result == thread->handleScopeStorageEnd_) {
        result = reinterpret_cast<JSTaggedType *>(thread->ExpandHandleStorage());
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    thread->handleScopeStorageNext_ = result + 1;
    *result = value;
    return reinterpret_cast<uintptr_t>(result);
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_HANDLE_SCOPE_INL_H
