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

#include "ecmascript/sharedMemoryManaged/sharedmemorymanager.h"

namespace panda::ecmascript {
static constexpr size_t MALLOC_SIZE_LIMIT = 2147483648; // Max internal memory used by the VM declared in options

JSSharedMemoryManager::~JSSharedMemoryManager()
{
    os::memory::LockHolder lock(jsSharedMemoryLock_);
    auto iter = loadedJSSharedMemory_.begin();
    while (iter != loadedJSSharedMemory_.end()) {
        const void *pointer = ToVoidPtr(iter->first);
        FreeBuffer(const_cast<void *>(pointer));
        iter = loadedJSSharedMemory_.erase(iter);
    }
}

void JSSharedMemoryManager::CreateOrLoad(void **pointer, size_t size)
{
    if (*pointer != nullptr) {
        if (loadedJSSharedMemory_.find((uint64_t)*pointer) != loadedJSSharedMemory_.end()) {
            IncreaseRefSharedMemory(*pointer);
        }
        return;
    }
    *pointer = AllocateBuffer(size);
    InsertSharedMemory(*pointer);
}

void JSSharedMemoryManager::InsertSharedMemory(const void *pointer)
{
    os::memory::LockHolder lock(jsSharedMemoryLock_);
    if (loadedJSSharedMemory_.find((uint64_t)pointer) == loadedJSSharedMemory_.end()) {
        loadedJSSharedMemory_[(uint64_t)pointer] = 1;
    }
}

void JSSharedMemoryManager::IncreaseRefSharedMemory(const void *pointer)
{
    os::memory::LockHolder lock(jsSharedMemoryLock_);
    if (loadedJSSharedMemory_.find((uint64_t)pointer) != loadedJSSharedMemory_.end()) {
        loadedJSSharedMemory_[(uint64_t)pointer]++;
    }
}

void JSSharedMemoryManager::DecreaseRefSharedMemory(const void *pointer)
{
    os::memory::LockHolder lock(jsSharedMemoryLock_);
    auto iter = loadedJSSharedMemory_.find((uint64_t)pointer);
    if (iter != loadedJSSharedMemory_.end()) {
        if (iter->second > 1) {
            iter->second--;
            return;
        }
        loadedJSSharedMemory_.erase(iter);
        FreeBuffer(const_cast<void *>(pointer));
    }
}

void *JSSharedMemoryManager::AllocateBuffer(size_t size)
{
    if (size == 0) {
        LOG_ECMA_MEM(FATAL) << "size must have a size bigger than 0";
        UNREACHABLE();
    }
    if (size >= MALLOC_SIZE_LIMIT) {
        LOG_ECMA_MEM(FATAL) << "size must be less than the maximum";
        UNREACHABLE();
    }
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    void *ptr = malloc(size);
    if (ptr == nullptr) {
        LOG_ECMA_MEM(FATAL) << "malloc failed";
        UNREACHABLE();
    }
#if ECMASCRIPT_ENABLE_ZAP_MEM
    if (memset_s(ptr, size, INVALID_VALUE, size) != EOK) {
        LOG_ECMA_MEM(FATAL) << "memset failed";
        UNREACHABLE();
    }
#endif
    return ptr;
}

void JSSharedMemoryManager::FreeBuffer(void *mem)
{
    if (mem == nullptr) {
        return;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-no-malloc)
    free(mem);
}

void JSSharedMemoryManager::RemoveSharedMemory(void *pointer, void *data)
{
    if (pointer == nullptr || data == nullptr) {
        return;
    }
    // dec ref in menorymanager
    JSSharedMemoryManager *jsSharedMemoryManager = static_cast<JSSharedMemoryManager *>(data);
    jsSharedMemoryManager->DecreaseRefSharedMemory(pointer);
}
} // namespace panda::ecmascript
