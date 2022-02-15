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

namespace panda::ecmascript {
JSHClass *TSObjectType::GetOrCreateHClass(JSThread *thread)
{
    JSTaggedValue mayBeHClass = GetHClass();
    if (mayBeHClass.IsJSHClass()) {
        return JSHClass::Cast(mayBeHClass.GetTaggedObject());
    }
    TSObjLayoutInfo *propTypeInfo = TSObjLayoutInfo::Cast(GetObjLayoutInfo().GetTaggedObject());
    JSHClass *hclass = CreateHClassByProps(thread, propTypeInfo);
    SetHClass(thread, JSTaggedValue(hclass));

    return hclass;
}

JSHClass *TSObjectType::CreateHClassByProps(JSThread *thread, TSObjLayoutInfo *propType) const
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

bool TSUnionType::IsEqual(JSThread *thread, JSHandle<TSUnionType> unionB)
{
    ASSERT(unionB->GetComponentTypes().IsTaggedArray());
    bool findUnionTag = 0;
    JSHandle<TaggedArray> unionArrayA(thread, TSUnionType::GetComponentTypes());
    JSHandle<TaggedArray> unionArrayB(thread, unionB->GetComponentTypes());
    uint32_t unionALength = unionArrayA->GetLength();
    uint32_t unionBLength = unionArrayB->GetLength();
    if (unionALength != unionBLength) {
        return false;
    }
    for (uint32_t unionAIndex = 0; unionAIndex < unionALength; unionAIndex++) {
        uint32_t argUnionA = unionArrayA->Get(unionAIndex).GetNumber();
        bool findArgTag = 0;
        for (uint32_t unionBIndex = 0; unionBIndex < unionBLength; unionBIndex++) {
            uint32_t argUnionB = unionArrayB->Get(unionBIndex).GetNumber();
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
}
