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

#ifndef PANDA_RUNTIME_ECMASCRIPT_HEAPMANAGER_HELPER_H
#define PANDA_RUNTIME_ECMASCRIPT_HEAPMANAGER_HELPER_H

namespace panda::ecmascript {
class EcmaHeapManagerHelper {
public:
    explicit EcmaHeapManagerHelper(panda::mem::HeapManager *heap, JSThread *thread)
        : heapManager_(heap), thread_(thread)
    {
    }

    TaggedObject *Create(JSHClass *hclass)
    {
        return CreateObject(hclass, false);
    }

    TaggedObject *CreateNonMovable(JSHClass *hclass)
    {
        return CreateObject(hclass, true);
    }

    TaggedObject *CreateObject(JSHClass *hclass, size_t size, bool nonMovable)
    {
        ASSERT(size != 0);
        TaggedObject *obj{nullptr};
        if (LIKELY(!nonMovable)) {
            obj = heapManager_->AllocateObject(hclass, size, DEFAULT_ALIGNMENT, thread_);
        } else {
            obj = heapManager_->AllocateNonMovableObject(hclass, size, DEFAULT_ALIGNMENT, thread_);
        }
        return obj;
    }

    TaggedObject *CreateObject(JSHClass *hclass, bool nonMovable)
    {
        size_t size = hclass->GetObjectSize();
        return CreateObject(hclass, size, nonMovable);
    }

    TaggedObject *AllocateNonMovableObject(JSHClass *cls, size_t size)
    {
        return heapManager_->AllocateNonMovableObject(cls, size);
    }
    ~EcmaHeapManagerHelper() = default;
private:
    panda::mem::HeapManager *heapManager_{nullptr};
    JSThread *thread_;
};
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_HEAPMANAGER_HELPER_H
