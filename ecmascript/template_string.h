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

#ifndef ECMASCRIPT_TEMPLATE_STRING_H
#define ECMASCRIPT_TEMPLATE_STRING_H

#include "ecmascript/js_array.h"

namespace panda::ecmascript {
class TemplateString {
public:
    static JSHandle<JSTaggedValue> GetTemplateObject(JSThread *thread, JSHandle<JSTaggedValue> templateLiteral);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TEMPLATE_STRING_H