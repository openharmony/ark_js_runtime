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

#ifndef ECMASCRIPT_IC_IC_HANDLER_H
#define ECMASCRIPT_IC_IC_HANDLER_H

#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/tagged_object.h"

namespace panda::ecmascript {
class HandlerBase {
public:
    static constexpr uint32_t KIND_BIT_LENGTH = 3;
    enum HandlerKind {
        NONE = 0,
        FIELD,
        ELEMENT,
        DICTIONARY,
        NON_EXIST,
    };

    using KindBit = BitField<HandlerKind, 0, KIND_BIT_LENGTH>;
    using InlinedPropsBit = KindBit::NextFlag;
    using AccessorBit = InlinedPropsBit::NextFlag;
    using InternalAccessorBit = AccessorBit::NextFlag;
    using IsJSArrayBit = InternalAccessorBit::NextFlag;
    using OffsetBit = IsJSArrayBit::NextField<uint32_t, PropertyAttributes::OFFSET_BITFIELD_NUM>;

    HandlerBase() = default;
    virtual ~HandlerBase() = default;

    static inline bool IsAccessor(uint32_t handler)
    {
        return AccessorBit::Get(handler);
    }

    static inline bool IsInternalAccessor(uint32_t handler)
    {
        return InternalAccessorBit::Get(handler);
    }

    static inline bool IsNonExist(uint32_t handler)
    {
        return GetKind(handler) == HandlerKind::NON_EXIST;
    }

    static inline bool IsField(uint32_t handler)
    {
        return GetKind(handler) == HandlerKind::FIELD;
    }

    static inline bool IsElement(uint32_t handler)
    {
        return GetKind(handler) == HandlerKind::ELEMENT;
    }

    static inline bool IsDictionary(uint32_t handler)
    {
        return GetKind(handler) == HandlerKind::DICTIONARY;
    }

    static inline bool IsInlinedProps(uint32_t handler)
    {
        return InlinedPropsBit::Get(handler);
    }

    static inline HandlerKind GetKind(uint32_t handler)
    {
        return KindBit::Get(handler);
    }

    static inline bool IsJSArray(uint32_t handler)
    {
        return IsJSArrayBit::Get(handler);
    }

    static inline int GetOffset(uint32_t handler)
    {
        return OffsetBit::Get(handler);
    }
};

class LoadHandler final : public HandlerBase {
public:
    static inline JSHandle<JSTaggedValue> LoadProperty(const JSThread *thread, const ObjectOperator &op);
    static inline JSHandle<JSTaggedValue> LoadElement(const JSThread *thread);
};

class StoreHandler final : public HandlerBase {
public:
    static inline JSHandle<JSTaggedValue> StoreProperty(const JSThread *thread, const ObjectOperator &op);
    static inline JSHandle<JSTaggedValue> StoreElement(const JSThread *thread,
                                                       JSHandle<JSTaggedValue> receiver);
};

class TransitionHandler : public TaggedObject {
public:
    static TransitionHandler *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsTransitionHandler());
        return static_cast<TransitionHandler *>(object);
    }

    static inline JSHandle<JSTaggedValue> StoreTransition(const JSThread *thread, const ObjectOperator &op);

    static constexpr size_t HANDLER_INFO_OFFSET = TaggedObjectSize();
    ACCESSORS(HandlerInfo, HANDLER_INFO_OFFSET, TRANSITION_HCLASS_OFFSET)

    ACCESSORS(TransitionHClass, TRANSITION_HCLASS_OFFSET, SIZE)

    DECL_VISIT_OBJECT(HANDLER_INFO_OFFSET, SIZE)
    DECL_DUMP()
};

class PrototypeHandler : public TaggedObject {
public:
    static PrototypeHandler *Cast(TaggedObject *object)
    {
        ASSERT(JSTaggedValue(object).IsPrototypeHandler());
        return static_cast<PrototypeHandler *>(object);
    }

    static inline JSHandle<JSTaggedValue> LoadPrototype(const JSThread *thread, const ObjectOperator &op,
                                                        const JSHandle<JSHClass> &hclass);
    static inline JSHandle<JSTaggedValue> StorePrototype(const JSThread *thread, const ObjectOperator &op,
                                                         const JSHandle<JSHClass> &hclass);

    static constexpr size_t HANDLER_INFO_OFFSET = TaggedObjectSize();

    ACCESSORS(HandlerInfo, HANDLER_INFO_OFFSET, PROTO_CELL_OFFSET)

    ACCESSORS(ProtoCell, PROTO_CELL_OFFSET, HOLDER_OFFSET)

    ACCESSORS(Holder, HOLDER_OFFSET, SIZE)

    DECL_VISIT_OBJECT(HANDLER_INFO_OFFSET, SIZE)
    DECL_DUMP()
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_IC_IC_HANDLER_H
