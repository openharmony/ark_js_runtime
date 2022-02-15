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

#ifndef ECMASCRIPT_MEM_UTILS_H
#define ECMASCRIPT_MEM_UTILS_H

#include "ecmascript/ecma_macros.h"
#include "securec.h"

namespace panda::ecmascript {
class Utils {
public:
    static ARK_INLINE void Copy(void *dest, size_t destCount, void *src, size_t count)
    {
        switch (count) {
#define COPY_BY_CONST(destCount, value)                              \
            case value:                                              \
                if (memcpy_sp(dest, destCount, src, value) != EOK) { \
                    LOG_ECMA(FATAL) << "memcpy_s failed";            \
                }                                                    \
                break;
            COPY_BY_CONST(destCount, 16)
            COPY_BY_CONST(destCount, 24)
            COPY_BY_CONST(destCount, 32)
            COPY_BY_CONST(destCount, 40)
            COPY_BY_CONST(destCount, 48)
            COPY_BY_CONST(destCount, 56)
            COPY_BY_CONST(destCount, 64)
            COPY_BY_CONST(destCount, 72)
            COPY_BY_CONST(destCount, 80)
            COPY_BY_CONST(destCount, 88)
            COPY_BY_CONST(destCount, 96)
            COPY_BY_CONST(destCount, 104)
            COPY_BY_CONST(destCount, 112)
            COPY_BY_CONST(destCount, 120)
            COPY_BY_CONST(destCount, 128)
#undef COPY_BY_CONST
            default:
                if (memcpy_s(dest, destCount, src, count) != EOK) {
                    LOG_ECMA(FATAL) << "memcpy_s failed";
                }
        }
    }
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_UTILS_H
