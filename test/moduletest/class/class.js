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

class Parent {
    constructor(x) {
        this.x = x;
    }

    static toString() {
        return 'parent';
    }
}

class Child extends Parent {
    constructor(x, y) {
        super(x);
        this.y = y;
    }

    value() {
        return this.x * this.y;
    }

    static toString() {
        return super.toString() + ' child';
    }
}

var c = new Child(2, 3);
print(c.value());
print(Child.toString());

try {
    class C {
        a = 1;
    }
    class D extends C {
        constructo() {
            delete super.a;
        }
    }
    d = new D();
} catch (err) {
    print("PASS");
}

class A {
    a = 10;
}
class B extends A {
    constructor() {
        let a = "a";
        super[a]  = 1;
    }
}
var par = new A;
print(par.a);