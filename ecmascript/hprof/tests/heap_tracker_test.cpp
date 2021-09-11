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

#include <cstdio>
#include <fstream>

#include "ecmascript/ecma_string.h"
#include "ecmascript/global_env.h"
#include "ecmascript/hprof/heap_profiler_interface.h"
#include "ecmascript/hprof/heap_snapshot_json_serializer.h"

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class HeapTrackerTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
        EcmaVM::Cast(instance)->GetFactory()->SetTriggerGc(false);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    PandaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(HeapTrackerTest, HeapTracker)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    HeapProfilerInterface *heapProfile = HeapProfilerInterface::CreateHeapProfiler(thread);
    heapProfile->StartHeapTracking(thread, 50);
    sleep(1);
    int count = 100;
    while (count-- > 0) {
        thread->GetEcmaVM()->GetFactory()->NewJSAsyncFuncObject();
    }
    sleep(1);
    count = 100;
    while (count-- > 0) {
        thread->GetEcmaVM()->GetFactory()->NewJSSymbol();
    }
    sleep(1);
    count = 100;
    while (count-- > 0) {
        JSHandle<EcmaString> string = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("Hello World");
        thread->GetEcmaVM()->GetFactory()->NewJSString(JSHandle<JSTaggedValue>(string));
    }

    // Create file test.heaptimeline
    std::string fileName = "test.heaptimeline";
    fstream outputString(fileName, std::ios::out);
    outputString.close();
    outputString.clear();

    heapProfile->StopHeapTracking(thread, DumpFormat::JSON, fileName.c_str());
    HeapProfilerInterface::Destory(thread, heapProfile);

    // Check
    fstream inputStream(fileName, std::ios::in);
    std::string line;
    std::string emptySample = "\"samples\":";
    std::string firstSample = "\"samples\":[0, ";
    int emptySize = emptySample.size();
    bool isFind = false;
    while (getline(inputStream, line)) {
        if (line.substr(0, emptySize) == emptySample) {
            ASSERT_TRUE(line.substr(0, firstSample.size()) == firstSample);
            isFind = true;
        }
    }
    ASSERT_TRUE(isFind);

    inputStream.close();
    inputStream.clear();
    std::remove(fileName.c_str());
}
}  // namespace panda::test
