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

#ifndef ECMASCRIPT_JOBS_MICRO_JOB_QUEUE_H
#define ECMASCRIPT_JOBS_MICRO_JOB_QUEUE_H

#include "ecmascript/ecma_string.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/record.h"
#include "ecmascript/tagged_array.h"

namespace panda::ecmascript::job {
enum class QueueType : uint8_t {
    QUEUE_PROMISE,
    QUEUE_SCRIPT,
};

class MicroJobQueue final : public Record {
public:
    static MicroJobQueue *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsMicroJobQueue());
        return static_cast<MicroJobQueue *>(object);
    }

    static void EnqueueJob(JSThread *thread, JSHandle<MicroJobQueue> jobQueue, QueueType queueType,
        const JSHandle<JSFunction> &job, const JSHandle<TaggedArray> &argv);
    static void ExecutePendingJob(JSThread *thread, JSHandle<MicroJobQueue> jobQueue);

    static constexpr size_t PROMISE_JOB_QUEUE_OFFSET = Record::SIZE;
    ACCESSORS(PromiseJobQueue, PROMISE_JOB_QUEUE_OFFSET, SCRIPT_JOB_QUEUE_OFFSET);
    ACCESSORS(ScriptJobQueue, SCRIPT_JOB_QUEUE_OFFSET, SIZE);

    DECL_DUMP()

    DECL_VISIT_OBJECT(PROMISE_JOB_QUEUE_OFFSET, SIZE)
};
}  // namespace panda::ecmascript::job
#endif  // ECMASCRIPT_JOBS_MICRO_JOB_QUEUE_H
