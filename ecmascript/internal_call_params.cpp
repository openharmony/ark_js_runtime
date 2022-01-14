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

#include "ecmascript/internal_call_params.h"

namespace panda::ecmascript {
void InternalCallParams::MakeArgv(const EcmaRuntimeCallInfo *info, uint32_t position)
{
    int32_t mayLenth = info->GetArgsNumber() - position;
    uint32_t length =  mayLenth > 0 ? mayLenth : 0;
    if (LIKELY(length <= InternalCallParams::RESERVE_INTERNAL_CALL_PARAMS_FIXED_LENGTH)) {
        EnableFixedModeAndSetLength(length);
        for (uint32_t index = 0; index < length; ++index) {
            SetFixedBuffer(index, info->GetCallArg(index + position));
        }
        return;
    }

    EnableVariableModeAndSetLength(length);
    for (uint32_t index = 0; index < length; ++index) {
        SetVariableBuffer(index, info->GetCallArg(index + position));
    }
}

void InternalCallParams::MakeArgListWithHole(const TaggedArray *argv, uint32_t length)
{
    if (length > argv->GetLength()) {
        length = argv->GetLength();
    }
    ASSERT(length <= argv->GetLength());
    if (LIKELY(length <= InternalCallParams::RESERVE_INTERNAL_CALL_PARAMS_FIXED_LENGTH)) {
        EnableFixedModeAndSetLength(length);
        for (uint32_t index = 0; index < length; ++index) {
            auto value = argv->Get(index);
            SetFixedBuffer(index, value.IsHole() ? JSTaggedValue::Undefined() : value);
        }
        return;
    }

    EnableVariableModeAndSetLength(length);
    for (uint32_t index = 0; index < length; ++index) {
        auto value = argv->Get(index);
        SetVariableBuffer(index, value.IsHole() ? JSTaggedValue::Undefined() : value);
    }
}

void InternalCallParams::MakeArgList(const TaggedArray *argv)
{
    uint32_t length = argv->GetLength();
    if (LIKELY(length <= InternalCallParams::RESERVE_INTERNAL_CALL_PARAMS_FIXED_LENGTH)) {
        EnableFixedModeAndSetLength(length);
        for (uint32_t index = 0; index < length; ++index) {
            SetFixedBuffer(index, argv->Get(index));
        }
        return;
    }

    EnableVariableModeAndSetLength(length);
    for (uint32_t index = 0; index < length; ++index) {
        SetVariableBuffer(index, argv->Get(index));
    }
}

void InternalCallParams::MakeBoundArgv(const JSThread *thread, const JSHandle<JSBoundFunction> &boundFunc)
{
    JSHandle<TaggedArray> boundArgs(thread, boundFunc->GetBoundArguments());
    uint32_t boundLength = boundArgs->GetLength();
    uint32_t length = IsFixedMode() ? boundLength + GetFixedLength()
                                    : boundLength + GetVariableLength();
    if (LIKELY(length <= InternalCallParams::RESERVE_INTERNAL_CALL_PARAMS_FIXED_LENGTH)) {
        EnableFixedModeAndSetLength(length);

        // Prevent override, reverse write order
        for (array_ssize_t index = length - 1; index >= static_cast<array_ssize_t>(boundLength); --index) {
            SetFixedBuffer(index, GetFixedBuffer(index - boundLength));
        }

        for (uint32_t index = 0; index < boundLength; ++index) {
            SetFixedBuffer(index, boundArgs->Get(index));
        }
        return;
    }

    // need cross mode: fixed -> variable
    if (IsFixedMode()) {
        // enable variable mode not clear fixed buffer
        EnableVariableModeAndSetLength(length);
        for (uint32_t index = 0; index < boundLength; ++index) {
            SetVariableBuffer(index, boundArgs->Get(index));
        }

        for (uint32_t index = boundLength; index < length; ++index) {
            SetVariableBuffer(index, GetFixedBuffer(index - boundLength));
        }
        return;
    }

    EnableVariableModeAndSetLength(length);
    for (array_ssize_t index = boundLength - 1; index >= 0; --index) {
        InsertVariableBuffer(boundArgs->Get(index));
    }
}

void InternalCallParams::Iterate(const RootRangeVisitor &v) const
{
    if (GetLength() == 0) {
        return;
    }
    uintptr_t start = 0U;
    uintptr_t end = 0U;
    if (LIKELY(IsFixedMode())) {
        start = GetFixedDataAddress();
        end = start + sizeof(TaggedType) * fixed_length_;
    } else {
        start = GetVariableDataAddress();
        end = start + sizeof(TaggedType) * variable_length_;
    }
    v(Root::ROOT_INTERNAL_CALL_PARAMS, ObjectSlot(start), ObjectSlot(end));
}
}  // namespace panda::ecmascript
