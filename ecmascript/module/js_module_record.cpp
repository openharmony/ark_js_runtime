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

#include "ecmascript/module/js_module_record.h"
#include "ecmascript/module/js_module_source_text.h"

namespace panda::ecmascript {
int32_t ModuleRecord::Instantiate(JSThread *thread, const JSHandle<JSTaggedValue> &module)
{
    if (module->IsSourceTextModule()) {
        JSHandle<SourceTextModule> moduleRecord = JSHandle<SourceTextModule>::Cast(module);
        return SourceTextModule::Instantiate(thread, moduleRecord);
    }
    UNREACHABLE();
}

int32_t ModuleRecord::Evaluate(JSThread *thread, const JSHandle<JSTaggedValue> &module)
{
    if (module->IsSourceTextModule()) {
        JSHandle<SourceTextModule> moduleRecord = JSHandle<SourceTextModule>::Cast(module);
        return SourceTextModule::Evaluate(thread, moduleRecord);
    }
    UNREACHABLE();
}

JSTaggedValue ModuleRecord::GetNamespace(JSTaggedValue module)
{
    if (module.IsSourceTextModule()) {
        return SourceTextModule::Cast(module.GetHeapObject())->GetNamespace();
    }
    UNREACHABLE();
}

void ModuleRecord::SetNamespace(JSThread *thread, JSTaggedValue module, JSTaggedValue value)
{
    if (module.IsSourceTextModule()) {
        SourceTextModule::Cast(module.GetHeapObject())->SetNamespace(thread, value);
    } else {
        UNREACHABLE();
    }
}
}  // namespace panda::ecmascript