/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_LIST_FORMAT_H
#define ECMASCRIPT_JS_LIST_FORMAT_H

#include "js_array.h"
#include "js_hclass.h"
#include "js_intl.h"
#include "js_locale.h"
#include "js_object.h"
#include "js_tagged_value.h"
#include "unicode/listformatter.h"

namespace panda::ecmascript {
enum class ListTypeOption : uint8_t {
    CONJUNCTION = 0x01,
    DISJUNCTION,
    UNIT,
    EXCEPTION
};

enum class ListStyleOption : uint8_t {
    LONG = 0x01,
    SHORT,
    NARROW,
    EXCEPTION
};

class JSListFormat : public JSObject {
public:
    CAST_CHECK(JSListFormat, IsJSListFormat);

    static constexpr size_t LOCALE_OFFSET = JSObject::SIZE;
    ACCESSORS(Locale, LOCALE_OFFSET, ICU_LIST_FORMAT)
    ACCESSORS(IcuLF, ICU_LIST_FORMAT, BIT_FIELD_OFFSET)
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t TYPE_BITS = 3;
    static constexpr size_t STYLE_BITS = 3;
    FIRST_BIT_FIELD(BitField, Type, ListTypeOption, TYPE_BITS)
    NEXT_BIT_FIELD(BitField, Style, ListStyleOption, STYLE_BITS, Type)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, LOCALE_OFFSET, BIT_FIELD_OFFSET)
    DECL_DUMP()

    icu::ListFormatter *GetIcuListFormatter() const;

    static void FreeIcuListFormatter(void *pointer, [[maybe_unused]] void* hint);

    static void SetIcuListFormatter(JSThread *thread, const JSHandle<JSListFormat> obj,
                                    icu::ListFormatter *icuListFormatter, const DeleteEntryPoint &callback);

    static JSHandle<TaggedArray> GetAvailableLocales(JSThread *thread);

    // 13. InitializeListFormat ( listformat, locales, options )
    static JSHandle<JSListFormat> InitializeListFormat(JSThread *thread, const JSHandle<JSListFormat> &listFormat,
                                                       const JSHandle<JSTaggedValue> &locales,
                                                       const JSHandle<JSTaggedValue> &options);

    // 13.1.3 FormatList ( listFormat, list )
    static JSHandle<EcmaString> FormatList(JSThread *thread, const JSHandle<JSListFormat> &listFormat,
                                            const JSHandle<JSArray> &listArray);

    // 13.1.4 FormatListToParts ( listFormat, list )
    static JSHandle<JSArray> FormatListToParts(JSThread *thread, const JSHandle<JSListFormat> &listFormat,
                                                const JSHandle<JSArray> &listArray);

    // 13.1.5 StringListFromIterable ( iterable )
    static JSHandle<JSTaggedValue> StringListFromIterable(JSThread *thread, const JSHandle<JSTaggedValue> &iterable);

    static void ResolvedOptions(JSThread *thread, const JSHandle<JSListFormat> &listFormat,
                                const JSHandle<JSObject> &options);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_LIST_FORMAT_H