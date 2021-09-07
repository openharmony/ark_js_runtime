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

#include "mem_controller.h"

namespace panda::ecmascript {
MemController::MemController(bool isInAppStartup) : isInAppStartup_(isInAppStartup) {}

double MemController::CalculateAllocLimit(size_t currentSize, size_t minSize, size_t maxSize, size_t newSpaceCapacity,
                                          double factor) const
{
    const uint64_t limit = std::max(static_cast<uint64_t>(currentSize * factor),
                                    static_cast<uint64_t>(currentSize) + MIN_AllOC_LIMIT_GROWING_STEP) +
                           SEMI_SPACE_SIZE_CAPACITY;

    const uint64_t limitAboveMinSize = std::max<uint64_t>(limit, minSize);
    const uint64_t halfToMaxSize = (static_cast<uint64_t>(currentSize) + maxSize) / 2;
    const auto result = static_cast<size_t>(std::min(limitAboveMinSize, halfToMaxSize));
    return result;
}

MemController *CreateMemController(std::string_view gcTriggerType)
{
    auto triggerType = GCTriggerType::INVALID_TRIGGER;
    if (gcTriggerType == "no-gc-for-start-up") {
        triggerType = GCTriggerType::NO_GC_FOR_STARTUP;
    } else if (gcTriggerType == "heap-trigger") {
        triggerType = GCTriggerType::HEAP_TRIGGER;
    }
    MemController *ret{nullptr};
    switch (triggerType) {  // NOLINT(hicpp-multiway-paths-covered)
        case GCTriggerType::NO_GC_FOR_STARTUP:
            ret = new MemController(true);
            break;
        case GCTriggerType::HEAP_TRIGGER:
            ret = new MemController(false);
            break;
        default:
            break;
    }
    return ret;
}
}  // namespace panda::ecmascript
