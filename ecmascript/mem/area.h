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

#ifndef ECMASCRIPT_MEM_AREA_H
#define ECMASCRIPT_MEM_AREA_H

namespace panda::ecmascript {
class Area {
public:
    Area(uintptr_t begin, size_t capacity)
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        : begin_(begin), end_(begin + capacity), next_(nullptr), prev_(nullptr)
    {
    }
    ~Area() = default;
    NO_COPY_SEMANTIC(Area);
    NO_MOVE_SEMANTIC(Area);

    uintptr_t GetBegin() const
    {
        return begin_;
    }

    uintptr_t GetEnd() const
    {
        return end_;
    }

    void LinkPrev(Area *prev)
    {
        prev_ = prev;
    }

    void LinkNext(Area *next)
    {
        next_ = next;
    }

    size_t GetSize()
    {
        return end_ - begin_;
    }

private:
    template<class T>
    friend class EcmaList;
    friend class Worker;
    Area *GetPrev() const
    {
        return prev_;
    }

    Area *GetNext() const
    {
        return next_;
    }
    uintptr_t begin_;
    uintptr_t end_;
    Area *next_;
    Area *prev_;
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_AREA_H
