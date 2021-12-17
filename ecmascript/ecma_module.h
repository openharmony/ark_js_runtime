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

#ifndef ECMASCRIPT_ECMA_MODULE_H
#define ECMASCRIPT_ECMA_MODULE_H

#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/mem/c_string.h"

namespace panda::ecmascript {
class EcmaVm;

// Forward declaration
class EcmaModule : public ECMAObject {
public:
    static EcmaModule *Cast(ObjectHeader *object)
    {
        return static_cast<EcmaModule *>(object);
    }

    JSHandle<JSTaggedValue> GetItem(const JSThread *thread, JSHandle<JSTaggedValue> itemName);

    static void AddItem(const JSThread *thread, JSHandle<EcmaModule> module, JSHandle<JSTaggedValue> itemName,
        JSHandle<JSTaggedValue> itemValue);

    static void RemoveItem(const JSThread *thread, JSHandle<EcmaModule> module, JSHandle<JSTaggedValue> itemName);

    void DebugPrint(const JSThread *thread, const CString &caller);

    static constexpr uint32_t DEAULT_DICTIONART_CAPACITY = 4;

    static constexpr size_t NAME_DICTIONARY_OFFSET = ECMAObject::SIZE;
    ACCESSORS(NameDictionary, NAME_DICTIONARY_OFFSET, SIZE)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(ECMAObject, NAME_DICTIONARY_OFFSET, SIZE)
    DECL_DUMP()

protected:
    static void CopyModuleInternal(const JSThread *thread, JSHandle<EcmaModule> dstModule,
        JSHandle<EcmaModule> srcModule);

    friend class ModuleManager;
};

class ModuleManager {
public:
    explicit ModuleManager(EcmaVM *vm);
    ~ModuleManager() = default;

    void AddModule(JSHandle<JSTaggedValue> moduleName, JSHandle<JSTaggedValue> module);

    void RemoveModule(JSHandle<JSTaggedValue> moduleName);

    JSHandle<JSTaggedValue> GetModule(const JSThread *thread, JSHandle<JSTaggedValue> moduleName);

    CString GenerateModuleFullPath(const std::string &currentPathFile, const CString &relativeFile);

    const CString &GetCurrentExportModuleName();

    const CString &GetPrevExportModuleName();

    void SetCurrentExportModuleName(const std::string_view &moduleFile);

    void RestoreCurrentExportModuleName();

    void AddModuleItem(const JSThread *thread, JSHandle<JSTaggedValue> itemName, JSHandle<JSTaggedValue> value);

    JSHandle<JSTaggedValue> GetModuleItem(const JSThread *thread, JSHandle<JSTaggedValue> module,
                                          JSHandle<JSTaggedValue> itemName);

    void CopyModule(const JSThread *thread, JSHandle<JSTaggedValue> src);

    void DebugPrint(const JSThread *thread, const CString &caller);

private:
    static constexpr uint32_t DEAULT_DICTIONART_CAPACITY = 4;

    NO_COPY_SEMANTIC(ModuleManager);
    NO_MOVE_SEMANTIC(ModuleManager);

    EcmaVM *vm_{nullptr};
    JSTaggedValue ecmaModules_{JSTaggedValue::Hole()};
    std::vector<CString> moduleNames_{DEAULT_DICTIONART_CAPACITY};

    friend class EcmaVM;
};
}  // namespace panda::ecmascript

#endif
