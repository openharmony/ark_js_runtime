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

#include "ecmascript/compiler/stub.h"
#include "ecmascript/compiler/llvm_ir_builder.h"
#include "ecmascript/compiler/stub-inl.h"
#include "ecmascript/js_api_arraylist.h"
#include "ecmascript/js_api_vector.h"
#include "ecmascript/js_object.h"
#include "ecmascript/mem/remembered_set.h"
#include "ecmascript/message_string.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tagged_hash_table.h"
#include "ecmascript/compiler/rt_call_signature.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript::kungfu {
void Stub::Jump(Label *label)
{
    ASSERT(label);
    auto currentLabel = env_.GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto jump = env_.GetBulder()->Goto(currentControl);
    currentLabel->SetControl(jump);
    label->AppendPredecessor(currentLabel);
    label->MergeControl(currentLabel->GetControl());
    env_.SetCurrentLabel(nullptr);
}

void Stub::Branch(GateRef condition, Label *trueLabel, Label *falseLabel)
{
    auto currentLabel = env_.GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef ifBranch = env_.GetBulder()->Branch(currentControl, condition);
    currentLabel->SetControl(ifBranch);
    GateRef ifTrue = env_.GetBulder()->IfTrue(ifBranch);
    trueLabel->AppendPredecessor(env_.GetCurrentLabel());
    trueLabel->MergeControl(ifTrue);
    GateRef ifFalse = env_.GetBulder()->IfFalse(ifBranch);
    falseLabel->AppendPredecessor(env_.GetCurrentLabel());
    falseLabel->MergeControl(ifFalse);
    env_.SetCurrentLabel(nullptr);
}

void Stub::Switch(GateRef index, Label *defaultLabel, int64_t *keysValue, Label *keysLabel, int numberOfKeys)
{
    auto currentLabel = env_.GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    GateRef switchBranch = env_.GetBulder()->SwitchBranch(currentControl, index, numberOfKeys);
    currentLabel->SetControl(switchBranch);
    for (int i = 0; i < numberOfKeys; i++) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        GateRef switchCase = env_.GetBulder()->SwitchCase(switchBranch, keysValue[i]);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        keysLabel[i].AppendPredecessor(currentLabel);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        keysLabel[i].MergeControl(switchCase);
    }

    GateRef defaultCase = env_.GetBulder()->DefaultCase(switchBranch);
    defaultLabel->AppendPredecessor(currentLabel);
    defaultLabel->MergeControl(defaultCase);
    env_.SetCurrentLabel(nullptr);
}

void Stub::LoopBegin(Label *loopHead)
{
    ASSERT(loopHead);
    auto loopControl = env_.GetBulder()->LoopBegin(loopHead->GetControl());
    loopHead->SetControl(loopControl);
    loopHead->SetPreControl(loopControl);
    loopHead->Bind();
    env_.SetCurrentLabel(loopHead);
}

void Stub::LoopEnd(Label *loopHead)
{
    ASSERT(loopHead);
    auto currentLabel = env_.GetCurrentLabel();
    auto currentControl = currentLabel->GetControl();
    auto loopend = env_.GetBulder()->LoopEnd(currentControl);
    currentLabel->SetControl(loopend);
    loopHead->AppendPredecessor(currentLabel);
    loopHead->MergeControl(loopend);
    loopHead->Seal();
    loopHead->MergeAllControl();
    loopHead->MergeAllDepend();
    env_.SetCurrentLabel(nullptr);
}

// FindElementWithCache in ecmascript/layout_info-inl.h
GateRef Stub::FindElementWithCache(GateRef glue, GateRef layoutInfo, GateRef hClass,
    GateRef key, GateRef propsNum)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    DEFVARIABLE(result, VariableType::INT32(), Int32(-1));
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    Label exit(env);
    Label notExceedUpper(env);
    Label exceedUpper(env);
    Label afterExceedCon(env);
    // 9 : Builtins Object properties number is nine
    Branch(Int32LessThanOrEqual(propsNum, Int32(9)), &notExceedUpper, &exceedUpper);
    {
        Bind(&notExceedUpper);
            Label loopHead(env);
            Label loopEnd(env);
            Label afterLoop(env);
            Jump(&loopHead);
            LoopBegin(&loopHead);
            {
                Label propsNumIsZero(env);
                Label propsNumNotZero(env);
                Branch(Int32Equal(propsNum, Int32(0)), &propsNumIsZero, &propsNumNotZero);
                Bind(&propsNumIsZero);
                Jump(&afterLoop);
                Bind(&propsNumNotZero);
                GateRef elementAddr = GetPropertiesAddrFromLayoutInfo(layoutInfo);
                GateRef keyInProperty = Load(VariableType::INT64(), elementAddr,
                    PtrMul(ChangeInt32ToIntPtr(*i),
                        IntPtr(sizeof(panda::ecmascript::Properties))));
                Label equal(env);
                Label notEqual(env);
                Label afterEqualCon(env);
                Branch(Int64Equal(keyInProperty, key), &equal, &notEqual);
                Bind(&equal);
                result = *i;
                Jump(&exit);
                Bind(&notEqual);
                Jump(&afterEqualCon);
                Bind(&afterEqualCon);
                i = Int32Add(*i, Int32(1));
                Branch(Int32UnsignedLessThan(*i, propsNum), &loopEnd, &afterLoop);
                Bind(&loopEnd);
                LoopEnd(&loopHead);
            }
            Bind(&afterLoop);
            result = Int32(-1);
            Jump(&exit);
        Bind(&exceedUpper);
        Jump(&afterExceedCon);
    }
    Bind(&afterExceedCon);
    result = CallNGCRuntime(glue, RTSTUB_ID(FindElementWithCache), { glue, hClass, key, propsNum });
    Jump(&exit);
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::FindElementFromNumberDictionary(GateRef glue, GateRef elements, GateRef index)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    DEFVARIABLE(result, VariableType::INT32(), Int32(-1));
    Label exit(env);
    GateRef capcityoffset =
        PtrMul(IntPtr(JSTaggedValue::TaggedTypeSize()),
            IntPtr(TaggedHashTable<NumberDictionary>::SIZE_INDEX));
    GateRef dataoffset = IntPtr(TaggedArray::DATA_OFFSET);
    GateRef capacity = TaggedCastToInt32(Load(VariableType::INT64(), elements,
                                              PtrAdd(dataoffset, capcityoffset)));
    DEFVARIABLE(count, VariableType::INT32(), Int32(1));
    GateRef len = Int32(sizeof(int) / sizeof(uint8_t));
    GateRef hash = CallRuntime(glue, RTSTUB_ID(GetHash32),
        { IntBuildTaggedTypeWithNoGC(index), IntBuildTaggedTypeWithNoGC(len) });
    DEFVARIABLE(entry, VariableType::INT32(),
        Int32And(TruncInt64ToInt32(ChangeTaggedPointerToInt64(hash)), Int32Sub(capacity, Int32(1))));
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    GateRef element = GetKeyFromDictionary<NumberDictionary>(VariableType::JS_ANY(), elements, *entry);
    Label isHole(env);
    Label notHole(env);
    Branch(TaggedIsHole(element), &isHole, &notHole);
    Bind(&isHole);
    Jump(&loopEnd);
    Bind(&notHole);
    Label isUndefined(env);
    Label notUndefined(env);
    Branch(TaggedIsUndefined(element), &isUndefined, &notUndefined);
    Bind(&isUndefined);
    result = Int32(-1);
    Jump(&exit);
    Bind(&notUndefined);
    Label isMatch(env);
    Label notMatch(env);
    Branch(Int32Equal(index, TaggedCastToInt32(element)), &isMatch, &notMatch);
    Bind(&isMatch);
    result = *entry;
    Jump(&exit);
    Bind(&notMatch);
    Jump(&loopEnd);
    Bind(&loopEnd);
    entry = GetNextPositionForHash(*entry, *count, capacity);
    count = Int32Add(*count, Int32(1));
    LoopEnd(&loopHead);
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

// int TaggedHashTable<Derived>::FindEntry(const JSTaggedValue &key) in tagged_hash_table.h
GateRef Stub::FindEntryFromNameDictionary(GateRef glue, GateRef elements, GateRef key)
{
    auto env = GetEnvironment();
    Label funcEntry(env);
    env->SubCfgEntry(&funcEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT32(), Int32(-1));
    GateRef capcityoffset =
        PtrMul(IntPtr(JSTaggedValue::TaggedTypeSize()),
            IntPtr(TaggedHashTable<NumberDictionary>::SIZE_INDEX));
    GateRef dataoffset = IntPtr(TaggedArray::DATA_OFFSET);
    GateRef capacity = TaggedCastToInt32(Load(VariableType::INT64(), elements,
                                              PtrAdd(dataoffset, capcityoffset)));
    DEFVARIABLE(count, VariableType::INT32(), Int32(1));
    DEFVARIABLE(hash, VariableType::INT32(), Int32(0));
    // NameDictionary::hash
    Label isSymbol(env);
    Label notSymbol(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label beforeDefineHash(env);
    Branch(IsSymbol(key), &isSymbol, &notSymbol);
    Bind(&isSymbol);
    {
        hash = TaggedCastToInt32(Load(VariableType::INT64(), key,
            IntPtr(JSSymbol::HASHFIELD_OFFSET)));
        Jump(&beforeDefineHash);
    }
    Bind(&notSymbol);
    {
        Label isString(env);
        Label notString(env);
        Branch(IsString(key), &isString, &notString);
        Bind(&isString);
        {
            hash = GetHashcodeFromString(glue, key);
            Jump(&beforeDefineHash);
        }
        Bind(&notString);
        {
            Jump(&beforeDefineHash);
        }
    }
    Bind(&beforeDefineHash);
    // GetFirstPosition(hash, size)
    DEFVARIABLE(entry, VariableType::INT32(), Int32And(*hash, Int32Sub(capacity, Int32(1))));
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        GateRef element = GetKeyFromDictionary<NameDictionary>(VariableType::JS_ANY(), elements, *entry);
        Label isHole(env);
        Label notHole(env);
        Branch(TaggedIsHole(element), &isHole, &notHole);
        {
            Bind(&isHole);
            {
                Jump(&loopEnd);
            }
            Bind(&notHole);
            {
                Label isUndefined(env);
                Label notUndefined(env);
                Branch(TaggedIsUndefined(element), &isUndefined, &notUndefined);
                {
                    Bind(&isUndefined);
                    {
                        result = Int32(-1);
                        Jump(&exit);
                    }
                    Bind(&notUndefined);
                    {
                        Label isMatch(env);
                        Label notMatch(env);
                        Branch(Int64Equal(key, element), &isMatch, &notMatch);
                        {
                            Bind(&isMatch);
                            {
                                result = *entry;
                                Jump(&exit);
                            }
                            Bind(&notMatch);
                            {
                                Jump(&loopEnd);
                            }
                        }
                    }
                }
            }
        }
        Bind(&loopEnd);
        {
            entry = GetNextPositionForHash(*entry, *count, capacity);
            count = Int32Add(*count, Int32(1));
            LoopEnd(&loopHead);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::IsMatchInTransitionDictionary(GateRef element, GateRef key, GateRef metaData, GateRef attr)
{
    return TruncInt32ToInt1(Int32And(ZExtInt1ToInt32(Int64Equal(element, key)),
        ZExtInt1ToInt32(Int32Equal(metaData, attr))));
}

// metaData is int32 type
GateRef Stub::FindEntryFromTransitionDictionary(GateRef glue, GateRef elements, GateRef key, GateRef metaData)
{
    auto env = GetEnvironment();
    Label funcEntry(env);
    env->SubCfgEntry(&funcEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT32(), Int32(-1));
    GateRef capcityoffset =
        PtrMul(IntPtr(JSTaggedValue::TaggedTypeSize()),
            IntPtr(TaggedHashTable<NumberDictionary>::SIZE_INDEX));
    GateRef dataoffset = IntPtr(TaggedArray::DATA_OFFSET);
    GateRef capacity = TaggedCastToInt32(Load(VariableType::INT64(), elements,
                                              PtrAdd(dataoffset, capcityoffset)));
    DEFVARIABLE(count, VariableType::INT32(), Int32(1));
    DEFVARIABLE(hash, VariableType::INT32(), Int32(0));
    // TransitionDictionary::hash
    Label isSymbol(env);
    Label notSymbol(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label afterLoop(env);
    Label beforeDefineHash(env);
    Branch(IsSymbol(key), &isSymbol, &notSymbol);
    Bind(&isSymbol);
    {
        hash = TaggedCastToInt32(Load(VariableType::INT64(), key,
            IntPtr(panda::ecmascript::JSSymbol::HASHFIELD_OFFSET)));
        Jump(&beforeDefineHash);
    }
    Bind(&notSymbol);
    {
        Label isString(env);
        Label notString(env);
        Branch(IsString(key), &isString, &notString);
        Bind(&isString);
        {
            hash = GetHashcodeFromString(glue, key);
            Jump(&beforeDefineHash);
        }
        Bind(&notString);
        {
            Jump(&beforeDefineHash);
        }
    }
    Bind(&beforeDefineHash);
    hash = Int32Add(*hash, metaData);
    // GetFirstPosition(hash, size)
    DEFVARIABLE(entry, VariableType::INT32(), Int32And(*hash, Int32Sub(capacity, Int32(1))));
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        GateRef element = GetKeyFromDictionary<TransitionsDictionary>(VariableType::JS_ANY(), elements, *entry);
        Label isHole(env);
        Label notHole(env);
        Branch(TaggedIsHole(element), &isHole, &notHole);
        {
            Bind(&isHole);
            {
                Jump(&loopEnd);
            }
            Bind(&notHole);
            {
                Label isUndefined(env);
                Label notUndefined(env);
                Branch(TaggedIsUndefined(element), &isUndefined, &notUndefined);
                {
                    Bind(&isUndefined);
                    {
                        result = Int32(-1);
                        Jump(&exit);
                    }
                    Bind(&notUndefined);
                    {
                        Label isMatch(env);
                        Label notMatch(env);
                        Branch(
                            IsMatchInTransitionDictionary(element, key, metaData,
                                GetAttributesFromDictionary<TransitionsDictionary>(elements, *entry)),
                            &isMatch, &notMatch);
                        {
                            Bind(&isMatch);
                            {
                                result = *entry;
                                Jump(&exit);
                            }
                            Bind(&notMatch);
                            {
                                Jump(&loopEnd);
                            }
                        }
                    }
                }
            }
        }
        Bind(&loopEnd);
        {
            entry = GetNextPositionForHash(*entry, *count, capacity);
            count = Int32Add(*count, Int32(1));
            LoopEnd(&loopHead);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::JSObjectGetProperty(VariableType returnType, GateRef obj, GateRef hClass, GateRef attr)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, returnType, Undefined(returnType));
    Label inlinedProp(env);
    Label notInlinedProp(env);
    GateRef attrOffset = GetOffsetFieldInPropAttr(attr);
    Branch(IsInlinedProperty(attr), &inlinedProp, &notInlinedProp);
    {
        Bind(&inlinedProp);
        {
            // GetPropertyInlinedProps
            GateRef inlinedPropsStart = GetInlinedPropsStartFromHClass(hClass);
            GateRef propOffset = Int32Mul(
                Int32Add(inlinedPropsStart, attrOffset),
                Int32(JSTaggedValue::TaggedTypeSize()));
            result = Load(returnType, obj, ZExtInt32ToInt64(propOffset));
            Jump(&exit);
        }
        Bind(&notInlinedProp);
        {
            // compute outOfLineProp offset, get it and return
            GateRef array =
                Load(VariableType::INT64(), obj, IntPtr(JSObject::PROPERTIES_OFFSET));
            result = GetValueFromTaggedArray(returnType, array, Int32Sub(attrOffset,
                GetInlinedPropertiesFromHClass(hClass)));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void Stub::JSObjectSetProperty(GateRef glue, GateRef obj, GateRef hClass, GateRef attr, GateRef value)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    Label inlinedProp(env);
    Label notInlinedProp(env);
    GateRef attrOffset = GetOffsetFieldInPropAttr(attr);
    Branch(IsInlinedProperty(attr), &inlinedProp, &notInlinedProp);
    {
        Bind(&inlinedProp);
        {
            SetPropertyInlinedProps(glue, obj, hClass, value, attrOffset);
            Jump(&exit);
        }
        Bind(&notInlinedProp);
        {
            // compute outOfLineProp offset, get it and return
            GateRef array = Load(VariableType::JS_POINTER(), obj,
                                 IntPtr(JSObject::PROPERTIES_OFFSET));
            SetValueToTaggedArray(VariableType::JS_ANY(), glue, array, Int32Sub(attrOffset,
                GetInlinedPropertiesFromHClass(hClass)), value);
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->SubCfgExit();
    return;
}

GateRef Stub::ComputePropertyCapacityInJSObj(GateRef oldLength)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));
    GateRef newL = Int32Add(oldLength, Int32(JSObject::PROPERTIES_GROW_SIZE));
    Label reachMax(env);
    Label notReachMax(env);
    Branch(Int32GreaterThan(newL, Int32(PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES)),
        &reachMax, &notReachMax);
    {
        Bind(&reachMax);
        result = Int32(PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES);
        Jump(&exit);
        Bind(&notReachMax);
        result = newL;
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::CallGetterHelper(GateRef glue, GateRef receiver, GateRef holder, GateRef accessor)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Exception());

    Label isInternal(env);
    Label notInternal(env);
    Branch(IsAccessorInternal(accessor), &isInternal, &notInternal);
    Bind(&isInternal);
    {
        Label arrayLength(env);
        Label tryContinue(env);
        auto lengthAccessor = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue, ConstantIndex::ARRAY_LENGTH_ACCESSOR);
        Branch(Int64Equal(accessor, lengthAccessor), &arrayLength, &tryContinue);
        Bind(&arrayLength);
        {
            result = Load(VariableType::JS_ANY(), holder,
                          IntPtr(JSArray::LENGTH_OFFSET));
            Jump(&exit);
        }
        Bind(&tryContinue);
        result = CallRuntime(glue, RTSTUB_ID(CallInternalGetter), { accessor, holder });
        Jump(&exit);
    }
    Bind(&notInternal);
    {
        auto getter = Load(VariableType::JS_ANY(), accessor,
                           IntPtr(AccessorData::GETTER_OFFSET));
        Label objIsUndefined(env);
        Label objNotUndefined(env);
        Branch(TaggedIsUndefined(getter), &objIsUndefined, &objNotUndefined);
        // if getter is undefined, return undefiend
        Bind(&objIsUndefined);
        {
            result = Undefined();
            Jump(&exit);
        }
        Bind(&objNotUndefined);
        {
            auto retValue = JSCallDispatch(glue, getter, IntPtr(0),
                                           JSCallMode::CALL_GETTER, { receiver });
            Label noPendingException(env);
            Branch(HasPendingException(glue), &exit, &noPendingException);
            Bind(&noPendingException);
            {
                result = retValue;
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::CallSetterHelper(GateRef glue, GateRef receiver, GateRef accessor, GateRef value)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Exception());

    Label isInternal(env);
    Label notInternal(env);
    Branch(IsAccessorInternal(accessor), &isInternal, &notInternal);
    Bind(&isInternal);
    {
        result = CallRuntime(glue, RTSTUB_ID(CallInternalSetter), { receiver, accessor, value });
        Jump(&exit);
    }
    Bind(&notInternal);
    {
        auto setter = Load(VariableType::JS_ANY(), accessor,
                           IntPtr(AccessorData::SETTER_OFFSET));
        Label objIsUndefined(env);
        Label objNotUndefined(env);
        Branch(TaggedIsUndefined(setter), &objIsUndefined, &objNotUndefined);
        // if getter is undefined, return undefiend
        Bind(&objIsUndefined);
        {
            result = Undefined();
            Jump(&exit);
        }
        Bind(&objNotUndefined);
        {
            auto retValue = JSCallDispatch(glue, setter, IntPtr(1),
                                           JSCallMode::CALL_SETTER, { receiver, value });
            Label noPendingException(env);
            Branch(HasPendingException(glue), &exit, &noPendingException);
            Bind(&noPendingException);
            {
                result = retValue;
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::ShouldCallSetter(GateRef receiver, GateRef holder, GateRef accessor, GateRef attr)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::BOOL(), True());
    Label isInternal(env);
    Label notInternal(env);
    Branch(IsAccessorInternal(accessor), &isInternal, &notInternal);
    Bind(&isInternal);
    {
        Label receiverEqualsHolder(env);
        Label receiverNotEqualsHolder(env);
        Branch(Int64Equal(receiver, holder), &receiverEqualsHolder, &receiverNotEqualsHolder);
        Bind(&receiverEqualsHolder);
        {
            result = IsWritable(attr);
            Jump(&exit);
        }
        Bind(&receiverNotEqualsHolder);
        {
            result = False();
            Jump(&exit);
        }
    }
    Bind(&notInternal);
    {
        result = True();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void Stub::JSHClassAddProperty(GateRef glue, GateRef receiver, GateRef key, GateRef attr)
{
    auto env = GetEnvironment();
    Label subEntry(env);
    env->SubCfgEntry(&subEntry);
    Label exit(env);
    GateRef hclass = LoadHClass(receiver);
    GateRef metaData = GetPropertyMetaDataFromAttr(attr);
    GateRef newDyn = FindTransitions(glue, receiver, hclass, key, metaData);
    Label findHClass(env);
    Label notFindHClass(env);
    Branch(Int64Equal(newDyn, Int64(JSTaggedValue::VALUE_UNDEFINED)), &notFindHClass, &findHClass);
    Bind(&findHClass);
    {
        Jump(&exit);
    }
    Bind(&notFindHClass);
    {
        GateRef type = GetObjectType(hclass);
        GateRef size = Int32Mul(GetInlinedPropsStartFromHClass(hclass),
                                Int32(JSTaggedValue::TaggedTypeSize()));
        GateRef inlineProps = GetInlinedPropertiesFromHClass(hclass);
        GateRef newJshclass = CallRuntime(glue, RTSTUB_ID(NewEcmaDynClass),
            { IntBuildTaggedTypeWithNoGC(size), IntBuildTaggedTypeWithNoGC(type),
              IntBuildTaggedTypeWithNoGC(inlineProps) });
        CopyAllHClass(glue, newJshclass, hclass);
        CallRuntime(glue, RTSTUB_ID(UpdateLayOutAndAddTransition),
                    { hclass, newJshclass, key, IntBuildTaggedTypeWithNoGC(attr) });
#if ECMASCRIPT_ENABLE_IC
        NotifyHClassChanged(glue, hclass, newJshclass);
#endif
        StoreHClass(glue, receiver, newJshclass);
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
    return;
}

// if condition:objHandle->IsJSArray() &&
//      keyHandle.GetTaggedValue() == thread->GlobalConstants()->GetConstructorString()
GateRef Stub::SetHasConstructorCondition(GateRef glue, GateRef receiver, GateRef key)
{
    GateRef gConstOffset = PtrAdd(glue,
                                  IntPtr(JSThread::GlueData::GetGlobalConstOffset(env_.Is32Bit())));
    GateRef gCtorStr = Load(VariableType::JS_ANY(),
        gConstOffset,
        Int64Mul(Int64(sizeof(JSTaggedValue)),
            Int64(static_cast<uint64_t>(ConstantIndex::CONSTRUCTOR_STRING_INDEX))));
    GateRef isCtorStr = Int64Equal(key, gCtorStr);
    return Int32NotEqual(
        Int32And(SExtInt1ToInt32(IsJsArray(receiver)), SExtInt1ToInt32(isCtorStr)), Int32(0));
}

// Note: set return exit node
GateRef Stub::AddPropertyByName(GateRef glue, GateRef receiver, GateRef key, GateRef value,
                                GateRef propertyAttributes)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT64(), Undefined(VariableType::INT64()));
    Label setHasCtor(env);
    Label notSetHasCtor(env);
    Label afterCtorCon(env);
    GateRef hClass = LoadHClass(receiver);
    Branch(SetHasConstructorCondition(glue, receiver, key), &setHasCtor, &notSetHasCtor);
    {
        Bind(&setHasCtor);
        SetHasConstructorToHClass(glue, hClass, Int32(1));
        Jump(&afterCtorCon);
        Bind(&notSetHasCtor);
        Jump(&afterCtorCon);
    }
    Bind(&afterCtorCon);
    // 0x111 : default attribute for property: writable, enumerable, configurable
    DEFVARIABLE(attr, VariableType::INT32(), propertyAttributes);
    GateRef numberOfProps = GetNumberOfPropsFromHClass(hClass);
    GateRef inlinedProperties = GetInlinedPropertiesFromHClass(hClass);
    Label hasUnusedInProps(env);
    Label noUnusedInProps(env);
    Label afterInPropsCon(env);
    Branch(Int32UnsignedLessThan(numberOfProps, inlinedProperties), &hasUnusedInProps, &noUnusedInProps);
    {
        Bind(&noUnusedInProps);
        Jump(&afterInPropsCon);
        Bind(&hasUnusedInProps);
        {
            SetPropertyInlinedProps(glue, receiver, hClass, value, numberOfProps);
            attr = SetOffsetFieldInPropAttr(*attr, numberOfProps);
            attr = SetIsInlinePropsFieldInPropAttr(*attr, Int32(1)); // 1: set inInlineProps true
            JSHClassAddProperty(glue, receiver, key, *attr);
            result = Undefined(VariableType::INT64());
            Jump(&exit);
        }
    }
    Bind(&afterInPropsCon);
    DEFVARIABLE(array, VariableType::JS_POINTER(), GetPropertiesArray(receiver));
    DEFVARIABLE(length, VariableType::INT32(), GetLengthOfTaggedArray(*array));
    Label lenIsZero(env);
    Label lenNotZero(env);
    Label afterLenCon(env);
    Branch(Int32Equal(*length, Int32(0)), &lenIsZero, &lenNotZero);
    {
        Bind(&lenIsZero);
        {
            length = Int32(JSObject::MIN_PROPERTIES_LENGTH);
            array = CallRuntime(glue, RTSTUB_ID(NewTaggedArray), { IntBuildTaggedTypeWithNoGC(*length) });
            SetPropertiesArray(glue, receiver, *array);
            Jump(&afterLenCon);
        }
        Bind(&lenNotZero);
        Jump(&afterLenCon);
    }
    Bind(&afterLenCon);
    Label isDictMode(env);
    Label notDictMode(env);
    Branch(IsDictionaryMode(*array), &isDictMode, &notDictMode);
    {
        Bind(&isDictMode);
        {
            GateRef res = CallRuntime(glue, RTSTUB_ID(NameDictPutIfAbsent),
                                      {receiver, *array, key, value, IntBuildTaggedTypeWithNoGC(*attr), TaggedFalse()});
            SetPropertiesArray(glue, receiver, res);
            Jump(&exit);
        }
        Bind(&notDictMode);
        {
            attr = SetIsInlinePropsFieldInPropAttr(*attr, Int32(0));
            GateRef outProps = Int32Sub(numberOfProps, inlinedProperties);
            Label isArrayFull(env);
            Label arrayNotFull(env);
            Label afterArrLenCon(env);
            Branch(Int32Equal(*length, outProps), &isArrayFull, &arrayNotFull);
            {
                Bind(&isArrayFull);
                {
                    Label ChangeToDict(env);
                    Label notChangeToDict(env);
                    Label afterDictChangeCon(env);
                    Branch(Int32Equal(*length, Int32(JSHClass::MAX_CAPACITY_OF_OUT_OBJECTS)),
                        &ChangeToDict, &notChangeToDict);
                    {
                        Bind(&ChangeToDict);
                        {
                            attr = SetDictionaryOrderFieldInPropAttr(*attr,
                                Int32(PropertyAttributes::MAX_CAPACITY_OF_PROPERTIES));
                            GateRef res = CallRuntime(glue, RTSTUB_ID(NameDictPutIfAbsent),
                                { receiver, *array, key, value, IntBuildTaggedTypeWithNoGC(*attr), TaggedTrue() });
                            SetPropertiesArray(glue, receiver, res);
                            result = Undefined(VariableType::INT64());
                            Jump(&exit);
                        }
                        Bind(&notChangeToDict);
                        Jump(&afterDictChangeCon);
                    }
                    Bind(&afterDictChangeCon);
                    GateRef capacity = ComputePropertyCapacityInJSObj(*length);
                    array = CallRuntime(glue, RTSTUB_ID(CopyArray),
                        { *array, IntBuildTaggedTypeWithNoGC(*length), IntBuildTaggedTypeWithNoGC(capacity) });
                    SetPropertiesArray(glue, receiver, *array);
                    Jump(&afterArrLenCon);
                }
                Bind(&arrayNotFull);
                Jump(&afterArrLenCon);
            }
            Bind(&afterArrLenCon);
            {
                attr = SetOffsetFieldInPropAttr(*attr, numberOfProps);
                JSHClassAddProperty(glue, receiver, key, *attr);
                SetValueToTaggedArray(VariableType::JS_ANY(), glue, *array, outProps, value);
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void Stub::ThrowTypeAndReturn(GateRef glue, int messageId, GateRef val)
{
    GateRef msgIntId = Int32(messageId);
    CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntBuildTaggedTypeWithNoGC(msgIntId) });
    Return(val);
}

GateRef Stub::TaggedToRepresentation(GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(resultRep, VariableType::INT64(),
                Int64(static_cast<int32_t>(Representation::OBJECT)));
    Label isInt(env);
    Label notInt(env);

    Branch(TaggedIsInt(value), &isInt, &notInt);
    Bind(&isInt);
    {
        resultRep = Int64(static_cast<int32_t>(Representation::INT));
        Jump(&exit);
    }
    Bind(&notInt);
    {
        Label isDouble(env);
        Label notDouble(env);
        Branch(TaggedIsDouble(value), &isDouble, &notDouble);
        Bind(&isDouble);
        {
            resultRep = Int64(static_cast<int32_t>(Representation::DOUBLE));
            Jump(&exit);
        }
        Bind(&notDouble);
        {
            resultRep = Int64(static_cast<int32_t>(Representation::OBJECT));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *resultRep;
    env->SubCfgExit();
    return ret;
}

void Stub::Store(VariableType type, GateRef glue, GateRef base, GateRef offset, GateRef value)
{
    auto depend = env_.GetCurrentLabel()->GetDepend();
    GateRef result;
    if (env_.IsArch64Bit()) {
        GateRef ptr = Int64Add(base, offset);
        if (type == VariableType::NATIVE_POINTER()) {
            type = VariableType::INT64();
        }
        result = env_.GetCircuit()->NewGate(OpCode(OpCode::STORE), 0, { depend, value, ptr }, type.GetGateType());
        env_.GetCurrentLabel()->SetDepend(result);
    } else if (env_.IsArch32Bit()) {
        if (type == VariableType::NATIVE_POINTER()) {
            type = VariableType::INT32();
        }
        GateRef ptr = Int32Add(base, offset);
        result = env_.GetCircuit()->NewGate(OpCode(OpCode::STORE), 0, { depend, value, ptr }, type.GetGateType());
        env_.GetCurrentLabel()->SetDepend(result);
    } else {
        UNREACHABLE();
    }
    if (type == VariableType::JS_POINTER() || type == VariableType::JS_ANY()) {
        SetValueWithBarrier(glue, base, offset, value);
    }
    return;
}

void Stub::SetValueWithBarrier(GateRef glue, GateRef obj, GateRef offset, GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isHeapObject(env);
    Label isVailedIndex(env);
    Label notValidIndex(env);

    Branch(TaggedIsHeapObject(value), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        GateRef objectRegion = ObjectAddressToRange(obj);
        GateRef valueRegion = ObjectAddressToRange(value);
        GateRef slotAddr = PtrAdd(TaggedCastToIntPtr(obj), offset);
        GateRef objectNotInYoung = BoolNot(InYoungGeneration(objectRegion));
        GateRef valueRegionInYoung = InYoungGeneration(valueRegion);
        Branch(BoolAnd(objectNotInYoung, valueRegionInYoung), &isVailedIndex, &notValidIndex);
        Bind(&isVailedIndex);
        {
            GateRef loadOffset = IntPtr(Region::GetOldToNewSetOffset(env_.Is32Bit()));
            auto oldToNewSet = Load(VariableType::NATIVE_POINTER(), objectRegion, loadOffset);
            Label isNullPtr(env);
            Label notNullPtr(env);
            Branch(IntPtrEuqal(oldToNewSet, IntPtr(0)), &isNullPtr, &notNullPtr);
            Bind(&notNullPtr);
            {
                // (slotAddr - this) >> TAGGED_TYPE_SIZE_LOG
                GateRef bitOffsetPtr = IntPtrLSR(PtrSub(slotAddr, objectRegion), IntPtr(TAGGED_TYPE_SIZE_LOG));
                GateRef bitOffset = TruncPtrToInt32(bitOffsetPtr);
                GateRef bitPerWordLog2 = Int32(GCBitset::BIT_PER_WORD_LOG2);
                GateRef bytePerWord = Int32(GCBitset::BYTE_PER_WORD);
                // bitOffset >> BIT_PER_WORD_LOG2
                GateRef index = Int32LSR(bitOffset, bitPerWordLog2);
                GateRef byteIndex = Int32Mul(index, bytePerWord);
                // bitset_[index] |= mask;
                GateRef bitsetData = PtrAdd(oldToNewSet, IntPtr(RememberedSet::GCBITSET_DATA_OFFSET));
                GateRef oldsetValue = Load(VariableType::INT32(), bitsetData, byteIndex);
                GateRef newmapValue = Int32Or(oldsetValue, GetBitMask(bitOffset));

                Store(VariableType::INT32(), glue, bitsetData, byteIndex, newmapValue);
                Jump(&notValidIndex);
            }
            Bind(&isNullPtr);
            {
                CallNGCRuntime(glue,
                    RTSTUB_ID(InsertOldToNewRSet),
                    { glue, objectRegion, slotAddr });
                Jump(&notValidIndex);
            }
        }
        Bind(&notValidIndex);
        {
            Label marking(env);
            bool isArch32 = GetEnvironment()->Is32Bit();
            GateRef stateBitFieldAddr = Int64Add(glue,
                Int64(JSThread::GlueData::GetStateBitFieldOffset(isArch32)));
            GateRef stateBitField = Load(VariableType::INT64(), stateBitFieldAddr, Int64(0));
            Branch(Int64Equal(stateBitField, Int64(0)), &exit, &marking);

            Bind(&marking);
            CallNGCRuntime(glue,
                RTSTUB_ID(MarkingBarrier), {
                glue, slotAddr, objectRegion, TaggedCastToIntPtr(value), valueRegion });
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->SubCfgExit();
    return;
}

GateRef Stub::TaggedIsString(GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label isHeapObject(env);
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        result = Int32Equal(GetObjectType(LoadHClass(obj)),
                            Int32(static_cast<int32_t>(JSType::STRING)));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::TaggedIsStringOrSymbol(GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label isHeapObject(env);
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        GateRef objType = GetObjectType(LoadHClass(obj));
        result = Int32Equal(objType, Int32(static_cast<int32_t>(JSType::STRING)));
        Label isString(env);
        Label notString(env);
        Branch(*result, &exit, &notString);
        Bind(&notString);
        {
            result = Int32Equal(objType, Int32(static_cast<int32_t>(JSType::SYMBOL)));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::TaggedIsBigInt(GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label isHeapObject(env);
    Branch(TaggedIsHeapObject(obj), &isHeapObject, &exit);
    Bind(&isHeapObject);
    {
        result = Int32Equal(GetObjectType(LoadHClass(obj)),
                            Int32(static_cast<int32_t>(JSType::BIGINT)));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::IsUtf16String(GateRef string)
{
    // compressedStringsEnabled fixed to true constant
    GateRef len = Load(VariableType::INT32(), string, IntPtr(EcmaString::MIX_LENGTH_OFFSET));
    return Int32Equal(
        Int32And(len, Int32(EcmaString::STRING_COMPRESSED_BIT)),
        Int32(EcmaString::STRING_UNCOMPRESSED));
}

GateRef Stub::IsUtf8String(GateRef string)
{
    // compressedStringsEnabled fixed to true constant
    GateRef len = Load(VariableType::INT32(), string, IntPtr(EcmaString::MIX_LENGTH_OFFSET));
    return Int32Equal(
        Int32And(len, Int32(EcmaString::STRING_COMPRESSED_BIT)),
        Int32(EcmaString::STRING_COMPRESSED));
}

GateRef Stub::IsInternalString(GateRef string)
{
    // compressedStringsEnabled fixed to true constant
    GateRef len = Load(VariableType::INT32(), string, IntPtr(EcmaString::MIX_LENGTH_OFFSET));
    return Int32NotEqual(
        Int32And(len, Int32(EcmaString::STRING_INTERN_BIT)),
        Int32(0));
}

GateRef Stub::IsDigit(GateRef ch)
{
    return TruncInt32ToInt1(
        Int32And(SExtInt1ToInt32(Int32LessThanOrEqual(ch, Int32('9'))),
                 SExtInt1ToInt32(Int32GreaterThanOrEqual(ch, Int32('0')))));
}

GateRef Stub::StringToElementIndex(GateRef string)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::INT32(), Int32(-1));
    Label greatThanZero(env);
    Label inRange(env);
    auto len = GetLengthFromString(string);
    Branch(Int32Equal(len, Int32(0)), &exit, &greatThanZero);
    Bind(&greatThanZero);
    Branch(Int32GreaterThan(len, Int32(MAX_INDEX_LEN)), &exit, &inRange);
    Bind(&inRange);
    {
        GateRef dataUtf16 = PtrAdd(string, IntPtr(EcmaString::DATA_OFFSET));
        DEFVARIABLE(c, VariableType::INT32(), Int32(0));
        Label isUtf16(env);
        Label isUtf8(env);
        Label getChar1(env);
        GateRef isUtf16String = IsUtf16String(string);
        Branch(isUtf16String, &isUtf16, &isUtf8);
        Bind(&isUtf16);
        {
            c = ZExtInt16ToInt32(Load(VariableType::INT16(), dataUtf16));
            Jump(&getChar1);
        }
        Bind(&isUtf8);
        {
            c = ZExtInt8ToInt32(Load(VariableType::INT8(), dataUtf16));
            Jump(&getChar1);
        }
        Bind(&getChar1);
        {
            Label isDigitZero(env);
            Label notDigitZero(env);
            Branch(Int32Equal(*c, Int32('0')), &isDigitZero, &notDigitZero);
            Bind(&isDigitZero);
            {
                Label lengthIsOne(env);
                Branch(Int32Equal(len, Int32(1)), &lengthIsOne, &exit);
                Bind(&lengthIsOne);
                {
                    result = Int32(0);
                    Jump(&exit);
                }
            }
            Bind(&notDigitZero);
            {
                Label isDigit(env);
                DEFVARIABLE(i, VariableType::INT32(), Int32(1));
                DEFVARIABLE(n, VariableType::INT32(), Int32Sub(*c, Int32('0')));
                Branch(IsDigit(*c), &isDigit, &exit);
                Label loopHead(env);
                Label loopEnd(env);
                Label afterLoop(env);
                Bind(&isDigit);
                Branch(Int32UnsignedLessThan(*i, len), &loopHead, &afterLoop);
                LoopBegin(&loopHead);
                {
                    Label isUtf16A(env);
                    Label notUtf16(env);
                    Label getChar2(env);
                    Branch(isUtf16String, &isUtf16A, &notUtf16);
                    Bind(&isUtf16A);
                    {
                        // 2 : 2 means utf16 char width is two bytes
                        auto charOffset = PtrMul(ChangeInt32ToIntPtr(*i),  IntPtr(2));
                        c = ZExtInt16ToInt32(Load(VariableType::INT16(), dataUtf16, charOffset));
                        Jump(&getChar2);
                    }
                    Bind(&notUtf16);
                    {
                        c = ZExtInt8ToInt32(Load(VariableType::INT8(), dataUtf16, ChangeInt32ToIntPtr(*i)));
                        Jump(&getChar2);
                    }
                    Bind(&getChar2);
                    {
                        Label isDigit2(env);
                        Label notDigit2(env);
                        Branch(IsDigit(*c), &isDigit2, &notDigit2);
                        Bind(&isDigit2);
                        {
                            // 10 means the base of digit is 10.
                            n = Int32Add(Int32Mul(*n, Int32(10)),
                                         Int32Sub(*c, Int32('0')));
                            i = Int32Add(*i, Int32(1));
                            Branch(Int32UnsignedLessThan(*i, len), &loopEnd, &afterLoop);
                        }
                        Bind(&notDigit2);
                        Jump(&exit);
                    }
                }
                Bind(&loopEnd);
                LoopEnd(&loopHead);
                Bind(&afterLoop);
                {
                    Label lessThanMaxIndex(env);
                    Branch(Int32UnsignedLessThan(*n, Int32(JSObject::MAX_ELEMENT_INDEX)),
                           &lessThanMaxIndex, &exit);
                    Bind(&lessThanMaxIndex);
                    {
                        result = *n;
                        Jump(&exit);
                    }
                }
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::TryToElementsIndex(GateRef key)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isKeyInt(env);
    Label notKeyInt(env);

    DEFVARIABLE(resultKey, VariableType::INT32(), Int32(-1));
    Branch(TaggedIsInt(key), &isKeyInt, &notKeyInt);
    Bind(&isKeyInt);
    {
        resultKey = TaggedCastToInt32(key);
        Jump(&exit);
    }
    Bind(&notKeyInt);
    {
        Label isString(env);
        Label notString(env);
        Branch(TaggedIsString(key), &isString, &notString);
        Bind(&isString);
        {
            resultKey = StringToElementIndex(key);
            Jump(&exit);
        }
        Bind(&notString);
        {
            Label isDouble(env);
            Branch(TaggedIsDouble(key), &isDouble, &exit);
            Bind(&isDouble);
            {
                GateRef number = TaggedCastToDouble(key);
                GateRef integer = ChangeFloat64ToInt32(number);
                Label isEqual(env);
                Branch(DoubleEqual(number, ChangeInt32ToFloat64(integer)), &isEqual, &exit);
                Bind(&isEqual);
                {
                    resultKey = integer;
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&exit);
    auto ret = *resultKey;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::LoadFromField(GateRef receiver, GateRef handlerInfo)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label handlerInfoIsInlinedProps(env);
    Label handlerInfoNotInlinedProps(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    GateRef index = HandlerBaseGetOffset(handlerInfo);
    Branch(HandlerBaseIsInlinedProperty(handlerInfo), &handlerInfoIsInlinedProps, &handlerInfoNotInlinedProps);
    Bind(&handlerInfoIsInlinedProps);
    {
        result = Load(VariableType::JS_ANY(), receiver, PtrMul(ChangeInt32ToIntPtr(index),
            IntPtr(JSTaggedValue::TaggedTypeSize())));
        Jump(&exit);
    }
    Bind(&handlerInfoNotInlinedProps);
    {
        result = GetValueFromTaggedArray(VariableType::JS_ANY(), GetPropertiesArray(receiver), index);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::LoadGlobal(GateRef cell)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label cellIsInvalid(env);
    Label cellNotInvalid(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Branch(IsInvalidPropertyBox(cell), &cellIsInvalid, &cellNotInvalid);
    Bind(&cellIsInvalid);
    {
        Jump(&exit);
    }
    Bind(&cellNotInvalid);
    {
        result = GetValueFromPropertyBox(cell);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::CheckPolyHClass(GateRef cachedValue, GateRef hclass)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label iLessLength(env);
    Label hasHclass(env);
    Label cachedValueNotWeak(env);
    DEFVARIABLE(i, VariableType::INT32(), Int32(0));
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Branch(TaggedIsWeak(cachedValue), &exit, &cachedValueNotWeak);
    Bind(&cachedValueNotWeak);
    {
        GateRef length = GetLengthOfTaggedArray(cachedValue);
        Jump(&loopHead);
        LoopBegin(&loopHead);
        {
            Branch(Int32UnsignedLessThan(*i, length), &iLessLength, &exit);
            Bind(&iLessLength);
            {
                GateRef element = GetValueFromTaggedArray(VariableType::JS_ANY(), cachedValue, *i);
                Branch(Int64Equal(TaggedCastToWeakReferentUnChecked(element), hclass), &hasHclass, &loopEnd);
                Bind(&hasHclass);
                result = GetValueFromTaggedArray(VariableType::JS_ANY(), cachedValue,
                                                 Int32Add(*i, Int32(1)));
                Jump(&exit);
            }
            Bind(&loopEnd);
            i = Int32Add(*i, Int32(2));  // 2 means one ic, two slot
            LoopEnd(&loopHead);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::LoadICWithHandler(GateRef glue, GateRef receiver, GateRef argHolder, GateRef argHandler)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label handlerIsInt(env);
    Label handlerNotInt(env);
    Label handlerInfoIsField(env);
    Label handlerInfoNotField(env);
    Label handlerInfoIsNonExist(env);
    Label handlerInfoNotNonExist(env);
    Label handlerIsPrototypeHandler(env);
    Label handlerNotPrototypeHandler(env);
    Label cellHasChanged(env);
    Label loopHead(env);
    Label loopEnd(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());
    DEFVARIABLE(holder, VariableType::JS_ANY(), argHolder);
    DEFVARIABLE(handler, VariableType::JS_ANY(), argHandler);

    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(TaggedIsInt(*handler), &handlerIsInt, &handlerNotInt);
        Bind(&handlerIsInt);
        {
            GateRef handlerInfo = TaggedCastToInt32(*handler);
            Branch(IsField(handlerInfo), &handlerInfoIsField, &handlerInfoNotField);
            Bind(&handlerInfoIsField);
            {
                result = LoadFromField(*holder, handlerInfo);
                Jump(&exit);
            }
            Bind(&handlerInfoNotField);
            {
                Branch(IsNonExist(handlerInfo), &handlerInfoIsNonExist, &handlerInfoNotNonExist);
                Bind(&handlerInfoIsNonExist);
                Jump(&exit);
                Bind(&handlerInfoNotNonExist);
                GateRef accessor = LoadFromField(*holder, handlerInfo);
                result = CallGetterHelper(glue, receiver, *holder, accessor);
                Jump(&exit);
            }
        }
        Bind(&handlerNotInt);
        Branch(TaggedIsPrototypeHandler(*handler), &handlerIsPrototypeHandler, &handlerNotPrototypeHandler);
        Bind(&handlerIsPrototypeHandler);
        {
            GateRef cellValue = GetProtoCell(*handler);
            Branch(GetHasChanged(cellValue), &cellHasChanged, &loopEnd);
            Bind(&cellHasChanged);
            {
                result = Hole();
                Jump(&exit);
            }
            Bind(&loopEnd);
            holder = GetPrototypeHandlerHolder(*handler);
            handler = GetPrototypeHandlerHandlerInfo(*handler);
            LoopEnd(&loopHead);
        }
    }
    Bind(&handlerNotPrototypeHandler);
    result = LoadGlobal(*handler);
    Jump(&exit);

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::LoadElement(GateRef receiver, GateRef key)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label indexLessZero(env);
    Label indexNotLessZero(env);
    Label lengthLessIndex(env);
    Label lengthNotLessIndex(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    GateRef index = TryToElementsIndex(key);
    Branch(Int32UnsignedLessThan(index, Int32(0)), &indexLessZero, &indexNotLessZero);
    Bind(&indexLessZero);
    {
        Jump(&exit);
    }
    Bind(&indexNotLessZero);
    {
        GateRef elements = GetElementsArray(receiver);
        Branch(Int32LessThanOrEqual(GetLengthOfTaggedArray(elements), index), &lengthLessIndex, &lengthNotLessIndex);
        Bind(&lengthLessIndex);
        Jump(&exit);
        Bind(&lengthNotLessIndex);
        result = GetValueFromTaggedArray(VariableType::JS_ANY(), elements, index);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::ICStoreElement(GateRef glue, GateRef receiver, GateRef key, GateRef value, GateRef handler)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label indexLessZero(env);
    Label indexNotLessZero(env);
    Label handerInfoIsJSArray(env);
    Label handerInfoNotJSArray(env);
    Label indexGreaterLength(env);
    Label indexGreaterCapacity(env);
    Label callRuntime(env);
    Label storeElement(env);
    Label handlerIsInt(env);
    Label handlerNotInt(env);
    Label cellHasChanged(env);
    Label cellHasNotChanged(env);
    Label loopHead(env);
    Label loopEnd(env);
    DEFVARIABLE(result, VariableType::INT64(), Hole(VariableType::INT64()));
    DEFVARIABLE(varHandler, VariableType::JS_ANY(), handler);
    GateRef index = TryToElementsIndex(key);
    Branch(Int32UnsignedLessThan(index, Int32(0)), &indexLessZero, &indexNotLessZero);
    Bind(&indexLessZero);
    {
        Jump(&exit);
    }
    Bind(&indexNotLessZero);
    {
        Jump(&loopHead);
        LoopBegin(&loopHead);
        Branch(TaggedIsInt(*varHandler), &handlerIsInt, &handlerNotInt);
        Bind(&handlerIsInt);
        {
            GateRef handlerInfo = TaggedCastToInt32(*varHandler);
            Branch(HandlerBaseIsJSArray(handlerInfo), &handerInfoIsJSArray, &handerInfoNotJSArray);
            Bind(&handerInfoIsJSArray);
            {
                GateRef oldLength = GetArrayLength(receiver);
                Branch(Int32GreaterThanOrEqual(index, oldLength), &indexGreaterLength, &handerInfoNotJSArray);
                Bind(&indexGreaterLength);
                Store(VariableType::INT64(), glue, receiver,
                    IntPtr(panda::ecmascript::JSArray::LENGTH_OFFSET),
                    IntBuildTaggedWithNoGC(Int32Add(index, Int32(1))));
                Jump(&handerInfoNotJSArray);
            }
            Bind(&handerInfoNotJSArray);
            {
                GateRef elements = GetElementsArray(receiver);
                GateRef capacity = GetLengthOfTaggedArray(elements);
                Branch(Int32GreaterThanOrEqual(index, capacity), &callRuntime, &storeElement);
                Bind(&callRuntime);
                {
                    result = ChangeTaggedPointerToInt64(CallRuntime(glue,
                        RTSTUB_ID(TaggedArraySetValue),
                        { receiver, value, elements, IntBuildTaggedTypeWithNoGC(index),
                          IntBuildTaggedTypeWithNoGC(capacity) }));
                    Jump(&exit);
                }
                Bind(&storeElement);
                {
                    SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, index, value);
                    result = Undefined(VariableType::INT64());
                    Jump(&exit);
                }
            }
        }
        Bind(&handlerNotInt);
        {
            GateRef cellValue = GetProtoCell(*varHandler);
            Branch(GetHasChanged(cellValue), &cellHasChanged, &loopEnd);
            Bind(&cellHasChanged);
            {
                Jump(&exit);
            }
            Bind(&loopEnd);
            {
                varHandler = GetPrototypeHandlerHandlerInfo(*varHandler);
                LoopEnd(&loopHead);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::GetArrayLength(GateRef object)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label lengthIsInt(env);
    Label lengthNotInt(env);
    DEFVARIABLE(result, VariableType::INT32(), Int32(0));
    GateRef lengthOffset = IntPtr(panda::ecmascript::JSArray::LENGTH_OFFSET);
    GateRef length = Load(VariableType::INT64(), object, lengthOffset);
    Branch(TaggedIsInt(length), &lengthIsInt, &lengthNotInt);
    Bind(&lengthIsInt);
    {
        result = TaggedCastToInt32(length);
        Jump(&exit);
    }
    Bind(&lengthNotInt);
    {
        result = ChangeFloat64ToInt32(TaggedCastToDouble(length));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::StoreICWithHandler(GateRef glue, GateRef receiver, GateRef argHolder, GateRef value, GateRef argHandler)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label handlerIsInt(env);
    Label handlerNotInt(env);
    Label handlerInfoIsField(env);
    Label handlerInfoNotField(env);
    Label handlerIsTransitionHandler(env);
    Label handlerNotTransitionHandler(env);
    Label handlerIsPrototypeHandler(env);
    Label handlerNotPrototypeHandler(env);
    Label handlerIsPropertyBox(env);
    Label handlerNotPropertyBox(env);
    Label cellHasChanged(env);
    Label loopHead(env);
    Label loopEnd(env);
    DEFVARIABLE(result, VariableType::INT64(), Undefined(VariableType::INT64()));
    DEFVARIABLE(holder, VariableType::JS_ANY(), argHolder);
    DEFVARIABLE(handler, VariableType::JS_ANY(), argHandler);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        Branch(TaggedIsInt(*handler), &handlerIsInt, &handlerNotInt);
        Bind(&handlerIsInt);
        {
            GateRef handlerInfo = TaggedCastToInt32(*handler);
            Branch(IsField(handlerInfo), &handlerInfoIsField, &handlerInfoNotField);
            Bind(&handlerInfoIsField);
            {
                StoreField(glue, receiver, value, handlerInfo);
                Jump(&exit);
            }
            Bind(&handlerInfoNotField);
            {
                GateRef accessor = LoadFromField(*holder, handlerInfo);
                result = ChangeTaggedPointerToInt64(CallSetterHelper(glue, receiver, accessor, value));
                Jump(&exit);
            }
        }
        Bind(&handlerNotInt);
        {
            Branch(TaggedIsTransitionHandler(*handler), &handlerIsTransitionHandler, &handlerNotTransitionHandler);
            Bind(&handlerIsTransitionHandler);
            {
                StoreWithTransition(glue, receiver, value, *handler);
                Jump(&exit);
            }
            Bind(&handlerNotTransitionHandler);
            {
                Branch(TaggedIsPrototypeHandler(*handler), &handlerIsPrototypeHandler, &handlerNotPrototypeHandler);
                Bind(&handlerNotPrototypeHandler);
                {
                    Branch(TaggedIsPropertyBox(*handler), &handlerIsPropertyBox, &handlerNotPropertyBox);
                    Bind(&handlerIsPropertyBox);
                    StoreGlobal(glue, value, *handler);
                    Jump(&exit);
                    Bind(&handlerNotPropertyBox);
                    Jump(&exit);
                }
            }
        }
        Bind(&handlerIsPrototypeHandler);
        {
            GateRef cellValue = GetProtoCell(*handler);
            Branch(GetHasChanged(cellValue), &cellHasChanged, &loopEnd);
            Bind(&cellHasChanged);
            {
                result = Hole(VariableType::INT64());
                Jump(&exit);
            }
            Bind(&loopEnd);
            {
                holder = GetPrototypeHandlerHolder(*handler);
                handler = GetPrototypeHandlerHandlerInfo(*handler);
                LoopEnd(&loopHead);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void Stub::StoreField(GateRef glue, GateRef receiver, GateRef value, GateRef handler)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label handlerIsInlinedProperty(env);
    Label handlerNotInlinedProperty(env);
    GateRef index = HandlerBaseGetOffset(handler);
    Branch(HandlerBaseIsInlinedProperty(handler), &handlerIsInlinedProperty, &handlerNotInlinedProperty);
    Bind(&handlerIsInlinedProperty);
    {
        Store(VariableType::JS_ANY(), glue, receiver,
            PtrMul(ChangeInt32ToIntPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize())),
            value);
        Jump(&exit);
    }
    Bind(&handlerNotInlinedProperty);
    {
        GateRef array = GetPropertiesArray(receiver);
        SetValueToTaggedArray(VariableType::JS_ANY(), glue, array, index, value);
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

void Stub::StoreWithTransition(GateRef glue, GateRef receiver, GateRef value, GateRef handler)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    Label handlerInfoIsInlinedProps(env);
    Label handlerInfoNotInlinedProps(env);
    Label indexMoreCapacity(env);
    Label indexLessCapacity(env);
    GateRef newHClass = GetTransitionFromHClass(handler);
    StoreHClass(glue, receiver, newHClass);
    GateRef handlerInfo = TaggedCastToInt32(GetTransitionHandlerInfo(handler));
    Branch(HandlerBaseIsInlinedProperty(handlerInfo), &handlerInfoIsInlinedProps, &handlerInfoNotInlinedProps);
    Bind(&handlerInfoNotInlinedProps);
    {
        GateRef array = GetPropertiesArray(receiver);
        GateRef capacity = GetLengthOfTaggedArray(array);
        GateRef index = HandlerBaseGetOffset(handlerInfo);
        Branch(Int32GreaterThanOrEqual(index, capacity), &indexMoreCapacity, &indexLessCapacity);
        Bind(&indexMoreCapacity);
        {
            CallRuntime(glue,
                        RTSTUB_ID(PropertiesSetValue),
                        { receiver, value, array, IntBuildTaggedTypeWithNoGC(capacity),
                          IntBuildTaggedTypeWithNoGC(index) });
            Jump(&exit);
        }
        Bind(&indexLessCapacity);
        {
            Store(VariableType::JS_ANY(), glue, PtrAdd(array, IntPtr(TaggedArray::DATA_OFFSET)),
                PtrMul(ChangeInt32ToIntPtr(index), IntPtr(JSTaggedValue::TaggedTypeSize())),
                value);
            Jump(&exit);
        }
    }
    Bind(&handlerInfoIsInlinedProps);
    {
        StoreField(glue, receiver, value, handlerInfo);
        Jump(&exit);
    }
    Bind(&exit);
    env->SubCfgExit();
}

GateRef Stub::StoreGlobal(GateRef glue, GateRef value, GateRef cell)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label cellIsInvalid(env);
    Label cellNotInvalid(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Branch(IsInvalidPropertyBox(cell), &cellIsInvalid, &cellNotInvalid);
    Bind(&cellIsInvalid);
    {
        Jump(&exit);
    }
    Bind(&cellNotInvalid);
    {
        Store(VariableType::JS_ANY(), glue, cell, IntPtr(PropertyBox::VALUE_OFFSET), value);
        result = Undefined();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

template<typename DictionaryT>
GateRef Stub::GetAttributesFromDictionary(GateRef elements, GateRef entry)
{
    GateRef arrayIndex =
    Int32Add(Int32(DictionaryT::TABLE_HEADER_SIZE),
             Int32Mul(entry, Int32(DictionaryT::ENTRY_SIZE)));
    GateRef attributesIndex =
        Int32Add(arrayIndex, Int32(DictionaryT::ENTRY_DETAILS_INDEX));
    return TaggedCastToInt32(GetValueFromTaggedArray(VariableType::INT64(), elements, attributesIndex));
}

template<typename DictionaryT>
GateRef Stub::GetValueFromDictionary(VariableType returnType, GateRef elements, GateRef entry)
{
    GateRef arrayIndex =
        Int32Add(Int32(DictionaryT::TABLE_HEADER_SIZE),
                 Int32Mul(entry, Int32(DictionaryT::ENTRY_SIZE)));
    GateRef valueIndex =
        Int32Add(arrayIndex, Int32(DictionaryT::ENTRY_VALUE_INDEX));
    return GetValueFromTaggedArray(returnType, elements, valueIndex);
}

template<typename DictionaryT>
GateRef Stub::GetKeyFromDictionary(VariableType returnType, GateRef elements, GateRef entry)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    Label exit(env);
    DEFVARIABLE(result, returnType, Undefined());
    Label ltZero(env);
    Label notLtZero(env);
    Label gtLength(env);
    Label notGtLength(env);
    GateRef dictionaryLength =
        Load(VariableType::INT32(), elements, IntPtr(TaggedArray::LENGTH_OFFSET));
    GateRef arrayIndex =
        Int32Add(Int32(DictionaryT::TABLE_HEADER_SIZE),
                 Int32Mul(entry, Int32(DictionaryT::ENTRY_SIZE)));
    Branch(Int32LessThan(arrayIndex, Int32(0)), &ltZero, &notLtZero);
    Bind(&ltZero);
    Jump(&exit);
    Bind(&notLtZero);
    Branch(Int32GreaterThan(arrayIndex, dictionaryLength), &gtLength, &notGtLength);
    Bind(&gtLength);
    Jump(&exit);
    Bind(&notGtLength);
    result = GetValueFromTaggedArray(returnType, elements, arrayIndex);
    Jump(&exit);
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

inline void Stub::UpdateValueAndAttributes(GateRef glue, GateRef elements, GateRef index, GateRef value, GateRef attr)
{
    GateRef arrayIndex =
        Int32Add(Int32(NameDictionary::TABLE_HEADER_SIZE),
                 Int32Mul(index, Int32(NameDictionary::ENTRY_SIZE)));
    GateRef valueIndex =
        Int32Add(arrayIndex, Int32(NameDictionary::ENTRY_VALUE_INDEX));
    GateRef attributesIndex =
        Int32Add(arrayIndex, Int32(NameDictionary::ENTRY_DETAILS_INDEX));
    SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, valueIndex, value);
    GateRef attroffset =
        PtrMul(ChangeInt32ToIntPtr(attributesIndex), IntPtr(JSTaggedValue::TaggedTypeSize()));
    GateRef dataOffset = PtrAdd(attroffset, IntPtr(TaggedArray::DATA_OFFSET));
    Store(VariableType::INT64(), glue, elements, dataOffset, IntBuildTaggedWithNoGC(attr));
}

inline void Stub::UpdateValueInDict(GateRef glue, GateRef elements, GateRef index, GateRef value)
{
    GateRef arrayIndex = Int32Add(Int32(NameDictionary::TABLE_HEADER_SIZE),
        Int32Mul(index, Int32(NameDictionary::ENTRY_SIZE)));
    GateRef valueIndex = Int32Add(arrayIndex, Int32(NameDictionary::ENTRY_VALUE_INDEX));
    SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, valueIndex, value);
}

GateRef Stub::GetPropertyByIndex(GateRef glue, GateRef receiver, GateRef index)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(holder, VariableType::JS_ANY(), receiver);
    Label exit(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label afterLoop(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        GateRef hclass = LoadHClass(*holder);
        GateRef jsType = GetObjectType(hclass);
        Label isSpecialIndexed(env);
        Label notSpecialIndexed(env);
        Branch(IsSpecialIndexedObj(jsType), &isSpecialIndexed, &notSpecialIndexed);
        Bind(&isSpecialIndexed);
        {
            Label isSpecialContainer(env);
            Label notSpecialContainer(env);
            // Add SpecialContainer
            Branch(IsSpecialContainer(jsType), &isSpecialContainer, &notSpecialContainer);
            Bind(&isSpecialContainer);
            {
                result = GetContainerProperty(glue, *holder, index, jsType);
                Jump(&exit);
            }
            Bind(&notSpecialContainer);
            {
                result = Hole();
                Jump(&exit);
            }
        }
        Bind(&notSpecialIndexed);
        {
            GateRef elements = GetElementsArray(*holder);
            Label isDictionaryElement(env);
            Label notDictionaryElement(env);
            Branch(IsDictionaryElement(hclass), &isDictionaryElement, &notDictionaryElement);
            Bind(&notDictionaryElement);
            {
                Label lessThanLength(env);
                Label notLessThanLength(env);
                Branch(Int32UnsignedLessThan(index, GetLengthOfTaggedArray(elements)),
                       &lessThanLength, &notLessThanLength);
                Bind(&lessThanLength);
                {
                    Label notHole(env);
                    Label isHole(env);
                    GateRef value = GetValueFromTaggedArray(VariableType::JS_ANY(), elements, index);
                    Branch(TaggedIsNotHole(value), &notHole, &isHole);
                    Bind(&notHole);
                    {
                        result = value;
                        Jump(&exit);
                    }
                    Bind(&isHole);
                    {
                        Jump(&loopExit);
                    }
                }
                Bind(&notLessThanLength);
                {
                    result = Hole();
                    Jump(&exit);
                }
            }
            Bind(&isDictionaryElement);
            {
                GateRef entryA = FindElementFromNumberDictionary(glue, elements, index);
                Label notNegtiveOne(env);
                Label negtiveOne(env);
                Branch(Int32NotEqual(entryA, Int32(-1)), &notNegtiveOne, &negtiveOne);
                Bind(&notNegtiveOne);
                {
                    GateRef attr = GetAttributesFromDictionary<NumberDictionary>(elements, entryA);
                    GateRef value = GetValueFromDictionary<NumberDictionary>(VariableType::JS_ANY(), elements, entryA);
                    Label isAccessor(env);
                    Label notAccessor(env);
                    Branch(IsAccessor(attr), &isAccessor, &notAccessor);
                    Bind(&isAccessor);
                    {
                        result = CallGetterHelper(glue, receiver, *holder, value);
                        Jump(&exit);
                    }
                    Bind(&notAccessor);
                    {
                        result = value;
                        Jump(&exit);
                    }
                }
                Bind(&negtiveOne);
                Jump(&loopExit);
            }
            Bind(&loopExit);
            {
                holder = GetPrototypeFromHClass(LoadHClass(*holder));
                Branch(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
            }
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&afterLoop);
        {
            result = Undefined();
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::GetPropertyByName(GateRef glue, GateRef receiver, GateRef key)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(holder, VariableType::JS_ANY(), receiver);
    Label exit(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label afterLoop(env);
    Jump(&loopHead);
    LoopBegin(&loopHead);
    {
        GateRef hClass = LoadHClass(*holder);
        GateRef jsType = GetObjectType(hClass);
        Label isSIndexObj(env);
        Label notSIndexObj(env);
        Branch(IsSpecialIndexedObj(jsType), &isSIndexObj, &notSIndexObj);
        Bind(&isSIndexObj);
        {
            result = Hole();
            Jump(&exit);
        }
        Bind(&notSIndexObj);
        {
            Label isDicMode(env);
            Label notDicMode(env);
            Branch(IsDictionaryModeByHClass(hClass), &isDicMode, &notDicMode);
            Bind(&notDicMode);
            {
                GateRef layOutInfo = GetLayoutFromHClass(hClass);
                GateRef propsNum = GetNumberOfPropsFromHClass(hClass);
                // int entry = layoutInfo->FindElementWithCache(thread, hclass, key, propsNumber)
                GateRef entryA = FindElementWithCache(glue, layOutInfo, hClass, key, propsNum);
                Label hasEntry(env);
                Label noEntry(env);
                // if branch condition : entry != -1
                Branch(Int32NotEqual(entryA, Int32(-1)), &hasEntry, &noEntry);
                Bind(&hasEntry);
                {
                    // PropertyAttributes attr(layoutInfo->GetAttr(entry))
                    GateRef propAttr = GetPropAttrFromLayoutInfo(layOutInfo, entryA);
                    GateRef attr = TaggedCastToInt32(propAttr);
                    GateRef value = JSObjectGetProperty(VariableType::JS_ANY(), *holder, hClass, attr);
                    Label isAccessor(env);
                    Label notAccessor(env);
                    Branch(IsAccessor(attr), &isAccessor, &notAccessor);
                    Bind(&isAccessor);
                    {
                        result = CallGetterHelper(glue, receiver, *holder, value);
                        Jump(&exit);
                    }
                    Bind(&notAccessor);
                    {
                        result = value;
                        Jump(&exit);
                    }
                }
                Bind(&noEntry);
                {
                    Jump(&loopExit);
                }
            }
            Bind(&isDicMode);
            {
                GateRef array = GetPropertiesArray(*holder);
                // int entry = dict->FindEntry(key)
                GateRef entryB = FindEntryFromNameDictionary(glue, array, key);
                Label notNegtiveOne(env);
                Label negtiveOne(env);
                // if branch condition : entry != -1
                Branch(Int32NotEqual(entryB, Int32(-1)), &notNegtiveOne, &negtiveOne);
                Bind(&notNegtiveOne);
                {
                    // auto value = dict->GetValue(entry)
                    GateRef attr = GetAttributesFromDictionary<NameDictionary>(array, entryB);
                    // auto attr = dict->GetAttributes(entry)
                    GateRef value = GetValueFromDictionary<NameDictionary>(VariableType::JS_ANY(), array, entryB);
                    Label isAccessor1(env);
                    Label notAccessor1(env);
                    Branch(IsAccessor(attr), &isAccessor1, &notAccessor1);
                    Bind(&isAccessor1);
                    {
                        result = CallGetterHelper(glue, receiver, *holder, value);
                        Jump(&exit);
                    }
                    Bind(&notAccessor1);
                    {
                        result = value;
                        Jump(&exit);
                    }
                }
                Bind(&negtiveOne);
                Jump(&loopExit);
            }
            Bind(&loopExit);
            {
                holder = GetPrototypeFromHClass(LoadHClass(*holder));
                Branch(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
            }
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&afterLoop);
        {
            result = Undefined();
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void Stub::CopyAllHClass(GateRef glue, GateRef dstHClass, GateRef srcHClass)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    auto proto = GetPrototypeFromHClass(srcHClass);
    SetPrototypeToHClass(VariableType::JS_POINTER(), glue, dstHClass, proto);
    SetBitFieldToHClass(glue, dstHClass, GetBitFieldFromHClass(srcHClass));
    SetNumberOfPropsToHClass(glue, dstHClass, GetNumberOfPropsFromHClass(srcHClass));
    SetTransitionsToHClass(VariableType::INT64(), glue, dstHClass, Int64(JSTaggedValue::VALUE_UNDEFINED));
    SetProtoChangeDetailsToHClass(VariableType::INT64(), glue, dstHClass,
                                  Int64(JSTaggedValue::VALUE_NULL));
    SetEnumCacheToHClass(VariableType::INT64(), glue, dstHClass, Int64(JSTaggedValue::VALUE_NULL));
    SetLayoutToHClass(VariableType::JS_POINTER(), glue, dstHClass, GetLayoutFromHClass(srcHClass));
    env->SubCfgExit();
    return;
}

GateRef Stub::FindTransitions(GateRef glue, GateRef receiver, GateRef hclass, GateRef key, GateRef metaData)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    GateRef transitionOffset = IntPtr(JSHClass::TRANSTIONS_OFFSET);
    GateRef transition = Load(VariableType::JS_POINTER(), hclass, transitionOffset);
    DEFVARIABLE(result, VariableType::JS_ANY(), transition);

    Label notUndefined(env);
    Branch(Int64Equal(transition, Undefined()), &exit, &notUndefined);
    Bind(&notUndefined);
    {
        Label isWeak(env);
        Label notWeak(env);
        Branch(TaggedIsWeak(transition), &isWeak, &notWeak);
        Bind(&isWeak);
        {
            GateRef transitionHClass = TaggedCastToWeakReferentUnChecked(transition);
            GateRef propNums = GetNumberOfPropsFromHClass(transitionHClass);
            GateRef last = Int32Sub(propNums, Int32(1));
            GateRef layoutInfo = GetLayoutFromHClass(transitionHClass);
            GateRef cachedKey = GetKeyFromLayoutInfo(layoutInfo, last);
            GateRef cachedAttr = TaggedCastToInt32(GetPropAttrFromLayoutInfo(layoutInfo, last));
            GateRef cachedMetaData = GetPropertyMetaDataFromAttr(cachedAttr);
            Label keyMatch(env);
            Label isMatch(env);
            Label notMatch(env);
            Branch(Int64Equal(cachedKey, key), &keyMatch, &notMatch);
            Bind(&keyMatch);
            {
                Branch(Int32Equal(metaData, cachedMetaData), &isMatch, &notMatch);
                Bind(&isMatch);
                {
#if ECMASCRIPT_ENABLE_IC
                    NotifyHClassChanged(glue, hclass, transitionHClass);
#endif
                    StoreHClass(glue, receiver, transitionHClass);
                    Jump(&exit);
                }
            }
            Bind(&notMatch);
            {
                result = Undefined();
                Jump(&exit);
            }
        }
        Bind(&notWeak);
        {
            // need to find from dictionary
            GateRef entryA = FindEntryFromTransitionDictionary(glue, transition, key, metaData);
            Label isFound(env);
            Label notFound(env);
            Branch(Int32NotEqual(entryA, Int32(-1)), &isFound, &notFound);
            Bind(&isFound);
            auto value = GetValueFromDictionary<TransitionsDictionary>(
                VariableType::JS_POINTER(), transition, entryA);
            Label valueUndefined(env);
            Label valueNotUndefined(env);
            Branch(Int64NotEqual(value, Undefined()), &valueNotUndefined,
                &valueUndefined);
            Bind(&valueNotUndefined);
            {
                GateRef newHClass = TaggedCastToWeakReferentUnChecked(value);
                result = ChangeInt64ToTagged(newHClass);
#if ECMASCRIPT_ENABLE_IC
                NotifyHClassChanged(glue, hclass, newHClass);
#endif
                StoreHClass(glue, receiver, newHClass);
                Jump(&exit);
                Bind(&notFound);
                result = Undefined();
                Jump(&exit);
            }
            Bind(&valueUndefined);
            {
                result = Undefined();
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::SetPropertyByIndex(GateRef glue, GateRef receiver, GateRef index, GateRef value, bool useOwn)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(returnValue, VariableType::INT64(), Hole(VariableType::INT64()));
    DEFVARIABLE(holder, VariableType::JS_ANY(), receiver);
    Label exit(env);
    Label ifEnd(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label afterLoop(env);
    if (!useOwn) {
        Jump(&loopHead);
        LoopBegin(&loopHead);
    }
    GateRef hclass = LoadHClass(*holder);
    GateRef jsType = GetObjectType(hclass);
    Label isSpecialIndex(env);
    Label notSpecialIndex(env);
    Branch(IsSpecialIndexedObj(jsType), &isSpecialIndex, &notSpecialIndex);
    Bind(&isSpecialIndex);
    {
        returnValue = Hole(VariableType::INT64());
        Jump(&exit);
    }
    Bind(&notSpecialIndex);
    {
        GateRef elements = GetElementsArray(*holder);
        Label isDictionaryElement(env);
        Label notDictionaryElement(env);
        Branch(IsDictionaryElement(hclass), &isDictionaryElement, &notDictionaryElement);
        Bind(&notDictionaryElement);
        {
            Label isReceiver(env);
            if (useOwn) {
                Branch(Int64Equal(*holder, receiver), &isReceiver, &ifEnd);
            } else {
                Branch(Int64Equal(*holder, receiver), &isReceiver, &afterLoop);
            }
            Bind(&isReceiver);
            {
                GateRef length = GetLengthOfTaggedArray(elements);
                Label inRange(env);
                if (useOwn) {
                    Branch(Int64LessThan(index, length), &inRange, &ifEnd);
                } else {
                    Branch(Int64LessThan(index, length), &inRange, &loopExit);
                }
                Bind(&inRange);
                {
                    GateRef value1 = GetValueFromTaggedArray(VariableType::JS_ANY(), elements, index);
                    Label notHole(env);
                    if (useOwn) {
                        Branch(Int64NotEqual(value1, Hole()), &notHole, &ifEnd);
                    } else {
                        Branch(Int64NotEqual(value1, Hole()), &notHole, &loopExit);
                    }
                    Bind(&notHole);
                    {
                        SetValueToTaggedArray(VariableType::JS_ANY(), glue, elements, index, value);
                        returnValue = Undefined(VariableType::INT64());
                        Jump(&exit);
                    }
                }
            }
        }
        Bind(&isDictionaryElement);
        {
            returnValue = Hole(VariableType::INT64());
            Jump(&exit);
        }
    }
    if (useOwn) {
        Bind(&ifEnd);
    } else {
        Bind(&loopExit);
        {
            holder = GetPrototypeFromHClass(LoadHClass(*holder));
            Branch(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&afterLoop);
    }
    Label isExtensible(env);
    Label notExtensible(env);
    Branch(IsExtensible(receiver), &isExtensible, &notExtensible);
    Bind(&isExtensible);
    {
        GateRef result = CallRuntime(glue, RTSTUB_ID(AddElementInternal),
            { receiver, IntBuildTaggedTypeWithNoGC(index), value,
            IntBuildTaggedTypeWithNoGC(Int32(PropertyAttributes::GetDefaultAttributes())) });
        Label success(env);
        Label failed(env);
        Branch(TaggedIsTrue(result), &success, &failed);
        Bind(&success);
        {
            returnValue = Undefined(VariableType::INT64());
            Jump(&exit);
        }
        Bind(&failed);
        {
            returnValue = Exception(VariableType::INT64());
            Jump(&exit);
        }
    }
    Bind(&notExtensible);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensible));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntBuildTaggedTypeWithNoGC(taggedId) });
        returnValue = Exception(VariableType::INT64());
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *returnValue;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::SetPropertyByName(GateRef glue, GateRef receiver, GateRef key, GateRef value, bool useOwn)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::INT64(), Hole(VariableType::INT64()));
    DEFVARIABLE(holder, VariableType::JS_POINTER(), receiver);
    Label exit(env);
    Label ifEnd(env);
    Label loopHead(env);
    Label loopEnd(env);
    Label loopExit(env);
    Label afterLoop(env);
    if (!useOwn) {
        // a do-while loop
        Jump(&loopHead);
        LoopBegin(&loopHead);
    }
    // auto *hclass = holder.GetTaggedObject()->GetClass()
    // JSType jsType = hclass->GetObjectType()
    GateRef hClass = LoadHClass(*holder);
    GateRef jsType = GetObjectType(hClass);
    Label isSIndexObj(env);
    Label notSIndexObj(env);
    // if branch condition : IsSpecialIndexedObj(jsType)
    Branch(IsSpecialIndexedObj(jsType), &isSIndexObj, &notSIndexObj);
    Bind(&isSIndexObj);
    {
        Label isSpecialContainer(env);
        Label notSpecialContainer(env);
        // Add SpecialContainer
        Branch(IsSpecialContainer(jsType), &isSpecialContainer, &notSpecialContainer);
        Bind(&isSpecialContainer);
        {
            GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(CanNotSetPropertyOnContainer));
            CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntBuildTaggedTypeWithNoGC(taggedId) });
            result = Exception(VariableType::INT64());
            Jump(&exit);
        }
        Bind(&notSpecialContainer);
        {
            result = Hole(VariableType::INT64());
            Jump(&exit);
        }
    }
    Bind(&notSIndexObj);
    {
        Label isDicMode(env);
        Label notDicMode(env);
        // if branch condition : LIKELY(!hclass->IsDictionaryMode())
        Branch(IsDictionaryModeByHClass(hClass), &isDicMode, &notDicMode);
        Bind(&notDicMode);
        {
            // LayoutInfo *layoutInfo = LayoutInfo::Cast(hclass->GetAttributes().GetTaggedObject())
            GateRef layOutInfo = GetLayoutFromHClass(hClass);
            // int propsNumber = hclass->NumberOfPropsFromHClass()
            GateRef propsNum = GetNumberOfPropsFromHClass(hClass);
            // int entry = layoutInfo->FindElementWithCache(thread, hclass, key, propsNumber)
            GateRef entry = FindElementWithCache(glue, layOutInfo, hClass, key, propsNum);
            Label hasEntry(env);
            // if branch condition : entry != -1
            if (useOwn) {
                Branch(Int32NotEqual(entry, Int32(-1)), &hasEntry, &ifEnd);
            } else {
                Branch(Int32NotEqual(entry, Int32(-1)), &hasEntry, &loopExit);
            }
            Bind(&hasEntry);
            {
                // PropertyAttributes attr(layoutInfo->GetAttr(entry))
                GateRef propAttr = GetPropAttrFromLayoutInfo(layOutInfo, entry);
                GateRef attr = TaggedCastToInt32(propAttr);
                Label isAccessor(env);
                Label notAccessor(env);
                Branch(IsAccessor(attr), &isAccessor, &notAccessor);
                Bind(&isAccessor);
                {
                    // auto accessor = JSObject::Cast(holder)->GetProperty(hclass, attr)
                    GateRef accessor = JSObjectGetProperty(VariableType::JS_ANY(), *holder, hClass, attr);
                    Label shouldCall(env);
                    // ShouldCallSetter(receiver, *holder, accessor, attr)
                    Branch(ShouldCallSetter(receiver, *holder, accessor, attr), &shouldCall, &notAccessor);
                    Bind(&shouldCall);
                    {
                        result = ChangeTaggedPointerToInt64(CallSetterHelper(glue, receiver, accessor, value));
                        Jump(&exit);
                    }
                }
                Bind(&notAccessor);
                {
                    Label writable(env);
                    Label notWritable(env);
                    Branch(IsWritable(attr), &writable, &notWritable);
                    Bind(&notWritable);
                    {
                        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetReadOnlyProperty));
                        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntBuildTaggedTypeWithNoGC(taggedId) });
                        result = Exception(VariableType::INT64());
                        Jump(&exit);
                    }
                    Bind(&writable);
                    {
                        Label holdEqualsRecv(env);
                        if (useOwn) {
                            Branch(Int64Equal(*holder, receiver), &holdEqualsRecv, &ifEnd);
                        } else {
                            Branch(Int64Equal(*holder, receiver), &holdEqualsRecv, &afterLoop);
                        }
                        Bind(&holdEqualsRecv);
                        {
                            // JSObject::Cast(holder)->SetProperty(thread, hclass, attr, value)
                            // return JSTaggedValue::Undefined()
                            JSObjectSetProperty(glue, *holder, hClass, attr, value);
                            result = Undefined(VariableType::INT64());
                            Jump(&exit);
                        }
                    }
                }
            }
        }
        Bind(&isDicMode);
        {
            GateRef array = GetPropertiesArray(*holder);
            // int entry = dict->FindEntry(key)
            GateRef entry1 = FindEntryFromNameDictionary(glue, array, key);
            Label notNegtiveOne(env);
            // if branch condition : entry != -1
            if (useOwn) {
                Branch(Int32NotEqual(entry1, Int32(-1)), &notNegtiveOne, &ifEnd);
            } else {
                Branch(Int32NotEqual(entry1, Int32(-1)), &notNegtiveOne, &loopExit);
            }
            Bind(&notNegtiveOne);
            {
                // auto attr = dict->GetAttributes(entry)
                GateRef attr1 = GetAttributesFromDictionary<NameDictionary>(array, entry1);
                Label isAccessor1(env);
                Label notAccessor1(env);
                // if branch condition : UNLIKELY(attr.IsAccessor())
                Branch(IsAccessor(attr1), &isAccessor1, &notAccessor1);
                Bind(&isAccessor1);
                {
                    // auto accessor = dict->GetValue(entry)
                    GateRef accessor1 = GetValueFromDictionary<NameDictionary>(VariableType::JS_ANY(),
                        array, entry1);
                    Label shouldCall1(env);
                    Branch(ShouldCallSetter(receiver, *holder, accessor1, attr1), &shouldCall1, &notAccessor1);
                    Bind(&shouldCall1);
                    {
                        result = ChangeTaggedPointerToInt64(CallSetterHelper(glue, receiver, accessor1, value));
                        Jump(&exit);
                    }
                }
                Bind(&notAccessor1);
                {
                    Label writable1(env);
                    Label notWritable1(env);
                    Branch(IsWritable(attr1), &writable1, &notWritable1);
                    Bind(&notWritable1);
                    {
                        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetReadOnlyProperty));
                        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntBuildTaggedTypeWithNoGC(taggedId) });
                        result = Exception(VariableType::INT64());
                        Jump(&exit);
                    }
                    Bind(&writable1);
                    {
                        Label holdEqualsRecv1(env);
                        if (useOwn) {
                            Branch(Int64Equal(*holder, receiver), &holdEqualsRecv1, &ifEnd);
                        } else {
                            Branch(Int64Equal(*holder, receiver), &holdEqualsRecv1, &afterLoop);
                        }
                        Bind(&holdEqualsRecv1);
                        {
                            // dict->UpdateValue(thread, entry, value)
                            // return JSTaggedValue::Undefined()
                            UpdateValueInDict(glue, array, entry1, value);
                            result = Undefined(VariableType::INT64());
                            Jump(&exit);
                        }
                    }
                }
            }
        }
    }
    if (useOwn) {
        Bind(&ifEnd);
    } else {
        Bind(&loopExit);
        {
            // holder = hclass->GetPrototype()
            holder = GetPrototypeFromHClass(LoadHClass(*holder));
            // loop condition for a do-while loop
            Branch(TaggedIsHeapObject(*holder), &loopEnd, &afterLoop);
        }
        Bind(&loopEnd);
        LoopEnd(&loopHead);
        Bind(&afterLoop);
    }
    Label extensible(env);
    Label inextensible(env);
    Branch(IsExtensible(receiver), &extensible, &inextensible);
    Bind(&inextensible);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(SetPropertyWhenNotExtensible));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntBuildTaggedTypeWithNoGC(taggedId) });
        result = Exception(VariableType::INT64());
        Jump(&exit);
    }
    Bind(&extensible);
    {
        result = AddPropertyByName(glue, receiver, key, value,
            Int32(PropertyAttributes::GetDefaultAttributes()));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::SetPropertyByValue(GateRef glue, GateRef receiver, GateRef key, GateRef value, bool useOwn)
{
    auto env = GetEnvironment();
    Label subEntry1(env);
    env->SubCfgEntry(&subEntry1);
    DEFVARIABLE(varKey, VariableType::JS_ANY(), key);
    DEFVARIABLE(result, VariableType::INT64(), Hole(VariableType::INT64()));
    Label isNumberOrStringSymbol(env);
    Label notNumber(env);
    Label isStringOrSymbol(env);
    Label notStringOrSymbol(env);
    Label exit(env);
    Branch(TaggedIsNumber(*varKey), &isNumberOrStringSymbol, &notNumber);
    Bind(&notNumber);
    {
        Branch(TaggedIsStringOrSymbol(*varKey), &isNumberOrStringSymbol, &notStringOrSymbol);
        Bind(&notStringOrSymbol);
        {
            result = Hole(VariableType::INT64());
            Jump(&exit);
        }
    }
    Bind(&isNumberOrStringSymbol);
    {
        GateRef index = TryToElementsIndex(*varKey);
        Label validIndex(env);
        Label notValidIndex(env);
        Branch(Int32GreaterThanOrEqual(index, Int32(0)), &validIndex, &notValidIndex);
        Bind(&validIndex);
        {
            result = SetPropertyByIndex(glue, receiver, index, value, useOwn);
            Jump(&exit);
        }
        Bind(&notValidIndex);
        {
            Label isNumber1(env);
            Label notNumber1(env);
            Label setByName(env);
            Branch(TaggedIsNumber(*varKey), &isNumber1, &notNumber1);
            Bind(&isNumber1);
            {
                result = Hole(VariableType::INT64());
                Jump(&exit);
            }
            Bind(&notNumber1);
            {
                Label isString(env);
                Label notIntenalString(env);
                Branch(TaggedIsString(*varKey), &isString, &setByName);
                Bind(&isString);
                {
                    Branch(IsInternalString(*varKey), &setByName, &notIntenalString);
                    Bind(&notIntenalString);
                    {
                        varKey = CallRuntime(glue, RTSTUB_ID(NewInternalString), { *varKey });
                        Jump(&setByName);
                    }
                }
            }
            Bind(&setByName);
            {
                result = SetPropertyByName(glue, receiver, *varKey, value, useOwn);
                Jump(&exit);
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void Stub::NotifyHClassChanged(GateRef glue, GateRef oldHClass, GateRef newHClass)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label isProtoType(env);
    Branch(IsProtoTypeHClass(oldHClass), &isProtoType, &exit);
    Bind(&isProtoType);
    {
        Label notEqualHClass(env);
        Branch(Int64Equal(oldHClass, newHClass), &exit, &notEqualHClass);
        Bind(&notEqualHClass);
        {
            SetIsProtoTypeToHClass(glue, newHClass, True());
            CallRuntime(glue, RTSTUB_ID(NoticeThroughChainAndRefreshUser), { oldHClass, newHClass });
            Jump(&exit);
        }
    }
    Bind(&exit);
    env->SubCfgExit();
    return;
}

GateRef Stub::GetContainerProperty(GateRef glue, GateRef receiver, GateRef index, GateRef jsType)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    Label isDefaultLabel(env);
    Label noDefaultLabel(env);
    Branch(IsSpecialContainer(jsType), &noDefaultLabel, &isDefaultLabel);
    Bind(&noDefaultLabel);
    {
        result = JSAPIContainerGet(glue, receiver, index);
        Jump(&exit);
    }
    Bind(&isDefaultLabel);
    {
        Jump(&exit);
    }
    Bind(&exit);

    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::FastTypeOf(GateRef glue, GateRef obj)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    GateRef gConstAddr = PtrAdd(glue,
        IntPtr(JSThread::GlueData::GetGlobalConstOffset(env->Is32Bit())));
    GateRef undefinedIndex = GetGlobalConstantString(ConstantIndex::UNDEFINED_STRING_INDEX);
    GateRef gConstUndefinedStr = Load(VariableType::JS_POINTER(), gConstAddr, undefinedIndex);
    DEFVARIABLE(result, VariableType::JS_POINTER(), gConstUndefinedStr);
    Label objIsTrue(env);
    Label objNotTrue(env);
    Label defaultLabel(env);
    GateRef gConstBooleanStr = Load(VariableType::JS_POINTER(), gConstAddr,
        GetGlobalConstantString(ConstantIndex::BOOLEAN_STRING_INDEX));
    Branch(TaggedIsTrue(obj), &objIsTrue, &objNotTrue);
    Bind(&objIsTrue);
    {
        result = gConstBooleanStr;
        Jump(&exit);
    }
    Bind(&objNotTrue);
    {
        Label objIsFalse(env);
        Label objNotFalse(env);
        Branch(TaggedIsFalse(obj), &objIsFalse, &objNotFalse);
        Bind(&objIsFalse);
        {
            result = gConstBooleanStr;
            Jump(&exit);
        }
        Bind(&objNotFalse);
        {
            Label objIsNull(env);
            Label objNotNull(env);
            Branch(TaggedIsNull(obj), &objIsNull, &objNotNull);
            Bind(&objIsNull);
            {
                result = Load(VariableType::JS_POINTER(), gConstAddr,
                    GetGlobalConstantString(ConstantIndex::OBJECT_STRING_INDEX));
                Jump(&exit);
            }
            Bind(&objNotNull);
            {
                Label objIsUndefined(env);
                Label objNotUndefined(env);
                Branch(TaggedIsUndefined(obj), &objIsUndefined, &objNotUndefined);
                Bind(&objIsUndefined);
                {
                    result = Load(VariableType::JS_POINTER(), gConstAddr,
                        GetGlobalConstantString(ConstantIndex::UNDEFINED_STRING_INDEX));
                    Jump(&exit);
                }
                Bind(&objNotUndefined);
                Jump(&defaultLabel);
            }
        }
    }
    Bind(&defaultLabel);
    {
        Label objIsHeapObject(env);
        Label objNotHeapObject(env);
        Branch(TaggedIsHeapObject(obj), &objIsHeapObject, &objNotHeapObject);
        Bind(&objIsHeapObject);
        {
            Label objIsString(env);
            Label objNotString(env);
            Branch(IsString(obj), &objIsString, &objNotString);
            Bind(&objIsString);
            {
                result = Load(VariableType::JS_POINTER(), gConstAddr,
                    GetGlobalConstantString(ConstantIndex::STRING_STRING_INDEX));
                Jump(&exit);
            }
            Bind(&objNotString);
            {
                Label objIsSymbol(env);
                Label objNotSymbol(env);
                Branch(IsSymbol(obj), &objIsSymbol, &objNotSymbol);
                Bind(&objIsSymbol);
                {
                    result = Load(VariableType::JS_POINTER(), gConstAddr,
                        GetGlobalConstantString(ConstantIndex::SYMBOL_STRING_INDEX));
                    Jump(&exit);
                }
                Bind(&objNotSymbol);
                {
                    Label objIsCallable(env);
                    Label objNotCallable(env);
                    Branch(IsCallable(obj), &objIsCallable, &objNotCallable);
                    Bind(&objIsCallable);
                    {
                        result = Load(VariableType::JS_POINTER(), gConstAddr,
                            GetGlobalConstantString(ConstantIndex::FUNCTION_STRING_INDEX));
                        Jump(&exit);
                    }
                    Bind(&objNotCallable);
                    {
                        Label objIsBigInt(env);
                        Label objNotBigInt(env);
                        Branch(IsBigInt(obj), &objIsBigInt, &objNotBigInt);
                        Bind(&objIsBigInt);
                        {
                            result = Load(VariableType::JS_POINTER(), gConstAddr,
                                GetGlobalConstantString(ConstantIndex::BIGINT_STRING_INDEX));
                            Jump(&exit);
                        }
                        Bind(&objNotBigInt);
                        {
                            result = Load(VariableType::JS_POINTER(), gConstAddr,
                                GetGlobalConstantString(ConstantIndex::OBJECT_STRING_INDEX));
                            Jump(&exit);
                        }
                    }
                }
            }
        }
        Bind(&objNotHeapObject);
        {
            Label objIsNum(env);
            Label objNotNum(env);
            Branch(TaggedIsNumber(obj), &objIsNum, &objNotNum);
            Bind(&objIsNum);
            {
                result = Load(VariableType::JS_POINTER(), gConstAddr,
                    GetGlobalConstantString(ConstantIndex::NUMBER_STRING_INDEX));
                Jump(&exit);
            }
            Bind(&objNotNum);
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::FastStrictEqual(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::BOOL(), False());
    Label strictEqual(env);
    Label leftIsNumber(env);
    Label leftIsNotNumber(env);
    Label sameVariableCheck(env);
    Label stringEqualCheck(env);
    Label stringCompare(env);
    Label bigIntEqualCheck(env);
    Label exit(env);
    Branch(TaggedIsNumber(left), &leftIsNumber, &leftIsNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        Branch(TaggedIsNumber(right), &rightIsNumber, &exit);
        Bind(&rightIsNumber);
        {
            DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0.0));
            DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0.0));
            Label leftIsInt(env);
            Label leftNotInt(env);
            Label getRight(env);
            Label numberEqualCheck(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftNotInt);
            Bind(&leftIsInt);
            {
                doubleLeft = ChangeInt32ToFloat64(TaggedCastToInt32(left));
                Jump(&getRight);
            }
            Bind(&leftNotInt);
            {
                doubleLeft = TaggedCastToDouble(left);
                Jump(&getRight);
            }
            Bind(&getRight);
            {
                Label rightIsInt(env);
                Label rightNotInt(env);
                Branch(TaggedIsInt(right), &rightIsInt, &rightNotInt);
                Bind(&rightIsInt);
                {
                    doubleRight = ChangeInt32ToFloat64(TaggedCastToInt32(right));
                    Jump(&numberEqualCheck);
                }
                Bind(&rightNotInt);
                {
                    doubleRight = TaggedCastToDouble(right);
                    Jump(&numberEqualCheck);
                }
            }
            Bind(&numberEqualCheck);
            {
                Label doubleEqualCheck(env);
                Branch(BoolOr(DoubleIsNAN(*doubleLeft), DoubleIsNAN(*doubleRight)), &exit, &doubleEqualCheck);
                Bind(&doubleEqualCheck);
                {
                    result = DoubleEqual(*doubleLeft, *doubleRight);
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&leftIsNotNumber);
    Branch(TaggedIsNumber(right), &exit, &sameVariableCheck);
    Bind(&sameVariableCheck);
    Branch(Int64Equal(left, right), &strictEqual, &stringEqualCheck);
    Bind(&stringEqualCheck);
    Branch(BoolAnd(TaggedIsString(left), TaggedIsString(right)), &stringCompare, &bigIntEqualCheck);
    Bind(&stringCompare);
    {
        Label lengthCompare(env);
        Label hashcodeCompare(env);
        Label contentsCompare(env);
        Branch(Int32Equal(ZExtInt1ToInt32(IsUtf16String(left)), ZExtInt1ToInt32(IsUtf16String(right))),
            &lengthCompare, &exit);
        Bind(&lengthCompare);
        Branch(Int32Equal(GetLengthFromString(left), GetLengthFromString(right)), &hashcodeCompare,
            &exit);
        Bind(&hashcodeCompare);
        Branch(Int32Equal(GetHashcodeFromString(glue, left), GetHashcodeFromString(glue, right)), &contentsCompare,
            &exit);
        Bind(&contentsCompare);
        {
            result = CallNGCRuntime(glue, RTSTUB_ID(StringsAreEquals), { left, right });
            Jump(&exit);
        }
    }
    Bind(&bigIntEqualCheck);
    {
        Label leftIsBigInt(env);
        Label leftIsNotBigInt(env);
        Branch(TaggedIsBigInt(left), &leftIsBigInt, &exit);
        Bind(&leftIsBigInt);
        {
            Label rightIsBigInt(env);
            Branch(TaggedIsBigInt(right), &rightIsBigInt, &exit);
            Bind(&rightIsBigInt);
            result = CallNGCRuntime(glue, RTSTUB_ID(BigIntEquals), { left, right });
            Jump(&exit);
        }
    }
    Bind(&strictEqual);
    {
        result = True();
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::FastEqual(GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label leftEqualRight(env);
    Label leftNotEqualRight(env);
    Label exit(env);
    Branch(Int64Equal(left, right), &leftEqualRight, &leftNotEqualRight);
    Bind(&leftEqualRight);
    {
        Label leftIsDouble(env);
        Label leftNotDoubleOrLeftNotNan(env);
        Branch(TaggedIsDouble(left), &leftIsDouble, &leftNotDoubleOrLeftNotNan);
        Bind(&leftIsDouble);
        {
            GateRef doubleLeft = TaggedCastToDouble(left);
            Label leftIsNan(env);
            Branch(DoubleIsNAN(doubleLeft), &leftIsNan, &leftNotDoubleOrLeftNotNan);
            Bind(&leftIsNan);
            {
                result = ChangeInt64ToTagged(TaggedFalse());
                Jump(&exit);
            }
        }
        Bind(&leftNotDoubleOrLeftNotNan);
        {
            result = ChangeInt64ToTagged(TaggedTrue());
            Jump(&exit);
        }
    }
    Bind(&leftNotEqualRight);
    {
        Label leftIsNumber(env);
        Label leftNotNumberOrLeftNotIntOrRightNotInt(env);
        Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrLeftNotIntOrRightNotInt);
        Bind(&leftIsNumber);
        {
            Label leftIsInt(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftNotNumberOrLeftNotIntOrRightNotInt);
            Bind(&leftIsInt);
            {
                Label rightIsInt(env);
                Branch(TaggedIsInt(right), &rightIsInt, &leftNotNumberOrLeftNotIntOrRightNotInt);
                Bind(&rightIsInt);
                {
                    result = ChangeInt64ToTagged(TaggedFalse());
                    Jump(&exit);
                }
            }
        }
        Bind(&leftNotNumberOrLeftNotIntOrRightNotInt);
        {
            Label rightIsUndefinedOrNull(env);
            Label leftOrRightNotUndefinedOrNull(env);
            Branch(TaggedIsUndefinedOrNull(right), &rightIsUndefinedOrNull, &leftOrRightNotUndefinedOrNull);
            Bind(&rightIsUndefinedOrNull);
            {
                Label leftIsHeapObject(env);
                Label leftNotHeapObject(env);
                Branch(TaggedIsHeapObject(left), &leftIsHeapObject, &leftNotHeapObject);
                Bind(&leftIsHeapObject);
                {
                    result = ChangeInt64ToTagged(TaggedFalse());
                    Jump(&exit);
                }
                Bind(&leftNotHeapObject);
                {
                    Label leftIsUndefinedOrNull(env);
                    Branch(TaggedIsUndefinedOrNull(left), &leftIsUndefinedOrNull, &leftOrRightNotUndefinedOrNull);
                    Bind(&leftIsUndefinedOrNull);
                    {
                        result = ChangeInt64ToTagged(TaggedTrue());
                        Jump(&exit);
                    }
                }
            }
            Bind(&leftOrRightNotUndefinedOrNull);
            {
                Label leftIsBool(env);
                Label leftNotBoolOrRightNotSpecial(env);
                Branch(TaggedIsBoolean(left), &leftIsBool, &leftNotBoolOrRightNotSpecial);
                Bind(&leftIsBool);
                {
                    Label rightIsSpecial(env);
                    Branch(TaggedIsSpecial(right), &rightIsSpecial, &leftNotBoolOrRightNotSpecial);
                    Bind(&rightIsSpecial);
                    {
                        result = ChangeInt64ToTagged(TaggedFalse());
                        Jump(&exit);
                    }
                }
                Bind(&leftNotBoolOrRightNotSpecial);
                {
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::FastToBoolean(GateRef value)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    Label exit(env);

    Label isSpecial(env);
    Label notSpecial(env);
    Label isNumber(env);
    Label isInt(env);
    Label isDouble(env);
    Label notNumber(env);
    Label notNan(env);
    Label isString(env);
    Label notString(env);
    Label isBigint(env);
    Label lengthIsOne(env);
    Label returnTrue(env);
    Label returnFalse(env);

    Branch(TaggedIsSpecial(value), &isSpecial, &notSpecial);
    Bind(&isSpecial);
    {
        Branch(TaggedIsTrue(value), &returnTrue, &returnFalse);
    }
    Bind(&notSpecial);
    {
        Branch(TaggedIsNumber(value), &isNumber, &notNumber);
        Bind(&notNumber);
        {
            Branch(IsString(value), &isString, &notString);
            Bind(&isString);
            {
                auto len = GetLengthFromString(value);
                Branch(Int32Equal(len, Int32(0)), &returnFalse, &returnTrue);
            }
            Bind(&notString);
            Branch(IsBigInt(value), &isBigint, &returnTrue);
            Bind(&isBigint);
            {
                auto data = Load(VariableType::JS_POINTER(), value, IntPtr(BigInt::DATA_OFFSET));
                auto len = GetLengthOfTaggedArray(data);
                Branch(Int32Equal(len, Int32(1)), &lengthIsOne, &returnTrue);
                Bind(&lengthIsOne);
                {
                    auto data0 = GetValueFromTaggedArray(VariableType::JS_POINTER(), data, Int32(0));
                    Branch(Int64Equal(data0, Int64(JSTaggedValue::VALUE_ZERO)), &returnFalse, &returnTrue);
                }
            }
        }
        Bind(&isNumber);
        {
            Branch(TaggedIsInt(value), &isInt, &isDouble);
            Bind(&isInt);
            {
                auto intValue = TaggedCastToInt32(value);
                Branch(Int32Equal(intValue, Int32(0)), &returnFalse, &returnTrue);
            }
            Bind(&isDouble);
            {
                auto doubleValue = TaggedCastToDouble(value);
                Branch(DoubleIsNAN(doubleValue), &returnFalse, &notNan);
                Bind(&notNan);
                Branch(DoubleEqual(doubleValue, Double(0.0)), &returnFalse, &returnTrue);
            }
        }
    }
    Bind(&returnTrue);
    {
        result = ChangeInt64ToTagged(TaggedTrue());
        Jump(&exit);
    }
    Bind(&returnFalse);
    {
        result = ChangeInt64ToTagged(TaggedFalse());
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::FastDiv(GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
    DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
    Label leftIsNumber(env);
    Label leftNotNumberOrRightNotNumber(env);
    Label leftIsNumberAndRightIsNumber(env);
    Label leftIsDoubleAndRightIsDouble(env);
    Label exit(env);
    Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
    Bind(&leftIsNumber);
    {
        Label rightIsNumber(env);
        Branch(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftNotInt(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftNotInt);
            Bind(&leftIsInt);
            {
                doubleLeft = ChangeInt32ToFloat64(TaggedCastToInt32(left));
                Jump(&leftIsNumberAndRightIsNumber);
            }
            Bind(&leftNotInt);
            {
                doubleLeft = TaggedCastToDouble(left);
                Jump(&leftIsNumberAndRightIsNumber);
            }
        }
    }
    Bind(&leftNotNumberOrRightNotNumber);
    {
        Jump(&exit);
    }
    Bind(&leftIsNumberAndRightIsNumber);
    {
        Label rightIsInt(env);
        Label rightNotInt(env);
        Branch(TaggedIsInt(right), &rightIsInt, &rightNotInt);
        Bind(&rightIsInt);
        {
            doubleRight = ChangeInt32ToFloat64(TaggedCastToInt32(right));
            Jump(&leftIsDoubleAndRightIsDouble);
        }
        Bind(&rightNotInt);
        {
            doubleRight = TaggedCastToDouble(right);
            Jump(&leftIsDoubleAndRightIsDouble);
        }
    }
    Bind(&leftIsDoubleAndRightIsDouble);
    {
        Label rightIsZero(env);
        Label rightNotZero(env);
        Branch(DoubleEqual(*doubleRight, Double(0.0)), &rightIsZero, &rightNotZero);
        Bind(&rightIsZero);
        {
            Label leftIsZero(env);
            Label leftNotZero(env);
            Label leftIsZeroOrNan(env);
            Label leftNotZeroAndNotNan(env);
            Branch(DoubleEqual(*doubleLeft, Double(0.0)), &leftIsZero, &leftNotZero);
            Bind(&leftIsZero);
            {
                Jump(&leftIsZeroOrNan);
            }
            Bind(&leftNotZero);
            {
                Label leftIsNan(env);
                Branch(DoubleIsNAN(*doubleLeft), &leftIsNan, &leftNotZeroAndNotNan);
                Bind(&leftIsNan);
                {
                    Jump(&leftIsZeroOrNan);
                }
            }
            Bind(&leftIsZeroOrNan);
            {
                result = DoubleBuildTaggedWithNoGC(Double(base::NAN_VALUE));
                Jump(&exit);
            }
            Bind(&leftNotZeroAndNotNan);
            {
                GateRef intLeftTmp = CastDoubleToInt64(*doubleLeft);
                GateRef intRightTmp = CastDoubleToInt64(*doubleRight);
                GateRef flagBit = Int64And(Int64Xor(intLeftTmp, intRightTmp), Int64(base::DOUBLE_SIGN_MASK));
                GateRef tmpResult = Int64Xor(flagBit, CastDoubleToInt64(Double(base::POSITIVE_INFINITY)));
                result = DoubleBuildTaggedWithNoGC(CastInt64ToFloat64(tmpResult));
                Jump(&exit);
            }
        }
        Bind(&rightNotZero);
        {
            result = DoubleBuildTaggedWithNoGC(DoubleDiv(*doubleLeft, *doubleRight));
            Jump(&exit);
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::FastBinaryOp(GateRef left, GateRef right,
                           const BinaryOperation& intOp,
                           const BinaryOperation& floatOp)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
    DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));

    Label exit(env);
    Label doFloatOp(env);
    Label doIntOp(env);
    Label leftIsNumber(env);
    Label rightIsNumber(env);
    Label leftIsIntRightIsDouble(env);
    Label rightIsInt(env);
    Label rightIsDouble(env);

    Branch(TaggedIsNumber(left), &leftIsNumber, &exit);
    Bind(&leftIsNumber);
    {
        Branch(TaggedIsNumber(right), &rightIsNumber, &exit);
        Bind(&rightIsNumber);
        {
            Label leftIsInt(env);
            Label leftIsDouble(env);
            Branch(TaggedIsInt(left), &leftIsInt, &leftIsDouble);
            Bind(&leftIsInt);
            {
                Branch(TaggedIsInt(right), &doIntOp, &leftIsIntRightIsDouble);
                Bind(&leftIsIntRightIsDouble);
                {
                    doubleLeft = ChangeInt32ToFloat64(TaggedCastToInt32(left));
                    doubleRight = TaggedCastToDouble(right);
                    Jump(&doFloatOp);
                }
            }
            Bind(&leftIsDouble);
            {
                Branch(TaggedIsInt(right), &rightIsInt, &rightIsDouble);
                Bind(&rightIsInt);
                {
                    doubleLeft = TaggedCastToDouble(left);
                    doubleRight = ChangeInt32ToFloat64(TaggedCastToInt32(right));
                    Jump(&doFloatOp);
                }
                Bind(&rightIsDouble);
                {
                    doubleLeft = TaggedCastToDouble(left);
                    doubleRight = TaggedCastToDouble(right);
                    Jump(&doFloatOp);
                }
            }
        }
    }
    Bind(&doIntOp);
    {
        result = intOp(env, left, right);
        Jump(&exit);
    }
    Bind(&doFloatOp);
    {
        result = floatOp(env, *doubleLeft, *doubleRight);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

template<OpCode::Op Op>
GateRef Stub::FastAddSubAndMul(GateRef left, GateRef right)
{
    auto intOperation = [=](Environment *env, GateRef left, GateRef right) {
        Label entry(env);
        env->SubCfgEntry(&entry);
        DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
        Label exit(env);
        Label overflow(env);
        Label notOverflow(env);
        auto res = BinaryOp<Op, MachineType::I64>(TaggedCastToInt64(left), TaggedCastToInt64(right));
        auto condition1 = Int64GreaterThan(res, Int64(INT32_MAX));
        auto condition2 = Int64LessThan(res, Int64(INT32_MIN));
        Branch(BoolOr(condition1, condition2), &overflow, &notOverflow);
        Bind(&overflow);
        {
            auto doubleLeft = ChangeInt32ToFloat64(TaggedCastToInt32(left));
            auto doubleRight = ChangeInt32ToFloat64(TaggedCastToInt32(right));
            auto ret = BinaryOp<Op, MachineType::F64>(doubleLeft, doubleRight);
            result = DoubleBuildTaggedWithNoGC(ret);
            Jump(&exit);
        }
        Bind(&notOverflow);
        {
            result = IntBuildTaggedWithNoGC(ChangeInt64ToInt32(res));
            Jump(&exit);
        }
        Bind(&exit);
        auto ret = *result;
        env->SubCfgExit();
        return ret;
    };
    auto floatOperation = [=]([[maybe_unused]] Environment *env, GateRef left, GateRef right) {
        auto res = BinaryOp<Op, MachineType::F64>(left, right);
        return DoubleBuildTaggedWithNoGC(res);
    };
    return FastBinaryOp(left, right, intOperation, floatOperation);
}

GateRef Stub::FastAdd(GateRef left, GateRef right)
{
    return FastAddSubAndMul<OpCode::ADD>(left, right);
}

GateRef Stub::FastSub(GateRef left, GateRef right)
{
    return FastAddSubAndMul<OpCode::SUB>(left, right);
}

GateRef Stub::FastMul(GateRef left, GateRef right)
{
    return FastAddSubAndMul<OpCode::MUL>(left, right);
}

GateRef Stub::FastMod(GateRef glue, GateRef left, GateRef right)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    DEFVARIABLE(intLeft, VariableType::INT32(), Int32(0));
    DEFVARIABLE(intRight, VariableType::INT32(), Int32(0));
    DEFVARIABLE(doubleLeft, VariableType::FLOAT64(), Double(0));
    DEFVARIABLE(doubleRight, VariableType::FLOAT64(), Double(0));
    Label leftIsInt(env);
    Label leftNotIntOrRightNotInt(env);
    Label exit(env);
    Branch(TaggedIsInt(left), &leftIsInt, &leftNotIntOrRightNotInt);
    Bind(&leftIsInt);
    {
        Label rightIsInt(env);
        Branch(TaggedIsInt(right), &rightIsInt, &leftNotIntOrRightNotInt);
        Bind(&rightIsInt);
        {
            intLeft = TaggedCastToInt32(left);
            intRight = TaggedCastToInt32(right);
            Label leftGreaterZero(env);
            Branch(Int32GreaterThan(*intLeft, Int32(0)), &leftGreaterZero, &leftNotIntOrRightNotInt);
            Bind(&leftGreaterZero);
            {
                Label rightGreaterZero(env);
                Branch(Int32GreaterThan(*intRight, Int32(0)), &rightGreaterZero, &leftNotIntOrRightNotInt);
                Bind(&rightGreaterZero);
                {
                    result = IntBuildTaggedWithNoGC(Int32Mod(*intLeft, *intRight));
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&leftNotIntOrRightNotInt);
    {
        Label leftIsNumber(env);
        Label leftNotNumberOrRightNotNumber(env);
        Label leftIsNumberAndRightIsNumber(env);
        Label leftIsDoubleAndRightIsDouble(env);
        Branch(TaggedIsNumber(left), &leftIsNumber, &leftNotNumberOrRightNotNumber);
        Bind(&leftIsNumber);
        {
            Label rightIsNumber(env);
            Branch(TaggedIsNumber(right), &rightIsNumber, &leftNotNumberOrRightNotNumber);
            Bind(&rightIsNumber);
            {
                Label leftIsInt1(env);
                Label leftNotInt1(env);
                Branch(TaggedIsInt(left), &leftIsInt1, &leftNotInt1);
                Bind(&leftIsInt1);
                {
                    doubleLeft = ChangeInt32ToFloat64(TaggedCastToInt32(left));
                    Jump(&leftIsNumberAndRightIsNumber);
                }
                Bind(&leftNotInt1);
                {
                    doubleLeft = TaggedCastToDouble(left);
                    Jump(&leftIsNumberAndRightIsNumber);
                }
            }
        }
        Bind(&leftNotNumberOrRightNotNumber);
        {
            Jump(&exit);
        }
        Bind(&leftIsNumberAndRightIsNumber);
        {
            Label rightIsInt1(env);
            Label rightNotInt1(env);
            Branch(TaggedIsInt(right), &rightIsInt1, &rightNotInt1);
            Bind(&rightIsInt1);
            {
                doubleRight = ChangeInt32ToFloat64(TaggedCastToInt32(right));
                Jump(&leftIsDoubleAndRightIsDouble);
            }
            Bind(&rightNotInt1);
            {
                doubleRight = TaggedCastToDouble(right);
                Jump(&leftIsDoubleAndRightIsDouble);
            }
        }
        Bind(&leftIsDoubleAndRightIsDouble);
        {
            Label rightNotZero(env);
            Label rightIsZeroOrNanOrLeftIsNanOrInf(env);
            Label rightNotZeroAndNanAndLeftNotNanAndInf(env);
            Branch(DoubleEqual(*doubleRight, Double(0.0)), &rightIsZeroOrNanOrLeftIsNanOrInf, &rightNotZero);
            Bind(&rightNotZero);
            {
                Label rightNotNan(env);
                Branch(DoubleIsNAN(*doubleRight), &rightIsZeroOrNanOrLeftIsNanOrInf, &rightNotNan);
                Bind(&rightNotNan);
                {
                    Label leftNotNan(env);
                    Branch(DoubleIsNAN(*doubleLeft), &rightIsZeroOrNanOrLeftIsNanOrInf, &leftNotNan);
                    Bind(&leftNotNan);
                    {
                        Branch(DoubleIsINF(*doubleLeft), &rightIsZeroOrNanOrLeftIsNanOrInf,
                            &rightNotZeroAndNanAndLeftNotNanAndInf);
                    }
                }
            }
            Bind(&rightIsZeroOrNanOrLeftIsNanOrInf);
            {
                result = DoubleBuildTaggedWithNoGC(Double(base::NAN_VALUE));
                Jump(&exit);
            }
            Bind(&rightNotZeroAndNanAndLeftNotNanAndInf);
            {
                Label leftNotZero(env);
                Label leftIsZeroOrRightIsInf(env);
                Branch(DoubleEqual(*doubleLeft, Double(0.0)), &leftIsZeroOrRightIsInf, &leftNotZero);
                Bind(&leftNotZero);
                {
                    Label rightNotInf(env);
                    Branch(DoubleIsINF(*doubleRight), &leftIsZeroOrRightIsInf, &rightNotInf);
                    Bind(&rightNotInf);
                    {
                        result = CallNGCRuntime(glue, RTSTUB_ID(FloatMod), { *doubleLeft, *doubleRight });
                        Jump(&exit);
                    }
                }
                Bind(&leftIsZeroOrRightIsInf);
                {
                    result = DoubleBuildTaggedWithNoGC(*doubleLeft);
                    Jump(&exit);
                }
            }
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::GetGlobalOwnProperty(GateRef glue, GateRef receiver, GateRef key)
{
    auto env = GetEnvironment();
    Label entryLabel(env);
    env->SubCfgEntry(&entryLabel);
    DEFVARIABLE(result, VariableType::JS_ANY(), Hole());
    GateRef properties = GetPropertiesFromJSObject(receiver);
    GateRef entry = FindEntryFromNameDictionary(glue, properties, key);
    Label notNegtiveOne(env);
    Label exit(env);
    Branch(Int32NotEqual(entry, Int32(-1)), &notNegtiveOne, &exit);
    Bind(&notNegtiveOne);
    {
        result = GetValueFromGlobalDictionary(properties, entry);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::JSAPIContainerGet(GateRef glue, GateRef receiver, GateRef index)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    DEFVARIABLE(result, VariableType::JS_ANY(), Undefined());

    GateRef lengthOffset = IntPtr(panda::ecmascript::JSAPIArrayList::LENGTH_OFFSET);
    GateRef length = TaggedCastToInt32(Load(VariableType::INT64(), receiver, lengthOffset));
    Label isVailedIndex(env);
    Label notValidIndex(env);
    Branch(TruncInt32ToInt1(Int32And(ZExtInt1ToInt32(Int32GreaterThanOrEqual(index, Int32(0))),
        ZExtInt1ToInt32(Int32UnsignedLessThan(index, length)))), &isVailedIndex, &notValidIndex);
    Bind(&isVailedIndex);
    {
        GateRef elements = GetElementsArray(receiver);
        result = GetValueFromTaggedArray(VariableType::JS_ANY(), elements, index);
        Jump(&exit);
    }
    Bind(&notValidIndex);
    {
        GateRef taggedId = Int32(GET_MESSAGE_STRING_ID(GetPropertyOutOfBounds));
        CallRuntime(glue, RTSTUB_ID(ThrowTypeError), { IntBuildTaggedTypeWithNoGC(taggedId) });
        result = Exception();
        Jump(&exit);
    }

    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::DoubleToInt(GateRef glue, GateRef x)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label overflow(env);

    GateRef xInt = ChangeFloat64ToInt32(x);
    DEFVARIABLE(result, VariableType::INT32(), xInt);

    if (env->IsAmd64()) {
        // 0x80000000: amd64 overflow return value
        Branch(Int32Equal(xInt, Int32(0x80000000)), &overflow, &exit);
    } else {
        GateRef xInt64 = CastDoubleToInt64(x);
        // exp = (u64 & DOUBLE_EXPONENT_MASK) >> DOUBLE_SIGNIFICAND_SIZE - DOUBLE_EXPONENT_BIAS
        GateRef exp = Int64And(xInt64, Int64(base::DOUBLE_EXPONENT_MASK));
        exp = ChangeInt64ToInt32(Int64LSR(exp, Int64(base::DOUBLE_SIGNIFICAND_SIZE)));
        exp = Int32Sub(exp, Int32(base::DOUBLE_EXPONENT_BIAS));
        GateRef bits = Int32(base::INT32_BITS - 1);
        // exp < 32 - 1
        Branch(Int32LessThan(exp, bits), &exit, &overflow);
    }
    Bind(&overflow);
    {
        result = CallNGCRuntime(glue, RTSTUB_ID(DoubleToInt), { x });
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void Stub::ReturnExceptionIfAbruptCompletion(GateRef glue)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label hasPendingException(env);
    GateRef exception = Load(VariableType::JS_ANY(), glue);
    Branch(Int64NotEqual(exception, Int64(JSTaggedValue::VALUE_HOLE)), &hasPendingException, &exit);
    Bind(&hasPendingException);
    Return(Exception());
    Bind(&exit);
    env->SubCfgExit();
    return;
}

GateRef Stub::GetHashcodeFromString(GateRef glue, GateRef value)
{
    auto env = GetEnvironment();
    Label subentry(env);
    env->SubCfgEntry(&subentry);
    Label noRawHashcode(env);
    Label exit(env);
    DEFVARIABLE(hashcode, VariableType::INT32(), Int32(0));
    hashcode = Load(VariableType::INT32(), value, IntPtr(EcmaString::HASHCODE_OFFSET));
    Branch(Int32Equal(*hashcode, Int32(0)), &noRawHashcode, &exit);
    Bind(&noRawHashcode);
    {
        hashcode = TaggedCastToInt32(CallRuntime(glue, RTSTUB_ID(ComputeHashcode), { value }));
        Store(VariableType::INT32(), glue, value, IntPtr(EcmaString::HASHCODE_OFFSET), *hashcode);
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *hashcode;
    env->SubCfgExit();
    return ret;
}

GateRef Stub::AllocateInYoung(GateRef glue, GateRef size)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label success(env);
    Label callRuntime(env);

    auto topOffset = JSThread::GlueData::GetNewSpaceAllocationTopAddressOffset(env->Is32Bit());
    auto endOffset = JSThread::GlueData::GetNewSpaceAllocationEndAddressOffset(env->Is32Bit());
    auto topAddress = Load(VariableType::NATIVE_POINTER(), glue, IntPtr(topOffset));
    auto endAddress = Load(VariableType::NATIVE_POINTER(), glue, IntPtr(endOffset));
    auto top = Load(VariableType::NATIVE_POINTER(), topAddress, IntPtr(0));
    auto end = Load(VariableType::NATIVE_POINTER(), endAddress, IntPtr(0));
    DEFVARIABLE(result, VariableType::INT64(), Undefined());
    auto newTop = PtrAdd(top, size);
    Branch(IntPtrGreaterThan(newTop, end), &callRuntime, &success);
    Bind(&success);
    {
        Store(VariableType::NATIVE_POINTER(), glue, topAddress, IntPtr(0), newTop);
        if (env->Is32Bit()) {
            top = ZExtInt32ToInt64(top);
        }
        result = top;
        Jump(&exit);
    }
    Bind(&callRuntime);
    {
        result = TaggedCastToInt64(CallRuntime(glue, RTSTUB_ID(AllocateInYoung), {
            IntBuildTaggedTypeWithNoGC(size) }));
        Jump(&exit);
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}

void Stub::InitializeTaggedArrayWithSpeicalValue(
    GateRef glue, GateRef array, GateRef value, GateRef start, GateRef length)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);
    Label begin(env);
    Label storeValue(env);
    Label end(env);
    Store(VariableType::INT32(), glue, array, IntPtr(TaggedArray::LENGTH_OFFSET), length);
    DEFVARIABLE(i, VariableType::INT32(), start);
    Jump(&begin);
    LoopBegin(&begin);
    {
        Branch(Int32UnsignedLessThan(*i, length), &storeValue, &exit);
        Bind(&storeValue);
        {
            SetValueToTaggedArray(VariableType::INT64(), glue, array, *i, value);
            i = Int32Add(*i, Int32(1));
            Jump(&end);
        }
        Bind(&end);
        LoopEnd(&begin);
    }
    Bind(&exit);
    env->SubCfgExit();
}

GateRef Stub::NewLexicalEnv(GateRef glue, GateRef numSlots, GateRef parent)
{
    auto env = GetEnvironment();
    Label entry(env);
    env->SubCfgEntry(&entry);
    Label exit(env);

    auto length = Int32Add(numSlots, Int32(LexicalEnv::RESERVED_ENV_LENGTH));
    auto size = ComputeTaggedArraySize(ChangeInt32ToIntPtr(length));
    // Be careful. NO GC is allowed when initization is not complete.
    auto object = AllocateInYoung(glue, size);
    Label hasPendingException(env);
    Label noException(env);
    Branch(TaggedIsException(object), &hasPendingException, &noException);
    Bind(&noException);
    {
        auto hclass = GetGlobalConstantValue(
            VariableType::JS_POINTER(), glue, ConstantIndex::ENV_CLASS_INDEX);
        StoreHClass(glue, object, hclass);
        InitializeTaggedArrayWithSpeicalValue(glue,
            object, Hole(), Int32(LexicalEnv::RESERVED_ENV_LENGTH), length);
        SetValueToTaggedArray(VariableType::INT64(),
            glue, object, Int32(LexicalEnv::SCOPE_INFO_INDEX), Hole());
        SetValueToTaggedArray(VariableType::JS_POINTER(),
            glue, object, Int32(LexicalEnv::PARENT_ENV_INDEX), parent);
        Jump(&exit);
    }
    Bind(&hasPendingException);
    {
        Jump(&exit);
    }

    Bind(&exit);
    env->SubCfgExit();
    return ChangeInt64ToTagged(object);
}

GateRef Stub::JSCallDispatch(GateRef glue, GateRef func, GateRef actualNumArgs,
                             JSCallMode mode, std::initializer_list<GateRef> args)
{
    auto env = GetEnvironment();
    Label entryPass(env);
    Label exit(env);
    env->SubCfgEntry(&entryPass);
    DEFVARIABLE(result, VariableType::JS_ANY(), Exception());
    // 1. call initialize
    Label funcIsHeapObject(env);
    Label funcIsCallable(env);
    Label funcNotCallable(env);
    // save pc
    SavePcIfNeeded(glue);
    GateRef bitfield = 0;
    if (mode != JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV) {
        Branch(TaggedIsHeapObject(func), &funcIsHeapObject, &funcNotCallable);
        Bind(&funcIsHeapObject);
        GateRef hclass = LoadHClass(func);
        bitfield = Load(VariableType::INT32(), hclass, IntPtr(JSHClass::BIT_FIELD_OFFSET));
        Branch(IsCallableFromBitField(bitfield), &funcIsCallable, &funcNotCallable);
        Bind(&funcNotCallable);
        {
            CallRuntime(glue, RTSTUB_ID(ThrowNotCallableException), {});
            Jump(&exit);
        }
        Bind(&funcIsCallable);
    }
    GateRef method = GetMethodFromJSFunction(func);
    GateRef callField = GetCallFieldFromMethod(method);
    GateRef isNativeMask = Int64(static_cast<uint64_t>(1) << JSMethod::IsNativeBit::START_BIT);

    // 2. call dispatch
    Label methodIsNative(env);
    Label methodNotNative(env);
    Branch(Int64NotEqual(Int64And(callField, isNativeMask), Int64(0)), &methodIsNative, &methodNotNative);
    auto data = std::begin(args);
    // 3. call native
    Bind(&methodIsNative);
    {
        GateRef nativeCode = Load(VariableType::NATIVE_POINTER(), method,
            IntPtr(JSMethod::GetBytecodeArrayOffset(env->IsArch32Bit())));
        GateRef newTarget = Undefined();
        GateRef thisValue = Undefined();
        switch (mode) {
            case JSCallMode::CALL_ARG0:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgsAndDispatchNative),
                    { glue, nativeCode, actualNumArgs, func, newTarget, thisValue });
                break;
            case JSCallMode::CALL_ARG1:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgsAndDispatchNative),
                    { glue, nativeCode, actualNumArgs, func, newTarget, thisValue, data[0]});
                break;
            case JSCallMode::CALL_ARG2:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgsAndDispatchNative),
                    { glue, nativeCode, actualNumArgs, func, newTarget, thisValue, data[0], data[1] });
                break;
            case JSCallMode::CALL_ARG3:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgsAndDispatchNative),
                    { glue, nativeCode, actualNumArgs, func,
                      newTarget, thisValue, data[0], data[1], data[2] }); // 2: args2
                break;
            case JSCallMode::CALL_THIS_WITH_ARGV: {
                thisValue = data[2]; // 2: this input
                [[fallthrough]];
            }
            case JSCallMode::CALL_WITH_ARGV:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallIRangeAndDispatchNative),
                    { glue, nativeCode, func, thisValue, data[0], data[1] });
                break;
            case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallNewAndDispatchNative),
                    { glue, nativeCode, func, data[2], data[0], data[1] });
                break;
            case JSCallMode::CALL_GETTER:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgsAndDispatchNative),
                    { glue, nativeCode, actualNumArgs, func,
                      newTarget, data[0] });
                break;
            case JSCallMode::CALL_SETTER:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgsAndDispatchNative),
                    { glue, nativeCode, actualNumArgs, func,
                      newTarget, data[0], data[1] });
                break;
            default:
                UNREACHABLE();
        }
        Jump(&exit);
    }
    // 4. call nonNative
    Bind(&methodNotNative);
    if (mode != JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV) {
        Label funcIsClassConstructor(env);
        Label funcNotClassConstructor(env);
        Branch(IsClassConstructorFromBitField(bitfield), &funcIsClassConstructor, &funcNotClassConstructor);
        Bind(&funcIsClassConstructor);
        {
            CallRuntime(glue, RTSTUB_ID(ThrowCallConstructorException), {});
            Jump(&exit);
        }
        Bind(&funcNotClassConstructor);
    }
    GateRef sp = 0;
    if (env->IsAsmInterp()) {
        sp = PtrArgument(static_cast<size_t>(InterpreterHandlerInputs::SP));
    }
    {
        switch (mode) {
            case JSCallMode::CALL_ARG0:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgs0AndDispatch),
                    { glue, sp, func, method, callField });
                Return();
                break;
            case JSCallMode::CALL_ARG1:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgs1AndDispatch),
                    { glue, sp, func, method, callField, data[0] });
                Return();
                break;
            case JSCallMode::CALL_ARG2:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgs2AndDispatch),
                    { glue, sp, func, method, callField, data[0], data[1] });
                Return();
                break;
            case JSCallMode::CALL_ARG3:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallArgs3AndDispatch),
                    { glue, sp, func, method, callField, data[0], data[1], data[2] }); // 2: args2
                Return();
                break;
            case JSCallMode::CALL_WITH_ARGV:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallIRangeAndDispatch),
                    { glue, sp, func, method, callField, data[0], data[1] });
                Return();
                break;
            case JSCallMode::CALL_THIS_WITH_ARGV:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallIThisRangeAndDispatch),
                    { glue, sp, func, method, callField, data[0], data[1] });
                Return();
                break;
            case JSCallMode::CALL_CONSTRUCTOR_WITH_ARGV:
                result = CallNGCRuntime(glue, RTSTUB_ID(PushCallNewAndDispatch),
                    { glue, sp, func, method, callField, data[0], data[1], data[2] });
                Return();
                break;
            case JSCallMode::CALL_GETTER:
                result = CallNGCRuntime(glue, RTSTUB_ID(CallGetter),
                    { glue, func, method, callField, data[0] });
                Jump(&exit);
                break;
            case JSCallMode::CALL_SETTER:
                result = CallNGCRuntime(glue, RTSTUB_ID(CallSetter),
                    { glue, func, method, callField, data[1], data[0] });
                Jump(&exit);
                break;
            default:
                UNREACHABLE();
        }
    }
    Bind(&exit);
    auto ret = *result;
    env->SubCfgExit();
    return ret;
}
}  // namespace panda::ecmascript::kungfu
