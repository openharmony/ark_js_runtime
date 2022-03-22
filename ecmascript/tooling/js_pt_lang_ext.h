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

#ifndef ECMASCRIPT_TOOLING_JS_PT_LANG_EXT_H
#define ECMASCRIPT_TOOLING_JS_PT_LANG_EXT_H

namespace panda::tooling::ecmascript {
class JSPtLangExt : public PtLangExt {
public:
    JSPtLangExt() = default;
    ~JSPtLangExt() override = default;

    // PtValue API
    PtObject ValueToObject(PtValue value) const override
    {
        return PtObject();
    }

    // PtClass API
    PtClass GetClass(PtObject object) const override
    {
        return PtClass();
    }
    PtClass GetClass(PtProperty property) const override
    {
        return PtClass();
    }
    void ReleaseClass(PtClass klass) const override {}
    const char *GetClassDescriptor(PtClass klass) const override
    {
        return nullptr;
    }

    // PtObject API
    PandaList<PtProperty> GetProperties(PtObject object) const override
    {
        return {};
    }
    PtProperty GetProperty(PtObject object, const char *propertyName) const override
    {
        return PtProperty();
    }
    bool AddProperty(PtObject object, const char *propertyName, PtValue value) const override
    {
        return false;
    }
    bool RemoveProperty(PtObject object, const char *propertyName) const override
    {
        return false;
    }

    // PtProperty API
    const char *GetPropertyName(PtProperty property) const override
    {
        return nullptr;
    }
    PtValue GetPropertyValue(PtProperty property) const override
    {
        return PtValue();
    }
    void SetPropertyPtValue(PtProperty property, PtValue value) const override {}
    void ReleasePtValue(const PtValue *value) const override {}

    NO_COPY_SEMANTIC(JSPtLangExt);
    NO_MOVE_SEMANTIC(JSPtLangExt);
};
}  // namespace panda::tooling::ecmascript
#endif  // ECMASCRIPT_TOOLING_JS_PT_LANG_EXT_H