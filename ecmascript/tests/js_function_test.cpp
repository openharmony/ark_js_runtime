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

#include "ecmascript/js_function.h"
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/interpreter/interpreter.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_array-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

using namespace panda::ecmascript::base;

namespace panda::test {
class JSFunctionTest : public testing::Test {
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

    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

JSFunction *JSObjectCreate(JSThread *thread)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    return globalEnv->GetObjectFunction().GetObject<JSFunction>();
}

HWTEST_F_L0(JSFunctionTest, Create)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSFunction> funHandle = thread->GetEcmaVM()->GetFactory()->NewJSFunction(env);
    EXPECT_TRUE(*funHandle != nullptr);
    EXPECT_EQ(funHandle->GetProtoOrDynClass(), JSTaggedValue::Hole());

    JSHandle<LexicalEnv> lexicalEnv = thread->GetEcmaVM()->GetFactory()->NewLexicalEnv(0);
    funHandle->SetLexicalEnv(thread, lexicalEnv.GetTaggedValue());
    EXPECT_EQ(funHandle->GetLexicalEnv(), lexicalEnv.GetTaggedValue());
    EXPECT_TRUE(*lexicalEnv != nullptr);
}
HWTEST_F_L0(JSFunctionTest, MakeConstructor)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSFunction> func = thread->GetEcmaVM()->GetFactory()->NewJSFunction(env, static_cast<void *>(nullptr),
                                                                                 FunctionKind::BASE_CONSTRUCTOR);
    EXPECT_TRUE(*func != nullptr);
    JSHandle<JSTaggedValue> funcHandle(func);
    func->GetJSHClass()->SetExtensible(true);

    JSHandle<JSObject> nullHandle(thread, JSTaggedValue::Null());
    JSHandle<JSObject> obj = JSObject::ObjectCreate(thread, nullHandle);
    JSHandle<JSTaggedValue> objValue(obj);

    JSFunction::MakeConstructor(thread, func, objValue);

    JSHandle<JSTaggedValue> constructorKey(
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("constructor"));

    JSHandle<JSTaggedValue> protoKey(thread->GetEcmaVM()->GetFactory()->NewFromASCII("prototype"));
    JSTaggedValue proto = JSObject::GetProperty(thread, funcHandle, protoKey).GetValue().GetTaggedValue();
    JSTaggedValue constructor =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(obj), constructorKey).GetValue().GetTaggedValue();
    EXPECT_EQ(constructor, funcHandle.GetTaggedValue());
    EXPECT_EQ(proto, obj.GetTaggedValue());
    EXPECT_EQ(func->GetFunctionKind(), FunctionKind::BASE_CONSTRUCTOR);
}

HWTEST_F_L0(JSFunctionTest, OrdinaryHasInstance)
{
    JSHandle<JSTaggedValue> objFun(thread, JSObjectCreate(thread));

    JSHandle<JSObject> jsobject =
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSHandle<JSTaggedValue> obj(thread, jsobject.GetTaggedValue());
    EXPECT_TRUE(*jsobject != nullptr);

    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> constructor = globalEnv->GetObjectFunction();
    EXPECT_TRUE(ecmascript::JSFunction::OrdinaryHasInstance(thread, constructor, obj));
}

JSTaggedValue TestInvokeInternal(EcmaRuntimeCallInfo *argv)
{
    if (argv->GetArgsNumber() == 1 && argv->GetCallArg(0).GetTaggedValue() == JSTaggedValue(1)) {
        return BuiltinsBase::GetTaggedBoolean(true);
    } else {
        return BuiltinsBase::GetTaggedBoolean(false);
    }
}

HWTEST_F_L0(JSFunctionTest, Invoke)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> dynclass(thread, JSObjectCreate(thread));
    JSHandle<JSTaggedValue> callee(
        thread->GetEcmaVM()->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(dynclass), dynclass));
    EXPECT_TRUE(*callee != nullptr);

    char keyArray[] = "invoked";
    JSHandle<JSTaggedValue> calleeKey(thread->GetEcmaVM()->GetFactory()->NewFromASCII(&keyArray[0]));
    JSHandle<JSFunction> calleeFunc =
        thread->GetEcmaVM()->GetFactory()->NewJSFunction(env, reinterpret_cast<void *>(TestInvokeInternal));
    calleeFunc->SetCallable(true);
    JSHandle<JSTaggedValue> calleeValue(calleeFunc);
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(callee), calleeKey, calleeValue);
    JSHandle<JSTaggedValue> undefined = thread->GlobalConstants()->GetHandledUndefined();
    EcmaRuntimeCallInfo *info = EcmaInterpreter::NewRuntimeCallInfo(thread, undefined, callee, undefined, 1);
    info->SetCallArg(JSTaggedValue(1));
    JSTaggedValue res = JSFunction::Invoke(info, calleeKey);

    JSTaggedValue ruler = BuiltinsBase::GetTaggedBoolean(true);
    EXPECT_EQ(res.GetRawData(), ruler.GetRawData());
}

HWTEST_F_L0(JSFunctionTest, SetSymbolFunctionName)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> jsFunction = factory->NewJSFunction(env);
    JSHandle<JSSymbol> symbol = factory->NewPublicSymbolWithChar("name");
    JSHandle<EcmaString> name = factory->NewFromASCII("[name]");
    JSHandle<JSTaggedValue> prefix(thread, JSTaggedValue::Undefined());
    JSFunction::SetFunctionName(thread, JSHandle<JSFunctionBase>(jsFunction), JSHandle<JSTaggedValue>(symbol), prefix);
    JSHandle<JSTaggedValue> functionName =
        JSFunctionBase::GetFunctionName(thread, JSHandle<JSFunctionBase>(jsFunction));
    EXPECT_TRUE(functionName->IsString());
    EXPECT_TRUE(EcmaString::StringsAreEqual(*(JSHandle<EcmaString>(functionName)), *name));
}
}  // namespace panda::test
