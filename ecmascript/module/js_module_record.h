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

#ifndef ECMASCRIPT_MODULE_JS_MODULE_RECORD_H
#define ECMASCRIPT_MODULE_JS_MODULE_RECORD_H

#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/record.h"

namespace panda::ecmascript {
class ModuleRecord : public Record {
public:
    CAST_CHECK(ModuleRecord, IsModuleRecord);

    // 15.2.1.16.4 Instantiate()
    static int Instantiate(JSThread *thread, const JSHandle<JSTaggedValue> &module);
    // 15.2.1.16.5 Evaluate()
    static int Evaluate(JSThread *thread, const JSHandle<JSTaggedValue> &module);

    static JSTaggedValue GetNamespace(JSTaggedValue module);
    static void SetNamespace(JSThread *thread, JSTaggedValue module, JSTaggedValue value);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MODULE_JS_MODULE_RECORD_H
