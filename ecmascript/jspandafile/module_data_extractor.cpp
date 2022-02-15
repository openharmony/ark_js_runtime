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

#include "module_data_extractor.h"
#include "accessor/module_data_accessor-inl.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/global_env.h"
#include "ecmascript/tagged_array-inl.h"
#include "libpandafile/literal_data_accessor-inl.h"

namespace panda::ecmascript {
using ModuleTag = jspandafile::ModuleTag;
using StringData = panda_file::StringData;

static constexpr auto MODULE_CLASS = "L_ESModuleRecord;";

JSHandle<JSTaggedValue> ModuleDataExtractor::ParseModule(JSThread *thread, const panda_file::File &pf,
                                                         const std::string &descriptor)
{
    Span<const uint32_t> classIndexes = pf.GetClasses();
    int moduleIdx = -1;
    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf.IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(pf, classId);
        const char *desc = utf::Mutf8AsCString(cda.GetDescriptor());
        if (std::strcmp(MODULE_CLASS, desc) == 0) { // module class
            cda.EnumerateFields([&](panda_file::FieldDataAccessor &field_accessor) -> void {
                panda_file::File::EntityId field_name_id = field_accessor.GetNameId();
                StringData sd = pf.GetStringData(field_name_id);
                if (std::strcmp((char *)sd.data, descriptor.c_str())) {
                    moduleIdx = field_accessor.GetValue<int32_t>().value();
                    return;
                }
            });
        }
    }
    ASSERT(moduleIdx != -1);
    panda_file::File::EntityId literalArraysId = pf.GetLiteralArraysId();
    panda_file::LiteralDataAccessor lda(pf, literalArraysId);
    panda_file::File::EntityId moduleId = lda.GetLiteralArrayId(moduleIdx);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<SourceTextModule> moduleRecord = factory->NewSourceTextModule();
    ModuleDataExtractor::ExtractModuleDatas(thread, pf, moduleId, moduleRecord);

    JSHandle<EcmaString> ecmaModuleFilename = factory->NewFromStdString(descriptor);
    moduleRecord->SetEcmaModuleFilename(thread, ecmaModuleFilename);

    moduleRecord->SetStatus(ModuleStatus::UNINSTANTIATED);
    return JSHandle<JSTaggedValue>::Cast(moduleRecord);
}

void ModuleDataExtractor::ExtractModuleDatas(JSThread *thread, const panda_file::File &pf,
                                             panda_file::File::EntityId moduleId,
                                             JSHandle<SourceTextModule> &moduleRecord)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    jspandafile::ModuleDataAccessor mda(pf, moduleId);
    std::vector<uint32_t> &requestModules = mda.getRequestModules();
    JSHandle<TaggedArray> requestModuleArray = factory->NewTaggedArray(requestModules.size());
    for (array_size_t idx = 0; idx < requestModules.size(); idx++) {
        StringData sd = pf.GetStringData(panda_file::File::EntityId(requestModules[idx]));
        JSTaggedValue value(factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii));
        requestModuleArray->Set(thread, idx, value);
    }
    moduleRecord->SetRequestedModules(thread, requestModuleArray);

    mda.EnumerateModuleRecord([factory, thread, &pf, requestModuleArray, moduleRecord]
        (const ModuleTag &tag, uint32_t exportNameOffset,
         uint32_t moduleRequestIdx, uint32_t importNameOffset, uint32_t localNameOffset) {
        size_t requestArraySize = requestModuleArray->GetLength();
        ASSERT((requestArraySize == 0 || moduleRequestIdx < requestArraySize));
        JSHandle<JSTaggedValue> defaultValue(thread, JSTaggedValue::Null());
        switch (tag) {
            case ModuleTag::REGULARIMPORT: {
                StringData sd = pf.GetStringData(panda_file::File::EntityId(localNameOffset));
                JSHandle<JSTaggedValue> localName(thread,
                    factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii));

                sd = pf.GetStringData(panda_file::File::EntityId(importNameOffset));
                JSHandle<JSTaggedValue> importName(thread,
                    factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii));

                JSMutableHandle<JSTaggedValue> moduleRequest(thread, JSTaggedValue::Undefined());
                if (requestArraySize != 0) {
                    moduleRequest.Update(requestModuleArray->Get(moduleRequestIdx));
                }
                JSHandle<ImportEntry> importEntry = factory->NewImportEntry(moduleRequest, importName, localName);
                moduleRecord->AddImportEntry(thread, importEntry);
                break;
            }
            case ModuleTag::NAMESPACEIMPORT: {
                StringData sd = pf.GetStringData(panda_file::File::EntityId(localNameOffset));
                JSHandle<JSTaggedValue> localName(thread,
                    factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii));

                JSMutableHandle<JSTaggedValue> moduleRequest(thread, JSTaggedValue::Undefined());
                if (requestArraySize != 0) {
                    moduleRequest.Update(requestModuleArray->Get(moduleRequestIdx));
                }
                JSHandle<JSTaggedValue> importName(thread, thread->GlobalConstants()->GetStarString());
                JSHandle<ImportEntry> importEntry = factory->NewImportEntry(moduleRequest, importName, localName);
                moduleRecord->AddImportEntry(thread, importEntry);
                break;
            }
            case ModuleTag::LOCALEXPORT: {
                StringData sd = pf.GetStringData(panda_file::File::EntityId(localNameOffset));
                JSHandle<JSTaggedValue> localName(thread,
                    factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii));

                sd = pf.GetStringData(panda_file::File::EntityId(exportNameOffset));
                JSHandle<JSTaggedValue> exportName(thread,
                    factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii));

                JSHandle<ExportEntry> exportEntry =
                    factory->NewExportEntry(exportName, defaultValue, defaultValue, localName);
                moduleRecord->AddLocalExportEntry(thread, exportEntry);
                break;
            }
            case ModuleTag::INDIRECTEXPORT: {
                StringData sd = pf.GetStringData(panda_file::File::EntityId(exportNameOffset));
                JSHandle<JSTaggedValue> exportName(thread,
                    factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii));

                sd = pf.GetStringData(panda_file::File::EntityId(importNameOffset));
                JSHandle<JSTaggedValue> importName(thread,
                    factory->GetRawStringFromStringTable(sd.data, sd.utf16_length, sd.is_ascii));

                JSMutableHandle<JSTaggedValue> moduleRequest(thread, JSTaggedValue::Undefined());
                if (requestArraySize != 0) {
                    moduleRequest.Update(requestModuleArray->Get(moduleRequestIdx));
                }

                JSHandle<ExportEntry> exportEntry =
                    factory->NewExportEntry(exportName, moduleRequest, importName, defaultValue);
                moduleRecord->AddIndirectExportEntry(thread, exportEntry);
                break;
            }
            case ModuleTag::STAREXPORT: {
                JSMutableHandle<JSTaggedValue> moduleRequest(thread, JSTaggedValue::Undefined());
                if (requestArraySize != 0) {
                    moduleRequest.Update(requestModuleArray->Get(moduleRequestIdx));
                }

                JSHandle<ExportEntry> exportEntry =
                    factory->NewExportEntry(defaultValue, moduleRequest, defaultValue, defaultValue);
                moduleRecord->AddStarExportEntry(thread, exportEntry);
                break;
            }
            default: {
                UNREACHABLE();
                break;
            }
        }
    });
}

bool ModuleDataExtractor::FindModule(const panda_file::File &pf)
{
    Span<const uint32_t> classIndexes = pf.GetClasses();

    for (const uint32_t index : classIndexes) {
        panda_file::File::EntityId classId(index);
        if (pf.IsExternal(classId)) {
            continue;
        }
        panda_file::ClassDataAccessor cda(pf, classId);
        const char *desc = utf::Mutf8AsCString(cda.GetDescriptor());
        if (std::strcmp(MODULE_CLASS, desc) == 0) {
            return true;
        }
    }
    return false;
}
}  // namespace panda::ecmascript
