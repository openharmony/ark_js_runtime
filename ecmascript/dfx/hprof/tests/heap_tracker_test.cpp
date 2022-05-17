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

#include "ecmascript/dfx/hprof/heap_profiler_interface.h"
#include "ecmascript/dfx/hprof/heap_snapshot_json_serializer.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/global_env.h"

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/tooling/interface/file_stream.h"

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
        instance->SetEnableForceGC(false);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(HeapTrackerTest, HeapTracker)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    HeapProfilerInterface *heapProfile = HeapProfilerInterface::GetInstance(instance);
    heapProfile->StartHeapTracking(50);
    sleep(1);
    int count = 100;
    while (count-- > 0) {
        instance->GetFactory()->NewJSAsyncFuncObject();
    }
    sleep(1);
    count = 100;
    while (count-- > 0) {
        instance->GetFactory()->NewJSSymbol();
    }
    sleep(1);
    count = 100;
    while (count-- > 0) {
        JSHandle<EcmaString> string = instance->GetFactory()->NewFromASCII("Hello World");
        instance->GetFactory()->NewJSString(JSHandle<JSTaggedValue>(string));
    }

    // Create file test.heaptimeline
    std::string fileName = "test.heaptimeline";
    fstream outputString(fileName, std::ios::out);
    outputString.close();
    outputString.clear();

    FileStream stream(fileName.c_str());
    heapProfile->StopHeapTracking(&stream, nullptr);
    HeapProfilerInterface::Destroy(instance);

    // Check
    fstream inputStream(fileName, std::ios::in);
    std::string line;
    std::string emptySample = "\"samples\":";
    std::string firstSample = "\"samples\":[0, ";
    uint32_t emptySize = emptySample.size();
    bool isFind = false;
    while (getline(inputStream, line)) {
        if (line.substr(0U, emptySize) == emptySample) {
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
