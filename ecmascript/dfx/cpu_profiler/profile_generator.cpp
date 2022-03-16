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

#include "ecmascript/dfx/cpu_profiler/profile_generator.h"

#include <climits>
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#include "ecmascript/interpreter/interpreter.h"
namespace panda::ecmascript {
bool ProfileGenerator::staticGcState_ = false;
ProfileGenerator::ProfileGenerator()
{
    stackTopLines_.push_back(0);
    struct MethodKey methodkey;
    struct MethodNode methodNode;
    methodkey.method = reinterpret_cast<JSMethod*>(INT_MAX - 1);
    methodMap_.insert(std::make_pair(methodkey, methodMap_.size() + 1));
    methodNode.parentId = 0;
    methodNode.codeEntry.codeType = "JS";
    methodNodes_.push_back(methodNode);
}

ProfileGenerator::~ProfileGenerator()
{
    if (fileHandle_.is_open()) {
        fileHandle_.close();
    }
}

void ProfileGenerator::AddSample(CVector<JSMethod *> sample, uint64_t sampleTimeStamp)
{
    static int PreviousId = 0;
    struct MethodKey methodkey;
    struct MethodNode methodNode;
    if (staticGcState_) {
        methodkey.method = reinterpret_cast<JSMethod*>(INT_MAX);
        methodNode.parentId = methodkey.parentId = PreviousId;
        auto result = methodMap_.find(methodkey);
        if (result == methodMap_.end()) {
            methodNode.id = methodMap_.size() + 1;
            methodMap_.insert(std::make_pair(methodkey, methodNode.id));
            methodNode.codeEntry = GetGcInfo();
            stackTopLines_.push_back(0);
            methodNodes_.push_back(methodNode);
        } else {
            methodNode.id = result->second;
        }
        staticGcState_ = false;
    } else {
        for (auto method = sample.rbegin(); method != sample.rend(); method++) {
            methodkey.method = *method;
            if (method == sample.rbegin()) {
                methodNode.id = 1;
                continue;
            } else {
                methodNode.parentId = methodkey.parentId = methodNode.id;
            }
            auto result = methodMap_.find(methodkey);
            if (result == methodMap_.end()) {
                int id = methodMap_.size() + 1;
                methodMap_.insert(std::make_pair(methodkey, id));
                PreviousId = methodNode.id = id;
                methodNode.codeEntry = GetMethodInfo(methodkey.method);
                stackTopLines_.push_back(methodNode.codeEntry.lineNumber);
                methodNodes_.push_back(methodNode);
            } else {
                PreviousId = methodNode.id = result->second;
            }
        }
    }
    static uint64_t threadStartTime = 0;
    struct SampleInfo sampleInfo;
    sampleInfo.id = methodNode.id == 0 ? PreviousId = 1, 1 : methodNode.id;
    sampleInfo.line = stackTopLines_[methodNode.id];
    if (threadStartTime == 0) {
        sampleInfo.timeStamp = sampleTimeStamp - threadStartTime_;
    } else {
        sampleInfo.timeStamp = sampleTimeStamp - threadStartTime;
    }
    samples_.push_back(sampleInfo);
    threadStartTime = sampleTimeStamp;
}

void ProfileGenerator::WriteAddNodes()
{
    sampleData_ += "{\"args\":{\"data\":{\"cpuProfile\":{\"nodes\":[";
    for (auto it : methodNodes_) {
        sampleData_ += "{\"callFrame\":{\"codeType\":\"" + it.codeEntry.codeType + "\",";
        if (it.parentId == 0) {
            sampleData_ += "\"functionName\":\"(root)\",\"scriptId\":0},\"id\":1},";
            continue;
        }
        if (it.codeEntry.codeType == "other" || it.codeEntry.codeType == "jsvm") {
            sampleData_ += "\"functionName\":\"" + it.codeEntry.functionName + "\",\"scriptId\":" +
                           std::to_string(it.codeEntry.scriptId) + "},\"id\":" + std::to_string(it.id);
        } else {
            sampleData_ += "\"columnNumber\":" + std::to_string(it.codeEntry.columnNumber) +
                           ",\"functionName\":\"" + it.codeEntry.functionName + "\",\"lineNumber\":\"" +
                           std::to_string(it.codeEntry.lineNumber) + "\",\"scriptId\":" +
                           std::to_string(it.codeEntry.scriptId) + ",\"url\":\"" + it.codeEntry.url +
                           "\"},\"id\":" + std::to_string(it.id);
        }
        sampleData_ += ",\"parent\":" + std::to_string(it.parentId) + "},";
    }
    sampleData_.pop_back();
    sampleData_ += "],\"samples\":[";
}

void ProfileGenerator::WriteAddSamples()
{
    if (samples_.empty()) {
        return;
    }
    std::string sampleId = "";
    std::string sampleLine = "";
    std::string timeStamp = "";
    for (auto it : samples_) {
        sampleId += std::to_string(it.id) + ",";
        sampleLine += std::to_string(it.line) + ",";
        timeStamp += std::to_string(it.timeStamp) + ",";
    }
    sampleId.pop_back();
    sampleLine.pop_back();
    timeStamp.pop_back();
    sampleData_ += sampleId + "]},\"lines\":[" + sampleLine + "],\"timeDeltas\":[" + timeStamp + "]}},";
}

void ProfileGenerator::WriteMethodsAndSampleInfo(bool timeEnd)
{
    if (methodNodes_.size() >= 10) { // 10:Number of nodes currently stored
        WriteAddNodes();
        WriteAddSamples();
        methodNodes_.clear();
        samples_.clear();
    } else if (samples_.size() == 100 || timeEnd) { // 100:Number of samples currently stored
        if (!methodNodes_.empty()) {
            WriteAddNodes();
            WriteAddSamples();
            methodNodes_.clear();
            samples_.clear();
        } else if (!samples_.empty()) {
            sampleData_ += "{\"args\":{\"data\":{\"cpuProfile\":{\"samples\":[";
            WriteAddSamples();
            samples_.clear();
        } else {
            return;
        }
    }
    sampleData_ += "\"cat\":\"disabled-by-default-ark.cpu_profiler\",\"id\":"
                    "\"0x2\",\"name\":\"ProfileChunk\",\"ph\":\"P\",\"pid\":";
    pid_t pid = getpid();
    pthread_t tid = syscall(SYS_gettid);
    uint64_t ts = ProfileProcessor::GetMicrosecondsTimeStamp();
    ts = ts % TIME_CHANGE;
    struct timespec time = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &time);
    uint64_t tts = time.tv_nsec / 1000; // 1000:Nanoseconds to milliseconds.
    sampleData_ += std::to_string(pid) + ",\"tid\":" +
                   std::to_string(tid) + ",\"ts\":" +
                   std::to_string(ts) + ",\"tts\":" +
                   std::to_string(tts) + "},\n";
}

CVector<struct MethodNode> ProfileGenerator::GetMethodNodes() const
{
    return methodNodes_;
}

CDeque<struct SampleInfo> ProfileGenerator::GetSamples() const
{
    return samples_;
}

std::string ProfileGenerator::GetSampleData() const
{
    return sampleData_;
}

struct StackInfo ProfileGenerator::GetMethodInfo(JSMethod *method)
{
    struct StackInfo entry;
    auto iter = CpuProfiler::staticStackInfo_.find(method);
    if (iter != CpuProfiler::staticStackInfo_.end()) {
        entry = iter->second;
    }
    return entry;
}

struct StackInfo ProfileGenerator::GetGcInfo()
{
    struct StackInfo gcEntry;
    gcEntry.codeType = "jsvm";
    gcEntry.functionName = "garbage collector";
    return gcEntry;
}

void ProfileGenerator::SetThreadStartTime(uint64_t threadStartTime)
{
    threadStartTime_ = threadStartTime;
}

void ProfileGenerator::SetStartsampleData(std::string sampleData)
{
    sampleData_ += sampleData;
}

void ProfileGenerator::SetFileName(std::string &fileName)
{
    fileName_ = fileName;
}

const std::string ProfileGenerator::GetFileName() const
{
    return fileName_;
}

void ProfileGenerator::ClearSampleData()
{
    sampleData_.clear();
}
} // namespace panda::ecmascript