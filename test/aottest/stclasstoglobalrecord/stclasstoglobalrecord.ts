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

declare function print(arg:any):string;

class Student {
  name : string;
  age : number;

  constructor({name, age}:{ name: string; age:number}) {
    this.name = name;
    this.age = age;
  }

  get getName(): string {
    return this.name;
  }

  set setName(tmp: string) {
    this.name = tmp;
  }

  info() {
      return this.name + "," + this.age;
  }
}

var xiaoMing = new Student({name :"xiaoMing", age : 18});
print(xiaoMing.info());

xiaoMing.setName = "xiaoXiao";
print(xiaoMing.getName);