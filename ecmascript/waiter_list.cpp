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

#include "ecmascript/waiter_list.h"
#include "ecmascript/base/number_helper.h"
#include "os/time.h"

namespace panda::ecmascript {
// --------------------------WaiterList------------------------------
void WaiterList::AddNode(WaiterListNode *node)
{
    ASSERT(node->prev_ == nullptr);
    ASSERT(node->next_ == nullptr);
    auto iter = locationListMap_.find(node->waitPointer_);
    if (iter != locationListMap_.end()) {
        iter->second.pTail->next_ = node;
        node->prev_ = iter->second.pTail;
        iter->second.pTail = node;
    } else {
        locationListMap_.insert(std::make_pair(node->waitPointer_, HeadAndTail {node, node}));
    }
}

void WaiterList::DeleteNode(WaiterListNode *node)
{
    auto iter = locationListMap_.find(node->waitPointer_);
    ASSERT(iter != locationListMap_.end());
    WaiterListNode *temp = iter->second.pHead;
    [[maybe_unused]] bool flag = false;
    while (temp != nullptr) {
        if (temp == node) {
            flag = true;
            break;
        }
        temp = temp->next_;
    }
    ASSERT(flag);
    if (node == iter->second.pHead && node == iter->second.pTail) {
        locationListMap_.erase(iter);
        return;
    }

    if (node == iter->second.pHead) {
        iter->second.pHead = node->next_;
    } else {
        ASSERT(node->prev_);
        node->prev_->next_ = node->next_;
    }

    if (node == iter->second.pTail) {
        iter->second.pTail = node->prev_;
    } else {
        ASSERT(node->next_);
        node->next_->prev_ = node->prev_;
    }

    node->prev_ = node->next_ = nullptr;
}
}  // namespace
