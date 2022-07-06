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

#ifndef ECMASCRIPT_SHARED_MEMORY_MANAGER_MANAGER_H
#define ECMASCRIPT_SHARED_MEMORY_MANAGER_MANAGER_H

#include "ecmascript/mem/c_containers.h"
#include "os/mutex.h"

namespace panda {
class EcmaVm;
namespace ecmascript {
class JSSharedMemoryManager {
public:
    PUBLIC_API ~JSSharedMemoryManager();

    static JSSharedMemoryManager *GetInstance()
    {
        static JSSharedMemoryManager jsSharedMemoryManager;
        return &jsSharedMemoryManager;
    }
    void CreateOrLoad(void **pointer, size_t size);
    static void RemoveSharedMemory(void *pointer, void *data);
private:
    JSSharedMemoryManager() = default;

    void InsertSharedMemory(const void *pointer);
    void IncreaseRefSharedMemory(const void *pointer);
    void DecreaseRefSharedMemory(const void *pointer);
    void FreeBuffer(void *mem);
    void *AllocateBuffer(size_t size);
    os::memory::RecursiveMutex jsSharedMemoryLock_;
    CMap<const uint64_t, int32_t> loadedJSSharedMemory_;
};
}  // namespace ecmascript
}  // namespace panda
#endif // ECMASCRIPT_SHARED_MEMORY_MANAGER_MANAGER_H
