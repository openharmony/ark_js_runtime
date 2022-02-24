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

#include <thread>

#include "ecmascript/builtins/builtins_arraybuffer.h"
#include "ecmascript/ecma_language_context.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_serializer.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/linked_hash_table-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace testing::ext;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class JSDeserializerTest {
public:
    JSDeserializerTest() : ecmaVm(nullptr), scope(nullptr), thread(nullptr) {}
    void Init()
    {
        JSRuntimeOptions options;
        options.SetShouldLoadBootPandaFiles(false);
        options.SetShouldInitializeIntrinsics(false);
        options.SetBootIntrinsicSpaces({"ecmascript"});
        options.SetBootClassSpaces({"ecmascript"});
        options.SetRuntimeType("ecmascript");
        options.SetEnableForceGC(true);
        ecmaVm = EcmaVM::Create(options);
        ecmaVm->SetEnableForceGC(true);
        EXPECT_TRUE(ecmaVm != nullptr) << "Cannot create Runtime";
        thread = ecmaVm->GetJSThread();
        scope = new EcmaHandleScope(thread);
    }
    void Destroy()
    {
        delete scope;
        scope = nullptr;
        ecmaVm->SetEnableForceGC(false);
        thread->ClearException();
        [[maybe_unused]] bool success = EcmaVM::Destroy(ecmaVm);
        EXPECT_TRUE(success) << "Cannot destroy Runtime";
    }
    void JSSpecialValueTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSHandle<JSTaggedValue> jsTrue(thread, JSTaggedValue::True());
        JSHandle<JSTaggedValue> jsFalse(thread, JSTaggedValue::False());
        JSHandle<JSTaggedValue> jsUndefined(thread, JSTaggedValue::Undefined());
        JSHandle<JSTaggedValue> jsNull(thread, JSTaggedValue::Null());
        JSHandle<JSTaggedValue> jsHole(thread, JSTaggedValue::Hole());

        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> retTrue = deserializer.DeserializeJSTaggedValue();
        EXPECT_TRUE(JSTaggedValue::SameValue(jsTrue, retTrue)) << "Not same value for JS_TRUE";
        JSHandle<JSTaggedValue> retFalse = deserializer.DeserializeJSTaggedValue();
        EXPECT_TRUE(JSTaggedValue::SameValue(jsFalse, retFalse)) << "Not same value for JS_FALSE";
        JSHandle<JSTaggedValue> retUndefined = deserializer.DeserializeJSTaggedValue();
        JSHandle<JSTaggedValue> retNull = deserializer.DeserializeJSTaggedValue();
        JSHandle<JSTaggedValue> retHole = deserializer.DeserializeJSTaggedValue();

        EXPECT_TRUE(JSTaggedValue::SameValue(jsUndefined, retUndefined)) << "Not same value for JS_UNDEFINED";
        EXPECT_TRUE(JSTaggedValue::SameValue(jsNull, retNull)) << "Not same value for JS_NULL";
        EXPECT_TRUE(JSTaggedValue::SameValue(jsHole, retHole)) << "Not same value for JS_HOLE";
        Destroy();
    }

    void JSPlainObjectTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> objValue1 = deserializer.DeserializeJSTaggedValue();
        JSHandle<JSTaggedValue> objValue2 = deserializer.DeserializeJSTaggedValue();

        JSHandle<JSObject> retObj1 = JSHandle<JSObject>::Cast(objValue1);
        JSHandle<JSObject> retObj2 = JSHandle<JSObject>::Cast(objValue2);
        EXPECT_FALSE(retObj1.IsEmpty());
        EXPECT_FALSE(retObj2.IsEmpty());

        JSHandle<TaggedArray> array1 = JSObject::GetOwnPropertyKeys(thread, retObj1);
        int length1 = array1->GetLength();
        EXPECT_EQ(length1, 4); // 4 : test case
        double sum1 = 0.0;
        for (int i = 0; i < length1; i++) {
            JSHandle<JSTaggedValue> key(thread, array1->Get(i));
            double a = JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(retObj1), key).GetValue()->GetNumber();
            sum1 += a;
        }
        EXPECT_EQ(sum1, 10); // 10 : test case

        JSHandle<TaggedArray> array2 = JSObject::GetOwnPropertyKeys(thread, retObj2);
        int length2 = array2->GetLength();
        EXPECT_EQ(length2, 4); // 4 : test case
        double sum2 = 0.0;
        for (int i = 0; i < length2; i++) {
            JSHandle<JSTaggedValue> key(thread, array2->Get(i));
            double a = JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(retObj2), key).GetValue()->GetNumber();
            sum2 += a;
        }
        EXPECT_EQ(sum2, 26); // 26 : test case
        Destroy();
    }

    void JSPlainObjectTest01(std::pair<uint8_t *, size_t> data)
    {
        Init();
        ObjectFactory *factory = ecmaVm->GetFactory();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> objValue1 = deserializer.DeserializeJSTaggedValue();

        JSHandle<JSTaggedValue> key1(factory->NewFromCanBeCompressString("detectResultItems"));
        JSHandle<JSTaggedValue> key2(factory->NewFromCanBeCompressString("adviceId"));
        OperationResult result = JSObject::GetProperty(thread, objValue1, key1);
        JSHandle<JSTaggedValue> value1 = result.GetRawValue();
        EXPECT_TRUE(value1->IsJSArray());
        JSHandle<JSTaggedValue> value2 = JSArray::FastGetPropertyByValue(thread, value1, 0);
        JSHandle<JSObject> obj = JSHandle<JSObject>::Cast(value2);
        bool res = JSObject::HasProperty(thread, obj, key2);
        EXPECT_TRUE(res);
        OperationResult result1 = JSObject::GetProperty(thread, value2, key2);
        JSHandle<JSTaggedValue> value3 = result1.GetRawValue();
        JSHandle<JSTaggedValue> value4 = JSHandle<JSTaggedValue>::Cast(factory->GetEmptyString());
        EXPECT_EQ(value3, value4);
        Destroy();
    }

    void DescriptionTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        ObjectFactory *factory = ecmaVm->GetFactory();
        JSHandle<JSTaggedValue> key1(factory->NewFromCanBeCompressString("x"));
        JSHandle<JSTaggedValue> key2(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("y"));
        JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(1));
        JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(2)); // 2 : test case

        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> objValue = deserializer.DeserializeJSTaggedValue();
        JSHandle<JSObject> retObj = JSHandle<JSObject>::Cast(objValue);

        PropertyDescriptor desc3(thread);
        PropertyDescriptor desc4(thread);

        JSHandle<TaggedArray> array1 = JSObject::GetOwnPropertyKeys(thread, retObj);
        JSHandle<JSTaggedValue> retKey1(thread, array1->Get(0));
        JSHandle<JSTaggedValue> retKey2(thread, array1->Get(1));
        JSObject::GetOwnProperty(thread, retObj, retKey1, desc3);
        JSObject::GetOwnProperty(thread, retObj, retKey2, desc4);
        EXPECT_EQ(key1.GetTaggedValue().GetRawData(), retKey1.GetTaggedValue().GetRawData());

        EXPECT_EQ(desc3.GetValue().GetTaggedValue().GetRawData(), value1.GetTaggedValue().GetRawData());
        EXPECT_TRUE(desc3.IsWritable());
        EXPECT_FALSE(desc3.IsEnumerable());
        EXPECT_TRUE(desc3.IsConfigurable());

        EXPECT_EQ(desc4.GetValue().GetTaggedValue().GetRawData(), value2.GetTaggedValue().GetRawData());
        EXPECT_FALSE(desc4.IsWritable());
        EXPECT_TRUE(desc4.IsEnumerable());
        EXPECT_FALSE(desc4.IsConfigurable());
        Destroy();
    }

    void JSSetTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        ObjectFactory *factory = ecmaVm->GetFactory();
        JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(7)); // 7 : test case
        JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(9)); // 9 : test case
        JSHandle<JSTaggedValue> value3(factory->NewFromCanBeCompressString("x"));
        JSHandle<JSTaggedValue> value4(factory->NewFromCanBeCompressString("y"));

        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> setValue = deserializer.DeserializeJSTaggedValue();
        EXPECT_TRUE(!setValue.IsEmpty());
        JSHandle<JSSet> retSet = JSHandle<JSSet>::Cast(setValue);
        JSHandle<TaggedArray> array = JSObject::GetOwnPropertyKeys(thread, JSHandle<JSObject>::Cast(retSet));
        int propertyLength = array->GetLength();
        EXPECT_EQ(propertyLength, 2); // 2 : test case
        int sum = 0;
        for (int i = 0; i < propertyLength; i++) {
            JSHandle<JSTaggedValue> key(thread, array->Get(i));
            double a = JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(retSet), key).GetValue()->GetNumber();
            sum += a;
        }
        EXPECT_EQ(sum, 16); // 16 : test case

        EXPECT_EQ(retSet->GetSize(), 4);  // 4 : test case
        EXPECT_TRUE(retSet->Has(value1.GetTaggedValue()));
        EXPECT_TRUE(retSet->Has(value2.GetTaggedValue()));
        EXPECT_TRUE(retSet->Has(value3.GetTaggedValue()));
        EXPECT_TRUE(retSet->Has(value4.GetTaggedValue()));
        Destroy();
    }

    void JSArrayTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> arrayValue = deserializer.DeserializeJSTaggedValue();
        EXPECT_TRUE(!arrayValue.IsEmpty());

        JSHandle<JSArray> retArray = JSHandle<JSArray>::Cast(arrayValue);

        JSHandle<TaggedArray> keyArray = JSObject::GetOwnPropertyKeys(thread, JSHandle<JSObject>(retArray));
        int propertyLength = keyArray->GetLength();
        EXPECT_EQ(propertyLength, 23);  // 23 : test case
        int sum = 0;
        for (int i = 0; i < propertyLength; i++) {
            JSHandle<JSTaggedValue> key(thread, keyArray->Get(i));
            double a = JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(retArray), key).GetValue()->GetNumber();
            sum += a;
        }
        EXPECT_EQ(sum, 226);  // 226 : test case

        // test get value from array
        for (int i = 0; i < 20; i++) {  // 20 : test case
            JSHandle<JSTaggedValue> value = JSArray::FastGetPropertyByValue(thread, arrayValue, i);
            EXPECT_EQ(i, value.GetTaggedValue().GetInt());
        }
        Destroy();
    }

    void ObjectsPropertyReferenceTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> objValue1 = deserializer.DeserializeJSTaggedValue();
        JSHandle<JSTaggedValue> objValue2 = deserializer.DeserializeJSTaggedValue();
        EXPECT_TRUE(!objValue1.IsEmpty()) << "[Empty] Deserialize obj1 fail";
        EXPECT_TRUE(!objValue2.IsEmpty()) << "[Empty] Deserialize obj2 fail";
        Destroy();
    }

    void EcmaStringTest1(std::pair<uint8_t *, size_t> data)
    {
        Init();
        const char *rawStr = "this is a test ecmaString";
        JSHandle<EcmaString> ecmaString = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString(rawStr);

        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.DeserializeJSTaggedValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize ecmaString fail";
        EXPECT_TRUE(res->IsString()) << "[NotString] Deserialize ecmaString fail";
        JSHandle<EcmaString> resEcmaString = JSHandle<EcmaString>::Cast(res);
        EXPECT_TRUE(ecmaString->GetHashcode() == resEcmaString->GetHashcode()) << "Not same HashCode";
        EXPECT_TRUE(EcmaString::StringsAreEqual(*ecmaString, *resEcmaString)) << "Not same EcmaString";
        Destroy();
    }

    void EcmaStringTest2(std::pair<uint8_t *, size_t> data)
    {
        Init();
        const char *rawStr = "ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"\
        "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"\
        "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"\
        "ssssss";
        JSHandle<EcmaString> ecmaString = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString(rawStr);

        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.DeserializeJSTaggedValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize ecmaString fail";
        EXPECT_TRUE(res->IsString()) << "[NotString] Deserialize ecmaString fail";
        JSHandle<EcmaString> resEcmaString = JSHandle<EcmaString>::Cast(res);
        EXPECT_TRUE(ecmaString->GetHashcode() == resEcmaString->GetHashcode()) << "Not same HashCode";
        EXPECT_TRUE(EcmaString::StringsAreEqual(*ecmaString, *resEcmaString)) << "Not same EcmaString";
        Destroy();
    }

    void EcmaStringTest3(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSHandle<EcmaString> ecmaString = thread->GetEcmaVM()->GetFactory()->GetEmptyString();

        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.DeserializeJSTaggedValue();
        EXPECT_TRUE(res->IsString()) << "[NotString] Deserialize ecmaString fail";
        JSHandle<EcmaString> resEcmaString = JSHandle<EcmaString>::Cast(res);
        EXPECT_TRUE(ecmaString->GetHashcode() == resEcmaString->GetHashcode()) << "Not same HashCode";
        EXPECT_TRUE(EcmaString::StringsAreEqual(*ecmaString, *resEcmaString)) << "Not same EcmaString";
        Destroy();
    }

    void Int32Test(std::pair<uint8_t *, size_t> data)
    {
        Init();
        int32_t a = 64;
        int32_t min = -2147483648;
        int32_t b = -63;
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> resA = deserializer.DeserializeJSTaggedValue();
        JSHandle<JSTaggedValue> resMin = deserializer.DeserializeJSTaggedValue();
        JSHandle<JSTaggedValue> resB = deserializer.DeserializeJSTaggedValue();
        EXPECT_TRUE(!resA.IsEmpty() && !resMin.IsEmpty() && !resB.IsEmpty()) << "[Empty] Deserialize Int32 fail";
        EXPECT_TRUE(resA->IsInt() && resMin->IsInt() && resB->IsInt()) << "[NotInt] Deserialize Int32 fail";
        EXPECT_TRUE(JSTaggedValue::ToInt32(thread, resA) == a) << "Not Same Value";
        EXPECT_TRUE(JSTaggedValue::ToInt32(thread, resMin) == min) << "Not Same Value";
        EXPECT_TRUE(JSTaggedValue::ToInt32(thread, resB) == b) << "Not Same Value";
        Destroy();
    }

    void DoubleTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        double a = 3.1415926535;
        double b = -3.1415926535;
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> resA = deserializer.DeserializeJSTaggedValue();
        JSHandle<JSTaggedValue> resB = deserializer.DeserializeJSTaggedValue();
        EXPECT_TRUE(!resA.IsEmpty() && !resB.IsEmpty()) << "[Empty] Deserialize double fail";
        EXPECT_TRUE(resA->IsDouble() && resB->IsDouble()) << "[NotInt] Deserialize double fail";
        EXPECT_TRUE(resA->GetDouble() == a) << "Not Same Value";
        EXPECT_TRUE(resB->GetDouble() == b) << "Not Same Value";
        Destroy();
    }

    void JSDateTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        double tm = 28 * 60 * 60 * 1000;  // 28 * 60 * 60 * 1000 : test case
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.DeserializeJSTaggedValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSDate fail";
        EXPECT_TRUE(res->IsDate()) << "[NotJSDate] Deserialize JSDate fail";
        JSHandle<JSDate> resDate = JSHandle<JSDate>(res);
        EXPECT_TRUE(resDate->GetTimeValue() == JSTaggedValue(tm)) << "Not Same Time Value";
        Destroy();
    }

    void JSMapTest(std::pair<uint8_t *, size_t> data, const JSHandle<JSMap> &originMap)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.DeserializeJSTaggedValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSMap fail";
        EXPECT_TRUE(res->IsJSMap()) << "[NotJSMap] Deserialize JSMap fail";
        JSHandle<JSMap> resMap = JSHandle<JSMap>::Cast(res);
        EXPECT_TRUE(originMap->GetSize() == resMap->GetSize()) << "the map size Not equal";
        uint32_t resSize = resMap->GetSize();
        for (uint32_t i = 0; i < resSize; i++) {
            JSHandle<JSTaggedValue> resKey(thread, resMap->GetKey(i));
            JSHandle<JSTaggedValue> resValue(thread, resMap->GetValue(i));
            JSHandle<JSTaggedValue> key(thread, originMap->GetKey(i));
            JSHandle<JSTaggedValue> value(thread, originMap->GetValue(i));

            JSHandle<EcmaString> resKeyStr = JSHandle<EcmaString>::Cast(resKey);
            JSHandle<EcmaString> keyStr = JSHandle<EcmaString>::Cast(key);
            EXPECT_TRUE(EcmaString::StringsAreEqual(*resKeyStr, *keyStr)) << "Not same map key";
            EXPECT_TRUE(JSTaggedValue::ToInt32(thread, resValue) == JSTaggedValue::ToInt32(thread, value))
                << "Not same map value";
        }
        Destroy();
    }

    void JSArrayBufferTest(std::pair<uint8_t *, size_t> data,
                           const JSHandle<JSArrayBuffer> &originArrayBuffer, int32_t byteLength, const char *msg)
    {
        Init();
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.DeserializeJSTaggedValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSArrayBuffer fail";
        EXPECT_TRUE(res->IsArrayBuffer()) << "[NotJSArrayBuffer] Deserialize JSArrayBuffer fail";
        JSHandle<JSArrayBuffer> resJSArrayBuffer = JSHandle<JSArrayBuffer>::Cast(res);
        int32_t resByteLength = resJSArrayBuffer->GetArrayBufferByteLength();
        EXPECT_TRUE(resByteLength == byteLength) << "Not Same ByteLength"; // 10 : test case

        JSHandle<JSTaggedValue> bufferData(thread, originArrayBuffer->GetArrayBufferData());
        auto np = JSHandle<JSNativePointer>::Cast(bufferData);
        void *buffer = np->GetExternalPointer();
        ASSERT_NE(buffer, nullptr);
        JSHandle<JSTaggedValue> resBufferData(thread, resJSArrayBuffer->GetArrayBufferData());
        JSHandle<JSNativePointer> resNp = JSHandle<JSNativePointer>::Cast(resBufferData);
        void *resBuffer = resNp->GetExternalPointer();
        ASSERT_NE(resBuffer, nullptr);

        for (int32_t i = 0; i < resByteLength; i++) {
            EXPECT_TRUE(static_cast<char *>(resBuffer)[i] == static_cast<char *>(buffer)[i]) << "Not Same Buffer";
        }

        if (msg != nullptr) {
            if (memcpy_s(resBuffer, byteLength, msg, byteLength) != EOK) {
                EXPECT_TRUE(false) << " memcpy error!";
            }
        }
        Destroy();
    }

    void JSRegexpTest(std::pair<uint8_t *, size_t> data)
    {
        Init();
        JSHandle<EcmaString> pattern = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("key2");
        JSHandle<EcmaString> flags = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("i");
        char buffer[] = "1234567";  // use char buffer to simulate byteCodeBuffer
        uint32_t bufferSize = 7;

        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.DeserializeJSTaggedValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize JSRegExp fail";
        EXPECT_TRUE(res->IsJSRegExp()) << "[NotJSRegexp] Deserialize JSRegExp fail";
        JSHandle<JSRegExp> resJSRegexp(res);

        uint32_t resBufferSize = resJSRegexp->GetLength();
        EXPECT_TRUE(resBufferSize == bufferSize) << "Not Same Length";
        JSHandle<JSTaggedValue> originalSource(thread, resJSRegexp->GetOriginalSource());
        EXPECT_TRUE(originalSource->IsString());
        JSHandle<JSTaggedValue> originalFlags(thread, resJSRegexp->GetOriginalFlags());
        EXPECT_TRUE(originalFlags->IsString());
        EXPECT_TRUE(EcmaString::StringsAreEqual(*JSHandle<EcmaString>(originalSource), *pattern));
        EXPECT_TRUE(EcmaString::StringsAreEqual(*JSHandle<EcmaString>(originalFlags), *flags));

        JSHandle<JSTaggedValue> resBufferData(thread, resJSRegexp->GetByteCodeBuffer());
        JSHandle<JSNativePointer> resNp = JSHandle<JSNativePointer>::Cast(resBufferData);
        void *resBuffer = resNp->GetExternalPointer();
        ASSERT_NE(resBuffer, nullptr);

        for (uint32_t i = 0; i < resBufferSize; i++) {
            EXPECT_TRUE(static_cast<char *>(resBuffer)[i] == buffer[i]) << "Not Same ByteCode";
        }
        Destroy();
    }

    void TypedArrayTest(std::pair<uint8_t *, size_t> data, const JSHandle<JSTypedArray> &originTypedArray)
    {
        Init();
        JSHandle<JSTaggedValue> originTypedArrayName(thread, originTypedArray->GetTypedArrayName());
        JSDeserializer deserializer(thread, data.first, data.second);
        JSHandle<JSTaggedValue> res = deserializer.DeserializeJSTaggedValue();
        EXPECT_TRUE(!res.IsEmpty()) << "[Empty] Deserialize TypedArray fail";
        EXPECT_TRUE(res->IsJSInt8Array()) << "[NotJSInt8Array] Deserialize TypedArray fail";
        JSHandle<JSTypedArray> resJSInt8Array = JSHandle<JSTypedArray>::Cast(res);

        JSHandle<JSTaggedValue> typedArrayName(thread, resJSInt8Array->GetTypedArrayName());
        JSTaggedValue byteLength = resJSInt8Array->GetByteLength();
        JSTaggedValue byteOffset = resJSInt8Array->GetByteOffset();
        JSTaggedValue arrayLength = resJSInt8Array->GetArrayLength();
        JSHandle<JSTaggedValue> viewedArrayBuffer(thread, resJSInt8Array->GetViewedArrayBuffer());

        EXPECT_TRUE(typedArrayName->IsString());
        EXPECT_TRUE(EcmaString::StringsAreEqual(*JSHandle<EcmaString>(typedArrayName),
                                                *JSHandle<EcmaString>(originTypedArrayName)));
        EXPECT_TRUE(byteLength.GetInt() == originTypedArray->GetByteLength().GetInt()) << "Not Same ByteLength";
        EXPECT_TRUE(byteOffset.GetInt() == originTypedArray->GetByteOffset().GetInt()) << "Not Same ByteOffset";
        EXPECT_TRUE(arrayLength.GetInt() == originTypedArray->GetArrayLength().GetInt()) << "Not Same ArrayLength";

        // check arrayBuffer
        JSHandle<JSArrayBuffer> resJSArrayBuffer(viewedArrayBuffer);
        JSHandle<JSArrayBuffer> originArrayBuffer(thread, originTypedArray->GetViewedArrayBuffer());
        uint32_t resTaggedLength = resJSArrayBuffer->GetArrayBufferByteLength();
        uint32_t originTaggedLength = originArrayBuffer->GetArrayBufferByteLength();
        EXPECT_TRUE(resTaggedLength == originTaggedLength) << "Not same viewedBuffer length";
        JSHandle<JSTaggedValue> bufferData(thread, originArrayBuffer->GetArrayBufferData());
        JSHandle<JSNativePointer> np = JSHandle<JSNativePointer>::Cast(bufferData);
        void *buffer = np->GetExternalPointer();
        JSHandle<JSTaggedValue> resBufferData(thread, resJSArrayBuffer->GetArrayBufferData());
        JSHandle<JSNativePointer> resNp = JSHandle<JSNativePointer>::Cast(resBufferData);
        void *resBuffer = resNp->GetExternalPointer();
        for (uint32_t i = 0; i < resTaggedLength; i++) {
            EXPECT_TRUE(static_cast<char *>(resBuffer)[i] == static_cast<char *>(buffer)[i]) << "Not same viewedBuffer";
        }
        Destroy();
    }

private:
    EcmaVM *ecmaVm = nullptr;
    EcmaHandleScope *scope = nullptr;
    JSThread *thread = nullptr;
};

class JSSerializerTest : public testing::Test {
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
        ecmaVm = EcmaVM::Cast(instance);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    JSThread *thread {nullptr};
    PandaVM *instance {nullptr};
    EcmaVM *ecmaVm {nullptr};
    EcmaHandleScope *scope {nullptr};
};

HWTEST_F_L0(JSSerializerTest, SerializeJSSpecialValue)
{
    JSSerializer *serializer = new JSSerializer(thread);
    JSHandle<JSTaggedValue> jsTrue(thread, JSTaggedValue::True());
    JSHandle<JSTaggedValue> jsFalse(thread, JSTaggedValue::False());
    JSHandle<JSTaggedValue> jsUndefined(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> jsNull(thread, JSTaggedValue::Null());
    JSHandle<JSTaggedValue> jsHole(thread, JSTaggedValue::Hole());
    serializer->SerializeJSTaggedValue(jsTrue);
    serializer->SerializeJSTaggedValue(jsFalse);
    serializer->SerializeJSTaggedValue(jsUndefined);
    serializer->SerializeJSTaggedValue(jsNull);
    serializer->SerializeJSTaggedValue(jsHole);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSSpecialValueTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSPlainObject)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj1 = factory->NewEmptyJSObject();
    JSHandle<JSObject> obj2 = factory->NewEmptyJSObject();

    JSHandle<JSTaggedValue> key1(factory->NewFromCanBeCompressString("2"));
    JSHandle<JSTaggedValue> key2(factory->NewFromCanBeCompressString("3"));
    JSHandle<JSTaggedValue> key3(factory->NewFromCanBeCompressString("x"));
    JSHandle<JSTaggedValue> key4(factory->NewFromCanBeCompressString("y"));
    JSHandle<JSTaggedValue> key5(factory->NewFromCanBeCompressString("a"));
    JSHandle<JSTaggedValue> key6(factory->NewFromCanBeCompressString("b"));
    JSHandle<JSTaggedValue> key7(factory->NewFromCanBeCompressString("5"));
    JSHandle<JSTaggedValue> key8(factory->NewFromCanBeCompressString("6"));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value3(thread, JSTaggedValue(3));
    JSHandle<JSTaggedValue> value4(thread, JSTaggedValue(4));
    JSHandle<JSTaggedValue> value5(thread, JSTaggedValue(5));
    JSHandle<JSTaggedValue> value6(thread, JSTaggedValue(6));
    JSHandle<JSTaggedValue> value7(thread, JSTaggedValue(7));
    JSHandle<JSTaggedValue> value8(thread, JSTaggedValue(8));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key2, value2);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key3, value3);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key4, value4);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key5, value5);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key6, value6);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key7, value7);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key8, value8);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success1 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj1));
    bool success2 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj2));

    EXPECT_TRUE(success1);
    EXPECT_TRUE(success2);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSPlainObjectTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSPlainObject01)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj1 = factory->NewEmptyJSObject();
    JSHandle<JSObject> obj2 = factory->NewEmptyJSObject();

    JSHandle<JSTaggedValue> key1(factory->NewFromCanBeCompressString("diagnosisItem"));
    JSHandle<JSTaggedValue> key2(factory->NewFromCanBeCompressString("detectStatus"));
    JSHandle<JSTaggedValue> key3(factory->NewFromCanBeCompressString("detectResultItems"));
    JSHandle<JSTaggedValue> key4(factory->NewFromCanBeCompressString("faultId"));
    JSHandle<JSTaggedValue> key5(factory->NewFromCanBeCompressString("faultContent"));
    JSHandle<JSTaggedValue> key6(factory->NewFromCanBeCompressString("faultContentParams"));
    JSHandle<JSTaggedValue> key7(factory->NewFromCanBeCompressString("adviceId"));
    JSHandle<JSTaggedValue> key8(factory->NewFromCanBeCompressString("adviceContent"));
    JSHandle<JSTaggedValue> key9(factory->NewFromCanBeCompressString("adviceContentParams"));
    JSHandle<JSTaggedValue> value1 = JSHandle<JSTaggedValue>::Cast(factory->NewFromCanBeCompressString("VoiceDetect1"));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value3 = JSHandle<JSTaggedValue>::Cast(factory->NewJSArray());
    JSHandle<JSTaggedValue> value4 = JSHandle<JSTaggedValue>::Cast(factory->NewFromCanBeCompressString("80000001"));
    JSHandle<JSTaggedValue> value5 = JSHandle<JSTaggedValue>::Cast(factory->GetEmptyString());
    JSHandle<JSTaggedValue> value6 = JSHandle<JSTaggedValue>::Cast(factory->NewJSArray());
    JSHandle<JSTaggedValue> value7 = JSHandle<JSTaggedValue>::Cast(factory->GetEmptyString());
    JSHandle<JSTaggedValue> value8 = JSHandle<JSTaggedValue>::Cast(factory->GetEmptyString());
    JSHandle<JSTaggedValue> value9 = JSHandle<JSTaggedValue>::Cast(factory->NewJSArray());

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key4, value4);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key5, value5);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key6, value6);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key7, value7);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key8, value8);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj2), key9, value9);
    JSArray::FastSetPropertyByValue(thread, value3, 0, JSHandle<JSTaggedValue>(obj2));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key2, value2);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj1), key3, value3);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success1 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj1));

    EXPECT_TRUE(success1);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSPlainObjectTest01, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, TestSerializeDescription)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();

    JSHandle<JSTaggedValue> key1(factory->NewFromCanBeCompressString("x"));
    JSHandle<JSTaggedValue> key2(thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("y"));

    PropertyDescriptor desc1(thread);
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(1));
    desc1.SetValue(value1);
    desc1.SetWritable(true);
    desc1.SetEnumerable(false);
    desc1.SetConfigurable(true);
    JSObject::DefineOwnProperty(thread, obj, key1, desc1);

    PropertyDescriptor desc2(thread);
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(2));
    desc2.SetValue(value2);
    desc2.SetWritable(false);
    desc2.SetEnumerable(true);
    desc2.SetConfigurable(false);
    JSObject::DefineOwnProperty(thread, obj, key2, desc2);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj));
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::DescriptionTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, TestSerializeJSSet)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsSetFunction();
    JSHandle<JSSet> set =
        JSHandle<JSSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashSet> linkedSet = LinkedHashSet::Create(thread);
    set->SetLinkedSet(thread, linkedSet);
    // set property to set
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(7));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(9));
    JSHandle<JSTaggedValue> value3(factory->NewFromCanBeCompressString("x"));
    JSHandle<JSTaggedValue> value4(factory->NewFromCanBeCompressString("y"));

    JSSet::Add(thread, set, value1);
    JSSet::Add(thread, set, value2);
    JSSet::Add(thread, set, value3);
    JSSet::Add(thread, set, value4);

    // set property to object
    JSHandle<JSTaggedValue> key1(factory->NewFromCanBeCompressString("5"));
    JSHandle<JSTaggedValue> key2(factory->NewFromCanBeCompressString("6"));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(set), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(set), key2, value2);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(set));
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSSetTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, TestSerializeJSArray)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSArray> array = factory->NewJSArray();

    // set property to object
    JSHandle<JSTaggedValue> key1(factory->NewFromCanBeCompressString("abasd"));
    JSHandle<JSTaggedValue> key2(factory->NewFromCanBeCompressString("qweqwedasd"));

    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(7));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(9));

    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(array), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(array), key2, value2);

    // set value to array
    array->SetArrayLength(thread, 20);
    for (int i = 0; i < 20; i++) {
        JSHandle<JSTaggedValue> data(thread, JSTaggedValue(i));
        JSArray::FastSetPropertyByValue(thread, JSHandle<JSTaggedValue>::Cast(array), i, data);
    }

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(array));
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSArrayTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

// Test the situation that Objects' properties stores values that reference with each other
HWTEST_F_L0(JSSerializerTest, TestObjectsPropertyReference)
{
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> obj1 = factory->NewEmptyJSObject();
    JSHandle<JSObject> obj2 = factory->NewEmptyJSObject();
    [[maybe_unused]] JSHandle<JSObject> obj3 = factory->NewEmptyJSObject();

    JSHandle<JSTaggedValue> key1(factory->NewFromCanBeCompressString("abc"));
    JSHandle<JSTaggedValue> key2(factory->NewFromCanBeCompressString("def"));
    JSHandle<JSTaggedValue> key3(factory->NewFromCanBeCompressString("dgsdgf"));
    JSHandle<JSTaggedValue> key4(factory->NewFromCanBeCompressString("qwjhrf"));

    JSHandle<JSTaggedValue> value3(thread, JSTaggedValue(10));
    JSHandle<JSTaggedValue> value4(thread, JSTaggedValue(5));

    // set property to obj1
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(obj1), key1, JSHandle<JSTaggedValue>::Cast(obj2));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(obj1), key3, value3);

    // set property to obj2
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(obj2), key2, JSHandle<JSTaggedValue>::Cast(obj1));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>::Cast(obj2), key4, value4);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success1 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj1));
    bool success2 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(obj2));
    EXPECT_TRUE(success1) << "Serialize obj1 fail";
    EXPECT_TRUE(success2) << "Serialize obj2 fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::ObjectsPropertyReferenceTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeEcmaString1)
{
    const char *rawStr = "this is a test ecmaString";
    JSHandle<EcmaString> ecmaString = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString(rawStr);
    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(ecmaString));
    EXPECT_TRUE(success) << "Serialize EcmaString fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::EcmaStringTest1, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeEcmaString2)
{
    const char *rawStr = "ssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"\
    "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"\
    "sssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssssss"\
    "ssssss";
    JSHandle<EcmaString> ecmaString = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString(rawStr);
    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(ecmaString));
    EXPECT_TRUE(success) << "Serialize EcmaString fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::EcmaStringTest2, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeEcmaString3)
{
    JSHandle<EcmaString> ecmaString = thread->GetEcmaVM()->GetFactory()->GetEmptyString();
    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(ecmaString));
    EXPECT_TRUE(success) << "Serialize EcmaString fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::EcmaStringTest3, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeInt32_t)
{
    JSSerializer *serializer = new JSSerializer(thread);
    int32_t a = 64, min = -2147483648, b = -63;
    JSTaggedValue aTag(a), minTag(min), bTag(b);
    bool success1 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(thread, aTag));
    bool success2 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(thread, minTag));
    bool success3 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(thread, bTag));
    EXPECT_TRUE(success1 && success2 && success3) << "Serialize Int32 fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::Int32Test, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeDouble)
{
    JSSerializer *serializer = new JSSerializer(thread);
    double a = 3.1415926535, b = -3.1415926535;
    JSTaggedValue aTag(a), bTag(b);
    bool success1 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(thread, aTag));
    bool success2 = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(thread, bTag));
    EXPECT_TRUE(success1 && success2) << "Serialize Double fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::DoubleTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

JSDate *JSDateCreate(EcmaVM *ecmaVM)
{
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> dateFunction = globalEnv->GetDateFunction();
    JSHandle<JSDate> dateObject =
        JSHandle<JSDate>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dateFunction), dateFunction));
    return *dateObject;
}

HWTEST_F_L0(JSSerializerTest, SerializeDate)
{
    double tm = 28 * 60 * 60 * 1000;
    JSHandle<JSDate> jsDate(thread, JSDateCreate(ecmaVm));
    jsDate->SetTimeValue(thread, JSTaggedValue(tm));
    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(jsDate));
    EXPECT_TRUE(success) << "Serialize JSDate fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSDateTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

JSMap *CreateMap(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = env->GetBuiltinsMapFunction();
    JSHandle<JSMap> map =
        JSHandle<JSMap>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
    JSHandle<LinkedHashMap> linkedMap = LinkedHashMap::Create(thread);
    map->SetLinkedMap(thread, linkedMap);
    return *map;
}

HWTEST_F_L0(JSSerializerTest, SerializeJSMap)
{
    JSHandle<JSMap> map(thread, CreateMap(thread));
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> key1(factory->NewFromCanBeCompressString("3"));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(12345));
    JSMap::Set(thread, map, key1, value1);
    JSHandle<JSTaggedValue> key2(factory->NewFromCanBeCompressString("key1"));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(34567));
    JSMap::Set(thread, map, key2, value2);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(map));
    EXPECT_TRUE(success) << "Serialize JSMap fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSMapTest, jsDeserializerTest, data, map);
    t1.join();
    delete serializer;
};

JSArrayBuffer *CreateJSArrayBuffer(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> target = env->GetArrayBufferFunction();
    JSHandle<JSArrayBuffer> jsArrayBuffer =
        JSHandle<JSArrayBuffer>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(target), target));
    return *jsArrayBuffer;
}

HWTEST_F_L0(JSSerializerTest, SerializeJSArrayBuffer)
{
    JSHandle<JSArrayBuffer> jsArrayBuffer(thread, CreateJSArrayBuffer(thread));
    int32_t byteLength = 10;
    thread->GetEcmaVM()->GetFactory()->NewJSArrayBufferData(jsArrayBuffer, byteLength);
    jsArrayBuffer->SetArrayBufferByteLength(byteLength);
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(jsArrayBuffer);
    BuiltinsArrayBuffer::SetValueInBuffer(obj.GetTaggedValue(), 1, DataViewType::UINT8, JSTaggedNumber(7), true);
    BuiltinsArrayBuffer::SetValueInBuffer(obj.GetTaggedValue(), 3, DataViewType::UINT8, JSTaggedNumber(17), true);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(jsArrayBuffer));
    EXPECT_TRUE(success) << "Serialize JSArrayBuffer fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSArrayBufferTest, jsDeserializerTest, data, jsArrayBuffer, 10, nullptr);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSArrayBufferShared)
{
    std::string msg = "hello world";
    int msgBufferLen = msg.length() + 1;
    char* msgBuffer = new char[msgBufferLen] { 0 };
    if (memcpy_s(msgBuffer, msgBufferLen, msg.c_str(), msgBufferLen) != EOK) {
        delete[] msgBuffer;
        EXPECT_TRUE(false) << " memcpy error";
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArrayBuffer> jsArrayBuffer = factory->NewJSArrayBuffer(msgBuffer, msgBufferLen, nullptr, nullptr);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(jsArrayBuffer));
    EXPECT_TRUE(success) << "Serialize JSArrayBuffer fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSArrayBufferTest, jsDeserializerTest, data, jsArrayBuffer, 12, nullptr);
    t1.join();
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSArrayBufferShared2)
{
    std::string msg = "hello world";
    int msgBufferLen = msg.length() + 1;
    char* msgBuffer = new char[msgBufferLen] { 0 };
    if (memcpy_s(msgBuffer, msgBufferLen, msg.c_str(), msgBufferLen) != EOK) {
        delete[] msgBuffer;
        EXPECT_TRUE(false) << " memcpy error";
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArrayBuffer> jsArrayBuffer = factory->NewJSArrayBuffer(msgBuffer, msgBufferLen, nullptr, nullptr, true);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(jsArrayBuffer));
    EXPECT_TRUE(success) << "Serialize JSArrayBuffer fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::string changeStr = "world hello";
    std::thread t1(&JSDeserializerTest::JSArrayBufferTest,
                   jsDeserializerTest, data, jsArrayBuffer, 12, changeStr.c_str());
    t1.join();
    EXPECT_TRUE(strcmp(msgBuffer, "world hello") == 0) << "Serialize JSArrayBuffer fail";
    delete serializer;
};

HWTEST_F_L0(JSSerializerTest, SerializeJSRegExp)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> target = env->GetRegExpFunction();
    JSHandle<JSRegExp> jsRegexp =
        JSHandle<JSRegExp>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(target), target));
    JSHandle<EcmaString> pattern = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("key2");
    JSHandle<EcmaString> flags = thread->GetEcmaVM()->GetFactory()->NewFromCanBeCompressString("i");
    char buffer[] = "1234567";  // use char to simulate bytecode
    uint32_t bufferSize = 7;
    factory->NewJSRegExpByteCodeData(jsRegexp, static_cast<void *>(buffer), bufferSize);
    jsRegexp->SetOriginalSource(thread, JSHandle<JSTaggedValue>(pattern));
    jsRegexp->SetOriginalFlags(thread, JSHandle<JSTaggedValue>(flags));

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(jsRegexp));
    EXPECT_TRUE(success) << "Serialize JSRegExp fail";
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::JSRegexpTest, jsDeserializerTest, data);
    t1.join();
    delete serializer;
};

JSArrayBuffer *CreateTestJSArrayBuffer(JSThread *thread)
{
    JSHandle<JSArrayBuffer> jsArrayBuffer(thread, CreateJSArrayBuffer(thread));
    int32_t byteLength = 10;
    thread->GetEcmaVM()->GetFactory()->NewJSArrayBufferData(jsArrayBuffer, byteLength);
    jsArrayBuffer->SetArrayBufferByteLength(byteLength);
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(jsArrayBuffer);
    // 7 : test case
    BuiltinsArrayBuffer::SetValueInBuffer(obj.GetTaggedValue(), 1, DataViewType::UINT8, JSTaggedNumber(7), true);
    // 3, 17 : test case
    BuiltinsArrayBuffer::SetValueInBuffer(obj.GetTaggedValue(), 3, DataViewType::UINT8, JSTaggedNumber(17), true);
    return *jsArrayBuffer;
}

HWTEST_F_L0(JSSerializerTest, SerializeJSTypedArray)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> target = env->GetInt8ArrayFunction();
    JSHandle<JSTypedArray> int8Array =
        JSHandle<JSTypedArray>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(target), target));
    JSHandle<JSTaggedValue> viewedArrayBuffer(thread, CreateTestJSArrayBuffer(thread));
    int8Array->SetViewedArrayBuffer(thread, viewedArrayBuffer);
    int byteLength = 10;
    int byteOffset = 0;
    int arrayLength = (byteLength - byteOffset) / (sizeof(int8_t));
    int8Array->SetByteLength(thread, JSTaggedValue(byteLength));
    int8Array->SetByteOffset(thread, JSTaggedValue(byteOffset));
    int8Array->SetTypedArrayName(thread, thread->GlobalConstants()->GetInt8ArrayString());
    int8Array->SetArrayLength(thread, JSTaggedValue(arrayLength));
    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>::Cast(int8Array));
    EXPECT_TRUE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializerTest jsDeserializerTest;
    std::thread t1(&JSDeserializerTest::TypedArrayTest, jsDeserializerTest, data, int8Array);
    t1.join();
    delete serializer;
};

// not support function
HWTEST_F_L0(JSSerializerTest, SerializeObjectWithFunction)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> function = env->GetRegExpFunction();
    EXPECT_TRUE(function->IsJSFunction());
    JSHandle<JSTaggedValue> key(factory->NewFromCanBeCompressString("2"));
    JSHandle<JSObject> obj = factory->NewEmptyJSObject();
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key, function);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(obj));
    EXPECT_FALSE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializer deserializer(thread, data.first, data.second);
    JSHandle<JSTaggedValue> ret = deserializer.DeserializeJSTaggedValue();
    EXPECT_TRUE(ret.IsEmpty());
    delete serializer;
};

// not support symbol
HWTEST_F_L0(JSSerializerTest, SerializeSymbolWithProperty)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSSymbol> jsSymbol = factory->NewJSSymbol();
    JSHandle<JSTaggedValue> key1(factory->NewFromCanBeCompressString("2"));
    JSHandle<JSTaggedValue> key2(factory->NewFromCanBeCompressString("x"));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(8));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsSymbol), key1, value1);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsSymbol), key2, value2);

    JSSerializer *serializer = new JSSerializer(thread);
    bool success = serializer->SerializeJSTaggedValue(JSHandle<JSTaggedValue>(jsSymbol));
    EXPECT_FALSE(success);
    std::pair<uint8_t *, size_t> data = serializer->ReleaseBuffer();
    JSDeserializer deserializer(thread, data.first, data.second);
    JSHandle<JSTaggedValue> ret = deserializer.DeserializeJSTaggedValue();
    EXPECT_TRUE(ret.IsEmpty());
    delete serializer;
};
}  // namespace panda::test
