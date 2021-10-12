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

#include "ecmascript/ecma_language_context.h"

#include "ecmascript/ecma_class_linker_extension.h"
#include "ecmascript/ecma_exceptions.h"
#include "ecmascript/js_method.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "include/tooling/pt_lang_extension.h"

namespace panda {
using ecmascript::InterpretedFrameHandler;
using ecmascript::EcmaVM;
using ecmascript::JSThread;

std::pair<Method *, uint32_t> EcmaLanguageContext::GetCatchMethodAndOffset(Method *method, ManagedThread *thread) const
{
    Method *catchMethod = method;
    uint32_t catchOffset = 0;
    auto jsThread = static_cast<JSThread *>(thread);
    InterpretedFrameHandler frameHandler(jsThread);
    for (; frameHandler.HasFrame(); frameHandler.PrevFrame()) {
        if (frameHandler.IsBreakFrame()) {
            continue;
        }
        catchMethod = frameHandler.GetMethod();
        if (catchMethod->IsNative()) {
            continue;
        }
        catchOffset = catchMethod->FindCatchBlock(jsThread->GetException().GetTaggedObject()->ClassAddr<Class>(),
                                                  frameHandler.GetBytecodeOffset());
        if (catchOffset != panda_file::INVALID_OFFSET) {
            break;
        }
    }

    return std::make_pair(catchMethod, catchOffset);
}

PandaVM *EcmaLanguageContext::CreateVM(Runtime *runtime, [[maybe_unused]] const RuntimeOptions &options) const
{
    auto ret = EcmaVM::Create(runtime);
    if (ret) {
        return ret.Value();
    }
    return nullptr;
}

std::unique_ptr<ClassLinkerExtension> EcmaLanguageContext::CreateClassLinkerExtension() const
{
    return std::make_unique<ecmascript::EcmaClassLinkerExtension>();
}

PandaUniquePtr<tooling::PtLangExt> EcmaLanguageContext::CreatePtLangExt() const
{
    return PandaUniquePtr<tooling::PtLangExt>();
}

void EcmaLanguageContext::ThrowException(ManagedThread *thread, const uint8_t *mutf8_name,
                                         const uint8_t *mutf8_msg) const
{
    ecmascript::ThrowException(JSThread::Cast(thread), reinterpret_cast<const char*>(mutf8_name),
                               reinterpret_cast<const char*>(mutf8_msg));
}
}  // namespace panda
