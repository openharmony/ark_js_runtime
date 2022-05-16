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
function Test262Error(message) {
    this.message = message;
}

RegExp.prototype.exec = function() {
    throw new Test262Error();
};

let iter = /./[Symbol.matchAll]('abc');

try{
    let res = iter.next();
    print(res.value());
} catch(error) {
    print("call exec is throw error")
}