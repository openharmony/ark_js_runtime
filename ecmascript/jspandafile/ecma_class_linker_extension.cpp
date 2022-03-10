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

#include "ecmascript/jspandafile/ecma_class_linker_extension.h"
#include "ecmascript/ecma_string.h"
#include "include/class_linker-inl.h"
#include "include/coretypes/class.h"

namespace panda::ecmascript {
using SourceLang = panda_file::SourceLang;

bool EcmaClassLinkerExtension::InitializeImpl(bool cmpStrEnabled)
{
    return true;
}

EcmaClassLinkerExtension::~EcmaClassLinkerExtension()
{
    if (!IsInitialized()) {
        return;
    }
    FreeLoadedClasses();
}

void EcmaClassLinkerExtension::InitClasses(EcmaVM *vm)
{
    ASSERT(IsInitialized());
    vm_ = vm;
    LanguageContext ctx = Runtime::GetCurrent()->GetLanguageContext(GetLanguage());
    auto *objClass = NewClass(ctx.GetObjectClassDescriptor(), 0, 0, GetClassSize(ClassRoot::OBJECT));
    if (objClass == nullptr) {
        return;
    }
    objClass->SetObjectSize(TaggedObject::TaggedObjectSize());
    objClass->SetSourceLang(SourceLang::ECMASCRIPT);
    objClass->SetState(Class::State::LOADED);
    Runtime::GetCurrent()->GetClassLinker()->AddClassRoot(ClassRoot::OBJECT, objClass);
}

ClassLinkerContext *EcmaClassLinkerExtension::CreateApplicationClassLinkerContext(const PandaVector<PandaString> &path)
{
    PandaVector<PandaFilePtr> app_files;
    app_files.reserve(path.size());
    for (auto &p : path) {
        auto pf = panda_file::OpenPandaFileOrZip(p, panda_file::File::READ_WRITE);
        if (pf == nullptr) {
            return nullptr;
        }
        app_files.push_back(std::move(pf));
    }
    return ClassLinkerExtension::CreateApplicationClassLinkerContext(std::move(app_files));
}

Class *EcmaClassLinkerExtension::NewClass(const uint8_t *descriptor, size_t vtableSize, size_t imtSize, size_t size)
{
    ASSERT(IsInitialized());
    if (vm_ == nullptr) {
        return nullptr;
    }
    void *ptr = vm_->GetChunk()->Allocate(coretypes::Class::GetSize(size));
    // CODECHECK-NOLINTNEXTLINE(CPP_RULE_ID_SMARTPOINTER_INSTEADOF_ORIGINPOINTER)
    auto *res = reinterpret_cast<coretypes::Class *>(ptr);
    res->InitClass(descriptor, vtableSize, imtSize, size);
    res->SetClass(GetClassRoot(ClassRoot::CLASS));
    auto *klass = res->GetRuntimeClass();
    klass->SetManagedObject(res);
    klass->SetSourceLang(GetLanguage());
    return klass;
}

size_t EcmaClassLinkerExtension::GetClassSize([[maybe_unused]] ClassRoot root)
{
    ASSERT(IsInitialized());
    // Used only in test scenarios.
    return sizeof(Class);
}

size_t EcmaClassLinkerExtension::GetArrayClassSize()
{
    ASSERT(IsInitialized());

    return GetClassSize(ClassRoot::OBJECT);
}

void EcmaClassLinkerExtension::FreeClass([[maybe_unused]] Class *klass)
{
    ASSERT(IsInitialized());
    if (vm_ == nullptr) {
        return;
    }

    auto *cls = coretypes::Class::FromRuntimeClass(klass);
    auto chunk = vm_->GetChunk();
    chunk->Delete(cls);
}
}  // namespace panda::ecmascript
