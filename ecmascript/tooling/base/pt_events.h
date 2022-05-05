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

#ifndef ECMASCRIPT_TOOLING_BASE_PT_EVENTS_H
#define ECMASCRIPT_TOOLING_BASE_PT_EVENTS_H

#include <memory>
#include <optional>

#include "libpandabase/macros.h"
#include "ecmascript/tooling/base/pt_script.h"
#include "ecmascript/tooling/base/pt_types.h"
#include "ecmascript/tooling/dispatcher.h"

namespace panda::ecmascript::tooling {
class PtBaseEvents : public PtBaseTypes {
public:
    PtBaseEvents() = default;
    ~PtBaseEvents() override = default;
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override = 0;
    virtual CString GetName() = 0;

private:
    NO_COPY_SEMANTIC(PtBaseEvents);
    NO_MOVE_SEMANTIC(PtBaseEvents);
};

class BreakpointResolved final : public PtBaseEvents {
public:
    BreakpointResolved() = default;
    ~BreakpointResolved() override = default;
    static std::unique_ptr<BreakpointResolved> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

    CString GetName() override
    {
        return "Debugger.breakpointResolved";
    }

    BreakpointId GetBreakpointId() const
    {
        return breakpointId_;
    }

    BreakpointResolved &SetBreakpointId(const BreakpointId &breakpointId)
    {
        breakpointId_ = breakpointId;
        return *this;
    }

    Location *GetLocation() const
    {
        return location_.get();
    }

    BreakpointResolved &SetLocation(std::unique_ptr<Location> location)
    {
        location_ = std::move(location);
        return *this;
    }

private:
    NO_COPY_SEMANTIC(BreakpointResolved);
    NO_MOVE_SEMANTIC(BreakpointResolved);

    BreakpointId breakpointId_ {};
    std::unique_ptr<Location> location_ {nullptr};
};

class Paused final : public PtBaseEvents {
public:
    Paused() = default;
    ~Paused() override = default;
    static std::unique_ptr<Paused> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

    CString GetName() override
    {
        return "Debugger.paused";
    }

    const CVector<std::unique_ptr<CallFrame>> *GetCallFrames() const
    {
        return &callFrames_;
    }

    Paused &SetCallFrames(CVector<std::unique_ptr<CallFrame>> callFrames)
    {
        callFrames_ = std::move(callFrames);
        return *this;
    }

    const CString &GetReason() const
    {
        return reason_;
    }

    Paused &SetReason(PauseReason reason)
    {
        reason_ = GetReasonString(reason);
        return *this;
    }

    static CString GetReasonString(PauseReason reason)
    {
        switch (reason) {
            case AMBIGUOUS: {
                return "ambiguous";
            }
            case ASSERT: {
                return "assert";
            }
            case DEBUGCOMMAND: {
                return "debugCommand";
            }
            case DOM: {
                return "DOM";
            }
            case EVENTLISTENER: {
                return "EventListener";
            }
            case EXCEPTION: {
                return "exception";
            }
            case INSTRUMENTATION: {
                return "instrumentation";
            }
            case OOM: {
                return "OOM";
            }
            case OTHER: {
                return "other";
            }
            case PROMISEREJECTION: {
                return "promiseRejection";
            }
            case XHR: {
                return "XHR";
            }
            case BREAK_ON_START: {
                return "Break on start";
            }
            default: {
                LOG(ERROR, DEBUGGER) << "Unknown paused reason: " << reason;
            }
        }
        return "";
    }

    RemoteObject *GetData() const
    {
        if (data_) {
            return data_->get();
        }
        return nullptr;
    }

    Paused &SetData(std::unique_ptr<RemoteObject> data)
    {
        data_ = std::move(data);
        return *this;
    }

    bool HasData() const
    {
        return data_.has_value();
    }

    CVector<BreakpointId> GetHitBreakpoints() const
    {
        return hitBreakpoints_.value_or(CVector<BreakpointId>());
    }

    Paused &SetHitBreakpoints(CVector<BreakpointId> hitBreakpoints)
    {
        hitBreakpoints_ = std::move(hitBreakpoints);
        return *this;
    }

    bool HasHitBreakpoints() const
    {
        return hitBreakpoints_.has_value();
    }

private:
    NO_COPY_SEMANTIC(Paused);
    NO_MOVE_SEMANTIC(Paused);

    CVector<std::unique_ptr<CallFrame>> callFrames_ {};
    CString reason_ {};
    std::optional<std::unique_ptr<RemoteObject>> data_ {};
    std::optional<CVector<BreakpointId>> hitBreakpoints_ {};
};

class Resumed final : public PtBaseEvents {
public:
    Resumed() = default;
    ~Resumed() override = default;
    static std::unique_ptr<Resumed> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

    CString GetName() override
    {
        return "Debugger.resumed";
    }

private:
    NO_COPY_SEMANTIC(Resumed);
    NO_MOVE_SEMANTIC(Resumed);
};

class ScriptFailedToParse final : public PtBaseEvents {
public:
    ScriptFailedToParse() = default;
    ~ScriptFailedToParse() override = default;
    static std::unique_ptr<ScriptFailedToParse> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

    CString GetName() override
    {
        return "Debugger.scriptFailedToParse";
    }

    ScriptId GetScriptId() const
    {
        return scriptId_;
    }

    ScriptFailedToParse &SetScriptId(ScriptId scriptId)
    {
        scriptId_ = scriptId;
        return *this;
    }

    const CString &GetUrl() const
    {
        return url_;
    }

    ScriptFailedToParse &SetUrl(const CString &url)
    {
        url_ = url;
        return *this;
    }

    int32_t GetStartLine() const
    {
        return startLine_;
    }

    ScriptFailedToParse &SetStartLine(int32_t startLine)
    {
        startLine_ = startLine;
        return *this;
    }

    int32_t GetStartColumn() const
    {
        return startColumn_;
    }

    ScriptFailedToParse &SetStartColumn(int32_t startColumn)
    {
        startColumn_ = startColumn;
        return *this;
    }

    int32_t GetEndLine() const
    {
        return endLine_;
    }

    ScriptFailedToParse &SetEndLine(int32_t endLine)
    {
        endLine_ = endLine;
        return *this;
    }

    int32_t GetEndColumn() const
    {
        return endColumn_;
    }

    ScriptFailedToParse &SetEndColumn(int32_t endColumn)
    {
        endColumn_ = endColumn;
        return *this;
    }

    int32_t GetExecutionContextId() const
    {
        return executionContextId_;
    }

    ScriptFailedToParse &SetExecutionContextId(int32_t executionContextId)
    {
        executionContextId_ = executionContextId;
        return *this;
    }

    const CString &GetHash() const
    {
        return hash_;
    }

    ScriptFailedToParse &SetHash(const CString &hash)
    {
        hash_ = hash;
        return *this;
    }

    Local<ObjectRef> GetExecutionContextAuxData() const
    {
        return execContextAuxData_.value_or(Local<ObjectRef>());
    }

    ScriptFailedToParse &SetExecutionContextAuxData(const Local<ObjectRef> &execContextAuxData)
    {
        execContextAuxData_ = execContextAuxData;
        return *this;
    }

    bool HasExecutionContextAuxData() const
    {
        return execContextAuxData_.has_value();
    }

    const CString &GetSourceMapURL() const
    {
        return sourceMapUrl_.value();
    }

    ScriptFailedToParse &SetSourceMapURL(const CString &sourceMapUrl)
    {
        sourceMapUrl_ = sourceMapUrl;
        return *this;
    }

    bool HasSourceMapURL() const
    {
        return sourceMapUrl_.has_value();
    }

    bool GetHasSourceURL() const
    {
        return hasSourceUrl_.value_or(false);
    }

    ScriptFailedToParse &SetHasSourceURL(bool hasSourceUrl)
    {
        hasSourceUrl_ = hasSourceUrl;
        return *this;
    }

    bool HasHasSourceURL() const
    {
        return hasSourceUrl_.has_value();
    }

    bool GetIsModule() const
    {
        return isModule_.value_or(false);
    }

    ScriptFailedToParse &SetIsModule(bool isModule)
    {
        isModule_ = isModule;
        return *this;
    }

    bool HasIsModule() const
    {
        return isModule_.has_value();
    }

    int32_t GetLength() const
    {
        return length_.value_or(0);
    }

    ScriptFailedToParse &SetLength(int32_t length)
    {
        length_ = length;
        return *this;
    }

    bool HasLength() const
    {
        return length_.has_value();
    }

    int32_t GetCodeOffset() const
    {
        return codeOffset_.value_or(0);
    }

    ScriptFailedToParse &SetCodeOffset(int32_t codeOffset)
    {
        codeOffset_ = codeOffset;
        return *this;
    }

    bool HasCodeOffset() const
    {
        return codeOffset_.has_value();
    }

    const CString &GetScriptLanguage() const
    {
        return scriptLanguage_.value();
    }

    ScriptFailedToParse &SetScriptLanguage(const CString &scriptLanguage)
    {
        scriptLanguage_ = scriptLanguage;
        return *this;
    }

    bool HasScriptLanguage() const
    {
        return scriptLanguage_.has_value();
    }

    const CString &GetEmbedderName() const
    {
        return embedderName_.value();
    }

    ScriptFailedToParse &SetEmbedderName(const CString &embedderName)
    {
        embedderName_ = embedderName;
        return *this;
    }

    bool HasEmbedderName() const
    {
        return embedderName_.has_value();
    }

private:
    NO_COPY_SEMANTIC(ScriptFailedToParse);
    NO_MOVE_SEMANTIC(ScriptFailedToParse);

    ScriptId scriptId_ {};
    CString url_ {};
    int32_t startLine_ {0};
    int32_t startColumn_ {0};
    int32_t endLine_ {0};
    int32_t endColumn_ {0};
    ExecutionContextId executionContextId_ {0};
    CString hash_ {};
    std::optional<Local<ObjectRef>> execContextAuxData_ {};
    std::optional<CString> sourceMapUrl_ {};
    std::optional<bool> hasSourceUrl_ {};
    std::optional<bool> isModule_ {};
    std::optional<int32_t> length_ {};
    std::optional<int32_t> codeOffset_ {};
    std::optional<CString> scriptLanguage_ {};
    std::optional<CString> embedderName_ {};
};

class ScriptParsed final : public PtBaseEvents {
public:
    ScriptParsed() = default;
    ~ScriptParsed() override = default;
    static std::unique_ptr<ScriptParsed> Create(const std::unique_ptr<PtScript> &script);
    static std::unique_ptr<ScriptParsed> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

    CString GetName() override
    {
        return "Debugger.scriptParsed";
    }

    ScriptId GetScriptId() const
    {
        return scriptId_;
    }

    ScriptParsed &SetScriptId(ScriptId scriptId)
    {
        scriptId_ = scriptId;
        return *this;
    }

    const CString &GetUrl() const
    {
        return url_;
    }

    ScriptParsed &SetUrl(const CString &url)
    {
        url_ = url;
        return *this;
    }

    int32_t GetStartLine() const
    {
        return startLine_;
    }

    ScriptParsed &SetStartLine(int32_t startLine)
    {
        startLine_ = startLine;
        return *this;
    }

    int32_t GetStartColumn() const
    {
        return startColumn_;
    }

    ScriptParsed &SetStartColumn(int32_t startColumn)
    {
        startColumn_ = startColumn;
        return *this;
    }

    int32_t GetEndLine() const
    {
        return endLine_;
    }

    ScriptParsed &SetEndLine(int32_t endLine)
    {
        endLine_ = endLine;
        return *this;
    }

    int32_t GetEndColumn() const
    {
        return endColumn_;
    }

    ScriptParsed &SetEndColumn(int32_t endColumn)
    {
        endColumn_ = endColumn;
        return *this;
    }

    int32_t GetExecutionContextId() const
    {
        return executionContextId_;
    }

    ScriptParsed &SetExecutionContextId(int32_t executionContextId)
    {
        executionContextId_ = executionContextId;
        return *this;
    }

    const CString &GetHash() const
    {
        return hash_;
    }

    ScriptParsed &SetHash(const CString &hash)
    {
        hash_ = hash;
        return *this;
    }

    bool GetIsLiveEdit() const
    {
        return isLiveEdit_.value_or(false);
    }

    ScriptParsed &SetIsLiveEdit(bool isLiveEdit)
    {
        isLiveEdit_ = isLiveEdit;
        return *this;
    }

    bool HasIsLiveEdit() const
    {
        return isLiveEdit_.has_value();
    }

    Local<ObjectRef> GetExecutionContextAuxData() const
    {
        return execContextAuxData_.value_or(Local<ObjectRef>());
    }

    ScriptParsed &SetExecutionContextAuxData(const Local<ObjectRef> &execContextAuxData)
    {
        execContextAuxData_ = execContextAuxData;
        return *this;
    }

    bool HasExecutionContextAuxData() const
    {
        return execContextAuxData_.has_value();
    }

    const CString &GetSourceMapURL() const
    {
        return sourceMapUrl_.value();
    }

    ScriptParsed &SetSourceMapURL(const CString &sourceMapUrl)
    {
        sourceMapUrl_ = sourceMapUrl;
        return *this;
    }

    bool HasSourceMapURL() const
    {
        return sourceMapUrl_.has_value();
    }

    bool GetHasSourceURL() const
    {
        return hasSourceUrl_.value_or(false);
    }

    ScriptParsed &SetHasSourceURL(bool hasSourceUrl)
    {
        hasSourceUrl_ = hasSourceUrl;
        return *this;
    }

    bool HasHasSourceURL() const
    {
        return hasSourceUrl_.has_value();
    }

    bool GetIsModule() const
    {
        return isModule_.value_or(false);
    }

    ScriptParsed &SetIsModule(bool isModule)
    {
        isModule_ = isModule;
        return *this;
    }

    bool HasIsModule() const
    {
        return isModule_.has_value();
    }

    uint32_t GetLength() const
    {
        return length_.value_or(0);
    }

    ScriptParsed &SetLength(uint32_t length)
    {
        length_ = length;
        return *this;
    }

    bool HasLength() const
    {
        return length_.has_value();
    }

    uint32_t GetCodeOffset() const
    {
        return codeOffset_.value_or(0);
    }

    ScriptParsed &SetCodeOffset(uint32_t codeOffset)
    {
        codeOffset_ = codeOffset;
        return *this;
    }

    bool HasCodeOffset() const
    {
        return codeOffset_.has_value();
    }

    const CString &GetScriptLanguage() const
    {
        return scriptLanguage_.value();
    }

    ScriptParsed &SetScriptLanguage(const CString &scriptLanguage)
    {
        scriptLanguage_ = scriptLanguage;
        return *this;
    }

    bool HasScriptLanguage() const
    {
        return scriptLanguage_.has_value();
    }

    const CString &GetEmbedderName() const
    {
        return embedderName_.value();
    }

    ScriptParsed &SetEmbedderName(const CString &embedderName)
    {
        embedderName_ = embedderName;
        return *this;
    }

    bool HasEmbedderName() const
    {
        return embedderName_.has_value();
    }

private:
    NO_COPY_SEMANTIC(ScriptParsed);
    NO_MOVE_SEMANTIC(ScriptParsed);

    ScriptId scriptId_ {};
    CString url_ {};
    int32_t startLine_ {0};
    int32_t startColumn_ {0};
    int32_t endLine_ {0};
    int32_t endColumn_ {0};
    ExecutionContextId executionContextId_ {0};
    CString hash_ {};
    std::optional<Local<ObjectRef>> execContextAuxData_ {};
    std::optional<bool> isLiveEdit_ {};
    std::optional<CString> sourceMapUrl_ {};
    std::optional<bool> hasSourceUrl_ {};
    std::optional<bool> isModule_ {};
    std::optional<uint32_t> length_ {};
    std::optional<uint32_t> codeOffset_ {};
    std::optional<CString> scriptLanguage_ {};
    std::optional<CString> embedderName_ {};
};

class AddHeapSnapshotChunk final : public PtBaseEvents {
public:
    AddHeapSnapshotChunk() = default;
    ~AddHeapSnapshotChunk() override = default;
    static std::unique_ptr<AddHeapSnapshotChunk> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    static std::unique_ptr<AddHeapSnapshotChunk> Create(char* data, int size);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

    CString GetName() override
    {
        return "HeapProfiler.addHeapSnapshotChunk";
    }

private:
    NO_COPY_SEMANTIC(AddHeapSnapshotChunk);
    NO_MOVE_SEMANTIC(AddHeapSnapshotChunk);

    CString chunk_ {};
};

class ConsoleProfileFinished final : public PtBaseEvents {
public:
    ConsoleProfileFinished() = default;
    ~ConsoleProfileFinished() override = default;
    static std::unique_ptr<ConsoleProfileFinished> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;
    CString GetName() override
    {
        return "Profile.ConsoleProfileFinished";
    }

    const CString &GetId() const
    {
        return id_;
    }

    ConsoleProfileFinished &SetId(const CString &id)
    {
        id_ = id;
        return *this;
    }

    Location *GetLocation() const
    {
        return location_.get();
    }

    ConsoleProfileFinished &SetLocation(std::unique_ptr<Location> location)
    {
        location_ = std::move(location);
        return *this;
    }

    Profile *GetProfile() const
    {
        return profile_.get();
    }

    ConsoleProfileFinished &SetProfile(std::unique_ptr<Profile> profile)
    {
        profile_ = std::move(profile);
        return *this;
    }

    const CString &GetTitle() const
    {
        return title_.value();
    }

    ConsoleProfileFinished &SetTitle(const CString &title)
    {
        title_ = title;
        return *this;
    }

    bool HasTitle() const
    {
        return title_.has_value();
    }

private:
    NO_COPY_SEMANTIC(ConsoleProfileFinished);
    NO_MOVE_SEMANTIC(ConsoleProfileFinished);

    CString id_ {};
    std::unique_ptr<Location> location_ {nullptr};
    std::unique_ptr<Profile> profile_ {nullptr};
    std::optional<CString> title_ {};
};

class ConsoleProfileStarted final : public PtBaseEvents {
public:
    ConsoleProfileStarted() = default;
    ~ConsoleProfileStarted() override = default;
    static std::unique_ptr<ConsoleProfileStarted> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;
    CString GetName() override
    {
        return "Profile.ConsoleProfileStarted";
    }

    const CString &GetId() const
    {
        return id_;
    }

    ConsoleProfileStarted &SetId(const CString &id)
    {
        id_ = id;
        return *this;
    }

    Location *GetLocation() const
    {
        return location_.get();
    }

    ConsoleProfileStarted &SetLocation(std::unique_ptr<Location> location)
    {
        location_ = std::move(location);
        return *this;
    }

    const CString &GetTitle() const
    {
        return title_.value();
    }

    ConsoleProfileStarted &SetTitle(const CString &title)
    {
        title_ = title;
        return *this;
    }

    bool HasTitle() const
    {
        return title_.has_value();
    }

private:
    NO_COPY_SEMANTIC(ConsoleProfileStarted);
    NO_MOVE_SEMANTIC(ConsoleProfileStarted);

    CString id_ {};
    std::unique_ptr<Location> location_ {nullptr};
    std::optional<CString> title_ {};
};

class PreciseCoverageDeltaUpdate final : public PtBaseEvents {
public:
    PreciseCoverageDeltaUpdate() = default;
    ~PreciseCoverageDeltaUpdate() override = default;
    static std::unique_ptr<PreciseCoverageDeltaUpdate> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;
    CString GetName() override
    {
        return "Profile.PreciseCoverageDeltaUpdate";
    }

    size_t GetTimestamp() const
    {
        return timestamp_;
    }

    PreciseCoverageDeltaUpdate &SetTimestamp(size_t timestamp)
    {
        timestamp_ = timestamp;
        return *this;
    }

    const CString &GetOccasion() const
    {
        return occasion_;
    }

    PreciseCoverageDeltaUpdate &SetOccasion(const CString &occasion)
    {
        occasion_ = occasion;
        return *this;
    }

    const CVector<std::unique_ptr<ScriptCoverage>> *GetResult() const
    {
        return &result_;
    }

    PreciseCoverageDeltaUpdate &SetResult(CVector<std::unique_ptr<ScriptCoverage>> result)
    {
        result_ = std::move(result);
        return *this;
    }

private:
    NO_COPY_SEMANTIC(PreciseCoverageDeltaUpdate);
    NO_MOVE_SEMANTIC(PreciseCoverageDeltaUpdate);

    size_t timestamp_ {0};
    CString occasion_ {};
    CVector<std::unique_ptr<ScriptCoverage>> result_ {};
};

class HeapStatsUpdate final : public PtBaseEvents {
public:
    HeapStatsUpdate() = default;
    ~HeapStatsUpdate() override = default;
    static std::unique_ptr<HeapStatsUpdate> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

    CString GetName() override
    {
        return "HeapProfiler.heapStatsUpdate";
    }

    const CVector<int32_t> *GetStatsUpdate() const
    {
        return &statsUpdate_;
    }

    HeapStatsUpdate &SetStatsUpdate(CVector<int32_t> statsUpdate)
    {
        statsUpdate_ = std::move(statsUpdate);
        return *this;
    }

private:
    NO_COPY_SEMANTIC(HeapStatsUpdate);
    NO_MOVE_SEMANTIC(HeapStatsUpdate);

    CVector<int32_t> statsUpdate_ {};
};

class LastSeenObjectId final : public PtBaseEvents {
public:
    LastSeenObjectId() = default;
    ~LastSeenObjectId() override = default;
    static std::unique_ptr<LastSeenObjectId> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

    CString GetName() override
    {
        return "HeapProfiler.lastSeenObjectId";
    }

    int32_t GetLastSeenObjectId() const
    {
        return lastSeenObjectId_;
    }

    LastSeenObjectId &SetLastSeenObjectId(int32_t lastSeenObjectId)
    {
        lastSeenObjectId_ = lastSeenObjectId;
        return *this;
    }

    size_t GetTimestamp() const
    {
        return timestamp_;
    }

    LastSeenObjectId &SetTimestamp(size_t timestamp)
    {
        timestamp_ = timestamp;
        return *this;
    }

private:
    NO_COPY_SEMANTIC(LastSeenObjectId);
    NO_MOVE_SEMANTIC(LastSeenObjectId);

    int32_t lastSeenObjectId_ {};
    size_t timestamp_ {};
};

class ReportHeapSnapshotProgress final : public PtBaseEvents {
public:
    ReportHeapSnapshotProgress() = default;
    ~ReportHeapSnapshotProgress() override = default;
    static std::unique_ptr<ReportHeapSnapshotProgress> Create(const EcmaVM *ecmaVm, const Local<JSValueRef> &params);
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

    CString GetName() override
    {
        return "HeapProfiler.reportHeapSnapshotProgress";
    }

    int32_t GetDone() const
    {
        return done_;
    }

    ReportHeapSnapshotProgress &SetDone(int32_t done)
    {
        done_ = done;
        return *this;
    }

    int32_t GetTotal() const
    {
        return total_;
    }

    ReportHeapSnapshotProgress &SetTotal(int32_t total)
    {
        total_ = total;
        return *this;
    }

    bool GetFinished() const
    {
        return finished_.value_or(false);
    }

    ReportHeapSnapshotProgress &SetFinished(bool finished)
    {
        finished_ = finished;
        return *this;
    }

private:
    NO_COPY_SEMANTIC(ReportHeapSnapshotProgress);
    NO_MOVE_SEMANTIC(ReportHeapSnapshotProgress);

    int32_t done_ {};
    int32_t total_ {};
    std::optional<bool> finished_ {};
};
}  // namespace panda::ecmascript::tooling
#endif
