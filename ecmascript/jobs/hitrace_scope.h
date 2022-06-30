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

#ifndef ECMASCRIPT_JOBS_HIREACE_SCOPE_H
#define ECMASCRIPT_JOBS_HIREACE_SCOPE_H

#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/js_handle.h"
#if defined(ENABLE_HITRACE)
#include "hitrace/hitraceid.h"
#endif

namespace panda::ecmascript::job {
#if defined(ENABLE_HITRACE)
using namespace OHOS::HiviewDFX;
#endif
class PendingJob;
class EnqueueJobScope {
public:
    explicit EnqueueJobScope(const JSHandle<PendingJob> &pendingJob, job::QueueType queueType);

    ~EnqueueJobScope();
};

class ExecuteJobScope {
public:
    explicit ExecuteJobScope(const JSHandle<PendingJob> &pendingJob);

    ~ExecuteJobScope();
private:
#if defined(ENABLE_HITRACE)
    HiTraceId saveId_;
    HiTraceId hitraceId_;
#endif
};
}  // namespace panda::ecmascript::job
#endif  // ECMASCRIPT_JOBS_HIREACE_SCOPE_H
