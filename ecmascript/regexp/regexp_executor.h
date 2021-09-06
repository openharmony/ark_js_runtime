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

#ifndef PANDA_RUNTIME_ECMASCRIPT_REGEXP_EXECUTOR_H
#define PANDA_RUNTIME_ECMASCRIPT_REGEXP_EXECUTOR_H

#include "ecmascript/regexp/regexp_parser.h"
#include "ecmascript/mem/chunk.h"

namespace panda::ecmascript {
class RegExpExecutor {
public:
    struct CaptureState {
        const uint8_t *captureStart;
        const uint8_t *captureEnd;
    };

    enum StateType : uint8_t {
        STATE_SPLIT = 0,
        STATE_MATCH_AHEAD,
        STATE_NEGATIVE_MATCH_AHEAD,
    };

    struct RegExpState {
        StateType type_ = STATE_SPLIT;
        uint32_t currentPc_ = 0;
        uint32_t currentStack_ = 0;
        const uint8_t *currentPtr_ = nullptr;
        __extension__ CaptureState *captureResultList_[0];  // NOLINT(modernize-avoid-c-arrays)
    };

    struct MatchResult {
        uint32_t endIndex_ = 0;
        uint32_t index_ = 0;
        // first value is true if result is undefined
        std::vector<std::pair<bool, JSHandle<EcmaString>>> captures_;
        bool isSuccess_ = false;
    };

    explicit RegExpExecutor(Chunk *chunk) : chunk_(chunk)
    {
        ASSERT(chunk_ != nullptr);
    };

    ~RegExpExecutor() = default;

    NO_COPY_SEMANTIC(RegExpExecutor);
    NO_MOVE_SEMANTIC(RegExpExecutor);

    bool Execute(const uint8_t *input, uint32_t lastIndex, uint32_t length, uint8_t *buf, bool isWideChar = false);

    bool ExecuteInternal(const DynChunk &byteCode, uint32_t pcEnd);
    bool HandleFirstSplit();
    bool HandleOpAll(uint8_t opCode);
    bool HandleOpChar(const DynChunk &byteCode, uint8_t opCode);
    bool HandleOpWordBoundary(uint8_t opCode);
    bool HandleOpLineStart(uint8_t opCode);
    bool HandleOpLineEnd(uint8_t opCode);
    void HandleOpSaveStart(const DynChunk &byteCode, uint8_t opCode);
    void HandleOpSaveEnd(const DynChunk &byteCode, uint8_t opCode);
    void HandleOpSaveReset(const DynChunk &byteCode, uint8_t opCode);
    void HandleOpMatch(const DynChunk &byteCode, uint8_t opCode);
    void HandleOpSplitFirst(const DynChunk &byteCode, uint8_t opCode);
    bool HandleOpPrev(uint8_t opCode);
    void HandleOpLoop(const DynChunk &byteCode, uint8_t opCode);
    bool HandleOpRange32(const DynChunk &byteCode);
    bool HandleOpRange(const DynChunk &byteCode);
    bool HandleOpBackReference(const DynChunk &byteCode, uint8_t opCode);

    inline void Advance(uint8_t opCode, uint32_t offset = 0)
    {
        currentPc_ += offset + RegExpOpCode::GetRegExpOpCode(opCode)->GetSize();
    }

    inline void AdvanceOffset(uint32_t offset)
    {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        currentPc_ += offset;
    }

    inline uint32_t GetCurrentChar()
    {
        return GetChar(&currentPtr_, inputEnd_);
    }

    inline void AdvanceCurrentPtr()
    {
        AdvancePtr(&currentPtr_, inputEnd_);
    }

    uint32_t GetChar(const uint8_t **pp, const uint8_t *end) const
    {
        uint32_t c;
        const uint8_t *cptr = *pp;
        if (!isWideChar_) {
            c = *cptr;
            *pp += 1;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        } else {
            uint16_t c1 = *(uint16_t *)cptr;  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
            c = c1;
            cptr += WIDE_CHAR_SIZE;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (U16_IS_LEAD(c) && IsUtf16() && cptr < end) {
                c1 = *(uint16_t *)cptr;  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
                if (U16_IS_TRAIL(c1)) {
                    c = U16_GET_SUPPLEMENTARY(c, c1);  // NOLINTNEXTLINE(hicpp-signed-bitwise)
                    cptr += WIDE_CHAR_SIZE;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                }
            }
            *pp = cptr;
        }
        return c;
    }

    uint32_t PeekChar(const uint8_t *p, const uint8_t *end) const
    {
        uint32_t c;
        const uint8_t *cptr = p;
        if (!isWideChar_) {
            c = *cptr;
        } else {
            uint16_t c1 = *(uint16_t *)cptr;  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
            c = c1;
            cptr += WIDE_CHAR_SIZE;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (U16_IS_LEAD(c) && IsUtf16() && cptr < end) {
                c1 = *(uint16_t *)cptr;  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
                if (U16_IS_TRAIL(c1)) {
                    c = U16_GET_SUPPLEMENTARY(c, c1);  // NOLINTNEXTLINE(hicpp-signed-bitwise)
                    cptr += WIDE_CHAR_SIZE;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                }
            }
        }
        return c;
    }

    void AdvancePtr(const uint8_t **pp, const uint8_t *end) const
    {
        const uint8_t *cptr = *pp;
        if (!isWideChar_) {
            *pp += 1;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        } else {
            uint16_t c1 = *(uint16_t *)cptr;  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
            cptr += WIDE_CHAR_SIZE;           // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (U16_IS_LEAD(c1) && IsUtf16() && cptr < end) {
                c1 = *(uint16_t *)cptr;  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
                if (U16_IS_TRAIL(c1)) {
                    cptr += WIDE_CHAR_SIZE;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                }
            }
            *pp = cptr;
        }
    }

    uint32_t PeekPrevChar(const uint8_t *p, const uint8_t *start) const
    {
        uint32_t c;
        const uint8_t *cptr = p;
        if (!isWideChar_) {
            c = *(cptr - 1);  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            cptr -= 1;        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        } else {
            cptr -= WIDE_CHAR_SIZE;           // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            uint16_t c1 = *(uint16_t *)cptr;  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
            c = c1;
            if (U16_IS_TRAIL(c) && IsUtf16() && cptr > start) {
                c1 = ((uint16_t *)cptr)[-1];  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                if (U16_IS_LEAD(c1)) {
                    c = U16_GET_SUPPLEMENTARY(c1, c);  // NOLINTNEXTLINE(hicpp-signed-bitwise)
                    cptr -= WIDE_CHAR_SIZE;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                }
            }
        }
        return c;
    }

    uint32_t GetPrevChar(const uint8_t **pp, const uint8_t *start) const
    {
        uint32_t c;
        const uint8_t *cptr = *pp;
        if (!isWideChar_) {
            c = *(cptr - 1);  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            cptr -= 1;        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            *pp = cptr;
        } else {
            cptr -= WIDE_CHAR_SIZE;           // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            uint16_t c1 = *(uint16_t *)cptr;  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
            c = c1;
            if (U16_IS_TRAIL(c) && IsUtf16() && cptr > start) {
                c1 = ((uint16_t *)cptr)[-1];  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                if (U16_IS_LEAD(c1)) {
                    c = U16_GET_SUPPLEMENTARY(c1, c);  // NOLINTNEXTLINE(hicpp-signed-bitwise)
                    cptr -= WIDE_CHAR_SIZE;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                }
            }
            *pp = cptr;
        }
        return c;
    }

    void PrevPtr(const uint8_t **pp, const uint8_t *start) const
    {
        const uint8_t *cptr = *pp;
        if (!isWideChar_) {
            cptr -= 1;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            *pp = cptr;
        } else {
            cptr -= WIDE_CHAR_SIZE;           // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            uint16_t c1 = *(uint16_t *)cptr;  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-cstyle-cast)
            if (U16_IS_TRAIL(c1) && IsUtf16() && cptr > start) {
                c1 = ((uint16_t *)cptr)[-1];  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                if (U16_IS_LEAD(c1)) {
                    cptr -= WIDE_CHAR_SIZE;  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
                }
            }
            *pp = cptr;
        }
    }

    bool MatchFailed(bool isMatched = false);

    void SetCurrentPC(uint32_t pc)
    {
        currentPc_ = pc;
    }

    void SetCurrentPtr(const uint8_t *ptr)
    {
        currentPtr_ = ptr;
    }

    bool IsEOF() const
    {
        return currentPtr_ >= inputEnd_;
    }

    uint32_t GetCurrentPC() const
    {
        return currentPc_;
    }

    void PushStack(uintptr_t val)
    {
        ASSERT(currentStack_ < nStack_);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        stack_[currentStack_++] = val;
    }

    void SetStackValue(uintptr_t val) const
    {
        ASSERT(currentStack_ >= 1);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        stack_[currentStack_ - 1] = val;
    }

    uintptr_t PopStack()
    {
        ASSERT(currentStack_ >= 1);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return stack_[--currentStack_];
    }

    uintptr_t PeekStack() const
    {
        ASSERT(currentStack_ >= 1);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        return stack_[currentStack_ - 1];
    }

    const uint8_t *GetCurrentPtr() const
    {
        return currentPtr_;
    }

    CaptureState *GetCaptureResultList() const
    {
        return captureResultList_;
    }

    void DumpResult(std::ostream &out) const;

    MatchResult GetResult(const JSThread *thread, bool isSuccess) const;

    void PushRegExpState(StateType type, uint32_t pc);

    RegExpState *PopRegExpState(bool copyCaptrue = true);

    void DropRegExpState()
    {
        stateStackLen_--;
    }

    RegExpState *PeekRegExpState() const
    {
        ASSERT(stateStackLen_ >= 1);
        return reinterpret_cast<RegExpState *>(
            stateStack_ +  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            (stateStackLen_ - 1) * stateSize_);
    }

    void ReAllocStack(uint32_t stackLen);

    inline bool IsWordChar(uint8_t value) const
    {
        return ((value >= '0' && value <= '9') || (value >= 'a' && value <= 'z') || (value >= 'A' && value <= 'Z') ||
                (value == '_'));
    }

    inline bool IsTerminator(uint32_t value) const
    {
        // NOLINTNEXTLINE(readability-magic-numbers)
        return (value == '\n' || value == '\r' || value == 0x2028 || value == 0x2029);
    }

    inline bool IsIgnoreCase() const
    {
        return (flags_ & RegExpParser::FLAG_IGNORECASE) != 0;
    }

    inline bool IsUtf16() const
    {
        return (flags_ & RegExpParser::FLAG_UTF16) != 0;
    }

private:
    static constexpr size_t CHAR_SIZE = 1;
    static constexpr size_t WIDE_CHAR_SIZE = 2;
    static constexpr size_t SAVE_RESET_START = 1;
    static constexpr size_t SAVE_RESET_END = 2;
    static constexpr size_t LOOP_MIN_OFFSET = 5;
    static constexpr size_t LOOP_MAX_OFFSET = 9;
    static constexpr size_t LOOP_PC_OFFSET = 1;
    static constexpr size_t RANGE32_HEAD_OFFSET = 3;
    static constexpr size_t RANGE32_MAX_HALF_OFFSET = 4;
    static constexpr size_t RANGE32_MAX_OFFSET = 8;
    static constexpr size_t RANGE32_OFFSET = 2;
    static constexpr uint32_t STACK_MULTIPLIER = 2;
    static constexpr uint32_t MIN_STACK_SIZE = 8;
    uint8_t *input_ = nullptr;
    uint8_t *inputEnd_ = nullptr;
    bool isWideChar_ = false;

    uint32_t currentPc_ = 0;
    const uint8_t *currentPtr_ = nullptr;
    CaptureState *captureResultList_ = nullptr;
    uintptr_t *stack_ = nullptr;
    uint32_t currentStack_ = 0;

    uint32_t nCapture_ = 0;
    uint32_t nStack_ = 0;

    uint32_t flags_ = 0;
    uint32_t stateStackLen_ = 0;
    uint32_t stateStackSize_ = 0;
    uint32_t stateSize_ = 0;
    uint8_t *stateStack_ = nullptr;
    Chunk *chunk_ = nullptr;
};
}  // namespace panda::ecmascript
#endif  // PANDA_RUNTIME_ECMASCRIPT_REGEXP_EXECUTOR_H
