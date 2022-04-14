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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_DATAVIEW_H
#define ECMASCRIPT_BUILTINS_BUILTINS_DATAVIEW_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_dataview.h"

namespace panda::ecmascript::builtins {
using DataViewType = ecmascript::DataViewType;
class BuiltinsDataView : public base::BuiltinsBase {
public:
    // 24.2.2.1 DataView (buffer [ , byteOffset [ , byteLength ] ] )
    static JSTaggedValue DataViewConstructor(EcmaRuntimeCallInfo *argv);
    // 24.2.4.1 get DataView.prototype.buffer
    static JSTaggedValue GetBuffer(EcmaRuntimeCallInfo *argv);
    // 24.2.4.2 get DataView.prototype.byteLength
    static JSTaggedValue GetByteLength(EcmaRuntimeCallInfo *argv);
    // 24.2.4.3 get DataView.prototype.byteOffset
    static JSTaggedValue GetOffset(EcmaRuntimeCallInfo *argv);
    // 24.2.4.5 DataView.prototype.getFloat32 ( byteOffset [ , littleEndian ] )
    static JSTaggedValue GetFloat32(EcmaRuntimeCallInfo *argv);
    // 24.2.4.6 DataView.prototype.getFloat64 ( byteOffset [ , littleEndian ] )
    static JSTaggedValue GetFloat64(EcmaRuntimeCallInfo *argv);
    // 24.2.4.7 DataView.prototype.getInt8 ( byteOffset )
    static JSTaggedValue GetInt8(EcmaRuntimeCallInfo *argv);
    // 24.2.4.8 DataView.prototype.getInt16 ( byteOffset [ , littleEndian ] )
    static JSTaggedValue GetInt16(EcmaRuntimeCallInfo *argv);
    // 24.2.4.9 DataView.prototype.getInt32 ( byteOffset [ , littleEndian ] )
    static JSTaggedValue GetInt32(EcmaRuntimeCallInfo *argv);
    // 24.2.4.10 DataView.prototype.getUint8 ( byteOffset )
    static JSTaggedValue GetUint8(EcmaRuntimeCallInfo *argv);
    // 24.2.4.11 DataView.prototype.getUint16 ( byteOffset [ , littleEndian ] )
    static JSTaggedValue GetUint16(EcmaRuntimeCallInfo *argv);
    // 24.2.4.12 DataView.prototype.getUint32 ( byteOffset [ , littleEndian ] )
    static JSTaggedValue GetUint32(EcmaRuntimeCallInfo *argv);
    // 25.3.4.5 DataView.prototype.getBigInt64 ( byteOffset [ , littleEndian ] )
    static JSTaggedValue GetBigInt64(EcmaRuntimeCallInfo *argv);
    // 25.3.4.6 DataView.prototype.getBigUint64 ( byteOffset [ , littleEndian ] )
    static JSTaggedValue GetBigUint64 (EcmaRuntimeCallInfo *argv);
    // 24.2.4.13 DataView.prototype.setFloat32 ( byteOffset, value [ , littleEndian ] )
    static JSTaggedValue SetFloat32(EcmaRuntimeCallInfo *argv);
    // 24.2.4.14 DataView.prototype.setFloat64 ( byteOffset, value [ , littleEndian ] )
    static JSTaggedValue SetFloat64(EcmaRuntimeCallInfo *argv);
    // 24.2.4.15 DataView.prototype.setInt8 ( byteOffset, value )
    static JSTaggedValue SetInt8(EcmaRuntimeCallInfo *argv);
    // 24.2.4.16 DataView.prototype.setInt16 ( byteOffset, value [ , littleEndian ] )
    static JSTaggedValue SetInt16(EcmaRuntimeCallInfo *argv);
    // 24.2.4.17 DataView.prototype.setInt32 ( byteOffset, value [ , littleEndian ] )
    static JSTaggedValue SetInt32(EcmaRuntimeCallInfo *argv);
    // 24.2.4.18 DataView.prototype.setUint8 ( byteOffset, value )
    static JSTaggedValue SetUint8(EcmaRuntimeCallInfo *argv);
    // 24.2.4.19 DataView.prototype.setUint16( byteOffset, value [ , littleEndian ] )
    static JSTaggedValue SetUint16(EcmaRuntimeCallInfo *argv);
    // 24.2.4.20 DataView.prototype.setUint32 ( byteOffset, value [ , littleEndian ] )
    static JSTaggedValue SetUint32(EcmaRuntimeCallInfo *argv);
    // 25.3.4.15 DataView.prototype.setBigInt64 ( byteOffset, value [ , littleEndian ] )
    static JSTaggedValue SetBigInt64(EcmaRuntimeCallInfo *argv);
    // 25.3.4.16 DataView.prototype.setBigUint64 ( byteOffset, value [ , littleEndian ] )
    static JSTaggedValue SetBigUint64(EcmaRuntimeCallInfo *argv);

private:
    // 24.2.1.1 GetViewValue ( view, requestIndex, isLittleEndian, type )
    static JSTaggedValue GetViewValue(JSThread *thread, const JSHandle<JSTaggedValue> &view,
                                      const JSHandle<JSTaggedValue> &requestIndex, JSTaggedValue littleEndian,
                                      DataViewType type);
    static JSTaggedValue SetViewValue(JSThread *thread, const JSHandle<JSTaggedValue> &view,
                                      const JSHandle<JSTaggedValue> &requestIndex, JSTaggedValue littleEndian,
                                      DataViewType type, const JSHandle<JSTaggedValue> &value);

    static JSTaggedValue GetTypedValue(EcmaRuntimeCallInfo *argv, DataViewType type);
    static JSTaggedValue SetTypedValue(EcmaRuntimeCallInfo *argv, DataViewType type);
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_DATAVIEW_H
