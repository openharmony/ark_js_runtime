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

#ifndef ECMASCRIPT_TOOLING_BASE_PT_RETURNS_H
#define ECMASCRIPT_TOOLING_BASE_PT_RETURNS_H

#include "ecmascript/tooling/base/pt_types.h"

namespace panda::ecmascript::tooling {
class PtBaseReturns : public PtBaseTypes {
public:
    PtBaseReturns() = default;
    ~PtBaseReturns() override = default;

    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override
    {
        return NewObject(ecmaVm);
    }

private:
    NO_COPY_SEMANTIC(PtBaseReturns);
    NO_MOVE_SEMANTIC(PtBaseReturns);
};

class EnableReturns : public PtBaseReturns {
public:
    explicit EnableReturns(UniqueDebuggerId id) : debuggerId_(id) {}
    ~EnableReturns() override = default;

    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

private:
    EnableReturns() = default;
    NO_COPY_SEMANTIC(EnableReturns);
    NO_MOVE_SEMANTIC(EnableReturns);

    UniqueDebuggerId debuggerId_ {};
};

class SetBreakpointByUrlReturns : public PtBaseReturns {
public:
    explicit SetBreakpointByUrlReturns(const CString &id, CVector<std::unique_ptr<Location>> locations)
        : id_(id), locations_(std::move(locations))
    {}
    ~SetBreakpointByUrlReturns() override = default;

    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

private:
    SetBreakpointByUrlReturns() = default;
    NO_COPY_SEMANTIC(SetBreakpointByUrlReturns);
    NO_MOVE_SEMANTIC(SetBreakpointByUrlReturns);

    CString id_ {};
    CVector<std::unique_ptr<Location>> locations_ {};
};

class EvaluateOnCallFrameReturns : public PtBaseReturns {
public:
    explicit EvaluateOnCallFrameReturns(std::unique_ptr<RemoteObject> result,
        std::optional<std::unique_ptr<ExceptionDetails>> exceptionDetails = std::nullopt)
        : result_(std::move(result)), exceptionDetails_(std::move(exceptionDetails))
    {}
    ~EvaluateOnCallFrameReturns() override = default;
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

private:
    EvaluateOnCallFrameReturns() = default;
    NO_COPY_SEMANTIC(EvaluateOnCallFrameReturns);
    NO_MOVE_SEMANTIC(EvaluateOnCallFrameReturns);

    std::unique_ptr<RemoteObject> result_ {};
    std::optional<std::unique_ptr<ExceptionDetails>> exceptionDetails_ {};
};

class GetPossibleBreakpointsReturns : public PtBaseReturns {
public:
    explicit GetPossibleBreakpointsReturns(CVector<std::unique_ptr<BreakLocation>> locations)
        : locations_(std::move(locations))
    {}
    ~GetPossibleBreakpointsReturns() override = default;

    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

private:
    GetPossibleBreakpointsReturns() = default;
    NO_COPY_SEMANTIC(GetPossibleBreakpointsReturns);
    NO_MOVE_SEMANTIC(GetPossibleBreakpointsReturns);

    CVector<std::unique_ptr<BreakLocation>> locations_ {};
};

class GetScriptSourceReturns : public PtBaseReturns {
public:
    explicit GetScriptSourceReturns(CString scriptSource, std::optional<CString> bytecode = std::nullopt)
        : scriptSource_(std::move(scriptSource)), bytecode_(std::move(bytecode))
    {}
    ~GetScriptSourceReturns() override = default;

    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

private:
    GetScriptSourceReturns() = default;
    NO_COPY_SEMANTIC(GetScriptSourceReturns);
    NO_MOVE_SEMANTIC(GetScriptSourceReturns);

    CString scriptSource_ {};
    std::optional<CString> bytecode_ {};
};

class RestartFrameReturns : public PtBaseReturns {
public:
    explicit RestartFrameReturns(CVector<std::unique_ptr<CallFrame>> callFrames)
        : callFrames_(std::move(callFrames))
    {}
    ~RestartFrameReturns() override = default;
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

private:
    RestartFrameReturns() = default;
    NO_COPY_SEMANTIC(RestartFrameReturns);
    NO_MOVE_SEMANTIC(RestartFrameReturns);

    CVector<std::unique_ptr<CallFrame>> callFrames_ {};
};

class SearchInContentReturns : public PtBaseReturns {
public:
    explicit SearchInContentReturns(CVector<std::unique_ptr<SearchMatch>> result) : result_(std::move(result))
    {}
    ~SearchInContentReturns() override = default;
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

private:
    SearchInContentReturns() = default;
    NO_COPY_SEMANTIC(SearchInContentReturns);
    NO_MOVE_SEMANTIC(SearchInContentReturns);

    CVector<std::unique_ptr<SearchMatch>> result_ {};
};

class SetBreakpointReturns : public PtBaseReturns {
public:
    explicit SetBreakpointReturns(const CString &id, std::unique_ptr<Location> location)
        : breakpointId_(id), location_(std::move(location))
    {}
    ~SetBreakpointReturns() override = default;
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

private:
    SetBreakpointReturns() = default;
    NO_COPY_SEMANTIC(SetBreakpointReturns);
    NO_MOVE_SEMANTIC(SetBreakpointReturns);
    CString breakpointId_ {};
    std::unique_ptr<Location> location_ {};
};

class SetInstrumentationBreakpointReturns : public PtBaseReturns {
public:
    explicit SetInstrumentationBreakpointReturns(const CString &id) : breakpointId_(id)
    {}
    ~SetInstrumentationBreakpointReturns() override = default;
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

private:
    SetInstrumentationBreakpointReturns() = default;
    NO_COPY_SEMANTIC(SetInstrumentationBreakpointReturns);
    NO_MOVE_SEMANTIC(SetInstrumentationBreakpointReturns);

    CString breakpointId_ {};
};

class SetScriptSourceReturns : public PtBaseReturns {
public:
    explicit SetScriptSourceReturns(std::optional<CVector<std::unique_ptr<CallFrame>>> callFrames = std::nullopt,
        std::optional<bool> stackChanged = std::nullopt,
        std::optional<std::unique_ptr<ExceptionDetails>> exceptionDetails = std::nullopt)
        : callFrames_(std::move(callFrames)),
          stackChanged_(stackChanged),
          exceptionDetails_(std::move(exceptionDetails))
    {}
    ~SetScriptSourceReturns() override = default;
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

private:
    SetScriptSourceReturns() = default;
    NO_COPY_SEMANTIC(SetScriptSourceReturns);
    NO_MOVE_SEMANTIC(SetScriptSourceReturns);

    std::optional<CVector<std::unique_ptr<CallFrame>>> callFrames_ {};
    std::optional<bool> stackChanged_ {};
    std::optional<std::unique_ptr<ExceptionDetails>> exceptionDetails_ {};
};

class GetPropertiesReturns : public PtBaseReturns {
public:
    explicit GetPropertiesReturns(CVector<std::unique_ptr<PropertyDescriptor>> descriptor,
        std::optional<CVector<std::unique_ptr<InternalPropertyDescriptor>>> internalDescripties = std::nullopt,
        std::optional<CVector<std::unique_ptr<PrivatePropertyDescriptor>>> privateProperties = std::nullopt,
        std::optional<std::unique_ptr<ExceptionDetails>> exceptionDetails = std::nullopt)
        : result_(std::move(descriptor)),
          internalPropertyDescripties_(std::move(internalDescripties)),
          privateProperties_(std::move(privateProperties)),
          exceptionDetails_(std::move(exceptionDetails))
    {}
    ~GetPropertiesReturns() override = default;
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

private:
    GetPropertiesReturns() = default;
    NO_COPY_SEMANTIC(GetPropertiesReturns);
    NO_MOVE_SEMANTIC(GetPropertiesReturns);

    CVector<std::unique_ptr<PropertyDescriptor>> result_ {};
    std::optional<CVector<std::unique_ptr<InternalPropertyDescriptor>>> internalPropertyDescripties_ {};
    std::optional<CVector<std::unique_ptr<PrivatePropertyDescriptor>>> privateProperties_ {};
    std::optional<std::unique_ptr<ExceptionDetails>> exceptionDetails_ {};
};

class CallFunctionOnReturns : public PtBaseReturns {
public:
    explicit CallFunctionOnReturns(std::unique_ptr<RemoteObject> result,
        std::optional<std::unique_ptr<ExceptionDetails>> exceptionDetails = std::nullopt)
        : result_(std::move(result)),
          exceptionDetails_(std::move(exceptionDetails))
    {}
    ~CallFunctionOnReturns() override = default;
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

private:
    CallFunctionOnReturns() = default;
    NO_COPY_SEMANTIC(CallFunctionOnReturns);
    NO_MOVE_SEMANTIC(CallFunctionOnReturns);

    std::unique_ptr<RemoteObject> result_ {};
    std::optional<std::unique_ptr<ExceptionDetails>> exceptionDetails_ {};
};

class StopSamplingReturns : public PtBaseReturns {
public:
    explicit StopSamplingReturns(std::unique_ptr<SamplingHeapProfile> profile)
        : profile_(std::move(profile))
    {}
    ~StopSamplingReturns() override = default;

    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

private:
    StopSamplingReturns() = default;
    NO_COPY_SEMANTIC(StopSamplingReturns);
    NO_MOVE_SEMANTIC(StopSamplingReturns);

    std::unique_ptr<SamplingHeapProfile> profile_ {};
};

class GetHeapObjectIdReturns : public PtBaseReturns {
public:
    explicit GetHeapObjectIdReturns(HeapSnapshotObjectId heapSnapshotObjectId)
        : heapSnapshotObjectId_(std::move(heapSnapshotObjectId))
    {}
    ~GetHeapObjectIdReturns() override = default;

    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

private:
    GetHeapObjectIdReturns() = default;
    NO_COPY_SEMANTIC(GetHeapObjectIdReturns);
    NO_MOVE_SEMANTIC(GetHeapObjectIdReturns);

    HeapSnapshotObjectId heapSnapshotObjectId_ {};
};

class GetObjectByHeapObjectIdReturns : public PtBaseReturns {
public:
    explicit GetObjectByHeapObjectIdReturns(std::unique_ptr<RemoteObject> remoteObjectResult)
        : remoteObjectResult_(std::move(remoteObjectResult))
    {}
    ~GetObjectByHeapObjectIdReturns() override = default;

    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

private:
    GetObjectByHeapObjectIdReturns() = default;
    NO_COPY_SEMANTIC(GetObjectByHeapObjectIdReturns);
    NO_MOVE_SEMANTIC(GetObjectByHeapObjectIdReturns);

    std::unique_ptr<RemoteObject> remoteObjectResult_ {};
};

class StopReturns : public PtBaseReturns {
public:
    explicit StopReturns(std::unique_ptr<Profile> profile) : profile_(std::move(profile)) {}
    ~StopReturns() override = default;
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

private:
    StopReturns() = default;
    NO_COPY_SEMANTIC(StopReturns);
    NO_MOVE_SEMANTIC(StopReturns);

    std::unique_ptr<Profile> profile_ {};
};

class GetHeapUsageReturns : public PtBaseReturns {
public:
    explicit GetHeapUsageReturns(double usedSize, double totalSize)
        : usedSize_(usedSize), totalSize_(totalSize) {}
    ~GetHeapUsageReturns() override = default;
    Local<ObjectRef> ToObject(const EcmaVM *ecmaVm) override;

private:
    GetHeapUsageReturns() = default;
    NO_COPY_SEMANTIC(GetHeapUsageReturns);
    NO_MOVE_SEMANTIC(GetHeapUsageReturns);

    double usedSize_ {0.0};
    double totalSize_ {0.0};
};
}  // namespace panda::ecmascript::tooling
#endif