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

#ifndef ECMASCRIPT_TOOLING_TEST_UTILS_TEST_UTIL_H
#define ECMASCRIPT_TOOLING_TEST_UTILS_TEST_UTIL_H

#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/tooling/interface/js_debugger.h"
#include "ecmascript/tooling/test/utils/test_events.h"
#include "ecmascript/tooling/test/utils/test_extractor.h"
#include "os/mutex.h"

namespace panda::ecmascript::tooling::test {
template<class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
using CUnorderedMap = panda::ecmascript::CUnorderedMap<Key, T, Hash, KeyEqual>;
using TestMap = CUnorderedMap<panda_file::SourceLang, CUnorderedMap<const char *, std::unique_ptr<TestEvents>>>;

class TestUtil {
public:
    static void RegisterTest(panda_file::SourceLang language, const char *testName, std::unique_ptr<TestEvents> test)
    {
        auto it = testMap_.find(language);
        if (it == testMap_.end()) {
            CUnorderedMap<const char *, std::unique_ptr<TestEvents>> entry;
            auto res = testMap_.emplace(language, std::move(entry));
            it = res.first;
        }
        it->second.insert({testName, std::move(test)});
    }

    static TestEvents *GetTest(const char *name)
    {
        for (auto it = testMap_.begin(); it != testMap_.end(); ++it) {
            auto &internalMap = it->second;
            auto internalIt = std::find_if(internalMap.begin(), internalMap.end(),
                                           [name](auto &it) { return !::strcmp(it.first, name); });
            if (internalIt != internalMap.end()) {
                return internalIt->second.get();
            }
        }
        LOG(FATAL, DEBUGGER) << "Test " << name << " not found";
        return nullptr;
    }

    static void WaitForBreakpoint(JSPtLocation location)
    {
        auto predicate = [&location]() REQUIRES(eventMutex_) { return lastEventLocation_ == location; };
        auto onSuccess = []() REQUIRES(eventMutex_) {
            // Need to reset location, because we might want to stop at the same point
            lastEventLocation_ = JSPtLocation("", EntityId(0), 0);
        };

        WaitForEvent(DebugEvent::BREAKPOINT, predicate, onSuccess);
    }

    static bool WaitForExit()
    {
        return WaitForEvent(DebugEvent::VM_DEATH,
            []() REQUIRES(eventMutex_) {
                return lastEvent_ == DebugEvent::VM_DEATH;
            }, [] {});
    }

    static bool WaitForException()
    {
        auto predicate = []() REQUIRES(eventMutex_) { return lastEvent_ == DebugEvent::EXCEPTION; };
        return WaitForEvent(DebugEvent::EXCEPTION, predicate, [] {});
    }

    static bool WaitForInit()
    {
        return WaitForEvent(DebugEvent::VM_START,
            []() REQUIRES(eventMutex_) {
                return initialized_;
            }, [] {});
    }

    static void Event(DebugEvent event, JSPtLocation location = JSPtLocation("", EntityId(0), 0))
    {
        LOG(DEBUG, DEBUGGER) << "Occurred event " << event;
        os::memory::LockHolder holder(eventMutex_);
        lastEvent_ = event;
        lastEventLocation_ = location;
        if (event == DebugEvent::VM_START) {
            initialized_ = true;
        }
        eventCv_.Signal();
    }

    static void Reset()
    {
        os::memory::LockHolder lock(eventMutex_);
        initialized_ = false;
        lastEvent_ = DebugEvent::VM_START;
    }

    static TestMap &GetTests()
    {
        return testMap_;
    }

    static bool IsTestFinished()
    {
        os::memory::LockHolder lock(eventMutex_);
        return lastEvent_ == DebugEvent::VM_DEATH;
    }

    static JSPtLocation GetLocation(const char *sourceFile, int32_t line, int32_t column, const char *pandaFile)
    {
        auto jsPandaFile = ::panda::ecmascript::JSPandaFileManager::GetInstance()->OpenJSPandaFile(pandaFile);
        if (jsPandaFile == nullptr) {
            return JSPtLocation("", EntityId(0), 0);
        }
        TestExtractor extractor(jsPandaFile);
        auto [id, offset] = extractor.GetBreakpointAddress({sourceFile, line, column});
        return JSPtLocation(pandaFile, id, offset);
    }

    static SourceLocation GetSourceLocation(const JSPtLocation &location, const char *pandaFile)
    {
        auto jsPandaFile = ::panda::ecmascript::JSPandaFileManager::GetInstance()->OpenJSPandaFile(pandaFile);
        if (jsPandaFile == nullptr) {
            return SourceLocation();
        }
        TestExtractor extractor(jsPandaFile);
        return extractor.GetSourceLocation(location.GetMethodId(), location.GetBytecodeOffset());
    }

    static bool SuspendUntilContinue(DebugEvent reason, JSPtLocation location)
    {
        os::memory::LockHolder lock(suspendMutex_);
        suspended_ = true;

        // Notify the debugger thread about the suspend event
        Event(reason, location);

        // Wait for continue
        while (suspended_) {
            suspendCv_.Wait(&suspendMutex_);
        }

        return true;
    }

    static bool Continue()
    {
        os::memory::LockHolder lock(suspendMutex_);
        suspended_ = false;
        suspendCv_.Signal();
        return true;
    }

private:
    template<class Predicate, class OnSuccessAction>
    static bool WaitForEvent(DebugEvent event, Predicate predicate, OnSuccessAction action)
    {
        os::memory::LockHolder holder(eventMutex_);
        while (!predicate()) {
            if (lastEvent_ == DebugEvent::VM_DEATH) {
                return false;
            }
            constexpr uint64_t TIMEOUT_MSEC = 10000U;
            bool timeExceeded = eventCv_.TimedWait(&eventMutex_, TIMEOUT_MSEC);
            if (timeExceeded) {
                LOG(FATAL, DEBUGGER) << "Time limit exceeded while waiting " << event;
                return false;
            }
        }
        action();
        return true;
    }

    static TestMap testMap_;
    static os::memory::Mutex eventMutex_;
    static os::memory::ConditionVariable eventCv_ GUARDED_BY(eventMutex_);
    static DebugEvent lastEvent_ GUARDED_BY(eventMutex_);
    static JSPtLocation lastEventLocation_ GUARDED_BY(eventMutex_);
    static os::memory::Mutex suspendMutex_;
    static os::memory::ConditionVariable suspendCv_ GUARDED_BY(suspendMutex_);
    static bool suspended_ GUARDED_BY(suspendMutex_);
    static bool initialized_ GUARDED_BY(eventMutex_);
};

std::ostream &operator<<(std::ostream &out, std::nullptr_t);

#define ASSERT_FAIL_(val1, val2, strval1, strval2, msg)                                    \
    do {                                                                                   \
        std::cerr << "Assertion failed at " << __FILE__ << ':' << __LINE__ << std::endl;   \
        std::cerr << "Expected that " strval1 " is " << (msg) << " " strval2 << std::endl; \
        std::cerr << "\t" strval1 ": " << (val1) << std::endl;                             \
        std::cerr << "\t" strval2 ": " << (val2) << std::endl;                             \
        std::abort();                                                                      \
    } while (0)

#define ASSERT_TRUE(cond)                                       \
    do {                                                        \
        auto res = (cond);                                      \
        if (!res) {                                             \
            ASSERT_FAIL_(res, true, #cond, "true", "equal to"); \
        }                                                       \
    } while (0)

#define ASSERT_FALSE(cond)                                        \
    do {                                                          \
        auto res = (cond);                                        \
        if (res) {                                                \
            ASSERT_FAIL_(res, false, #cond, "false", "equal to"); \
        }                                                         \
    } while (0)

#define ASSERT_EQ(lhs, rhs)                                   \
    do {                                                      \
        auto res1 = (lhs);                                    \
        decltype(res1) res2 = (rhs);                          \
        if (res1 != res2) {                                   \
            ASSERT_FAIL_(res1, res2, #lhs, #rhs, "equal to"); \
        }                                                     \
    } while (0)

#define ASSERT_NE(lhs, rhs)                                       \
    do {                                                          \
        auto res1 = (lhs);                                        \
        decltype(res1) res2 = (rhs);                              \
        if (res1 == res2) {                                       \
            ASSERT_FAIL_(res1, res2, #lhs, #rhs, "not equal to"); \
        }                                                         \
    } while (0)

#define ASSERT_STREQ(lhs, rhs)                                \
    do {                                                      \
        auto res1 = (lhs);                                    \
        decltype(res1) res2 = (rhs);                          \
        if (::strcmp(res1, res2) != 0) {                      \
            ASSERT_FAIL_(res1, res2, #lhs, #rhs, "equal to"); \
        }                                                     \
    } while (0)

#define ASSERT_SUCCESS(api_call)                                                                    \
    do {                                                                                            \
        auto error = api_call;                                                                      \
        if (error) {                                                                                \
            ASSERT_FAIL_(error.value().GetMessage(), "Success", "API call result", "Expected", ""); \
        }                                                                                           \
    } while (0)

#define ASSERT_EXITED()                                                                               \
    do {                                                                                              \
        bool res = TestUtil::WaitForExit();                                                           \
        if (!res) {                                                                                   \
            ASSERT_FAIL_(TestUtil::IsTestFinished(), true, "TestUtil::IsTestFinished()", "true", ""); \
        }                                                                                             \
    } while (0)

#define ASSERT_LOCATION_EQ(lhs, rhs)                                                 \
    do {                                                                             \
        ASSERT_STREQ((lhs).GetPandaFile(), (rhs).GetPandaFile());                    \
        ASSERT_EQ((lhs).GetMethodId().GetOffset(), (rhs).GetMethodId().GetOffset()); \
        ASSERT_EQ((lhs).GetBytecodeOffset(), (rhs).GetBytecodeOffset());             \
    } while (0)

#define ASSERT_BREAKPOINT_SUCCESS(location)                         \
    do {                                                            \
        TestUtil::WaitForBreakpoint(location);                      \
    } while (0)
}  // namespace panda::ecmascript::tooling::test

#endif  // ECMASCRIPT_TOOLING_TEST_UTILS_TEST_UTIL_H
