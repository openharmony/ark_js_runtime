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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/ts_types/ts_type_table.h"
#include "ecmascript/ts_types/ts_obj_layout_info-inl.h"

using namespace panda::ecmascript;
namespace panda::test {
class TSTypeTest : public testing::Test {
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

    PandaVM *instance {nullptr};
    EcmaVM *ecmaVm = nullptr;
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

HWTEST_F_L0(TSTypeTest, UnionType)
{
    auto factory = ecmaVm->GetFactory();
    JSHandle<TSTypeTable> table = factory->NewTSTypeTable(2);

    const int literalLength = 4;
    const int unionLength = 2;
    JSHandle<TaggedArray> literal = factory->NewTaggedArray(literalLength);
    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeTable::TypeLiteralFlag::UNION)));
    literal->Set(thread, 1, JSTaggedValue(unionLength));
    literal->Set(thread, 2, JSTaggedValue(51));
    literal->Set(thread, 3, JSTaggedValue(57));

    CVector<JSHandle<EcmaString>> recordImportMdoules {};
    JSHandle<JSTaggedValue> type = TSTypeTable::ParseType(thread, table, literal,
                                                          factory->NewFromString(CString("test")),
                                                          recordImportMdoules);
    ASSERT_TRUE(type->IsTSUnionType());

    JSHandle<TSUnionType> unionType = JSHandle<TSUnionType>(type);
    ASSERT_TRUE(unionType->GetComponentTypes().IsTaggedArray());

    JSHandle<TaggedArray> unionArray(thread, unionType->GetComponentTypes());
    ASSERT_EQ(unionArray->GetLength(), unionLength);
    ASSERT_EQ(unionArray->Get(0).GetInt(), 51);
    ASSERT_EQ(unionArray->Get(1).GetInt(), 57);
}

HWTEST_F_L0(TSTypeTest, InterfaceType)
{
    auto factory = ecmaVm->GetFactory();
    JSHandle<TSTypeTable> table = factory->NewTSTypeTable(2);

    const int literalLength = 12;
    const uint32_t propsNum = 2;
    JSHandle<TaggedArray> literal = factory->NewTaggedArray(literalLength);
    JSHandle<EcmaString> propsNameA = factory->NewFromCanBeCompressString("propsA");
    JSHandle<EcmaString> propsNameB = factory->NewFromCanBeCompressString("propsB");
    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeTable::TypeLiteralFlag::INTERFACE)));
    literal->Set(thread, 1, JSTaggedValue(0));
    literal->Set(thread, 2, JSTaggedValue(propsNum));
    literal->Set(thread, 3, propsNameA.GetTaggedValue());
    literal->Set(thread, 4, JSTaggedValue(static_cast<int>(TSTypeKind::TS_STRING)));
    literal->Set(thread, 5, JSTaggedValue(0));
    literal->Set(thread, 6, JSTaggedValue(0));
    literal->Set(thread, 7, propsNameB.GetTaggedValue());
    literal->Set(thread, 8, JSTaggedValue(static_cast<int>(TSTypeKind::TS_NUMBER)));
    literal->Set(thread, 9, JSTaggedValue(0));
    literal->Set(thread, 10, JSTaggedValue(0));
    literal->Set(thread, 11, JSTaggedValue(0));

    CVector<JSHandle<EcmaString>> recordImportMdoules {};
    JSHandle<JSTaggedValue> type = TSTypeTable::ParseType(thread, table, literal,
                                                          factory->NewFromString(CString("test")),
                                                          recordImportMdoules);
    ASSERT_TRUE(type->IsTSInterfaceType());
    JSHandle<TSInterfaceType> interfaceType = JSHandle<TSInterfaceType>(type);
    JSHandle<TSObjectType> fields =  JSHandle<TSObjectType>(thread, interfaceType->GetFields());
    TSObjLayoutInfo *fieldsTypeInfo = TSObjLayoutInfo::Cast(fields->GetObjLayoutInfo().GetTaggedObject());

    ASSERT_EQ(fieldsTypeInfo->GetPropertiesCapacity(), propsNum);
    ASSERT_EQ(fieldsTypeInfo->GetKey(0), propsNameA.GetTaggedValue());
    ASSERT_EQ(fieldsTypeInfo->GetTypeId(0).GetInt(), static_cast<int>(TSTypeKind::TS_STRING));
    ASSERT_EQ(fieldsTypeInfo->GetKey(1), propsNameB.GetTaggedValue());
    ASSERT_EQ(fieldsTypeInfo->GetTypeId(1).GetInt(), static_cast<int>(TSTypeKind::TS_NUMBER));
}

HWTEST_F_L0(TSTypeTest, ClassType)
{
    auto factory = ecmaVm->GetFactory();
    JSHandle<TSTypeTable> table = factory->NewTSTypeTable(2);

    const int literalLength = 18;
    const uint32_t propsNum = 2;
    JSHandle<TaggedArray> literal = factory->NewTaggedArray(literalLength);
    JSHandle<EcmaString> propsNameA = factory->NewFromCanBeCompressString("propsA");
    JSHandle<EcmaString> propsNameB = factory->NewFromCanBeCompressString("propsB");
    JSHandle<EcmaString> funcName = factory->NewFromCanBeCompressString("constructor");
    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeTable::TypeLiteralFlag::CLASS)));
    literal->Set(thread, 1, JSTaggedValue(0));
    literal->Set(thread, 2, JSTaggedValue(0));
    literal->Set(thread, 3, JSTaggedValue(0));
    literal->Set(thread, 4, JSTaggedValue(propsNum));
    literal->Set(thread, 5, propsNameA.GetTaggedValue());
    literal->Set(thread, 6, JSTaggedValue(static_cast<int>(TSTypeKind::TS_STRING)));
    literal->Set(thread, 7, JSTaggedValue(0));
    literal->Set(thread, 8, JSTaggedValue(0));
    literal->Set(thread, 9, propsNameB.GetTaggedValue());
    literal->Set(thread, 10, JSTaggedValue(static_cast<int>(TSTypeKind::TS_NUMBER)));
    literal->Set(thread, 11, JSTaggedValue(0));
    literal->Set(thread, 12, JSTaggedValue(0));
    literal->Set(thread, 13, JSTaggedValue(1));
    literal->Set(thread, 14, funcName.GetTaggedValue());
    literal->Set(thread, 15, JSTaggedValue(static_cast<int>(TSTypeTable::TypeLiteralFlag::FUNCTION)));
    literal->Set(thread, 16, JSTaggedValue(0));
    literal->Set(thread, 17, JSTaggedValue(0));

    CVector<JSHandle<EcmaString>> recordImportMdoules {};
    JSHandle<JSTaggedValue> type = TSTypeTable::ParseType(thread, table, literal,
                                                          factory->NewFromString(CString("test")),
                                                          recordImportMdoules);
    ASSERT_TRUE(type->IsTSClassType());
    JSHandle<TSClassType> classType = JSHandle<TSClassType>(type);
    JSHandle<TSObjectType> fields =  JSHandle<TSObjectType>(thread, classType->GetInstanceType());
    TSObjLayoutInfo *fieldsTypeInfo = TSObjLayoutInfo::Cast(fields->GetObjLayoutInfo().GetTaggedObject());

    ASSERT_EQ(fieldsTypeInfo->GetPropertiesCapacity(), propsNum);
    ASSERT_EQ(fieldsTypeInfo->GetKey(0), propsNameA.GetTaggedValue());
    ASSERT_EQ(fieldsTypeInfo->GetTypeId(0).GetInt(), static_cast<int>(TSTypeKind::TS_STRING));
    ASSERT_EQ(fieldsTypeInfo->GetKey(1), propsNameB.GetTaggedValue());
    ASSERT_EQ(fieldsTypeInfo->GetTypeId(1).GetInt(), static_cast<int>(TSTypeKind::TS_NUMBER));
}

HWTEST_F_L0(TSTypeTest, ClassInstanceType)
{
    auto factory = ecmaVm->GetFactory();
    JSHandle<TSTypeTable> table = factory->NewTSTypeTable(3);
    JSHandle<TSClassType> extendClass = factory->NewTSClassType();
    const int literalLength = 2;
    table->Set(thread, 0, extendClass);
    JSHandle<TaggedArray> literal = factory->NewTaggedArray(literalLength);

    literal->Set(thread, 0, JSTaggedValue(static_cast<int>(TSTypeTable::TypeLiteralFlag::CLASS_INSTANCE)));
    literal->Set(thread, 1, JSTaggedValue(50));

    CVector<JSHandle<EcmaString>> recordImportMdoules {};
    JSHandle<JSTaggedValue> type = TSTypeTable::ParseType(thread, table, literal,
                                                          factory->NewFromString(CString("test")),
                                                          recordImportMdoules);
    ASSERT_TRUE(type->IsTSClassInstanceType());
    JSHandle<TSClassInstanceType> classInstanceType = JSHandle<TSClassInstanceType>(type);

    ASSERT_EQ(classInstanceType->GetCreateClassType(), extendClass.GetTaggedValue());
}
}
