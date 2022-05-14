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

// Not supported on windows x64 yet

.global JSFunctionEntry
JSFunctionEntry:

.global OptimizedCallRuntime
OptimizedCallRuntime:

.global AsmIntCallRuntime
AsmIntCallRuntime:

.global OptimizedCallOptimized
OptimizedCallOptimized:

.global PushCallIThisRangeAndDispatch
PushCallIThisRangeAndDispatch:

.global PushCallIRangeAndDispatch
PushCallIRangeAndDispatch:

.global PushCallArgs3AndDispatch
PushCallArgs3AndDispatch:

.global PushCallArgs2AndDispatch
PushCallArgs2AndDispatch:

.global PushCallArgs1AndDispatch
PushCallArgs1AndDispatch:

.global PushCallArgs0AndDispatch
PushCallArgs0AndDispatch:

.global PushCallIThisRangeAndDispatchSlowPath
PushCallIThisRangeAndDispatchSlowPath:

.global PushCallIRangeAndDispatchSlowPath
PushCallIRangeAndDispatchSlowPath:

.global PushCallArgs3AndDispatchSlowPath
PushCallArgs3AndDispatchSlowPath:

.global PushCallArgs2AndDispatchSlowPath
PushCallArgs2AndDispatchSlowPath:

.global PushCallArgs1AndDispatchSlowPath
PushCallArgs1AndDispatchSlowPath:

.global PushCallArgs0AndDispatchSlowPath
PushCallArgs0AndDispatchSlowPath:

.global PushCallIRangeAndDispatchNative
PushCallIRangeAndDispatchNative:

.global PushCallArgsAndDispatchNative
PushCallArgsAndDispatchNative:

.global ResumeRspAndDispatch
ResumeRspAndDispatch:

.global ResumeRspAndReturn
ResumeRspAndReturn:

.global ResumeCaughtFrameAndDispatch
ResumeCaughtFrameAndDispatch:

.global OptimizedCallRuntimeWithArgv
OptimizedCallRuntimeWithArgv:

.global JSCall
JSCall:

.global JSCallWithArgV
JSCallWithArgV:

.global JSCallEntry
JSCallEntry:

.global CallRuntimeWithArgv
CallRuntimeWithArgv:

.global CallRuntime
CallRuntime:

.global AsmInterpreterEntry
AsmInterpreterEntry:

.global JSCallDispatch
JSCallDispatch:
