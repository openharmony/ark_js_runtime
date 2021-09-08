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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_LOCALE_H
#define ECMASCRIPT_BUILTINS_BUILTINS_LOCALE_H

#include "ecmascript/base/builtins_base.h"

namespace panda::ecmascript::builtins {
class BuiltinsLocale : public base::BuiltinsBase {
public:
    // 10.1.3 Intl.Locale( tag [, options] )
    static JSTaggedValue LocaleConstructor(EcmaRuntimeCallInfo *argv);

    // 10.3.3 Intl.Locale.prototype.maximize ()
    static JSTaggedValue Maximize(EcmaRuntimeCallInfo *argv);
    // 10.3.4 Intl.Locale.prototype.minimize ()
    static JSTaggedValue Minimize(EcmaRuntimeCallInfo *argv);
    // 10.3.5 Intl.Locale.prototype.toString ()
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);

    // 10.3.6 get Intl.Locale.prototype.baseName
    static JSTaggedValue GetBaseName(EcmaRuntimeCallInfo *argv);
    // 10.3.7 get Intl.Locale.prototype.calendar
    static JSTaggedValue GetCalendar(EcmaRuntimeCallInfo *argv);
    // 10.3.8 get Intl.Locale.prototype.caseFirst
    static JSTaggedValue GetCaseFirst(EcmaRuntimeCallInfo *argv);
    // 10.3.9 get Intl.Locale.prototype.collation
    static JSTaggedValue GetCollation(EcmaRuntimeCallInfo *argv);
    // 10.3.10 get Intl.Locale.prototype.hourCycle
    static JSTaggedValue GetHourCycle(EcmaRuntimeCallInfo *argv);
    // 10.3.11 get Intl.Locale.prototype.numeric
    static JSTaggedValue GetNumeric(EcmaRuntimeCallInfo *argv);
    // 10.3.12 get Intl.Locale.prototype.numberingSystem
    static JSTaggedValue GetNumberingSystem(EcmaRuntimeCallInfo *argv);
    // 10.3.13 get Intl.Locale.prototype.language
    static JSTaggedValue GetLanguage(EcmaRuntimeCallInfo *argv);
    // 10.3.14 get Intl.Locale.prototype.script
    static JSTaggedValue GetScript(EcmaRuntimeCallInfo *argv);
    // 10.3.15 get Intl.Locale.prototype.region
    static JSTaggedValue GetRegion(EcmaRuntimeCallInfo *argv);
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_LOCALE_H