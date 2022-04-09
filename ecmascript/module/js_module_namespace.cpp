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

#include "ecmascript/module/js_module_namespace.h"

#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/module/js_module_record.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/base/string_helper.h"

namespace panda::ecmascript {
JSHandle<ModuleNamespace> ModuleNamespace::ModuleNamespaceCreate(JSThread *thread,
                                                                 const JSHandle<JSTaggedValue> &module,
                                                                 const JSHandle<TaggedArray> &exports)
{
    auto globalConst = thread->GlobalConstants();
    // 1. Assert: module is a Module Record.
    ASSERT(module->IsModuleRecord());
    // 2. Assert: module.[[Namespace]] is undefined.
    JSHandle<ModuleRecord> moduleRecord = JSHandle<ModuleRecord>::Cast(module);
    ASSERT(ModuleRecord::GetNamespace(moduleRecord.GetTaggedValue()).IsUndefined());
    // 3. Assert: exports is a List of String values.
    // 4. Let M be a newly created object.
    // 5. Set M's essential internal methods to the definitions specified in 9.4.6.
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ModuleNamespace> mNp = factory->NewModuleNamespace();
    // 6. Set M.[[Module]] to module.
    mNp->SetModule(thread, module);
    // 7. Let sortedExports be a new List containing the same values as the list exports where the values
    // are ordered as if an Array of the same values had been sorted using
    // Array.prototype.sort using undefined as comparefn.
    JSHandle<JSArray> exportsArray = JSArray::CreateArrayFromList(thread, exports);
    JSHandle<JSObject> sortedExports = JSHandle<JSObject>::Cast(exportsArray);
    JSHandle<JSTaggedValue> fn = globalConst->GetHandledUndefined();
    JSArray::Sort(thread, sortedExports, fn);
    // 8. Set M.[[Exports]] to sortedExports.
    mNp->SetExports(thread, sortedExports);
    // 9. Create own properties of M corresponding to the definitions in 26.3.

    JSHandle<JSTaggedValue> toStringTag = thread->GetEcmaVM()->GetGlobalEnv()->GetToStringTagSymbol();
    JSHandle<JSTaggedValue> moduleString = globalConst->GetHandledModuleString();
    PropertyDescriptor des(thread, moduleString, false, false, false);
    JSHandle<JSObject> mNpObj = JSHandle<JSObject>::Cast(mNp);
    JSObject::DefineOwnProperty(thread, mNpObj, toStringTag, des);
    // 10. Set module.[[Namespace]] to M.
    ModuleRecord::SetNamespace(thread, moduleRecord.GetTaggedValue(), mNp.GetTaggedValue());
    return mNp;
}

OperationResult ModuleNamespace::GetProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                             const JSHandle<JSTaggedValue> &key)
{
    // 1. Assert: IsPropertyKey(P) is true.
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    // 2. If Type(P) is Symbol, then
    //   a. Return ? OrdinaryGet(O, P, Receiver).
    if (key->IsSymbol()) {
        return JSObject::GetProperty(thread, obj, key);
    }
    JSHandle<ModuleNamespace> moduleNamespace = JSHandle<ModuleNamespace>::Cast(obj);
    // 3. Let exports be O.[[Exports]].
    JSHandle<JSTaggedValue> exports(thread, moduleNamespace->GetExports());
    // 4. If P is not an element of exports, return undefined.
    if (exports->IsUndefined()) {
        return OperationResult(thread, thread->GlobalConstants()->GetUndefined(), PropertyMetaData(false));
    }
    if (!JSArray::IncludeInSortedValue(thread, exports, key)) {
        return OperationResult(thread, thread->GlobalConstants()->GetUndefined(), PropertyMetaData(false));
    }
    // 5. Let m be O.[[Module]].
    JSHandle<SourceTextModule> mm(thread, moduleNamespace->GetModule());
    // 6. Let binding be ! m.ResolveExport(P, « »).
    CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> resolveSet;
    JSHandle<JSTaggedValue> binding = SourceTextModule::ResolveExport(thread, mm, key, resolveSet);
    // 7. Assert: binding is a ResolvedBinding Record.
    ASSERT(binding->IsResolvedBinding());
    // 8. Let targetModule be binding.[[Module]].
    JSHandle<ResolvedBinding> resolvedBind = JSHandle<ResolvedBinding>::Cast(binding);
    JSTaggedValue targetModule = resolvedBind->GetModule();
    // 9. Assert: targetModule is not undefined.
    ASSERT(!targetModule.IsUndefined());
    JSTaggedValue result = SourceTextModule::Cast(targetModule.GetHeapObject())->
                                                  GetModuleValue(thread, resolvedBind->GetBindingName(), true);
    return OperationResult(thread, result, PropertyMetaData(true));
}

JSHandle<TaggedArray> ModuleNamespace::OwnPropertyKeys(JSThread *thread, const JSHandle<JSTaggedValue> &obj)
{
    ASSERT(obj->IsModuleNamespace());
    // 1. Let exports be a copy of O.[[Exports]].
    JSHandle<ModuleNamespace> moduleNamespace = JSHandle<ModuleNamespace>::Cast(obj);
    JSHandle<JSTaggedValue> exports(thread, moduleNamespace->GetExports());
    JSHandle<TaggedArray> exportsArray = JSArray::ToTaggedArray(thread, exports);
    if (!moduleNamespace->ValidateKeysAvailable(thread, exportsArray)) {
        return exportsArray;
    }

    // 2. Let symbolKeys be ! OrdinaryOwnPropertyKeys(O).
    JSHandle<TaggedArray> symbolKeys = JSObject::GetOwnPropertyKeys(thread, JSHandle<JSObject>(obj));
    // 3. Append all the entries of symbolKeys to the end of exports.
    JSHandle<TaggedArray> result = TaggedArray::Append(thread, exportsArray, symbolKeys);
    // 4. Return exports.
    return result;
}

bool ModuleNamespace::IsExtensible()
{
    return false;
}

bool ModuleNamespace::PreventExtensions()
{
    return true;
}

bool ModuleNamespace::DefineOwnProperty([[maybe_unused]] JSThread *thread,
                                        [[maybe_unused]] const JSHandle<JSTaggedValue> &obj,
                                        [[maybe_unused]] const JSHandle<JSTaggedValue> &key,
                                        [[maybe_unused]] PropertyDescriptor desc)
{
    return false;
}

bool ModuleNamespace::HasProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                  const JSHandle<JSTaggedValue> &key)
{
    ASSERT(obj->IsModuleNamespace());
    // 1. If Type(P) is Symbol, return OrdinaryHasProperty(O, P).
    if (key->IsSymbol()) {
        return JSObject::HasProperty(thread, JSHandle<JSObject>(obj), key);
    }
    // 2. Let exports be O.[[Exports]].
    JSHandle<ModuleNamespace> moduleNamespace = JSHandle<ModuleNamespace>::Cast(obj);
    JSHandle<JSTaggedValue> exports(thread, moduleNamespace->GetExports());
    // 3. If P is an element of exports, return true.
    if (exports->IsUndefined()) {
        return false;
    }
    if (JSArray::IncludeInSortedValue(thread, exports, key)) {
        return true;
    }
    // 4. Return false.
    return false;
}

bool ModuleNamespace::SetPrototype([[maybe_unused]]const JSHandle<JSTaggedValue> &obj,
                                   const JSHandle<JSTaggedValue> &proto)
{
    ASSERT(obj->IsModuleNamespace());
    // 1. Assert: Either Type(V) is Object or Type(V) is Null.
    ASSERT(proto->IsECMAObject() || proto->IsNull());
    return proto->IsNull();
}

bool ModuleNamespace::GetOwnProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                     const JSHandle<JSTaggedValue> &key, PropertyDescriptor &desc)
{
    // 1. If Type(P) is Symbol, return OrdinaryGetOwnProperty(O, P).
    if (key->IsSymbol()) {
        return JSObject::GetOwnProperty(thread, JSHandle<JSObject>(obj), key, desc);
    }
    // 2. Let exports be O.[[Exports]].
    JSHandle<ModuleNamespace> moduleNamespace = JSHandle<ModuleNamespace>::Cast(obj);
    JSHandle<JSTaggedValue> exports(thread, moduleNamespace->GetExports());
    // 3. If P is not an element of exports, return undefined.
    if (exports->IsUndefined()) {
        return false;
    }
    if (!JSArray::IncludeInSortedValue(thread, exports, key)) {
        return false;
    }
    // 4. Let value be ? O.[[Get]](P, O).
    JSHandle<JSTaggedValue> value = ModuleNamespace::GetProperty(thread, obj, key).GetValue();
    RETURN_VALUE_IF_ABRUPT_COMPLETION(thread, false);
    // 5. Return PropertyDescriptor {
    //    [[Value]]: value, [[Writable]]: true, [[Enumerable]]: true, [[Configurable]]: false }.
    desc.SetValue(value);
    desc.SetEnumerable(true);
    desc.SetWritable(true);
    desc.SetConfigurable(false);
    return true;
}

bool ModuleNamespace::SetProperty(JSThread *thread, bool mayThrow)
{
    if (mayThrow) {
        THROW_TYPE_ERROR_AND_RETURN(thread, "Cannot assign to read only property of Object Module", false);
    }
    return false;
}

bool ModuleNamespace::DeleteProperty(JSThread *thread, const JSHandle<JSTaggedValue> &obj,
                                     const JSHandle<JSTaggedValue> &key)
{
    ASSERT(obj->IsModuleNamespace());
    // 1. Assert: IsPropertyKey(P) is true.
    ASSERT_PRINT(JSTaggedValue::IsPropertyKey(key), "Key is not a property key");
    // 2. If Type(P) is Symbol, then
    //    Return ? OrdinaryDelete(O, P).
    if (key->IsSymbol()) {
        return JSObject::DeleteProperty(thread, JSHandle<JSObject>(obj), key);
    }
    // 3. Let exports be O.[[Exports]].
    JSHandle<ModuleNamespace> moduleNamespace = JSHandle<ModuleNamespace>::Cast(obj);
    JSHandle<JSTaggedValue> exports(thread, moduleNamespace->GetExports());
    // 4. If P is an element of exports, return false.
    if (exports->IsUndefined()) {
        return true;
    }
    if (JSArray::IncludeInSortedValue(thread, exports, key)) {
        return false;
    }
    return true;
}

bool ModuleNamespace::ValidateKeysAvailable(JSThread *thread, const JSHandle<TaggedArray> &exports)
{
    JSHandle<ModuleNamespace> moduleNamespace(thread, this);
    JSHandle<SourceTextModule> mm(thread, moduleNamespace->GetModule());
    int32_t exportsLength = exports->GetLength();
    for (int32_t idx = 0; idx < exportsLength; idx++) {
        JSHandle<JSTaggedValue> key(thread, exports->Get(idx));
        CVector<std::pair<JSHandle<SourceTextModule>, JSHandle<JSTaggedValue>>> resolveSet;
        JSHandle<JSTaggedValue> binding = SourceTextModule::ResolveExport(thread, mm, key, resolveSet);
        ASSERT(binding->IsResolvedBinding());
        JSTaggedValue targetModule = JSHandle<ResolvedBinding>::Cast(binding)->GetModule();
        ASSERT(!targetModule.IsUndefined());
        JSTaggedValue dictionary = SourceTextModule::Cast(targetModule.GetHeapObject())->GetNameDictionary();
        if (dictionary.IsUndefined()) {
            THROW_REFERENCE_ERROR_AND_RETURN(thread, "module environment is undefined", false);
        }
    }
    return true;
}
}  // namespace panda::ecmascript
