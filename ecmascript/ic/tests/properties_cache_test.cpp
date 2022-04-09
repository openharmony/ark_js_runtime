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

#include "ecmascript/ic/properties_cache.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class PropertiesCacheTest : public testing::Test {
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

/**
 * @tc.name: SetAndGet
 * @tc.desc: Creating PropertiesCache through "GetPropertiesCache" function,Set two values through "Set" function,
 *           one is string and the other is symbol, and then check whether it is correct through the "Get" function.
 * @tc.type: FUNC
 * @tc.requre:
 */
HWTEST_F_L0(PropertiesCacheTest, SetAndGet)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> handleFunction(factory->NewJSFunction(env));
    JSHandle<JSSymbol> handleSymbol(factory->NewJSSymbol());
    JSHandle<JSTaggedValue> handleKey10(factory->NewFromASCII("10"));
    JSHClass *FuncClass = JSObject::Cast(handleFunction->GetHeapObject())->GetJSHClass();

    PropertiesCache *handleProCache = thread->GetPropertiesCache();
    // key is string
    for (int i = 0; i < 10; i++) {
        JSHandle<JSTaggedValue> handleNumKey(thread, JSTaggedValue(1));
        JSHandle<JSTaggedValue> handleStrKey(JSTaggedValue::ToString(thread, handleNumKey));
        handleProCache->Set(FuncClass, handleStrKey.GetTaggedValue(), i);
        EXPECT_EQ(handleProCache->Get(FuncClass, handleStrKey.GetTaggedValue()), i);
    }
    EXPECT_EQ(handleProCache->Get(FuncClass, handleKey10.GetTaggedValue()), -1); // PropertiesCache::NOT_FOUND
    // key is symbol
    for (int i = 0; i < 10; i++) {
        handleSymbol->SetHashField(static_cast<uint32_t>(i));
        handleProCache->Set(FuncClass, handleSymbol.GetTaggedValue(), i);
        EXPECT_EQ(handleProCache->Get(FuncClass, handleSymbol.GetTaggedValue()), i);
    }
    handleSymbol->SetHashField(static_cast<uint32_t>(10));
    EXPECT_EQ(handleProCache->Get(FuncClass, handleSymbol.GetTaggedValue()), -1); // PropertiesCache::NOT_FOUND
    handleProCache->Clear();
}

/**
 * @tc.name: Clear
 * @tc.desc: Creating PropertiesCache through "GetPropertiesCache" function,Set two values through "Set" function,
 *           then call "Clear" function,check the returned value through "get" function is within expectations.
 * @tc.type: FUNC
 * @tc.requre:
 */
HWTEST_F_L0(PropertiesCacheTest, Clear)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    
    JSHandle<JSTaggedValue> handleKey(factory->NewFromASCII("10"));
    JSHandle<JSTaggedValue> handleFunction(factory->NewJSFunction(env));
    JSHClass *FuncClass = JSObject::Cast(handleFunction->GetHeapObject())->GetJSHClass();
    PropertiesCache *handleProCache = thread->GetPropertiesCache();

    handleProCache->Set(FuncClass, handleKey.GetTaggedValue(), 10);
    EXPECT_EQ(handleProCache->Get(FuncClass, handleKey.GetTaggedValue()), 10);
    handleProCache->Clear();
    EXPECT_EQ(handleProCache->Get(FuncClass, handleKey.GetTaggedValue()), -1); // PropertiesCache::NOT_FOUND
}
} // namespace panda::test
