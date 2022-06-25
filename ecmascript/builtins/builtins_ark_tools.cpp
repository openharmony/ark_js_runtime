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

#include "ecmascript/builtins/builtins_ark_tools.h"
#include "ecmascript/base/string_helper.h"

namespace panda::ecmascript::builtins {
using StringHelper = base::StringHelper;

JSTaggedValue BuiltinsArkTools::ObjectDump(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<EcmaString> str = JSTaggedValue::ToString(thread, GetCallArg(info, 0));
    // The default log level of ace_engine and js_runtime is error
    LOG(ERROR, RUNTIME) << ": " << base::StringHelper::ToStdString(*str);

    uint32_t numArgs = info->GetArgsNumber();
    for (uint32_t i = 1; i < numArgs; i++) {
        JSHandle<JSTaggedValue> obj = GetCallArg(info, i);
        std::ostringstream oss;
        obj->Dump(oss);

        // The default log level of ace_engine and js_runtime is error
        LOG(ERROR, RUNTIME) << ": " << oss.str();
    }

    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::CompareHClass(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj1 = GetCallArg(info, 0);
    JSHandle<JSTaggedValue> obj2 = GetCallArg(info, 1);
    JSHClass* obj1Hclass = obj1->GetTaggedObject()->GetClass();
    JSHClass* obj2Hclass = obj2->GetTaggedObject()->GetClass();
    std::ostringstream oss;
    obj1Hclass->Dump(oss);
    bool res = (obj1Hclass == obj2Hclass);
    if (!res) {
        LOG(ERROR, RUNTIME) << "These two object don't share the same hclass:" << oss.str();
    }
    return JSTaggedValue(res);
}

JSTaggedValue BuiltinsArkTools::DumpHClass(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    JSHandle<JSTaggedValue> obj = GetCallArg(info, 0);
    JSHClass* objHclass = obj->GetTaggedObject()->GetClass();
    std::ostringstream oss;
    objHclass->Dump(oss);

    LOG(ERROR, RUNTIME) << "hclass:" << oss.str();
    return JSTaggedValue::Undefined();
}

JSTaggedValue BuiltinsArkTools::IsTSHClass(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> object = GetCallArg(info, 0);
    JSHClass* hclass = object->GetTaggedObject()->GetClass();
    bool isTSHClass = hclass->IsTSType();
    return GetTaggedBoolean(isTSHClass);
}

JSTaggedValue BuiltinsArkTools::GetHClass(EcmaRuntimeCallInfo *info)
{
    ASSERT(info);
    JSThread *thread = info->GetThread();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    ASSERT(info->GetArgsNumber() == 1);
    JSHandle<JSTaggedValue> object = GetCallArg(info, 0);
    JSHClass* hclass = object->GetTaggedObject()->GetClass();
    return JSTaggedValue(hclass);
}
}  // namespace panda::ecmascript::builtins
