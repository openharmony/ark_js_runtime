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

var fn = function(held) {
    print(held);
  }
  var obj = {name:'ddd'};
  var obj2 = {name:'fff'};
  var fin = new FinalizationRegistry(fn);
  var fin2 = new FinalizationRegistry(fn);
  var fin3 = new FinalizationRegistry(fn);
  
  if(true){
    fin.register(obj, 'hello');
    fin.register(obj2, 'hello123');
    fin2.register(obj, 'hello====222');
    fin2.register(obj2, 'hello====456');
    fin3.register(obj, 'hello=====3333');
    fin3.register(obj2, 'hello=====789');
  }
  obj = undefined;
  // Create variable to wait for variable obj to be GC
  for(var i = 0; i < 500000; ++i) {
    var a = BigInt(123);
  }
