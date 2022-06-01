/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_FINALIZATION_REGISTRY_H
#define ECMASCRIPT_JS_FINALIZATION_REGISTRY_H

#include "ecmascript/js_object.h"
#include "ecmascript/record.h"
#include "ecmascript/weak_vector.h"

namespace panda::ecmascript {
class CheckAndCallScrop {
public:
    CheckAndCallScrop(JSThread *thread) : thread_(thread)
    {
        thread_->SetCheckAndCallEnterState(true);
    }
    ~CheckAndCallScrop()
    {
        thread_->SetCheckAndCallEnterState(false);
    }
private:
    JSThread *thread_;
};

class CellRecord final : public Record {
public:
    CAST_CHECK(CellRecord, IsCellRecord);
    void SetToWeakRefTarget(JSTaggedValue value)
    {
        JSTaggedValue weakObj = JSTaggedValue(value.CreateAndGetWeakRef());
        ASSERT(weakObj.IsWeak());
        SetWeakRefTarget(weakObj);
    }

    JSTaggedValue GetFromWeakRefTarget() const
    {
        JSTaggedValue weakObj = GetWeakRefTarget();
        if (!weakObj.IsUndefined()) {
            return JSTaggedValue(weakObj.GetWeakReferent());
        }
        return weakObj;
    }
    static constexpr size_t WEAKREF_TARGET_OFFSET = Record::SIZE;
    ACCESSORS(WeakRefTarget, WEAKREF_TARGET_OFFSET, HELD_VALUE_OFFSET)
    ACCESSORS(HeldValue, HELD_VALUE_OFFSET, SIZE)

    DECL_VISIT_OBJECT(WEAKREF_TARGET_OFFSET, SIZE)
    DECL_DUMP()
};

class CellRecordVector : public WeakVector {
public:
    static CellRecordVector *Cast(ObjectHeader *object)
    {
        return static_cast<CellRecordVector *>(object);
    }
    static constexpr uint32_t DEFAULT_GROW_SIZE = 5; // If the capacity is not enough, we Expansion five each time
    static JSHandle<CellRecordVector> Append(const JSThread *thread, const JSHandle<CellRecordVector> &array,
                                             const JSHandle<JSTaggedValue> &value);
    bool IsEmpty();
private:
    static uint32_t CheckHole(const JSHandle<CellRecordVector> &array);
};

class JSFinalizationRegistry : public JSObject {
public:
    CAST_CHECK(JSFinalizationRegistry, IsJSFinalizationRegistry);

    static void Register(JSThread *thread, JSHandle<JSTaggedValue> target, JSHandle<JSTaggedValue> heldValue,
                         JSHandle<JSTaggedValue> unregisterToken, JSHandle<JSFinalizationRegistry> obj);
    static bool Unregister(JSThread *thread, JSHandle<JSTaggedValue> UnregisterToken,
                           JSHandle<JSFinalizationRegistry> obj);
    static void CheckAndCall(JSThread *thread);
    static bool CleanupFinalizationRegistry(JSThread *thread, JSHandle<JSFinalizationRegistry> obj);
    static void AddFinRegLists(JSThread *thread, JSHandle<JSFinalizationRegistry> next);
    static void CleanFinRegLists(JSThread *thread, JSHandle<JSFinalizationRegistry> obj);
    static constexpr size_t CLEANUP_CALLBACK_OFFSET = JSObject::SIZE;
    ACCESSORS(CleanupCallback, CLEANUP_CALLBACK_OFFSET, NO_UNREGISTER_OFFSET)
    ACCESSORS(NoUnregister, NO_UNREGISTER_OFFSET, MAYBE_UNREGISTER_OFFSET)
    ACCESSORS(MaybeUnregister, MAYBE_UNREGISTER_OFFSET, NEXT_OFFSET)
    ACCESSORS(Next, NEXT_OFFSET, PREV_OFFSET)
    ACCESSORS(Prev, PREV_OFFSET, SIZE)
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, CLEANUP_CALLBACK_OFFSET, SIZE)
    DECL_DUMP()
};
} // namespace
#endif // ECMASCRIPT_JS_FINALIZATION_REGISTRY_H