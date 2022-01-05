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

#ifndef ECMASCRIPT_INTERNAL_CALL_PARAMS_H
#define ECMASCRIPT_INTERNAL_CALL_PARAMS_H

#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_function.h"
#include "ecmascript/mem/c_containers.h"

namespace panda::ecmascript {
class InternalCallParams {
public:
    static constexpr uint8_t RESERVE_INTERNAL_CALL_PARAMS_FIXED_LENGTH = 128;

    InternalCallParams()
    {
    }

    ~InternalCallParams() = default;

    inline const JSTaggedType *GetArgv() const
    {
        if (IsFixedMode()) {
            return &fixed_data_.front();
        }

        ASSERT_PRINT(variable_length_ > RESERVE_INTERNAL_CALL_PARAMS_FIXED_LENGTH, "internal call params mode error");
        return variable_data_.data();
    }

    inline uint32_t GetLength() const
    {
        if (IsFixedMode()) {
            return fixed_length_;
        }

        ASSERT_PRINT(variable_length_ > RESERVE_INTERNAL_CALL_PARAMS_FIXED_LENGTH, "internal call params mode error");
        return variable_length_;
    }

    template<typename T>
    inline void MakeArgv(const JSHandle<T> &arg)
    {
        EnableFixedModeAndSetLength(1);
        fixed_data_[0] = arg.GetTaggedType();
    }

    template<typename T0, typename T1>
    inline void MakeArgv(const JSHandle<T0> &arg0, const JSHandle<T1> &arg1)
    {
        EnableFixedModeAndSetLength(2);
        fixed_data_[0] = arg0.GetTaggedType();
        fixed_data_[1] = arg1.GetTaggedType();
    }

    template<typename T0, typename T1, typename T2>
    inline void MakeArgv(const JSHandle<T0> &arg0, const JSHandle<T1> &arg1, const JSHandle<T2> &arg2)
    {
        EnableFixedModeAndSetLength(3);
        fixed_data_[0] = arg0.GetTaggedType();
        fixed_data_[1] = arg1.GetTaggedType();
        fixed_data_[2] = arg2.GetTaggedType();
    }

    template<typename T0, typename T1, typename T2, typename T3>
    inline void MakeArgv(const JSHandle<T0> &arg0, const JSHandle<T1> &arg1, const JSHandle<T2> &arg2,
                         const JSHandle<T3> &arg3)
    {
        EnableFixedModeAndSetLength(4);
        fixed_data_[0] = arg0.GetTaggedType();
        fixed_data_[1] = arg1.GetTaggedType();
        fixed_data_[2] = arg2.GetTaggedType();
        fixed_data_[3] = arg3.GetTaggedType();
    }

    inline void MakeEmptyArgv()
    {
        EnableFixedModeAndSetLength(0);
    }

    inline void MakeArgv(const JSTaggedValue arg)
    {
        EnableFixedModeAndSetLength(1);
        fixed_data_[0] = arg.GetRawData();
    }

    inline void MakeArgv(const JSTaggedValue arg0, const JSTaggedValue arg1)
    {
        EnableFixedModeAndSetLength(2);
        fixed_data_[0] = arg0.GetRawData();
        fixed_data_[1] = arg1.GetRawData();
    }

    inline void MakeArgv(const JSTaggedValue arg0, const JSTaggedValue arg1, const JSTaggedValue arg2)
    {
        EnableFixedModeAndSetLength(3);
        fixed_data_[0] = arg0.GetRawData();
        fixed_data_[1] = arg1.GetRawData();
        fixed_data_[2] = arg2.GetRawData();
    }

    inline void MakeArgv(const JSTaggedValue arg0, const JSTaggedValue arg1, const JSTaggedValue arg2,
                         const JSTaggedValue arg3)
    {
        EnableFixedModeAndSetLength(4);
        fixed_data_[0] = arg0.GetRawData();
        fixed_data_[1] = arg1.GetRawData();
        fixed_data_[2] = arg2.GetRawData();
        fixed_data_[3] = arg3.GetRawData();
    }

    void MakeArgv(const EcmaRuntimeCallInfo *info, uint32_t position);

    void MakeArgList(const TaggedArray *argv);
    void MakeArgListWithHole(const TaggedArray *argv, uint32_t length);

    void MakeBoundArgv(const JSThread *thread, const JSHandle<JSBoundFunction> &boundFunc);

    void Iterate(const RootRangeVisitor &v) const;

private:
    DEFAULT_COPY_SEMANTIC(InternalCallParams);
    DEFAULT_MOVE_SEMANTIC(InternalCallParams);

    inline bool IsFixedMode() const
    {
        return !variable_mode_;
    }

    inline void EnableFixedModeAndSetLength(uint32_t length)
    {
        variable_mode_ = false;
        variable_data_.clear();
        variable_length_ = 0;
        fixed_length_ = length;
    }

    inline uint32_t GetFixedLength() const
    {
        return fixed_length_;
    }

    inline uintptr_t GetFixedDataAddress() const
    {
        return ToUintPtr(&fixed_data_);
    }

    inline JSTaggedType GetFixedBuffer(uint32_t idx) const
    {
        return fixed_data_[idx];
    }

    inline void SetFixedBuffer(uint32_t idx, JSHandle<JSTaggedValue> val)
    {
        fixed_data_[idx] = val.GetTaggedType();
    }

    inline void SetFixedBuffer(uint32_t idx, JSTaggedValue val)
    {
        fixed_data_[idx] = val.GetRawData();
    }

    inline void SetFixedBuffer(uint32_t idx, JSTaggedType val)
    {
        fixed_data_[idx] = val;
    }

    inline void EnableVariableModeAndSetLength(uint32_t length)
    {
        variable_mode_ = true;
        fixed_length_ = 0;
        variable_length_ = length;
        variable_data_.resize(variable_length_);
    }

    inline uint32_t GetVariableLength() const
    {
        return variable_length_;
    }

    inline uintptr_t GetVariableDataAddress() const
    {
        return ToUintPtr(variable_data_.data());
    }

    inline JSTaggedType GetVariableBuffer(uint32_t idx) const
    {
        return variable_data_[idx];
    }

    inline void SetVariableBuffer(uint32_t idx, JSHandle<JSTaggedValue> val)
    {
        variable_data_[idx] = val.GetTaggedType();
    }

    inline void SetVariableBuffer(uint32_t idx, JSTaggedValue val)
    {
        variable_data_[idx] = val.GetRawData();
    }

    inline void SetVariableBuffer(uint32_t idx, JSTaggedType val)
    {
        variable_data_[idx] = val;
    }

    inline void InsertVariableBuffer(JSTaggedValue val)
    {
        variable_data_.insert(variable_data_.begin(), val.GetRawData());
    }

    std::array<JSTaggedType, RESERVE_INTERNAL_CALL_PARAMS_FIXED_LENGTH> fixed_data_{};
    CVector<JSTaggedType> variable_data_{};
    uint32_t fixed_length_{0};
    uint32_t variable_length_{0};
    bool variable_mode_{false};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_INTERNAL_CALL_PARAMS_H
