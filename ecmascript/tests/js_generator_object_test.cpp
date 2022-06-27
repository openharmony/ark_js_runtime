/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/global_env.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/interpreter/slow_runtime_stub.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSGeneratorObjectTest : public testing::Test {
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

/**
 * @tc.name: GeneratorValidate
 * @tc.desc: Get the current status of the generator and return it. If the generator is executing, an exception will
 *           be thrown.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSGeneratorObjectTest, GeneratorValidate_001)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    JSHandle<JSGeneratorObject> genOjb = factory->NewJSGeneratorObject(env->GetGeneratorFunctionFunction());
    JSGeneratorState state = JSGeneratorObject::GeneratorValidate(thread, JSHandle<JSTaggedValue>::Cast(genOjb));
    EXPECT_EQ(state, JSGeneratorState::UNDEFINED);
}

HWTEST_F_L0(JSGeneratorObjectTest, GeneratorValidate_002)
{
    auto vm = thread->GetEcmaVM();
    auto env = vm->GetGlobalEnv();
    JSTaggedValue genObjTagVal =
        SlowRuntimeStub::CreateGeneratorObj(thread, env->GetGeneratorFunctionFunction().GetTaggedValue());
    JSHandle<JSGeneratorObject> genObj(thread, genObjTagVal);
    JSGeneratorState state = JSGeneratorObject::GeneratorValidate(thread, JSHandle<JSTaggedValue>::Cast(genObj));
    EXPECT_EQ(state, JSGeneratorState::SUSPENDED_START);
}

HWTEST_F_L0(JSGeneratorObjectTest, GeneratorValidate_003)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    JSHandle<JSHClass> genClass =
        factory->NewEcmaDynClass(JSObject::SIZE, JSType::JS_GENERATOR_OBJECT, env->GetGeneratorFunctionPrototype());
    TaggedObject *genObjectHeader = factory->NewDynObject(genClass);
    JSHandle<JSGeneratorObject> genObj(thread, JSGeneratorObject::Cast(genObjectHeader));
    JSGeneratorState state = JSGeneratorObject::GeneratorValidate(thread, JSHandle<JSTaggedValue>::Cast(genObj));
    EXPECT_EQ(state, JSGeneratorState::SUSPENDED_YIELD);

    genObj->SetGeneratorState(JSGeneratorState::COMPLETED);
    state = JSGeneratorObject::GeneratorValidate(thread, JSHandle<JSTaggedValue>::Cast(genObj));
    EXPECT_EQ(state, JSGeneratorState::COMPLETED);
}

/**
 * @tc.name: GeneratorResume
 * @tc.desc: Gets the next iteration value of the generator. If the generator has completed, returns
 *           object(Undefined, True).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSGeneratorObjectTest, GeneratorResume)
{
    auto vm = thread->GetEcmaVM();
    auto env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> valueKey = thread->GlobalConstants()->GetHandledValueString();
    JSHandle<JSTaggedValue> doneKey = thread->GlobalConstants()->GetHandledDoneString();
    JSTaggedValue genObjTagVal =
        SlowRuntimeStub::CreateGeneratorObj(thread, env->GetGeneratorFunctionFunction().GetTaggedValue());
    JSHandle<JSGeneratorObject> genObj(thread, genObjTagVal);

    // If state is completed, return object(undefined, true).
    genObj->SetGeneratorState(JSGeneratorState::COMPLETED);
    JSHandle<JSObject> result = JSGeneratorObject::GeneratorResume(thread, genObj, JSTaggedValue::Undefined());
    EXPECT_EQ((JSObject::GetProperty(thread, result, valueKey).GetValue()).GetTaggedValue(),
        JSTaggedValue::Undefined());
    EXPECT_EQ((JSObject::GetProperty(thread, result, doneKey).GetValue()).GetTaggedValue(), JSTaggedValue::True());
}

/**
 * @tc.name: GeneratorResumeAbrupt
 * @tc.desc: If the status of the generator is completed, the value in abruptcompletion is returned. Otherwise, the
 *           generator is in a suspended state, and the Return or Throw value of the generator is returned.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSGeneratorObjectTest, GeneratorResumeAbrupt)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> valueKey = thread->GlobalConstants()->GetHandledValueString();
    JSHandle<JSTaggedValue> doneKey = thread->GlobalConstants()->GetHandledDoneString();
    JSTaggedValue genObjTagVal =
        SlowRuntimeStub::CreateGeneratorObj(thread, env->GetGeneratorFunctionFunction().GetTaggedValue());
    JSHandle<JSGeneratorObject> genObj(thread, genObjTagVal);
    genObj->SetGeneratorState(JSGeneratorState::COMPLETED);
    CompletionRecordType type = CompletionRecordType::RETURN;
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSHandle<CompletionRecord> abruptCompletion = factory->NewCompletionRecord(type, value);
    JSHandle<JSObject> result = JSGeneratorObject::GeneratorResumeAbrupt(thread, genObj, abruptCompletion);
    EXPECT_EQ((JSObject::GetProperty(thread, result, valueKey).GetValue()).GetTaggedValue(), JSTaggedValue(1));
    EXPECT_EQ((JSObject::GetProperty(thread, result, doneKey).GetValue()).GetTaggedValue(), JSTaggedValue::True());
}
}  // namespace panda::test