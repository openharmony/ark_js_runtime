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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_ARRAYBUFFER_H
#define ECMASCRIPT_BUILTINS_BUILTINS_ARRAYBUFFER_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/js_dataview.h"

namespace panda::ecmascript::builtins {
static constexpr double NUMBER_HALF = 0.5;
static constexpr uint32_t BITS_EIGHT = 8;
static constexpr uint32_t BITS_TWENTY_FOUR = 24;
static constexpr uint32_t BITS_FORTY = 40;
static constexpr uint32_t BITS_FIFTY_SIX = 56;
using DataViewType = ecmascript::DataViewType;
union UnionType32 {
    uint32_t uValue;
    float value;
};
union UnionType64 {
    uint64_t uValue;
    double value;
};
class BuiltinsArrayBuffer : public base::BuiltinsBase {
public:
    enum NumberSize : uint8_t { UINT16 = 2, INT16 = 2, UINT32 = 4, INT32 = 4, FLOAT32 = 4, FLOAT64 = 8 };

    // 24.1.2.1 ArrayBuffer(length)
    static JSTaggedValue ArrayBufferConstructor(EcmaRuntimeCallInfo *argv);
    // 24.1.3.1 ArrayBuffer.isView(arg)
    static JSTaggedValue IsView(EcmaRuntimeCallInfo *argv);
    // 24.1.3.3 get ArrayBuffer[@@species]
    static JSTaggedValue Species(EcmaRuntimeCallInfo *argv);
    // 24.1.4.1 get ArrayBuffer.prototype.byteLength
    static JSTaggedValue GetByteLength(EcmaRuntimeCallInfo *argv);
    // 24.1.4.3 ArrayBuffer.prototype.slice()
    static JSTaggedValue Slice(EcmaRuntimeCallInfo *argv);
    // 24.1.1.2 IsDetachedBuffer(arrayBuffer)
    static bool IsDetachedBuffer(JSTaggedValue arrayBuffer);
    // 24.1.1.5 GetValueFromBuffer ( arrayBuffer, byteIndex, type, isLittleEndian )
    static JSTaggedValue GetValueFromBuffer(JSTaggedValue arrBuf, uint32_t byteIndex, DataViewType type,
                                            bool littleEndian);
    // 24.1.1.6 SetValueInBuffer ( arrayBuffer, byteIndex, type, value, isLittleEndian )
    static JSTaggedValue SetValueInBuffer(JSTaggedValue arrBuf, uint32_t byteIndex, DataViewType type,
                                          JSTaggedNumber value, bool littleEndian);
    // 24.1.1.4 CloneArrayBuffer( srcBuffer, srcByteOffset [, cloneConstructor] )
    static JSTaggedValue CloneArrayBuffer(JSThread *thread, const JSHandle<JSTaggedValue> &srcBuffer,
                                          uint32_t srcByteOffset, JSHandle<JSTaggedValue> constructor);
    // 24.1.1.1 AllocateArrayBuffer(constructor, byteLength)
    static JSTaggedValue AllocateArrayBuffer(JSThread *thread, const JSHandle<JSTaggedValue> &newTarget,
                                             double byteLength);

private:
    template <typename T>
    static T LittleEndianToBigEndian(T liValue);

    static uint64_t LittleEndianToBigEndianUint64(uint64_t liValue);

    template<typename T>
    static void SetTypeData(uint8_t *block, T value, uint32_t index);

    template<typename T, NumberSize size>
    static JSTaggedValue GetValueFromBufferForInteger(uint8_t *block, uint32_t byteIndex, bool littleEndian);

    template<typename T, typename UnionType, NumberSize size>
    static JSTaggedValue GetValueFromBufferForFloat(uint8_t *block, uint32_t byteIndex, bool littleEndian);

    template<typename T>
    static void SetValueInBufferForByte(double val, uint8_t *block, uint32_t byteIndex);

    static void SetValueInBufferForUint8Clamped(double val, uint8_t *block, uint32_t byteIndex);

    template<typename T>
    static void SetValueInBufferForInteger(double val, uint8_t *block, uint32_t byteIndex, bool littleEndian);

    template<typename T>
    static void SetValueInBufferForFloat(double val, uint8_t *block, uint32_t byteIndex, bool littleEndian);
};
}  // namespace panda::ecmascript::builtins

#endif  // ECMASCRIPT_BUILTINS_BUILTINS_ARRAYBUFFER_H
