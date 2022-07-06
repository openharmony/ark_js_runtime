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
#include "ecmascript/dfx/cpu_profiler/sampling_processor.h"

#include <csignal>
#include <sys/time.h>
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#include "ecmascript/interpreter/interpreter.h"

namespace panda::ecmascript {
JSThread *SamplingProcessor::thread_ = nullptr;
bool SamplingProcessor::isStart_ = false;
SamplingProcessor::SamplingProcessor(SamplesRecord *generator, const EcmaVM *vm, int interval, bool outToFile)
{
    generator_ = generator;
    interval_ = interval;
    pid_ = pthread_self();
    thread_ = vm->GetAssociatedJSThread();
    outToFile_ = outToFile;
}
SamplingProcessor::~SamplingProcessor() {}

bool SamplingProcessor::Run([[maybe_unused]] uint32_t threadIndex)
{
    uint64_t startTime = 0;
    uint64_t endTime = 0;
    startTime = GetMicrosecondsTimeStamp();
    generator_->SetThreadStartTime(startTime);
    while (isStart_) {
        static int collectCount = 1;
#if ECMASCRIPT_ENABLE_ACTIVE_CPUPROFILER
        JSThread *thread = GetJSThread();
        SamplesRecord::staticGcState_ = thread->GetGcState();
        if (!SamplesRecord::staticGcState_) {
            thread->SetGetStackSignal(true);
            if (sem_wait(&CpuProfiler::sem_[0]) != 0) {
                LOG_ECMA(ERROR) << "sem_[0] wait failed";
            }
        }
#else
        if (pthread_kill(pid_, SIGINT) != 0) {
            LOG_ECMA(ERROR) << "pthread_kill signal failed";
            return false;
        }
        if (sem_wait(&CpuProfiler::sem_[0]) != 0) {
            LOG_ECMA(ERROR) << "sem_[0] wait failed";
            return false;
        }
#endif
        endTime = GetMicrosecondsTimeStamp();
        generator_->AddSample(CpuProfiler::staticFrameStack_, endTime, outToFile_);
        if (outToFile_) {
            if (generator_->GetMethodNodes().size() >= 10 || // 10:Number of nodes currently stored
                generator_->GetSamples().size() == 100) { // 100:Number of Samples currently stored
                generator_->WriteMethodsAndSampleInfo(false);
            }
        }
        int64_t ts = interval_ - static_cast<int64_t>(endTime - startTime);
        if (ts > 0) {
            usleep(ts);
        }
        startTime = GetMicrosecondsTimeStamp();
        if (outToFile_) {
            if (collectCount % 50 == 0) { // 50:The sampling times reached 50 times.
                WriteSampleDataToFile();
            }
            collectCount++;
        }
    }
    uint64_t stopTime = GetMicrosecondsTimeStamp();
    generator_->SetThreadStopTime(stopTime);
    if (sem_post(&CpuProfiler::sem_[1]) != 0) {
        LOG_ECMA(ERROR) << "sem_[1] post failed";
        return false;
    }
    return true;
}

uint64_t SamplingProcessor::GetMicrosecondsTimeStamp()
{
    struct timeval time;
    gettimeofday(&time, nullptr);
    return (time.tv_sec * 1000000 + time.tv_usec); // 1000000:Second to subtle
}

JSThread *SamplingProcessor::GetJSThread()
{
    return thread_;
}

void SamplingProcessor::SetIsStart(bool isStart)
{
    isStart_ = isStart;
}

void SamplingProcessor::WriteSampleDataToFile()
{
    static int flag = 0;
    if (!generator_->fileHandle_.is_open() || generator_->GetSampleData().size() < 1024 * 1024) { // 1024 * 1024:1M
        return;
    }
    if (flag == 0) {
        generator_->fileHandle_ << generator_->GetSampleData();
        generator_->ClearSampleData();
        generator_->fileHandle_.close();
        generator_->fileHandle_.open(generator_->GetFileName().c_str(), std::ios::app);
        flag++;
        return;
    }
    generator_->fileHandle_ << generator_->GetSampleData();
    generator_->ClearSampleData();
}
} // namespace panda::ecmascript