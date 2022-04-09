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

#include "ecmascript/template_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/template_map.h"

namespace panda::ecmascript {
JSHandle<JSTaggedValue> TemplateString::GetTemplateObject(JSThread *thread, JSHandle<JSTaggedValue> templateLiteral)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> rawStringsTag = JSObject::GetProperty(thread, templateLiteral, 0).GetValue();
    JSHandle<JSTaggedValue> templateMapTag = env->GetTemplateMap();
    JSHandle<TemplateMap> templateMap(templateMapTag);
    int32_t element = templateMap->FindEntry(rawStringsTag.GetTaggedValue());
    if (element != -1) {
        return JSHandle<JSTaggedValue>(thread, templateMap->GetValue(element));
    }
    JSHandle<JSTaggedValue> cookedStringsTag = JSObject::GetProperty(thread, templateLiteral, 1).GetValue();
    JSHandle<JSArray> cookedStrings(cookedStringsTag);
    int32_t count = cookedStrings->GetArrayLength();
    auto countNum = JSTaggedNumber(count);
    JSHandle<JSTaggedValue> templateArr = JSArray::ArrayCreate(thread, countNum);
    JSHandle<JSTaggedValue> rawArr = JSArray::ArrayCreate(thread, countNum);
    JSHandle<JSObject> templateObj(templateArr);
    JSHandle<JSObject> rawObj(rawArr);
    for (int32_t i = 0; i < count; i++) {
        JSHandle<JSTaggedValue> cookedValue = JSObject::GetProperty(thread, cookedStringsTag, i).GetValue();
        PropertyDescriptor descCooked(thread, cookedValue, true, false, false);
        JSArray::DefineOwnProperty(thread, templateObj, i, descCooked);
        JSHandle<JSTaggedValue> rawValue = JSObject::GetProperty(thread, rawStringsTag, i).GetValue();
        PropertyDescriptor descRaw(thread, rawValue, true, false, false);
        JSArray::DefineOwnProperty(thread, rawObj, i, descRaw);
    }
    JSObject::SetIntegrityLevel(thread, rawObj, IntegrityLevel::FROZEN);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> raw(factory->NewFromASCII("raw"));
    PropertyDescriptor desc(thread, rawArr, false, false, false);
    JSArray::DefineOwnProperty(thread, templateObj, raw, desc);
    JSObject::SetIntegrityLevel(thread, templateObj, IntegrityLevel::FROZEN);
    TemplateMap::Insert(thread, templateMap, rawStringsTag, templateArr);
    env->SetTemplateMap(thread, templateMap.GetTaggedValue());
    return templateArr;
}
}  // namespace panda::ecmascript
