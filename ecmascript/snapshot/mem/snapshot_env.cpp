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

#include "ecmascript/snapshot/mem/snapshot_env.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"

namespace panda::ecmascript {
void SnapshotEnv::Initialize()
{
#if ECMASCRIPT_ENABLE_SNAPSHOT
    InitGlobalConst();
    InitGlobalEnv();
#endif
}

void SnapshotEnv::InitGlobalConst()
{
    auto globalConst = const_cast<GlobalEnvConstants *>(vm_->GetJSThread()->GlobalConstants());
    size_t constantCount = globalConst->GetConstantCount();
    for (size_t index = 0; index < constantCount; index++) {
        JSTaggedValue objectValue = globalConst->GetGlobalConstantObject(index);
        if (objectValue.IsHeapObject() && !objectValue.IsString()) {
            TaggedObject *object = objectValue.GetTaggedObject();
            object->GetClass()->SetGlobalConstOrBuiltinsObject(true);
            objectVector_.emplace_back(ToUintPtr(object));
        }
    }
}

void SnapshotEnv::InitGlobalEnv()
{
    auto globalEnv = vm_->GetGlobalEnv();
    CQueue<TaggedObject *> objectQueue;
    std::set<TaggedObject *> objectSet;
    objectQueue.emplace(*globalEnv);
    objectSet.emplace(*globalEnv);
    while (!objectQueue.empty()) {
        auto taggedObject = objectQueue.front();
        if (taggedObject == nullptr) {
            break;
        }
        taggedObject->GetClass()->SetGlobalConstOrBuiltinsObject(true);
        objectVector_.emplace_back(ToUintPtr(taggedObject));
        objectQueue.pop();
        HandleObjectField(taggedObject, &objectQueue, &objectSet);
    }
}

void SnapshotEnv::HandleObjectField(TaggedObject *objectHeader, CQueue<TaggedObject *> *objectQueue,
                                    std::set<TaggedObject *> *objectSet)
{
    auto visitor = [objectQueue, objectSet]([[maybe_unused]]TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                            [[maybe_unused]]bool isNative) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            auto fieldAddr = reinterpret_cast<JSTaggedType *>(slot.SlotAddress());
            JSTaggedValue fieldValue(*fieldAddr);
            if (fieldValue.IsHeapObject() && !fieldValue.IsWeak() && !fieldValue.IsString()
                && objectSet->find(fieldValue.GetTaggedObject()) == objectSet->end()) {
                auto object = fieldValue.GetTaggedObject();
                objectQueue->emplace(object);
                objectSet->emplace(object);
            }
        }
    };
    objXRay_.VisitObjectBody<VisitType::OLD_GC_VISIT>(objectHeader, objectHeader->GetClass(), visitor);
}

void SnapshotEnv::Iterate(const RootVisitor &v)
{
    uint64_t length = objectVector_.size();
    for (uint64_t i = 0; i < length; i++) {
        v(Root::ROOT_VM, ObjectSlot(reinterpret_cast<uintptr_t>(&(objectVector_.data()[i]))));
    }
}
}  // namespace panda::ecmascript

