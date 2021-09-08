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

#ifndef ECMASCRIPT_IC_IC_RUNTIME_H
#define ECMASCRIPT_IC_IC_RUNTIME_H

#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/accessor_data.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_handle.h"

namespace panda::ecmascript {
class ProfileTypeInfo;
class JSThread;
class ObjectOperator;

class ICRuntime {
public:
    ICRuntime(JSThread *thread, JSHandle<ProfileTypeInfo> profileTypeInfo, uint32_t slotId, ICKind kind)
        : thread_(thread), icAccessor_(thread, profileTypeInfo, slotId, kind)
    {
    }

    ~ICRuntime() = default;

    void UpdateLoadHandler(const ObjectOperator &op, JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> receiver);
    void UpdateStoreHandler(const ObjectOperator &op, JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> receiver);

    JSThread *GetThread() const
    {
        return thread_;
    }

    void UpdateReceiverHClass(JSHandle<JSTaggedValue> receiverHClass)
    {
        receiverHClass_ = receiverHClass;
    }

    ICKind GetICKind() const
    {
        return icAccessor_.GetKind();
    }

    void TraceIC(JSHandle<JSTaggedValue> receiver, JSHandle<JSTaggedValue> key) const;

protected:
    JSThread *thread_;
    JSHandle<JSTaggedValue> receiverHClass_{};
    ProfileTypeAccessor icAccessor_;
};

class LoadICRuntime : public ICRuntime {
public:
    LoadICRuntime(JSThread *thread, JSHandle<ProfileTypeInfo> profileTypeInfo, uint32_t slotId, ICKind kind)
        : ICRuntime(thread, profileTypeInfo, slotId, kind)
    {
    }

    ~LoadICRuntime() = default;

    JSTaggedValue LoadMiss(JSHandle<JSTaggedValue> receiver, JSHandle<JSTaggedValue> key);
};

class StoreICRuntime : public ICRuntime {
public:
    StoreICRuntime(JSThread *thread, JSHandle<ProfileTypeInfo> profileTypeInfo, uint32_t slotId, ICKind kind)
        : ICRuntime(thread, profileTypeInfo, slotId, kind)
    {
    }

    ~StoreICRuntime() = default;

    JSTaggedValue StoreMiss(JSHandle<JSTaggedValue> receiver, JSHandle<JSTaggedValue> key,
                                 JSHandle<JSTaggedValue> value);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_IC_IC_RUNTIME_H
