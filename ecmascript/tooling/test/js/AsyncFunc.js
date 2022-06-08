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

async function helloAsync(){
    print ("helloAsync log 1");
    print ("helloAsync log 2");
    print ("helloAsync log 3");
    print ("helloAsync log 4");
    return "helloAsync";
}

print("main test 1");
print(helloAsync());
print("main test 2");
print("main test 3");

helloAsync().then(v=>{
   print(v);
   print("helloAsync then end!");
})
print("main test end!");