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

#ifndef ECMASCRIPT_WAITER_LIST_H
#define ECMASCRIPT_WAITER_LIST_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/mem/c_containers.h"
#include "os/mutex.h"

namespace panda::ecmascript {
using Mutex = os::memory::Mutex;
using LockHolder = os::memory::LockHolder<Mutex>;

class WaiterListNode {
public:
    WaiterListNode() = default;
    ~WaiterListNode() = default;

public:
    NO_COPY_SEMANTIC(WaiterListNode);
    NO_MOVE_SEMANTIC(WaiterListNode);

    WaiterListNode *prev_ {nullptr};
    WaiterListNode *next_ {nullptr};
    // Used to call wait or Signal() to unlock wait and wake up
    os::memory::ConditionVariable cond_;

    // Managed Arraybuffer or SharedArrayBuffer memory data
    void *date_ {nullptr};

    // the offset of the element in the typedArray
    size_t index_ {0};

    // the memory address data corresponding to the offset
    int8_t *waitPointer_ {nullptr};

    // used to determine whether to wait, start wait when waiting_ is true
    bool waiting_ {false};

private:
    friend class WaiterList;
};

// WaiterList to manage WaiterListNode
class WaiterList {
public:
    WaiterList() = default;
    void AddNode(WaiterListNode *node);
    void DeleteNode(WaiterListNode *node);
    struct HeadAndTail {
        WaiterListNode *pHead {nullptr};
        WaiterListNode *pTail {nullptr};
    };
    
    // locationListMap_  is used AddNode or DeleteNode
    // When calling addnode If there is no corresponding memory data, add the node corresponding to the key
    CMap<int8_t *, HeadAndTail> locationListMap_;
};

// The Singleton pattern is used to creat a global metux and WaiterList
template <class T>
class Singleton {
public:
    ~Singleton() {}
    NO_COPY_SEMANTIC(Singleton);
    NO_MOVE_SEMANTIC(Singleton);
    static T *GetInstance()
    {
        static T instance;
        return &instance;
    }

private:
    Singleton() = default;
};

class SCOPED_CAPABILITY MutexGuard
{
public:
    explicit MutexGuard(Mutex *mutex) : mutex_(mutex), lockHolder_(*mutex) {}
    void Unlock() RELEASE()
    {
        mutex_->Unlock();
    }
    void Lock() ACQUIRE()
    {
        mutex_->Lock();
    }

public:
    Mutex *mutex_;
    LockHolder lockHolder_;
};
}  // namespace
#endif  // ECMASCRIPT_WAITER_LIST_H