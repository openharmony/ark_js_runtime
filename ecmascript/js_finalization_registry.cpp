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

#include "ecmascript/js_finalization_registry.h"

#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/js_function.h"
#include "ecmascript/linked_hash_table.h"
namespace panda::ecmascript {
// -------------------------------CellRecordVector-----------------------------------
JSHandle<CellRecordVector> CellRecordVector::Append(const JSThread *thread, const JSHandle<CellRecordVector> &array,
                                                    const JSHandle<JSTaggedValue> &value)
{
    if (!array->Full()) {
        array->PushBack(thread, value.GetTaggedValue());
        return array;
    }
    // if exist hole, use it.
    uint32_t holeIndex = CheckHole(array);
    if (holeIndex != TaggedArray::MAX_ARRAY_INDEX) {
        array->Set(thread, holeIndex, value.GetTaggedValue());
        return array;
    }
    // the vector is full and no hole exists.
    uint32_t newCapacity = array->GetCapacity() + DEFAULT_GROW_SIZE;
    JSHandle<WeakVector> newArray = WeakVector::Grow(thread, JSHandle<WeakVector>(array), newCapacity);
    [[maybe_unused]] uint32_t arrayIndex = newArray->PushBack(thread, value.GetTaggedValue());
    ASSERT(arrayIndex != TaggedArray::MAX_ARRAY_INDEX);
    return JSHandle<CellRecordVector>(newArray);
}

bool CellRecordVector::IsEmpty()
{
    if (Empty()) {
        return true;
    }
    for (uint32_t i = 0; i < GetEnd(); i++) {
        JSTaggedValue value = Get(i);
        if (value != JSTaggedValue::Hole()) {
            return false;
        }
    }
    return true;
}

uint32_t CellRecordVector::CheckHole(const JSHandle<CellRecordVector> &array)
{
    for (uint32_t i = 0; i < array->GetEnd(); i++) {
        JSTaggedValue value = array->Get(i);
        if (value == JSTaggedValue::Hole()) {
            return i;
        }
    }
    return TaggedArray::MAX_ARRAY_INDEX;
}

// ---------------------------JSFinalizationRegistry-----------------------------------
void JSFinalizationRegistry::Register(JSThread *thread, JSHandle<JSTaggedValue> target,
                                      JSHandle<JSTaggedValue> heldValue,
                                      JSHandle<JSTaggedValue> unregisterToken, JSHandle<JSFinalizationRegistry> obj)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<CellRecord> cellRecord = factory->NewCellRecord();
    cellRecord->SetToWeakRefTarget(target.GetTaggedValue());
    cellRecord->SetHeldValue(thread, heldValue);
    JSHandle<JSTaggedValue> cell(cellRecord);
    // If unregisterToken is undefined, we use vector to store
    // otherwise we use hash map to store to facilitate subsequent delete operations
    if (!unregisterToken->IsUndefined()) {
        JSHandle<LinkedHashMap> maybeUnregister(thread, obj->GetMaybeUnregister());
        JSHandle<CellRecordVector> array(thread, JSTaggedValue::Undefined());
        if (maybeUnregister->Has(unregisterToken.GetTaggedValue())) {
            array = JSHandle<CellRecordVector>(thread, maybeUnregister->Get(unregisterToken.GetTaggedValue()));
        } else {
            array = JSHandle<CellRecordVector>(CellRecordVector::Create(thread));
        }
        array = CellRecordVector::Append(thread, array, cell);
        JSHandle<JSTaggedValue> arrayValue(array);
        maybeUnregister = LinkedHashMap::SetWeakRef(thread, maybeUnregister, unregisterToken, arrayValue);
        obj->SetMaybeUnregister(thread, maybeUnregister);
    } else {
        JSHandle<CellRecordVector> noUnregister(thread, obj->GetNoUnregister());
        noUnregister = CellRecordVector::Append(thread, noUnregister, cell);
        obj->SetNoUnregister(thread, noUnregister);
    }
    JSFinalizationRegistry::AddFinRegLists(thread, obj);
}

bool JSFinalizationRegistry::Unregister(JSThread *thread, JSHandle<JSTaggedValue> UnregisterToken,
                                        JSHandle<JSFinalizationRegistry> obj)
{
    // Because we have stored the object that may be unregistered in the hash map when registering,
    // at this time we just need to find it in the map and delete it
    JSHandle<LinkedHashMap> maybeUnregister(thread, obj->GetMaybeUnregister());
    int entry = maybeUnregister->FindElement(UnregisterToken.GetTaggedValue());
    if (entry == -1) {
        return false;
    }
    maybeUnregister->RemoveEntry(thread, entry);
    JSHandle<LinkedHashMap> newMaybeUnregister = LinkedHashMap::Shrink(thread, maybeUnregister);
    obj->SetMaybeUnregister(thread, newMaybeUnregister);
    return true;
}

void JSFinalizationRegistry::CleanFinRegLists(JSThread *thread, JSHandle<JSFinalizationRegistry> obj)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    if (obj->GetPrev().IsNull() && obj->GetNext().IsNull()) {
        env->SetFinRegLists(thread, JSTaggedValue::Undefined());
        return;
    }
    if (!obj->GetPrev().IsNull()) {
        JSHandle<JSFinalizationRegistry> prev(thread, obj->GetPrev());
        prev->SetNext(obj->GetNext());
    }
    if (!obj->GetNext().IsNull()) {
        JSHandle<JSFinalizationRegistry> next(thread, obj->GetNext());
        next->SetPrev(obj->GetPrev());
    } else {
        env->SetFinRegLists(thread, obj->GetPrev());
    }
    obj->SetPrev(thread, JSTaggedValue::Null());
    obj->SetNext(thread, JSTaggedValue::Null());
}

void JSFinalizationRegistry::CheckAndCall(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> prev = env->GetFinRegLists();

    if (prev->IsUndefined()) {
        return;
    }
    JSMutableHandle<JSTaggedValue> current(thread, prev.GetTaggedValue());
    JSMutableHandle<JSFinalizationRegistry> jsFinalizationRegistry(thread, current.GetTaggedValue());
    while (!current->IsNull()) {
        jsFinalizationRegistry.Update(current.GetTaggedValue());
        current.Update(jsFinalizationRegistry->GetPrev());
        if (CleanupFinalizationRegistry(thread, jsFinalizationRegistry)) {
            // If the objects registered on the current JSFinalizationRegistry object have all been gc,
            // then we clean up this JSFinalizationRegistry object from the FinRegLists
            CleanFinRegLists(thread, jsFinalizationRegistry);
        }
    }
}

void DealCallBackOfMap(JSThread *thread, JSHandle<CellRecordVector> &cellVect,
    JSHandle<job::MicroJobQueue> &job, JSHandle<JSFunction> &func)
{
    if (!cellVect->Empty()) {
        uint32_t cellVectLen = cellVect->GetEnd();
        for (uint32_t i = 0; i < cellVectLen; ++i) {
            JSTaggedValue value = cellVect->Get(i);
            if (value.IsHole()) {
                continue;
            }
            JSHandle<CellRecord> cellRecord(thread, value);
            // if WeakRefTarget have been gc, set callback to job and delete
            if (cellRecord->GetFromWeakRefTarget().IsUndefined()) {
                JSHandle<TaggedArray> argv = thread->GetEcmaVM()->GetFactory()->NewTaggedArray(1);
                argv->Set(thread, 0, cellRecord->GetHeldValue());
                job::MicroJobQueue::EnqueueJob(thread, job, job::QueueType::QUEUE_PROMISE, func, argv);
                cellVect->Delete(thread, i);
            }
        }
    }
}

bool JSFinalizationRegistry::CleanupFinalizationRegistry(JSThread *thread, JSHandle<JSFinalizationRegistry> obj)
{
    // 1. Assert: finalizationRegistry has [[Cells]] and [[CleanupCallback]] internal slots.
    // 2. Let callback be finalizationRegistry.[[CleanupCallback]].
    // 3. While finalizationRegistry.[[Cells]] contains a Record cell such that cell.[[WeakRefTarget]] is empty,
    // an implementation may perform the following steps:
    //     a. Choose any such cell.
    //     b. Remove cell from finalizationRegistry.[[Cells]].
    //     c. Perform ? HostCallJobCallback(callback, undefined, « cell.[[HeldValue]] »).
    // 4. Return unused.
    ASSERT(obj->IsECMAObject());
    auto ecmaVm = thread->GetEcmaVM();
    JSHandle<job::MicroJobQueue> job = ecmaVm->GetMicroJobQueue();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSFunction> func(thread, obj->GetCleanupCallback());
    JSHandle<CellRecordVector> noUnregister(thread, obj->GetNoUnregister());
    if (!noUnregister->Empty()) {
        uint32_t noUnregisterLen = noUnregister->GetEnd();
        for (uint32_t i = 0; i < noUnregisterLen; ++i) {
            JSTaggedValue value = noUnregister->Get(i);
            if (value.IsHole()) {
                continue;
            }
            JSHandle<CellRecord> cellRecord(thread, value);
            // if WeakRefTarget have been gc, set callback to job and delete
            if (cellRecord->GetFromWeakRefTarget().IsUndefined()) {
                JSHandle<TaggedArray> argv = factory->NewTaggedArray(1);
                argv->Set(thread, 0, cellRecord->GetHeldValue());
                job::MicroJobQueue::EnqueueJob(thread, job, job::QueueType::QUEUE_PROMISE, func, argv);
                noUnregister->Delete(thread, i);
            }
        }
    }
    JSMutableHandle<LinkedHashMap> maybeUnregister(thread, obj->GetMaybeUnregister());
    int index = 0;
    int totalElements = maybeUnregister->NumberOfElements() + maybeUnregister->NumberOfDeletedElements();
    JSMutableHandle<JSTaggedValue> key(thread, maybeUnregister->GetKey(index));
    while (index < totalElements) {
        key.Update(maybeUnregister->GetKey(index++));
        if (!key->IsHole()) {
            JSHandle<CellRecordVector> cellVect(thread, maybeUnregister->GetValue(index - 1));
            DealCallBackOfMap(thread, cellVect, job, func);
            if (!cellVect->Empty()) {
                continue;
            }
            maybeUnregister->RemoveEntry(thread, index - 1);
        }
    }
    JSHandle<LinkedHashMap> newMap = LinkedHashMap::Shrink(thread, maybeUnregister);
    obj->SetMaybeUnregister(thread, newMap);
    // Determine whether the objects registered on the current JSFinalizationRegistry object
    // have all been gc
    int remainSize = newMap->NumberOfElements() + newMap->NumberOfDeletedElements();
    if (noUnregister->IsEmpty() && remainSize == 0) {
        return true;
    }
    return false;
}

void JSFinalizationRegistry::AddFinRegLists(JSThread *thread, JSHandle<JSFinalizationRegistry> next)
{
    // If any of prev and next is not null, it means that the current object is already in the linked list,
    // ignore this addition
    if (!next->GetPrev().IsNull() || !next->GetNext().IsNull()) {
        return;
    }
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> lists = env->GetFinRegLists();
    if (!lists->IsUndefined()) {
        JSHandle<JSFinalizationRegistry> prev(lists);
        // Prevent the same object from being connected end to end,
        // which should not happen and will lead to an infinite loop
        if (JSTaggedValue::SameValue(next.GetTaggedValue(), prev.GetTaggedValue())) {
            return;
        }
        prev->SetNext(thread, next);
        next->SetPrev(thread, prev);
    }
    env->SetFinRegLists(thread, next);
}
} // namespace