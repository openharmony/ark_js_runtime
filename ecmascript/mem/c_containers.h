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

#ifndef ECMASCRIPT_MEM_C_CONTAINERS_H
#define ECMASCRIPT_MEM_C_CONTAINERS_H

#include <deque>
#include <list>
#include <stack>
#include <queue>
#include <vector>
#include <set>
#include <string>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <functional>

#include "ecmascript/mem/caddress_allocator.h"

namespace panda::ecmascript {
template<class T>
using CVector = std::vector<T, CAddressAllocator<T>>;

template<class T>
using CList = std::list<T, CAddressAllocator<T>>;

template<class Key, class T, class Compare = std::less<>>
using CMap = std::map<Key, T, Compare, CAddressAllocator<std::pair<const Key, T>>>;

template<class Key, class T, class Compare = std::less<>>
using CMultiMap = std::multimap<Key, T, Compare, CAddressAllocator<std::pair<const Key, T>>>;

template<class Key, class Value, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
using CUnorderedMultiMap =
    std::unordered_multimap<Key, Value, Hash, KeyEqual, CAddressAllocator<std::pair<const Key, Value>>>;

template<class T>
using CDeque = std::deque<T, CAddressAllocator<T>>;

template<class T, class Container = CDeque<T>>
using CQueue = std::queue<T, Container>;

template<class T, class Container = CDeque<T>>
using CStack = std::stack<T, Container>;

template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
using CUnorderedMap = std::unordered_map<Key, T, Hash, KeyEqual, CAddressAllocator<std::pair<const Key, T>>>;

template<class Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
using CUnorderedSet = std::unordered_set<Key, Hash, KeyEqual, CAddressAllocator<Key>>;
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_C_CONTAINERS_H
