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
Local<ObjectRef> BreakpointResolved::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "breakpointId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, breakpointId_.c_str())));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "location")),
        Local<JSValueRef>(location_->ToObject(ecmaVm)));

    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

Local<ObjectRef> Paused::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    size_t len = callFrames_.size();
    Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
    for (size_t i = 0; i < len; i++) {
        Local<ObjectRef> callFrame = callFrames_[i]->ToObject(ecmaVm);
        values->Set(ecmaVm, i, callFrame);
    }
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "callFrames")), values);
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "reason")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, reason_.c_str())));
    if (data_) {
        params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "data")),
            Local<JSValueRef>(data_.value()->ToObject(ecmaVm)));
    }
    if (hitBreakpoints_) {
        len = hitBreakpoints_->size();
        values = ArrayRef::New(ecmaVm, len);
        for (size_t i = 0; i < len; i++) {
            Local<StringRef> id = StringRef::NewFromUtf8(ecmaVm, hitBreakpoints_.value()[i].c_str());
            values->Set(ecmaVm, i, id);
        }
        params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "hitBreakpoints")), values);
    }

    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

Local<ObjectRef> Resumed::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

Local<ObjectRef> ScriptFailedToParse::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, std::to_string(scriptId_).c_str())));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, url_.c_str())));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startLine")),
        IntegerRef::New(ecmaVm, startLine_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startColumn")),
        IntegerRef::New(ecmaVm, startColumn_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endLine")),
        IntegerRef::New(ecmaVm, endLine_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endColumn")),
        IntegerRef::New(ecmaVm, endColumn_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "executionContextId")),
        IntegerRef::New(ecmaVm, executionContextId_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "hash")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, hash_.c_str())));
    if (execContextAuxData_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "executionContextAuxData")),
            Local<JSValueRef>(execContextAuxData_.value()));
    }
    if (sourceMapUrl_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "sourceMapURL")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, sourceMapUrl_->c_str())));
    }
    if (hasSourceUrl_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "hasSourceURL")),
            BooleanRef::New(ecmaVm, hasSourceUrl_.value()));
    }
    if (isModule_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "isModule")),
            BooleanRef::New(ecmaVm, isModule_.value()));
    }
    if (length_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "length")),
            IntegerRef::New(ecmaVm, length_.value()));
    }
    if (codeOffset_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "codeOffset")),
            IntegerRef::New(ecmaVm, codeOffset_.value()));
    }
    if (scriptLanguage_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptLanguage")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, scriptLanguage_->c_str())));
    }
    if (embedderName_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "embedderName")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, embedderName_->c_str())));
    }

    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

Local<ObjectRef> ScriptParsed::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptId")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, std::to_string(scriptId_).c_str())));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "url")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, url_.c_str())));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startLine")),
        IntegerRef::New(ecmaVm, startLine_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "startColumn")),
        IntegerRef::New(ecmaVm, startColumn_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endLine")),
        IntegerRef::New(ecmaVm, endLine_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "endColumn")),
        IntegerRef::New(ecmaVm, endColumn_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "executionContextId")),
        IntegerRef::New(ecmaVm, executionContextId_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "hash")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, hash_.c_str())));
    if (execContextAuxData_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "executionContextAuxData")),
            Local<JSValueRef>(execContextAuxData_.value()));
    }
    if (isLiveEdit_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "isLiveEdit")),
            BooleanRef::New(ecmaVm, isLiveEdit_.value()));
    }
    if (sourceMapUrl_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "sourceMapURL")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, sourceMapUrl_->c_str())));
    }
    if (hasSourceUrl_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "hasSourceURL")),
            BooleanRef::New(ecmaVm, hasSourceUrl_.value()));
    }
    if (isModule_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "isModule")),
            BooleanRef::New(ecmaVm, isModule_.value()));
    }
    if (length_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "length")),
            IntegerRef::New(ecmaVm, length_.value()));
    }
    if (codeOffset_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "codeOffset")),
            IntegerRef::New(ecmaVm, codeOffset_.value()));
    }
    if (scriptLanguage_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "scriptLanguage")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, scriptLanguage_->c_str())));
    }
    if (embedderName_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "embedderName")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, embedderName_->c_str())));
    }
    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

Local<ObjectRef> AddHeapSnapshotChunk::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "chunk")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, chunk_.c_str())));

    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

Local<ObjectRef> ConsoleProfileFinished::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "id")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, id_.c_str())));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "location")),
        Local<JSValueRef>(location_->ToObject(ecmaVm)));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "profile")),
        Local<JSValueRef>(profile_->ToObject(ecmaVm)));
    if (title_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "title")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, title_->c_str())));
    }
    
    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

Local<ObjectRef> ConsoleProfileStarted::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "id")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, id_.c_str())));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "location")),
        Local<JSValueRef>(location_->ToObject(ecmaVm)));
    if (title_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "title")),
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, title_->c_str())));
    }
    
    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

Local<ObjectRef> PreciseCoverageDeltaUpdate::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "timestamp")),
        NumberRef::New(ecmaVm, timestamp_));
    params->Set(ecmaVm,
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "occasion")),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, occasion_.c_str())));
    size_t len = result_.size();
    Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
    for (size_t i = 0; i < len; i++) {
        Local<ObjectRef> result = result_[i]->ToObject(ecmaVm);
        values->Set(ecmaVm, i, result);
    }
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "result")), values);
    
    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

Local<ObjectRef> HeapStatsUpdate::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    size_t len = statsUpdate_.size();
    Local<ArrayRef> values = ArrayRef::New(ecmaVm, len);
    for (size_t i = 0; i < len; i++) {
        Local<IntegerRef> elem = IntegerRef::New(ecmaVm, statsUpdate_[i]);
        values->Set(ecmaVm, i, elem);
    }
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "statsUpdate")), values);
    
    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

Local<ObjectRef> LastSeenObjectId::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "lastSeenObjectId")),
        IntegerRef::New(ecmaVm, lastSeenObjectId_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "timestamp")),
        NumberRef::New(ecmaVm, timestamp_));

    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}

Local<ObjectRef> ReportHeapSnapshotProgress::ToObject(const EcmaVM *ecmaVm) const
{
    Local<ObjectRef> params = NewObject(ecmaVm);

    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "done")),
        IntegerRef::New(ecmaVm, done_));
    params->Set(ecmaVm, Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "total")),
        IntegerRef::New(ecmaVm, total_));

    if (finished_) {
        params->Set(ecmaVm,
            Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, "finished")),
            BooleanRef::New(ecmaVm, finished_.value()));
    }

    Local<ObjectRef> object = NewObject(ecmaVm);
    object->Set(ecmaVm,
        StringRef::NewFromUtf8(ecmaVm, "method"),
        Local<JSValueRef>(StringRef::NewFromUtf8(ecmaVm, GetName().c_str())));
    object->Set(ecmaVm, StringRef::NewFromUtf8(ecmaVm, "params"), Local<JSValueRef>(params));

    return object;
}
}  // namespace panda::ecmascript::tooling
