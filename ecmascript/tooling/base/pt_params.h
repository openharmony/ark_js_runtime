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

#ifndef ECMASCRIPT_TOOLING_BASE_PT_PARAMS_H
#define ECMASCRIPT_TOOLING_BASE_PT_PARAMS_H

#include "ecmascript/tooling/base/pt_types.h"

namespace panda::ecmascript::tooling {
class PtBaseParams : public PtBaseTypes {
public:
    PtBaseParams() = default;
    ~PtBaseParams() override = default;
    std::unique_ptr<PtJson> ToJson() const override
    {
        UNREACHABLE();
    }

private:
    NO_COPY_SEMANTIC(PtBaseParams);
    NO_MOVE_SEMANTIC(PtBaseParams);
};

class EnableParams : public PtBaseParams {
public:
    EnableParams() = default;
    ~EnableParams() override = default;

    static std::unique_ptr<EnableParams> Create(const PtJson &params);

    double GetMaxScriptsCacheSize() const
    {
        return maxScriptsCacheSize_.value_or(0);
    }

    bool HasMaxScriptsCacheSize() const
    {
        return maxScriptsCacheSize_.has_value();
    }

private:
    NO_COPY_SEMANTIC(EnableParams);
    NO_MOVE_SEMANTIC(EnableParams);

    std::optional<double> maxScriptsCacheSize_ {};
};

class EvaluateOnCallFrameParams : public PtBaseParams {
public:
    EvaluateOnCallFrameParams() = default;
    ~EvaluateOnCallFrameParams() override = default;

    static std::unique_ptr<EvaluateOnCallFrameParams> Create(const PtJson &params);

    CallFrameId GetCallFrameId() const
    {
        return callFrameId_;
    }

    const std::string &GetExpression() const
    {
        return expression_;
    }

private:
    NO_COPY_SEMANTIC(EvaluateOnCallFrameParams);
    NO_MOVE_SEMANTIC(EvaluateOnCallFrameParams);

    CallFrameId callFrameId_ {};
    std::string expression_ {};
    std::optional<std::string> objectGroup_ {};
    std::optional<bool> includeCommandLineAPI_ {};
    std::optional<bool> silent_ {};
    std::optional<bool> returnByValue_ {};
    std::optional<bool> generatePreview_ {};
    std::optional<bool> throwOnSideEffect_ {};
};

class GetPossibleBreakpointsParams : public PtBaseParams {
public:
    GetPossibleBreakpointsParams() = default;
    ~GetPossibleBreakpointsParams() override = default;

    static std::unique_ptr<GetPossibleBreakpointsParams> Create(const PtJson &params);

    Location *GetStart() const
    {
        return start_.get();
    }

    Location *GetEnd() const
    {
        if (end_) {
            return end_->get();
        }
        return nullptr;
    }

    bool HasEnd() const
    {
        return end_.has_value();
    }

    bool GetRestrictToFunction() const
    {
        return restrictToFunction_.value_or(false);
    }

    bool HasRestrictToFunction() const
    {
        return restrictToFunction_.has_value();
    }

private:
    NO_COPY_SEMANTIC(GetPossibleBreakpointsParams);
    NO_MOVE_SEMANTIC(GetPossibleBreakpointsParams);

    std::unique_ptr<Location> start_ {nullptr};
    std::optional<std::unique_ptr<Location>> end_ {};
    std::optional<bool> restrictToFunction_ {};
};

class GetScriptSourceParams : public PtBaseParams {
public:
    GetScriptSourceParams() = default;
    ~GetScriptSourceParams() override = default;

    static std::unique_ptr<GetScriptSourceParams> Create(const PtJson &params);

    ScriptId GetScriptId() const
    {
        return scriptId_;
    }

private:
    NO_COPY_SEMANTIC(GetScriptSourceParams);
    NO_MOVE_SEMANTIC(GetScriptSourceParams);

    ScriptId scriptId_ {0};
};

class RemoveBreakpointParams : public PtBaseParams {
public:
    RemoveBreakpointParams() = default;
    ~RemoveBreakpointParams() override = default;

    static std::unique_ptr<RemoveBreakpointParams> Create(const PtJson &params);

    BreakpointId GetBreakpointId() const
    {
        return breakpointId_;
    }

private:
    NO_COPY_SEMANTIC(RemoveBreakpointParams);
    NO_MOVE_SEMANTIC(RemoveBreakpointParams);

    BreakpointId breakpointId_ {};
};

class ResumeParams : public PtBaseParams {
public:
    ResumeParams() = default;
    ~ResumeParams() override = default;

    static std::unique_ptr<ResumeParams> Create(const PtJson &params);

    bool GetTerminateOnResume() const
    {
        return terminateOnResume_.value_or(false);
    }

    bool HasTerminateOnResume() const
    {
        return terminateOnResume_.has_value();
    }

private:
    NO_COPY_SEMANTIC(ResumeParams);
    NO_MOVE_SEMANTIC(ResumeParams);

    std::optional<bool> terminateOnResume_ {};
};

class SetAsyncCallStackDepthParams : public PtBaseParams {
public:
    SetAsyncCallStackDepthParams() = default;
    ~SetAsyncCallStackDepthParams() override = default;

    static std::unique_ptr<SetAsyncCallStackDepthParams> Create(const PtJson &params);

    int32_t GetMaxDepth() const
    {
        return maxDepth_;
    }

private:
    NO_COPY_SEMANTIC(SetAsyncCallStackDepthParams);
    NO_MOVE_SEMANTIC(SetAsyncCallStackDepthParams);

    int32_t maxDepth_ {0};
};

class SetBlackboxPatternsParams : public PtBaseParams {
public:
    SetBlackboxPatternsParams() = default;
    ~SetBlackboxPatternsParams() override = default;
    static std::unique_ptr<SetBlackboxPatternsParams> Create(const PtJson &params);

    std::list<std::string> GetPatterns() const
    {
        return patterns_;
    }

private:
    NO_COPY_SEMANTIC(SetBlackboxPatternsParams);
    NO_MOVE_SEMANTIC(SetBlackboxPatternsParams);

    std::list<std::string> patterns_ {};
};

class SetBreakpointByUrlParams : public PtBaseParams {
public:
    SetBreakpointByUrlParams() = default;
    ~SetBreakpointByUrlParams() override = default;

    static std::unique_ptr<SetBreakpointByUrlParams> Create(const PtJson &params);

    int32_t GetLine() const
    {
        return lineNumber_;
    }

    const std::string &GetUrl() const
    {
        ASSERT(HasUrl());
        return url_.value();
    }

    bool HasUrl() const
    {
        return url_.has_value();
    }

    const std::string &GetUrlRegex() const
    {
        ASSERT(HasUrlRegex());
        return urlRegex_.value();
    }

    bool HasUrlRegex() const
    {
        return urlRegex_.has_value();
    }

    const std::string &GetScriptHash() const
    {
        ASSERT(HasScriptHash());
        return scriptHash_.value();
    }

    bool HasScriptHash() const
    {
        return scriptHash_.has_value();
    }

    int32_t GetColumn() const
    {
        return columnNumber_.value_or(0);
    }

    bool HasColumn() const
    {
        return columnNumber_.has_value();
    }

    const std::string &GetCondition() const
    {
        ASSERT(HasCondition());
        return condition_.value();
    }

    bool HasCondition() const
    {
        return condition_.has_value();
    }

private:
    NO_COPY_SEMANTIC(SetBreakpointByUrlParams);
    NO_MOVE_SEMANTIC(SetBreakpointByUrlParams);

    int32_t lineNumber_ {0};
    std::optional<std::string> url_ {};
    std::optional<std::string> urlRegex_ {};
    std::optional<std::string> scriptHash_ {};
    std::optional<int32_t> columnNumber_ {0};
    std::optional<std::string> condition_ {};
};

enum class PauseOnExceptionsState : uint8_t { NONE, UNCAUGHT, ALL };

class SetPauseOnExceptionsParams : public PtBaseParams {
public:
    SetPauseOnExceptionsParams() = default;
    ~SetPauseOnExceptionsParams() override = default;
    static std::unique_ptr<SetPauseOnExceptionsParams> Create(const PtJson &params);

    PauseOnExceptionsState GetState() const
    {
        return state_;
    }

    bool StoreState(const std::string &state)
    {
        if (state == "none") {
            state_ = PauseOnExceptionsState::NONE;
            return true;
        }
        if (state == "uncaught") {
            state_ = PauseOnExceptionsState::UNCAUGHT;
            return true;
        }
        if (state == "all") {
            state_ = PauseOnExceptionsState::ALL;
            return true;
        }
        return false;
    }

private:
    NO_COPY_SEMANTIC(SetPauseOnExceptionsParams);
    NO_MOVE_SEMANTIC(SetPauseOnExceptionsParams);

    PauseOnExceptionsState state_ {PauseOnExceptionsState::ALL};
};

class StepIntoParams : public PtBaseParams {
public:
    StepIntoParams() = default;
    ~StepIntoParams() override = default;

    static std::unique_ptr<StepIntoParams> Create(const PtJson &params);

    bool GetBreakOnAsyncCall() const
    {
        return breakOnAsyncCall_.value_or(false);
    }

    bool HasBreakOnAsyncCall() const
    {
        return breakOnAsyncCall_.has_value();
    }

    const std::list<std::unique_ptr<LocationRange>> *GetSkipList() const
    {
        if (!skipList_) {
            return nullptr;
        }
        return &(skipList_.value());
    }

    bool HasSkipList() const
    {
        return skipList_.has_value();
    }

private:
    NO_COPY_SEMANTIC(StepIntoParams);
    NO_MOVE_SEMANTIC(StepIntoParams);

    std::optional<bool> breakOnAsyncCall_ {};
    std::optional<std::list<std::unique_ptr<LocationRange>>> skipList_ {};
};

class StepOverParams : public PtBaseParams {
public:
    StepOverParams() = default;
    ~StepOverParams() override = default;

    static std::unique_ptr<StepOverParams> Create(const PtJson &params);

    const std::list<std::unique_ptr<LocationRange>> *GetSkipList() const
    {
        if (!skipList_) {
            return nullptr;
        }
        return &(skipList_.value());
    }

    bool HasSkipList() const
    {
        return skipList_.has_value();
    }

private:
    NO_COPY_SEMANTIC(StepOverParams);
    NO_MOVE_SEMANTIC(StepOverParams);

    std::optional<std::list<std::unique_ptr<LocationRange>>> skipList_ {};
};

class GetPropertiesParams : public PtBaseParams {
public:
    GetPropertiesParams() = default;
    ~GetPropertiesParams() override = default;

    static std::unique_ptr<GetPropertiesParams> Create(const PtJson &params);

    RemoteObjectId GetObjectId() const
    {
        return objectId_;
    }

    bool GetOwnProperties() const
    {
        return ownProperties_.value_or(false);
    }

    bool HasOwnProperties() const
    {
        return ownProperties_.has_value();
    }

    bool GetAccessPropertiesOnly() const
    {
        return accessorPropertiesOnly_.value_or(false);
    }

    bool HasAccessPropertiesOnly() const
    {
        return accessorPropertiesOnly_.has_value();
    }

    bool GetGeneratePreview() const
    {
        return generatePreview_.value_or(false);
    }

    bool HasGeneratePreview() const
    {
        return generatePreview_.has_value();
    }

private:
    NO_COPY_SEMANTIC(GetPropertiesParams);
    NO_MOVE_SEMANTIC(GetPropertiesParams);

    RemoteObjectId objectId_ {};
    std::optional<bool> ownProperties_ {};
    std::optional<bool> accessorPropertiesOnly_ {};
    std::optional<bool> generatePreview_ {};
};

class CallFunctionOnParams : public PtBaseParams {
public:
    CallFunctionOnParams() = default;
    ~CallFunctionOnParams() override = default;

    static std::unique_ptr<CallFunctionOnParams> Create(const PtJson &params);

    const std::string &GetFunctionDeclaration()
    {
        return functionDeclaration_;
    }

    RemoteObjectId GetObjectId() const
    {
        return objectId_.value_or(-1);
    }

    CallFunctionOnParams &SetObjectId(RemoteObjectId objectId)
    {
        objectId_ = objectId;
        return *this;
    }

    bool HasObjectId() const
    {
        return objectId_.has_value();
    }

    const std::vector<std::unique_ptr<CallArgument>> *GetArguments() const
    {
        if (!arguments_) {
            return nullptr;
        }
        return &(arguments_.value());
    }

    bool HasArguments() const
    {
        return arguments_.has_value();
    }

    bool GetSilent() const
    {
        return silent_.value_or(false);
    }

    bool HasSilent() const
    {
        return silent_.has_value();
    }

    bool GetReturnByValue() const
    {
        return returnByValue_.value_or(false);
    }

    bool HasReturnByValue() const
    {
        return returnByValue_.has_value();
    }

    bool GetGeneratePreview() const
    {
        return generatePreview_.value_or(false);
    }

    bool HasGeneratePreview() const
    {
        return generatePreview_.has_value();
    }

    bool GetUserGesture() const
    {
        return userGesture_.value_or(false);
    }

    bool HasUserGesture() const
    {
        return userGesture_.has_value();
    }

    bool GetAwaitPromise() const
    {
        return awaitPromise_.value_or(false);
    }

    bool HasAwaitPromise() const
    {
        return awaitPromise_.has_value();
    }

    ExecutionContextId GetExecutionContextId() const
    {
        return executionContextId_.value_or(-1);
    }

    CallFunctionOnParams &SetExecutionContextId(ExecutionContextId executionContextId)
    {
        executionContextId_ = executionContextId;
        return *this;
    }

    bool HasExecutionContextId() const
    {
        return executionContextId_.has_value();
    }

    const std::string &GetObjectGroup() const
    {
        ASSERT(HasObjectGroup());
        return objectGroup_.value();
    }

    bool HasObjectGroup() const
    {
        return objectGroup_.has_value();
    }

    bool GetThrowOnSideEffect() const
    {
        return throwOnSideEffect_.value_or(false);
    }

    bool HasThrowOnSideEffect() const
    {
        return throwOnSideEffect_.has_value();
    }

private:
    NO_COPY_SEMANTIC(CallFunctionOnParams);
    NO_MOVE_SEMANTIC(CallFunctionOnParams);

    std::string functionDeclaration_ {};
    std::optional<RemoteObjectId> objectId_ {};
    std::optional<std::vector<std::unique_ptr<CallArgument>>> arguments_ {};
    std::optional<bool> silent_ {};
    std::optional<bool> returnByValue_ {};
    std::optional<bool> generatePreview_ {};
    std::optional<bool> userGesture_ {};
    std::optional<bool> awaitPromise_ {};
    std::optional<ExecutionContextId> executionContextId_ {};
    std::optional<std::string> objectGroup_ {};
    std::optional<bool> throwOnSideEffect_ {};
};

#ifdef SUPPORT_PROFILER_CDP
class StartSamplingParams : public PtBaseParams {
public:
    StartSamplingParams() = default;
    ~StartSamplingParams() override = default;

    static std::unique_ptr<StartSamplingParams> Create(const PtJson &params);

    int32_t GetSamplingInterval() const
    {
        return samplingInterval_.value_or(32768);
    }

private:
    NO_COPY_SEMANTIC(StartSamplingParams);
    NO_MOVE_SEMANTIC(StartSamplingParams);

    std::optional<int32_t> samplingInterval_ {32768};
};

class StartTrackingHeapObjectsParams : public PtBaseParams {
public:
    StartTrackingHeapObjectsParams() = default;
    ~StartTrackingHeapObjectsParams() override = default;

    static std::unique_ptr<StartTrackingHeapObjectsParams> Create(const PtJson &params);

    bool GetTrackAllocations() const
    {
        return trackAllocations_.value_or(false);
    }

    bool HasTrackAllocations() const
    {
        return trackAllocations_.has_value();
    }

private:
    NO_COPY_SEMANTIC(StartTrackingHeapObjectsParams);
    NO_MOVE_SEMANTIC(StartTrackingHeapObjectsParams);

    std::optional<bool> trackAllocations_;
};

class StopTrackingHeapObjectsParams : public PtBaseParams {
public:
    StopTrackingHeapObjectsParams() = default;
    ~StopTrackingHeapObjectsParams() override = default;

    static std::unique_ptr<StopTrackingHeapObjectsParams> Create(const PtJson &params);

    bool GetReportProgress() const
    {
        return reportProgress_.value_or(false);
    }

    bool HasReportProgress() const
    {
        return reportProgress_.has_value();
    }

    bool GetTreatGlobalObjectsAsRoots() const
    {
        return treatGlobalObjectsAsRoots_.value_or(false);
    }

    bool HasTreatGlobalObjectsAsRoots() const
    {
        return treatGlobalObjectsAsRoots_.has_value();
    }

    bool GetCaptureNumericValue() const
    {
        return captureNumericValue_.value_or(false);
    }

    bool HasCaptureNumericValue() const
    {
        return captureNumericValue_.has_value();
    }

private:
    NO_COPY_SEMANTIC(StopTrackingHeapObjectsParams);
    NO_MOVE_SEMANTIC(StopTrackingHeapObjectsParams);

    std::optional<bool> reportProgress_ {};
    std::optional<bool> treatGlobalObjectsAsRoots_ {};
    std::optional<bool> captureNumericValue_ {};
};

class AddInspectedHeapObjectParams : public PtBaseParams {
public:
    AddInspectedHeapObjectParams() = default;
    ~AddInspectedHeapObjectParams() override = default;

    static std::unique_ptr<AddInspectedHeapObjectParams> Create(const PtJson &params);

    HeapSnapshotObjectId GetHeapObjectId() const
    {
        return heapObjectId_;
    }

private:
    NO_COPY_SEMANTIC(AddInspectedHeapObjectParams);
    NO_MOVE_SEMANTIC(AddInspectedHeapObjectParams);

    HeapSnapshotObjectId heapObjectId_ {};
};

class GetHeapObjectIdParams : public PtBaseParams {
public:
    GetHeapObjectIdParams() = default;
    ~GetHeapObjectIdParams() override = default;

    static std::unique_ptr<GetHeapObjectIdParams> Create(const PtJson &params);

    RemoteObjectId GetObjectId() const
    {
        return objectId_;
    }

private:
    NO_COPY_SEMANTIC(GetHeapObjectIdParams);
    NO_MOVE_SEMANTIC(GetHeapObjectIdParams);

    RemoteObjectId objectId_ {};
};

class GetObjectByHeapObjectIdParams : public PtBaseParams {
public:
    GetObjectByHeapObjectIdParams() = default;
    ~GetObjectByHeapObjectIdParams() override = default;

    static std::unique_ptr<GetObjectByHeapObjectIdParams> Create(const PtJson &params);

    HeapSnapshotObjectId GetObjectId() const
    {
        return objectId_;
    }

    const std::string &GetObjectGroup() const
    {
        ASSERT(HasObjectGroup());
        return objectGroup_.value();
    }

    bool HasObjectGroup() const
    {
        return objectGroup_.has_value();
    }

private:
    NO_COPY_SEMANTIC(GetObjectByHeapObjectIdParams);
    NO_MOVE_SEMANTIC(GetObjectByHeapObjectIdParams);

    HeapSnapshotObjectId objectId_ {};
    std::optional<std::string> objectGroup_ {};
};

class StartPreciseCoverageParams : public PtBaseParams {
public:
    StartPreciseCoverageParams() = default;
    ~StartPreciseCoverageParams() override = default;

    static std::unique_ptr<StartPreciseCoverageParams> Create(const PtJson &params);

    bool GetCallCount() const
    {
        return callCount_.value_or(false);
    }

    bool HasCallCount() const
    {
        return callCount_.has_value();
    }

    bool GetDetailed() const
    {
        return detailed_.value_or(false);
    }

    bool HasDetailed() const
    {
        return detailed_.has_value();
    }

    bool GetAllowTriggeredUpdates() const
    {
        return allowTriggeredUpdates_.value_or(false);
    }

    bool HasAllowTriggeredUpdates() const
    {
        return allowTriggeredUpdates_.has_value();
    }

private:
    NO_COPY_SEMANTIC(StartPreciseCoverageParams);
    NO_MOVE_SEMANTIC(StartPreciseCoverageParams);

    std::optional<bool> callCount_ {};
    std::optional<bool> detailed_ {};
    std::optional<bool> allowTriggeredUpdates_ {};
};

class SetSamplingIntervalParams : public PtBaseParams {
public:
    SetSamplingIntervalParams() = default;
    ~SetSamplingIntervalParams() override = default;

    static std::unique_ptr<SetSamplingIntervalParams> Create(const PtJson &params);

    int32_t GetInterval() const
    {
        return interval_;
    }

    SetSamplingIntervalParams &SetInterval(int32_t interval)
    {
        interval_ = interval;
        return *this;
    }

private:
    NO_COPY_SEMANTIC(SetSamplingIntervalParams);
    NO_MOVE_SEMANTIC(SetSamplingIntervalParams);

    int32_t interval_ {0};
};
#endif
}  // namespace panda::ecmascript::tooling
#endif