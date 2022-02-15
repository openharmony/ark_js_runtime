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

#ifndef ECMASCRIPT_TS_TYPES_TS_TYPE_H
#define ECMASCRIPT_TS_TYPES_TS_TYPE_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/tagged_object.h"
#include "ecmascript/property_attributes.h"
#include "ecmascript/ts_types/ts_loader.h"
#include "include/hclass.h"
#include "utils/bit_field.h"
#include "ecmascript/ts_types/ts_obj_layout_info.h"

namespace panda::ecmascript {

class TSType : public TaggedObject {
public:
    static constexpr size_t BIT_FIELD_OFFSET = TaggedObjectSize();

    inline static TSType *Cast(const TaggedObject *object)
    {
        return static_cast<TSType *>(const_cast<TaggedObject *>(object));
    }

    ACCESSORS_PRIMITIVE_FIELD(Uint64, uint64_t, BIT_FIELD_OFFSET, SIZE);

    GlobalTSTypeRef GetGTRef() const
    {
        return GlobalTSTypeRef(GetUint64());
    }

    void SetGTRef(GlobalTSTypeRef r)
    {
        SetUint64(r.GetGlobalTSTypeRef());
    }
};

class TSObjectType : public TSType {
public:
    CAST_CHECK(TSObjectType, IsTSObjectType);

    JSHClass *GetOrCreateHClass(JSThread *thread);

    uint64_t GetTypeIdOfKey(JSTaggedValue key);

    static constexpr size_t PROPERTIES_OFFSET = TSType::SIZE;
    ACCESSORS(ObjLayoutInfo, PROPERTIES_OFFSET, HCLASS_OFFSET);
    ACCESSORS(HClass, HCLASS_OFFSET, SIZE);

    DECL_VISIT_OBJECT(PROPERTIES_OFFSET, SIZE)
    DECL_DUMP()

private:
    JSHClass* CreateHClassByProps(JSThread *thread, TSObjLayoutInfo *propType) const;
};

class TSClassType : public TSType {
public:
    CAST_CHECK(TSClassType, IsTSClassType);

    static constexpr uint8_t FIELD_LENGTH = 4;  // every field record name, typeIndex, accessFlag, readonly
    static constexpr size_t INSTANCE_TYPE_OFFSET = TSType::SIZE;

    ACCESSORS(InstanceType, INSTANCE_TYPE_OFFSET, CONSTRUCTOR_TYPE_OFFSET);
    ACCESSORS(ConstructorType, CONSTRUCTOR_TYPE_OFFSET, PROTOTYPE_TYPE_OFFSET);
    ACCESSORS(PrototypeType, PROTOTYPE_TYPE_OFFSET, EXTENSION_TYPE_OFFSET);
    ACCESSORS(ExtensionType, EXTENSION_TYPE_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT(INSTANCE_TYPE_OFFSET, LAST_OFFSET)
    DECL_DUMP()
};

class TSClassInstanceType : public TSType {
public:
    CAST_CHECK(TSClassInstanceType, IsTSClassInstanceType);

    static constexpr size_t CREATE_CLASS_TYPE_OFFSET = TSType::SIZE;
    ACCESSORS(CreateClassType, CREATE_CLASS_TYPE_OFFSET, SIZE);

    DECL_VISIT_OBJECT(CREATE_CLASS_TYPE_OFFSET, SIZE)
    DECL_DUMP()
};

class TSImportType : public TSType {
public:
    CAST_CHECK(TSImportType, IsTSImportType);

    static constexpr size_t IMPORT_TYPE_ID_OFFSET = TSType::SIZE;

    ACCESSORS(ImportPath, IMPORT_TYPE_ID_OFFSET, IMPORT_PATH);
    ACCESSORS(TargetType, IMPORT_PATH, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT(IMPORT_TYPE_ID_OFFSET, IMPORT_PATH)
    DECL_DUMP()
};

class TSUnionType : public TSType {
public:
    CAST_CHECK(TSUnionType, IsTSUnionType);

    bool IsEqual(JSThread *thread, JSHandle<TSUnionType> unionB);

    static constexpr size_t KEYS_OFFSET = TSType::SIZE;
    ACCESSORS(ComponentTypes, KEYS_OFFSET, SIZE);

    DECL_VISIT_OBJECT(KEYS_OFFSET, SIZE)
    DECL_DUMP()
};

class TSInterfaceType : public TSType {
public:
    CAST_CHECK(TSInterfaceType, IsTSInterfaceType);

    static constexpr size_t EXTENDS_TYPE_ID_OFFSET = TSType::SIZE;
    ACCESSORS(Extends, EXTENDS_TYPE_ID_OFFSET, KEYS_OFFSET);
    ACCESSORS(Fields, KEYS_OFFSET, SIZE);

    DECL_VISIT_OBJECT(KEYS_OFFSET, SIZE)
    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_TS_TYPE_H
