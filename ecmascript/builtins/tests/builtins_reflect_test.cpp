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

#include "ecmascript/builtins/builtins_reflect.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/tagged_array-inl.h"

#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
using BuiltinsBase = panda::ecmascript::base::BuiltinsBase;
using JSArray = panda::ecmascript::JSArray;

namespace panda::test {
class BuiltinsReflectTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    PandaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

// use for create a JSObject of test
static JSHandle<JSFunction> TestObjectCreate(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    return JSHandle<JSFunction>::Cast(env->GetObjectFunction());
}

// native function for test Reflect.apply
JSTaggedValue TestReflectApply(EcmaRuntimeCallInfo *argv)
{
    auto thread = argv->GetThread();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    int result = 0;
    for (uint32_t index = 0; index < argv->GetArgsNumber(); ++index) {
        result += BuiltinsBase::GetCallArg(argv, index).GetTaggedValue().GetInt();
    }
    JSHandle<JSTaggedValue> thisValue = BuiltinsBase::GetThis(argv);

    JSTaggedValue testA =
        JSObject::GetProperty(thread, thisValue,
                              JSHandle<JSTaggedValue>(factory->NewFromCanBeCompressString("test_reflect_apply_a")))
            .GetValue()
            .GetTaggedValue();
    JSTaggedValue testB =
        JSObject::GetProperty(thread, thisValue,
                              JSHandle<JSTaggedValue>(factory->NewFromCanBeCompressString("test_reflect_apply_b")))
            .GetValue()
            .GetTaggedValue();

    result = result + testA.GetInt() + testB.GetInt();
    return BuiltinsBase::GetTaggedInt(result);
}

// Reflect.apply (target, thisArgument, argumentsList)
HWTEST_F_L0(BuiltinsReflectTest, ReflectApply)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // target
    JSHandle<JSFunction> target = factory->NewJSFunction(env, reinterpret_cast<void *>(TestReflectApply));
    // thisArgument
    JSHandle<JSObject> thisArgument =
        factory->NewJSObjectByConstructor(TestObjectCreate(thread), JSHandle<JSTaggedValue>(TestObjectCreate(thread)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(thisArgument),
                          JSHandle<JSTaggedValue>(factory->NewFromCanBeCompressString("test_reflect_apply_a")),
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(11)));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(thisArgument),
                          JSHandle<JSTaggedValue>(factory->NewFromCanBeCompressString("test_reflect_apply_b")),
                          JSHandle<JSTaggedValue>(thread, JSTaggedValue(22)));
    // argumentsList
    JSHandle<JSObject> argumentsList(JSArray::ArrayCreate(thread, JSTaggedNumber(2)));
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(33)));
    JSArray::DefineOwnProperty(thread, argumentsList, JSHandle<JSTaggedValue>(thread, JSTaggedValue(0)), desc);

    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(44)));
    JSArray::DefineOwnProperty(thread, argumentsList, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), desc1);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, target.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(*thisArgument));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(*argumentsList));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsReflect::ReflectApply(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(110).GetRawData());

    JSObject::DeleteProperty(thread, (thisArgument),
                             JSHandle<JSTaggedValue>(factory->NewFromCanBeCompressString("test_reflect_apply_a")));
    JSObject::DeleteProperty(thread, (thisArgument),
                             JSHandle<JSTaggedValue>(factory->NewFromCanBeCompressString("test_reflect_apply_b")));
}

// Reflect.construct (target, argumentsList [ , newTarget])
HWTEST_F_L0(BuiltinsReflectTest, ReflectConstruct)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // target
    JSHandle<JSFunction> target = JSHandle<JSFunction>::Cast(env->GetStringFunction());
    // argumentsList
    JSHandle<JSObject> argumentsList(JSArray::ArrayCreate(thread, JSTaggedNumber(1)));
    PropertyDescriptor desc(thread,
                            JSHandle<JSTaggedValue>::Cast(factory->NewFromCanBeCompressString("ReflectConstruct")));
    JSArray::DefineOwnProperty(thread, argumentsList, JSHandle<JSTaggedValue>(thread, JSTaggedValue(0)), desc);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, target.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(*argumentsList));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsReflect::ReflectConstruct(ecmaRuntimeCallInfo.get());

    ASSERT_TRUE(result.IsECMAObject());
    JSHandle<JSTaggedValue> taggedResult(thread, result);
    JSHandle<JSPrimitiveRef> refResult = JSHandle<JSPrimitiveRef>::Cast(taggedResult);
    JSHandle<EcmaString> ruler = factory->NewFromCanBeCompressString("ReflectConstruct");
    ASSERT_EQ(EcmaString::Cast(refResult->GetValue().GetTaggedObject())->Compare(*ruler), 0);
}

// Reflect.defineProperty (target, propertyKey, attributes)
HWTEST_F_L0(BuiltinsReflectTest, ReflectDefineProperty)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // target
    JSHandle<JSObject> target =
        factory->NewJSObjectByConstructor(TestObjectCreate(thread), JSHandle<JSTaggedValue>(TestObjectCreate(thread)));
    // propertyKey
    JSHandle<JSTaggedValue> key(factory->NewFromCanBeCompressString("test_reflect_define_property"));
    // attributes
    JSHandle<JSObject> attributes =
        factory->NewJSObjectByConstructor(TestObjectCreate(thread), JSHandle<JSTaggedValue>(TestObjectCreate(thread)));
    // attributes value
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> valueKey = globalConst->GetHandledValueString();
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(100));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(attributes), valueKey, value);
    // attributes writable
    JSHandle<JSTaggedValue> writableKey = globalConst->GetHandledWritableString();
    JSHandle<JSTaggedValue> writable(thread, JSTaggedValue::True());
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(attributes), writableKey, writable);
    // attributes enumerable
    JSHandle<JSTaggedValue> enumerableKey = globalConst->GetHandledEnumerableString();
    JSHandle<JSTaggedValue> enumerable(thread, JSTaggedValue::False());
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(attributes), enumerableKey, enumerable);
    // attributes configurable
    JSHandle<JSTaggedValue> configurableKey = globalConst->GetHandledConfigurableString();
    JSHandle<JSTaggedValue> configurable(thread, JSTaggedValue::True());
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(attributes), configurableKey, configurable);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, target.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, key.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(*attributes));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsReflect::ReflectDefineProperty(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());

    PropertyDescriptor descRuler(thread);
    JSObject::GetOwnProperty(thread, target, key, descRuler);
    ASSERT_EQ(descRuler.GetValue()->GetInt(), 100);
    ASSERT_EQ(descRuler.IsWritable(), true);
    ASSERT_EQ(descRuler.IsEnumerable(), false);
    ASSERT_EQ(descRuler.IsConfigurable(), true);
}

// Reflect.deleteProperty (target, propertyKey)
HWTEST_F_L0(BuiltinsReflectTest, ReflectDeleteProperty)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // target
    JSHandle<JSObject> target =
        factory->NewJSObjectByConstructor(TestObjectCreate(thread), JSHandle<JSTaggedValue>(TestObjectCreate(thread)));
    // propertyKey
    JSHandle<JSTaggedValue> key(factory->NewFromCanBeCompressString("test_reflect_delete_property"));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(101));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(target), key, value);

    PropertyDescriptor desc(thread);
    ASSERT_EQ(JSObject::GetOwnProperty(thread, target, key, desc), true);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, target.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, key.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsReflect::ReflectDeleteProperty(ecmaRuntimeCallInfo.get());
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());
    ASSERT_EQ(JSObject::GetOwnProperty(thread, target, key, desc), false);
}

// Reflect.get (target, propertyKey [ , receiver])
HWTEST_F_L0(BuiltinsReflectTest, ReflectGet)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // target
    JSHandle<JSObject> target =
        factory->NewJSObjectByConstructor(TestObjectCreate(thread), JSHandle<JSTaggedValue>(TestObjectCreate(thread)));
    // propertyKey
    JSHandle<JSTaggedValue> key(factory->NewFromCanBeCompressString("test_reflect_get"));
    // set property
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(101.5));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(target), key, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, target.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, key.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsReflect::ReflectGet(ecmaRuntimeCallInfo.get());

    JSHandle<JSTaggedValue> resultValue(thread, result);
    ASSERT_EQ(resultValue->GetDouble(), 101.5);
}

// Reflect.getOwnPropertyDescriptor ( target, propertyKey )
HWTEST_F_L0(BuiltinsReflectTest, ReflectGetOwnPropertyDescriptor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // target
    JSHandle<JSObject> target =
        factory->NewJSObjectByConstructor(TestObjectCreate(thread), JSHandle<JSTaggedValue>(TestObjectCreate(thread)));
    // propertyKey
    JSHandle<JSTaggedValue> key(factory->NewFromCanBeCompressString("test_reflect_get_property_descriptor"));
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(102)), true, false, true);
    ASSERT_EQ(JSTaggedValue::DefinePropertyOrThrow(thread, JSHandle<JSTaggedValue>(target), key, desc), true);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, target.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, key.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsReflect::ReflectGetOwnPropertyDescriptor(ecmaRuntimeCallInfo.get());

    ASSERT_TRUE(result.IsECMAObject());

    JSHandle<JSTaggedValue> resultObj(thread, result);
    // test value
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> valueKey = globalConst->GetHandledValueString();
    JSHandle<JSTaggedValue> resultValue = JSObject::GetProperty(thread, resultObj, valueKey).GetValue();
    ASSERT_EQ(resultValue->GetInt(), 102);
    // test writable
    JSHandle<JSTaggedValue> writableKey = globalConst->GetHandledWritableString();
    JSHandle<JSTaggedValue> resultWritable = JSObject::GetProperty(thread, resultObj, writableKey).GetValue();
    ASSERT_EQ(resultWritable->ToBoolean(), true);
    // test enumerable
    JSHandle<JSTaggedValue> enumerableKey = globalConst->GetHandledEnumerableString();
    JSHandle<JSTaggedValue> resultEnumerable = JSObject::GetProperty(thread, resultObj, enumerableKey).GetValue();
    ASSERT_EQ(resultEnumerable->ToBoolean(), false);
    // test configurable
    JSHandle<JSTaggedValue> configurableKey = globalConst->GetHandledConfigurableString();
    JSHandle<JSTaggedValue> resultConfigurable = JSObject::GetProperty(thread, resultObj, configurableKey).GetValue();
    ASSERT_EQ(resultConfigurable->ToBoolean(), true);
}

// Reflect.getPrototypeOf (target)
HWTEST_F_L0(BuiltinsReflectTest, ReflectGetPrototypeOf)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSObject> target =
        factory->NewJSObjectByConstructor(TestObjectCreate(thread), JSHandle<JSTaggedValue>(TestObjectCreate(thread)));
    JSHandle<JSObject> proto =
        factory->NewJSObjectByConstructor(TestObjectCreate(thread), JSHandle<JSTaggedValue>(TestObjectCreate(thread)));

    ASSERT_EQ(JSObject::SetPrototype(thread, target, JSHandle<JSTaggedValue>(proto)), true);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, target.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsReflect::ReflectGetPrototypeOf(ecmaRuntimeCallInfo.get());
    ASSERT_TRUE(result.IsECMAObject());
    JSHandle<JSTaggedValue> resultObj(thread, JSTaggedValue(reinterpret_cast<TaggedObject *>(result.GetRawData())));
    ASSERT_EQ(JSTaggedValue::SameValue(resultObj.GetTaggedValue(), proto.GetTaggedValue()), true);
}

// Reflect.has (target, propertyKey)
HWTEST_F_L0(BuiltinsReflectTest, ReflectHas)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // target
    JSHandle<JSObject> target =
        factory->NewJSObjectByConstructor(TestObjectCreate(thread), JSHandle<JSTaggedValue>(TestObjectCreate(thread)));
    // propertyKey
    JSHandle<JSTaggedValue> key(factory->NewFromCanBeCompressString("test_reflect_has"));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(103));
    ASSERT_EQ(JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(target), key, value), true);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, target.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, key.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsReflect::ReflectHas(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());
}

// Reflect.isExtensible (target)
HWTEST_F_L0(BuiltinsReflectTest, ReflectIsExtensible)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // target
    JSHandle<JSObject> target =
        factory->NewJSObjectByConstructor(TestObjectCreate(thread), JSHandle<JSTaggedValue>(TestObjectCreate(thread)));
    target->GetJSHClass()->SetExtensible(false);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, target.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsReflect::ReflectIsExtensible(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
}

// Reflect.ownKeys (target)
HWTEST_F_L0(BuiltinsReflectTest, ReflectOwnKeys)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // target
    JSHandle<JSObject> target =
        factory->NewJSObjectByConstructor(TestObjectCreate(thread), JSHandle<JSTaggedValue>(TestObjectCreate(thread)));
    JSHandle<JSTaggedValue> key0(factory->NewFromCanBeCompressString("test_reflect_own_keys1"));
    JSHandle<JSTaggedValue> value0(thread, JSTaggedValue(104));
    ASSERT_EQ(JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(target), key0, value0), true);
    JSHandle<JSTaggedValue> key1(factory->NewFromCanBeCompressString("test_reflect_own_keys2"));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(105));
    ASSERT_EQ(JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(target), key1, value1), true);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, target.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsReflect::ReflectOwnKeys(ecmaRuntimeCallInfo.get());

    ASSERT_TRUE(result.IsECMAObject());
    JSHandle<JSTaggedValue> resultTaggedValue(thread, reinterpret_cast<TaggedObject *>(result.GetRawData()));
    JSHandle<JSArray> resultArray = JSHandle<JSArray>::Cast(resultTaggedValue);
    // test length
    JSHandle<JSTaggedValue> resultLengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> resultLength =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(resultArray), resultLengthKey).GetValue();
    ASSERT_EQ(resultLength->GetInt(), 2);
    // test array[0]
    JSHandle<JSTaggedValue> resultKey0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> resultValue0 =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(resultArray), resultKey0).GetValue();
    ASSERT_EQ(reinterpret_cast<EcmaString *>(resultValue0.GetTaggedValue().GetTaggedObject())
                  ->Compare(reinterpret_cast<EcmaString *>(key0.GetTaggedValue().GetTaggedObject())),
              0);
    // test array[1]
    JSHandle<JSTaggedValue> resultKey1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> resultValue1 =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(resultArray), resultKey1).GetValue();
    ASSERT_EQ(reinterpret_cast<EcmaString *>(resultValue1.GetTaggedValue().GetTaggedObject())
                  ->Compare(reinterpret_cast<EcmaString *>(key1.GetTaggedValue().GetTaggedObject())),
              0);
}

// Reflect.preventExtensions (target)
HWTEST_F_L0(BuiltinsReflectTest, ReflectPreventExtensions)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // target
    JSHandle<JSObject> target =
        factory->NewJSObjectByConstructor(TestObjectCreate(thread), JSHandle<JSTaggedValue>(TestObjectCreate(thread)));
    target->GetJSHClass()->SetExtensible(true);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, target.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsReflect::ReflectPreventExtensions(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());
    ASSERT_EQ(target->IsExtensible(), false);
}

// Reflect.set (target, propertyKey, V [ , receiver])
HWTEST_F_L0(BuiltinsReflectTest, ReflectSet)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    // target
    JSHandle<JSObject> target =
        factory->NewJSObjectByConstructor(TestObjectCreate(thread), JSHandle<JSTaggedValue>(TestObjectCreate(thread)));
    // propertyKey
    JSHandle<JSTaggedValue> key(factory->NewFromCanBeCompressString("test_reflect_set"));
    // value
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(106));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, target.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, key.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsReflect::ReflectSet(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());

    JSHandle<JSTaggedValue> ruler = JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(target), key).GetValue();
    ASSERT_EQ(JSTaggedValue::ToInt32(thread, ruler), 106);
}

// Reflect.setPrototypeOf (target, proto)
HWTEST_F_L0(BuiltinsReflectTest, ReflectSetPrototypeOf)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSObject> target =
        factory->NewJSObjectByConstructor(TestObjectCreate(thread), JSHandle<JSTaggedValue>(TestObjectCreate(thread)));
    JSHandle<JSObject> proto =
        factory->NewJSObjectByConstructor(TestObjectCreate(thread), JSHandle<JSTaggedValue>(TestObjectCreate(thread)));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, target.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, proto.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo.get());
    JSTaggedValue result = BuiltinsReflect::ReflectSetPrototypeOf(ecmaRuntimeCallInfo.get());

    ASSERT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());
    JSHandle<JSTaggedValue> resultObj(thread, target->GetJSHClass()->GetPrototype());
    ASSERT_EQ(JSTaggedValue::SameValue(resultObj.GetTaggedValue(), proto.GetTaggedValue()), true);
}
}  // namespace panda::test
