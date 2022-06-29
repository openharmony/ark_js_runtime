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

#ifndef ECMASCRIPT_REGEXP_OPCODE_H
#define ECMASCRIPT_REGEXP_OPCODE_H

#include <list>

#include "ecmascript/mem/dyn_chunk.h"

namespace panda {
namespace ecmascript {
class RegExpOpCode {
public:
    enum : uint8_t {
        OP_SAVE_START = 0U,
        OP_SAVE_END,
        OP_CHAR,
        OP_GOTO,
        OP_SPLIT_FIRST,
        OP_SPLIT_NEXT,
        OP_MATCH_AHEAD,
        OP_NEGATIVE_MATCH_AHEAD,
        OP_MATCH,
        OP_LOOP,
        OP_LOOP_GREEDY,
        OP_PUSH_CHAR,
        OP_CHECK_CHAR,
        OP_PUSH,
        OP_POP,
        OP_SAVE_RESET,
        OP_LINE_START,
        OP_LINE_END,
        OP_WORD_BOUNDARY,
        OP_NOT_WORD_BOUNDARY,
        OP_ALL,
        OP_DOTS,
        OP_MATCH_END,
        OP_PREV,
        OP_RANGE,
        OP_BACKREFERENCE,
        OP_BACKWARD_BACKREFERENCE,
        OP_CHAR32,
        OP_RANGE32,
        OP_INVALID,
    };

    static constexpr size_t OP_SIZE_ONE = 1;
    static constexpr size_t OP_SIZE_TWO = 2;
    static constexpr size_t OP_SIZE_THREE = 3;
    static constexpr size_t OP_SIZE_FOUR = 4;
    static constexpr size_t OP_SIZE_FIVE = 5;
    static constexpr size_t OP_SIZE_EIGHT = 8;
    static constexpr size_t OP_SIZE_NINE = 9;
    static constexpr size_t OP_SIZE_THIRTEEN = 13;

    RegExpOpCode(uint8_t opCode, int size);
    NO_COPY_SEMANTIC(RegExpOpCode);
    NO_MOVE_SEMANTIC(RegExpOpCode);

    virtual ~RegExpOpCode() = default;
    static RegExpOpCode *GetRegExpOpCode(const DynChunk &buf, int pcOffset);
    static RegExpOpCode *GetRegExpOpCode(uint8_t opCode);
    static void DumpRegExpOpCode(std::ostream &out, const DynChunk &buf);
    inline uint8_t GetSize() const
    {
        return size_;
    }
    inline uint8_t GetOpCode() const
    {
        return opCode_;
    }
    inline int GetDynChunkfSize(const DynChunk &buf) const
    {
        return buf.size_;
    }
    virtual uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const = 0;

private:
    uint8_t opCode_{0};
    uint8_t size_{0};
};

class SaveStartOpCode : public RegExpOpCode {
public:
    SaveStartOpCode() : RegExpOpCode(OP_SAVE_START, RegExpOpCode::OP_SIZE_TWO) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t para) const;
    ~SaveStartOpCode() override = default;
    NO_COPY_SEMANTIC(SaveStartOpCode);
    NO_MOVE_SEMANTIC(SaveStartOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class SaveEndOpCode : public RegExpOpCode {
public:
    SaveEndOpCode() : RegExpOpCode(OP_SAVE_END, RegExpOpCode::OP_SIZE_TWO) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t para) const;
    ~SaveEndOpCode() override = default;
    NO_COPY_SEMANTIC(SaveEndOpCode);
    NO_MOVE_SEMANTIC(SaveEndOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class CharOpCode : public RegExpOpCode {
public:
    CharOpCode() : RegExpOpCode(OP_CHAR, RegExpOpCode::OP_SIZE_THREE) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t para) const;
    ~CharOpCode() override = default;
    NO_COPY_SEMANTIC(CharOpCode);
    NO_MOVE_SEMANTIC(CharOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class GotoOpCode : public RegExpOpCode {
public:
    GotoOpCode() : RegExpOpCode(OP_GOTO, RegExpOpCode::OP_SIZE_FIVE) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t para) const;
    void UpdateOpPara(DynChunk *buf, uint32_t offset, uint32_t para) const;
    ~GotoOpCode() override = default;
    NO_COPY_SEMANTIC(GotoOpCode);
    NO_MOVE_SEMANTIC(GotoOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class SplitNextOpCode : public RegExpOpCode {
public:
    SplitNextOpCode() : RegExpOpCode(OP_SPLIT_NEXT, RegExpOpCode::OP_SIZE_FIVE) {}
    uint32_t InsertOpCode(DynChunk *buf, uint32_t offset, uint32_t para) const;
    ~SplitNextOpCode() override = default;
    NO_COPY_SEMANTIC(SplitNextOpCode);
    NO_MOVE_SEMANTIC(SplitNextOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class SplitFirstOpCode : public RegExpOpCode {
public:
    SplitFirstOpCode() : RegExpOpCode(OP_SPLIT_FIRST, RegExpOpCode::OP_SIZE_FIVE) {}
    uint32_t InsertOpCode(DynChunk *buf, uint32_t offset, uint32_t para) const;
    ~SplitFirstOpCode() override = default;
    NO_COPY_SEMANTIC(SplitFirstOpCode);
    NO_MOVE_SEMANTIC(SplitFirstOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class PushOpCode : public RegExpOpCode {
public:
    PushOpCode() : RegExpOpCode(OP_PUSH, RegExpOpCode::OP_SIZE_ONE) {}
    uint32_t InsertOpCode(DynChunk *buf, uint32_t offset) const;
    ~PushOpCode() override = default;
    NO_COPY_SEMANTIC(PushOpCode);
    NO_MOVE_SEMANTIC(PushOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class PopOpCode : public RegExpOpCode {
public:
    PopOpCode() : RegExpOpCode(OP_POP, RegExpOpCode::OP_SIZE_ONE) {}
    uint32_t EmitOpCode(DynChunk *buf) const;
    ~PopOpCode() override = default;
    NO_COPY_SEMANTIC(PopOpCode);
    NO_MOVE_SEMANTIC(PopOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class PushCharOpCode : public RegExpOpCode {
public:
    PushCharOpCode() : RegExpOpCode(OP_PUSH_CHAR, RegExpOpCode::OP_SIZE_ONE) {}
    uint32_t InsertOpCode(DynChunk *buf, uint32_t offset) const;
    ~PushCharOpCode() override = default;
    NO_COPY_SEMANTIC(PushCharOpCode);
    NO_MOVE_SEMANTIC(PushCharOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class CheckCharOpCode : public RegExpOpCode {
public:
    CheckCharOpCode() : RegExpOpCode(OP_CHECK_CHAR, RegExpOpCode::OP_SIZE_FIVE) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t offset) const;
    ~CheckCharOpCode() override = default;
    NO_COPY_SEMANTIC(CheckCharOpCode);
    NO_MOVE_SEMANTIC(CheckCharOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class LoopOpCode : public RegExpOpCode {
public:
    LoopOpCode() : RegExpOpCode(OP_LOOP, RegExpOpCode::OP_SIZE_THIRTEEN) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t start, uint32_t min, uint32_t max) const;
    ~LoopOpCode() override = default;
    NO_COPY_SEMANTIC(LoopOpCode);
    NO_MOVE_SEMANTIC(LoopOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class LoopGreedyOpCode : public RegExpOpCode {
public:
    LoopGreedyOpCode() : RegExpOpCode(OP_LOOP_GREEDY, RegExpOpCode::OP_SIZE_THIRTEEN) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t start, uint32_t min, uint32_t max) const;
    ~LoopGreedyOpCode() override = default;
    NO_COPY_SEMANTIC(LoopGreedyOpCode);
    NO_MOVE_SEMANTIC(LoopGreedyOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class SaveResetOpCode : public RegExpOpCode {
public:
    SaveResetOpCode() : RegExpOpCode(OP_SAVE_RESET, RegExpOpCode::OP_SIZE_THREE) {}
    uint32_t InsertOpCode(DynChunk *buf, uint32_t offset, uint32_t start, uint32_t end) const;
    ~SaveResetOpCode() override = default;
    NO_COPY_SEMANTIC(SaveResetOpCode);
    NO_MOVE_SEMANTIC(SaveResetOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class MatchOpCode : public RegExpOpCode {
public:
    MatchOpCode() : RegExpOpCode(OP_MATCH, RegExpOpCode::OP_SIZE_ONE) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t para) const;
    ~MatchOpCode() override = default;
    NO_COPY_SEMANTIC(MatchOpCode);
    NO_MOVE_SEMANTIC(MatchOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class MatchEndOpCode : public RegExpOpCode {
public:
    MatchEndOpCode() : RegExpOpCode(OP_MATCH_END, RegExpOpCode::OP_SIZE_ONE) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t para) const;
    ~MatchEndOpCode() override = default;
    NO_COPY_SEMANTIC(MatchEndOpCode);
    NO_MOVE_SEMANTIC(MatchEndOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class LineStartOpCode : public RegExpOpCode {
public:
    LineStartOpCode() : RegExpOpCode(OP_LINE_START, RegExpOpCode::OP_SIZE_ONE) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t para) const;
    ~LineStartOpCode() override = default;
    NO_COPY_SEMANTIC(LineStartOpCode);
    NO_MOVE_SEMANTIC(LineStartOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class LineEndOpCode : public RegExpOpCode {
public:
    LineEndOpCode() : RegExpOpCode(OP_LINE_END, RegExpOpCode::OP_SIZE_ONE) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t para) const;
    ~LineEndOpCode() override = default;
    NO_COPY_SEMANTIC(LineEndOpCode);
    NO_MOVE_SEMANTIC(LineEndOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class WordBoundaryOpCode : public RegExpOpCode {
public:
    WordBoundaryOpCode() : RegExpOpCode(OP_WORD_BOUNDARY, RegExpOpCode::OP_SIZE_ONE) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t para) const;
    ~WordBoundaryOpCode() override = default;
    NO_COPY_SEMANTIC(WordBoundaryOpCode);
    NO_MOVE_SEMANTIC(WordBoundaryOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class NotWordBoundaryOpCode : public RegExpOpCode {
public:
    NotWordBoundaryOpCode() : RegExpOpCode(OP_NOT_WORD_BOUNDARY, RegExpOpCode::OP_SIZE_ONE) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t para) const;
    ~NotWordBoundaryOpCode() override = default;
    NO_COPY_SEMANTIC(NotWordBoundaryOpCode);
    NO_MOVE_SEMANTIC(NotWordBoundaryOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class AllOpCode : public RegExpOpCode {
public:
    AllOpCode() : RegExpOpCode(OP_ALL, RegExpOpCode::OP_SIZE_ONE) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t para) const;
    ~AllOpCode() override = default;
    NO_COPY_SEMANTIC(AllOpCode);
    NO_MOVE_SEMANTIC(AllOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class DotsOpCode : public RegExpOpCode {
public:
    DotsOpCode() : RegExpOpCode(OP_DOTS, RegExpOpCode::OP_SIZE_ONE) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t para) const;
    ~DotsOpCode() override = default;
    NO_COPY_SEMANTIC(DotsOpCode);
    NO_MOVE_SEMANTIC(DotsOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class RangeSet {
public:
    RangeSet() = default;
    explicit RangeSet(uint32_t value)
    {
        Insert(value, value);
    }
    explicit RangeSet(uint32_t start, uint32_t end)
    {
        Insert(start, end);
    }
    explicit RangeSet(const std::list<std::pair<uint32_t, uint32_t>> &rangeSet)
    {
        rangeSet_ = rangeSet;
    }
    ~RangeSet() = default;

    inline bool IsIntersect(uint64_t start, uint64_t end, uint64_t start1, uint64_t end1) const
    {
        return ((start1 > start) && (start1 < end)) || ((start > start1) && (start < end1));
    }
    inline bool IsAdjacent(uint64_t start, uint64_t end, uint64_t start1, uint64_t end1) const
    {
        return ((end == start1 || (end + 1) == start1)) || ((end1 == start) || (end1 + 1 == start));
    }

    inline bool operator==(const RangeSet &other) const
    {
        return rangeSet_ == other.rangeSet_;
    }

    inline bool IsContain(uint32_t value) const
    {
        for (auto range : rangeSet_) {
            if (value >= range.first && value <= range.second) {
                return true;
            }
        }
        return false;
    }
    inline uint32_t HighestValue() const
    {
        if (!rangeSet_.empty()) {
            return rangeSet_.back().second;
        }
        return 0;
    }
    RangeSet(RangeSet const &) = default;
    RangeSet &operator=(RangeSet const &) = default;
    RangeSet(RangeSet &&) = default;
    RangeSet &operator=(RangeSet &&) = default;

    void Insert(uint32_t start, uint32_t end);
    void Insert(const RangeSet &s1);
    void Invert(bool isUtf16);
    void Compress();

private:
    friend class RangeOpCode;
    friend class Range32OpCode;
    std::list<std::pair<uint32_t, uint32_t>> rangeSet_{};
};

class RangeOpCode : public RegExpOpCode {
public:
    RangeOpCode() : RegExpOpCode(OP_RANGE, RegExpOpCode::OP_SIZE_ONE) {}
    ~RangeOpCode() override = default;
    NO_COPY_SEMANTIC(RangeOpCode);
    NO_MOVE_SEMANTIC(RangeOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
    uint32_t InsertOpCode(DynChunk *buf, const RangeSet &rangeSet) const;
};

class MatchAheadOpCode : public RegExpOpCode {
public:
    MatchAheadOpCode() : RegExpOpCode(OP_MATCH_AHEAD, RegExpOpCode::OP_SIZE_FIVE) {}
    ~MatchAheadOpCode() override = default;
    NO_COPY_SEMANTIC(MatchAheadOpCode);
    NO_MOVE_SEMANTIC(MatchAheadOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
    uint32_t InsertOpCode(DynChunk *buf, uint32_t offset, uint32_t para) const;
};

class NegativeMatchAheadOpCode : public RegExpOpCode {
public:
    NegativeMatchAheadOpCode() : RegExpOpCode(OP_NEGATIVE_MATCH_AHEAD, RegExpOpCode::OP_SIZE_FIVE) {}
    uint32_t InsertOpCode(DynChunk *buf, uint32_t offset, uint32_t para) const;
    ~NegativeMatchAheadOpCode() override = default;
    NO_COPY_SEMANTIC(NegativeMatchAheadOpCode);
    NO_MOVE_SEMANTIC(NegativeMatchAheadOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class PrevOpCode : public RegExpOpCode {
public:
    PrevOpCode() : RegExpOpCode(OP_PREV, RegExpOpCode::OP_SIZE_ONE) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t para) const;
    ~PrevOpCode() override = default;
    NO_COPY_SEMANTIC(PrevOpCode);
    NO_MOVE_SEMANTIC(PrevOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class BackReferenceOpCode : public RegExpOpCode {
public:
    BackReferenceOpCode() : RegExpOpCode(OP_BACKREFERENCE, RegExpOpCode::OP_SIZE_TWO) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t para) const;
    ~BackReferenceOpCode() override = default;
    NO_COPY_SEMANTIC(BackReferenceOpCode);
    NO_MOVE_SEMANTIC(BackReferenceOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class BackwardBackReferenceOpCode : public RegExpOpCode {
public:
    BackwardBackReferenceOpCode() : RegExpOpCode(OP_BACKWARD_BACKREFERENCE, RegExpOpCode::OP_SIZE_TWO) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t para) const;
    ~BackwardBackReferenceOpCode() override = default;
    NO_COPY_SEMANTIC(BackwardBackReferenceOpCode);
    NO_MOVE_SEMANTIC(BackwardBackReferenceOpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class Char32OpCode : public RegExpOpCode {
public:
    Char32OpCode() : RegExpOpCode(OP_CHAR32, RegExpOpCode::OP_SIZE_FIVE) {}
    uint32_t EmitOpCode(DynChunk *buf, uint32_t para) const;
    ~Char32OpCode() override = default;
    NO_COPY_SEMANTIC(Char32OpCode);
    NO_MOVE_SEMANTIC(Char32OpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
};

class Range32OpCode : public RegExpOpCode {
public:
    Range32OpCode() : RegExpOpCode(OP_RANGE32, RegExpOpCode::OP_SIZE_ONE) {}
    ~Range32OpCode() override = default;
    NO_COPY_SEMANTIC(Range32OpCode);
    NO_MOVE_SEMANTIC(Range32OpCode);
    uint32_t DumpOpCode(std::ostream &out, const DynChunk &buf, uint32_t offset) const override;
    uint32_t InsertOpCode(DynChunk *buf, const RangeSet &rangeSet) const;
};
}  // namespace ecmascript
}  // namespace panda
#endif
