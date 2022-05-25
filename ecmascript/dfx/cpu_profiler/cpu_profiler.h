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

#ifndef ECMASCRIPT_CPU_PROFILER_H
#define ECMASCRIPT_CPU_PROFILER_H

#include <semaphore.h>

#include "ecmascript/dfx/cpu_profiler/samples_record.h"
#include "ecmascript/dfx/cpu_profiler/sampling_processor.h"
#include "os/mutex.h"

namespace panda::ecmascript {
struct CurrentProcessInfo {
    uint64_t nowTimeStamp = 0;
    uint64_t tts = 0;
    pid_t pid = 0;
    pthread_t tid = 0;
};

class GcStateScope {
public:
    inline explicit GcStateScope(JSThread *thread)
    {
        thread_ = thread;
        thread_->SetGcState(true);
    }

    inline ~GcStateScope()
    {
        thread_->SetGcState(false);
    }
private:
    JSThread *thread_ = nullptr;
};

class CpuProfiler {
public:
    static CpuProfiler *GetInstance();
    static void ParseMethodInfo(JSMethod *method, FrameHandler frameHandler);
    static void GetFrameStack(JSThread *thread);
    static void IsNeedAndGetStack(JSThread *thread);
    static void GetStackSignalHandler(int signal);

    static CMap<JSMethod *, struct FrameInfo> staticStackInfo_;
    static sem_t sem_;
    static CVector<JSMethod *> staticFrameStack_;

    void StartCpuProfilerForInfo(const EcmaVM *vm);
    std::unique_ptr<struct ProfileInfo> StopCpuProfilerForInfo();
    void StartCpuProfilerForFile(const EcmaVM *vm, const std::string &fileName);
    void StopCpuProfilerForFile();
    std::string GetProfileName() const;
    virtual ~CpuProfiler();

private:
    static CMap<std::string, int> scriptIdMap_;
    static std::atomic<CpuProfiler*> singleton_;
    static os::memory::Mutex synchronizationMutex_;

    explicit CpuProfiler();
    void SetProfileStart(uint64_t nowTimeStamp);
    void GetCurrentProcessInfo(struct CurrentProcessInfo &currentProcessInfo);
    bool CheckFileName(const std::string &fileName, std::string &absoluteFilePath) const;

    bool isProfiling_ = false;
    bool outToFile_ = false;
    int interval_ = 500; // 500:Sampling interval 500 microseconds
    std::string fileName_ = "";
    SamplesRecord *generator_ = nullptr;
    pthread_t tid_ = 0;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_CPU_PROFILE_H