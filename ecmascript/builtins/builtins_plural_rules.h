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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_PLURAL_RULES_H
#define ECMASCRIPT_BUILTINS_BUILTINS_PLURAL_RULES_H

#include "ecmascript/base/builtins_base.h"

namespace panda::ecmascript::builtins {
class BuiltinsPluralRules : public base::BuiltinsBase {
public:
    // 15.2.1 Intl.PluralRules ( [ locales [ , options ] ] )
    static JSTaggedValue PluralRulesConstructor(EcmaRuntimeCallInfo *argv);

    // 15.3.2 Intl.PluralRules.supportedLocalesOf ( locales [, options ] )
    static JSTaggedValue SupportedLocalesOf(EcmaRuntimeCallInfo *argv);

    // 15.4.3 Intl.PluralRules.prototype.select( value )
    static JSTaggedValue Select(EcmaRuntimeCallInfo *argv);

    // 15.4.4 Intl.PluralRules.prototype.resolvedOptions ()
    static JSTaggedValue ResolvedOptions(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_PLURAL_RULES_H