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

    ACCESSORS_PRIMITIVE_FIELD(GT, uint64_t, BIT_FIELD_OFFSET, SIZE);

    GlobalTSTypeRef GetGTRef() const
    {
        return GlobalTSTypeRef(GetGT());
    }

    void SetGTRef(GlobalTSTypeRef r)
    {
        SetGT(r.GetGlobalTSTypeRef());
    }
};

class TSObjectType : public TSType {
public:
    CAST_CHECK(TSObjectType, IsTSObjectType);

    static constexpr size_t PROPERTIES_OFFSET = TSType::SIZE;

    JSHClass *GetOrCreateHClass(JSThread *thread);

    static GlobalTSTypeRef GetPropTypeGT(JSHandle<TSTypeTable> &table, JSHandle<TSObjectType> objType,
                                          JSHandle<EcmaString> propName);

    ACCESSORS(ObjLayoutInfo, PROPERTIES_OFFSET, HCLASS_OFFSET);
    ACCESSORS(HClass, HCLASS_OFFSET, SIZE);

    DECL_VISIT_OBJECT(PROPERTIES_OFFSET, SIZE)
    DECL_DUMP()

private:
    JSHClass* CreateHClassByProps(JSThread *thread, JSHandle<TSObjLayoutInfo> propType) const;
};

class TSClassType : public TSType {
public:
    CAST_CHECK(TSClassType, IsTSClassType);

    static constexpr size_t FIELD_LENGTH = 4;  // every field record name, typeIndex, accessFlag, readonly
    static constexpr size_t INSTANCE_TYPE_OFFSET = TSType::SIZE;

    static GlobalTSTypeRef GetPropTypeGT(const JSThread *thread, JSHandle<TSTypeTable> &table,
                                          int localtypeId, JSHandle<EcmaString> propName);

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

    static GlobalTSTypeRef GetPropTypeGT(const JSThread *thread, JSHandle<TSTypeTable> &table,
                                          int localtypeId, JSHandle<EcmaString> propName);

    static constexpr size_t CREATE_CLASS_TYPE_OFFSET = TSType::SIZE;
    static constexpr size_t CREATE_CLASS_OFFSET = 1;
    ACCESSORS_PRIMITIVE_FIELD(ClassTypeRef, uint64_t, CREATE_CLASS_TYPE_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    GlobalTSTypeRef GetClassRefGT() const
    {
        return GlobalTSTypeRef(GetClassTypeRef());
    }

    void SetClassRefGT(GlobalTSTypeRef r)
    {
        SetClassTypeRef(r.GetGlobalTSTypeRef());
    }

    DECL_DUMP()
};

class TSImportType : public TSType {
public:
    CAST_CHECK(TSImportType, IsTSImportType);

    static constexpr size_t IMPORT_TYPE_ID_OFFSET = TSType::SIZE;
    static constexpr size_t IMPORT_PATH_OFFSET_IN_LITERAL = 1;
    ACCESSORS(ImportPath, IMPORT_TYPE_ID_OFFSET, IMPORT_PATH);
    ACCESSORS_PRIMITIVE_FIELD(TargetTypeRef, uint64_t, IMPORT_PATH, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    GlobalTSTypeRef GetTargetRefGT() const
    {
        return GlobalTSTypeRef(GetTargetTypeRef());
    }

    void SetTargetRefGT(GlobalTSTypeRef r)
    {
        SetTargetTypeRef(r.GetGlobalTSTypeRef());
    }

    DECL_VISIT_OBJECT(IMPORT_TYPE_ID_OFFSET, IMPORT_PATH)
    DECL_DUMP()
};

class TSUnionType : public TSType {
public:
    CAST_CHECK(TSUnionType, IsTSUnionType);

    bool IsEqual(JSThread *thread, JSHandle<TSUnionType> unionB);

    static constexpr size_t COMPONENT_OFFSET = TSType::SIZE;
    ACCESSORS(ComponentTypes, COMPONENT_OFFSET, SIZE);

    DECL_VISIT_OBJECT(COMPONENT_OFFSET, SIZE)
    DECL_DUMP()
};

class TSInterfaceType : public TSType {
public:
    CAST_CHECK(TSInterfaceType, IsTSInterfaceType);

    static constexpr size_t EXTENDS_TYPE_ID_OFFSET = TSType::SIZE;
    ACCESSORS(Extends, EXTENDS_TYPE_ID_OFFSET, KEYS_OFFSET);
    ACCESSORS(Fields, KEYS_OFFSET, SIZE);

    DECL_VISIT_OBJECT(EXTENDS_TYPE_ID_OFFSET, SIZE)
    DECL_DUMP()
};

class TSFunctionType : public TSType {
public:
    CAST_CHECK(TSFunctionType, IsTSFunctionType);

    static constexpr size_t PARAMETER_TYPE_OFFSET = TSType::SIZE;
    static constexpr size_t FUNCTION_NAME_OFFSET = 0;
    static constexpr size_t RETURN_VALUE_TYPE_OFFSET = 1;
    static constexpr size_t PARAMETER_START_ENTRY = 2;
    static constexpr size_t FIELD_LENGTH = 2;  // every function record accessFlag, modifierStatic
    static constexpr size_t DEFAULT_LENGTH = 2;

    ACCESSORS(ParameterTypes, PARAMETER_TYPE_OFFSET, SIZE);

    int GetParametersNum();

    GlobalTSTypeRef GetParameterTypeGT(JSHandle<TSTypeTable> typeTable, int index);

    GlobalTSTypeRef GetReturnValueTypeGT(JSHandle<TSTypeTable> typeTable);

    DECL_VISIT_OBJECT(PARAMETER_TYPE_OFFSET, SIZE)
    DECL_DUMP()
};

class TSArrayType : public TSType {
public:
    CAST_CHECK(TSArrayType, IsTSArrayType);
    static constexpr size_t  PARAMETER_TYPE_REF_OFFSET = TSType::SIZE;

    GlobalTSTypeRef GetElementTypeGT(JSHandle<TSTypeTable> typeTable);
    ACCESSORS_PRIMITIVE_FIELD(ElementTypeRef, uint64_t, PARAMETER_TYPE_REF_OFFSET, SIZE);
    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_TS_TYPES_TS_TYPE_H
