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

#ifndef ECMASCRIPT_JS_GENERATOR_OBJECT_H
#define ECMASCRIPT_JS_GENERATOR_OBJECT_H

#include "ecmascript/js_function.h"

namespace panda {
namespace ecmascript {
enum class JSGeneratorState {
    UNDEFINED = 0,
    SUSPENDED_START,
    SUSPENDED_YIELD,
    EXECUTING,
    COMPLETED,
};

class GeneratorContext : TaggedObject {
public:
    static GeneratorContext *Cast(ObjectHeader *object)
    {
        ASSERT(!JSTaggedValue(object).IsJSHClass());
        return static_cast<GeneratorContext *>(object);
    }

    static constexpr size_t GENERATOR_REGS_ARRAY_OFFSET = TaggedObjectSize();
    ACCESSORS(RegsArray, GENERATOR_REGS_ARRAY_OFFSET, GENERATOR_METHOD_OFFSET)
    ACCESSORS(Method, GENERATOR_METHOD_OFFSET, GENERATOR_ACC_OFFSET)
    ACCESSORS(Acc, GENERATOR_ACC_OFFSET, GENERATOR_NREGS_OFFSET)
    ACCESSORS(NRegs, GENERATOR_NREGS_OFFSET, GENERATOR_BC_OFFSET_OFFSET)
    ACCESSORS(BCOffset, GENERATOR_BC_OFFSET_OFFSET, GENERATOR_GENERATOR_OBJECT_OFFSET)
    ACCESSORS(GeneratorObject, GENERATOR_GENERATOR_OBJECT_OFFSET, GENERATOR_LEXICALENV_OFFSET)
    ACCESSORS(LexicalEnv, GENERATOR_LEXICALENV_OFFSET, SIZE)

    DECL_VISIT_OBJECT(GENERATOR_REGS_ARRAY_OFFSET, SIZE)
};

class JSGeneratorObject : public JSObject {
public:
    static JSGeneratorObject *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsGeneratorObject());
        return static_cast<JSGeneratorObject *>(object);
    }

    static constexpr size_t GENERATOR_STATE_OFFSET = JSObject::SIZE;
    ACCESSORS(GeneratorState, GENERATOR_STATE_OFFSET, GENERATOR_CONTEXT_OFFSET)
    ACCESSORS(GeneratorContext, GENERATOR_CONTEXT_OFFSET, GENERATOR_RESUME_RESULT_OFFSET)
    ACCESSORS(ResumeResult, GENERATOR_RESUME_RESULT_OFFSET, GENERATOR_RESUME_MODE_OFFSET)
    ACCESSORS(ResumeMode, GENERATOR_RESUME_MODE_OFFSET, SIZE)

    // 26.4.3.2 GeneratorValidate(generator)
    static JSTaggedValue GeneratorValidate(JSThread *thread, const JSHandle<JSTaggedValue> &obj);

    // 26.4.3.3 GeneratorResume(generator, value)
    static JSHandle<JSObject> GeneratorResume(JSThread *thread, const JSHandle<JSGeneratorObject> &generator,
                                              JSTaggedValue value);

    // 26.4.3.4 GeneratorResumeAbrupt(generator, abruptCompletion)
    static JSHandle<JSObject> GeneratorResumeAbrupt(JSThread *thread, const JSHandle<JSGeneratorObject> &generator,
                                                    const JSHandle<CompletionRecord> &abruptCompletion);

    inline bool IsSuspendYield() const
    {
        return GetGeneratorState() == JSTaggedValue(static_cast<int32_t>(JSGeneratorState::SUSPENDED_YIELD));
    }

    inline bool IsExecuting() const
    {
        return GetGeneratorState() == JSTaggedValue(static_cast<int32_t>(JSGeneratorState::EXECUTING));
    }

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, GENERATOR_STATE_OFFSET, SIZE)
};

class JSAsyncFuncObject : public JSGeneratorObject {
public:
    static JSAsyncFuncObject *Cast(ObjectHeader *object)
    {
        ASSERT(JSTaggedValue(object).IsAsyncFuncObject());
        return static_cast<JSAsyncFuncObject *>(object);
    }

    static constexpr size_t GENERATOR_PROMISE_OFFSET = JSGeneratorObject::SIZE;
    ACCESSORS(Promise, GENERATOR_PROMISE_OFFSET, SIZE);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSGeneratorObject, GENERATOR_PROMISE_OFFSET, SIZE)
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_JS_GENERATOR_OBJECT_H
