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
#include "ts_type.h"

#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/global_env.h"
#include "ecmascript/layout_info.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/ts_types/ts_obj_layout_info-inl.h"
#include "ecmascript/ts_types/ts_type_table.h"

namespace panda::ecmascript {
JSHClass *TSObjectType::GetOrCreateHClass(JSThread *thread)
{
    JSTaggedValue mayBeHClass = GetHClass();
    if (mayBeHClass.IsJSHClass()) {
        return JSHClass::Cast(mayBeHClass.GetTaggedObject());
    }
    JSHandle<TSObjLayoutInfo> propTypeInfo(thread, GetObjLayoutInfo().GetTaggedObject());
    JSHClass *hclass = CreateHClassByProps(thread, propTypeInfo);
    SetHClass(thread, JSTaggedValue(hclass));

    return hclass;
}

JSHClass *TSObjectType::CreateHClassByProps(JSThread *thread, JSHandle<TSObjLayoutInfo> propType) const
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    int length = propType->GetLength();
    if (length > PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES) {
        LOG(ERROR, RUNTIME) << "TSobject type has too many keys and cannot create hclass";
        UNREACHABLE();
    }

    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSHandle<LayoutInfo> layout = factory->CreateLayoutInfo(length);
    for (int index = 0; index < length; ++index) {
        JSTaggedValue tsPropKey = propType->GetKey(index);
        key.Update(tsPropKey);
        ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
        PropertyAttributes attributes = PropertyAttributes::Default();
        attributes.SetIsInlinedProps(true);
        attributes.SetRepresentation(Representation::MIXED);
        attributes.SetOffset(index);
        layout->AddKey(thread, index, key.GetTaggedValue(), attributes);
    }
    JSHandle<JSHClass> hclass = factory->NewEcmaDynClass(JSObject::SIZE, JSType::JS_OBJECT, length);
    hclass->SetLayout(thread, layout);
    hclass->SetNumberOfProps(length);

    return *hclass;
}

bool TSUnionType::IsEqual(JSThread *thread, JSHandle<TSUnionType> unionB)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(unionB->GetComponentTypes().IsTaggedArray());
    bool findUnionTag = 0;

    TaggedArray *unionArrayA = TaggedArray::Cast(TSUnionType::GetComponentTypes().GetTaggedObject());
    TaggedArray *unionArrayB = TaggedArray::Cast(unionB->GetComponentTypes().GetTaggedObject());
    int unionALength = unionArrayA->GetLength();
    int unionBLength = unionArrayB->GetLength();
    if (unionALength != unionBLength) {
        return false;
    }
    for (int unionAIndex = 0; unionAIndex < unionALength; unionAIndex++) {
        int argUnionA = unionArrayA->Get(unionAIndex).GetNumber();
        bool findArgTag = 0;
        for (int unionBIndex = 0; unionBIndex < unionBLength; unionBIndex++) {
            int argUnionB = unionArrayB->Get(unionBIndex).GetNumber();
            if (argUnionA == argUnionB) {
                findArgTag = 1;
                break;
            }
        }
        if (!findArgTag) {
            return findUnionTag;
        }
    }
    findUnionTag = 1;
    return findUnionTag;
}

GlobalTSTypeRef TSClassType::SearchProperty(JSThread *thread, JSHandle<TSTypeTable> &table, TSTypeKind typeKind,
                                            int localtypeId, JSHandle<EcmaString> propName)
{
    DISALLOW_GARBAGE_COLLECTION;
    CString propertyName = ConvertToString(propName.GetTaggedValue());

    TSClassType *classType = TSClassType::Cast(table->Get(localtypeId).GetTaggedObject());
    TSObjectType *constructorType = TSObjectType::Cast(classType->GetConstructorType().GetTaggedObject());
    TSObjLayoutInfo *propTypeInfo = TSObjLayoutInfo::Cast(constructorType->GetObjLayoutInfo().GetTaggedObject());

    // search static propType in constructorType
    for (int index = 0; index < propTypeInfo->NumberOfElements(); ++index) {
        JSTaggedValue tsPropKey = propTypeInfo->GetKey(index);
        if (ConvertToString(tsPropKey) == propertyName) {
            int localId = propTypeInfo->GetTypeId(index).GetNumber();
            if (localId < GlobalTSTypeRef::TS_TYPE_RESERVED_COUNT) {
                return GlobalTSTypeRef(localId);
            }
            int localIdNonOffset = TSLoader::GetUserdefinedTypeId(localId);
            TSType *propertyType = TSType::Cast(table->Get(localIdNonOffset).GetTaggedObject());
            if (JSTaggedValue(propertyType).IsTSImportType()) {
                TSImportType *importType = TSImportType::Cast(table->Get(localIdNonOffset).GetTaggedObject());
                return importType->GetGTRef();
            }
            return propertyType->GetGTRef();
        }
    }
    return GlobalTSTypeRef::Default();
}

GlobalTSTypeRef TSClassInstanceType::SearchProperty(JSThread *thread, JSHandle<TSTypeTable> &table, TSTypeKind typeKind,
                                                    int localtypeId, JSHandle<EcmaString> propName)
{
    DISALLOW_GARBAGE_COLLECTION;
    CString propertyName = ConvertToString(propName.GetTaggedValue());

    TSClassInstanceType *classInstanceType = TSClassInstanceType::Cast(table->Get(localtypeId).GetTaggedObject());
    int localId = 0;
    TSClassType *createClassType = TSClassType::Cast(classInstanceType->GetCreateClassType().GetTaggedObject());
    TSObjectType *instanceType = TSObjectType::Cast(createClassType->GetInstanceType().GetTaggedObject());
    TSObjectType *prototype = TSObjectType::Cast(createClassType->GetPrototypeType().GetTaggedObject());
    TSObjLayoutInfo *instanceTypeInfo = TSObjLayoutInfo::Cast(instanceType->GetObjLayoutInfo().GetTaggedObject());
    TSObjLayoutInfo *prototypeTypeInfo = TSObjLayoutInfo::Cast(prototype->GetObjLayoutInfo().GetTaggedObject());

    // search non-static propType in instanceType
    for (int index = 0; index < instanceTypeInfo->NumberOfElements(); ++index) {
        JSTaggedValue instancePropKey = instanceTypeInfo->GetKey(index);
        if (ConvertToString(instancePropKey) == propertyName) {
            localId = instanceTypeInfo->GetTypeId(index).GetNumber();
            if (localId < GlobalTSTypeRef::TS_TYPE_RESERVED_COUNT) {
                return GlobalTSTypeRef(localId);
            }
            int localIdNonOffset = TSLoader::GetUserdefinedTypeId(localId);
            TSType *propertyType = TSType::Cast(table->Get(localIdNonOffset).GetTaggedObject());
            if (propertyType->GetGTRef().GetUserDefineTypeKind() ==
                static_cast<int>(TSTypeKind::TS_IMPORT)) {
                TSImportType *importType = TSImportType::Cast(table->Get(localIdNonOffset).GetTaggedObject());
                return importType->GetGTRef();
            }
            return propertyType->GetGTRef();
        }
    }
    // search functionType in prototype
    for (int index = 0; index < prototypeTypeInfo->NumberOfElements(); ++index) {
        JSTaggedValue prototypePropKey = prototypeTypeInfo->GetKey(index);
        if (ConvertToString(prototypePropKey) == propertyName) {
            localId = prototypeTypeInfo->GetTypeId(index).GetNumber();
            ASSERT(localId > GlobalTSTypeRef::TS_TYPE_RESERVED_COUNT);
            int localIdNonOffset = TSLoader::GetUserdefinedTypeId(localId);
            TSType *propertyType = TSType::Cast(table->Get(localIdNonOffset).GetTaggedObject());
            return propertyType->GetGTRef();
        }
    }
    return GlobalTSTypeRef::Default();
}
}