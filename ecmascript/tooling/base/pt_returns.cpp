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
Local<ObjectRef> EnableReturns::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    result->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "debuggerId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, std::to_string(debuggerId_).c_str())));

    return result;
}

std::unique_ptr<PtJson> EnableReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("debuggerId", std::to_string(debuggerId_).c_str());

    return result;
}

Local<ObjectRef> SetBreakpointByUrlReturns::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> SetBreakpointByUrlReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("breakpointId", id_.c_str());
    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    size_t len = locations_.size();
    for (size_t i = 0; i < len; i++) {
        std::unique_ptr<PtJson> location = locations_[i]->ToJson();
        array->Push(location);
    }
    result->Add("locations", array);

    return result;
}

Local<ObjectRef> EvaluateOnCallFrameReturns::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> EvaluateOnCallFrameReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    if (result_ != nullptr) {
        result->Add("result", result_->ToJson());
    }
    if (exceptionDetails_ && exceptionDetails_.value() != nullptr) {
        result->Add("exceptionDetails", exceptionDetails_.value()->ToJson());
    }

    return result;
}

Local<ObjectRef> GetPossibleBreakpointsReturns::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> GetPossibleBreakpointsReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    size_t len = locations_.size();
    for (size_t i = 0; i < len; i++) {
        std::unique_ptr<PtJson> location = locations_[i]->ToJson();
        array->Push(location);
    }
    result->Add("locations", array);

    return result;
}

Local<ObjectRef> GetScriptSourceReturns::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> GetScriptSourceReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("scriptSource", scriptSource_.c_str());
    if (bytecode_) {
        result->Add("bytecode", bytecode_->c_str());
    }

    return result;
}

Local<ObjectRef> RestartFrameReturns::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> RestartFrameReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    size_t len = callFrames_.size();
    for (size_t i = 0; i < len; i++) {
        std::unique_ptr<PtJson> location = callFrames_[i]->ToJson();
        array->Push(location);
    }
    result->Add("callFrames", array);

    return result;
}

Local<ObjectRef> SearchInContentReturns::ToObject(const EcmaVM *ecmaVm) const
{
    size_t len = result_.size();
    Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
    for (size_t i = 0; i < len; i++) {
        Local<ObjectRef> res = result_[i]->ToObject(ecmaVm);
        values->Set(ecmaVm, i, res);
    }

    Local<ObjectRef> result = NewObject(ecmaVm);
    result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "result")), values);

    return result;
}

std::unique_ptr<PtJson> SearchInContentReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    size_t len = result_.size();
    for (size_t i = 0; i < len; i++) {
        std::unique_ptr<PtJson> res = result_[i]->ToJson();
        array->Push(res);
    }
    result->Add("result", array);

    return result;
}

Local<ObjectRef> SetBreakpointReturns::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> SetBreakpointReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("breakpointId", breakpointId_.c_str());
    if (location_ != nullptr) {
        result->Add("actualLocation", location_->ToJson());
    }

    return result;
}

Local<ObjectRef> SetInstrumentationBreakpointReturns::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    result->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "breakpointId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, breakpointId_.c_str())));

    return result;
}

std::unique_ptr<PtJson> SetInstrumentationBreakpointReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("breakpointId", breakpointId_.c_str());

    return result;
}

Local<ObjectRef> SetScriptSourceReturns::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    if (callFrames_) {
        const std::vector<std::unique_ptr<CallFrame>> &callFrame = callFrames_.value();
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

std::unique_ptr<PtJson> SetScriptSourceReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    if (callFrames_) {
        std::unique_ptr<PtJson> array = PtJson::CreateArray();
        size_t len = callFrames_->size();
        for (size_t i = 0; i < len; i++) {
            std::unique_ptr<PtJson> location = callFrames_.value()[i]->ToJson();
            array->Push(location);
        }
        result->Add("callFrames", array);
    }
    if (stackChanged_) {
        result->Add("stackChanged", stackChanged_.value());
    }
    if (exceptionDetails_ && exceptionDetails_.value() != nullptr) {
        result->Add("exceptionDetails", exceptionDetails_.value()->ToJson());
    }

    return result;
}

Local<ObjectRef> GetPropertiesReturns::ToObject(const EcmaVM *ecmaVm) const
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
        auto &descripties = internalPropertyDescripties_.value();
        len = descripties.size();
        values = ArrayRef::New(ecmaVm, len);
        for (size_t i = 0; i < len; i++) {
            Local<ObjectRef> descriptor = descripties[i]->ToObject(ecmaVm);
            values->Set(ecmaVm, i, descriptor);
        }
        result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "internalProperties")), values);
    }
    if (privateProperties_) {
        auto &descripties = privateProperties_.value();
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

std::unique_ptr<PtJson> GetPropertiesReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    size_t len = result_.size();
    for (size_t i = 0; i < len; i++) {
        std::unique_ptr<PtJson> location = result_[i]->ToJson();
        array->Push(location);
    }
    result->Add("result", array);
    if (internalPropertyDescripties_) {
        array = PtJson::CreateArray();
        len = internalPropertyDescripties_->size();
        for (size_t i = 0; i < len; i++) {
            std::unique_ptr<PtJson> location = internalPropertyDescripties_.value()[i]->ToJson();
            array->Push(location);
        }
        result->Add("internalProperties", array);
    }
    if (privateProperties_) {
        array = PtJson::CreateArray();
        len = privateProperties_->size();
        for (size_t i = 0; i < len; i++) {
            std::unique_ptr<PtJson> location = privateProperties_.value()[i]->ToJson();
            array->Push(location);
        }
        result->Add("privateProperties", array);
    }
    if (exceptionDetails_ && exceptionDetails_.value() != nullptr) {
        result->Add("exceptionDetails", exceptionDetails_.value()->ToJson());
    }

    return result;
}

Local<ObjectRef> CallFunctionOnReturns::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> CallFunctionOnReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    if (result_ != nullptr) {
        result->Add("result", result_->ToJson());
    }
    if (exceptionDetails_ && exceptionDetails_.value() != nullptr) {
        result->Add("exceptionDetails", exceptionDetails_.value()->ToJson());
    }

    return result;
}

Local<ObjectRef> StopSamplingReturns::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    if (profile_ != nullptr) {
        Local<ObjectRef> profile = profile_->ToObject(ecmaVm);
        result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "profile")),
            Local<JSValueRef>(profile));
    }
    return result;
}

std::unique_ptr<PtJson> StopSamplingReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    if (profile_ != nullptr) {
        result->Add("profile", profile_->ToJson());
    }

    return result;
}

Local<ObjectRef> GetHeapObjectIdReturns::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    result->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "heapSnapshotObjectId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, std::to_string(heapSnapshotObjectId_).c_str())));

    return result;
}

std::unique_ptr<PtJson> GetHeapObjectIdReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("heapSnapshotObjectId", std::to_string(heapSnapshotObjectId_).c_str());

    return result;
}

Local<ObjectRef> GetObjectByHeapObjectIdReturns::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> result = NewObject(ecmaVm);
    
    if (remoteObjectResult_ != nullptr) {
        Local<ObjectRef> remoteObjectResult = remoteObjectResult_->ToObject(ecmaVm);
        result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "result")),
            Local<JSValueRef>(remoteObjectResult));
    }

    return result;
}

std::unique_ptr<PtJson> GetObjectByHeapObjectIdReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    if (remoteObjectResult_ != nullptr) {
        result->Add("result", remoteObjectResult_->ToJson());
    }

    return result;
}

Local<ObjectRef> StopReturns::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    Local<ObjectRef> profile = profile_->ToObject(ecmaVm);
    result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "profile")), Local<JSValueRef>(profile));

    return result;
}

std::unique_ptr<PtJson> StopReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    if (profile_ != nullptr) {
        result->Add("profile", profile_->ToJson());
    }

    return result;
}

Local<ObjectRef> GetHeapUsageReturns::ToObject(const EcmaVM *ecmaVm) const
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

std::unique_ptr<PtJson> GetHeapUsageReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("usedSize", usedSize_);
    result->Add("totalSize", totalSize_);

    return result;
}

Local<ObjectRef> GetBestEffortCoverageReturns::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    size_t len = result_.size();
    Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
    for (size_t i = 0; i < len; i++) {
        Local<ObjectRef> scriptCoverage = result_[i]->ToObject(ecmaVm);
        values->Set(ecmaVm, i, scriptCoverage);
    }
    result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "result")), values);

    return result;
}

std::unique_ptr<PtJson> GetBestEffortCoverageReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    size_t len = result_.size();
    for (size_t i = 0; i < len; i++) {
        std::unique_ptr<PtJson> scriptCoverage = result_[i]->ToJson();
        array->Push(scriptCoverage);
    }
    result->Add("result", array);

    return result;
}

Local<ObjectRef> StartPreciseCoverageReturns::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> result = NewObject(ecmaVm);
    result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "timestamp")),
        NumberRef::New(ecmaVm, timestamp_));

    return result;
}

std::unique_ptr<PtJson> StartPreciseCoverageReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("timestamp", timestamp_);

    return result;
}

Local<ObjectRef> TakePreciseCoverageReturns::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> returns = NewObject(ecmaVm);

    size_t len = result_.size();
    Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
    for (size_t i = 0; i < len; i++) {
        Local<ObjectRef> scriptCoverage = result_[i]->ToObject(ecmaVm);
        values->Set(ecmaVm, i, scriptCoverage);
    }
    returns->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "result")), values);
    returns->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "timestamp")),
        NumberRef::New(ecmaVm, timestamp_));

    return returns;
}

std::unique_ptr<PtJson> TakePreciseCoverageReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    size_t len = result_.size();
    for (size_t i = 0; i < len; i++) {
        std::unique_ptr<PtJson> scriptTypeProfile = result_[i]->ToJson();
        array->Push(scriptTypeProfile);
    }
    result->Add("result", array);
    result->Add("timestamp", timestamp_);

    return result;
}

Local<ObjectRef> TakeTypeProfileReturns::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> result = NewObject(ecmaVm);

    size_t len = result_.size();
    Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
    for (size_t i = 0; i < len; i++) {
        Local<ObjectRef> scriptTypeProfile = result_[i]->ToObject(ecmaVm);
        values->Set(ecmaVm, i, scriptTypeProfile);
    }
    result->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "result")), values);
    return result;
}

std::unique_ptr<PtJson> TakeTypeProfileReturns::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    size_t len = result_.size();
    for (size_t i = 0; i < len; i++) {
        std::unique_ptr<PtJson> scriptTypeProfile = result_[i]->ToJson();
        array->Push(scriptTypeProfile);
    }
    result->Add("result", array);

    return result;
}
}  // namespace panda::ecmascript::tooling