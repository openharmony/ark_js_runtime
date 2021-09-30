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

#include "runtime_trampolines.h"
#include "ecmascript/accessor_data.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_proxy.h"
#include "ecmascript/layout_info.h"
#include "ecmascript/message_string.h"
#include "ecmascript/object_factory.h"

namespace panda::ecmascript {
bool RuntimeTrampolines::AddElementInternal(uint64_t argThread, uint64_t argReceiver, uint32_t argIndex,
                                            uint64_t argValue, uint32_t argAttr)
{
    auto thread = reinterpret_cast<JSThread *>(argThread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSObject> receiver(thread, reinterpret_cast<TaggedObject *>(argReceiver));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(argValue)));
    auto attr = static_cast<PropertyAttributes>(argAttr);
    return JSObject::AddElementInternal(thread, receiver, argIndex, value, attr);
}

bool RuntimeTrampolines::CallSetter(uint64_t argThread, uint64_t argSetter, uint64_t argReceiver, uint64_t argValue,
                                    bool argMayThrow)
{
    auto thread = reinterpret_cast<JSThread *>(argThread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSTaggedValue> receiver(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(argReceiver)));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(argValue)));
    auto setter = AccessorData::Cast((reinterpret_cast<panda::ObjectHeader *>(argSetter)));
    return JSObject::CallSetter(thread, *setter, receiver, value, argMayThrow);
}

void RuntimeTrampolines::ThrowTypeError(uint64_t argThread, int argMessageStringId)
{
    auto thread = reinterpret_cast<JSThread *>(argThread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    std::string message = MessageString::GetMessageString(argMessageStringId);
    ObjectFactory *factory = JSThread::Cast(thread)->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::TYPE_ERROR, message.c_str());
    THROW_NEW_ERROR_AND_RETURN(thread, error.GetTaggedValue());
}

bool RuntimeTrampolines::JSProxySetProperty(uint64_t argThread, uint64_t argProxy, uint64_t argKey, uint64_t argValue,
                                            uint64_t argReceiver, bool argMayThrow)
{
    auto thread = reinterpret_cast<JSThread *>(argThread);
    [[maybe_unused]] EcmaHandleScope handleScope(thread);
    JSHandle<JSProxy> proxy(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(argProxy)));
    JSHandle<JSTaggedValue> index(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(argKey)));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(argValue)));
    JSHandle<JSTaggedValue> receiver(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(argReceiver)));

    return JSProxy::SetProperty(thread, proxy, index, value, receiver, argMayThrow);
}

uint32_t RuntimeTrampolines::GetHash32(uint64_t key, uint64_t len)
{
    auto pkey = reinterpret_cast<uint8_t *>(key);
    return panda::GetHash32(pkey, static_cast<size_t>(len));
}

uint64_t RuntimeTrampolines::CallGetter(uint64_t argThread, uint64_t argGetter, uint64_t argReceiver)
{
    auto thread = reinterpret_cast<JSThread *>(argThread);
    auto accessor = AccessorData::Cast(reinterpret_cast<TaggedObject *>(argGetter));
    JSHandle<JSTaggedValue> objHandle(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(argReceiver)));
    return JSObject::CallGetter(thread, accessor, objHandle).GetRawData();
}

uint64_t RuntimeTrampolines::AccessorGetter(uint64_t argThread, uint64_t argGetter, uint64_t argReceiver)
{
    auto thread = reinterpret_cast<JSThread *>(argThread);
    auto accessor = AccessorData::Cast(reinterpret_cast<TaggedObject *>(argGetter));
    JSHandle<JSObject> objHandle(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(argReceiver)));
    return accessor->CallInternalGet(thread, objHandle).GetRawData();
}

int32_t RuntimeTrampolines::FindElementWithCache(uint64_t argThread, uint64_t hClass, uint64_t key, int32_t num)
{
    auto thread = reinterpret_cast<JSThread *>(argThread);
    auto cls  = reinterpret_cast<JSHClass *>(hClass);
    auto layoutInfo = LayoutInfo::Cast(cls->GetAttributes().GetTaggedObject());
    return layoutInfo->FindElementWithCache(thread, cls, JSTaggedValue(key), num);
}

uint32_t RuntimeTrampolines::StringGetHashCode(uint64_t ecmaString)
{
    auto string = reinterpret_cast<EcmaString *>(ecmaString);
    return string->GetHashcode();
}
}  // namespace panda::ecmascript
