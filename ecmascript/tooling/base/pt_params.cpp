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

#include "ecmascript/tooling/base/pt_params.h"

namespace panda::ecmascript::tooling {
std::unique_ptr<EnableParams> EnableParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<EnableParams>();
    std::string error;
    Result ret;

    double maxScriptsCacheSize;
    ret = params.GetDouble("maxScriptsCacheSize", &maxScriptsCacheSize);
    if (ret == Result::SUCCESS) {
        paramsObject->maxScriptsCacheSize_ = maxScriptsCacheSize;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'maxScriptsCacheSize';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "EnableParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<EvaluateOnCallFrameParams> EvaluateOnCallFrameParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<EvaluateOnCallFrameParams>();
    std::string error;
    Result ret;

    std::string callFrameId;
    ret = params.GetString("callFrameId", &callFrameId);
    if (ret == Result::SUCCESS) {
        paramsObject->callFrameId_ = std::stoi(callFrameId);
    } else {
        error += "Unknown 'callFrameId';";
    }
    std::string expression;
    ret = params.GetString("expression", &expression);
    if (ret == Result::SUCCESS) {
        paramsObject->expression_ = std::move(expression);
    } else {
        error += "Unknown 'expression';";
    }
    std::string objectGroup;
    ret = params.GetString("objectGroup", &objectGroup);
    if (ret == Result::SUCCESS) {
        paramsObject->objectGroup_ = std::move(objectGroup);
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'objectGroup';";
    }
    bool includeCommandLineAPI;
    ret = params.GetBool("includeCommandLineAPI", &includeCommandLineAPI);
    if (ret == Result::SUCCESS) {
        paramsObject->includeCommandLineAPI_ = includeCommandLineAPI;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'includeCommandLineAPI';";
    }
    bool silent;
    ret = params.GetBool("silent", &silent);
    if (ret == Result::SUCCESS) {
        paramsObject->silent_ = silent;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'silent';";
    }
    bool returnByValue;
    ret = params.GetBool("returnByValue", &returnByValue);
    if (ret == Result::SUCCESS) {
        paramsObject->returnByValue_ = returnByValue;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'returnByValue';";
    }
    bool generatePreview;
    ret = params.GetBool("generatePreview", &generatePreview);
    if (ret == Result::SUCCESS) {
        paramsObject->generatePreview_ = generatePreview;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'generatePreview';";
    }
    bool throwOnSideEffect;
    ret = params.GetBool("throwOnSideEffect", &throwOnSideEffect);
    if (ret == Result::SUCCESS) {
        paramsObject->throwOnSideEffect_ = throwOnSideEffect;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'throwOnSideEffect';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "EvaluateOnCallFrameParams::Create " << error;
        return nullptr;
    }
    return paramsObject;
}

std::unique_ptr<GetPossibleBreakpointsParams> GetPossibleBreakpointsParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<GetPossibleBreakpointsParams>();
    std::string error;
    Result ret;

    std::unique_ptr<PtJson> start;
    ret = params.GetObject("start", &start);
    if (ret == Result::SUCCESS) {
        std::unique_ptr<Location> location = Location::Create(*start);
        if (location == nullptr) {
            error += "Unknown 'start';";
        } else {
            paramsObject->start_ = std::move(location);
        }
    } else {
        error += "Unknown 'start';";
    }
    std::unique_ptr<PtJson> end;
    ret = params.GetObject("start", &end);
    if (ret == Result::SUCCESS) {
        std::unique_ptr<Location> location = Location::Create(*end);
        if (location == nullptr) {
            error += "Unknown 'end';";
        } else {
            paramsObject->end_ = std::move(location);
        }
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'end';";
    }
    bool restrictToFunction;
    ret = params.GetBool("restrictToFunction", &restrictToFunction);
    if (ret == Result::SUCCESS) {
        paramsObject->restrictToFunction_ = restrictToFunction;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'restrictToFunction';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "GetPossibleBreakpointsParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<GetScriptSourceParams> GetScriptSourceParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<GetScriptSourceParams>();
    std::string error;
    Result ret;

    std::string scriptId;
    ret = params.GetString("scriptId", &scriptId);
    if (ret == Result::SUCCESS) {
        paramsObject->scriptId_ = std::stoi(scriptId);
    } else {
        error += "Unknown 'scriptId';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "GetScriptSourceParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<RemoveBreakpointParams> RemoveBreakpointParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<RemoveBreakpointParams>();
    std::string error;
    Result ret;

    std::string breakpointId;
    ret = params.GetString("breakpointId", &breakpointId);
    if (ret == Result::SUCCESS) {
        paramsObject->breakpointId_ = std::move(breakpointId);
    } else {
        error += "Unknown 'breakpointId';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "RemoveBreakpointParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<ResumeParams> ResumeParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<ResumeParams>();
    std::string error;
    Result ret;

    bool terminateOnResume;
    ret = params.GetBool("terminateOnResume", &terminateOnResume);
    if (ret == Result::SUCCESS) {
        paramsObject->terminateOnResume_ = terminateOnResume;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'terminateOnResume';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "ResumeParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<SetAsyncCallStackDepthParams> SetAsyncCallStackDepthParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<SetAsyncCallStackDepthParams>();
    std::string error;
    Result ret;

    int32_t maxDepth;
    ret = params.GetInt("maxDepth", &maxDepth);
    if (ret == Result::SUCCESS) {
        paramsObject->maxDepth_ = maxDepth;
    } else {
        error += "Unknown 'maxDepth';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "SetAsyncCallStackDepthParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<SetBlackboxPatternsParams> SetBlackboxPatternsParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<SetBlackboxPatternsParams>();
    std::string error;
    Result ret;

    std::unique_ptr<PtJson> patterns;
    ret = params.GetArray("patterns", &patterns);
    if (ret == Result::SUCCESS) {
        int32_t len = patterns->GetSize();
        for (int32_t i = 0; i < len; ++i) {
            std::unique_ptr<PtJson> item = patterns->Get(i);
            if (item->IsString()) {
                paramsObject->patterns_.emplace_back(item->GetString());
            } else {
                error += "'patterns' items should be a String;";
            }
        }
    } else {
        error += "Unknown 'patterns';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "SetBlackboxPatternsParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<SetBreakpointByUrlParams> SetBreakpointByUrlParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<SetBreakpointByUrlParams>();
    std::string error;
    Result ret;

    int32_t lineNumber;
    ret = params.GetInt("lineNumber", &lineNumber);
    if (ret == Result::SUCCESS) {
        paramsObject->lineNumber_ = lineNumber;
    } else {
        error += "Unknown 'lineNumber';";
    }
    std::string url;
    ret = params.GetString("url", &url);
    if (ret == Result::SUCCESS) {
        paramsObject->url_ = std::move(url);
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'url';";
    }
    std::string urlRegex;
    ret = params.GetString("urlRegex", &urlRegex);
    if (ret == Result::SUCCESS) {
        paramsObject->urlRegex_ = std::move(urlRegex);
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'urlRegex';";
    }
    std::string scriptHash;
    ret = params.GetString("scriptHash", &scriptHash);
    if (ret == Result::SUCCESS) {
        paramsObject->scriptHash_ = std::move(scriptHash);
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'scriptHash';";
    }
    int32_t columnNumber;
    ret = params.GetInt("columnNumber", &columnNumber);
    if (ret == Result::SUCCESS) {
        paramsObject->columnNumber_ = columnNumber;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'columnNumber';";
    }
    std::string condition;
    ret = params.GetString("condition", &condition);
    if (ret == Result::SUCCESS) {
        paramsObject->condition_ = std::move(condition);
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'condition';";
    }
    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "SetBreakpointByUrlParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<SetPauseOnExceptionsParams> SetPauseOnExceptionsParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<SetPauseOnExceptionsParams>();
    std::string error;
    Result ret;

    std::string state;
    ret = params.GetString("state", &state);
    if (ret == Result::SUCCESS) {
        paramsObject->StoreState(state);
    } else {
        error += "Unknown 'state';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "SetPauseOnExceptionsParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<StepIntoParams> StepIntoParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<StepIntoParams>();
    std::string error;
    Result ret;

    bool breakOnAsyncCall;
    ret = params.GetBool("breakOnAsyncCall", &breakOnAsyncCall);
    if (ret == Result::SUCCESS) {
        paramsObject->breakOnAsyncCall_ = breakOnAsyncCall;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'breakOnAsyncCall';";
    }
    std::unique_ptr<PtJson> skipList;
    ret = params.GetArray("skipList", &skipList);
    if (ret == Result::SUCCESS) {
        int32_t len = skipList->GetSize();
        for (int32_t i = 0; i < len; ++i) {
            std::unique_ptr<LocationRange> obj = LocationRange::Create(*skipList->Get(i));
            if (obj == nullptr) {
                error += "'skipList' items LocationRange is invalid;";
            } else {
                paramsObject->skipList_->emplace_back(std::move(obj));
            }
        }
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'skipList';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "StepIntoParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<StepOverParams> StepOverParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<StepOverParams>();
    std::string error;
    Result ret;

    std::unique_ptr<PtJson> skipList;
    ret = params.GetArray("skipList", &skipList);
    if (ret == Result::SUCCESS) {
        int32_t len = skipList->GetSize();
        for (int32_t i = 0; i < len; ++i) {
            std::unique_ptr<LocationRange> obj = LocationRange::Create(*skipList->Get(i));
            if (obj == nullptr) {
                error += "'skipList' items LocationRange is invalid;";
            } else {
                paramsObject->skipList_->emplace_back(std::move(obj));
            }
        }
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'skipList';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "StepOverParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<GetPropertiesParams> GetPropertiesParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<GetPropertiesParams>();
    std::string error;
    Result ret;

    std::string objectId;
    ret = params.GetString("objectId", &objectId);
    if (ret == Result::SUCCESS) {
        paramsObject->objectId_ = std::stoi(objectId);
    } else {
        error += "Unknown 'objectId';";
    }
    bool ownProperties;
    ret = params.GetBool("ownProperties", &ownProperties);
    if (ret == Result::SUCCESS) {
        paramsObject->ownProperties_ = ownProperties;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'ownProperties';";
    }
    bool accessorPropertiesOnly;
    ret = params.GetBool("accessorPropertiesOnly", &accessorPropertiesOnly);
    if (ret == Result::SUCCESS) {
        paramsObject->accessorPropertiesOnly_ = accessorPropertiesOnly;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'accessorPropertiesOnly';";
    }
    bool generatePreview;
    ret = params.GetBool("generatePreview", &generatePreview);
    if (ret == Result::SUCCESS) {
        paramsObject->generatePreview_ = generatePreview;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'generatePreview';";
    }
    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "GetPropertiesParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<CallFunctionOnParams> CallFunctionOnParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<CallFunctionOnParams>();
    std::string error;
    Result ret;

    // paramsObject->functionDeclaration_
    std::string functionDeclaration;
    ret = params.GetString("functionDeclaration", &functionDeclaration);
    if (ret == Result::SUCCESS) {
        paramsObject->functionDeclaration_ = std::move(functionDeclaration);
    } else {
        error += "Unknown 'functionDeclaration';";
    }
    // paramsObject->objectId_
    std::string objectId;
    ret = params.GetString("objectId", &objectId);
    if (ret == Result::SUCCESS) {
        paramsObject->objectId_ = std::stoi(objectId);
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'objectId';";
    }
    // paramsObject->arguments_
    std::unique_ptr<PtJson> arguments;
    ret = params.GetArray("arguments", &arguments);
    if (ret == Result::SUCCESS) {
        int32_t len = arguments->GetSize();
        for (int32_t i = 0; i < len; ++i) {
            std::unique_ptr<CallArgument> obj = CallArgument::Create(*arguments->Get(i));
            if (obj == nullptr) {
                error += "'arguments' items CallArgument is invaild;";
            } else {
                paramsObject->arguments_->emplace_back(std::move(obj));
            }
        }
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'arguments';";
    }
    // paramsObject->silent_
    bool silent;
    ret = params.GetBool("silent", &silent);
    if (ret == Result::SUCCESS) {
        paramsObject->silent_ = silent;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'silent';";
    }
    // paramsObject->returnByValue_
    bool returnByValue;
    ret = params.GetBool("returnByValue", &returnByValue);
    if (ret == Result::SUCCESS) {
        paramsObject->returnByValue_ = returnByValue;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'returnByValue';";
    }
    // paramsObject->generatePreview_
    bool generatePreview;
    ret = params.GetBool("generatePreview", &generatePreview);
    if (ret == Result::SUCCESS) {
        paramsObject->generatePreview_ = generatePreview;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'generatePreview';";
    }
    // paramsObject->userGesture_
    bool userGesture;
    ret = params.GetBool("userGesture", &userGesture);
    if (ret == Result::SUCCESS) {
        paramsObject->userGesture_ = userGesture;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'userGesture';";
    }
    // paramsObject->awaitPromise_
    bool awaitPromise;
    ret = params.GetBool("awaitPromise", &awaitPromise);
    if (ret == Result::SUCCESS) {
        paramsObject->awaitPromise_ = awaitPromise;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'awaitPromise';";
    }
    // paramsObject->executionContextId_
    int32_t executionContextId;
    ret = params.GetInt("executionContextId", &executionContextId);
    if (ret == Result::SUCCESS) {
        paramsObject->executionContextId_ = executionContextId;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'executionContextId';";
    }
    // paramsObject->objectGroup_
    std::string objectGroup;
    ret = params.GetString("objectGroup", &objectGroup);
    if (ret == Result::SUCCESS) {
        paramsObject->objectGroup_ = std::move(objectGroup);
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'objectGroup';";
    }
    // paramsObject->throwOnSideEffect_
    bool throwOnSideEffect;
    ret = params.GetBool("throwOnSideEffect", &throwOnSideEffect);
    if (ret == Result::SUCCESS) {
        paramsObject->throwOnSideEffect_ = throwOnSideEffect;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'throwOnSideEffect';";
    }

    // Check whether the error is empty.
    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "CallFunctionOnParams::Create " << error;
        return nullptr;
    }

    return paramsObject;
}

std::unique_ptr<StartSamplingParams> StartSamplingParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<StartSamplingParams>();
    std::string error;
    Result ret;

    int32_t samplingInterval;
    ret = params.GetInt("samplingInterval", &samplingInterval);
    if (ret == Result::SUCCESS) {
        paramsObject->samplingInterval_ = samplingInterval;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'samplingInterval';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "StartSamplingParams::Create " << error;
        return nullptr;
    }
    return paramsObject;
}

std::unique_ptr<StartTrackingHeapObjectsParams> StartTrackingHeapObjectsParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<StartTrackingHeapObjectsParams>();
    std::string error;
    Result ret;

    bool trackAllocations;
    ret = params.GetBool("trackAllocations", &trackAllocations);
    if (ret == Result::SUCCESS) {
        paramsObject->trackAllocations_ = trackAllocations;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'trackAllocations';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "StartTrackingHeapObjectsParams::Create " << error;
        return nullptr;
    }
    return paramsObject;
}

std::unique_ptr<StopTrackingHeapObjectsParams> StopTrackingHeapObjectsParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<StopTrackingHeapObjectsParams>();
    std::string error;
    Result ret;

    bool reportProgress;
    ret = params.GetBool("reportProgress", &reportProgress);
    if (ret == Result::SUCCESS) {
        paramsObject->reportProgress_ = reportProgress;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'reportProgress';";
    }

    bool treatGlobalObjectsAsRoots;
    ret = params.GetBool("treatGlobalObjectsAsRoots", &treatGlobalObjectsAsRoots);
    if (ret == Result::SUCCESS) {
        paramsObject->treatGlobalObjectsAsRoots_ = treatGlobalObjectsAsRoots;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'treatGlobalObjectsAsRoots';";
    }

    bool captureNumericValue;
    ret = params.GetBool("captureNumericValue", &captureNumericValue);
    if (ret == Result::SUCCESS) {
        paramsObject->captureNumericValue_ = captureNumericValue;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'captureNumericValue';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "StopTrackingHeapObjectsParams::Create " << error;
        return nullptr;
    }
    return paramsObject;
}

std::unique_ptr<AddInspectedHeapObjectParams> AddInspectedHeapObjectParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<AddInspectedHeapObjectParams>();
    std::string error;
    Result ret;

    std::string heapObjectId;
    ret = params.GetString("heapObjectId", &heapObjectId);
    if (ret == Result::SUCCESS) {
        paramsObject->heapObjectId_ = std::stoi(heapObjectId);
    } else {
        error += "Unknown 'heapObjectId';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "AddInspectedHeapObjectParams::Create " << error;
        return nullptr;
    }
    return paramsObject;
}

std::unique_ptr<GetHeapObjectIdParams> GetHeapObjectIdParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<GetHeapObjectIdParams>();
    std::string error;
    Result ret;

    std::string objectId;
    ret = params.GetString("objectId", &objectId);
    if (ret == Result::SUCCESS) {
        paramsObject->objectId_ = std::stoi(objectId);
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'objectId';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "GetHeapObjectIdParams::Create " << error;
        return nullptr;
    }
    return paramsObject;
}

std::unique_ptr<GetObjectByHeapObjectIdParams> GetObjectByHeapObjectIdParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<GetObjectByHeapObjectIdParams>();
    std::string error;
    Result ret;

    std::string objectId;
    ret = params.GetString("objectId", &objectId);
    if (ret == Result::SUCCESS) {
        paramsObject->objectId_ = std::stoi(objectId);
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'objectId';";
    }

    std::string objectGroup;
    ret = params.GetString("objectGroup", &objectGroup);
    if (ret == Result::SUCCESS) {
        paramsObject->objectGroup_ = std::move(objectGroup);
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'objectGroup';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "GetObjectByHeapObjectIdParams::Create " << error;
        return nullptr;
    }
    return paramsObject;
}

std::unique_ptr<StartPreciseCoverageParams> StartPreciseCoverageParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<StartPreciseCoverageParams>();
    std::string error;
    Result ret;

    bool callCount;
    ret = params.GetBool("callCount", &callCount);
    if (ret == Result::SUCCESS) {
        paramsObject->callCount_ = callCount;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'callCount';";
    }

    bool detailed;
    ret = params.GetBool("detailed", &detailed);
    if (ret == Result::SUCCESS) {
        paramsObject->detailed_ = detailed;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'detailed';";
    }

    bool allowTriggeredUpdates;
    ret = params.GetBool("allowTriggeredUpdates", &allowTriggeredUpdates);
    if (ret == Result::SUCCESS) {
        paramsObject->allowTriggeredUpdates_ = allowTriggeredUpdates;
    } else if (ret == Result::TYPE_ERROR) {  // optional value
        error += "Unknown 'allowTriggeredUpdates';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "StartPreciseCoverageParams::Create " << error;
        return nullptr;
    }
    return paramsObject;
}

std::unique_ptr<SetSamplingIntervalParams> SetSamplingIntervalParams::Create(const PtJson &params)
{
    auto paramsObject = std::make_unique<SetSamplingIntervalParams>();
    std::string error;
    Result ret;

    int32_t interval;
    ret = params.GetInt("interval", &interval);
    if (ret == Result::SUCCESS) {
        paramsObject->interval_ = interval;
    } else {
        error += "Unknown 'interval';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "SetSamplingIntervalParams::Create " << error;
        return nullptr;
    }
    return paramsObject;
}

std::unique_ptr<RecordClockSyncMarkerParams> RecordClockSyncMarkerParams::Create(const PtJson &params)
{
    std::string error;
    auto recordClockSyncMarkerParams = std::make_unique<RecordClockSyncMarkerParams>();
    Result ret;
    
    std::string syncId;
    ret = params.GetString("syncId", &syncId);
    if (ret == Result::SUCCESS) {
        recordClockSyncMarkerParams->syncId_ = syncId;
    } else {
        error += "Unknown 'syncId';";
    }
    
    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "RecordClockSyncMarkerParams::Create " << error;
        return nullptr;
    }

    return recordClockSyncMarkerParams;
}

std::unique_ptr<RequestMemoryDumpParams> RequestMemoryDumpParams::Create(const PtJson &params)
{
    std::string error;
    auto requestMemoryDumpParams = std::make_unique<RequestMemoryDumpParams>();
    Result ret;
    
    bool deterministic;
    ret = params.GetBool("deterministic", &deterministic);
    if (ret == Result::SUCCESS) {
        requestMemoryDumpParams->deterministic_ = deterministic;
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'deterministic';";
    }

    std::string levelOfDetail;
    ret = params.GetString("levelOfDetail", &levelOfDetail);
    if (ret == Result::SUCCESS) {
        if (MemoryDumpLevelOfDetailValues::Valid(levelOfDetail)) {
            requestMemoryDumpParams->levelOfDetail_ = std::move(levelOfDetail);
        } else {
            error += "'levelOfDetail' is invalid;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'levelOfDetail';";
    }
    
    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "RequestMemoryDumpParams::Create " << error;
        return nullptr;
    }

    return requestMemoryDumpParams;
}

std::unique_ptr<StartParams> StartParams::Create(const PtJson &params)
{
    std::string error;
    auto startParams = std::make_unique<StartParams>();
    Result ret;
    
    std::string categories;
    ret = params.GetString("categories", &categories);
    if (ret == Result::SUCCESS) {
        startParams->categories_ = std::move(categories);
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'categories';";
    }

    std::string options;
    ret = params.GetString("options", &options);
    if (ret == Result::SUCCESS) {
        startParams->options_ = std::move(options);
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'options';";
    }

    int32_t bufferUsageReportingInterval;
    ret = params.GetInt("bufferUsageReportingInterval", &bufferUsageReportingInterval);
    if (ret == Result::SUCCESS) {
        startParams->bufferUsageReportingInterval_ = bufferUsageReportingInterval;
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'bufferUsageReportingInterval';";
    }

    std::string transferMode;
    ret = params.GetString("transferMode", &transferMode);
    if (ret == Result::SUCCESS) {
        if (StartParams::TransferModeValues::Valid(transferMode)) {
            startParams->transferMode_ = std::move(transferMode);
        } else {
            error += "'transferMode' is invalid;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'transferMode';";
    }

    std::string streamFormat;
    ret = params.GetString("streamFormat", &streamFormat);
    if (ret == Result::SUCCESS) {
        if (StreamFormatValues::Valid(streamFormat)) {
            startParams->streamFormat_ = std::move(streamFormat);
        } else {
            error += "'streamFormat' is invalid;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'streamFormat';";
    }

    std::string streamCompression;
    ret = params.GetString("streamCompression", &streamCompression);
    if (ret == Result::SUCCESS) {
        if (StreamCompressionValues::Valid(streamCompression)) {
            startParams->streamCompression_ = std::move(streamCompression);
        } else {
            error += "'streamCompression' is invalid;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'streamCompression';";
    }

    std::unique_ptr<PtJson> traceConfig;
    ret = params.GetObject("traceConfig", &traceConfig);
    if (ret == Result::SUCCESS) {
        std::unique_ptr<TraceConfig> pTraceConfig = TraceConfig::Create(*traceConfig);
        if (pTraceConfig == nullptr) {
            error += "'traceConfig' format invalid;";
        } else {
            startParams->traceConfig_ = std::move(pTraceConfig);
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'traceConfig';";
    }

    std::string perfettoConfig;
    ret = params.GetString("perfettoConfig", &perfettoConfig);
    if (ret == Result::SUCCESS) {
        startParams->perfettoConfig_ = std::move(perfettoConfig);
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'perfettoConfig';";
    }

    std::string tracingBackend;
    ret = params.GetString("tracingBackend", &tracingBackend);
    if (ret == Result::SUCCESS) {
        if (TracingBackendValues::Valid(tracingBackend)) {
            startParams->tracingBackend_ = std::move(tracingBackend);
        } else {
            error += "'tracingBackend' is invalid;";
        }
    } else if (ret == Result::TYPE_ERROR) {
        error += "Unknown 'tracingBackend';";
    }

    if (!error.empty()) {
        LOG_DEBUGGER(ERROR) << "StartParams::Create " << error;
        return nullptr;
    }

    return startParams;
}
}  // namespace panda::ecmascript::tooling
