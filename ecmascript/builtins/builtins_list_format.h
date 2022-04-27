/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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
 
#ifndef ECMASCRIPT_BUILTINS_BUILTINS_LIST_FORMAT_H
#define ECMASCRIPT_BUILTINS_BUILTINS_LIST_FORMAT_H

#include "ecmascript/base/builtins_base.h"

namespace panda::ecmascript::builtins {
class BuiltinsListFormat : public base::BuiltinsBase {
public:
    // 13.2.1 Intl.ListFormat ( [ locales [ , options ] ] )
    static JSTaggedValue ListFormatConstructor(EcmaRuntimeCallInfo *argv);

    // 13.3.2 Intl.ListFormat.supportedLocalesOf ( locales [ , options ] )
    static JSTaggedValue SupportedLocalesOf(EcmaRuntimeCallInfo *argv);

    // 13.4.3 Intl.ListFormat.prototype.format( list )
    static JSTaggedValue Format(EcmaRuntimeCallInfo *argv);

    // 13.4.4 Intl.ListFormat.prototype.formatToParts( list )
    static JSTaggedValue FormatToParts(EcmaRuntimeCallInfo *argv);

    // 13.4.5 Intl.ListFormat.prototype.resolvedOptions()
    static JSTaggedValue ResolvedOptions(EcmaRuntimeCallInfo *argv);
};
}   // namespace panda::ecmascript::builtins
#endif  // EECMASCRIPT_BUILTINS_BUILTINS_LIST_FORMAT_H