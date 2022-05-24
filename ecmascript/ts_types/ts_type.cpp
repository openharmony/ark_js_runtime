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
#include "ecmascript/ts_types/ts_obj_layout_info.h"
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

    uint32_t length = propType->GetLength();
    if (length > PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES) {
        LOG(ERROR, RUNTIME) << "TSobject type has too many keys and cannot create hclass";
        UNREACHABLE();
    }

    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSHandle<LayoutInfo> layout = factory->CreateLayoutInfo(length);
    for (uint32_t index = 0; index < length; ++index) {
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

bool TSUnionType::IsEqual(JSHandle<TSUnionType> unionB)
{
    DISALLOW_GARBAGE_COLLECTION;
    ASSERT(unionB->GetComponentTypes().IsTaggedArray());
    bool findUnionTag = 0;

    TaggedArray *unionArrayA = TaggedArray::Cast(TSUnionType::GetComponentTypes().GetTaggedObject());
    TaggedArray *unionArrayB = TaggedArray::Cast(unionB->GetComponentTypes().GetTaggedObject());
    uint32_t unionALength = unionArrayA->GetLength();
    uint32_t unionBLength = unionArrayB->GetLength();
    if (unionALength != unionBLength) {
        return false;
    }
    for (uint32_t unionAIndex = 0; unionAIndex < unionALength; unionAIndex++) {
        int argUnionA = unionArrayA->Get(unionAIndex).GetNumber();
        bool findArgTag = 0;
        for (uint32_t unionBIndex = 0; unionBIndex < unionBLength; unionBIndex++) {
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

GlobalTSTypeRef TSClassType::GetPropTypeGT(const JSThread *thread, JSHandle<TSTypeTable> &table,
                                           int localtypeId, JSHandle<EcmaString> propName)
{
    JSHandle<TSClassType> classType(thread, table->Get(localtypeId));
    JSHandle<TSObjectType> constructorType(thread, classType->GetConstructorType());

    // search static propType in constructorType
    return TSObjectType::GetPropTypeGT(table, constructorType, propName);
}

GlobalTSTypeRef TSClassInstanceType::GetPropTypeGT(const JSThread *thread, JSHandle<TSTypeTable> &table,
                                                   int localtypeId, JSHandle<EcmaString> propName)
{
    JSHandle<TSClassInstanceType> classInstanceType(thread, table->Get(localtypeId));
    GlobalTSTypeRef createClassTypeRefGT = classInstanceType->GetClassRefGT();
    uint32_t localId = createClassTypeRefGT.GetLocalId();

    JSHandle<TSClassType> createClassType(thread, table->Get(localId));
    JSHandle<TSObjectType> instanceType(thread, createClassType->GetInstanceType());
    JSHandle<TSObjectType> protoTypeType(thread, createClassType->GetPrototypeType());

    // search non-static propType in instanceType
    GlobalTSTypeRef propTypeGT = TSObjectType::GetPropTypeGT(table, instanceType, propName);
    if (propTypeGT.IsDefault()) {
        // search non-static propType in prototypeType
        propTypeGT = TSObjectType::GetPropTypeGT(table, protoTypeType, propName);
    }
    return propTypeGT;
}

GlobalTSTypeRef TSObjectType::GetPropTypeGT(JSHandle<TSTypeTable> &table, JSHandle<TSObjectType> objType,
                                            JSHandle<EcmaString> propName)
{
    DISALLOW_GARBAGE_COLLECTION;
    TSObjLayoutInfo *objTypeInfo = TSObjLayoutInfo::Cast(objType->GetObjLayoutInfo().GetTaggedObject());
    for (uint32_t index = 0; index < objTypeInfo->NumberOfElements(); ++index) {
        EcmaString* propKey = EcmaString::Cast(objTypeInfo->GetKey(index).GetTaggedObject());
        if (EcmaString::StringsAreEqual(propKey, *propName)) {
            int localId = objTypeInfo->GetTypeId(index).GetInt();
            if (localId < GlobalTSTypeRef::TS_TYPE_RESERVED_COUNT) {
                return GlobalTSTypeRef(localId);
            }
            int localIdNonOffset = TSTypeTable::GetUserdefinedTypeId(localId);
            TSType *propertyType = TSType::Cast(table->Get(localIdNonOffset).GetTaggedObject());
            if (JSTaggedValue(propertyType).IsTSImportType()) {
                TSImportType *importType = TSImportType::Cast(table->Get(localIdNonOffset).GetTaggedObject());
                return importType->GetTargetRefGT();
            }
            return propertyType->GetGTRef();
        }
    }
    return GlobalTSTypeRef::Default();
}

int TSFunctionType::GetParametersNum()
{
    DISALLOW_GARBAGE_COLLECTION;
    TaggedArray* functionParametersArray = TaggedArray::Cast(GetParameterTypes().GetTaggedObject());
    return functionParametersArray->GetLength() - DEFAULT_LENGTH;
}

GlobalTSTypeRef TSFunctionType::GetParameterTypeGT(JSHandle<TSTypeTable> typeTable, int index)
{
    DISALLOW_GARBAGE_COLLECTION;
    TaggedArray* functionParametersArray = TaggedArray::Cast(GetParameterTypes().GetTaggedObject());
    JSTaggedValue parameterType = functionParametersArray->Get(index + DEFAULT_LENGTH);
    ASSERT(parameterType.IsInt());
    int parameterTypeRef = parameterType.GetInt();
    if (GlobalTSTypeRef(parameterTypeRef).IsBuiltinType()) {
        return GlobalTSTypeRef(parameterTypeRef);
    }
    ASSERT(parameterTypeRef > GlobalTSTypeRef::TS_TYPE_RESERVED_COUNT);
    int parameterLocalId = TSTypeTable::GetUserdefinedTypeId(parameterTypeRef);
    TSType* Type = TSType::Cast(typeTable->Get(parameterLocalId).GetTaggedObject());
    return Type->GetGTRef();
}

GlobalTSTypeRef TSFunctionType::GetReturnValueTypeGT(JSHandle<TSTypeTable> typeTable)
{
    DISALLOW_GARBAGE_COLLECTION;
    TaggedArray* functionParametersArray = TaggedArray::Cast(GetParameterTypes().GetTaggedObject());
    JSTaggedValue returnType = functionParametersArray->Get(RETURN_VALUE_TYPE_OFFSET);

    ASSERT(returnType.IsInt());
    int returnTypeRef = returnType.GetInt();
    if (GlobalTSTypeRef(returnTypeRef).IsBuiltinType()) {
        return GlobalTSTypeRef(returnTypeRef);
    }
    ASSERT(returnTypeRef > GlobalTSTypeRef::TS_TYPE_RESERVED_COUNT);
    int index = TSTypeTable::GetUserdefinedTypeId(returnTypeRef);
    TSType* Type = TSType::Cast(typeTable->Get(index).GetTaggedObject());
    return Type->GetGTRef();
}

GlobalTSTypeRef TSArrayType::GetElementTypeGT(JSHandle<TSTypeTable> typeTable)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint64_t parameterTypeRef = GetElementTypeRef();
    if (GlobalTSTypeRef(parameterTypeRef).IsBuiltinType()) {
        return GlobalTSTypeRef(parameterTypeRef);
    }
    ASSERT(parameterTypeRef > GlobalTSTypeRef::TS_TYPE_RESERVED_COUNT);
    int index = TSTypeTable::GetUserdefinedTypeId(parameterTypeRef);
    TSType* Type = TSType::Cast(typeTable->Get(index).GetTaggedObject());
    return Type->GetGTRef();
}
} // namespace panda::ecmascript