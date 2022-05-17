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
#ifndef ECMASCRIPT_COMPILER_ASSEMBLER_H
#define ECMASCRIPT_COMPILER_ASSEMBLER_H

#include "ecmascript/mem/dyn_chunk.h"

namespace panda::ecmascript {
class GCStackMapRegisters {
public:
#if defined(PANDA_TARGET_AMD64)
    static constexpr int SP = 7;
    static constexpr int FP = 6;
#elif defined(PANDA_TARGET_ARM64)
    static constexpr int SP = 31;  /* x31 */
    static constexpr int FP = 29;  /* x29 */
#elif defined(PANDA_TARGET_ARM32)
    static constexpr int SP = 13;
    static constexpr int FP = 11;
#else
    static constexpr int SP = -1;
    static constexpr int FP = -1;
#endif
};

enum Distance {
    Near,
    Far
};

class Label {
public:
    bool IsBound() const
    {
        return pos_ > 0;
    }

    bool IsLinked() const
    {
        return pos_ < 0;
    }

    bool IsLinkedNear() const
    {
        return nearPos_ > 0;
    }

    uint32_t GetPos() const
    {
        return static_cast<uint32_t>(pos_ - 1);
    }

    uint32_t GetLinkedPos() const
    {
        ASSERT(!IsBound());
        return static_cast<uint32_t>(-pos_ - 1);
    }

    void BindTo(int32_t pos)
    {
        // +1 skip offset 0
        pos_ = pos + 1;
    }

    void LinkTo(int32_t pos)
    {
        // +1 skip offset 0
        pos_ = - (pos + 1);
    }

    void UnlinkNearPos()
    {
        nearPos_ = 0;
    }

    void LinkNearPos(uint32_t pos)
    {
        // +1 skip offset 0
        nearPos_ = pos + 1;
    }

    uint32_t GetLinkedNearPos() const
    {
        ASSERT(!IsBound());
        return static_cast<uint32_t>(nearPos_ - 1);
    }

private:
    int32_t pos_ = 0;
    uint32_t nearPos_ = 0;
};

class Assembler {
public:
    explicit Assembler(Chunk *chunk)
        : buffer_(chunk)
    {
    }

    void EmitU8(uint8_t v)
    {
        buffer_.EmitChar(v);
    }

    void EmitI8(int8_t v)
    {
        buffer_.EmitChar(static_cast<uint8_t>(v));
    }

    void EmitU16(uint16_t v)
    {
        buffer_.EmitU16(v);
    }

    void EmitU32(uint32_t v)
    {
        buffer_.EmitU32(v);
    }

    void EmitI32(int32_t v)
    {
        buffer_.EmitU32(static_cast<uint32_t>(v));
    }

    void EmitU64(uint64_t v)
    {
        buffer_.EmitU64(v);
    }

    void PutI8(size_t offset, int8_t data)
    {
        buffer_.PutU8(offset, static_cast<int8_t>(data));
    }

    void PutI32(size_t offset, int32_t data)
    {
        buffer_.PutU32(offset, static_cast<int32_t>(data));
    }

    uint32_t GetU32(size_t offset) const
    {
        return buffer_.GetU32(offset);
    }

    int8_t GetI8(size_t offset) const
    {
        return static_cast<int8_t>(buffer_.GetU8(offset));
    }

    uint8_t GetU8(size_t offset) const
    {
        return buffer_.GetU8(offset);
    }

    size_t GetCurrentPosition() const
    {
        return buffer_.GetSize();
    }

    uint8_t *GetBegin() const
    {
        return buffer_.GetBegin();
    }

    static bool InRangeN(int32_t x, uint32_t n)
    {
        int32_t limit = 1 << (n - 1);
        return (x >= -limit) && (x < limit);
    }

    static bool InRange8(int32_t x)
    {
        // 8: range8
        return InRangeN(x, 8);
    }
private:
    DynChunk buffer_;
};
}  // panda::ecmascript
#endif  // ECMASCRIPT_COMPILER_ASSEMBLER_H
