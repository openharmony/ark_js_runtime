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

#include "ecmascript/cpu_profiler/profile_generator.h"
#include "ecmascript/cpu_profiler/profile_processor.h"

namespace panda::ecmascript {
struct CurrentProcessInfo {
    time_t nowTimeStamp = 0;
    time_t tts = 0;
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
    static void ParseMethodInfo(JSMethod *method, JSThread *thread, InterpretedFrameHandler frameHandler);
    static void GetFrameStack(JSThread *thread);
    static void IsNeedAndGetStack(JSThread *thread);
    static void GetStackSignalHandler(int signal);

    static CMap<JSMethod *, struct StackInfo> staticStackInfo_;
    static sem_t sem_;
    static CVector<JSMethod *> staticFrameStack_;

    void StartCpuProfiler(const EcmaVM *vm, const std::string &fileName);
    void StopCpuProfiler();
    std::string GetProfileName() const;
    virtual ~CpuProfiler();
private:
    static CMap<std::string, int> scriptIdMap_;
    static CpuProfiler *singleton_;

    explicit CpuProfiler();
    void SetProfileStart(time_t nowTimeStamp);
    void GetCurrentProcessInfo(struct CurrentProcessInfo &currentProcessInfo) const;
    bool CheckFileName(const std::string &fileName) const;

    bool isOnly_ = false;
    int interval_ = 500; // 500:Sampling interval 500 microseconds
    std::string fileName_ = "";
    ProfileGenerator *generator_ = nullptr;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_CPU_PROFILE_H