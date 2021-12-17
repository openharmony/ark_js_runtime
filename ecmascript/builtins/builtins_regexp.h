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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_REGEXP_H
#define ECMASCRIPT_BUILTINS_BUILTINS_REGEXP_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/builtins/builtins_string.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/regexp/regexp_executor.h"
#include "ecmascript/regexp/regexp_parser.h"

namespace panda::ecmascript::builtins {
class BuiltinsRegExp : public base::BuiltinsBase {
public:
    // 21.2.3.1 RegExp ( pattern, flags )
    static JSTaggedValue RegExpConstructor(EcmaRuntimeCallInfo *argv);

    // prototype
    // 21.2.5.2 RegExp.prototype.exec ( string )
    static JSTaggedValue Exec(EcmaRuntimeCallInfo *argv);
    // 21.2.5.13 RegExp.prototype.test( S )
    static JSTaggedValue Test(EcmaRuntimeCallInfo *argv);
    // 21.2.5.14 RegExp.prototype.toString ( )
    static JSTaggedValue ToString(EcmaRuntimeCallInfo *argv);
    // 21.2.5.3 get RegExp.prototype.flags
    static JSTaggedValue GetFlags(EcmaRuntimeCallInfo *argv);
    // 21.2.5.4 get RegExp.prototype.global
    static JSTaggedValue GetGlobal(EcmaRuntimeCallInfo *argv);
    // 21.2.5.5 get RegExp.prototype.ignoreCase
    static JSTaggedValue GetIgnoreCase(EcmaRuntimeCallInfo *argv);
    // 21.2.5.7 get RegExp.prototype.multiline
    static JSTaggedValue GetMultiline(EcmaRuntimeCallInfo *argv);
    static JSTaggedValue GetDotAll(EcmaRuntimeCallInfo *argv);
    // 21.2.5.10 get RegExp.prototype.source
    static JSTaggedValue GetSource(EcmaRuntimeCallInfo *argv);
    // 21.2.5.12 get RegExp.prototype.sticky
    static JSTaggedValue GetSticky(EcmaRuntimeCallInfo *argv);
    // 21.2.5.15 get RegExp.prototype.unicode
    static JSTaggedValue GetUnicode(EcmaRuntimeCallInfo *argv);
    // 21.2.4.2 get RegExp [ @@species ]
    static JSTaggedValue GetSpecies(EcmaRuntimeCallInfo *argv);
    // 21.2.5.6 RegExp.prototype [ @@match ] ( string )
    static JSTaggedValue Match(EcmaRuntimeCallInfo *argv);
    // 21.2.5.8 RegExp.prototype [ @@replace ] ( string, replaceValue )
    static JSTaggedValue Replace(EcmaRuntimeCallInfo *argv);
    // 21.2.5.9 RegExp.prototype [ @@search ] ( string )
    static JSTaggedValue Search(EcmaRuntimeCallInfo *argv);
    // 21.2.5.11 RegExp.prototype [ @@split ] ( string, limit )
    static JSTaggedValue Split(EcmaRuntimeCallInfo *argv);
    // 21.2.3.2.3 Runtime Semantics: RegExpCreate ( P, F )
    static JSTaggedValue RegExpCreate(JSThread *thread, const JSHandle<JSTaggedValue> &pattern,
                                      const JSHandle<JSTaggedValue> &flags);
    static JSTaggedValue FlagsBitsToString(JSThread *thread, uint8_t flags);

private:
    static constexpr uint32_t MIN_REPLACE_STRING_LENGTH = 1000;
    static constexpr uint32_t MAX_SPLIT_LIMIT = 0xFFFFFFFFu;

    static RegExpExecutor::MatchResult Matcher(JSThread *thread, const JSHandle<JSTaggedValue> &regexp,
                                               const uint8_t *buffer, size_t length, int32_t lastindex, bool isUtf16);
    // 21.2.5.2.3 AdvanceStringIndex ( S, index, unicode )
    static uint32_t AdvanceStringIndex(JSThread *thread, const JSHandle<JSTaggedValue> &inputStr, uint32_t index,
                                       bool unicode);

    static bool GetFlagsInternal(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                 const uint8_t mask);
    // 21.2.5.2.2 Runtime Semantics: RegExpBuiltinExec ( R, S )
    static JSTaggedValue RegExpBuiltinExec(JSThread *thread, const JSHandle<JSTaggedValue> &regexp,
                                           const JSHandle<JSTaggedValue> &inputStr, bool isCached);
    // 21.2.5.2.1 Runtime Semantics: RegExpExec ( R, S )
    static JSTaggedValue RegExpExec(JSThread *thread, const JSHandle<JSTaggedValue> &regexp,
                                    const JSHandle<JSTaggedValue> &inputString, bool isCached);
    // 21.2.3.2.1 Runtime Semantics: RegExpAlloc ( newTarget )
    static JSTaggedValue RegExpAlloc(JSThread *thread, const JSHandle<JSTaggedValue> &newTarget);

    static uint32_t UpdateExpressionFlags(JSThread *thread, const CString &checkStr);

    // 21.2.3.2.2 Runtime Semantics: RegExpInitialize ( obj, pattern, flags )
    static JSTaggedValue RegExpInitialize(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                          const JSHandle<JSTaggedValue> &pattern, const JSHandle<JSTaggedValue> &flags);
    // 21.2.3.2.4 Runtime Semantics: EscapeRegExpPattern ( P, F )
    static EcmaString *EscapeRegExpPattern(JSThread *thread, const JSHandle<JSTaggedValue> &src,
                                           const JSHandle<JSTaggedValue> &flags);
    static JSTaggedValue RegExpReplaceFast(JSThread *thread, JSHandle<JSTaggedValue> &regexp,
                                           JSHandle<EcmaString> inputString, uint32_t inputLength);
};

class RegExpExecResultCache : public TaggedArray {
public:
    enum CacheType {
        REPLACE_TYPE,
        SPLIT_TYPE,
        MATCH_TYPE,
        EXEC_TYPE
    };
    static RegExpExecResultCache *Cast(TaggedObject *object)
    {
        return reinterpret_cast<RegExpExecResultCache *>(object);
    }
    static JSTaggedValue CreateCacheTable(JSThread *thread);
    JSTaggedValue FindCachedResult(JSThread *thread, const JSHandle<JSTaggedValue> &patten,
                                   const JSHandle<JSTaggedValue> &flags, const JSHandle<JSTaggedValue> &input,
                                   CacheType type, const JSHandle<JSTaggedValue> &regexp);
    static void AddResultInCache(JSThread *thread, JSHandle<RegExpExecResultCache> cache,
                                 const JSHandle<JSTaggedValue> &patten, const JSHandle<JSTaggedValue> &flags,
                                 const JSHandle<JSTaggedValue> &input, const JSHandle<JSTaggedValue> &resultArray,
                                 CacheType type, uint32_t lastIndex);

    static void GrowRegexpCache(JSThread *thread, JSHandle<RegExpExecResultCache> cache);

    void ClearEntry(JSThread *thread, int entry);
    void SetEntry(JSThread *thread, int entry, JSTaggedValue &patten, JSTaggedValue &flags, JSTaggedValue &input,
                  JSTaggedValue &lastIndexValue);
    void UpdateResultArray(JSThread *thread, int entry, JSTaggedValue resultArray, CacheType type);
    bool Match(int entry, JSTaggedValue &pattenStr, JSTaggedValue &flagsStr, JSTaggedValue &inputStr);
    inline void SetHitCount(JSThread *thread, int hitCount)
    {
        Set(thread, CACHE_HIT_COUNT_INDEX, JSTaggedValue(hitCount));
    }

    inline int GetHitCount()
    {
        return Get(CACHE_HIT_COUNT_INDEX).GetInt();
    }

    inline void SetCacheCount(JSThread *thread, int hitCount)
    {
        Set(thread, CACHE_COUNT_INDEX, JSTaggedValue(hitCount));
    }

    inline int GetCacheCount()
    {
        return Get(CACHE_COUNT_INDEX).GetInt();
    }

    void Print()
    {
        std::cout << "cache count: " << GetCacheCount() << std::endl;
        std::cout << "cache hit count: " << GetHitCount() << std::endl;
    }

    inline void SetLargeStrCount(JSThread *thread, uint32_t newCount)
    {
        Set(thread, LARGE_STRING_COUNT_INDEX, JSTaggedValue(newCount));
    }

    inline void SetConflictCount(JSThread *thread, uint32_t newCount)
    {
        Set(thread, CONFLICT_COUNT_INDEX, JSTaggedValue(newCount));
    }

    inline void SetStrLenThreshold(JSThread *thread, uint32_t newThreshold)
    {
        Set(thread, STRING_LENGTH_THRESHOLD_INDEX, JSTaggedValue(newThreshold));
    }

    inline uint32_t GetLargeStrCount()
    {
        return Get(LARGE_STRING_COUNT_INDEX).GetInt();
    }

    inline uint32_t GetConflictCount()
    {
        return Get(CONFLICT_COUNT_INDEX).GetInt();
    }

    inline uint32_t GetStrLenThreshold()
    {
        return Get(STRING_LENGTH_THRESHOLD_INDEX).GetInt();
    }

    inline void SetCacheLength(JSThread *thread, int length)
    {
        Set(thread, CACHE_LENGTH_INDEX, JSTaggedValue(length));
    }

    inline int GetCacheLength()
    {
        return Get(CACHE_LENGTH_INDEX).GetInt();
    }

private:
    static constexpr int DEFAULT_LARGE_STRING_COUNT = 10;
    static constexpr int DEFAULT_CONFLICT_COUNT = 100;
    static constexpr int INITIAL_CACHE_NUMBER = 0x10;
    static constexpr int DEFAULT_CACHE_NUMBER = 0x1000;
    static constexpr int CACHE_COUNT_INDEX = 0;
    static constexpr int CACHE_HIT_COUNT_INDEX = 1;
    static constexpr int LARGE_STRING_COUNT_INDEX = 2;
    static constexpr int CONFLICT_COUNT_INDEX = 3;
    static constexpr int STRING_LENGTH_THRESHOLD_INDEX = 4;
    static constexpr int CACHE_LENGTH_INDEX = 5;
    static constexpr int CACHE_TABLE_HEADER_SIZE = 6;
    static constexpr int PATTERN_INDEX = 0;
    static constexpr int FLAG_INDEX = 1;
    static constexpr int INPUT_STRING_INDEX = 2;
    static constexpr int LAST_INDEX_INDEX = 3;
    static constexpr int RESULT_REPLACE_INDEX = 4;
    static constexpr int RESULT_SPLIT_INDEX = 5;
    static constexpr int RESULT_MATCH_INDEX = 6;
    static constexpr int RESULT_EXEC_INDEX = 7;
    static constexpr int ENTRY_SIZE = 8;
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_REGEXP_H
