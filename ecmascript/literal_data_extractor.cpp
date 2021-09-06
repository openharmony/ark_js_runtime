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

#include "literal_data_extractor.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_string.h"

#include "ecmascript/js_thread.h"
#include "ecmascript/tagged_array-inl.h"
#include "libpandafile/literal_data_accessor-inl.h"

namespace panda::ecmascript {
using LiteralTag = panda_file::LiteralTag;
using StringData = panda_file::StringData;
using LiteralValue = panda_file::LiteralDataAccessor::LiteralValue;

void LiteralDataExtractor::ExtractObjectDatas(JSThread *thread, const panda_file::File *pf, size_t index,
                                              JSMutableHandle<TaggedArray> elements,
                                              JSMutableHandle<TaggedArray> properties, PandaFileTranslator *pft)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    LOG_ECMA(DEBUG) << "Panda File" << pf->GetFilename();
    panda_file::File::EntityId literalArraysId = pf->GetLiteralArraysId();
    panda_file::LiteralDataAccessor lda(*pf, literalArraysId);

    uint32_t num = lda.GetLiteralValsNum(index) / 2;  // 2: half
    elements.Update(factory->NewTaggedArray(num).GetTaggedValue());
    properties.Update(factory->NewTaggedArray(num).GetTaggedValue());
    array_size_t epos = 0;
    array_size_t ppos = 0;
    const uint8_t pairSize = 2;
    lda.EnumerateLiteralVals(
        index, [elements, properties, &epos, &ppos, factory, thread, pft](const LiteralValue &value,
                                                                          const LiteralTag &tag) {
        JSTaggedValue jt = JSTaggedValue::Null();
        bool flag = false;
        switch (tag) {
            case LiteralTag::INTEGER: {
                jt = JSTaggedValue(std::get<uint32_t>(value));
                break;
            }
            case LiteralTag::DOUBLE: {
                jt = JSTaggedValue(std::get<double>(value));
                break;
            }
            case LiteralTag::BOOL: {
                jt = JSTaggedValue(std::get<bool>(value));
                break;
            }
            case LiteralTag::STRING: {
                StringData sd = std::get<StringData>(value);
                EcmaString *str = factory->GetRawStringFromStringTable(sd.data, sd.utf16_length);
                jt = JSTaggedValue(str);
                uint32_t index = 0;
                if (JSTaggedValue::ToElementIndex(jt, &index) && ppos % pairSize == 0) {
                    flag = true;
                }
                break;
            }
            case LiteralTag::METHOD: {
                ASSERT(pft != nullptr);
                uint32_t methodId = std::get<uint32_t>(value);
                JSHandle<JSFunction> jsFunc = pft->DefineMethodById(methodId, FunctionKind::NORMAL_FUNCTION);
                jt = jsFunc.GetTaggedValue();
                break;
            }
            case LiteralTag::GENERATORMETHOD: {
                ASSERT(pft != nullptr);
                uint32_t methodId = std::get<uint32_t>(value);
                JSHandle<JSFunction> jsFunc = pft->DefineMethodById(methodId, FunctionKind::GENERATOR_FUNCTION);
                jt = jsFunc.GetTaggedValue();
                break;
            }
            case LiteralTag::ACCESSOR: {
                JSHandle<AccessorData> accessor = factory->NewAccessorData();
                jt = JSTaggedValue(accessor.GetTaggedValue());
                break;
            }
            case LiteralTag::NULLVALUE: {
                break;
            }
            default: {
                UNREACHABLE();
                break;
            }
        }
        if (epos % pairSize == 0 && !flag) {
            properties->Set(thread, ppos++, jt);
        } else {
            elements->Set(thread, epos++, jt);
        }
    });
}

JSHandle<TaggedArray> LiteralDataExtractor::GetDatasIgnoreType(JSThread *thread, const panda_file::File *pf,
                                                               size_t index, PandaFileTranslator *pft)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    LOG_ECMA(DEBUG) << "Panda File" << pf->GetFilename();
    panda_file::File::EntityId literalArraysId = pf->GetLiteralArraysId();
    panda_file::LiteralDataAccessor lda(*pf, literalArraysId);

    uint32_t num = lda.GetLiteralValsNum(index) / 2;  // 2: half
    JSHandle<TaggedArray> literals = factory->NewTaggedArray(num);
    array_size_t pos = 0;
    lda.EnumerateLiteralVals(
        index, [literals, &pos, factory, thread, pft](const panda_file::LiteralDataAccessor::LiteralValue &value,
                                                      const LiteralTag &tag) {
            JSTaggedValue jt = JSTaggedValue::Null();
            switch (tag) {
                case LiteralTag::INTEGER: {
                    jt = JSTaggedValue(std::get<uint32_t>(value));
                    break;
                }
                case LiteralTag::DOUBLE: {
                    jt = JSTaggedValue(std::get<double>(value));
                    break;
                }
                case LiteralTag::BOOL: {
                    jt = JSTaggedValue(std::get<bool>(value));
                    break;
                }
                case LiteralTag::STRING: {
                    StringData sd = std::get<StringData>(value);
                    EcmaString *str = factory->GetRawStringFromStringTable(sd.data, sd.utf16_length);
                    jt = JSTaggedValue(str);
                    break;
                }
                case LiteralTag::METHOD: {
                    ASSERT(pft != nullptr);
                    uint32_t methodId = std::get<uint32_t>(value);
                    JSHandle<JSFunction> jsFunc = pft->DefineMethodById(methodId, FunctionKind::NORMAL_FUNCTION);
                    jt = jsFunc.GetTaggedValue();
                    break;
                }
                case LiteralTag::GENERATORMETHOD: {
                    ASSERT(pft != nullptr);
                    uint32_t methodId = std::get<uint32_t>(value);
                    JSHandle<JSFunction> jsFunc = pft->DefineMethodById(methodId, FunctionKind::GENERATOR_FUNCTION);
                    jt = jsFunc.GetTaggedValue();
                    break;
                }
                case LiteralTag::ACCESSOR: {
                    JSHandle<AccessorData> accessor = factory->NewAccessorData();
                    jt = accessor.GetTaggedValue();
                    break;
                }
                case LiteralTag::NULLVALUE: {
                    break;
                }
                default: {
                    UNREACHABLE();
                    break;
                }
            }
            literals->Set(thread, pos++, jt);
        });
    return literals;
}
}  // namespace panda::ecmascript
