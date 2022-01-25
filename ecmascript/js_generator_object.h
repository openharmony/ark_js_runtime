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
enum class JSGeneratorState : uint8_t {
    UNDEFINED = 0,
    SUSPENDED_START,
    SUSPENDED_YIELD,
    EXECUTING,
    COMPLETED,
};
enum class GeneratorResumeMode : uint8_t {
    RETURN = 0,
    THROW,
    NEXT,
    UNDEFINED
};

class GeneratorContext : TaggedObject {
public:
    CAST_CHECK(GeneratorContext, IsGeneratorContext);

    static constexpr size_t GENERATOR_REGS_ARRAY_OFFSET = TaggedObjectSize();
    ACCESSORS(RegsArray, GENERATOR_REGS_ARRAY_OFFSET, GENERATOR_METHOD_OFFSET)
    ACCESSORS(Method, GENERATOR_METHOD_OFFSET, GENERATOR_ACC_OFFSET)
    ACCESSORS(Acc, GENERATOR_ACC_OFFSET, GENERATOR_GENERATOR_OBJECT_OFFSET)
    ACCESSORS(GeneratorObject, GENERATOR_GENERATOR_OBJECT_OFFSET, GENERATOR_LEXICALENV_OFFSET)
    ACCESSORS(LexicalEnv, GENERATOR_LEXICALENV_OFFSET, GENERATOR_NREGS_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(NRegs, uint32_t, GENERATOR_NREGS_OFFSET, GENERATOR_BC_OFFSET_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(BCOffset, uint32_t, GENERATOR_BC_OFFSET_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    DECL_VISIT_OBJECT(GENERATOR_REGS_ARRAY_OFFSET, GENERATOR_NREGS_OFFSET)
    DECL_DUMP()
};

class JSGeneratorObject : public JSObject {
public:
    CAST_CHECK(JSGeneratorObject, IsGeneratorObject);

    static constexpr size_t GENERATOR_CONTEXT_OFFSET = JSObject::SIZE;
    ACCESSORS(GeneratorContext, GENERATOR_CONTEXT_OFFSET, GENERATOR_RESUME_RESULT_OFFSET)
    ACCESSORS(ResumeResult, GENERATOR_RESUME_RESULT_OFFSET, BIT_FIELD_OFFSET)
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t GENERATOE_STATE_BITS = 3;
    static constexpr size_t RESUME_MODE_BITS = 3;
    FIRST_BIT_FIELD(BitField, GeneratorState, JSGeneratorState, GENERATOE_STATE_BITS)
    NEXT_BIT_FIELD(BitField, ResumeMode, GeneratorResumeMode, RESUME_MODE_BITS, GeneratorState)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, GENERATOR_CONTEXT_OFFSET, BIT_FIELD_OFFSET)
    DECL_DUMP()

    // 26.4.3.2 GeneratorValidate(generator)
    static JSGeneratorState GeneratorValidate(JSThread *thread, const JSHandle<JSTaggedValue> &obj);

    // 26.4.3.3 GeneratorResume(generator, value)
    static JSHandle<JSObject> GeneratorResume(JSThread *thread, const JSHandle<JSGeneratorObject> &generator,
                                              JSTaggedValue value);

    // 26.4.3.4 GeneratorResumeAbrupt(generator, abruptCompletion)
    static JSHandle<JSObject> GeneratorResumeAbrupt(JSThread *thread, const JSHandle<JSGeneratorObject> &generator,
                                                    const JSHandle<CompletionRecord> &abruptCompletion);

    inline bool IsSuspendYield() const
    {
        return GetGeneratorState() == JSGeneratorState::SUSPENDED_YIELD;
    }

    inline bool IsExecuting() const
    {
        return GetGeneratorState() == JSGeneratorState::EXECUTING;
    }
};

class JSAsyncFuncObject : public JSGeneratorObject {
public:
    CAST_CHECK(JSAsyncFuncObject, IsAsyncFuncObject);

    static constexpr size_t GENERATOR_PROMISE_OFFSET = JSGeneratorObject::SIZE;
    ACCESSORS(Promise, GENERATOR_PROMISE_OFFSET, SIZE);

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSGeneratorObject, GENERATOR_PROMISE_OFFSET, SIZE)
    DECL_DUMP()
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_JS_GENERATOR_OBJECT_H
