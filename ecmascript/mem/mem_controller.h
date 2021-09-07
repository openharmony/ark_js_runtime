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

#ifndef ECMASCRIPT_MEM_MEM_CONTROLLER_H
#define ECMASCRIPT_MEM_MEM_CONTROLLER_H

#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {
constexpr static int MILLISECONDS_PER_SECOND = 1000;

enum class GCTriggerType {
    INVALID_TRIGGER,
    NO_GC_FOR_STARTUP,  // guarantee no gc for app startup
    HEAP_TRIGGER,
};

using BytesAndDuration = std::pair<uint64_t, double>;

inline BytesAndDuration MakeBytesAndDuration(uint64_t bytes, double duration)
{
    return std::make_pair(bytes, duration);
}

class MemController {
public:
    explicit MemController(bool isInAppStartUp = false);
    MemController() = default;
    ~MemController() = default;
    NO_COPY_SEMANTIC(MemController);
    NO_MOVE_SEMANTIC(MemController);

    double CalculateAllocLimit(size_t currentSize, size_t minSize, size_t maxSize, size_t newSpaceCapacity,
                               double factor) const;

    inline bool IsInAppStartup() const
    {
        return isInAppStartup_;
    }

    void ResetAppStartup()
    {
        isInAppStartup_ = false;
    }

private:
    bool isInAppStartup_{false};
};

MemController *CreateMemController(std::string_view gcTriggerType);
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_MEM_CONTROLLER_H
