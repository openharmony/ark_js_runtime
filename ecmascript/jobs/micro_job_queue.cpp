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

#include "ecmascript/jobs/micro_job_queue.h"

#include "ecmascript/global_env.h"
#include "ecmascript/jobs/pending_job.h"
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_queue-inl.h"
#include "ecmascript/tagged_queue.h"
#ifndef PANDA_TARGET_LINUX
#include "hitrace/hitrace.h"
#include "hitrace/hitraceid.h"
#endif
#include "utils/expected.h"

namespace panda::ecmascript::job {
#ifndef PANDA_TARGET_LINUX
using namespace OHOS::HiviewDFX;
#endif
void MicroJobQueue::EnqueueJob(JSThread *thread, JSHandle<MicroJobQueue> jobQueue, QueueType queueType,
    const JSHandle<JSFunction> &job, const JSHandle<TaggedArray> &argv)
{
    // 1. Assert: Type(queueName) is String and its value is the name of a Job Queue recognized by this implementation.
    // 2. Assert: job is the name of a Job.
    // 3. Assert: arguments is a List that has the same number of elements as the number of parameters required by job.
    // 4. Let callerContext be the running execution context.
    // 5. Let callerRealm be callerContext’s Realm.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<PendingJob> pendingJob(factory->NewPendingJob(job, argv));
#ifndef PANDA_TARGET_LINUX
    HiTraceId id = HiTrace::GetId();
    if (id.IsValid() && id.IsFlagEnabled(HITRACE_FLAG_INCLUDE_ASYNC)) {
        HiTraceId childId = HiTrace::CreateSpan();

        pendingJob->SetChainId(childId.GetChainId());
        pendingJob->SetSpanId(childId.GetSpanId());
        pendingJob->SetParentSpanId(childId.GetParentSpanId());
        pendingJob->SetFlags(childId.GetFlags());

        if (id.IsFlagEnabled(HITRACE_FLAG_TP_INFO)) {
            if (queueType == QueueType::QUEUE_PROMISE) {
                HiTrace::Tracepoint(HITRACE_CM_THREAD, HITRACE_TP_CS,
                                    childId, "Queue type:%s", "Promise queue");
            } else if (queueType == QueueType::QUEUE_SCRIPT) {
                HiTrace::Tracepoint(HITRACE_CM_THREAD, HITRACE_TP_CS,
                                    childId, "Queue type:%s", "Script queue");
            } else {
                HiTrace::Tracepoint(HITRACE_CM_THREAD, HITRACE_TP_CS,
                                    childId, "Queue type:%s", "Other queue");
            }
        }
    }
#endif
    if (queueType == QueueType::QUEUE_PROMISE) {
        JSHandle<TaggedQueue> promiseQueue(thread, jobQueue->GetPromiseJobQueue());
        LOG_ECMA(DEBUG) << "promiseQueue start length: " << promiseQueue->Size();
        TaggedQueue *newPromiseQueue = TaggedQueue::Push(thread, promiseQueue, JSHandle<JSTaggedValue>(pendingJob));
        jobQueue->SetPromiseJobQueue(thread, JSTaggedValue(newPromiseQueue));
        LOG_ECMA(DEBUG) << "promiseQueue end length: " << newPromiseQueue->Size();
    } else if (queueType == QueueType::QUEUE_SCRIPT) {
        JSHandle<TaggedQueue> scriptQueue(thread, jobQueue->GetScriptJobQueue());
        TaggedQueue *newScriptQueue = TaggedQueue::Push(thread, scriptQueue, JSHandle<JSTaggedValue>(pendingJob));
        jobQueue->SetScriptJobQueue(thread, JSTaggedValue(newScriptQueue));
    }
}

void MicroJobQueue::ExecutePendingJob(JSThread *thread, JSHandle<MicroJobQueue> jobQueue)
{
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSMutableHandle<TaggedQueue> promiseQueue(thread, jobQueue->GetPromiseJobQueue());
    JSMutableHandle<PendingJob> pendingJob(thread, JSTaggedValue::Undefined());
    while (!promiseQueue->Empty()) {
        LOG_ECMA(DEBUG) << "promiseQueue start length: " << promiseQueue->Size();
        pendingJob.Update(promiseQueue->Pop(thread));
        LOG_ECMA(DEBUG) << "promiseQueue end length: " << promiseQueue->Size();
#ifndef PANDA_TARGET_LINUX
        HiTraceId hitraceId;
        if (pendingJob->GetChainId() != 0) {
            hitraceId.SetChainId(pendingJob->GetChainId());
            hitraceId.SetSpanId(pendingJob->GetSpanId());
            hitraceId.SetParentSpanId(pendingJob->GetParentSpanId());
            hitraceId.SetFlags(pendingJob->GetFlags());

            if (hitraceId.IsValid()) {
                HiTrace::SetId(hitraceId);
                if (hitraceId.IsFlagEnabled(HITRACE_FLAG_TP_INFO)) {
                    HiTrace::Tracepoint(HITRACE_CM_THREAD, HITRACE_TP_SR,
                                    hitraceId, "Before %s queue job execute", "Promise");
                }
            }
        }
#endif
        PendingJob::ExecutePendingJob(pendingJob, thread);
#ifndef PANDA_TARGET_LINUX
        if (pendingJob->GetChainId() != 0) {
            if (hitraceId.IsValid()) {
                if (hitraceId.IsFlagEnabled(HITRACE_FLAG_TP_INFO)) {
                    HiTrace::Tracepoint(HITRACE_CM_THREAD, HITRACE_TP_SS,
                                    hitraceId, "After %s queue job execute", "Promise");
                }
                HiTrace::ClearId();
            }
        }
#endif
        if (thread->HasPendingException()) {
            return;
        }
        promiseQueue.Update(jobQueue->GetPromiseJobQueue());
    }

    JSHandle<TaggedQueue> scriptQueue(thread, jobQueue->GetScriptJobQueue());
    while (!scriptQueue->Empty()) {
        pendingJob.Update(scriptQueue->Pop(thread));
#ifndef PANDA_TARGET_LINUX
        HiTraceId hitraceId;
        if (pendingJob->GetChainId() != 0) {
            hitraceId.SetChainId(pendingJob->GetChainId());
            hitraceId.SetSpanId(pendingJob->GetSpanId());
            hitraceId.SetParentSpanId(pendingJob->GetParentSpanId());
            hitraceId.SetFlags(pendingJob->GetFlags());

            if (hitraceId.IsValid()) {
                HiTrace::SetId(hitraceId);
                if (hitraceId.IsFlagEnabled(HITRACE_FLAG_TP_INFO)) {
                    HiTrace::Tracepoint(HITRACE_CM_THREAD, HITRACE_TP_SR,
                                    hitraceId, "Before %s queue job execute", "Script");
                }
            }
        }
#endif
        PendingJob::ExecutePendingJob(pendingJob, thread);
#ifndef PANDA_TARGET_LINUX
        if (pendingJob->GetChainId() != 0) {
            if (hitraceId.IsValid()) {
                if (hitraceId.IsFlagEnabled(HITRACE_FLAG_TP_INFO)) {
                    HiTrace::Tracepoint(HITRACE_CM_THREAD, HITRACE_TP_SS,
                                    hitraceId, "After %s queue job execute", "Script");
                }
                HiTrace::ClearId();
            }
        }
#endif
        if (thread->HasPendingException()) {
            return;
        }
    }
}
}  // namespace panda::ecmascript::job
