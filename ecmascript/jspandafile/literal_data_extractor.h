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

#ifndef ECMASCRIPT_LITERAL_DATA_EXTRACTOR_H
#define ECMASCRIPT_LITERAL_DATA_EXTRACTOR_H

#include "ecmascript/jspandafile/panda_file_translator.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
using EntityId = panda_file::File::EntityId;

enum class FieldTag : uint8_t { OBJECTLITERAL = 0, ARRAYLITERAL };

class LiteralDataExtractor {
public:
    explicit LiteralDataExtractor() = default;
    virtual ~LiteralDataExtractor() = default;

    DEFAULT_NOEXCEPT_MOVE_SEMANTIC(LiteralDataExtractor);
    DEFAULT_COPY_SEMANTIC(LiteralDataExtractor);

    static void ExtractObjectDatas(JSThread *thread, const panda_file::File *pf, size_t index,
                                   JSMutableHandle<TaggedArray> elements, JSMutableHandle<TaggedArray> properties,
                                   PandaFileTranslator *pft);
    static JSHandle<TaggedArray> GetDatasIgnoreType(JSThread *thread, const panda_file::File *pf, size_t index,
                                                    PandaFileTranslator *pft = nullptr);
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_LITERAL_DATA_EXTRACTOR_H
