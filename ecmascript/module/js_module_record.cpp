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

#include "js_module_record.h"
#include "ecmascript/module/js_module_source_text.h"

namespace panda::ecmascript {
int32_t ModuleRecord::Instantiate(JSThread *thread, JSHandle<JSTaggedValue> module)
{
    ASSERT(module->IsHeapObject());
    JSHClass *klass = module->GetTaggedObject()->GetClass();
    JSType type = klass->GetObjectType();
    switch (type) {
        case JSType::SOURCE_TEXT_MODULE_RECORD:
            return JSHandle<SourceTextModule>::Cast(module)->Instantiate(thread);
            break;
        default:
            UNREACHABLE();
    }
}

int32_t ModuleRecord::Evaluate(JSThread *thread, JSHandle<JSTaggedValue> module)
{
    ASSERT(module->IsHeapObject());
    JSHClass *klass = module->GetTaggedObject()->GetClass();
    JSType type = klass->GetObjectType();
    switch (type) {
        case JSType::SOURCE_TEXT_MODULE_RECORD:
            return JSHandle<SourceTextModule>::Cast(module)->Evaluate(thread);
            break;
        default:
            UNREACHABLE();
    }
}

JSTaggedValue ModuleRecord::GetNamespace(JSTaggedValue module)
{
    ASSERT(module.IsHeapObject());
    JSHClass *klass = module.GetTaggedObject()->GetClass();
    JSType type = klass->GetObjectType();
    switch (type) {
        case JSType::SOURCE_TEXT_MODULE_RECORD:
            return SourceTextModule::Cast(module.GetHeapObject())->GetNamespace();
            break;
        default:
            UNREACHABLE();
    }
}

void ModuleRecord::SetNamespace(JSThread *thread, JSTaggedValue module, JSTaggedValue value)
{
    ASSERT(module.IsHeapObject());
    JSHClass *klass = module.GetTaggedObject()->GetClass();
    JSType type = klass->GetObjectType();
    switch (type) {
        case JSType::SOURCE_TEXT_MODULE_RECORD:
            SourceTextModule::Cast(module.GetHeapObject())->SetNamespace(thread, value);
            break;
        default:
            UNREACHABLE();
    }
}
}  // namespace panda::ecmascript