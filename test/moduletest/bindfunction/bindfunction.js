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

const module = {
    x: 42,
    getX: function() {
        return this.x;
    }
};

const unboundGetX = module.getX;
// print(unboundGetX()); // expected output: undefined
  
const boundGetX = unboundGetX.bind(module);
print(boundGetX()); // expected output: 42

function list() {
    return Array.prototype.slice.call(arguments);
}

function addArguments(arg1, arg2) {
    return arg1 + arg2;
}

const list1 = list(1, 2, 3); // [1, 2, 3]
print(list1);

const result1 = addArguments(1, 2); // 3
print(result1);

// Create a function with a preset leading argument
const leadingThirtysevenList = list.bind(null, 37);

// Create a function with a preset first argument.
const addThirtySeven = addArguments.bind(null, 37);

const list2 = leadingThirtysevenList(); // [37]
print(list2);

const list3 = leadingThirtysevenList(1, 2, 3); // [37, 1, 2, 3]
print(list3);

const result2 = addThirtySeven(5); // 37 + 5 = 42
print(result2);

const result3 = addThirtySeven(5, 10); // 37 + 5 = 42, (the second argument is ignored)
print(result3);
  
  