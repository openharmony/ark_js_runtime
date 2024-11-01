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

function assertEqual(a, b) {
    var t1 = JSON.stringify(a);
    var t2 = JSON.stringify(b);
    if (t1 == t2) {
        print("PASS");
    } else {
        print("FAIL");
    }
}

var obj1 = {a:2, b:3, c:4};
var obj2 = {d:1, ...obj1, e:5};
assertEqual(obj2, {d:1, a:2, b:3, c:4, e:5});

var obj = {["a" + "b" + "de"]:function() {return 1;}}
assertEqual(obj.abde.name, "abde");