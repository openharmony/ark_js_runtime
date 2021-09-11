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

#ifndef ECMASCRIPT_LANGUAGE_CONTEXT_H
#define ECMASCRIPT_LANGUAGE_CONTEXT_H

#include "ecmascript/common.h"
#include "include/language_context.h"

namespace panda {
class PUBLIC_API EcmaLanguageContext : public LanguageContextBase {
public:
    EcmaLanguageContext() = default;

    DEFAULT_COPY_SEMANTIC(EcmaLanguageContext);
    DEFAULT_MOVE_SEMANTIC(EcmaLanguageContext);

    ~EcmaLanguageContext() override = default;

    panda_file::SourceLang GetLanguage() const override
    {
        return panda_file::SourceLang::ECMASCRIPT;
    }

    std::pair<Method *, uint32_t> GetCatchMethodAndOffset(Method *method, ManagedThread *thread) const override;

    PandaVM *CreateVM(Runtime *runtime, const RuntimeOptions &options) const override;

    std::unique_ptr<ClassLinkerExtension> CreateClassLinkerExtension() const override;

    PandaUniquePtr<tooling::PtLangExt> CreatePtLangExt() const override;

    void ThrowException(ManagedThread *thread, const uint8_t *mutf8_name, const uint8_t *mutf8_msg) const override;

    coretypes::TaggedValue GetInitialTaggedValue() const override
    {
        UNREACHABLE();
    }

    uint64_t GetTypeTag([[maybe_unused]] interpreter::TypeTag tag) const override
    {
        UNREACHABLE();
    }

    DecodedTaggedValue GetInitialDecodedValue() const override
    {
        UNREACHABLE();
    }

    DecodedTaggedValue GetDecodedTaggedValue([[maybe_unused]] const coretypes::TaggedValue &value) const override
    {
        UNREACHABLE();
    }

    coretypes::TaggedValue GetEncodedTaggedValue([[maybe_unused]] int64_t value,
                                                 [[maybe_unused]] int64_t tag) const override
    {
        UNREACHABLE();
    }

    mem::GC *CreateGC([[maybe_unused]] mem::GCType gc_type, [[maybe_unused]] mem::ObjectAllocatorBase *object_allocator,
                      [[maybe_unused]] const mem::GCSettings &settings) const override
    {
        UNREACHABLE();
        return nullptr;
    }

    void SetExceptionToVReg([[maybe_unused]] Frame::VRegister &vreg, [[maybe_unused]] ObjectHeader *obj) const override
    {
        UNREACHABLE();
    }

    bool IsCallableObject([[maybe_unused]] ObjectHeader *obj) const override
    {
        UNREACHABLE();
    }

    Method *GetCallTarget([[maybe_unused]] ObjectHeader *obj) const override
    {
        UNREACHABLE();
    }

    const uint8_t *GetStringClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/JSString;");
    }

    const uint8_t *GetObjectClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/JSObject;");
    }

    const uint8_t *GetClassClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/HClass;");
    }

    const uint8_t *GetClassArrayClassDescriptor() const override
    {
        return utf::CStringAsMutf8("[Lpanda/JSObject;");
    }

    const uint8_t *GetStringArrayClassDescriptor() const override
    {
        return utf::CStringAsMutf8("[Lpanda/JSString;");
    }

    const uint8_t *GetCtorName() const override
    {
        return utf::CStringAsMutf8(".ctor");
    }

    const uint8_t *GetCctorName() const override
    {
        return utf::CStringAsMutf8(".cctor");
    }

    const uint8_t *GetNullPointerExceptionClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/NullPointerException;");
    }

    const uint8_t *GetArrayIndexOutOfBoundsExceptionClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/ArrayIndexOutOfBoundsException;");
    }

    const uint8_t *GetIndexOutOfBoundsExceptionClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/IndexOutOfBoundsException;");
    }

    const uint8_t *GetIllegalStateExceptionClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/IllegalStateException;");
    }

    const uint8_t *GetNegativeArraySizeExceptionClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/NegativeArraySizeException;");
    }

    const uint8_t *GetStringIndexOutOfBoundsExceptionClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/StringIndexOutOfBoundsException;");
    }

    const uint8_t *GetArithmeticExceptionClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/ArithmeticException;");
    }

    const uint8_t *GetClassCastExceptionClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/ClassCastException;");
    }

    const uint8_t *GetAbstractMethodErrorClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/AbstractMethodError;");
    }

    const uint8_t *GetArrayStoreExceptionClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/ArrayStoreException;");
    }

    const uint8_t *GetRuntimeExceptionClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/RuntimeException;");
    }

    const uint8_t *GetFileNotFoundExceptionClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/FileNotFoundException;");
    }

    const uint8_t *GetIOExceptionClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/IOException;");
    }

    const uint8_t *GetIllegalArgumentExceptionClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/IllegalArgumentException;");
    }

    const uint8_t *GetOutOfMemoryErrorClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/OutOfMemoryError;");
    }

    const uint8_t *GetNoClassDefFoundErrorDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/NoClassDefFoundError;");
    }

    const uint8_t *GetClassCircularityErrorDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/ClassCircularityError;");
    }

    const uint8_t *GetNoSuchFieldErrorDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/NoSuchFieldError;");
    }

    const uint8_t *GetNoSuchMethodErrorDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/NoSuchMethodError;");
    }

    const uint8_t *GetExceptionInInitializerErrorDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/ExceptionInInitializerError;");
    }

    const uint8_t *GetClassNotFoundExceptionDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/ClassNotFoundException;");
    }

    const uint8_t *GetInstantiationErrorDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/InstantiationError;");
    }

    const uint8_t *GetUnsupportedOperationExceptionClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/UnsupportedOperationException;");
    }

    const uint8_t *GetVerifyErrorClassDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/VerifyError;");
    }

    const uint8_t *GetIllegalMonitorStateExceptionDescriptor() const override
    {
        return utf::CStringAsMutf8("Lpanda/IllegalMonitorStateException;");
    }

    const uint8_t *GetReferenceErrorDescriptor() const override
    {
        return utf::CStringAsMutf8("Lecma/ReferenceError;");
    }

    const uint8_t *GetTypedErrorDescriptor() const override
    {
        return utf::CStringAsMutf8("Lecma/TypedError;");
    }
};
}  // namespace panda

#endif  // ECMASCRIPT_LANGUAGE_CONTEXT_H
