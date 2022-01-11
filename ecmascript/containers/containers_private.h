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

#ifndef ECMASCRIPT_CONTAINERS_CONTAINERS_PRIVATE_H
#define ECMASCRIPT_CONTAINERS_CONTAINERS_PRIVATE_H

#include "ecmascript/base/builtins_base.h"

namespace panda::ecmascript::containers {
enum FuncLength : uint8_t { ZERO = 0, ONE, TWO, THREE, FOUR };
enum ContainerTag : uint8_t {
    ArrayList = 0,
    Queue,
    Deque,
    Stack,
    Vector,
    List,
    LinkedList,
    TreeMap,
    TreeSet,
    HashMap,
    HashSet,
    LightWightMap,
    LightWightSet,
    PlainArray,
    END
};
// Using Lazy-loading container, including ArrayList, Queue, Stack, Vector, List, LinkedList, Deque,
// TreeMap, TreeSet, HashMap, HashSet, LightWightMap, LightWightSet, PlainArray.
// Use through ArkPrivate.Load([ContainerTag]) in js, ContainTag was declaerd in ArkPrivate like ArkPrivate::ArrayList.
class ContainersPrivate : public base::BuiltinsBase {
public:
    static JSTaggedValue Load(EcmaRuntimeCallInfo *msg);

private:
    static JSHandle<JSFunction> NewContainerConstructor(JSThread *thread, const JSHandle<JSObject> &prototype,
                                                        EcmaEntrypoint ctorFunc, const char *name, int length);
    static JSHandle<JSFunction> NewFunction(JSThread *thread, const JSHandle<JSTaggedValue> &key, EcmaEntrypoint func,
                                            int length);
    static void SetFrozenFunction(JSThread *thread, const JSHandle<JSObject> &obj, const char *key, EcmaEntrypoint func,
                                  int length);
    static void SetFrozenConstructor(JSThread *thread, const JSHandle<JSObject> &obj, const char *keyChar,
                                     JSHandle<JSTaggedValue> &value);
    static JSHandle<JSTaggedValue> CreateGetter(JSThread *thread, EcmaEntrypoint func, const char *name,
                                                int length);
    static void SetGetter(JSThread *thread, const JSHandle<JSObject> &obj,
                          const JSHandle<JSTaggedValue> &key, const JSHandle<JSTaggedValue> &getter);
    static void SetFunctionAtSymbol(JSThread *thread, const JSHandle<GlobalEnv> &env,
                                    const JSHandle<JSObject> &obj, const JSHandle<JSTaggedValue> &symbol,
                                    const char *name, EcmaEntrypoint func, int length);
    static void SetStringTagSymbol(JSThread *thread, const JSHandle<GlobalEnv> &env,
                                   const JSHandle<JSObject> &obj, const char *key);
    static JSHandle<JSTaggedValue> InitializeArrayList(JSThread *thread);
};
}  // namespace panda::ecmascript::containers

#endif  // ECMASCRIPT_CONTAINERS_CONTAINERS_PRIVATE_H
