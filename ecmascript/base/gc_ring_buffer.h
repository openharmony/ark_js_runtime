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

#ifndef ECMASCRIPT_BASE_GC_RING_BUFFER_H
#define ECMASCRIPT_BASE_GC_RING_BUFFER_H

#include <array>

#include "libpandabase/macros.h"

namespace panda::ecmascript::base {
// GC Ring Buffer is base tool class. It will be used to collect the data of the last N GCs. The max length is
// fixed, so we call it a GC ring buffer. In this class, we define the functions to push data, count the length,
// return the sum and reset data.
template<typename T, int N>
class GCRingBuffer {
public:
    GCRingBuffer() = default;
    ~GCRingBuffer() = default;
    NO_COPY_SEMANTIC(GCRingBuffer);
    NO_MOVE_SEMANTIC(GCRingBuffer);

    void Push(const T &value)
    {
        if (count_ == N) {
            elements_[start_++] = value;
            if (start_ == N) {
                start_ = 0;
            }
        } else {
            ASSERT(start_ == 0);
            elements_[count_++] = value;
        }
    }

    int Count() const
    {
        return count_;
    }

    // This function will return the sum of the elements_. The parameter callback
    // should define the sum function of data type T.
    template<typename Callback>
    T Sum(Callback callback, const T &initial) const
    {
        T result = initial;
        for (int i = 0; i < count_; i++) {
            result = callback(result, elements_[i]);
        }
        return result;
    }

    void Reset()
    {
        start_ = count_ = 0;
    }

private:
    std::array<T, N> elements_;
    int start_ {0};
    int count_ {0};
};
}  // namespace panda::ecmascript::base

#endif  // ECMASCRIPT_BASE_GC_RING_BUFFER_H