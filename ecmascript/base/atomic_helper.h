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

#ifndef ECMASCRIPT_BASE_ATOMIC_HELPER_H
#define ECMASCRIPT_BASE_ATOMIC_HELPER_H

#include "ecmascript/js_dataview.h"

namespace panda::ecmascript::base {
enum class BytesSize : int32_t {ONEBYTES = 1, TWOBYTES = 2, FOURBYTES = 4, EIGHTBYTES = 8};

class AtomicHelper final {
public:
    struct SubFun {
        template<typename T>
        T operator()(T *ptr, const T *arg) const
        {
            std::atomic<T> *atomicValue = reinterpret_cast<std::atomic<T> *>(ptr);
            return atomicValue->fetch_sub(arg[0], std::memory_order_seq_cst);
        }
    };

    struct AddFun {
        template<typename T>
        T operator()(T *ptr, const T *arg) const
        {
            std::atomic<T> *atomicValue = reinterpret_cast<std::atomic<T> *>(ptr);
            return atomicValue->fetch_add(arg[0], std::memory_order_seq_cst);
        }
    };

    struct AndFun {
        template<typename T>
        T operator()(T *ptr, const T *arg) const
        {
            std::atomic<T> *atomicValue = reinterpret_cast<std::atomic<T> *>(ptr);
            return atomicValue->fetch_and(arg[0], std::memory_order_seq_cst);
        }
    };

    struct OrFun {
        template<typename T>
        T operator()(T *ptr, const T *arg) const
        {
            std::atomic<T> *atomicValue = reinterpret_cast<std::atomic<T> *>(ptr);
            return atomicValue->fetch_or(arg[0], std::memory_order_seq_cst);
        }
    };

    struct XorFun {
        template<typename T>
        T operator()(T *ptr, const T *arg) const
        {
            std::atomic<T> *atomicValue = reinterpret_cast<std::atomic<T> *>(ptr);
            return atomicValue->fetch_xor(arg[0], std::memory_order_seq_cst);
        }
    };

    struct CompareExchangeFun {
        template<typename T>
        T operator()(T *ptr, const T *arg) const
        {
            T a = arg[0];
            std::atomic<T> *atomicValue = reinterpret_cast<std::atomic<T> *>(ptr);
            atomicValue->compare_exchange_strong(a, arg[1], std::memory_order_seq_cst);
            return a;
        }
    };

    struct ExchangeFun {
        template<typename T>
        T operator()(T *ptr, const T *arg) const
        {
            std::atomic<T> *atomicValue = reinterpret_cast<std::atomic<T> *>(ptr);
            return atomicValue->exchange(arg[0], std::memory_order_seq_cst);
        }
    };

    // 25.4.1.1 ValidateIntegerTypedArray ( typedArray [ , waitable ] )
    static JSTaggedValue ValidateIntegerTypedArray(JSThread *thread, JSHandle<JSTaggedValue> typedArray,
                                                   bool waitable = false);
    // 25.4.2.2 ValidateAtomicAccess ( typedArray, requestIndex )
    static uint32_t ValidateAtomicAccess(JSThread *thread, const JSHandle<JSTaggedValue> typedArray,
                                        JSHandle<JSTaggedValue> requestIndex);
    static JSTaggedValue AtomicStore(JSThread *thread, const JSHandle<JSTaggedValue> &typedArray,
                                     JSHandle<JSTaggedValue> index, JSHandle<JSTaggedValue> &value);
    static JSTaggedValue AtomicLoad(JSThread *thread, const JSHandle<JSTaggedValue> &typedArray,
                                    JSHandle<JSTaggedValue> index);
};
}  // namespace panda::ecmascript::base

#endif  // ECMASCRIPT_BASE_ATOMIC_HELPER_H