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

#include "ecmascript/js_invoker.h"

#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/interpreter/interpreter-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/tagged_array-inl.h"
#include "libpandabase/utils/span.h"

namespace panda::ecmascript {
JSTaggedValue JsInvoker::Invoke(JSThread *thread)
{
    UNREACHABLE();
}

JSTaggedValue InvokeJsFunction(JSThread *thread, const JSHandle<JSFunction> &func, const JSHandle<JSTaggedValue> &obj,
                               const JSHandle<JSTaggedValue> &newTgt, const JSHandle<TaggedArray> &args)
{
    ASSERT(func->GetCallTarget() != nullptr);

    CallParams params;
    params.callTarget = ECMAObject::Cast(*func);
    params.newTarget = newTgt.GetTaggedType();
    params.thisArg = obj.GetTaggedType();
    params.argc = args->GetLength();
    CVector<JSTaggedType> values;
    values.reserve(params.argc);

    for (uint32_t i = 0; i < params.argc; ++i) {
        JSTaggedValue arg = args->Get(thread, i);
        values.emplace_back(arg.GetRawData());
    }

    params.argv = values.data();
    return EcmaInterpreter::Execute(thread, params);
}
}  // namespace panda::ecmascript
