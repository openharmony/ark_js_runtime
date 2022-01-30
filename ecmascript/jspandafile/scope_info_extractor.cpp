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

#include "ecmascript/jspandafile/scope_info_extractor.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/jspandafile/literal_data_extractor.h"
#include "ecmascript/tagged_array-inl.h"

namespace panda::ecmascript {
JSTaggedValue ScopeInfoExtractor::GenerateScopeInfo(JSThread *thread, uint16_t scopeId)
{
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSMethod *method = InterpretedFrameHandler(thread).GetMethod();
    const panda_file::File *pf = method->GetPandaFile();
    JSHandle<TaggedArray> elementsLiteral = LiteralDataExtractor::GetDatasIgnoreType(thread, pf, scopeId);
    ASSERT(elementsLiteral->GetLength() > 0);
    size_t length = elementsLiteral->GetLength();

    auto buffer = ecmaVm->GetNativeAreaAllocator()->New<struct ScopeDebugInfo>();
    auto scopeDebugInfo = static_cast<struct ScopeDebugInfo *>(buffer);
    scopeDebugInfo->scopeInfo.reserve(length);

    for (size_t i = 1; i < length; i += 2) {  // 2: Each literal buffer contains a pair of key-value.
        CString name = ConvertToString(elementsLiteral->Get(i));
        uint32_t slot = elementsLiteral->Get(i + 1).GetInt();
        scopeDebugInfo->scopeInfo.push_back({slot, name});
    }

    JSHandle<JSNativePointer> pointer = factory->NewJSNativePointer(
        buffer, NativeAreaAllocator::FreeObjectFunc<struct ScopeDebugInfo>, ecmaVm->GetNativeAreaAllocator());
    return pointer.GetTaggedValue();
}
}  // namespace panda::ecmascript
