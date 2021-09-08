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

#ifndef LIBPANDABASE_ECMASCRIPT_MEM_CONTAINERS_H
#define LIBPANDABASE_ECMASCRIPT_MEM_CONTAINERS_H

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

#include "ecmascript/mem/chunk_allocator.h"

namespace panda::ecmascript {
template<typename T>
class ChunkVector : public std::vector<T, ChunkAllocator<T>> {
public:
    explicit ChunkVector(Chunk *chunk) : std::vector<T, ChunkAllocator<T>>(ChunkAllocator<T>(chunk)) {}

    ChunkVector(size_t size, Chunk *chunk) : std::vector<T, ChunkAllocator<T>>(size, T(), ChunkAllocator<T>(chunk)) {}

    ChunkVector(size_t size, T def, Chunk *chunk)
        : std::vector<T, ChunkAllocator<T>>(size, def, ChunkAllocator<T>(chunk))
    {
    }
    ~ChunkVector() = default;
    NO_COPY_SEMANTIC(ChunkVector);
    NO_MOVE_SEMANTIC(ChunkVector);
};

template<typename K, typename V, typename Compare = std::less<K>>
class ChunkMap : public std::map<K, V, Compare, ChunkAllocator<std::pair<const K, V>>> {
public:
    // Constructs an empty map.
    explicit ChunkMap(Chunk *chunk)
        : std::map<K, V, Compare, ChunkAllocator<std::pair<const K, V>>>(Compare(),
                                                                         ChunkAllocator<std::pair<const K, V>>(chunk))
    {
    }
    ~ChunkMap() = default;
    NO_COPY_SEMANTIC(ChunkMap);
    NO_MOVE_SEMANTIC(ChunkMap);
};

template<typename K, typename V, typename Hash = std::hash<K>, typename KeyEqual = std::equal_to<K>>
class ChunkUnorderedMap : public std::unordered_map<K, V, Hash, KeyEqual, ChunkAllocator<std::pair<const K, V>>> {
public:
    // NOLINTNEXTLINE(readability-magic-numbers)
    explicit ChunkUnorderedMap(Chunk *chunk, size_t bucket_count = 100)
        : std::unordered_map<K, V, Hash, KeyEqual, ChunkAllocator<std::pair<const K, V>>>(
              bucket_count, Hash(), KeyEqual(), ChunkAllocator<std::pair<const K, V>>(chunk))
    {
    }
    ~ChunkUnorderedMap() = default;
    NO_COPY_SEMANTIC(ChunkUnorderedMap);
    NO_MOVE_SEMANTIC(ChunkUnorderedMap);
};

template<typename K, typename V, typename Compare = std::less<K>>
class ChunkMultimap : public std::multimap<K, V, Compare, ChunkAllocator<std::pair<const K, V>>> {
public:
    // Constructs an empty multimap.
    explicit ChunkMultimap(Chunk *chunk)
        : std::multimap<K, V, Compare, ChunkAllocator<std::pair<const K, V>>>(
              Compare(), ChunkAllocator<std::pair<const K, V>>(chunk))
    {
    }
    ~ChunkMultimap() = default;
    NO_COPY_SEMANTIC(ChunkMultimap);
    NO_MOVE_SEMANTIC(ChunkMultimap);
};
}  // namespace panda::ecmascript

#endif  // LIBPANDABASE_ECMASCRIPT_MEM_CONTAINERS_H
