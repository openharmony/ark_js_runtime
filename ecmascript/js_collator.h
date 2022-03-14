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

#ifndef ECMASCRIPT_JS_COLLATOR_H
#define ECMASCRIPT_JS_COLLATOR_H

#include "js_locale.h"

#include "unicode/udata.h"

namespace panda::ecmascript {
enum class UsageOption : uint8_t { SORT = 0x01, SEARCH, EXCEPTION };
enum class CaseFirstOption : uint8_t { UPPER = 0x01, LOWER, FALSE_OPTION, UNDEFINED, EXCEPTION };
enum class SensitivityOption : uint8_t { BASE = 0x01, ACCENT, CASE, VARIANT, UNDEFINED, EXCEPTION };

class JSCollator : public JSObject {
public:
    // NOLINTNEXTLINE (readability-identifier-naming, fuchsia-statically-constructed-objects)
    static const CString uIcuDataColl;

    static const std::map<std::string, CaseFirstOption> caseFirstMap;

    static const std::map<CaseFirstOption, UColAttributeValue> uColAttributeValueMap;

    CAST_CHECK(JSCollator, IsJSCollator);

    static constexpr size_t ICU_FIELD_OFFSET = JSObject::SIZE;
    // icu field.
    ACCESSORS(IcuField, ICU_FIELD_OFFSET, LOCALE_OFFSET)
    ACCESSORS(Locale, LOCALE_OFFSET, COLLATION_OFFSET)
    ACCESSORS(Collation, COLLATION_OFFSET, BOUND_COMPARE_OFFSET)
    ACCESSORS(BoundCompare, BOUND_COMPARE_OFFSET, BIT_FIELD_OFFSET)
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t USAGE_BITS = 2;
    static constexpr size_t CASE_FIRST_BITS = 3;
    static constexpr size_t SENSITIVITY_BITS = 3;
    static constexpr size_t IGNORE_PUNCTUATION_BITS = 1;
    static constexpr size_t NUMERIC_BITS = 1;
    FIRST_BIT_FIELD(BitField, Usage, UsageOption, USAGE_BITS)
    NEXT_BIT_FIELD(BitField, CaseFirst, CaseFirstOption, CASE_FIRST_BITS, Usage)
    NEXT_BIT_FIELD(BitField, Sensitivity, SensitivityOption, SENSITIVITY_BITS, CaseFirst)
    NEXT_BIT_FIELD(BitField, IgnorePunctuation, bool, IGNORE_PUNCTUATION_BITS, Sensitivity)
    NEXT_BIT_FIELD(BitField, Numeric, bool, NUMERIC_BITS, IgnorePunctuation)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, ICU_FIELD_OFFSET, BIT_FIELD_OFFSET)
    DECL_DUMP()

    icu::Collator *GetIcuCollator() const
    {
        ASSERT(GetIcuField().IsJSNativePointer());
        auto result = JSNativePointer::Cast(GetIcuField().GetTaggedObject())->GetExternalPointer();
        return reinterpret_cast<icu::Collator *>(result);
    }

    static void FreeIcuCollator(void *pointer, [[maybe_unused]] void *hint = nullptr)
    {
        if (pointer == nullptr) {
            return;
        }
        auto icuCollator = reinterpret_cast<icu::Collator *>(pointer);
        icuCollator->~Collator();
    }

    static void SetIcuCollator(JSThread *thread, const JSHandle<JSCollator> &collator,
        icu::Collator *icuCollator, const DeleteEntryPoint &callback);

    // 11.1.1 InitializeCollator ( collator, locales, options )
    static JSHandle<JSCollator> InitializeCollator(JSThread *thread, const JSHandle<JSCollator> &collator,
                                                   const JSHandle<JSTaggedValue> &locales,
                                                   const JSHandle<JSTaggedValue> &options);

    // 11.3.4 Intl.Collator.prototype.resolvedOptions ()
    static JSHandle<JSObject> ResolvedOptions(JSThread *thread, const JSHandle<JSCollator> &collator);

    static JSHandle<TaggedArray> GetAvailableLocales(JSThread *thread);

    static JSTaggedValue CompareStrings(const icu::Collator *icuCollator, const JSHandle<EcmaString> &string1,
                                        const JSHandle<EcmaString> &string2);

private:
    static CaseFirstOption StringToCaseForstOption(const std::string &str);

    static UColAttributeValue OptionToUColAttribute(CaseFirstOption caseFirstOption);

    static std::set<std::string> BuildLocaleSet(const std::vector<std::string> &availableLocales, const char *path,
                                                const char *key);

    static void SetNumericOption(icu::Collator *icuCollator, bool numeric);

    static void SetCaseFirstOption(icu::Collator *icuCollator, CaseFirstOption caseFirstOption);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_JS_COLLATOR_H