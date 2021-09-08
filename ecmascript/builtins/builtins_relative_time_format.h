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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_RELATIVE_TIME_FORMAT_H
#define ECMASCRIPT_BUILTINS_BUILTINS_RELATIVE_TIME_FORMAT_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_intl.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_relative_time_format.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript::builtins {
class BuiltinsRelativeTimeFormat : public base::BuiltinsBase {
public:
    // 14.2.1 Intl.RelativeTimeFormat ([ locales [ , options ]])
    static JSTaggedValue RelativeTimeFormatConstructor(EcmaRuntimeCallInfo *argv);

    // 14.3.1 Intl.RelativeTimeFormat.supportedLocalesOf ( locales [ , options ] )
    static JSTaggedValue SupportedLocalesOf(EcmaRuntimeCallInfo *argv);

    // 14.4.3 Intl.RelativeTimeFormat.prototype.format( value, unit )
    static JSTaggedValue Format(EcmaRuntimeCallInfo *argv);

    // 14.4.4 Intl.RelativeTimeFormat.prototype.formatToParts( value, unit )
    static JSTaggedValue FormatToParts(EcmaRuntimeCallInfo *argv);

    // 14.4.5 Intl.RelativeTimeFormat.prototype.resolvedOptions ()
    static JSTaggedValue ResolvedOptions(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_RELATIVE_TIME_FORMAT_H