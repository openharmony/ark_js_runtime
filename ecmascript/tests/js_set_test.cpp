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

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_hash_table.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class JSSetTest : public testing::Test {
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
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};

protected:
    JSSet *CreateSet()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> constructor = env->GetBuiltinsSetFunction();
        JSHandle<JSSet> set =
            JSHandle<JSSet>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<LinkedHashSet> hashSet = LinkedHashSet::Create(thread);
        set->SetLinkedSet(thread, hashSet);
        return JSSet::Cast(set.GetTaggedValue().GetTaggedObject());
    }
};

HWTEST_F_L0(JSSetTest, SetCreate)
{
    JSSet *set = CreateSet();
    EXPECT_TRUE(set != nullptr);
}

HWTEST_F_L0(JSSetTest, AddAndHas)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create jsSet
    JSHandle<JSSet> set(thread, CreateSet());

    JSHandle<JSTaggedValue> key(factory->NewFromASCII("key"));
    JSSet::Add(thread, set, key);
    EXPECT_TRUE(set->Has(key.GetTaggedValue()));
}

HWTEST_F_L0(JSSetTest, DeleteAndGet)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // create jsSet
    JSHandle<JSSet> set(thread, CreateSet());

    // add 40 keys
    char keyArray[] = "key0";
    for (uint32_t i = 0; i < 40; i++) {
        keyArray[3] = '1' + i;
        JSHandle<JSTaggedValue> key(factory->NewFromASCII(keyArray));
        JSSet::Add(thread, set, key);
        EXPECT_TRUE(set->Has(key.GetTaggedValue()));
    }
    EXPECT_EQ(set->GetSize(), 40);
    // whether jsSet has delete key
    keyArray[3] = '1' + 8;
    JSHandle<JSTaggedValue> deleteKey(factory->NewFromASCII(keyArray));
    JSSet::Delete(thread, set, deleteKey);
    EXPECT_FALSE(set->Has(deleteKey.GetTaggedValue()));
    EXPECT_EQ(set->GetSize(), 39);
}

HWTEST_F_L0(JSSetTest, Iterator)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSSet> set(thread, CreateSet());
    for (int i = 0; i < 5; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        JSSet::Add(thread, set, key);
    }

    JSHandle<JSTaggedValue> keyIter(factory->NewJSSetIterator(set, IterationKind::KEY));
    JSHandle<JSTaggedValue> valueIter(factory->NewJSSetIterator(set, IterationKind::VALUE));

    JSHandle<JSTaggedValue> keyResult0 = JSIterator::IteratorStep(thread, keyIter);
    JSHandle<JSTaggedValue> valueResult0 = JSIterator::IteratorStep(thread, valueIter);

    EXPECT_EQ(0, JSIterator::IteratorValue(thread, keyResult0)->GetInt());
    EXPECT_EQ(0, JSIterator::IteratorValue(thread, valueResult0)->GetInt());

    JSHandle<JSTaggedValue> keyResult1 = JSIterator::IteratorStep(thread, keyIter);
    EXPECT_EQ(1, JSIterator::IteratorValue(thread, keyResult1)->GetInt());

    for (int i = 0; i < 3; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        JSSet::Delete(thread, set, key);
    }

    JSHandle<JSTaggedValue> keyResult2 = JSIterator::IteratorStep(thread, keyIter);
    EXPECT_EQ(3, JSIterator::IteratorValue(thread, keyResult2)->GetInt());
    JSHandle<JSTaggedValue> keyResult3 = JSIterator::IteratorStep(thread, keyIter);
    EXPECT_EQ(4, JSIterator::IteratorValue(thread, keyResult3)->GetInt());
    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(5));
    JSSet::Add(thread, set, key);
    JSHandle<JSTaggedValue> keyResult4 = JSIterator::IteratorStep(thread, keyIter);
    EXPECT_EQ(5, JSIterator::IteratorValue(thread, keyResult4)->GetInt());
    JSHandle<JSTaggedValue> keyResult5 = JSIterator::IteratorStep(thread, keyIter);
    EXPECT_EQ(JSTaggedValue::False(), keyResult5.GetTaggedValue());
}
}  // namespace panda::test
