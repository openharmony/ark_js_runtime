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

#ifndef ECMASCRIPT_JS_RELATIVE_TIME_FORMAT_H
#define ECMASCRIPT_JS_RELATIVE_TIME_FORMAT_H

#include "unicode/reldatefmt.h"

#include "ecmascript/base/number_helper.h"
#include "ecmascript/common.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_intl.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
class JSRelativeTimeFormat : public JSObject {
public:
    static JSRelativeTimeFormat *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsJSRelativeTimeFormat());
        return reinterpret_cast<JSRelativeTimeFormat *>(object);
    }

    static constexpr size_t LOCALE_OFFSET = JSObject::SIZE;

    ACCESSORS(Locale, LOCALE_OFFSET, INITIALIZED_RELATIVE_TIME_FORMAT)
    ACCESSORS(InitializedRelativeTimeFormat, INITIALIZED_RELATIVE_TIME_FORMAT, NUMBERING_SYSTEM_OFFSET)
    ACCESSORS(NumberingSystem, NUMBERING_SYSTEM_OFFSET, STYLE_OFFSET)
    ACCESSORS(Style, STYLE_OFFSET, NUMERIC_OFFSET)
    ACCESSORS(Numeric, NUMERIC_OFFSET, AVAILABLE_LOCALES_OFFSET)
    ACCESSORS(AvailableLocales, AVAILABLE_LOCALES_OFFSET, ICU_FIELD_OFFSET)

    // icu field
    ACCESSORS(IcuField, ICU_FIELD_OFFSET, SIZE)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, LOCALE_OFFSET, SIZE)
    DECL_DUMP()

    // 14.1.1 InitializeRelativeTimeFormat ( relativeTimeFormat, locales, options )
    static JSHandle<JSRelativeTimeFormat> InitializeRelativeTimeFormat(
        JSThread *thread, const JSHandle<JSRelativeTimeFormat> &relativeTimeFormat,
        const JSHandle<JSTaggedValue> &locales, const JSHandle<JSTaggedValue> &options);

    // UnwrapRelativeTimeFormat
    static JSHandle<JSTaggedValue> UnwrapRelativeTimeFormat(JSThread *thread, const JSHandle<JSTaggedValue> &rtf);

    // Get icu formatter from icu field
    icu::RelativeDateTimeFormatter *GetIcuRTFFormatter() const
    {
        ASSERT(GetIcuField().IsJSNativePointer());
        auto result = JSNativePointer::Cast(GetIcuField().GetTaggedObject())->GetExternalPointer();
        return reinterpret_cast<icu::RelativeDateTimeFormatter *>(result);
    }

    static void FreeIcuRTFFormatter(void *pointer, void *data)
    {
        if (pointer == nullptr) {
            return;
        }
        auto icuFormatter = reinterpret_cast<icu::RelativeDateTimeFormatter *>(pointer);
        icuFormatter->~RelativeDateTimeFormatter();
        if (data != nullptr) {
            reinterpret_cast<EcmaVM *>(data)->GetRegionFactory()->FreeBuffer(pointer);
        }
    }

    static void ResolvedOptions(JSThread *thread, const JSHandle<JSRelativeTimeFormat> &relativeTimeFormat,
                                const JSHandle<JSObject> &options);

    static JSHandle<EcmaString> Format(JSThread *thread, double value, const JSHandle<EcmaString> &unit,
                                       const JSHandle<JSRelativeTimeFormat> &relativeTimeFormat);

    static JSHandle<JSArray> FormatToParts(JSThread *thread, double value, const JSHandle<EcmaString> &unit,
                                           const JSHandle<JSRelativeTimeFormat> &relativeTimeFormat);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_JS_RELATIVE_TIME_FORMAT_H