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

#ifndef ECMASCRIPT_ECMA_CLASS_LINKER_EXTENSION_H
#define ECMASCRIPT_ECMA_CLASS_LINKER_EXTENSION_H

#include "libpandafile/file_items.h"
#include "include/class_linker.h"
#include "include/class_linker_extension.h"

namespace panda::coretypes {
class Class;  // NOLINT(bugprone-forward-declaration-namespace)
}  // namespace panda::coretypes

namespace panda {
namespace ecmascript {
class EcmaVM;
class EcmaClassLinkerExtension : public ClassLinkerExtension {
public:
    static EcmaClassLinkerExtension *Cast(ClassLinkerExtension *object)
    {
        return reinterpret_cast<EcmaClassLinkerExtension *>(object);
    }

    EcmaClassLinkerExtension() : ClassLinkerExtension(panda_file::SourceLang::ECMASCRIPT) {}

    ~EcmaClassLinkerExtension() override;

    NO_COPY_SEMANTIC(EcmaClassLinkerExtension);
    NO_MOVE_SEMANTIC(EcmaClassLinkerExtension);

    void InitClasses(EcmaVM *vm);

    ClassLinkerContext *CreateApplicationClassLinkerContext(const PandaVector<PandaString> &path) override;

    bool CanThrowException([[maybe_unused]] const Method *method) const override
    {
        return true;
    }

    void InitializeArrayClass([[maybe_unused]] Class *array_class, [[maybe_unused]] Class *componentClass) override {}

    void InitializePrimitiveClass([[maybe_unused]] Class *primitiveClass) override {}

    size_t GetClassVTableSize([[maybe_unused]] ClassRoot root) override
    {
        return 0;
    }

    size_t GetClassIMTSize([[maybe_unused]] ClassRoot root) override
    {
        return 0;
    }

    size_t GetClassSize([[maybe_unused]] ClassRoot root) override;

    size_t GetArrayClassVTableSize() override
    {
        return 0;
    }

    size_t GetArrayClassIMTSize() override
    {
        return 0;
    }

    size_t GetArrayClassSize() override;
    Class *CreateClass([[maybe_unused]] const uint8_t *descriptor, [[maybe_unused]] size_t vtableSize, size_t imtSize,
                       [[maybe_unused]] size_t size) override
    {
        return NewClass(descriptor, vtableSize, imtSize, size);
    }

    void InitializeClass([[maybe_unused]] Class *klass) override {}

    const void *GetNativeEntryPointFor([[maybe_unused]] Method *method) const override
    {
        return nullptr;
    }

    ClassLinkerErrorHandler *GetErrorHandler() override
    {
        return nullptr;
    }

    void FreeClass(Class *klass) override;

private:
    bool InitializeImpl(bool cmpStrEnabled) override;
    Class *NewClass(const uint8_t *descriptor, size_t vtableSize, size_t imtSize, size_t size);

    EcmaVM *vm_{nullptr};
};
}  // namespace ecmascript
}  // namespace panda

#endif  // ECMASCRIPT_ECMA_CLASS_LINKER_EXTENSION_H
