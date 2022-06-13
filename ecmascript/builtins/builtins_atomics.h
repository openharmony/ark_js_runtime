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

#ifndef ECMASCRIPT_BUILTINS_BUILTINS_ATOMICS_H
#define ECMASCRIPT_BUILTINS_BUILTINS_ATOMICS_H

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/waiter_list.h"

namespace panda::ecmascript::builtins {
enum class WaitResult: uint8_t {OK = 0, NOT_EQ, TIME_OUT};

class BuiltinsAtomics : public base::BuiltinsBase {
public:
    // 25.4.2 Atomics.add ( typedArray, index, value )
    static JSTaggedValue Add(EcmaRuntimeCallInfo *argv);
    // 25.4.3 Atomics.and ( typedArray, index, value )
    static JSTaggedValue And(EcmaRuntimeCallInfo *argv);
    // 25.4.4 Atomics.compareExchange ( typedArray, index, expectedValue, replacementValue)
    static JSTaggedValue CompareExchange(EcmaRuntimeCallInfo *argv);
    // 25.4.5 Atomics.exchange ( typedArray, index, value )
    static JSTaggedValue Exchange(EcmaRuntimeCallInfo *argv);
    // 25.4.6 Atomics.isLockFree ( size )
    static JSTaggedValue IsLockFree(EcmaRuntimeCallInfo *argv);
    // 25.4.7 Atomics.load ( typedArray, index )
    static JSTaggedValue Load(EcmaRuntimeCallInfo *argv);
	 // 25.4.8 Atomics.or ( typedArray, index, value )
    static JSTaggedValue Or(EcmaRuntimeCallInfo *argv);
    // 25.4.9 Atomics.store ( typedArray, index, value )
    static JSTaggedValue Store(EcmaRuntimeCallInfo *argv);
    // 25.4.10 Atomics.sub ( typedArray, index, value )
    static JSTaggedValue Sub(EcmaRuntimeCallInfo *argv);
    // 25.4.11 Atomics.wait ( typedArray, index, value, timeout)
    static JSTaggedValue Wait(EcmaRuntimeCallInfo *argv);
    // 25.4.12 Atomics.notify ( typedArray, index, count)
    static JSTaggedValue Notify(EcmaRuntimeCallInfo *argv);
    // 25.4.13 Atomics.xor ( typedArray, index, value )
    static JSTaggedValue Xor(EcmaRuntimeCallInfo *argv);

private:
    static uint32_t Signal(JSHandle<JSTaggedValue> &arrayBuffer, const size_t &index, double wakeCount);
    template <typename T>
    static WaitResult DoWait(JSThread *thread, JSHandle<JSTaggedValue> &arrayBuffer,
                             size_t index, T execpt, double timeout);
    template<typename callbackfun>
    static JSTaggedValue AtomicReadModifyWrite(JSThread *thread, const JSHandle<JSTaggedValue> &typedArray,
                                               JSHandle<JSTaggedValue> &index, EcmaRuntimeCallInfo *argv,
                                               const callbackfun &op);
    template<typename callbackfun>
    static JSTaggedValue AtomicReadModifyWriteCase(JSThread *thread, JSTaggedValue buffer, DataViewType type,
                                                   uint32_t indexedPosition, EcmaRuntimeCallInfo *argv,
                                                   const callbackfun &op);
    template<typename callbackfun>
    static JSTaggedValue HandleWithUint8(JSThread *thread, uint32_t size, uint8_t *block, uint32_t indexedPosition,
                                         EcmaRuntimeCallInfo *argv, const callbackfun &op);
    template<typename callbackfun>
    static JSTaggedValue HandleWithInt8(JSThread *thread, uint32_t size, uint8_t *block, uint32_t indexedPosition,
                                        EcmaRuntimeCallInfo *argv, const callbackfun &op);
    template<typename callbackfun>
    static JSTaggedValue HandleWithUint16(JSThread *thread, uint32_t size, uint8_t *block, uint32_t indexedPosition,
                                          EcmaRuntimeCallInfo *argv, const callbackfun &op);
    template<typename callbackfun>
    static JSTaggedValue HandleWithInt16(JSThread *thread, uint32_t size, uint8_t *block, uint32_t indexedPosition,
                                         EcmaRuntimeCallInfo *argv, const callbackfun &op);
    template<typename callbackfun>
    static JSTaggedValue HandleWithUint32(JSThread *thread, uint32_t size, uint8_t *block, uint32_t indexedPosition,
                                          EcmaRuntimeCallInfo *argv, const callbackfun &op);
    template<typename callbackfun>
    static JSTaggedValue HandleWithInt32(JSThread *thread, uint32_t size, uint8_t *block, uint32_t indexedPosition,
                                         EcmaRuntimeCallInfo *argv, const callbackfun &op);
    template<typename callbackfun>
    static JSTaggedValue HandleWithBigInt64(JSThread *thread, uint32_t size, uint8_t *block, uint32_t indexedPosition,
                                            EcmaRuntimeCallInfo *argv, const callbackfun &op);
    template<typename callbackfun>
    static JSTaggedValue HandleWithBigUint64(JSThread *thread, uint32_t size, uint8_t *block, uint32_t indexedPosition,
                                             EcmaRuntimeCallInfo *argv, const callbackfun &op);

    static constexpr int ARGS_NUMBER = 2;
};
}  // namespace panda::ecmascript::builtins
#endif  // ECMASCRIPT_BUILTINS_BUILTINS_MATH_H