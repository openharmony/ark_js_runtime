/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_SHAREDARRAYBUFFER_H
#define ECMASCRIPT_BUILTINS_BUILTINS_SHAREDARRAYBUFFER_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/js_dataview.h"

namespace panda::ecmascript::builtins {
class BuiltinsSharedArrayBuffer : public base::BuiltinsBase {
public:
    // 25.2.2.1 SharedArrayBuffer ( [ length ] )
    static JSTaggedValue SharedArrayBufferConstructor(EcmaRuntimeCallInfo *argv);
    // 25.2.1.2 IsSharedArrayBuffer ( obj )
    static JSTaggedValue IsSharedArrayBuffer(EcmaRuntimeCallInfo *argv);
    // 24.2.3.2 get SharedArrayBuffer [ @@species ]
    static JSTaggedValue Species(EcmaRuntimeCallInfo *argv);
    // 25.2.4.1 get SharedArrayBuffer.prototype.byteLength
    static JSTaggedValue GetByteLength(EcmaRuntimeCallInfo *argv);
    // 25.2.4.3 SharedArrayBuffer.prototype.slice ( start, end )
    static JSTaggedValue Slice(EcmaRuntimeCallInfo *argv);

    static bool IsShared(JSTaggedValue arrayBuffer);
private:
    // 25.2.1.1 AllocateSharedArrayBuffer ( constructor, byteLength )
    static JSTaggedValue AllocateSharedArrayBuffer(JSThread *thread, const JSHandle<JSTaggedValue> &newTarget,
                                             double byteLength);
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_SHAREDARRAYBUFFER_H