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

#ifndef ECMASCRIPT_CPU_PROCESSOR_H
#define ECMASCRIPT_CPU_PROCESSOR_H
#include <pthread.h>

#include "ecmascript/dfx/cpu_profiler/profile_generator.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/taskpool/task.h"

namespace panda::ecmascript {
class ProfileProcessor : public Task {
public:
    static uint64_t GetMicrosecondsTimeStamp();
    static JSThread *GetJSThread();
    static void SetIsStart(bool isStart);

    explicit ProfileProcessor(ProfileGenerator *generator, const EcmaVM *vm, int interval);
    virtual ~ProfileProcessor();

    bool Run(uint32_t threadIndex) override;
    void WriteSampleDataToFile();

    NO_COPY_SEMANTIC(ProfileProcessor);
    NO_MOVE_SEMANTIC(ProfileProcessor);
private:
    static JSThread *thread_;
    static bool isStart_;
    ProfileGenerator *generator_ = nullptr;
    int interval_ = 0;
    pthread_t pid_ = 0;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_CPU_PROCESSOR_H