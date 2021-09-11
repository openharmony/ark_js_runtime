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

#include "js_method.h"

namespace panda::ecmascript {
// It's not allowed '#' token appear in ECMA function(method) name, which discriminates same names in panda methods.
CString JSMethod::ParseFunctionName() const
{
    CString methodName(utf::Mutf8AsCString(GetName().data));
    if (LIKELY(methodName[0] != '#')) {
        return methodName;
    }
    size_t index = methodName.find_last_of('#');
    return CString(methodName.substr(index + 1));
}
}