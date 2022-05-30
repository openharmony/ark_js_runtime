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

#include "ecmascript/js_method.h"

#include "ecmascript/jspandafile/js_pandafile.h"
#include "libpandafile/code_data_accessor-inl.h"
#include "libpandafile/method_data_accessor-inl.h"

namespace panda::ecmascript {
JSMethod::JSMethod(const JSPandaFile *jsPandaFile, panda_file::File::EntityId methodId)
{
    jsPandaFile_ = jsPandaFile;
    if (jsPandaFile_ != nullptr) {
        panda_file::MethodDataAccessor mda(*(jsPandaFile_->GetPandaFile()), methodId);
        auto codeId = mda.GetCodeId().value();
        if (!codeId.IsValid()) {
            nativePointerOrBytecodeArray_ = nullptr;
        }
        panda_file::CodeDataAccessor cda(*(jsPandaFile_->GetPandaFile()), codeId);
        nativePointerOrBytecodeArray_ = cda.GetInstructions();
    }
    SetHotnessCounter(static_cast<int16_t>(0));
    SetMethodId(methodId);
    UpdateSlotSize(static_cast<uint8_t>(0));
}

// It's not allowed '#' token appear in ECMA function(method) name, which discriminates same names in panda methods.
CString JSMethod::ParseFunctionName() const
{
    CString methodName(utf::Mutf8AsCString(GetName().data));
    if (LIKELY(methodName[0] != '#')) {
        return methodName;
    }
    size_t index = methodName.find_last_of('#');
    return CString(methodName.substr(index + 1));
}

void JSMethod::InitializeCallField(uint32_t numVregs, uint32_t numArgs)
{
    uint32_t callType = UINT32_MAX;  // UINT32_MAX means not found
    const panda_file::File *pandaFile = jsPandaFile_->GetPandaFile();
    panda_file::MethodDataAccessor mda(*pandaFile, GetMethodId());
    mda.EnumerateAnnotations([&](panda_file::File::EntityId annotation_id) {
        panda_file::AnnotationDataAccessor ada(*pandaFile, annotation_id);
        auto *annotation_name = reinterpret_cast<const char *>(pandaFile->GetStringData(ada.GetClassId()).data);
        if (::strcmp("L_ESCallTypeAnnotation;", annotation_name) == 0) {
            uint32_t elem_count = ada.GetCount();
            for (uint32_t i = 0; i < elem_count; i++) {
                panda_file::AnnotationDataAccessor::Elem adae = ada.GetElement(i);
                auto *elem_name = reinterpret_cast<const char *>(pandaFile->GetStringData(adae.GetNameId()).data);
                if (::strcmp("callType", elem_name) == 0) {
                    callType = adae.GetScalarValue().GetValue();
                }
            }
        }
    });
    // Needed info for call can be got by loading callField only once.
    // Native bit will be set in NewMethodForNativeFunction();
    callField_ = (callType & CALL_TYPE_MASK) |
                 NumVregsBits::Encode(numVregs) |
                 NumArgsBits::Encode(numArgs - HaveFuncBit::Decode(callType)  // exclude func
                                             - HaveNewTargetBit::Decode(callType)  // exclude new target
                                             - HaveThisBit::Decode(callType));  // exclude this
}

const char *JSMethod::GetMethodName() const
{
    return utf::Mutf8AsCString(GetName().data);
}

panda_file::File::StringData JSMethod::GetName() const
{
    panda_file::MethodDataAccessor mda(*(jsPandaFile_->GetPandaFile()), GetMethodId());
    return jsPandaFile_->GetPandaFile()->GetStringData(mda.GetNameId());
}

uint32_t JSMethod::GetNumVregs() const
{
    if (jsPandaFile_ == nullptr) {
        return 0;
    }
    panda_file::MethodDataAccessor mda(*(jsPandaFile_->GetPandaFile()), GetMethodId());
    auto codeId = mda.GetCodeId().value();
    if (!codeId.IsValid()) {
        return 0;
    }
    panda_file::CodeDataAccessor cda(*(jsPandaFile_->GetPandaFile()), codeId);
    return cda.GetNumVregs();
}

uint32_t JSMethod::GetCodeSize() const
{
    if (jsPandaFile_ == nullptr) {
        return 0;
    }
    panda_file::MethodDataAccessor mda(*(jsPandaFile_->GetPandaFile()), GetMethodId());
    auto codeId = mda.GetCodeId().value();
    if (!codeId.IsValid()) {
        return 0;
    }
    panda_file::CodeDataAccessor cda(*(jsPandaFile_->GetPandaFile()), codeId);
    return cda.GetCodeSize();
}

uint32_t JSMethod::GetCodeSize(panda_file::File::EntityId methodId) const
{
    if (jsPandaFile_ == nullptr) {
        return 0;
    }
    panda_file::MethodDataAccessor mda(*(jsPandaFile_->GetPandaFile()), methodId);
    auto codeId = mda.GetCodeId().value();
    if (!codeId.IsValid()) {
        return 0;
    }
    panda_file::CodeDataAccessor cda(*(jsPandaFile_->GetPandaFile()), codeId);
    return cda.GetCodeSize();
}

const panda_file::File *JSMethod::GetPandaFile() const
{
    if (jsPandaFile_ == nullptr) {
        return nullptr;
    }
    return jsPandaFile_->GetPandaFile();
}

uint32_t JSMethod::GetBytecodeArraySize() const
{
    return GetCodeSize(GetMethodId());
}
} // namespace panda::ecmascript
