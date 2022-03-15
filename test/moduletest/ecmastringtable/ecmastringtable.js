/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

// Using to test the concat interface of EcmaStringTable about intern string and non-intern string
let REGISTRY = Symbol();
function System() {
  this[REGISTRY] = {};
}
function getOrCreateLoad(loader, id) {
  let load = loader[REGISTRY][id];
  print("id: " + id + " - load: "+ load);
  load = loader[REGISTRY][id] = {};
  return load;
}

// non-intern + non-intern
let head1 = "no-".concat("schema:");
let tail1 = "/src/".concat("xxx-js/instantiated-1af0bf5b.js");
let key1 = head1 + tail1;
let key2 = head1 + tail1;

// intern + intern
let key3 = "no-schema:/src/xxx-js/instantiation.js";
let key4 = "no-schema:" + "/src/xxx-js/instantiation.js";

// non-intern + intern
let head2 = "no-".concat("schema:");
let tail2 = "/src/xxx-js/cc.js";
let key5 = "no-schema:" + "/src/xxx-js/cc.js";
let key6 = head2 + tail2;

// intern + non-intern
let head3 = "no-schema:";
let tail3 = "/src".concat("/instantiated-1af0bf5b.js");
let key7 = "no-schema:" + "/src/instantiated-1af0bf5b.js";
let key8 = head3 + tail3;

let keyArray = [key1, key3, key5, key7];
let system = new System();
for (let i = 0; i < keyArray.length; i++) {
    getOrCreateLoad(system, keyArray[i]);
}

print("key1 === key2: ", key1 === key2);
print("key3 === key4: ", key3 === key4);
print("key5 === key6: ", key5 === key6);
print("key7 === key8: ", key7 === key8);
getOrCreateLoad(system, key2);
getOrCreateLoad(system, key4);
getOrCreateLoad(system, key6);
getOrCreateLoad(system, key8);
