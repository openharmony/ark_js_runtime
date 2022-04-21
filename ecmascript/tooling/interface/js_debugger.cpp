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

#include "ecmascript/tooling/interface/js_debugger.h"

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/fast_runtime_stub-inl.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/napi/jsnapi_helper.h"

namespace panda::ecmascript::tooling {
using panda::ecmascript::base::BuiltinsBase;

bool JSDebugger::SetBreakpoint(const JSPtLocation &location, const std::optional<CString> &condition)
{
    JSMethod *method = FindMethod(location);
    if (method == nullptr) {
        LOG(ERROR, DEBUGGER) << "SetBreakpoint: Cannot find JSMethod";
        return false;
    }

    if (location.GetBytecodeOffset() >= method->GetCodeSize()) {
        LOG(ERROR, DEBUGGER) << "SetBreakpoint: Invalid breakpoint location";
        return false;
    }

    if (!breakpoints_.emplace(method, location.GetBytecodeOffset(), condition).second) {
        // also return true
        LOG(WARNING, DEBUGGER) << "SetBreakpoint: Breakpoint already exists";
    }

    return true;
}

bool JSDebugger::RemoveBreakpoint(const JSPtLocation &location)
{
    JSMethod *method = FindMethod(location);
    if (method == nullptr) {
        LOG(ERROR, DEBUGGER) << "RemoveBreakpoint: Cannot find JSMethod";
        return false;
    }

    if (!RemoveBreakpoint(method, location.GetBytecodeOffset())) {
        LOG(ERROR, DEBUGGER) << "RemoveBreakpoint: Breakpoint not found";
        return false;
    }

    return true;
}

void JSDebugger::BytecodePcChanged(JSThread *thread, JSMethod *method, uint32_t bcOffset)
{
    ASSERT(bcOffset < method->GetCodeSize() && "code size of current JSMethod less then bcOffset");

    HandleExceptionThrowEvent(thread, method, bcOffset);

    // Step event is reported before breakpoint, according to the spec.
    HandleStep(method, bcOffset);
    HandleBreakpoint(method, bcOffset);
}

bool JSDebugger::HandleBreakpoint(const JSMethod *method, uint32_t bcOffset)
{
    auto breakpoint = FindBreakpoint(method, bcOffset);
    if (hooks_ == nullptr || !breakpoint.has_value()) {
        return false;
    }

    JSThread *thread = ecmaVm_->GetJSThread();
    // evaluate true, iff the condition exists and is executable
    if (breakpoint.value().HasCondition()) {
        LOG(INFO, DEBUGGER) << "HandleBreakpoint: begin evaluate condition";
        const auto &condition = breakpoint.value().GetCondition();
        auto res = DebuggerApi::ExecuteFromBuffer(const_cast<EcmaVM *>(ecmaVm_),
            condition.data(), condition.size());
        if (thread->HasPendingException()) {
            LOG(ERROR, DEBUGGER) << "HandleBreakpoint: has pending exception";
            thread->ClearException();
            return false;
        }
        bool isMeet = res->ToBoolean(ecmaVm_)->Value();
        if (!isMeet) {
            LOG(ERROR, DEBUGGER) << "HandleBreakpoint: condition not meet";
            return false;
        }
    }

    auto *pf = method->GetPandaFile();
    JSPtLocation location {pf->GetFilename().c_str(), method->GetMethodId(), bcOffset};

    hooks_->Breakpoint(location);
    return true;
}

void JSDebugger::HandleExceptionThrowEvent(const JSThread *thread, const JSMethod *method, uint32_t bcOffset)
{
    if (hooks_ == nullptr || !thread->HasPendingException()) {
        return;
    }

    auto *pf = method->GetPandaFile();
    JSPtLocation throwLocation {pf->GetFilename().c_str(), method->GetMethodId(), bcOffset};

    hooks_->Exception(throwLocation);
}

bool JSDebugger::HandleStep(const JSMethod *method, uint32_t bcOffset)
{
    if (hooks_ == nullptr) {
        return false;
    }

    auto *pf = method->GetPandaFile();
    JSPtLocation location {pf->GetFilename().c_str(), method->GetMethodId(), bcOffset};

    hooks_->SingleStep(location);
    return true;
}

std::optional<JSBreakpoint> JSDebugger::FindBreakpoint(const JSMethod *method, uint32_t bcOffset) const
{
    for (const auto &bp : breakpoints_) {
        if (bp.GetBytecodeOffset() == bcOffset &&
            bp.GetMethod()->GetPandaFile()->GetFilename() == method->GetPandaFile()->GetFilename() &&
            bp.GetMethod()->GetMethodId() == method->GetMethodId()) {
            return bp;
        }
    }
    return {};
}

bool JSDebugger::RemoveBreakpoint(const JSMethod *method, uint32_t bcOffset)
{
    for (auto it = breakpoints_.begin(); it != breakpoints_.end(); ++it) {
        const auto &bp = *it;
        if (bp.GetBytecodeOffset() == bcOffset && bp.GetMethod() == method) {
            it = breakpoints_.erase(it);
            return true;
        }
    }

    return false;
}

JSMethod *JSDebugger::FindMethod(const JSPtLocation &location) const
{
    JSMethod *method = nullptr;
    ::panda::ecmascript::JSPandaFileManager::GetInstance()->EnumerateJSPandaFiles([&method, location](
        const panda::ecmascript::JSPandaFile *jsPandaFile) {
        if (jsPandaFile->GetJSPandaFileDesc() == location.GetPandaFile()) {
            JSMethod *methodsData = jsPandaFile->GetMethods();
            uint32_t numberMethods = jsPandaFile->GetNumMethods();
            for (uint32_t i = 0; i < numberMethods; ++i) {
                if (methodsData[i].GetMethodId() == location.GetMethodId()) {
                    method = methodsData + i;
                    return false;
                }
            }
        }
        return true;
    });
    return method;
}

void JSDebugger::PrepareEvaluateEnv(const EcmaVM *ecmaVm, InterpretedFrameHandler &frameHandler)
{
   /*
    * The abc being executed may have its own lexcial env to resolve symbols, And
    * the DebuggerSetValue or DebuggerGetValue only need be called when symbols
    * not in its own env, usually means we need the env under evaluation context.
    */
    JSTaggedType *sp = const_cast<JSTaggedType *>(ecmaVm->GetJsDebuggerManager()->GetEvaluateCtxFrameSp());
    ASSERT(sp);
    frameHandler = InterpretedFrameHandler {sp};
    ecmaVm->GetJSThread()->SetCurrentLexenv(frameHandler.GetEnv());
}

JSTaggedValue JSDebugger::DebuggerSetValue(EcmaRuntimeCallInfo *argv)
{
    LOG(INFO, DEBUGGER) << "DebuggerSetValue: called";
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    const EcmaVM *ecmaVm = thread->GetEcmaVM();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    InterpretedFrameHandler frameHandler {static_cast<JSTaggedType *>(nullptr)};
    PrepareEvaluateEnv(ecmaVm, frameHandler);

    JSHandle<JSTaggedValue> var = BuiltinsBase::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> newVal = BuiltinsBase::GetCallArg(argv, 1);
    CString varName = ConvertToString(var.GetTaggedValue());
    LOG(INFO, DEBUGGER) << "DebuggerSetValue: name = " << varName;

    JSMethod *method = frameHandler.GetMethod();
    int32_t regIndex = -1;
    bool found = EvaluateLocalValue(method, thread, varName, regIndex);
    if (regIndex != -1) {
        LOG(INFO, DEBUGGER) << "DebuggerSetValue: found regIndex = " << regIndex;
        frameHandler.SetVRegValue(regIndex, newVal.GetTaggedValue());
        return newVal.GetTaggedValue();
    }

    int32_t level = 0;
    uint32_t slot = 0;
    found = DebuggerApi::EvaluateLexicalValue(ecmaVm, varName, level, slot);
    if (found) {
        LOG(INFO, DEBUGGER) << "DebuggerSetValue: found level = " << level;
        DebuggerApi::SetProperties(ecmaVm, level, slot, JSNApiHelper::ToLocal<JSValueRef>(newVal));
        return newVal.GetTaggedValue();
    }

    JSTaggedValue result = SetGlobalValue(ecmaVm, var.GetTaggedValue(), newVal.GetTaggedValue());
    if (!result.IsException()) {
        return newVal.GetTaggedValue();
    }

    CString msg = varName + " is not defined";
    THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.data(), JSTaggedValue::Exception());
}

JSTaggedValue JSDebugger::DebuggerGetValue(EcmaRuntimeCallInfo *argv)
{
    LOG(INFO, DEBUGGER) << "DebuggerGetValue: called";
    ASSERT(argv);
    JSThread *thread = argv->GetThread();
    const EcmaVM *ecmaVm = thread->GetEcmaVM();
    [[maybe_unused]] EcmaHandleScope handleScope(thread);

    InterpretedFrameHandler frameHandler {static_cast<JSTaggedType *>(nullptr)};
    PrepareEvaluateEnv(ecmaVm, frameHandler);

    JSHandle<JSTaggedValue> var = BuiltinsBase::GetCallArg(argv, 0);
    JSHandle<JSTaggedValue> isThrow = BuiltinsBase::GetCallArg(argv, 1);
    CString varName = ConvertToString(var.GetTaggedValue());
    LOG(INFO, DEBUGGER) << "DebuggerGetValue: name = " << varName;

    JSMethod *method = frameHandler.GetMethod();
    int32_t regIndex = -1;
    bool found = EvaluateLocalValue(method, thread, varName, regIndex);
    if (regIndex != -1) {
        LOG(INFO, DEBUGGER) << "DebuggerGetValue: found regIndex = " << regIndex;
        return frameHandler.GetVRegValue(regIndex);
    }

    int32_t level = 0;
    uint32_t slot = 0;
    found = DebuggerApi::EvaluateLexicalValue(ecmaVm, varName, level, slot);
    if (found) {
        LOG(INFO, DEBUGGER) << "DebuggerGetValue: found level = " << level;
        Local<JSValueRef> valRef = DebuggerApi::GetProperties(ecmaVm, level, slot);
        return JSNApiHelper::ToJSTaggedValue(*valRef);
    }

    JSTaggedValue globalVal = GetGlobalValue(ecmaVm, var.GetTaggedValue());
    if (!globalVal.IsException()) {
        return globalVal;
    }

    if (isThrow->ToBoolean()) {
        LOG(ERROR, DEBUGGER) << "DebuggerGetValue: not found and throw exception";
        CString msg = varName + " is not defined";
        THROW_REFERENCE_ERROR_AND_RETURN(thread, msg.data(), JSTaggedValue::Exception());
    }

    // in case of `typeof`, no exception instead of an undefined
    thread->ClearException();
    return JSTaggedValue::Undefined();
}

JSTaggedValue JSDebugger::GetGlobalValue(const EcmaVM *ecmaVm, JSTaggedValue key)
{
    JSTaggedValue globalObj = ecmaVm->GetGlobalEnv()->GetGlobalObject();
    JSThread *thread = ecmaVm->GetJSThread();

    JSTaggedValue globalRec = SlowRuntimeStub::LdGlobalRecord(thread, key);
    if (!globalRec.IsUndefined()) {
        ASSERT(globalRec.IsPropertyBox());
        return PropertyBox::Cast(globalRec.GetTaggedObject())->GetValue();
    }

    JSTaggedValue globalVar = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, key);
    if (!globalVar.IsHole()) {
        return globalVar;
    } else {
        return SlowRuntimeStub::TryLdGlobalByName(thread, globalObj, key);
    }

    return JSTaggedValue::Exception();
}

JSTaggedValue JSDebugger::SetGlobalValue(const EcmaVM *ecmaVm, JSTaggedValue key, JSTaggedValue newVal)
{
    JSTaggedValue globalObj = ecmaVm->GetGlobalEnv()->GetGlobalObject();
    JSThread *thread = ecmaVm->GetJSThread();

    JSTaggedValue globalRec = SlowRuntimeStub::LdGlobalRecord(thread, key);
    if (!globalRec.IsUndefined()) {
        return SlowRuntimeStub::TryUpdateGlobalRecord(thread, key, newVal);
    }

    JSTaggedValue globalVar = FastRuntimeStub::GetGlobalOwnProperty(thread, globalObj, key);
    if (!globalVar.IsHole()) {
        return SlowRuntimeStub::StGlobalVar(thread, key, newVal);
    }

    return JSTaggedValue::Exception();
}

bool JSDebugger::EvaluateLocalValue(JSMethod *method, JSThread *thread, const CString &varName, int32_t &regIndex)
{
    if (method->IsNativeWithCallField()) {
        LOG(ERROR, DEBUGGER) << "EvaluateLocalValue: native frame not support";
        THROW_TYPE_ERROR_AND_RETURN(thread, "native frame not support", false);
    }
    JSPtExtractor *extractor = JSPandaFileManager::GetInstance()->GetJSPtExtractor(method->GetJSPandaFile());
    if (extractor == nullptr) {
        LOG(ERROR, DEBUGGER) << "EvaluateLocalValue: extractor is null";
        THROW_TYPE_ERROR_AND_RETURN(thread, "extractor is null", false);
    }

    auto varTbl = extractor->GetLocalVariableTable(method->GetMethodId());
    auto iter = varTbl.find(varName);
    if (iter == varTbl.end()) {
        return false;
    }
    regIndex = iter->second;
    return true;
}

void JSDebugger::Init()
{
    JSThread *thread = ecmaVm_->GetJSThread();
    [[maybe_unused]] ecmascript::EcmaHandleScope scope(thread);

    ObjectFactory *factory = ecmaVm_->GetFactory();
    constexpr int32_t NUM_ARGS = 2;
    JSHandle<JSTaggedValue> getter(factory->NewFromUtf8("debuggerGetValue"));
    SetGlobalFunction(getter, JSDebugger::DebuggerGetValue, NUM_ARGS);
    JSHandle<JSTaggedValue> setter(factory->NewFromUtf8("debuggerSetValue"));
    SetGlobalFunction(setter, JSDebugger::DebuggerSetValue, NUM_ARGS);
}

void JSDebugger::SetGlobalFunction(const JSHandle<JSTaggedValue> &funcName,
    EcmaEntrypoint nativeFunc, int32_t numArgs) const
{
    ObjectFactory *factory = ecmaVm_->GetFactory();
    JSThread *thread = ecmaVm_->GetJSThread();
    [[maybe_unused]] ecmascript::EcmaHandleScope scope(thread);
    JSHandle<GlobalEnv> env = ecmaVm_->GetGlobalEnv();
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    JSHandle<JSFunction> jsFunc = factory->NewJSFunction(env, reinterpret_cast<void *>(nativeFunc));
    JSFunction::SetFunctionLength(thread, jsFunc, JSTaggedValue(numArgs));
    JSHandle<JSFunctionBase> baseFunc(jsFunc);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    JSFunction::SetFunctionName(thread, baseFunc, funcName, undefined);

    JSHandle<JSFunction> funcHandle(jsFunc);
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(funcHandle), true, false, true);
    JSObject::DefineOwnProperty(thread, globalObject, funcName, desc);
}
}  // namespace panda::tooling::ecmascript
