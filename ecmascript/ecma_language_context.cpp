/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "ecmascript/ecma_exceptions.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/jspandafile/ecma_class_linker_extension.h"
#include "ecmascript/js_method.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"

namespace panda {
using ecmascript::JSTaggedType;
using ecmascript::InterpretedFrameHandler;
using ecmascript::EcmaVM;
using ecmascript::JSThread;

PandaVM *EcmaLanguageContext::CreateVM(Runtime *runtime, [[maybe_unused]] const RuntimeOptions &options) const
{
    return EcmaVM::Create(runtime);
}

std::unique_ptr<ClassLinkerExtension> EcmaLanguageContext::CreateClassLinkerExtension() const
{
    return std::make_unique<ecmascript::EcmaClassLinkerExtension>();
}

void EcmaLanguageContext::ThrowException(ManagedThread *thread, const uint8_t *mutf8_name,
                                         const uint8_t *mutf8_msg) const
{
    ecmascript::ThrowException(JSThread::Cast(thread), reinterpret_cast<const char*>(mutf8_name),
                               reinterpret_cast<const char*>(mutf8_msg));
}
}  // namespace panda
