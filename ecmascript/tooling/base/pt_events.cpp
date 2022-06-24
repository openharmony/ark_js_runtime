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

#include "ecmascript/tooling/base/pt_events.h"

namespace panda::ecmascript::tooling {
std::unique_ptr<PtJson> BreakpointResolved::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();
    result->Add("breakpointId", breakpointId_.c_str());
    ASSERT(location_ != nullptr);
    result->Add("location", location_->ToJson());

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> Paused::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    size_t len = callFrames_.size();
    for (size_t i = 0; i < len; i++) {
        ASSERT(callFrames_[i] != nullptr);
        array->Push(callFrames_[i]->ToJson());
    }
    result->Add("callFrames", array);
    result->Add("reason", reason_.c_str());
    if (data_) {
        ASSERT(data_.value() != nullptr);
        result->Add("data", data_.value()->ToJson());
    }
    if (hitBreakpoints_) {
        std::unique_ptr<PtJson> breakpoints = PtJson::CreateArray();
        len = hitBreakpoints_->size();
        for (size_t i = 0; i < len; i++) {
            breakpoints->Push(hitBreakpoints_.value()[i].c_str());
        }
        result->Add("hitBreakpoints", breakpoints);
    }

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> Resumed::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> ScriptFailedToParse::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("scriptId", std::to_string(scriptId_).c_str());
    result->Add("url", url_.c_str());
    result->Add("startLine", startLine_);
    result->Add("startColumn", startColumn_);
    result->Add("endLine", endLine_);
    result->Add("endColumn", endColumn_);
    result->Add("executionContextId", executionContextId_);
    result->Add("hash", hash_.c_str());
    if (sourceMapUrl_) {
        result->Add("sourceMapURL", sourceMapUrl_->c_str());
    }
    if (hasSourceUrl_) {
        result->Add("hasSourceURL", hasSourceUrl_.value());
    }
    if (isModule_) {
        result->Add("isModule", isModule_.value());
    }
    if (length_) {
        result->Add("length", length_.value());
    }
    if (codeOffset_) {
        result->Add("codeOffset", codeOffset_.value());
    }
    if (scriptLanguage_) {
        result->Add("scriptLanguage", scriptLanguage_->c_str());
    }
    if (embedderName_) {
        result->Add("embedderName", embedderName_->c_str());
    }

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> ScriptParsed::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("scriptId", std::to_string(scriptId_).c_str());
    result->Add("url", url_.c_str());
    result->Add("startLine", startLine_);
    result->Add("startColumn", startColumn_);
    result->Add("endLine", endLine_);
    result->Add("endColumn", endColumn_);
    result->Add("executionContextId", executionContextId_);
    result->Add("hash", hash_.c_str());
    if (isLiveEdit_) {
        result->Add("isLiveEdit", isLiveEdit_.value());
    }
    if (sourceMapUrl_) {
        result->Add("sourceMapURL", sourceMapUrl_->c_str());
    }
    if (hasSourceUrl_) {
        result->Add("hasSourceURL", hasSourceUrl_.value());
    }
    if (isModule_) {
        result->Add("isModule", isModule_.value());
    }
    if (length_) {
        result->Add("length", length_.value());
    }
    if (codeOffset_) {
        result->Add("codeOffset", codeOffset_.value());
    }
    if (scriptLanguage_) {
        result->Add("scriptLanguage", scriptLanguage_->c_str());
    }
    if (embedderName_) {
        result->Add("embedderName", embedderName_->c_str());
    }

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> AddHeapSnapshotChunk::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("chunk", chunk_.c_str());

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> ConsoleProfileFinished::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("id", id_.c_str());
    ASSERT(location_ != nullptr);
    result->Add("location", location_->ToJson());
    ASSERT(profile_ != nullptr);
    result->Add("profile", profile_->ToJson());
    if (title_) {
        result->Add("title", title_->c_str());
    }

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> ConsoleProfileStarted::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("id", id_.c_str());
    ASSERT(location_ != nullptr);
    result->Add("location", location_->ToJson());
    if (title_) {
        result->Add("title", title_->c_str());
    }

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> PreciseCoverageDeltaUpdate::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("timestamp", timestamp_);
    result->Add("occasion", occasion_.c_str());
    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    size_t len = result_.size();
    for (size_t i = 0; i < len; i++) {
        ASSERT(result_[i] != nullptr);
        std::unique_ptr<PtJson> res = result_[i]->ToJson();
        array->Push(res);
    }
    result->Add("result", array);

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> HeapStatsUpdate::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    size_t len = statsUpdate_.size();
    for (size_t i = 0; i < len; i++) {
        array->Push(statsUpdate_[i]);
    }
    result->Add("statsUpdate", array);

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> LastSeenObjectId::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("lastSeenObjectId", lastSeenObjectId_);
    result->Add("timestamp", timestamp_);

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> ReportHeapSnapshotProgress::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("done", done_);
    result->Add("total", total_);
    if (finished_) {
        result->Add("finished", finished_.value());
    }

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> BufferUsage::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    if (percentFull_) {
        result->Add("percentFull", percentFull_.value());
    }
    if (eventCount_) {
        result->Add("eventCount", eventCount_.value());
    }
    if (value_) {
        result->Add("value", value_.value());
    }

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> DataCollected::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    std::unique_ptr<PtJson> array = PtJson::CreateArray();
    size_t len = value_.size();
    for (size_t i = 0; i < len; i++) {
        array->Push(value_[i]);
    }
    result->Add("value", array);

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}

std::unique_ptr<PtJson> TracingComplete::ToJson() const
{
    std::unique_ptr<PtJson> result = PtJson::CreateObject();

    result->Add("dataLossOccurred", dataLossOccurred_);
    if (traceFormat_) {
        result->Add("traceFormat", traceFormat_.value()->c_str());
    }
    if (streamCompression_) {
        result->Add("streamCompression", streamCompression_.value()->c_str());
    }

    std::unique_ptr<PtJson> object = PtJson::CreateObject();
    object->Add("method", GetName().c_str());
    object->Add("params", result);

    return object;
}
}  // namespace panda::ecmascript::tooling
