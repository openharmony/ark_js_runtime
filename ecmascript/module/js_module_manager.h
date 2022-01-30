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

#ifndef ECMASCRIPT_MODULE_MANAGER_H
#define ECMASCRIPT_MODULE_MANAGER_H

#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
class EcmaModuleManager {
public:
    explicit EcmaModuleManager(EcmaVM *vm);
    ~EcmaModuleManager() = default;

    void Instantiate(JSThread *thread, JSHandle<SourceTextModule> module);
    void Evaluate(JSThread *thread, JSHandle<SourceTextModule> module);

    void AddSourceModule(JSTaggedValue ecmaModule);
    void PopSourceModule();
    JSTaggedValue GetCurrentSourceModule();

    JSTaggedValue GetModuleValueInner(JSThread *thread, JSTaggedValue key);
    JSTaggedValue GetModuleValueOutter(JSThread *thread, JSTaggedValue key);
    void StoreModuleValue(JSThread *thread, JSTaggedValue key, JSTaggedValue value);
    JSHandle<SourceTextModule> HostGetImportedModule(JSThread *thread, const CString &referencingModule);
    JSHandle<SourceTextModule> HostResolveImportedModule(JSThread *thread, const std::string &referencingModule);
    JSTaggedValue GetModuleNamespace(JSThread *thread, JSTaggedValue localName);
    void AddResolveImportedModule(JSThread *thread, const panda_file::File &pf, const std::string &referencingModule);

private:
    NO_COPY_SEMANTIC(EcmaModuleManager);
    NO_MOVE_SEMANTIC(EcmaModuleManager);

    static constexpr uint32_t DEAULT_DICTIONART_CAPACITY = 4;

    EcmaVM *vm_{nullptr};
    JSTaggedValue ecmaResolvedModules_{JSTaggedValue::Hole()};
    CStack<CString> moduleStack_;

    friend class EcmaVM;
};
} // namespace panda::ecmascript

#endif