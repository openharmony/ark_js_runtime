/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 #include "../asm_defines.h"

// Not supported on mac x86_64 yet

.global _JSFunctionEntry
_JSFunctionEntry:

.global _OptimizedCallRuntime
_OptimizedCallRuntime:

.global _AsmIntCallRuntime
_AsmIntCallRuntime:

.global _OptimizedCallOptimized
_OptimizedCallOptimized:

.global _PushCallIThisRangeAndDispatch
_PushCallIThisRangeAndDispatch:

.global _PushCallIRangeAndDispatch
_PushCallIRangeAndDispatch:

.global _PushCallArgs3AndDispatch
_PushCallArgs3AndDispatch:

.global _PushCallArgs2AndDispatch
_PushCallArgs2AndDispatch:

.global _PushCallArgs1AndDispatch
_PushCallArgs1AndDispatch:

.global _PushCallArgs0AndDispatch
_PushCallArgs0AndDispatch:

.global _PushCallIThisRangeAndDispatchSlowPath
_PushCallIThisRangeAndDispatchSlowPath:

.global _PushCallIRangeAndDispatchSlowPath
_PushCallIRangeAndDispatchSlowPath:

.global _PushCallArgs3AndDispatchSlowPath
_PushCallArgs3AndDispatchSlowPath:

.global _PushCallArgs2AndDispatchSlowPath
_PushCallArgs2AndDispatchSlowPath:

.global _PushCallArgs1AndDispatchSlowPath
_PushCallArgs1AndDispatchSlowPath:

.global _PushCallArgs0AndDispatchSlowPath
_PushCallArgs0AndDispatchSlowPath:

.global _PushCallIRangeAndDispatchNative
_PushCallIRangeAndDispatchNative:

.global _PushCallArgsAndDispatchNative
_PushCallArgsAndDispatchNative:

.global _ResumeRspAndDispatch
_ResumeRspAndDispatch:

.global _ResumeRspAndReturn
_ResumeRspAndReturn:

.global _OptimizedCallRuntimeWithArgv
_OptimizedCallRuntimeWithArgv:

.global _JSCall
_JSCall:

.global _JSCallWithArgV
_JSCallWithArgV:

.global _JSCallEntry
_JSCallEntry:

.global _CallRuntimeWithArgv
_CallRuntimeWithArgv:

.global _CallRuntime
_CallRuntime:
