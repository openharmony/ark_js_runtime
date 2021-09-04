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

#include "intrinsics.h"

namespace panda::ecmascript::intrinsics {
DecodedTaggedValue Ldnan()
{
    UNREACHABLE();
}

DecodedTaggedValue Ldinfinity()
{
    UNREACHABLE();
}

DecodedTaggedValue Ldglobalthis()
{
    UNREACHABLE();
}

DecodedTaggedValue Ldundefined()
{
    UNREACHABLE();
}

DecodedTaggedValue Ldboolean([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0)
{
    UNREACHABLE();
}

DecodedTaggedValue Ldnumber([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0)
{
    UNREACHABLE();
}

DecodedTaggedValue Ldstring([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0)
{
    UNREACHABLE();
}

DecodedTaggedValue Ldbigint([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0)
{
    UNREACHABLE();
}

DecodedTaggedValue Ldnull()
{
    UNREACHABLE();
}

DecodedTaggedValue Ldsymbol()
{
    UNREACHABLE();
}

DecodedTaggedValue Ldobject([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                            [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue Ldfunction([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                              [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue Ldglobal()
{
    UNREACHABLE();
}

DecodedTaggedValue Ldtrue()
{
    UNREACHABLE();
}

DecodedTaggedValue Ldfalse()
{
    UNREACHABLE();
}

DecodedTaggedValue Add2Dyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                           [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue Sub2Dyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                           [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

// 12.6.3 Runtime Semantics: Evaluation
DecodedTaggedValue Mul2Dyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                           [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

// 12.6.3 Runtime Semantics: Evaluation
DecodedTaggedValue Div2Dyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                           [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

// 12.6.3 Runtime Semantics: Evaluation
DecodedTaggedValue Mod2Dyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                           [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue ExpDyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                          [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue EqDyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                         [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue NotEqDyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                            [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue StrictEqDyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                               [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue StrictNotEqDyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                                  [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue LessDyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                           [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue LessEqDyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                             [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue GreaterDyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                              [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue GreaterEqDyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                                [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue LdObjByValue([[maybe_unused]] uint16_t slotId, [[maybe_unused]] int64_t objValue,
                                [[maybe_unused]] int64_t objTag, [[maybe_unused]] int64_t propValue,
                                [[maybe_unused]] int64_t propTag)
{
    UNREACHABLE();
}

void StObjByValue([[maybe_unused]] uint16_t slotId, [[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag,
                  [[maybe_unused]] int64_t propValue, [[maybe_unused]] int64_t propTag,
                  [[maybe_unused]] int64_t valValue, [[maybe_unused]] int64_t valTag)
{
    UNREACHABLE();
}

DecodedTaggedValue TryLdGlobalByValue([[maybe_unused]] uint16_t slotId, [[maybe_unused]] int64_t propValue,
                                      [[maybe_unused]] int64_t propTag)
{
    UNREACHABLE();
}

void TryStGlobalByValue([[maybe_unused]] uint16_t slotId, [[maybe_unused]] int64_t propValue,
                        [[maybe_unused]] int64_t propTag, [[maybe_unused]] int64_t valValue,
                        [[maybe_unused]] int64_t valTag)
{
    UNREACHABLE();
}

DecodedTaggedValue TryLdGlobalByName([[maybe_unused]] uint32_t stringId, [[maybe_unused]] uint16_t slotId)
{
    UNREACHABLE();
}

void TryStGlobalByName([[maybe_unused]] uint32_t stringId, [[maybe_unused]] uint16_t slotId,
                       [[maybe_unused]] int64_t valValue, [[maybe_unused]] int64_t valTag)
{
    UNREACHABLE();
}

DecodedTaggedValue LdGlobalVar([[maybe_unused]] uint32_t stringId, [[maybe_unused]] uint16_t slotId)
{
    UNREACHABLE();
}

void StGlobalVar([[maybe_unused]] uint32_t stringId, [[maybe_unused]] uint16_t slotId,
                 [[maybe_unused]] int64_t valValue, [[maybe_unused]] int64_t valTag)
{
    UNREACHABLE();
}

DecodedTaggedValue LdObjByName([[maybe_unused]] uint32_t stringId, [[maybe_unused]] uint16_t slotId,
                               [[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

void StObjByName([[maybe_unused]] uint32_t stringId, [[maybe_unused]] uint16_t slotId,
                 [[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag, [[maybe_unused]] int64_t valValue,
                 [[maybe_unused]] int64_t valTag)
{
    UNREACHABLE();
}

DecodedTaggedValue LdObjByIndex([[maybe_unused]] uint16_t slotId, [[maybe_unused]] int64_t objValue,
                                [[maybe_unused]] int64_t objTag, [[maybe_unused]] int64_t idxValue,
                                [[maybe_unused]] int64_t idxTag)
{
    UNREACHABLE();
}

void StObjByIndex([[maybe_unused]] uint16_t slotId, [[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag,
                  [[maybe_unused]] int64_t idxValue, [[maybe_unused]] int64_t idxTag, [[maybe_unused]] int64_t valValue,
                  [[maybe_unused]] int64_t valTag)
{
    UNREACHABLE();
}

DecodedTaggedValue And2Dyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                           [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue Or2Dyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                          [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue Xor2Dyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                           [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue Shl2Dyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                           [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue Shr2Dyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                           [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue Ashr2Dyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                            [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue Tonumber([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0)
{
    UNREACHABLE();
}

DecodedTaggedValue NegDyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0)
{
    UNREACHABLE();
}

DecodedTaggedValue NotDyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0)
{
    UNREACHABLE();
}

DecodedTaggedValue IncDyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0)
{
    UNREACHABLE();
}

DecodedTaggedValue DecDyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0)
{
    UNREACHABLE();
}

void ThrowDyn([[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

DecodedTaggedValue Delobjprop([[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag,
                              [[maybe_unused]] int64_t propValue, [[maybe_unused]] int64_t propTag)
{
    UNREACHABLE();
}

DecodedTaggedValue Defineglobalvar([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0,
                                   [[maybe_unused]] int64_t a1, [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue Definelocalvar([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                                  [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue Definefuncexpr([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                                  [[maybe_unused]] int64_t t1)
{
    return DecodedTaggedValue(0, 0);
}

DecodedTaggedValue DefinefuncDyn([[maybe_unused]] uint32_t methodId, [[maybe_unused]] int64_t a1,
                                 [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

// define not constructor function
DecodedTaggedValue DefineNCFuncDyn([[maybe_unused]] uint32_t methodId, [[maybe_unused]] int64_t a1,
                                   [[maybe_unused]] int64_t t1, [[maybe_unused]] int64_t a2,
                                   [[maybe_unused]] int64_t t2)
{
    UNREACHABLE();
}

// a0 is args' length which include ctor and new_target, and a1 is the start index of args
DecodedTaggedValue NewobjDynrange([[maybe_unused]] uint16_t numArgs, [[maybe_unused]] int64_t a1,
                                  [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue RefeqDyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                            [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue TypeofDyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0)
{
    UNREACHABLE();
}

DecodedTaggedValue Callruntimerange([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0,
                                    [[maybe_unused]] int64_t a1, [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue LdLexVarDyn([[maybe_unused]] uint16_t level, [[maybe_unused]] uint16_t slot)
{
    UNREACHABLE();
}

DecodedTaggedValue LdlexenvDyn()
{
    UNREACHABLE();
}

DecodedTaggedValue PopLexenvDyn()
{
    UNREACHABLE();
}

DecodedTaggedValue IsinDyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                           [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue InstanceofDyn([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0, [[maybe_unused]] int64_t a1,
                                 [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue NewobjspreadDyn([[maybe_unused]] int64_t funcValue, [[maybe_unused]] int64_t funcTag,
                                   [[maybe_unused]] int64_t targetValue, [[maybe_unused]] int64_t targetTag,
                                   [[maybe_unused]] int64_t arrValue, [[maybe_unused]] int64_t arrTag)
{
    UNREACHABLE();
}

DecodedTaggedValue CallspreadDyn([[maybe_unused]] int64_t funcValue, [[maybe_unused]] int64_t funcTag,
                                 [[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag,
                                 [[maybe_unused]] int64_t arrValue, [[maybe_unused]] int64_t arrTag)
{
    UNREACHABLE();
}

DecodedTaggedValue NewlexenvDyn([[maybe_unused]] uint16_t numSlot)
{
    UNREACHABLE();
}

void StLexVarDyn([[maybe_unused]] uint16_t level, [[maybe_unused]] uint16_t slot, [[maybe_unused]] int64_t valueValue,
                 [[maybe_unused]] int64_t valueTag)
{
    UNREACHABLE();
}

DecodedTaggedValue GetUnmappedArgs()
{
    UNREACHABLE();
}

DecodedTaggedValue Toboolean([[maybe_unused]] int64_t value, [[maybe_unused]] int64_t tag)
{
    UNREACHABLE();
}

DecodedTaggedValue GetPropIterator([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0)
{
    UNREACHABLE();
}

DecodedTaggedValue DefineGeneratorFunc([[maybe_unused]] uint32_t methodId, [[maybe_unused]] int64_t a1,
                                       [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue CreateIterResultObj([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0,
                                       [[maybe_unused]] int64_t a1, [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue SuspendGenerator([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0,
                                    [[maybe_unused]] int64_t a1, [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue ResumeGenerator([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0)
{
    UNREACHABLE();
}

DecodedTaggedValue GetResumeMode([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0)
{
    UNREACHABLE();
}

DecodedTaggedValue CreateGeneratorObj([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0)
{
    UNREACHABLE();
}

DecodedTaggedValue DefineAsyncFunc([[maybe_unused]] uint32_t methodId, [[maybe_unused]] int64_t a1,
                                   [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue AsyncFunctionEnter()
{
    UNREACHABLE();
}

DecodedTaggedValue AsyncFunctionAwaitUncaught([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0,
                                              [[maybe_unused]] int64_t a1, [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue AsyncFunctionResolve([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0,
                                        [[maybe_unused]] int64_t a1, [[maybe_unused]] int64_t t1,
                                        [[maybe_unused]] int64_t a2, [[maybe_unused]] int64_t t2)
{
    UNREACHABLE();
}

DecodedTaggedValue AsyncFunctionReject([[maybe_unused]] int64_t a0, [[maybe_unused]] int64_t t0,
                                       [[maybe_unused]] int64_t a1, [[maybe_unused]] int64_t t1,
                                       [[maybe_unused]] int64_t a2, [[maybe_unused]] int64_t t2)
{
    UNREACHABLE();
}

void ThrowUndefined([[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

void ThrowConstAssignment([[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

void ThrowUndefinedIfHole([[maybe_unused]] int64_t accValue, [[maybe_unused]] int64_t accTag,
                          [[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

DecodedTaggedValue Copyrestargs([[maybe_unused]] uint16_t index)
{
    UNREACHABLE();
}

DecodedTaggedValue LdHole()
{
    UNREACHABLE();
}

DecodedTaggedValue GetTemplateObject([[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

DecodedTaggedValue GetNextPropName([[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

void ReturnUndefined()
{
    UNREACHABLE();
}

DecodedTaggedValue CallArg0Dyn([[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

DecodedTaggedValue CallArg1Dyn([[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag,
                               [[maybe_unused]] int64_t arg0Value, [[maybe_unused]] int64_t arg0Tag)
{
    UNREACHABLE();
}

DecodedTaggedValue CallArgs2Dyn([[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag,
                                [[maybe_unused]] int64_t arg0Value, [[maybe_unused]] int64_t arg0Tag,
                                [[maybe_unused]] int64_t arg1Value, [[maybe_unused]] int64_t arg1Tag)
{
    UNREACHABLE();
}

DecodedTaggedValue CallArgs3Dyn([[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag,
                                [[maybe_unused]] int64_t arg0Value, [[maybe_unused]] int64_t arg0Tag,
                                [[maybe_unused]] int64_t arg1Value, [[maybe_unused]] int64_t arg1Tag,
                                [[maybe_unused]] int64_t arg2Value, [[maybe_unused]] int64_t arg2Vag)
{
    UNREACHABLE();
}

DecodedTaggedValue CalliRangeDyn([[maybe_unused]] uint16_t num, [[maybe_unused]] int64_t objValue,
                                 [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

DecodedTaggedValue CalliThisRangeDyn([[maybe_unused]] uint16_t num, [[maybe_unused]] int64_t objValue,
                                     [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

DecodedTaggedValue CreateEmptyObject()
{
    UNREACHABLE();
}

DecodedTaggedValue CreateObjectWithBuffer([[maybe_unused]] uint16_t index)
{
    UNREACHABLE();
}

void SetObjectWithProto([[maybe_unused]] int64_t protoValue, [[maybe_unused]] int64_t protoTag,
                        [[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

DecodedTaggedValue CopyDataProperties([[maybe_unused]] int64_t dstValue, [[maybe_unused]] int64_t dstTag,
                                      [[maybe_unused]] int64_t srcValue, [[maybe_unused]] int64_t srcTag)
{
    UNREACHABLE();
}

DecodedTaggedValue DefineGetterSetterByValue([[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag,
                                             [[maybe_unused]] int64_t propValue, [[maybe_unused]] int64_t propTag,
                                             [[maybe_unused]] int64_t getterValue, [[maybe_unused]] int64_t getterTag,
                                             [[maybe_unused]] int64_t setterValue, [[maybe_unused]] int64_t setterTag,
                                             [[maybe_unused]] int64_t flagValue, [[maybe_unused]] int64_t flagTag)
{
    UNREACHABLE();
}

DecodedTaggedValue CreateEmptyArray()
{
    UNREACHABLE();
}

DecodedTaggedValue CreateArrayWithBuffer([[maybe_unused]] uint16_t index)
{
    UNREACHABLE();
}

void StOwnByIndex([[maybe_unused]] uint16_t slotId, [[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag,
                  [[maybe_unused]] int64_t idxValue, [[maybe_unused]] int64_t idxTag, [[maybe_unused]] int64_t valValue,
                  [[maybe_unused]] int64_t valTag)
{
    UNREACHABLE();
}

void StOwnByName([[maybe_unused]] uint32_t stringId, [[maybe_unused]] uint16_t slotId,
                 [[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag, [[maybe_unused]] int64_t valValue,
                 [[maybe_unused]] int64_t valTag)
{
    UNREACHABLE();
}

void StOwnByValue([[maybe_unused]] uint16_t slotId, [[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag,
                  [[maybe_unused]] int64_t propValue, [[maybe_unused]] int64_t propTag,
                  [[maybe_unused]] int64_t valValue, [[maybe_unused]] int64_t valTag)
{
    UNREACHABLE();
}

DecodedTaggedValue StArraySpread([[maybe_unused]] int64_t dstValue, [[maybe_unused]] int64_t dstTag,
                                 [[maybe_unused]] int64_t indexValue, [[maybe_unused]] int64_t indexTag,
                                 [[maybe_unused]] int64_t srcValue, [[maybe_unused]] int64_t srcTag)
{
    UNREACHABLE();
}

DecodedTaggedValue GetIterator([[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

void ThrowIfNotObject([[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

void ThrowThrowNotExists()
{
    UNREACHABLE();
}

DecodedTaggedValue CreateObjectWithExcludedKeys([[maybe_unused]] uint16_t numKeys, [[maybe_unused]] int64_t objValue,
                                                [[maybe_unused]] int64_t objTag, [[maybe_unused]] int64_t a0,
                                                [[maybe_unused]] int64_t t0)
{
    UNREACHABLE();
}

void ThrowPatternNonCoercible()
{
    UNREACHABLE();
}

DecodedTaggedValue IterNext([[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

DecodedTaggedValue GetIteratorNext([[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag,
                                   [[maybe_unused]] int64_t methodValue, [[maybe_unused]] int64_t methodTag)
{
    UNREACHABLE();
}

DecodedTaggedValue CloseIterator([[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

DecodedTaggedValue ImportModule([[maybe_unused]] uint32_t stringId)
{
    UNREACHABLE();
}

void StModuleVar([[maybe_unused]] uint32_t stringId, [[maybe_unused]] int64_t valValue, [[maybe_unused]] int64_t valTag)
{
    UNREACHABLE();
}

void CopyModule([[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

DecodedTaggedValue LdModvarByName([[maybe_unused]] uint32_t stringId, [[maybe_unused]] uint16_t slotId,
                                  [[maybe_unused]] int64_t objTag, [[maybe_unused]] int64_t objValue)
{
    return DecodedTaggedValue(0, 0);
}

DecodedTaggedValue DefineClassWithBuffer([[maybe_unused]] uint32_t methodId, [[maybe_unused]] uint16_t literalIndex,
                                         [[maybe_unused]] int64_t lexenvValue, [[maybe_unused]] int64_t lexenvTag,
                                         [[maybe_unused]] int64_t parentValue, [[maybe_unused]] int64_t parentTag)
{
    UNREACHABLE();
}

DecodedTaggedValue SuperCall([[maybe_unused]] uint16_t range, [[maybe_unused]] int64_t firstVRegValue,
                             [[maybe_unused]] int64_t firstVRegTag, [[maybe_unused]] int64_t objValue,
                             [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

DecodedTaggedValue SuperCallSpread([[maybe_unused]] int64_t arrayValue, [[maybe_unused]] int64_t arrayTag,
                                   [[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

DecodedTaggedValue DefineMethod([[maybe_unused]] uint32_t methodId, [[maybe_unused]] int64_t a1,
                                [[maybe_unused]] int64_t t1, [[maybe_unused]] int64_t a2, [[maybe_unused]] int64_t t2)
{
    UNREACHABLE();
}

void StSuperByName([[maybe_unused]] uint32_t stringId, [[maybe_unused]] uint16_t slotId,
                   [[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag,
                   [[maybe_unused]] int64_t valValue, [[maybe_unused]] int64_t valTag)
{
    UNREACHABLE();
}

DecodedTaggedValue LdSuperByName([[maybe_unused]] uint32_t stringId, [[maybe_unused]] uint16_t slotId,
                                 [[maybe_unused]] int64_t objValue, [[maybe_unused]] int64_t objTag)
{
    UNREACHABLE();
}

void StSuperByValue([[maybe_unused]] uint16_t slotId, [[maybe_unused]] int64_t objValue,
                    [[maybe_unused]] int64_t objTag, [[maybe_unused]] int64_t propValue,
                    [[maybe_unused]] int64_t propTag, [[maybe_unused]] int64_t valValue,
                    [[maybe_unused]] int64_t valTag)
{
    UNREACHABLE();
}

DecodedTaggedValue LdSuperByValue([[maybe_unused]] uint16_t slotId, [[maybe_unused]] int64_t objValue,
                                  [[maybe_unused]] int64_t objTag, [[maybe_unused]] int64_t propValue,
                                  [[maybe_unused]] int64_t propTag)
{
    UNREACHABLE();
}

DecodedTaggedValue CreateObjectHavingMethod([[maybe_unused]] uint16_t index, [[maybe_unused]] int64_t a1,
                                            [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

void ThrowIfSuperNotCorrectCall([[maybe_unused]] uint16_t index, [[maybe_unused]] int64_t a1,
                                [[maybe_unused]] int64_t t1)
{
    UNREACHABLE();
}

DecodedTaggedValue LdHomeObject()
{
    UNREACHABLE();
}

void ThrowDeleteSuperProperty()
{
    UNREACHABLE();
}

void Debugger()
{
    UNREACHABLE();
}
}  // namespace panda::ecmascript::intrinsics
