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
declare function print(str:any):number;

function func1(a:any) {
  print(arguments[0]);
}
func1(1);

function func2(a:any, b:any, c:any) {
  print(arguments[0]);
  print(arguments[1]);
  print(arguments[2]);
}
func2(1, "abc", 3.14);

function func3() {
  print(arguments[0]);
  print(arguments[1]);
}
func3();