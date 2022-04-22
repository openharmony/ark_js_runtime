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

#include "ecmascript/tooling/base/pt_returns.h"

namespace panda::ecmascript::tooling {
Local<ObjectRef> EnableReturns::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    result->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "debuggerId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, std::to_string(debuggerId_).c_str())));

    return result;
}

Local<ObjectRef> SetBreakpointByUrlReturns::ToObject(const EcmaVM *ecmaVm)
{
    size_t len = locations_.size();
    Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
    for (size_t i = 0; i < len; i++) {
        Local<ObjectRef> location = locations_[i]->ToObject(ecmaVm);
        values->Set(ecmaVm, i, location);
    }

    Local<ObjectRef> result = NewObject(ecmaVm);
    result->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "breakpointId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, id_.c_str())));
    result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "locations")), values);

    return result;
}

Local<ObjectRef> EvaluateOnCallFrameReturns::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    Local<ObjectRef> location = result_->ToObject(ecmaVm);
    result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "result")), Local<JSValueRef>(location));
    if (exceptionDetails_) {
        Local<ObjectRef> exception = exceptionDetails_.value()->ToObject(ecmaVm);
        result->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "exceptionDetails")),
            Local<JSValueRef>(exception));
    }

    return result;
}

Local<ObjectRef> GetPossibleBreakpointsReturns::ToObject(const EcmaVM *ecmaVm)
{
    size_t len = locations_.size();
    Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
    for (size_t i = 0; i < len; i++) {
        Local<ObjectRef> location = locations_[i]->ToObject(ecmaVm);
        values->Set(ecmaVm, i, location);
    }

    Local<ObjectRef> result = NewObject(ecmaVm);
    result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "locations")), values);

    return result;
}

Local<ObjectRef> GetScriptSourceReturns::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    result->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptSource")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, scriptSource_.c_str())));
    if (bytecode_) {
        result->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "bytecode")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, bytecode_->c_str())));
    }

    return result;
}

Local<ObjectRef> RestartFrameReturns::ToObject(const EcmaVM *ecmaVm)
{
    size_t len = callFrames_.size();
    Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
    for (size_t i = 0; i < len; i++) {
        Local<ObjectRef> location = callFrames_[i]->ToObject(ecmaVm);
        values->Set(ecmaVm, i, location);
    }

    Local<ObjectRef> result = NewObject(ecmaVm);
    result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "callFrames")), values);
    return result;
}

Local<ObjectRef> SearchInContentReturns::ToObject(const EcmaVM *ecmaVm)
{
    size_t len = result_.size();
    Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
    for (size_t i = 0; i < len; i++) {
        Local<ObjectRef> location = result_[i]->ToObject(ecmaVm);
        values->Set(ecmaVm, i, location);
    }

    Local<ObjectRef> result = NewObject(ecmaVm);
    result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "result")), values);

    return result;
}

Local<ObjectRef> SetBreakpointReturns::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    result->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "breakpointId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, breakpointId_.c_str())));

    Local<ObjectRef> location = location_->ToObject(ecmaVm);
    result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "actualLocation")),
        Local<JSValueRef>(location));

    return result;
}

Local<ObjectRef> SetInstrumentationBreakpointReturns::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    result->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "breakpointId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, breakpointId_.c_str())));

    return result;
}

Local<ObjectRef> SetScriptSourceReturns::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    if (callFrames_) {
        CVector<std::unique_ptr<CallFrame>> callFrame(std::move(callFrames_.value()));
        size_t len = callFrame.size();
        Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
        for (size_t i = 0; i < len; i++) {
            Local<ObjectRef> location = callFrame[i]->ToObject(ecmaVm);
            values->Set(ecmaVm, i, location);
        }
        result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "callFrames")), values);
    }

    if (stackChanged_) {
        result->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "stackChanged")),
            BooleanRef::New(ecmaVm, stackChanged_.value()));
    }

    if (exceptionDetails_) {
        result->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "exceptionDetails")),
            Local<JSValueRef>(exceptionDetails_.value()->ToObject(ecmaVm)));
    }

    return result;
}

Local<ObjectRef> GetPropertiesReturns::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    size_t len = result_.size();
    Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
    for (size_t i = 0; i < len; i++) {
        Local<ObjectRef> descriptor = result_[i]->ToObject(ecmaVm);
        values->Set(ecmaVm, i, descriptor);
    }
    result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "result")), values);
    if (internalPropertyDescripties_) {
        auto descripties = std::move(internalPropertyDescripties_.value());
        len = descripties.size();
        values = ArrayRef::New(ecmaVm, len);
        for (size_t i = 0; i < len; i++) {
            Local<ObjectRef> descriptor = descripties[i]->ToObject(ecmaVm);
            values->Set(ecmaVm, i, descriptor);
        }
        result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "internalProperties")), values);
    }
    if (privateProperties_) {
        auto descripties = std::move(privateProperties_.value());
        len = descripties.size();
        values = ArrayRef::New(ecmaVm, len);
        for (size_t i = 0; i < len; i++) {
            Local<ObjectRef> descriptor = descripties[i]->ToObject(ecmaVm);
            values->Set(ecmaVm, i, descriptor);
        }
        result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "privateProperties")), values);
    }
    if (exceptionDetails_) {
        result->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "exceptionDetails")),
            Local<JSValueRef>(exceptionDetails_.value()->ToObject(ecmaVm)));
    }

    return result;
}

Local<ObjectRef> CallFunctionOnReturns::ToObject(const EcmaVM *ecmaVm)
{
    // For this
    Local<ObjectRef> returns = NewObject(ecmaVm);

    // For this.result_
    Local<ObjectRef> result = result_->ToObject(ecmaVm);
    returns->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "result")),
        Local<JSValueRef>(result));
    // For this.exceptionDetails_
    if (exceptionDetails_) {
        returns->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "exceptionDetails")),
            Local<JSValueRef>(exceptionDetails_.value()->ToObject(ecmaVm)));
    }

    return returns;
}

Local<ObjectRef> StopSamplingReturns::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    if (profile_ != nullptr) {
        Local<ObjectRef> profile = profile_->ToObject(ecmaVm);
        result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "profile")),
            Local<JSValueRef>(profile));
    }
    return result;
}

Local<ObjectRef> GetHeapObjectIdReturns::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    result->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "heapSnapshotObjectId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, std::to_string(heapSnapshotObjectId_).c_str())));

    return result;
}

Local<ObjectRef> GetObjectByHeapObjectIdReturns::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> result = NewObject(ecmaVm);
    
    if (remoteObjectResult_ != nullptr) {
        Local<ObjectRef> remoteObjectResult = remoteObjectResult_->ToObject(ecmaVm);
        result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "result")),
            Local<JSValueRef>(remoteObjectResult));
    }

    return result;
}

Local<ObjectRef> StopReturns::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    Local<ObjectRef> profile = profile_->ToObject(ecmaVm);
    result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "profile")), Local<JSValueRef>(profile));

    return result;
}

Local<ObjectRef> GetHeapUsageReturns::ToObject(const EcmaVM *ecmaVm)
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    result->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "usedSize")),
        NumberRef::New(ecmaVm, usedSize_));
    result->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "totalSize")),
        NumberRef::New(ecmaVm, totalSize_));

    return result;
}
}  // namespace panda::ecmascript::tooling