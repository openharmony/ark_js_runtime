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

#include "js_method.h"
#include "libpandafile/method_data_accessor-inl.h"

namespace panda::ecmascript {
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

void JSMethod::SetCallTypeFromAnnotation()
{
    const panda_file::File *panda_file_ = GetPandaFile();
    panda_file::File::EntityId field_id_ = GetFileId();
    panda_file::MethodDataAccessor mda(*panda_file_, field_id_);
    mda.EnumerateAnnotations([&](panda_file::File::EntityId annotation_id) {
        panda_file::AnnotationDataAccessor ada(*panda_file_, annotation_id);
        auto *annotation_name = reinterpret_cast<const char *>(panda_file_->GetStringData(ada.GetClassId()).data);
        if (::strcmp("L_ESCallTypeAnnotation;", annotation_name) == 0) {
            uint32_t elem_count = ada.GetCount();
            for (uint32_t i = 0; i < elem_count; i++) {
                panda_file::AnnotationDataAccessor::Elem adae = ada.GetElement(i);
                auto *elem_name = reinterpret_cast<const char *>(panda_file_->GetStringData(adae.GetNameId()).data);
                if (::strcmp("callType", elem_name) == 0) {
                    callType_ = adae.GetScalarValue().GetValue();
                }
            }
        }
    });
}
}