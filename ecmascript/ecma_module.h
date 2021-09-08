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

    void AddItem(const JSThread *thread, JSHandle<JSTaggedValue> itemName, JSHandle<JSTaggedValue> itemValue);

    void RemoveItem(const JSThread *thread, JSHandle<JSTaggedValue> itemName);

    void DebugPrint(const JSThread *thread, const CString &caller);

    static constexpr size_t NAME_DICTIONARY_OFFSET = ECMAObject::SIZE;
    ACCESSORS(NameDictionary, NAME_DICTIONARY_OFFSET, SIZE)

    DECL_VISIT_OBJECT_FOR_JS_OBJECT(ECMAObject, NAME_DICTIONARY_OFFSET, SIZE)

protected:
    void CopyModuleInternal(const JSThread *thread, JSHandle<EcmaModule> srcModule);

    friend class ModuleManager;
};

class ModuleStack {
public:
    ModuleStack() = default;
    ~ModuleStack() = default;

    NO_COPY_SEMANTIC(ModuleStack);
    NO_MOVE_SEMANTIC(ModuleStack);

    void PushModule(const CString &moduleName);
    void PopModule();
    const CString &GetTop();
    const CString &GetPrevModule();
    void DebugPrint(std::ostream &dump) const;

private:
    std::vector<CString> data_;
};

class ModuleManager {
public:
    explicit ModuleManager(EcmaVM *vm);
    ~ModuleManager() = default;

    void AddModule(JSHandle<JSTaggedValue> moduleName, JSHandle<JSTaggedValue> module);

    void RemoveModule(JSHandle<JSTaggedValue> moduleName);

    JSHandle<JSTaggedValue> GetModule(const JSThread *thread, JSHandle<JSTaggedValue> moduleName);

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
    NO_COPY_SEMANTIC(ModuleManager);
    NO_MOVE_SEMANTIC(ModuleManager);

    EcmaVM *vm_{nullptr};
    JSTaggedValue ecmaModules_{JSTaggedValue::Hole()};
    ModuleStack moduleStack_;

    friend class EcmaVM;
    friend class ModuleStack;
};
}  // namespace panda::ecmascript

#endif
