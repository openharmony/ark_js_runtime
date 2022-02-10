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

#ifndef ECMASCRIPT_ECMA_MACROS_H
#define ECMASCRIPT_ECMA_MACROS_H

#include "ecmascript/common.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/mem/slots.h"
#include "utils/logger.h"

#if (!defined PANDA_TARGET_LINUX) && (defined IS_PUBLIC_VERSION)
    #include "bytrace.h"
#endif

#if defined(__cplusplus)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define LOG_ECMA(type) \
    LOG(type, ECMASCRIPT) << __func__ << " Line:" << __LINE__ << " "  // NOLINT(bugprone-lambda-function-name)

#define ECMA_GC_LOG() LOG(DEBUG, ECMASCRIPT) << " ecmascript gc log: "

#define OPTIONAL_LOG(ecmaVM, level, component) \
    LOG_IF(ecmaVM->IsOptionalLogEnabled(), level, component)

#if (!defined PANDA_TARGET_LINUX) && (defined IS_PUBLIC_VERSION)
    #define ECMA_BYTRACE_NAME(tag, name)                            \
        BYTRACE_NAME(tag, name);                                    \
        trace::ScopedTrace scopedTrace(name)
#else
    #define ECMA_BYTRACE_NAME(tag, name) trace::ScopedTrace scopedTrace(name)
#endif

/* Note: We can't statically decide the element type is a primitive or heap object, especially for */
/*       dynamically-typed languages like JavaScript. So we simply skip the read-barrier.          */
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_VALUE(addr, offset) Barriers::GetDynValue<JSTaggedType>((addr), (offset))

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SET_VALUE_WITH_BARRIER(thread, addr, offset, value)                          \
    if ((value).IsHeapObject()) {                                                    \
        Barriers::SetDynObject<true>(thread, addr, offset, (value).GetRawData());    \
    } else {                                                                         \
        Barriers::SetDynPrimitive<JSTaggedType>(addr, offset, (value).GetRawData()); \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SET_VALUE_PRIMITIVE(addr, offset, value) \
    Barriers::SetDynPrimitive<JSTaggedType>(this, offset, (value).GetRawData())

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ACCESSORS(name, offset, endOffset)                                                                    \
    static constexpr size_t endOffset = (offset) + JSTaggedValue::TaggedTypeSize();                           \
    JSTaggedValue Get##name() const                                                                           \
    {                                                                                                         \
        /* Note: We can't statically decide the element type is a primitive or heap object, especially for */ \
        /*       dynamically-typed languages like JavaScript. So we simply skip the read-barrier.          */ \
        return JSTaggedValue(Barriers::GetDynValue<JSTaggedType>(this, offset));                              \
    }                                                                                                         \
    template<typename T>                                                                                      \
    void Set##name(const JSThread *thread, JSHandle<T> value, BarrierMode mode = WRITE_BARRIER)               \
    {                                                                                                         \
        if (mode == WRITE_BARRIER) {                                                                          \
            if (value.GetTaggedValue().IsHeapObject()) {                                                      \
                Barriers::SetDynObject<true>(thread, this, offset, value.GetTaggedValue().GetRawData());      \
            } else {                                                                                          \
                Barriers::SetDynPrimitive<JSTaggedType>(this, offset, value.GetTaggedValue().GetRawData());   \
            }                                                                                                 \
        } else {                                                                                              \
            Barriers::SetDynPrimitive<JSTaggedType>(this, offset, value.GetTaggedValue().GetRawData());       \
        }                                                                                                     \
    }                                                                                                         \
    void Set##name(const JSThread *thread, JSTaggedValue value, BarrierMode mode = WRITE_BARRIER)             \
    {                                                                                                         \
        if (mode == WRITE_BARRIER) {                                                                          \
            if (value.IsHeapObject()) {                                                                       \
                Barriers::SetDynObject<true>(thread, this, offset, value.GetRawData());                       \
            } else {                                                                                          \
                Barriers::SetDynPrimitive<JSTaggedType>(this, offset, value.GetRawData());                    \
            }                                                                                                 \
        } else {                                                                                              \
            Barriers::SetDynPrimitive<JSTaggedType>(this, offset, value.GetRawData());                        \
        }                                                                                                     \
    }                                                                                                         \
    void Set##name(JSTaggedValue value)                                                                       \
    {                                                                                                         \
        Barriers::SetDynPrimitive<JSTaggedType>(this, offset, value.GetRawData());                            \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DEFINE_ALIGN_SIZE(offset) \
    static constexpr size_t SIZE = ((offset) + sizeof(JSTaggedType) - 1U) & (~(sizeof(JSTaggedType) - 1U))

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ACCESSORS_FIXED_SIZE_FIELD(name, type, sizeType, offset, endOffset) \
    static_assert(sizeof(type) <= sizeof(sizeType));                        \
    static constexpr size_t endOffset = (offset) + sizeof(sizeType);        \
    inline void Set##name(type value)                                       \
    {                                                                       \
        Barriers::SetDynPrimitive<type>(this, offset, value);               \
    }                                                                       \
    inline type Get##name() const                                           \
    {                                                                       \
        return Barriers::GetDynValue<type>(this, offset);                   \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ACCESSORS_NATIVE_FIELD(name, type, offset, endOffset) \
    ACCESSORS_FIXED_SIZE_FIELD(name, type *, type *, offset, endOffset)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ACCESSORS_PRIMITIVE_FIELD(name, type, offset, endOffset) \
    ACCESSORS_FIXED_SIZE_FIELD(name, type, type, offset, endOffset)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ACCESSORS_BIT_FIELD(name, offset, endOffset)                        \
    ACCESSORS_FIXED_SIZE_FIELD(name, uint32_t, uint32_t, offset, endOffset) \
    inline void Clear##name()                                               \
    {                                                                       \
        Set##name(0UL);                                                     \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SET_GET_BIT_FIELD(bitFieldName, name, type)                    \
    inline type Get##name() const                                      \
    {                                                                  \
        return name##Bits::Decode(Get##bitFieldName());                \
    }                                                                  \
    inline void Set##name(type t)                                      \
    {                                                                  \
        Set##bitFieldName(name##Bits::Update(Get##bitFieldName(), t)); \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define FIRST_BIT_FIELD(bitFieldName, name, type, bits) \
    using name##Bits = BitField<type, 0, bits>;         \
    SET_GET_BIT_FIELD(bitFieldName, name, type)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NEXT_BIT_FIELD(bitFieldName, name, type, bits, lastName) \
    using name##Bits = lastName##Bits::NextField<type, bits>;    \
    SET_GET_BIT_FIELD(bitFieldName, name, type)

#if !defined(NDEBUG)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DASSERT(cond) assert(cond)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DASSERT_PRINT(cond, message)                     \
    if (auto cond_val = (cond); UNLIKELY(!(cond_val))) { \
        std::cerr << (message) << std::endl;             \
        ASSERT(#cond &&cond_val);                        \
    }
#else                                                      // NDEBUG
#define DASSERT(cond) static_cast<void>(0)                 // NOLINT(cppcoreguidelines-macro-usage)
#define DASSERT_PRINT(cond, message) static_cast<void>(0)  // NOLINT(cppcoreguidelines-macro-usage)
#endif                                                     // !NDEBUG

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RASSERT(cond) assert(cond)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RASSERT_PRINT(cond, message)                   \
    if (auto cond_val = cond; UNLIKELY(!(cond_val))) { \
        std::cerr << message << std::endl;             \
        RASSERT(#cond &&cond_val);                     \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_IF_ABRUPT_COMPLETION(thread)  \
    do {                                     \
        if (thread->HasPendingException()) { \
            return;                          \
        }                                    \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, value) \
    do {                                                 \
        if ((thread)->HasPendingException()) {           \
            return (value);                              \
        }                                                \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread) \
    do {                                              \
        if ((thread)->HasPendingException()) {        \
            return JSTaggedValue::Exception();        \
        }                                             \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_HANDLE_IF_ABRUPT_COMPLETION(type, thread)               \
    do {                                                               \
        if ((thread)->HasPendingException()) {                         \
            return JSHandle<type>(thread, JSTaggedValue::Exception()); \
        }                                                              \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ASSERT_NO_ABRUPT_COMPLETION(thread) ASSERT(!(thread)->HasPendingException());

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, value) \
    do {                                                       \
        if (!(thread)->HasPendingException()) {                \
            (thread)->SetException(error);                     \
        }                                                      \
        return (value);                                        \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_NEW_ERROR_AND_RETURN_EXCEPTION(thread, error) \
    THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error, JSTaggedValue::Exception());

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define SET_DATE_VALUE(name, code, isLocal)                                                       \
    static JSTaggedValue name(EcmaRuntimeCallInfo *argv)                                          \
    {                                                                                             \
        ASSERT(argv);                                                                             \
        JSThread *thread = argv->GetThread();                                                     \
        JSHandle<JSTaggedValue> msg = GetThis(argv);                                              \
        if (!msg->IsDate()) {                                                                     \
            THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception()); \
        }                                                                                         \
        JSHandle<JSDate> jsDate(thread, JSDate::Cast(msg->GetTaggedObject()));                    \
        JSTaggedValue result = jsDate->SetDateValue(argv, code, isLocal);                         \
        RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);                                            \
        jsDate->SetTimeValue(thread, result);                                                     \
        return result;                                                                            \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DATE_TO_STRING(name)                                                                      \
    static JSTaggedValue name(EcmaRuntimeCallInfo *argv)                                          \
    {                                                                                             \
        ASSERT(argv);                                                                             \
        JSThread *thread = argv->GetThread();                                                     \
        JSHandle<JSTaggedValue> msg = GetThis(argv);                                              \
        if (!msg->IsDate()) {                                                                     \
            THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception()); \
        }                                                                                         \
        if (std::isnan(JSDate::Cast(msg->GetTaggedObject())->GetTimeValue().GetDouble())) {       \
            THROW_RANGE_ERROR_AND_RETURN(thread, "range error", JSTaggedValue::Exception());      \
        }                                                                                         \
        return JSDate::Cast(msg->GetTaggedObject())->name(thread);                                \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DATE_STRING(name)                                                                                          \
    static JSTaggedValue name(EcmaRuntimeCallInfo *argv)                                                           \
    {                                                                                                              \
        ASSERT(argv);                                                                                              \
        JSThread *thread = argv->GetThread();                                                                      \
        JSHandle<JSTaggedValue> msg = GetThis(argv);                                                               \
        if (!msg->IsDate()) {                                                                                      \
            THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception());                  \
        }                                                                                                          \
        if (std::isnan(JSDate::Cast(msg->GetTaggedObject())->GetTimeValue().GetDouble())) {                        \
            return thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("Invalid Date").GetTaggedValue(); \
        }                                                                                                          \
        return JSDate::Cast(msg->GetTaggedObject())->name(thread);                                                 \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define GET_DATE_VALUE(name, code, isLocal)                                                       \
    static JSTaggedValue name(EcmaRuntimeCallInfo *argv)                                          \
    {                                                                                             \
        ASSERT(argv);                                                                             \
        JSThread *thread = argv->GetThread();                                                     \
        JSHandle<JSTaggedValue> msg = GetThis(argv);                                              \
        if (!msg->IsDate()) {                                                                     \
            THROW_TYPE_ERROR_AND_RETURN(thread, "Not a Date Object", JSTaggedValue::Exception()); \
        }                                                                                         \
        JSHandle<JSDate> jsDate(thread, JSDate::Cast(msg->GetTaggedObject()));                    \
        double result = jsDate->GetDateValue(jsDate->GetTimeValue().GetDouble(), code, isLocal);  \
        return GetTaggedDouble(result);                                                           \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_NEW_ERROR_AND_RETURN(thread, error) \
    do {                                          \
        if (!(thread)->HasPendingException()) {   \
            (thread)->SetException(error);        \
        }                                         \
        return;                                   \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_TYPE_ERROR_AND_RETURN(thread, message, exception)                               \
    do {                                                                                      \
        ObjectFactory *objectFactory = (thread)->GetEcmaVM()->GetFactory();                   \
        JSHandle<JSObject> error = objectFactory->GetJSError(ErrorType::TYPE_ERROR, message); \
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error.GetTaggedValue(), exception);          \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_TYPE_ERROR(thread, message)                                               \
    do {                                                                                \
        ObjectFactory *factory = (thread)->GetEcmaVM()->GetFactory();                   \
        JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR, message); \
        THROW_NEW_ERROR_AND_RETURN(thread, error.GetTaggedValue());                     \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_RANGE_ERROR_AND_RETURN(thread, message, exception)                               \
    do {                                                                                       \
        ObjectFactory *objectFactory = (thread)->GetEcmaVM()->GetFactory();                    \
        JSHandle<JSObject> error = objectFactory->GetJSError(ErrorType::RANGE_ERROR, message); \
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error.GetTaggedValue(), exception);           \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_RANGE_ERROR(thread, message)                                               \
    do {                                                                                 \
        ObjectFactory *factory = (thread)->GetEcmaVM()->GetFactory();                    \
        JSHandle<JSObject> error = factory->GetJSError(ErrorType::RANGE_ERROR, message); \
        THROW_NEW_ERROR_AND_RETURN(thread, error.GetTaggedValue());                      \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define THROW_URI_ERROR_AND_RETURN(thread, message, exception)                               \
    do {                                                                                     \
        ObjectFactory *objectFactory = (thread)->GetEcmaVM()->GetFactory();                  \
        JSHandle<JSObject> error = objectFactory->GetJSError(ErrorType::URI_ERROR, message); \
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error.GetTaggedValue(), exception);         \
    } while (false)

#define THROW_SYNTAX_ERROR_AND_RETURN(thread, message, exception)                         \
    do {                                                                                  \
        ObjectFactory *factory = (thread)->GetEcmaVM()->GetFactory();                     \
        JSHandle<JSObject> error = factory->GetJSError(ErrorType::SYNTAX_ERROR, message); \
        THROW_NEW_ERROR_AND_RETURN_VALUE(thread, error.GetTaggedValue(), exception);      \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_REJECT_PROMISE_IF_ABRUPT(thread, value, capability)                                 \
    do {                                                                                           \
        const GlobalEnvConstants *globalConst = (thread)->GlobalConstants();                       \
        if ((value).GetTaggedValue().IsCompletionRecord()) {                                       \
            JSHandle<CompletionRecord> record = JSHandle<CompletionRecord>::Cast(value);           \
            if (record->IsThrow()) {                                                               \
                JSHandle<JSTaggedValue> reject(thread, (capability)->GetReject());                 \
                JSHandle<JSTaggedValue> undefine = globalConst->GetHandledUndefined();             \
                InternalCallParams *arg = (thread)->GetInternalCallParams();                       \
                arg->MakeArgv(record->GetValue());                                                 \
                JSTaggedValue res = JSFunction::Call(thread, reject, undefine, 1, arg->GetArgv()); \
                RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, res);                                    \
                return (capability)->GetPromise();                                                 \
            }                                                                                      \
        }                                                                                          \
        if ((thread)->HasPendingException()) {                                                     \
            (thread)->ClearException();                                                            \
            JSHandle<JSTaggedValue> reject(thread, (capability)->GetReject());                     \
            JSHandle<JSTaggedValue> undefined = globalConst->GetHandledUndefined();                \
            InternalCallParams *arg = (thread)->GetInternalCallParams();                           \
            arg->MakeArgv(value);                                                                  \
            JSTaggedValue res = JSFunction::Call(thread, reject, undefined, 1, arg->GetArgv());    \
            RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, res);                                        \
            return (capability)->GetPromise();                                                     \
        }                                                                                          \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define RETURN_COMPLETION_IF_ABRUPT(thread, value)                                \
    do {                                                                          \
        if ((thread)->HasPendingException()) {                                    \
            JSHandle<CompletionRecord> completionRecord =                         \
                factory->NewCompletionRecord(CompletionRecordType::THROW, value); \
            return (completionRecord);                                            \
        }                                                                         \
    } while (false)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECL_DUMP()                                                    \
    void Dump(JSThread *thread, std::ostream &os) const DUMP_API_ATTR; \
    void Dump(JSThread *thread) const DUMP_API_ATTR                    \
    {                                                                  \
        Dump(thread, std::cout);                                       \
    }                                                                  \
    void DumpForSnapshot(JSThread *thread, std::vector<std::pair<CString, JSTaggedValue>> &vec) const;

#endif  // defined(__cplusplus)

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECL_CAST(TYPE)                           \
    static TYPE *Cast(ObjectHeader *object)       \
    {                                             \
        ASSERT(JSTaggedValue(object).Is##TYPE()); \
        return reinterpret_cast<TYPE *>(object);  \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECL_VISIT_ARRAY(BEGIN_OFFSET, LENGTH)                                                                \
    void VisitRangeSlot(const EcmaObjectRangeVisitor &visitor)                                                \
    {                                                                                                         \
        size_t endOffset = (BEGIN_OFFSET) + (LENGTH) * JSTaggedValue::TaggedTypeSize();                       \
        visitor(this, ObjectSlot(ToUintPtr(this) + (BEGIN_OFFSET)), ObjectSlot(ToUintPtr(this) + endOffset)); \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECL_VISIT_OBJECT(BEGIN_OFFSET, END_OFFSET)                                                              \
    void VisitRangeSlot(const EcmaObjectRangeVisitor &visitor)                                                   \
    {                                                                                                            \
        visitor(this, ObjectSlot(ToUintPtr(this) + (BEGIN_OFFSET)), ObjectSlot(ToUintPtr(this) + (END_OFFSET))); \
    }

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define DECL_VISIT_OBJECT_FOR_JS_OBJECT(PARENTCLASS, BEGIN_OFFSET, END_OFFSET)                                   \
    void VisitRangeSlot(const EcmaObjectRangeVisitor &visitor)                                                   \
    {                                                                                                            \
        VisitObjects(visitor);                                                                                   \
        /* visit in object fields */                                                                             \
        auto objSize = this->GetClass()->GetObjectSize();                                                        \
        if (objSize > SIZE) {                                                                            \
            visitor(this, ObjectSlot(ToUintPtr(this) + SIZE), ObjectSlot(ToUintPtr(this) + objSize));    \
        }                                                                                                        \
    }                                                                                                            \
    void VisitObjects(const EcmaObjectRangeVisitor &visitor)                                                     \
    {                                                                                                            \
        PARENTCLASS::VisitObjects(visitor);                                                                      \
        if ((BEGIN_OFFSET) == (END_OFFSET)) {                                                                    \
            return;                                                                                              \
        }                                                                                                        \
        visitor(this, ObjectSlot(ToUintPtr(this) + (BEGIN_OFFSET)), ObjectSlot(ToUintPtr(this) + (END_OFFSET))); \
    }

#if ECMASCRIPT_ENABLE_CAST_CHECK
    #define CAST_CHECK(CAST_TYPE, CHECK_METHOD)                                                 \
        static inline CAST_TYPE *Cast(ObjectHeader *object)                                     \
        {                                                                                       \
            if (!JSTaggedValue(object).CHECK_METHOD()) {                                        \
                std::abort();                                                                   \
            }                                                                                   \
            return static_cast<CAST_TYPE *>(object);                                            \
        }                                                                                       \
        static inline const CAST_TYPE *ConstCast(const ObjectHeader *object)                    \
        {                                                                                       \
            if (!JSTaggedValue(object).CHECK_METHOD()) {                                        \
                std::abort();                                                                   \
            }                                                                                   \
            return static_cast<const CAST_TYPE *>(object);                                      \
        }
# else
    #define CAST_CHECK(CAST_TYPE, CHECK_METHOD)                                                   \
        static inline CAST_TYPE *Cast(ObjectHeader *object)                                       \
        {                                                                                         \
            ASSERT(JSTaggedValue(object).CHECK_METHOD());                                         \
            return static_cast<CAST_TYPE *>(object);                                              \
        }                                                                                         \
        static const inline CAST_TYPE *ConstCast(const ObjectHeader *object)                      \
        {                                                                                         \
            ASSERT(JSTaggedValue(object).CHECK_METHOD());                                         \
            return static_cast<const CAST_TYPE *>(object);                                        \
        }

    #define CAST_NO_CHECK(CAST_TYPE)                                                              \
        static inline CAST_TYPE *Cast(ObjectHeader *object)                                       \
        {                                                                                         \
            return static_cast<CAST_TYPE *>(object);                                              \
        }                                                                                         \
        static const inline CAST_TYPE *ConstCast(const ObjectHeader *object)                      \
        {                                                                                         \
            return static_cast<const CAST_TYPE *>(object);                                        \
        }
#endif

#if ECMASCRIPT_ENABLE_CAST_CHECK
    #define CAST_CHECK_TAGGEDVALUE(CAST_TYPE, CHECK_METHOD)                                     \
        static inline CAST_TYPE *Cast(JSTaggedValue value)                                      \
        {                                                                                       \
            if (value.IsHeapObject() && value.GetTaggedObject()->GetClass()->CHECK_METHOD()) {  \
                return static_cast<CAST_TYPE *>(value.GetTaggedObject());                       \
            }                                                                                   \
            std::abort();                                                                       \
        }
# else
    #define CAST_CHECK_TAGGEDVALUE(CAST_TYPE, CHECK_METHOD)                                       \
        static inline CAST_TYPE *Cast(JSTaggedValue value)                                        \
        {                                                                                         \
            ASSERT(value.IsHeapObject() && value.GetTaggedObject()->GetClass()->CHECK_METHOD());  \
            return static_cast<CAST_TYPE *>(value.GetTaggedObject());                             \
        }
#endif

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CHECK_DUMP_FILEDS(begin, end, num)                                               \
    LOG_IF((num) != ((end) - (begin)) / JSTaggedValue::TaggedTypeSize(), FATAL, RUNTIME) \
        << "Fileds in obj are not in dump list. ";

#define CHECK_OBJECT_SIZE(size)                                                                     \
    if ((size) == 0) {                                                                              \
        LOG(FATAL, ECMASCRIPT) << __func__ << " Line: " << __LINE__ << " objectSize is " << (size); \
    }

#define CHECK_REGION_END(begin, end)                                                                               \
    if ((begin) > (end)) {                                                                                         \
        LOG(FATAL, ECMASCRIPT) << __func__ << " Line: " << __LINE__ << " begin: " << (begin) << " end: " << (end); \
    }

#endif  // ECMASCRIPT_ECMA_MACROS_H
