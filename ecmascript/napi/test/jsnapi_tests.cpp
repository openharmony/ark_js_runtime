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

#include "ecmascript/tests/test_helper.h"

#include <cstddef>
#include "ecmascript/builtins/builtins_function.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/napi/jsnapi_helper-inl.h"
#include "ecmascript/object_factory.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
using BuiltinsFunction = ecmascript::builtins::BuiltinsFunction;
class JSNApiTests : public testing::Test {
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
        RuntimeOption option;
        option.SetLogLevel(RuntimeOption::LOG_LEVEL::ERROR);
        vm_ = JSNApi::CreateJSVM(option);
        ASSERT_TRUE(vm_ != nullptr) << "Cannot create Runtime";
        thread_ = vm_->GetJSThread();
        vm_->SetEnableForceGC(true);
    }

    void TearDown() override
    {
        vm_->SetEnableForceGC(false);
        JSNApi::DestroyJSVM(vm_);
    }

protected:
    JSThread *thread_ = nullptr;
    EcmaVM *vm_ = nullptr;
};

Local<JSValueRef> FunctionCallback(EcmaVM *vm, Local<JSValueRef>, const Local<JSValueRef> *, int32_t length, void *)
{
    EscapeLocalScope scope(vm);
    return scope.Escape(ArrayRef::New(vm, length));
}

void ThreadCheck(const EcmaVM *vm)
{
    EXPECT_TRUE(vm->GetJSThread()->GetThreadId() != JSThread::GetCurrentThreadId());
}

HWTEST_F_L0(JSNApiTests, GetGlobalObject)
{
    LocalScope scope(vm_);
    Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm_);
    ASSERT_FALSE(globalObject.IsEmpty());
    ASSERT_TRUE(globalObject->IsObject());
}

HWTEST_F_L0(JSNApiTests, ThreadIdCheck)
{
    EXPECT_TRUE(vm_->GetJSThread()->GetThreadId() == JSThread::GetCurrentThreadId());
#if defined(ECMASCRIPT_ENABLE_THREAD_CHECK) && !ECMASCRIPT_ENABLE_THREAD_CHECK
    std::thread testThread(ThreadCheck, vm_);
    testThread.join();
#endif
}

HWTEST_F_L0(JSNApiTests, RegisterFunction)
{
    LocalScope scope(vm_);
    Local<FunctionRef> callback = FunctionRef::New(vm_, FunctionCallback, nullptr);
    ASSERT_TRUE(!callback.IsEmpty());
    std::vector<Local<JSValueRef>> arguments;
    arguments.emplace_back(JSValueRef::Undefined(vm_));
    Local<JSValueRef> result =
        callback->Call(vm_, JSValueRef::Undefined(vm_), arguments.data(), arguments.size());
    ASSERT_TRUE(result->IsArray(vm_));
    Local<ArrayRef> array(result);
    ASSERT_EQ(array->Length(vm_), arguments.size());
}

HWTEST_F_L0(JSNApiTests, GetProperty)
{
    LocalScope scope(vm_);
    Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm_);
    ASSERT_FALSE(globalObject.IsEmpty());
    ASSERT_TRUE(globalObject->IsObject());

    Local<ObjectRef> key = StringRef::NewFromUtf8(vm_, "Number");
    Local<ObjectRef> property = globalObject->Get(vm_, key);
    ASSERT_TRUE(property->IsFunction());
}

HWTEST_F_L0(JSNApiTests, SetProperty)
{
    LocalScope scope(vm_);
    Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm_);
    ASSERT_FALSE(globalObject.IsEmpty());
    ASSERT_TRUE(globalObject->IsObject());

    Local<ArrayRef> property = ArrayRef::New(vm_, 3); // 3 : length
    ASSERT_TRUE(property->IsArray(vm_));
    ASSERT_EQ(property->Length(vm_), 3); // 3 : test case of input

    Local<ObjectRef> key = StringRef::NewFromUtf8(vm_, "Test");
    bool result = globalObject->Set(vm_, key, property);
    ASSERT_TRUE(result);

    Local<ObjectRef> propertyGet = globalObject->Get(vm_, key);
    ASSERT_TRUE(propertyGet->IsArray(vm_));
    ASSERT_EQ(Local<ArrayRef>(propertyGet)->Length(vm_), 3); // 3 : test case of input
}

HWTEST_F_L0(JSNApiTests, JsonParser)
{
    LocalScope scope(vm_);
    Local<ObjectRef> globalObject = JSNApi::GetGlobalObject(vm_);
    ASSERT_FALSE(globalObject.IsEmpty());
    ASSERT_TRUE(globalObject->IsObject());

    const char *const test{R"({"orientation": "portrait"})"};
    Local<ObjectRef> jsonString = StringRef::NewFromUtf8(vm_, test);

    Local<JSValueRef> result = JSON::Parse(vm_, jsonString);
    ASSERT_TRUE(result->IsObject());

    Local<ObjectRef> keyString = StringRef::NewFromUtf8(vm_, "orientation");
    Local<JSValueRef> property = Local<ObjectRef>(result)->Get(vm_, keyString);
    ASSERT_TRUE(property->IsString());
}

HWTEST_F_L0(JSNApiTests, StrictEqual)
{
    LocalScope scope(vm_);
    Local<StringRef> origin = StringRef::NewFromUtf8(vm_, "1");
    Local<StringRef> target1 = StringRef::NewFromUtf8(vm_, "1");
    Local<NumberRef> target = NumberRef::New(vm_, 1);

    ASSERT_FALSE(origin->IsStrictEquals(vm_, target));
    ASSERT_TRUE(origin->IsStrictEquals(vm_, target1));
}

HWTEST_F_L0(JSNApiTests, InstanceOf)
{
    LocalScope scope(vm_);
    Local<FunctionRef> target = FunctionRef::New(vm_, nullptr, nullptr);
    Local<ArrayRef> origin = ArrayRef::New(vm_, 1);

    ASSERT_FALSE(origin->InstanceOf(vm_, target));
}

HWTEST_F_L0(JSNApiTests, TypeOf)
{
    LocalScope scope(vm_);
    Local<StringRef> origin = StringRef::NewFromUtf8(vm_, "1");
    Local<StringRef> typeString = origin->Typeof(vm_);
    ASSERT_EQ(typeString->ToString(), "string");

    Local<NumberRef> target = NumberRef::New(vm_, 1);
    typeString = target->Typeof(vm_);
    ASSERT_EQ(typeString->ToString(), "number");
}

HWTEST_F_L0(JSNApiTests, Symbol)
{
    LocalScope scope(vm_);
    Local<StringRef> description = StringRef::NewFromUtf8(vm_, "test");
    Local<SymbolRef> symbol = SymbolRef::New(vm_, description);

    ASSERT_FALSE(description->IsSymbol());
    ASSERT_TRUE(symbol->IsSymbol());
}

HWTEST_F_L0(JSNApiTests, StringUtf8_001)
{
    LocalScope scope(vm_);
    std::string test = "Hello world";
    Local<StringRef> testString = StringRef::NewFromUtf8(vm_, test.c_str());

    EXPECT_EQ(testString->Utf8Length(), 12);          // 12 : length of testString("Hello World")
    char buffer[12];                                      // 12 : length of testString
    EXPECT_EQ(testString->WriteUtf8(buffer, 12), 12); // 12 : length of testString("Hello World")
    std::string res(buffer);
    ASSERT_EQ(res, test);
}

HWTEST_F_L0(JSNApiTests, StringUtf8_002)
{
    LocalScope scope(vm_);
    std::string test = "年";
    Local<StringRef> testString = StringRef::NewFromUtf8(vm_, test.c_str());

    EXPECT_EQ(testString->Utf8Length(), 4);          // 4 : length of testString("年")
    char buffer[4];                                      // 4 : length of testString
    EXPECT_EQ(testString->WriteUtf8(buffer, 4), 4); // 4 : length of testString("年")
    std::string res(buffer);
    ASSERT_EQ(res, test);
}

HWTEST_F_L0(JSNApiTests, ToType)
{
    LocalScope scope(vm_);
    Local<StringRef> toString = StringRef::NewFromUtf8(vm_, "-123.3");
    Local<JSValueRef> toValue(toString);

    ASSERT_EQ(toString->ToNumber(vm_)->Value(), -123.3); // -123 : test case of input
    ASSERT_EQ(toString->ToBoolean(vm_)->Value(), true);
    ASSERT_EQ(toValue->ToString(vm_)->ToString(), "-123.3");
    ASSERT_TRUE(toValue->ToObject(vm_)->IsObject());
}

HWTEST_F_L0(JSNApiTests, TypeValue)
{
    LocalScope scope(vm_);
    Local<StringRef> toString = StringRef::NewFromUtf8(vm_, "-123");
    Local<JSValueRef> toValue(toString);

    ASSERT_EQ(toString->Int32Value(vm_), -123);        // -123 : test case of input
    ASSERT_EQ(toString->BooleaValue(), true);
    ASSERT_EQ(toString->Uint32Value(vm_), 4294967173); // 4294967173 : test case of input
    ASSERT_EQ(toString->IntegerValue(vm_), -123);      // -123 : test case of input
}

HWTEST_F_L0(JSNApiTests, DefineProperty)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute attribute(value, true, true, true);

    ASSERT_TRUE(object->DefineProperty(vm_, key, attribute));
    Local<JSValueRef> value1 = object->Get(vm_, key);
    ASSERT_TRUE(value->IsStrictEquals(vm_, value1));
}

HWTEST_F_L0(JSNApiTests, HasProperty)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute attribute(value, true, true, true);

    ASSERT_TRUE(object->DefineProperty(vm_, key, attribute));
    ASSERT_TRUE(object->Has(vm_, key));
}

HWTEST_F_L0(JSNApiTests, DeleteProperty)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = ObjectRef::New(vm_);
    Local<JSValueRef> key = StringRef::NewFromUtf8(vm_, "TestKey");
    Local<JSValueRef> value = ObjectRef::New(vm_);
    PropertyAttribute attribute(value, true, true, true);

    ASSERT_TRUE(object->DefineProperty(vm_, key, attribute));
    ASSERT_TRUE(object->Delete(vm_, key));
    ASSERT_FALSE(object->Has(vm_, key));
}

HWTEST_F_L0(JSNApiTests, GetProtoType)
{
    LocalScope scope(vm_);
    Local<FunctionRef> function = FunctionRef::New(vm_, nullptr, nullptr);
    Local<JSValueRef> protoType = function->GetPrototype(vm_);
    ASSERT_TRUE(protoType->IsObject());

    Local<FunctionRef> object = ObjectRef::New(vm_);
    protoType = object->GetPrototype(vm_);
    ASSERT_TRUE(protoType->IsObject());
}

void CheckReject(EcmaVM *, Local<JSValueRef>, const Local<JSValueRef> argv[], int32_t length, void *)
{
    ASSERT_EQ(length, 1);
    Local<JSValueRef> reason = argv[0];
    ASSERT_TRUE(reason->IsString());
    ASSERT_EQ(Local<StringRef>(reason)->ToString(), "Reject");
}

Local<JSValueRef> RejectCallback(EcmaVM *vm, Local<JSValueRef> thisArg, const Local<JSValueRef> argv[], int32_t length,
                                 void *data)
{
    LocalScope scope(vm);
    CheckReject(vm, thisArg, argv, length, data);
    return JSValueRef::Undefined(vm);
}

HWTEST_F_L0(JSNApiTests, PromiseCatch)
{
    LocalScope scope(vm_);
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);

    Local<PromiseRef> promise = capability->GetPromise(vm_);
    Local<FunctionRef> reject = FunctionRef::New(vm_, RejectCallback, nullptr);
    Local<PromiseRef> catchPromise = promise->Catch(vm_, reject);
    ASSERT_TRUE(promise->IsPromise());
    ASSERT_TRUE(catchPromise->IsPromise());

    Local<StringRef> reason = StringRef::NewFromUtf8(vm_, "Reject");
    ASSERT_TRUE(capability->Reject(vm_, reason));

    vm_->ExecutePromisePendingJob();
}

void CheckResolve(EcmaVM *, Local<JSValueRef>, const Local<JSValueRef> argv[], int32_t length, void *)
{
    ASSERT_EQ(length, 1);
    Local<JSValueRef> value = argv[0];
    ASSERT_TRUE(value->IsNumber());
    ASSERT_EQ(Local<NumberRef>(value)->Value(), 300.3); // 300.3 : test case of input
}

Local<JSValueRef> ResolvedCallback(EcmaVM *vm, Local<JSValueRef> thisArg, const Local<JSValueRef> argv[],
                                   int32_t length, void *data)
{
    LocalScope scope(vm);
    CheckResolve(vm, thisArg, argv, length, data);
    return JSValueRef::Undefined(vm);
}

HWTEST_F_L0(JSNApiTests, PromiseThen)
{
    LocalScope scope(vm_);
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);

    Local<PromiseRef> promise = capability->GetPromise(vm_);
    Local<FunctionRef> resolve = FunctionRef::New(vm_, ResolvedCallback, nullptr);
    Local<FunctionRef> reject = FunctionRef::New(vm_, RejectCallback, nullptr);
    Local<PromiseRef> thenPromise = promise->Then(vm_, resolve, reject);
    ASSERT_TRUE(promise->IsPromise());
    ASSERT_TRUE(thenPromise->IsPromise());

    Local<StringRef> value = NumberRef::New(vm_, 300.3); // 300.3 : test case of input
    ASSERT_TRUE(capability->Resolve(vm_, value));
    vm_->ExecutePromisePendingJob();
}

HWTEST_F_L0(JSNApiTests, Constructor)
{
    LocalScope scope(vm_);
    Local<ObjectRef> object = JSNApi::GetGlobalObject(vm_);
    Local<StringRef> key = StringRef::NewFromUtf8(vm_, "Number");
    Local<FunctionRef> numberConstructor = object->Get(vm_, key);
    Local<JSValueRef> argv[1];
    argv[0] = NumberRef::New(vm_, 1.3); // 1.3 : test case of input
    Local<JSValueRef> result = numberConstructor->Constructor(vm_, argv, 1);
    ASSERT_TRUE(result->IsObject());
    ASSERT_EQ(result->ToNumber(vm_)->Value(), 1.3); // 1.3 : size of arguments
}

HWTEST_F_L0(JSNApiTests, ArrayBuffer)
{
    LocalScope scope(vm_);
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());
    ASSERT_EQ(arrayBuffer->ByteLength(vm_), length);
    ASSERT_NE(arrayBuffer->GetBuffer(), nullptr);
    JSNApi::TriggerGC(vm_);
}

HWTEST_F_L0(JSNApiTests, ArrayBufferWithBuffer)
{
    static bool isFree = false;
    struct Data {
        int32_t length;
    };
    const int32_t length = 15;
    Data *data = new Data();
    data->length = length;
    Deleter deleter = [](void *buffer, void *data) -> void {
        delete[] reinterpret_cast<uint8_t *>(buffer);
        Data *currentData = reinterpret_cast<Data *>(data);
        ASSERT_EQ(currentData->length, 15); // 5 : size of arguments
        delete currentData;
        isFree = true;
    };
    {
        LocalScope scope(vm_);
        uint8_t *buffer = new uint8_t[length]();
        Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, buffer, length, deleter, data);
        ASSERT_TRUE(arrayBuffer->IsArrayBuffer());
        ASSERT_EQ(arrayBuffer->ByteLength(vm_), length);
        ASSERT_EQ(arrayBuffer->GetBuffer(), buffer);
    }
    JSNApi::TriggerGC(vm_, JSNApi::TRIGGER_GC_TYPE::FULL_GC);
    ASSERT_TRUE(isFree);
}

HWTEST_F_L0(JSNApiTests, DataView)
{
    LocalScope scope(vm_);
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    JSNApi::TriggerGC(vm_);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 5 : offset of byte, 7 : length
    Local<DataViewRef> dataView = DataViewRef::New(vm_, arrayBuffer, 5, 7);
    ASSERT_TRUE(dataView->IsDataView());
    ASSERT_EQ(dataView->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
    ASSERT_EQ(dataView->ByteLength(), 7); // 7 : size of arguments
    ASSERT_EQ(dataView->ByteOffset(), 5); // 5 : size of arguments

    // 5 : offset of byte, 11 : length
    dataView = DataViewRef::New(vm_, arrayBuffer, 5, 11);
    ASSERT_TRUE(dataView->IsException());
}

HWTEST_F_L0(JSNApiTests, Int8Array)
{
    LocalScope scope(vm_);
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 5 : offset of byte, 6 : length
    Local<Int8ArrayRef> typedArray = Int8ArrayRef::New(vm_, arrayBuffer, 5, 6);
    ASSERT_TRUE(typedArray->IsInt8Array());
    ASSERT_EQ(typedArray->ByteLength(vm_), 6); // 6 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 5); // 5 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, Uint8Array)
{
    LocalScope scope(vm_);
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 5 : offset of byte, 6 : length
    Local<Uint8ArrayRef> typedArray = Uint8ArrayRef::New(vm_, arrayBuffer, 5, 6);
    ASSERT_TRUE(typedArray->IsUint8Array());
    ASSERT_EQ(typedArray->ByteLength(vm_), 6);  // 6 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 5);  // 5 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, Uint8ClampedArray)
{
    LocalScope scope(vm_);
    const int32_t length = 15;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 5 : offset of byte, 6 : length
    Local<Uint8ClampedArrayRef> typedArray = Uint8ClampedArrayRef::New(vm_, arrayBuffer, 5, 6);
    ASSERT_TRUE(typedArray->IsUint8ClampedArray());
    ASSERT_EQ(typedArray->ByteLength(vm_), 6);  // 6 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 5);  // 5 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6); // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, Int16Array)
{
    LocalScope scope(vm_);
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 4 : offset of byte, 6 : length
    Local<Int16ArrayRef> typedArray = Int16ArrayRef::New(vm_, arrayBuffer, 4, 6);
    ASSERT_TRUE(typedArray->IsInt16Array());
    ASSERT_EQ(typedArray->ByteLength(vm_), 12);  // 12 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 4);   // 4 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6);  // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, Uint16Array)
{
    LocalScope scope(vm_);
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 4 : offset of byte, 6 : length
    Local<Uint16ArrayRef> typedArray = Uint16ArrayRef::New(vm_, arrayBuffer, 4, 6);
    ASSERT_TRUE(typedArray->IsUint16Array());
    ASSERT_EQ(typedArray->ByteLength(vm_), 12);  // 12 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 4);   // 4 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6);  // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, Uint32Array)
{
    LocalScope scope(vm_);
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 4 : offset of byte, 6 : length
    Local<Uint32ArrayRef> typedArray = Uint32ArrayRef::New(vm_, arrayBuffer, 4, 6);
    ASSERT_TRUE(typedArray->IsUint32Array());
    ASSERT_EQ(typedArray->ByteLength(vm_), 24);  // 24 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 4);   // 4 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6);  // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, Int32Array)
{
    LocalScope scope(vm_);
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 4 : offset of byte, 6 : length
    Local<Int32ArrayRef> typedArray = Int32ArrayRef::New(vm_, arrayBuffer, 4, 6);
    ASSERT_TRUE(typedArray->IsInt32Array());
    ASSERT_EQ(typedArray->ByteLength(vm_), 24);  // 24 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 4);   // 4 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6);  // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, Float32Array)
{
    LocalScope scope(vm_);
    const int32_t length = 30;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 4 : offset of byte, 6 : length
    Local<Float32ArrayRef> typedArray = Float32ArrayRef::New(vm_, arrayBuffer, 4, 6);
    ASSERT_TRUE(typedArray->IsFloat32Array());
    ASSERT_EQ(typedArray->ByteLength(vm_), 24);  // 24 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 4);   // 4 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6);  // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, Float64Array)
{
    LocalScope scope(vm_);
    const int32_t length = 57;
    Local<ArrayBufferRef> arrayBuffer = ArrayBufferRef::New(vm_, length);
    ASSERT_TRUE(arrayBuffer->IsArrayBuffer());

    // 8 : offset of byte, 6 : length
    Local<Float64ArrayRef> typedArray = Float64ArrayRef::New(vm_, arrayBuffer, 8, 6);
    ASSERT_TRUE(typedArray->IsFloat64Array());
    ASSERT_EQ(typedArray->ByteLength(vm_), 48);  // 48 : length of bytes
    ASSERT_EQ(typedArray->ByteOffset(vm_), 8);   // 8 : offset of byte
    ASSERT_EQ(typedArray->ArrayLength(vm_), 6);  // 6 : length of array
    ASSERT_EQ(typedArray->GetArrayBuffer(vm_)->GetBuffer(), arrayBuffer->GetBuffer());
}

HWTEST_F_L0(JSNApiTests, Error)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::Error(vm_, message);
    ASSERT_TRUE(error->IsError());

    JSNApi::ThrowException(vm_, error);
    ASSERT_TRUE(thread_->HasPendingException());
}

HWTEST_F_L0(JSNApiTests, RangeError)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::RangeError(vm_, message);
    ASSERT_TRUE(error->IsError());

    JSNApi::ThrowException(vm_, error);
    ASSERT_TRUE(thread_->HasPendingException());
}

HWTEST_F_L0(JSNApiTests, TypeError)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::TypeError(vm_, message);
    ASSERT_TRUE(error->IsError());

    JSNApi::ThrowException(vm_, error);
    ASSERT_TRUE(thread_->HasPendingException());
}

HWTEST_F_L0(JSNApiTests, ReferenceError)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::ReferenceError(vm_, message);
    ASSERT_TRUE(error->IsError());

    JSNApi::ThrowException(vm_, error);
    ASSERT_TRUE(thread_->HasPendingException());
}

HWTEST_F_L0(JSNApiTests, SyntaxError)
{
    LocalScope scope(vm_);
    Local<StringRef> message = StringRef::NewFromUtf8(vm_, "ErrorTest");
    Local<JSValueRef> error = Exception::SyntaxError(vm_, message);
    ASSERT_TRUE(error->IsError());

    JSNApi::ThrowException(vm_, error);
    ASSERT_TRUE(thread_->HasPendingException());
}

HWTEST_F_L0(JSNApiTests, InheritPrototype_001)
{
    LocalScope scope(vm_);
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    // new with Builtins::Set Prototype
    JSHandle<JSTaggedValue> set = env->GetBuiltinsSetFunction();
    Local<FunctionRef> setLocal = JSNApiHelper::ToLocal<FunctionRef>(set);
    // new with Builtins::Map Prototype
    JSHandle<JSTaggedValue> map = env->GetBuiltinsMapFunction();
    Local<FunctionRef> mapLocal = JSNApiHelper::ToLocal<FunctionRef>(map);
    JSHandle<JSTaggedValue> setPrototype(thread_, JSHandle<JSFunction>::Cast(set)->GetFunctionPrototype());
    JSHandle<JSTaggedValue> mapPrototype(thread_, JSHandle<JSFunction>::Cast(map)->GetFunctionPrototype());
    JSHandle<JSTaggedValue> mapPrototypeProto(thread_, JSHandle<JSObject>::Cast(mapPrototype)->GetPrototype(thread_));
    bool same = JSTaggedValue::SameValue(setPrototype, mapPrototypeProto);
    // before inherit, map.Prototype.__proto__ should be different from set.Prototype
    ASSERT_FALSE(same);
    // before inherit, map.__proto__ should be different from set
    JSHandle<JSTaggedValue> mapProto(thread_, JSHandle<JSObject>::Cast(map)->GetPrototype(thread_));
    bool same1 = JSTaggedValue::SameValue(set, mapProto);
    ASSERT_FALSE(same1);

    // Set property to Set Function
    auto factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> defaultString = thread_->GlobalConstants()->GetHandledDefaultString();
    PropertyDescriptor desc = PropertyDescriptor(thread_, defaultString);
    bool success = JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(set), defaultString, desc);
    ASSERT_TRUE(success);
    JSHandle<JSTaggedValue> property1String(thread_, factory->NewFromCanBeCompressString("property1").GetTaggedValue());
    JSHandle<JSTaggedValue> func = env->GetTypedArrayFunction();
    PropertyDescriptor desc1 = PropertyDescriptor(thread_, func);
    bool success1 = JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(set), property1String, desc1);
    ASSERT_TRUE(success1);

    mapLocal->Inherit(vm_, setLocal);
    JSHandle<JSTaggedValue> sonHandle = JSNApiHelper::ToJSHandle(mapLocal);
    JSHandle<JSTaggedValue> sonPrototype(thread_, JSHandle<JSFunction>::Cast(sonHandle)->GetFunctionPrototype());
    JSHandle<JSTaggedValue> sonPrototypeProto(thread_, JSHandle<JSObject>::Cast(sonPrototype)->GetPrototype(thread_));
    bool same2 = JSTaggedValue::SameValue(setPrototype, sonPrototypeProto);
    ASSERT_TRUE(same2);
    JSHandle<JSTaggedValue> sonProto(thread_, JSHandle<JSObject>::Cast(map)->GetPrototype(thread_));
    bool same3 = JSTaggedValue::SameValue(set, sonProto);
    ASSERT_TRUE(same3);

    // son = new Son(), Son() inherit from Parent(), Test whether son.InstanceOf(Parent) is true
    JSHandle<JSObject> sonObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(sonHandle), sonHandle);
    bool isInstance = JSObject::InstanceOf(thread_, JSHandle<JSTaggedValue>::Cast(sonObj), set);
    ASSERT_TRUE(isInstance);

    // Test whether son Function can access to property of parent Function
    OperationResult res = JSObject::GetProperty(thread_, JSHandle<JSObject>::Cast(sonHandle), defaultString);
    bool same4 = JSTaggedValue::SameValue(defaultString, res.GetValue());
    ASSERT_TRUE(same4);
    OperationResult res1 = JSObject::GetProperty(thread_, JSHandle<JSObject>::Cast(sonHandle), property1String);
    bool same5 = JSTaggedValue::SameValue(func, res1.GetValue());
    ASSERT_TRUE(same5);

    // new with empty Function Constructor
    Local<FunctionRef> son1 = FunctionRef::New(vm_, FunctionCallback, nullptr);
    son1->Inherit(vm_, mapLocal);
    JSHandle<JSFunction> son1Handle = JSHandle<JSFunction>::Cast(JSNApiHelper::ToJSHandle(son1));
    ASSERT_TRUE(son1Handle->HasFunctionPrototype());
}

HWTEST_F_L0(JSNApiTests, InheritPrototype_002)
{
    LocalScope scope(vm_);
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    // new with Builtins::weakSet Prototype
    JSHandle<JSTaggedValue> weakSet = env->GetBuiltinsWeakSetFunction();
    Local<FunctionRef> weakSetLocal = JSNApiHelper::ToLocal<FunctionRef>(weakSet);
    // new with Builtins::weakMap Prototype
    JSHandle<JSTaggedValue> weakMap = env->GetBuiltinsWeakMapFunction();
    Local<FunctionRef> weakMapLocal = JSNApiHelper::ToLocal<FunctionRef>(weakMap);

    weakMapLocal->Inherit(vm_, weakSetLocal);

    auto factory = vm_->GetFactory();
    JSHandle<JSTaggedValue> property1String(thread_, factory->NewFromCanBeCompressString("property1").GetTaggedValue());
    JSHandle<JSTaggedValue> func = env->GetArrayFunction();
    PropertyDescriptor desc1 = PropertyDescriptor(thread_, func);
    bool success1 = JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(weakMap), property1String, desc1);
    ASSERT_TRUE(success1);

    JSHandle<JSTaggedValue> sonHandle = JSNApiHelper::ToJSHandle(weakMapLocal);
    JSHandle<JSObject> sonObj =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(sonHandle), sonHandle);

    JSHandle<JSTaggedValue> fatherHandle = JSNApiHelper::ToJSHandle(weakSetLocal);
    JSHandle<JSObject> fatherObj =
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(fatherHandle), fatherHandle);

    JSHandle<JSTaggedValue> sonMethod =
        JSObject::GetMethod(thread_, JSHandle<JSTaggedValue>(sonObj), property1String);
    JSHandle<JSTaggedValue> fatherMethod =
        JSObject::GetMethod(thread_, JSHandle<JSTaggedValue>(fatherObj), property1String);
    bool same = JSTaggedValue::SameValue(sonMethod, fatherMethod);
    ASSERT_TRUE(same);
}

HWTEST_F_L0(JSNApiTests, InheritPrototype_003)
{
    LocalScope scope(vm_);
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    auto factory = vm_->GetFactory();

    JSMethod *invokeSelf =
        vm_->GetMethodForNativeFunction(reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf));
    // father type
    JSHandle<JSHClass> protoDynclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithProto());
    JSHandle<JSFunction> protoFunc = factory->NewJSFunctionByDynClass(invokeSelf, protoDynclass);
    Local<FunctionRef> protoLocal = JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(protoFunc));
    // son type
    JSHandle<JSHClass> noProtoDynclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
    JSHandle<JSFunction> noProtoFunc = factory->NewJSFunctionByDynClass(invokeSelf, noProtoDynclass);
    Local<FunctionRef> noProtoLocal = JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(noProtoFunc));

    JSHandle<JSFunction> sonHandle = JSHandle<JSFunction>::Cast(JSNApiHelper::ToJSHandle(noProtoLocal));
    EXPECT_FALSE(sonHandle->HasFunctionPrototype());

    JSHandle<JSTaggedValue> defaultString = thread_->GlobalConstants()->GetHandledDefaultString();
    PropertyDescriptor desc = PropertyDescriptor(thread_, defaultString);
    JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(protoFunc), defaultString, desc);

    noProtoLocal->Inherit(vm_, protoLocal);
    JSHandle<JSFunction> son1Handle = JSHandle<JSFunction>::Cast(JSNApiHelper::ToJSHandle(noProtoLocal));
    EXPECT_TRUE(son1Handle->HasFunctionPrototype());

    OperationResult res = JSObject::GetProperty(thread_, JSHandle<JSObject>::Cast(son1Handle), defaultString);
    EXPECT_EQ(JSTaggedValue::SameValue(defaultString, res.GetValue()), true);

    JSHandle<JSTaggedValue> propertyString(thread_, factory->NewFromCanBeCompressString("property").GetTaggedValue());
    JSHandle<JSTaggedValue> func = env->GetArrayFunction();
    PropertyDescriptor desc1 = PropertyDescriptor(thread_, func);
    JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(protoFunc), propertyString, desc1);
    OperationResult res1 = JSObject::GetProperty(thread_, JSHandle<JSObject>::Cast(son1Handle), propertyString);
    EXPECT_EQ(JSTaggedValue::SameValue(func, res1.GetValue()), true);
}

HWTEST_F_L0(JSNApiTests, InheritPrototype_004)
{
    LocalScope scope(vm_);
    JSHandle<GlobalEnv> env = vm_->GetGlobalEnv();
    auto factory = vm_->GetFactory();

    JSHandle<JSTaggedValue> weakSet = env->GetBuiltinsWeakSetFunction();
    JSHandle<JSTaggedValue> deleteString(factory->NewFromCanBeCompressString("delete"));
    JSHandle<JSTaggedValue> addString(factory->NewFromCanBeCompressString("add"));
    JSHandle<JSTaggedValue> defaultString = thread_->GlobalConstants()->GetHandledDefaultString();
    JSHandle<JSTaggedValue> deleteMethod = JSObject::GetMethod(thread_, weakSet, deleteString);
    JSHandle<JSTaggedValue> addMethod = JSObject::GetMethod(thread_, weakSet, addString);

    JSMethod *invokeSelf =
        vm_->GetMethodForNativeFunction(reinterpret_cast<void *>(BuiltinsFunction::FunctionPrototypeInvokeSelf));
    JSMethod *ctor = vm_->GetMethodForNativeFunction(reinterpret_cast<void *>(BuiltinsFunction::FunctionConstructor));

    JSHandle<JSHClass> protoDynclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithProto());
    JSHandle<JSFunction> funcFuncPrototype = factory->NewJSFunctionByDynClass(invokeSelf, protoDynclass);
    // add method in funcPrototype
    PropertyDescriptor desc = PropertyDescriptor(thread_, deleteMethod);
    JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(funcFuncPrototype), deleteString, desc);
    JSHandle<JSTaggedValue> funcFuncPrototypeValue(funcFuncPrototype);

    JSHandle<JSHClass> funcFuncProtoIntanceDynclass =
       factory->NewEcmaDynClass(JSFunction::SIZE, JSType::JS_FUNCTION, funcFuncPrototypeValue);
    // new with NewJSFunctionByDynClass::function DynClass
    JSHandle<JSFunction> protoFunc =
       factory->NewJSFunctionByDynClass(ctor, funcFuncProtoIntanceDynclass, FunctionKind::BUILTIN_CONSTRUCTOR);
    EXPECT_TRUE(*protoFunc != nullptr);
    // add method in funcnction
    PropertyDescriptor desc1 = PropertyDescriptor(thread_, addMethod);
    JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(protoFunc), addString, desc1);
    JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(protoFunc), deleteString, desc);
    // father type
    Local<FunctionRef> protoLocal = JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(protoFunc));

    JSHandle<JSHClass> noProtoDynclass = JSHandle<JSHClass>::Cast(env->GetFunctionClassWithoutProto());
    JSHandle<JSFunction> funcFuncNoProtoPrototype = factory->NewJSFunctionByDynClass(invokeSelf, noProtoDynclass);
    JSHandle<JSTaggedValue> funcFuncNoProtoPrototypeValue(funcFuncNoProtoPrototype);

    JSHandle<JSHClass> funcFuncNoProtoProtoIntanceDynclass =
       factory->NewEcmaDynClass(JSFunction::SIZE, JSType::JS_FUNCTION, funcFuncNoProtoPrototypeValue);
    // new with NewJSFunctionByDynClass::function DynClass
    JSHandle<JSFunction> noProtoFunc =
       factory->NewJSFunctionByDynClass(ctor, funcFuncNoProtoProtoIntanceDynclass, FunctionKind::BUILTIN_CONSTRUCTOR);
    EXPECT_TRUE(*noProtoFunc != nullptr);
    // set property that has same key with fater type
    PropertyDescriptor desc2 = PropertyDescriptor(thread_, defaultString);
    JSObject::DefineOwnProperty(thread_, JSHandle<JSObject>::Cast(noProtoFunc), addString, desc2);
    // son type
    Local<FunctionRef> noProtoLocal = JSNApiHelper::ToLocal<FunctionRef>(JSHandle<JSTaggedValue>(noProtoFunc));

    noProtoLocal->Inherit(vm_, protoLocal);

    JSHandle<JSFunction> sonHandle = JSHandle<JSFunction>::Cast(JSNApiHelper::ToJSHandle(noProtoLocal));
    OperationResult res = JSObject::GetProperty(thread_, JSHandle<JSObject>::Cast(sonHandle), deleteString);
    EXPECT_EQ(JSTaggedValue::SameValue(deleteMethod, res.GetValue()), true);
    // test if the property value changed after inherit
    OperationResult res1 = JSObject::GetProperty(thread_, JSHandle<JSObject>::Cast(sonHandle), addString);
    EXPECT_EQ(JSTaggedValue::SameValue(defaultString, res1.GetValue()), true);
}

HWTEST_F_L0(JSNApiTests, ClassFunction)
{
    LocalScope scope(vm_);
    Local<FunctionRef> cls = FunctionRef::NewClassFunction(vm_, nullptr, nullptr, nullptr);

    JSHandle<JSTaggedValue> clsObj = JSNApiHelper::ToJSHandle(Local<JSValueRef>(cls));
    ASSERT_TRUE(clsObj->IsClassConstructor());

    JSTaggedValue accessor = JSHandle<JSFunction>(clsObj)->GetPropertyInlinedProps(
                                                           JSFunction::CLASS_PROTOTYPE_INLINE_PROPERTY_INDEX);
    ASSERT_TRUE(accessor.IsInternalAccessor());
}
}  // namespace panda::test