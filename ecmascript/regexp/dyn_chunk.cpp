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

#include "ecmascript/regexp/dyn_chunk.h"
#include "securec.h"

namespace panda::ecmascript {
int DynChunk::Expand(size_t newSize)
{
    if (newSize > allocatedSize_) {
        if (error_) {
            return FAILURE;
        }
        ASSERT(allocatedSize_ <= std::numeric_limits<size_t>::max() / ALLOCATE_MULTIPLIER);
        size_t size = allocatedSize_ * ALLOCATE_MULTIPLIER;
        if (size > newSize) {
            newSize = size;
        }
        newSize = std::max(newSize, ALLOCATE_MIN_SIZE);
        auto *newBuf = chunk_->NewArray<uint8_t>(newSize);
        if (newBuf == nullptr) {
            error_ = true;
            return FAILURE;
        }
        if (memset_s(newBuf, newSize, 0, newSize) != EOK) {
            error_ = true;
            return FAILURE;
        }
        if (buf_ != nullptr) {
            if (memcpy_s(newBuf, size_, buf_, size_) != EOK) {
                error_ = true;
                return FAILURE;
            }
        }
        buf_ = newBuf;
        allocatedSize_ = newSize;
    }
    return SUCCESS;
}

int DynChunk::Insert(uint32_t position, size_t len)
{
    if (size_ < position) {
        return FAILURE;
    }
    if (Expand(size_ + len) != 0) {
        return FAILURE;
    }
    size_t moveSize = size_ - position;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    if (memmove_s(buf_ + position + len, moveSize, buf_ + position, moveSize) != EOK) {
        return FAILURE;
    }
    size_ += len;
    return SUCCESS;
}

int DynChunk::Emit(const uint8_t *data, size_t length)
{
    if (UNLIKELY((size_ + length) > allocatedSize_)) {
        if (Expand(size_ + length) != 0) {
            return FAILURE;
        }
    }

    if (memcpy_s(buf_ + size_,  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        length, data, length) != EOK) {
        return FAILURE;
    }
    size_ += length;
    return SUCCESS;
}

int DynChunk::EmitChar(uint8_t c)
{
    return Emit(&c, 1);
}

int DynChunk::EmitSelf(size_t offset, size_t length)
{
    if (UNLIKELY((size_ + length) > allocatedSize_)) {
        if (Expand(size_ + length) != 0) {
            return FAILURE;
        }
    }

    if (memcpy_s(buf_ + size_,  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        length,
        buf_ + offset,  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        length) != EOK) {
        return FAILURE;
    }
    size_ += length;
    return SUCCESS;
}

int DynChunk::EmitStr(const char *str)
{
    return Emit(reinterpret_cast<const uint8_t *>(str), strlen(str) + 1);
}
}  // namespace panda::ecmascript
