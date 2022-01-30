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

#ifndef ECMASCRIPT_CLASS_INFO_EXTRACTOR_H
#define ECMASCRIPT_CLASS_INFO_EXTRACTOR_H

#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
// ClassInfoExtractor will analyze and extract the contents from class literal to keys, properties and elements(both
// non-static and static), later generate the complete hclass (both prototype and constructor) based on keys.
// Attention: keys accessor stores the property key and properties accessor stores the property value, but elements
// accessor stores the key-value pair abuttally.
class ClassInfoExtractor : public TaggedObject {
public:
    static constexpr uint8_t NON_STATIC_RESERVED_LENGTH = 1;
    static constexpr uint8_t STATIC_RESERVED_LENGTH = 3;

    static constexpr uint8_t CONSTRUCTOR_INDEX = 0;
    static constexpr uint8_t LENGTH_INDEX = 0;
    static constexpr uint8_t NAME_INDEX = 1;
    static constexpr uint8_t PROTOTYPE_INDEX = 2;

    struct ExtractContentsDetail {
        uint32_t extractBegin;
        uint32_t extractEnd;
        uint8_t fillStartLoc;
        JSMethod *ctorMethod;
    };

    CAST_CHECK(ClassInfoExtractor, IsClassInfoExtractor);

    static void BuildClassInfoExtractorFromLiteral(JSThread *thread, JSHandle<ClassInfoExtractor> &extractor,
                                                   const JSHandle<TaggedArray> &literal);

    static constexpr size_t PROTOTYPE_HCLASS_OFFSET = TaggedObjectSize();
    ACCESSORS(PrototypeHClass, PROTOTYPE_HCLASS_OFFSET, NON_STATIC_KEYS_OFFSET)
    ACCESSORS(NonStaticKeys, NON_STATIC_KEYS_OFFSET, NON_STATIC_PROPERTIES_OFFSET)
    ACCESSORS(NonStaticProperties, NON_STATIC_PROPERTIES_OFFSET, NON_STATIC_ELEMENTS_OFFSET)
    ACCESSORS(NonStaticElements, NON_STATIC_ELEMENTS_OFFSET, CONSTRUCTOR_HCLASS_OFFSET)
    ACCESSORS(ConstructorHClass, CONSTRUCTOR_HCLASS_OFFSET, STATIC_KEYS_OFFSET)
    ACCESSORS(StaticKeys, STATIC_KEYS_OFFSET, STATIC_PROPERTIES_OFFSET)
    ACCESSORS(StaticProperties, STATIC_PROPERTIES_OFFSET, STATIC_ELEMENTS_OFFSET)
    ACCESSORS(StaticElements, STATIC_ELEMENTS_OFFSET, CONSTRUCTOR_METHOD_OFFSET)
    ACCESSORS_NATIVE_FIELD(ConstructorMethod, JSMethod, CONSTRUCTOR_METHOD_OFFSET, BIT_FIELD_OFFSET)
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t NON_STATIC_BITS = 1;
    static constexpr size_t STATIC_BITS = 1;
    FIRST_BIT_FIELD(BitField, NonStaticWithElements, bool, NON_STATIC_BITS)
    NEXT_BIT_FIELD(BitField, StaticWithElements, bool, STATIC_BITS, NonStaticWithElements)

    DECL_VISIT_OBJECT(PROTOTYPE_HCLASS_OFFSET, CONSTRUCTOR_METHOD_OFFSET)
    DECL_DUMP()

private:
    static bool ExtractAndReturnWhetherWithElements(JSThread *thread, const JSHandle<TaggedArray> &literal,
                                                    const ExtractContentsDetail &detail,
                                                    JSHandle<TaggedArray> &keys, JSHandle<TaggedArray> &properties,
                                                    JSHandle<TaggedArray> &elements);

    static JSHandle<JSHClass> CreatePrototypeHClass(JSThread *thread, JSHandle<TaggedArray> &keys,
                                                    JSHandle<TaggedArray> &properties);

    static JSHandle<JSHClass> CreateConstructorHClass(JSThread *thread, JSHandle<TaggedArray> &keys,
                                                      JSHandle<TaggedArray> &properties);
};

enum class ClassPropertyType : uint8_t { NON_STATIC = 0, STATIC };

class ClassHelper {
public:
    static JSHandle<JSFunction> DefineClassTemplate(JSThread *thread, JSHandle<ClassInfoExtractor> &extractor,
                                                    const JSHandle<ConstantPool> &constantpool);

private:
    static JSHandle<NameDictionary> BuildDictionaryPropeties(JSThread *thread, const JSHandle<JSObject> &object,
                                                             JSHandle<TaggedArray> &keys,
                                                             JSHandle<TaggedArray> &properties, ClassPropertyType type,
                                                             const JSHandle<ConstantPool> &constantpool);

    static void HandleElementsProperties(JSThread *thread, const JSHandle<JSObject> &object,
                                         JSHandle<TaggedArray> &elements, const JSHandle<ConstantPool> &constantpool);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_CLASS_INFO_EXTRACTOR_H
