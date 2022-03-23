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

#include <string>
#include "ecmascript/base/builtins_base.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_tree-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;

using namespace panda::ecmascript;

namespace panda::test {
class TaggedTreeTest : public testing::Test {
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

    JSHandle<GlobalEnv> GetGlobalEnv()
    {
        EcmaVM *ecma = thread->GetEcmaVM();
        return ecma->GetGlobalEnv();
    }
    class TestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestCompareFunction(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> valueX = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> valueY = GetCallArg(argv, 1);

            if (valueX->IsString() && valueY->IsString()) {
                auto xString = static_cast<EcmaString *>(valueX->GetTaggedObject());
                auto yString = static_cast<EcmaString *>(valueY->GetTaggedObject());
                int result = xString->Compare(yString);
                if (result < 0) {
                    return JSTaggedValue(1);
                }
                if (result == 0) {
                    return JSTaggedValue(0);
                }
                return JSTaggedValue(-1);
            }

            if (valueX->IsNumber() && valueY->IsString()) {
                return JSTaggedValue(1);
            }
            if (valueX->IsString() && valueY->IsNumber()) {
                return JSTaggedValue(-1);
            }

            ComparisonResult res = ComparisonResult::UNDEFINED;
            if (valueX->IsNumber() && valueY->IsNumber()) {
                res = JSTaggedValue::StrictNumberCompare(valueY->GetNumber(), valueX->GetNumber());
            } else {
                res = JSTaggedValue::Compare(thread, valueY, valueX);
            }
            return res == ComparisonResult::GREAT ?
                JSTaggedValue(1) : (res == ComparisonResult::LESS ? JSTaggedValue(-1) : JSTaggedValue(0));
        }
    };
};

template <typename T>
bool CheckRBTreeOfAllPaths(JSHandle<T> &tree, int numsOfBlack, int index, int count)
{
    if (index < 0) {
        return count == numsOfBlack;
    }
    if (tree->GetColor(index) == TreeColor::BLACK) {
        count++;
    }
    if (CheckRBTreeOfAllPaths(tree, numsOfBlack, tree->GetLeftChildIndex(index), count) &&
        CheckRBTreeOfAllPaths(tree, numsOfBlack, tree->GetRightChildIndex(index), count)) {
        return true;
    }
    return false;
}

template <typename T>
bool CheckBlackNodeNumbers(JSHandle<T> &tree, int index)
{
    int numsOfBlack = 0;
    int child = index;
    while (child >= 0) {
        if (tree->GetColor(child) == TreeColor::BLACK) {
            numsOfBlack++;
        }
        child = tree->GetLeftChildIndex(child);
    }
    return CheckRBTreeOfAllPaths(tree, numsOfBlack, index, 0);
}

template <typename T>
bool IsVaildRBTree(JSThread *thread, JSHandle<T> &tree, int nodeIndex)
{
    CQueue<int> nodes;
    nodes.emplace(nodeIndex);
    while (!nodes.empty()) {
        int index = nodes.front();
        int parent = tree->GetParent(index);
        int pleft = tree->GetLeftChildIndex(parent);
        int pright = tree->GetRightChildIndex(parent);
        if (parent >= 0 && index != pleft && index != pright) {
            return false;
        }

        int ileft = tree->GetLeftChildIndex(index);
        JSHandle<JSTaggedValue> indexKey(thread, tree->GetKey(index));
        if (ileft >= 0) {
            JSHandle<JSTaggedValue> leftKey(thread, tree->GetKey(ileft));
            ComparisonResult result = TaggedTree<T>::EntryCompare(thread, leftKey, indexKey, tree);
            if (tree->GetParent(ileft) != index || result != ComparisonResult::LESS) {
                return false;
            }
            nodes.emplace(ileft);
        }
        int iright = tree->GetRightChildIndex(index);
        if (iright >= 0) {
            JSHandle<JSTaggedValue> rightKey(thread, tree->GetKey(iright));
            ComparisonResult result = TaggedTree<T>::EntryCompare(thread, rightKey, indexKey, tree);
            if (tree->GetParent(iright) != index || result != ComparisonResult::GREAT) {
                return false;
            }
            nodes.emplace(iright);
        }

        // check red node
        TreeColor indexColor = tree->GetColor(index);
        if (indexColor == TreeColor::RED) {
            if (ileft >= 0 && tree->GetColor(ileft) == TreeColor::RED) {
                return false;
            }
            if (iright >= 0 && tree->GetColor(iright) == TreeColor::RED) {
                return false;
            }
        }
        // check black node
        if (!CheckBlackNodeNumbers(tree, index)) {
            return false;
        }
        nodes.pop();
    }
    return true;
}

HWTEST_F_L0(TaggedTreeTest, TreeMapCreate)
{
    constexpr uint32_t NODE_NUMBERS = 64;
    JSHandle<TaggedTreeMap> tmap(thread, TaggedTreeMap::Create(thread, NODE_NUMBERS));
    EXPECT_EQ(tmap->Capacity(), NODE_NUMBERS);
    EXPECT_EQ(tmap->GetRootEntries(), -1);
    EXPECT_EQ(tmap->NumberOfElements(), 0);
    EXPECT_EQ(tmap->NumberOfDeletedElements(), 0);
}

HWTEST_F_L0(TaggedTreeTest, TreeSetCreate)
{
    constexpr uint32_t NODE_NUMBERS = 64;
    JSHandle<TaggedTreeSet> tset(thread, TaggedTreeSet::Create(thread, NODE_NUMBERS));
    EXPECT_EQ(tset->Capacity(), NODE_NUMBERS);
    EXPECT_EQ(tset->GetRootEntries(), -1);
    EXPECT_EQ(tset->NumberOfElements(), 0);
    EXPECT_EQ(tset->NumberOfDeletedElements(), 0);
}

HWTEST_F_L0(TaggedTreeTest, TestTreeMapAddKeyAndValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    constexpr uint32_t NODE_NUMBERS = 64;
    JSMutableHandle<TaggedTreeMap> tmap(thread, TaggedTreeMap::Create(thread, NODE_NUMBERS));
    JSHandle<JSTaggedValue> objFun = GetGlobalEnv()->GetObjectFunction();

    char keyArray[] = "mykey1";
    JSHandle<EcmaString> stringKey1 = factory->NewFromCanBeCompressString(keyArray);
    JSHandle<JSTaggedValue> key1(stringKey1);
    JSHandle<JSTaggedValue> value1(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    char key2Array[] = "mykey2";
    JSHandle<EcmaString> stringKey2 = factory->NewFromCanBeCompressString(key2Array);
    JSHandle<JSTaggedValue> key2(stringKey2);
    JSHandle<JSTaggedValue> value2(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));

    // test set()
    tmap.Update(TaggedTreeMap::Set(thread, tmap, key1, value1));
    EXPECT_EQ(tmap->NumberOfElements(), 1);
    tmap.Update(TaggedTreeMap::Set(thread, tmap, key2, value2));
    EXPECT_EQ(tmap->NumberOfElements(), 2);

    // test find()
    JSTaggedValue res = TaggedTreeMap::Get(thread, tmap, key1);
    EXPECT_EQ(value1.GetTaggedValue(), res);

    // test remove()
    int entry = TaggedTreeMap::FindEntry(thread, tmap, key1);
    res = TaggedTreeMap::Delete(thread, tmap, entry);
    EXPECT_EQ(TaggedTreeMap::Cast(res.GetTaggedObject())->NumberOfElements(), 1);
    tmap.Update(res);
    EXPECT_EQ(tmap->NumberOfElements(), 1);
    res = TaggedTreeMap::Get(thread, tmap, key1);
    EXPECT_EQ(res, JSTaggedValue::Undefined());
}

HWTEST_F_L0(TaggedTreeTest, TestTreeSetAddValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    constexpr uint32_t NODE_NUMBERS = 64;
    JSMutableHandle<TaggedTreeSet> tset(thread, TaggedTreeSet::Create(thread, NODE_NUMBERS));

    char keyArray[] = "mykey1";
    JSHandle<EcmaString> stringKey1 = factory->NewFromCanBeCompressString(keyArray);
    JSHandle<JSTaggedValue> key1(stringKey1);
    char key2Array[] = "mykey2";
    JSHandle<EcmaString> stringKey2 = factory->NewFromCanBeCompressString(key2Array);
    JSHandle<JSTaggedValue> key2(stringKey2);

    // test set()
    tset.Update(TaggedTreeSet::Add(thread, tset, key1));
    EXPECT_EQ(tset->NumberOfElements(), 1);
    tset.Update(TaggedTreeSet::Add(thread, tset, key2));
    EXPECT_EQ(tset->NumberOfElements(), 2);

    // test find()
    int entry = TaggedTreeSet::FindEntry(thread, tset, key1);
    EXPECT_TRUE(entry >= 0);

    // test remove()
    JSTaggedValue res = TaggedTreeSet::Delete(thread, tset, entry);
    EXPECT_EQ(TaggedTreeSet::Cast(res.GetTaggedObject())->NumberOfElements(), 1);
    EXPECT_EQ(-1, TaggedTreeSet::FindEntry(thread, tset, key1));
    EXPECT_EQ(tset->NumberOfElements(), 1);
}

HWTEST_F_L0(TaggedTreeTest, TestTreeMapGrowCapacity)
{
    constexpr uint32_t NODE_NUMBERS = 32;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<TaggedTreeMap> tmap(thread, TaggedTreeMap::Create(thread));
    JSHandle<JSFunction> objFun(GetGlobalEnv()->GetObjectFunction());
    char keyArray[7] = "mykey"; // 7 means array length
    for (int i = 0; i < NODE_NUMBERS; i++) {
        keyArray[5] = '1' + i; // 5 means index of keyArray
        keyArray[6] = 0;       // 6 means index of keyArray
        JSHandle<JSTaggedValue> key(factory->NewFromCanBeCompressString(keyArray));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        // test set()
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
        EXPECT_TRUE(TaggedTreeMap::FindEntry(thread, tmap, key) >= 0);
    }

    for (int i = 0; i < NODE_NUMBERS; i++) {
        keyArray[5] = '1' + i; // 5 means index of keyArray
        keyArray[6] = 0;       // 6 means index of keyArray
        JSHandle<JSTaggedValue> stringKey(factory->NewFromCanBeCompressString(keyArray));
        // test get()
        JSTaggedValue res = TaggedTreeMap::Get(thread, tmap, stringKey);
        EXPECT_EQ(JSTaggedValue(i), res);
    }
    EXPECT_EQ(tmap->NumberOfElements(), NODE_NUMBERS);
    EXPECT_EQ(tmap->Capacity(), 63); // 63 means capacity after Grow
}

HWTEST_F_L0(TaggedTreeTest, TestTreeSetGrowCapacity)
{
    constexpr uint32_t NODE_NUMBERS = 32;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<TaggedTreeSet> tset(thread, TaggedTreeSet::Create(thread));
    // create key
    char keyArray[7] = "mykey"; // 7 means array length
    for (int i = 0; i < NODE_NUMBERS; i++) {
        keyArray[5] = '1' + i; // 5 means index of keyArray
        keyArray[6] = 0;       // 6 means index of keyArray
        JSHandle<EcmaString> stringKey = factory->NewFromCanBeCompressString(keyArray);
        JSHandle<JSTaggedValue> key(stringKey);

        // test set()
        tset.Update(TaggedTreeSet::Add(thread, tset, key));
        EXPECT_TRUE(TaggedTreeSet::FindEntry(thread, tset, key) >= 0);
    }

    for (int i = 0; i < NODE_NUMBERS; i++) {
        keyArray[5] = '1' + i; // 5 means index of keyArray
        keyArray[6] = 0;       // 6 means index of keyArray
        JSHandle<JSTaggedValue> stringKey(factory->NewFromCanBeCompressString(keyArray));
        // test get()
        EXPECT_TRUE(TaggedTreeSet::FindEntry(thread, tset, stringKey) >= 0);
    }
    EXPECT_EQ(tset->NumberOfElements(), NODE_NUMBERS);
    EXPECT_EQ(tset->Capacity(), 63); // 63 means capacity after Grow
}

HWTEST_F_L0(TaggedTreeTest, TestTreeMapHasValue)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");

    // test TaggedTreeMap HasValue
    JSMutableHandle<TaggedTreeMap> tmap(thread, TaggedTreeMap::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
    }
    EXPECT_EQ(tmap->NumberOfElements(), NODE_NUMBERS);
    EXPECT_EQ(tmap->Capacity(), 15); // 15 means capacity after Grow

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        bool success = tmap->HasValue(thread, value.GetTaggedValue());
        EXPECT_TRUE(success);
    }
}

HWTEST_F_L0(TaggedTreeTest, TestTreeMapGetLowerKey)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");

    // test TaggedTreeMap
    JSMutableHandle<TaggedTreeMap> tmap(thread, TaggedTreeMap::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
    }

    std::string minKey = myKey + std::to_string(0);
    key.Update(factory->NewFromStdString(minKey).GetTaggedValue());
    JSTaggedValue lowerKey = TaggedTreeMap::GetLowerKey(thread, tmap, key);
    EXPECT_TRUE(lowerKey.IsUndefined());

    // add [1, 1]
    key.Update(JSTaggedValue(1));
    value.Update(JSTaggedValue(1));
    tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));

    key.Update(factory->NewFromStdString(minKey).GetTaggedValue());
    lowerKey = TaggedTreeMap::GetLowerKey(thread, tmap, key);
    EXPECT_EQ(lowerKey, JSTaggedValue(1));

    // check mykey1 ...mykeyn
    JSMutableHandle<JSTaggedValue> keyToCompare(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 1; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        std::string tmp = myKey + std::to_string(i - 1);
        keyToCompare.Update(factory->NewFromStdString(tmp).GetTaggedValue());
        lowerKey = TaggedTreeMap::GetLowerKey(thread, tmap, key);
        EXPECT_EQ(lowerKey, keyToCompare.GetTaggedValue());
    }
}

HWTEST_F_L0(TaggedTreeTest, TestTreeMapGetHigherKey)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // test TaggedTreeMap
    JSMutableHandle<TaggedTreeMap> tmap(thread, TaggedTreeMap::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        value.Update(JSTaggedValue(i));
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
    }

    JSMutableHandle<JSTaggedValue> maxKey(thread, JSTaggedValue(NODE_NUMBERS - 1));
    JSTaggedValue higherKey = TaggedTreeMap::GetHigherKey(thread, tmap, maxKey);
    EXPECT_TRUE(higherKey.IsUndefined());

    // add [mykey, mykey]
    std::string myKey("mykey");
    key.Update(factory->NewFromStdString(myKey).GetTaggedValue());
    tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));

    higherKey = TaggedTreeMap::GetHigherKey(thread, tmap, maxKey);
    EXPECT_EQ(higherKey, key.GetTaggedValue());

    // check 1 ...n
    JSMutableHandle<JSTaggedValue> keyToCompare(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS - 1; i++) {
        key.Update(JSTaggedValue(i));
        keyToCompare.Update(JSTaggedValue(i + 1));
        higherKey = TaggedTreeMap::GetHigherKey(thread, tmap, key);
        EXPECT_EQ(higherKey, keyToCompare.GetTaggedValue());
    }
}

HWTEST_F_L0(TaggedTreeTest, TestTreeMapGetFirsKey)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");

    // test TaggedTreeMap
    JSMutableHandle<TaggedTreeMap> tmap(thread, TaggedTreeMap::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
    }
    std::string ckey = myKey + std::to_string(0);
    key.Update(factory->NewFromStdString(ckey).GetTaggedValue());
    JSTaggedValue firstKey = tmap->GetFirstKey();
    EXPECT_EQ(firstKey, key.GetTaggedValue());

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        value.Update(JSTaggedValue(i));
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
    }
    firstKey = tmap->GetFirstKey();
    EXPECT_EQ(firstKey, JSTaggedValue(0));
}

HWTEST_F_L0(TaggedTreeTest, TestTreeMapGetLastKey)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // test TaggedTreeMap
    JSMutableHandle<TaggedTreeMap> tmap(thread, TaggedTreeMap::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        value.Update(JSTaggedValue(i));
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
    }
    JSTaggedValue lastKey = tmap->GetLastKey();
    EXPECT_EQ(lastKey, JSTaggedValue(NODE_NUMBERS - 1));

    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
    }
    std::string ckey = myKey + std::to_string(NODE_NUMBERS - 1);
    key.Update(factory->NewFromStdString(ckey).GetTaggedValue());
    lastKey = tmap->GetLastKey();
    EXPECT_EQ(lastKey, key.GetTaggedValue());
}

HWTEST_F_L0(TaggedTreeTest, TestTreeMapSetAll)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // test TaggedTreeMap
    JSMutableHandle<TaggedTreeMap> smap(thread, TaggedTreeMap::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        value.Update(JSTaggedValue(i));
        smap.Update(TaggedTreeMap::Set(thread, smap, key, value));
    }

    JSMutableHandle<TaggedTreeMap> dmap(thread, TaggedTreeMap::Create(thread));
    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        dmap.Update(TaggedTreeMap::Set(thread, dmap, key, value));
    }
    dmap.Update(TaggedTreeMap::SetAll(thread, dmap, smap));

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        value.Update(JSTaggedValue(i));
        JSTaggedValue res = TaggedTreeMap::Get(thread, dmap, key);
        EXPECT_EQ(res, value.GetTaggedValue());

        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        res = TaggedTreeMap::Get(thread, dmap, key);
        EXPECT_EQ(res, value.GetTaggedValue());
    }
}

HWTEST_F_L0(TaggedTreeTest, TestTreeMapGetArrayFromMap)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // test TaggedTreeMap
    JSMutableHandle<TaggedTreeMap> tmap(thread, TaggedTreeMap::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        value.Update(JSTaggedValue(i));
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
    }

    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
    }
    JSHandle<TaggedArray> arr = TaggedTreeMap::GetArrayFromMap(thread, tmap);
    EXPECT_EQ(arr->GetLength(), static_cast<uint32_t>(NODE_NUMBERS * 2));

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        EXPECT_EQ(tmap->GetKey(arr->Get(i).GetInt()), JSTaggedValue(i));
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        EXPECT_EQ(tmap->GetKey(arr->Get(NODE_NUMBERS + i).GetInt()), key.GetTaggedValue());
    }
}

// TaggedTreeSet
HWTEST_F_L0(TaggedTreeTest, TestTreeSetGetLowerKey)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");

    // test TaggedTreeSet
    JSMutableHandle<TaggedTreeSet> tset(thread, TaggedTreeSet::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        tset.Update(TaggedTreeSet::Add(thread, tset, key));
    }

    std::string minKey = myKey + std::to_string(0);
    key.Update(factory->NewFromStdString(minKey).GetTaggedValue());
    JSTaggedValue lowerKey = TaggedTreeSet::GetLowerKey(thread, tset, key);
    EXPECT_TRUE(lowerKey.IsUndefined());

    // add [1, 1]
    key.Update(JSTaggedValue(1));
    tset.Update(TaggedTreeSet::Add(thread, tset, key));

    key.Update(factory->NewFromStdString(minKey).GetTaggedValue());
    lowerKey = TaggedTreeSet::GetLowerKey(thread, tset, key);
    EXPECT_EQ(lowerKey, JSTaggedValue(1));

    // check mykey1 ...mykeyn
    JSMutableHandle<JSTaggedValue> keyToCompare(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 1; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        std::string tmp = myKey + std::to_string(i - 1);
        keyToCompare.Update(factory->NewFromStdString(tmp).GetTaggedValue());
        lowerKey = TaggedTreeSet::GetLowerKey(thread, tset, key);
        EXPECT_EQ(lowerKey, keyToCompare.GetTaggedValue());
    }
}

HWTEST_F_L0(TaggedTreeTest, TestTreeSetGetHigherKey)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());

    // test TaggedTreeSet
    JSMutableHandle<TaggedTreeSet> tset(thread, TaggedTreeSet::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        tset.Update(TaggedTreeSet::Add(thread, tset, key));
    }

    JSMutableHandle<JSTaggedValue> maxKey(thread, JSTaggedValue(NODE_NUMBERS - 1));
    JSTaggedValue higherKey = TaggedTreeSet::GetHigherKey(thread, tset, maxKey);
    EXPECT_TRUE(higherKey.IsUndefined());

    // add [mykey, mykey]
    std::string myKey("mykey");
    key.Update(factory->NewFromStdString(myKey).GetTaggedValue());
    tset.Update(TaggedTreeSet::Add(thread, tset, key));

    higherKey = TaggedTreeSet::GetHigherKey(thread, tset, maxKey);
    EXPECT_EQ(higherKey, key.GetTaggedValue());

    // check 1 ...n
    JSMutableHandle<JSTaggedValue> keyToCompare(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS - 1; i++) {
        key.Update(JSTaggedValue(i));
        keyToCompare.Update(JSTaggedValue(i + 1));
        higherKey = TaggedTreeSet::GetHigherKey(thread, tset, key);
        EXPECT_EQ(higherKey, keyToCompare.GetTaggedValue());
    }
}

HWTEST_F_L0(TaggedTreeTest, TestTreeSetGetFirsKey)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");

    // test TaggedTreeSet
    JSMutableHandle<TaggedTreeSet> tset(thread, TaggedTreeSet::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        tset.Update(TaggedTreeSet::Add(thread, tset, key));
    }
    std::string ckey = myKey + std::to_string(0);
    key.Update(factory->NewFromStdString(ckey).GetTaggedValue());
    JSTaggedValue firstKey = tset->GetFirstKey();
    EXPECT_EQ(firstKey, key.GetTaggedValue());

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        tset.Update(TaggedTreeSet::Add(thread, tset, key));
    }
    firstKey = tset->GetFirstKey();
    EXPECT_EQ(firstKey, JSTaggedValue(0));
}

HWTEST_F_L0(TaggedTreeTest, TestTreeSetGetLastKey)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());

    // test TaggedTreeSet
    JSMutableHandle<TaggedTreeSet> tset(thread, TaggedTreeSet::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        tset.Update(TaggedTreeSet::Add(thread, tset, key));
    }
    JSTaggedValue lastKey = tset->GetLastKey();
    EXPECT_EQ(lastKey, JSTaggedValue(NODE_NUMBERS - 1));

    std::string myKey("mykey");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        tset.Update(TaggedTreeSet::Add(thread, tset, key));
    }
    std::string ckey = myKey + std::to_string(NODE_NUMBERS - 1);
    key.Update(factory->NewFromStdString(ckey).GetTaggedValue());
    lastKey = tset->GetLastKey();
    EXPECT_EQ(lastKey, key.GetTaggedValue());
}

HWTEST_F_L0(TaggedTreeTest, TestTreeSetGetArrayFromSet)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());

    // test TaggedTreeSet
    JSMutableHandle<TaggedTreeSet> tset(thread, TaggedTreeSet::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        tset.Update(TaggedTreeSet::Add(thread, tset, key));
    }

    std::string myKey("mykey");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        tset.Update(TaggedTreeSet::Add(thread, tset, key));
    }
    JSHandle<TaggedArray> arr = TaggedTreeSet::GetArrayFromSet(thread, tset);
    EXPECT_EQ(arr->GetLength(), static_cast<uint32_t>(NODE_NUMBERS * 2));

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        EXPECT_EQ(tset->GetKey(arr->Get(i).GetInt()), JSTaggedValue(i));
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        EXPECT_EQ(tset->GetKey(arr->Get(NODE_NUMBERS + i).GetInt()), key.GetTaggedValue());
    }
}

HWTEST_F_L0(TaggedTreeTest, TestSetAfterDelete)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // test TaggedTreeMap
    JSMutableHandle<TaggedTreeMap> tmap(thread, TaggedTreeMap::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        value.Update(JSTaggedValue(i));
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
    }
    EXPECT_EQ(tmap->NumberOfElements(), NODE_NUMBERS);
    EXPECT_EQ(tmap->NumberOfDeletedElements(), 0);

    for (uint32_t i = 0; i < NODE_NUMBERS / 2; i++) {
        key.Update(JSTaggedValue(i));
        value.Update(JSTaggedValue(i));
        JSTaggedValue dvalue = TaggedTreeMap::Delete(thread, tmap, TaggedTreeMap::FindEntry(thread, tmap, key));
        EXPECT_EQ(dvalue, tmap.GetTaggedValue());
    }
    EXPECT_EQ(tmap->NumberOfElements(), NODE_NUMBERS / 2);
    EXPECT_EQ(tmap->NumberOfDeletedElements(), NODE_NUMBERS / 2);

    std::string myKey("mykey");
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
    }
    EXPECT_EQ(tmap->NumberOfElements(), NODE_NUMBERS + NODE_NUMBERS / 2);
    EXPECT_EQ(tmap->NumberOfDeletedElements(), 0);

    for (uint32_t i = NODE_NUMBERS / 2; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        value.Update(JSTaggedValue(i));
        JSTaggedValue gvalue = TaggedTreeMap::Get(thread, tmap, key);
        EXPECT_EQ(gvalue, value.GetTaggedValue());
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSTaggedValue gvalue = TaggedTreeMap::Get(thread, tmap, key);
        EXPECT_EQ(gvalue, value.GetTaggedValue());
    }
    EXPECT_EQ(tmap->NumberOfElements(), NODE_NUMBERS + NODE_NUMBERS / 2);
    EXPECT_EQ(tmap->Capacity(), 31); // 31 means capacity after grow

    // TaggedTreeSet
    JSMutableHandle<TaggedTreeSet> tset(thread, TaggedTreeSet::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        tset.Update(TaggedTreeSet::Add(thread, tset, key));
    }
    EXPECT_EQ(tset->NumberOfElements(), NODE_NUMBERS);
    EXPECT_EQ(tset->NumberOfDeletedElements(), 0);

    for (uint32_t i = 0; i < NODE_NUMBERS / 2; i++) {
        key.Update(JSTaggedValue(i));
        JSTaggedValue dvalue = TaggedTreeSet::Delete(thread, tset, TaggedTreeSet::FindEntry(thread, tset, key));
        EXPECT_EQ(dvalue, tset.GetTaggedValue());
    }
    EXPECT_EQ(tset->NumberOfElements(), NODE_NUMBERS / 2);
    EXPECT_EQ(tset->NumberOfDeletedElements(), NODE_NUMBERS / 2);

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        tset.Update(TaggedTreeSet::Add(thread, tset, key));
    }
    EXPECT_EQ(tset->NumberOfElements(), NODE_NUMBERS + NODE_NUMBERS / 2);
    EXPECT_EQ(tset->NumberOfDeletedElements(), 0);

    for (uint32_t i = NODE_NUMBERS / 2; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        int entry = TaggedTreeSet::FindEntry(thread, tset, key);
        EXPECT_TRUE(entry >= 0);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        int entry = TaggedTreeSet::FindEntry(thread, tset, key);
        EXPECT_TRUE(entry >= 0);
    }
    EXPECT_EQ(tset->NumberOfElements(), NODE_NUMBERS + NODE_NUMBERS / 2);
    EXPECT_EQ(tset->Capacity(), 31); // 31 means capacity after grow
}

HWTEST_F_L0(TaggedTreeTest, RBTreeAddCheck)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    constexpr uint32_t NODE_NUMBERS = 16;
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");

    // test TaggedTreeMap
    JSMutableHandle<TaggedTreeMap> tmap(thread, TaggedTreeMap::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        value.Update(JSTaggedValue(i));
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
        bool success = IsVaildRBTree<TaggedTreeMap>(thread, tmap, tmap->GetRootEntries());
        EXPECT_TRUE(success);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
        bool success = IsVaildRBTree<TaggedTreeMap>(thread, tmap, tmap->GetRootEntries());
        EXPECT_TRUE(success);
    }
    EXPECT_TRUE(tmap->NumberOfElements() == NODE_NUMBERS * 2);

    // test TaggedTreeSet
    JSMutableHandle<TaggedTreeSet> tset(thread, TaggedTreeSet::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        tset.Update(TaggedTreeSet::Add(thread, tset, key));
        bool success = IsVaildRBTree<TaggedTreeSet>(thread, tset, tset->GetRootEntries());
        EXPECT_TRUE(success);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        tset.Update(TaggedTreeSet::Add(thread, tset, key));
        bool success = IsVaildRBTree<TaggedTreeSet>(thread, tset, tset->GetRootEntries());
        EXPECT_TRUE(success);
    }
    EXPECT_TRUE(tset->NumberOfElements() == NODE_NUMBERS * 2);
}

HWTEST_F_L0(TaggedTreeTest, RBTreeDeleteCheck)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    constexpr uint32_t NODE_NUMBERS = 16;
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");

    // test TaggedTreeMap
    JSMutableHandle<TaggedTreeMap> tmap(thread, TaggedTreeMap::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        value.Update(JSTaggedValue(i));
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
        bool success = IsVaildRBTree<TaggedTreeMap>(thread, tmap, tmap->GetRootEntries());
        EXPECT_TRUE(success);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
        bool success = IsVaildRBTree<TaggedTreeMap>(thread, tmap, tmap->GetRootEntries());
        EXPECT_TRUE(success);
    }

    JSMutableHandle<JSTaggedValue> resOfDelete(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        resOfDelete.Update(TaggedTreeMap::Delete(thread, tmap, TaggedTreeMap::FindEntry(thread, tmap, key)));
        bool success = IsVaildRBTree<TaggedTreeMap>(thread, tmap, tmap->GetRootEntries());
        EXPECT_TRUE(success);
        EXPECT_EQ(resOfDelete.GetTaggedValue(), tmap.GetTaggedValue());
    }
    EXPECT_TRUE(tmap->NumberOfElements() == NODE_NUMBERS);

    // test TaggedTreeSet
    JSMutableHandle<TaggedTreeSet> tset(thread, TaggedTreeSet::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        tset.Update(TaggedTreeSet::Add(thread, tset, key));
        bool success = IsVaildRBTree<TaggedTreeSet>(thread, tset, tset->GetRootEntries());
        EXPECT_TRUE(success);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        tset.Update(TaggedTreeSet::Add(thread, tset, key));
        bool success = IsVaildRBTree<TaggedTreeSet>(thread, tset, tset->GetRootEntries());
        EXPECT_TRUE(success);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        resOfDelete.Update(TaggedTreeSet::Delete(thread, tset, TaggedTreeSet::FindEntry(thread, tset, key)));
        bool success = IsVaildRBTree<TaggedTreeSet>(thread, tset, tset->GetRootEntries());
        EXPECT_TRUE(success);
        EXPECT_EQ(resOfDelete.GetTaggedValue(), tset.GetTaggedValue());
    }
    EXPECT_TRUE(tset->NumberOfElements() == NODE_NUMBERS);
}

HWTEST_F_L0(TaggedTreeTest, CustomCompareFunctionTest)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    constexpr uint32_t NODE_NUMBERS = 9;
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");

    // test TaggedTreeMap
    JSMutableHandle<TaggedTreeMap> tmap(thread, TaggedTreeMap::Create(thread));
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestCompareFunction));
    tmap->SetCompare(thread, func.GetTaggedValue());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        value.Update(JSTaggedValue(i));
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
        bool success = IsVaildRBTree<TaggedTreeMap>(thread, tmap, tmap->GetRootEntries());
        EXPECT_TRUE(success);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        std::string ivalue = myValue + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
        bool success = IsVaildRBTree<TaggedTreeMap>(thread, tmap, tmap->GetRootEntries());
        EXPECT_TRUE(success);
    }
    EXPECT_TRUE(tmap->NumberOfElements() == NODE_NUMBERS * 2);

    JSHandle<TaggedArray> arr = TaggedTreeMap::GetArrayFromMap(thread, tmap);
    EXPECT_EQ(arr->GetLength(), static_cast<uint32_t>(NODE_NUMBERS * 2));
    for (uint32_t i = NODE_NUMBERS; i < NODE_NUMBERS * 2; i++) {
        EXPECT_EQ(tmap->GetKey(arr->Get(i).GetInt()).GetInt(), (NODE_NUMBERS * 2  - 1 - i));
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(NODE_NUMBERS - 1 - i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        EXPECT_EQ(tmap->GetKey(arr->Get(i).GetInt()), key.GetTaggedValue());
    }

    // test TaggedTreeSet
    JSMutableHandle<TaggedTreeSet> tset(thread, TaggedTreeSet::Create(thread));
    tset->SetCompare(thread, func.GetTaggedValue());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        tset.Update(TaggedTreeSet::Add(thread, tset, key));
        bool success = IsVaildRBTree<TaggedTreeSet>(thread, tset, tset->GetRootEntries());
        EXPECT_TRUE(success);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        tset.Update(TaggedTreeSet::Add(thread, tset, key));
        bool success = IsVaildRBTree<TaggedTreeSet>(thread, tset, tset->GetRootEntries());
        EXPECT_TRUE(success);
    }
    EXPECT_TRUE(tset->NumberOfElements() == NODE_NUMBERS * 2);

    JSHandle<TaggedArray> sarr = TaggedTreeSet::GetArrayFromSet(thread, tset);
    EXPECT_EQ(arr->GetLength(), static_cast<uint32_t>(NODE_NUMBERS * 2));
    for (uint32_t i = NODE_NUMBERS; i < NODE_NUMBERS * 2; i++) {
        EXPECT_EQ(tset->GetKey(sarr->Get(i).GetInt()), JSTaggedValue(NODE_NUMBERS * 2  - 1 - i));
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(NODE_NUMBERS - 1 - i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        EXPECT_EQ(tset->GetKey(sarr->Get(i).GetInt()), key.GetTaggedValue());
    }
}

HWTEST_F_L0(TaggedTreeTest, RBTreeDeleteShrink)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    constexpr uint32_t NODE_NUMBERS = 32;
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    std::string myValue("myvalue");

    // test TaggedTreeMap
    JSMutableHandle<TaggedTreeMap> tmap(thread, TaggedTreeMap::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        value.Update(JSTaggedValue(i));
        tmap.Update(TaggedTreeMap::Set(thread, tmap, key, value));
        bool success = IsVaildRBTree<TaggedTreeMap>(thread, tmap, tmap->GetRootEntries());
        EXPECT_TRUE(success);
    }

    JSMutableHandle<JSTaggedValue> resOfDelete(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS / 2; i++) {
        key.Update(JSTaggedValue(i));
        resOfDelete.Update(TaggedTreeMap::Delete(thread, tmap, TaggedTreeMap::FindEntry(thread, tmap, key)));
        bool success = IsVaildRBTree<TaggedTreeMap>(thread, tmap, tmap->GetRootEntries());
        EXPECT_TRUE(success);
        EXPECT_EQ(resOfDelete.GetTaggedValue(), tmap.GetTaggedValue());
    }

    {
        EXPECT_EQ(tmap->Capacity(), NODE_NUMBERS * 2 - 1);
        key.Update(JSTaggedValue(NODE_NUMBERS / 2));
        resOfDelete.Update(TaggedTreeMap::Delete(thread, tmap, TaggedTreeMap::FindEntry(thread, tmap, key)));
        EXPECT_NE(resOfDelete.GetTaggedValue(), tmap.GetTaggedValue());

        key.Update(JSTaggedValue(NODE_NUMBERS / 2 + 1));
        JSHandle<TaggedTreeMap> newMap = JSHandle<TaggedTreeMap>::Cast(resOfDelete);
        resOfDelete.Update(TaggedTreeMap::Delete(thread, newMap, TaggedTreeMap::FindEntry(thread, newMap, key)));
        EXPECT_EQ(tmap->NumberOfElements(), NODE_NUMBERS / 2 - 1);
        EXPECT_EQ(newMap->NumberOfElements(), NODE_NUMBERS / 2 - 2); // 2 means two elements
        bool success = IsVaildRBTree<TaggedTreeMap>(thread, newMap, newMap->GetRootEntries());
        EXPECT_TRUE(success);
        EXPECT_EQ(newMap->Capacity(), NODE_NUMBERS - 1);
    }

    // test TaggedTreeSet
    JSMutableHandle<TaggedTreeSet> tset(thread, TaggedTreeSet::Create(thread));
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        tset.Update(TaggedTreeSet::Add(thread, tset, key));
        bool success = IsVaildRBTree<TaggedTreeSet>(thread, tset, tset->GetRootEntries());
        EXPECT_TRUE(success);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS / 2; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        resOfDelete.Update(TaggedTreeSet::Delete(thread, tset, TaggedTreeSet::FindEntry(thread, tset, key)));
        bool success = IsVaildRBTree<TaggedTreeSet>(thread, tset, tset->GetRootEntries());
        EXPECT_TRUE(success);
        EXPECT_EQ(resOfDelete.GetTaggedValue(), tset.GetTaggedValue());
    }

    {
        EXPECT_EQ(tset->Capacity(), NODE_NUMBERS * 2 - 1);
        std::string ikey = myKey + std::to_string(NODE_NUMBERS / 2);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        resOfDelete.Update(TaggedTreeSet::Delete(thread, tset, TaggedTreeSet::FindEntry(thread, tset, key)));
        EXPECT_NE(resOfDelete.GetTaggedValue(), tset.GetTaggedValue());

        ikey = myKey + std::to_string(NODE_NUMBERS / 2 + 1);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        JSHandle<TaggedTreeSet> newSet = JSHandle<TaggedTreeSet>::Cast(resOfDelete);
        resOfDelete.Update(TaggedTreeSet::Delete(thread, newSet, TaggedTreeSet::FindEntry(thread, newSet, key)));
        EXPECT_EQ(tset->NumberOfElements(), NODE_NUMBERS / 2 - 1);
        EXPECT_EQ(newSet->NumberOfElements(), NODE_NUMBERS / 2 - 2);
        bool success = IsVaildRBTree<TaggedTreeSet>(thread, newSet, newSet->GetRootEntries());
        EXPECT_TRUE(success);
        EXPECT_EQ(newSet->Capacity(), NODE_NUMBERS - 1);
    }
}
}  // namespace panda::test
