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

#include "ecmascript/dfx/cpu_profiler/samples_record.h"

#include <climits>
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#include "ecmascript/interpreter/interpreter.h"
namespace panda::ecmascript {
bool SamplesRecord::staticGcState_ = false;
SamplesRecord::SamplesRecord()
{
    stackTopLines_.push_back(0);
    struct MethodKey methodkey;
    struct CpuProfileNode methodNode;
    methodkey.method = reinterpret_cast<JSMethod*>(INT_MAX - 1);
    methodMap_.insert(std::make_pair(methodkey, methodMap_.size() + 1));
    methodNode.parentId = 0;
    methodNode.codeEntry.codeType = "JS";
    methodNode.codeEntry.functionName = "(root)";
    methodNode.id = 1;
    profileInfo_ = std::make_unique<struct ProfileInfo>();
    profileInfo_->nodes.push_back(methodNode);
}

SamplesRecord::~SamplesRecord()
{
    if (fileHandle_.is_open()) {
        fileHandle_.close();
    }
}

void SamplesRecord::AddSample(CVector<JSMethod *> sample, uint64_t sampleTimeStamp, bool outToFile)
{
    static int PreviousId = 0;
    struct MethodKey methodkey;
    struct CpuProfileNode methodNode;
    if (staticGcState_) {
        methodkey.method = reinterpret_cast<JSMethod*>(INT_MAX);
        methodNode.parentId = methodkey.parentId = PreviousId;
        auto result = methodMap_.find(methodkey);
        if (result == methodMap_.end()) {
            methodNode.id = static_cast<int>(methodMap_.size() + 1);
            methodMap_.insert(std::make_pair(methodkey, methodNode.id));
            methodNode.codeEntry = GetGcInfo();
            stackTopLines_.push_back(0);
            profileInfo_->nodes.push_back(methodNode);
            if (!outToFile) {
                if (UNLIKELY(methodNode.parentId) == 0) {
                    profileInfo_->nodes[0].children.push_back(methodNode.id);
                } else {
                    profileInfo_->nodes[methodNode.parentId - 1].children.push_back(methodNode.id);
                }
            }
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
                int id = static_cast<int>(methodMap_.size() + 1);
                methodMap_.insert(std::make_pair(methodkey, id));
                PreviousId = methodNode.id = id;
                methodNode.codeEntry = GetMethodInfo(methodkey.method);
                stackTopLines_.push_back(methodNode.codeEntry.lineNumber);
                profileInfo_->nodes.push_back(methodNode);
                if (!outToFile) {
                    profileInfo_->nodes[methodNode.parentId - 1].children.push_back(id);
                }
            } else {
                PreviousId = methodNode.id = result->second;
            }
        }
    }
    static uint64_t threadStartTime = 0;
    struct SampleInfo sampleInfo;
    int sampleNodeId = methodNode.id == 0 ? PreviousId = 1, 1 : methodNode.id;
    int timeDelta = static_cast<int>(sampleTimeStamp -
        (threadStartTime == 0 ? profileInfo_->startTime : threadStartTime));
    if (outToFile) {
        sampleInfo.id = sampleNodeId;
        sampleInfo.line = stackTopLines_[methodNode.id];
        sampleInfo.timeStamp = timeDelta;
        samples_.push_back(sampleInfo);
    } else {
        profileInfo_->nodes[sampleNodeId].hitCount++;
        profileInfo_->samples.push_back(sampleNodeId);
        profileInfo_->timeDeltas.push_back(timeDelta);
    }
    threadStartTime = sampleTimeStamp;
}

void SamplesRecord::WriteAddNodes()
{
    sampleData_ += "{\"args\":{\"data\":{\"cpuProfile\":{\"nodes\":[";
    for (auto it : profileInfo_->nodes) {
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

void SamplesRecord::WriteAddSamples()
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

void SamplesRecord::WriteMethodsAndSampleInfo(bool timeEnd)
{
    if (profileInfo_->nodes.size() >= 10) { // 10:Number of nodes currently stored
        WriteAddNodes();
        WriteAddSamples();
        profileInfo_->nodes.clear();
        samples_.clear();
    } else if (samples_.size() == 100 || timeEnd) { // 100:Number of samples currently stored
        if (!profileInfo_->nodes.empty()) {
            WriteAddNodes();
            WriteAddSamples();
            profileInfo_->nodes.clear();
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
    int64_t tid = syscall(SYS_gettid);
    uint64_t ts = SamplingProcessor::GetMicrosecondsTimeStamp();
    ts = ts % TIME_CHANGE;
    struct timespec time = {0, 0};
    clock_gettime(CLOCK_MONOTONIC, &time);
    uint64_t tts = static_cast<uint64_t>(time.tv_nsec) / 1000; // 1000:Nanoseconds to milliseconds.
    sampleData_ += std::to_string(pid) + ",\"tid\":" +
                   std::to_string(tid) + ",\"ts\":" +
                   std::to_string(ts) + ",\"tts\":" +
                   std::to_string(tts) + "},\n";
}

CVector<struct CpuProfileNode> SamplesRecord::GetMethodNodes() const
{
    return profileInfo_->nodes;
}

CDeque<struct SampleInfo> SamplesRecord::GetSamples() const
{
    return samples_;
}

std::string SamplesRecord::GetSampleData() const
{
    return sampleData_;
}

struct FrameInfo SamplesRecord::GetMethodInfo(JSMethod *method)
{
    struct FrameInfo entry;
    auto iter = CpuProfiler::staticStackInfo_.find(method);
    if (iter != CpuProfiler::staticStackInfo_.end()) {
        entry = iter->second;
    }
    return entry;
}

struct FrameInfo SamplesRecord::GetGcInfo()
{
    struct FrameInfo gcEntry;
    gcEntry.codeType = "jsvm";
    gcEntry.functionName = "garbage collector";
    return gcEntry;
}

void SamplesRecord::SetThreadStartTime(uint64_t threadStartTime)
{
    profileInfo_->startTime = threadStartTime;
}

void SamplesRecord::SetThreadStopTime(uint64_t threadStopTime)
{
    profileInfo_->stopTime = threadStopTime;
}

void SamplesRecord::SetStartsampleData(std::string sampleData)
{
    sampleData_ += sampleData;
}

void SamplesRecord::SetFileName(std::string &fileName)
{
    fileName_ = fileName;
}

const std::string SamplesRecord::GetFileName() const
{
    return fileName_;
}

void SamplesRecord::ClearSampleData()
{
    sampleData_.clear();
}

std::unique_ptr<struct ProfileInfo> SamplesRecord::GetProfileInfo()
{
    return std::move(profileInfo_);
}
} // namespace panda::ecmascript