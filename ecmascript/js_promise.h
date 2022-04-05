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

#ifndef ECMASCRIPT_PROMISE_H
#define ECMASCRIPT_PROMISE_H

#include "ecmascript/accessor_data.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/tagged_queue.h"
#include "ecmascript/tagged_queue.h"

namespace panda::ecmascript {
enum class PromiseState : uint8_t { PENDING = 0, FULFILLED, REJECTED };
enum class PromiseType : uint8_t { RESOLVE = 0, REJECT };
enum class PromiseRejectionEvent : uint8_t { REJECT = 0, HANDLE };

class PromiseReaction final : public Record {
public:
    CAST_CHECK(PromiseReaction, IsPromiseReaction);

    static constexpr size_t PROMISE_CAPABILITY_OFFSET = Record::SIZE;
    ACCESSORS(PromiseCapability, PROMISE_CAPABILITY_OFFSET, HANDLER_OFFSET)
    ACCESSORS(Handler, HANDLER_OFFSET, BIT_FIELD_OFFSET)
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t TYPE_BITS = 1;
    FIRST_BIT_FIELD(BitField, Type, PromiseType, TYPE_BITS)

    DECL_VISIT_OBJECT(PROMISE_CAPABILITY_OFFSET, BIT_FIELD_OFFSET)
    DECL_DUMP()
};

class PromiseCapability final : public Record {
public:
    CAST_CHECK(PromiseCapability, IsPromiseCapability);

    static constexpr size_t PROMISE_OFFSET = Record::SIZE;
    ACCESSORS(Promise, PROMISE_OFFSET, RESOLVE_OFFSET);
    ACCESSORS(Resolve, RESOLVE_OFFSET, REJECT_OFFSET);
    ACCESSORS(Reject, REJECT_OFFSET, SIZE);

    DECL_DUMP()

    DECL_VISIT_OBJECT(PROMISE_OFFSET, SIZE)
};

class PromiseIteratorRecord final : public Record {
public:
    CAST_CHECK(PromiseIteratorRecord, IsPromiseIteratorRecord);

    static constexpr size_t ITERATOR_OFFSET = Record::SIZE;
    ACCESSORS(Iterator, ITERATOR_OFFSET, BIT_FIELD_OFFSET);
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t DONE_BITS = 1;
    FIRST_BIT_FIELD(BitField, Done, bool, DONE_BITS)

    DECL_VISIT_OBJECT(ITERATOR_OFFSET, BIT_FIELD_OFFSET)
    DECL_DUMP()
};

class PromiseRecord final : public Record {
public:
    CAST_CHECK(PromiseRecord, IsPromiseRecord);

    static constexpr size_t VALUE_OFFSET = Record::SIZE;
    ACCESSORS(Value, VALUE_OFFSET, SIZE);
    DECL_DUMP()

    DECL_VISIT_OBJECT(VALUE_OFFSET, SIZE)
};

class ResolvingFunctionsRecord final : public Record {
public:
    CAST_CHECK(ResolvingFunctionsRecord, IsResolvingFunctionsRecord);

    static constexpr size_t RESOLVE_FUNCTION_OFFSET = Record::SIZE;
    ACCESSORS(ResolveFunction, RESOLVE_FUNCTION_OFFSET, REJECT_FUNCTION_OFFSET);
    ACCESSORS(RejectFunction, REJECT_FUNCTION_OFFSET, SIZE);

    DECL_DUMP()

    DECL_VISIT_OBJECT(RESOLVE_FUNCTION_OFFSET, SIZE)
};

class JSPromise final : public JSObject {
public:
    CAST_CHECK(JSPromise, IsJSPromise);

    // ES6 25.4.1.3 CreateResolvingFunctions ( promise )
    static JSHandle<ResolvingFunctionsRecord> CreateResolvingFunctions(JSThread *thread,
                                                                       const JSHandle<JSPromise> &promise);

    // ES6 25.4.1.4 FulfillPromise ( promise, value)
    static JSTaggedValue FulfillPromise(JSThread *thread, const JSHandle<JSPromise> &promise,
                                        const JSHandle<JSTaggedValue> &value);

    // 25.4.1.5 NewPromiseCapability ( C )
    static JSHandle<PromiseCapability> NewPromiseCapability(JSThread *thread, const JSHandle<JSTaggedValue> &obj);

    // ES6 24.4.1.6 IsPromise (x)
    static bool IsPromise(const JSHandle<JSTaggedValue> &value);

    // ES6 25.4.1.7 RejectPromise (promise, reason)
    static JSTaggedValue RejectPromise(JSThread *thread, const JSHandle<JSPromise> &promise,
                                       const JSHandle<JSTaggedValue> &reason);

    // 25.4.1.8 TriggerPromiseReactions (reactions, argument)
    static JSTaggedValue TriggerPromiseReactions(JSThread *thread, const JSHandle<TaggedQueue> &reactions,
                                                 const JSHandle<JSTaggedValue> &argument);

    static JSHandle<JSTaggedValue> IfThrowGetThrowValue(JSThread *thread);

    static constexpr size_t PROMISE_RESULT_OFFSET = JSObject::SIZE;
    ACCESSORS(PromiseResult, PROMISE_RESULT_OFFSET, PROMISE_FULFILL_REACTIONS_OFFSET);
    ACCESSORS(PromiseFulfillReactions, PROMISE_FULFILL_REACTIONS_OFFSET, PROMISE_REJECT_REACTIONS_OFFSET);
    ACCESSORS(PromiseRejectReactions, PROMISE_REJECT_REACTIONS_OFFSET, BIT_FIELD_OFFSET);
    ACCESSORS_BIT_FIELD(BitField, BIT_FIELD_OFFSET, LAST_OFFSET)
    DEFINE_ALIGN_SIZE(LAST_OFFSET);

    // define BitField
    static constexpr size_t PROMISE_STATE_BITS = 2;
    static constexpr size_t PROMISE_IS_HANDLED_BITS = 1;
    FIRST_BIT_FIELD(BitField, PromiseState, PromiseState, PROMISE_STATE_BITS)
    NEXT_BIT_FIELD(BitField, PromiseIsHandled, bool, PROMISE_IS_HANDLED_BITS, PromiseState)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, PROMISE_RESULT_OFFSET, BIT_FIELD_OFFSET)
    DECL_DUMP()
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_PROMISE_H
