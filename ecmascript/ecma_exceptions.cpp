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

#include "ecmascript/ecma_exceptions.h"
#include "ecmascript/base/error_helper.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
void SetException(JSThread *thread, JSObject *error)
{
    if (!thread->HasPendingException()) {
        thread->SetException(JSTaggedValue(error));
    }
}

void ThrowException(JSThread *thread, const char *name, const char *msg)
{
    auto jsThread = static_cast<JSThread *>(thread);
    ObjectFactory *factory = jsThread->GetEcmaVM()->GetFactory();
    if (std::strcmp(name, REFERENCE_ERROR_STRING) == 0) {
        SetException(jsThread, *factory->GetJSError(base::ErrorType::REFERENCE_ERROR, msg));
        return;
    }

    if (std::strcmp(name, TYPE_ERROR_STRING) == 0) {
        SetException(jsThread, *factory->GetJSError(base::ErrorType::TYPE_ERROR, msg));
        return;
    }
}
}  // namespace panda::ecmascript
