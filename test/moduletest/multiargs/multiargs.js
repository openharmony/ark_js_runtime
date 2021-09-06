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

function zero()
{
    var a = '   0\n'
    var b = '  000\n'
    var c = ' 00000\n'
    var d = '0000000'
    print(a+b+c+d)
}

function one(x)
{
    print(x)
}

function two(x,y)
{
    print(x+y)
}

function three(x,y,z)
{
    print(x+y+z)
}

function four(x,y,z,t)
{
    print(x+y+z+t)
}

function five(x,y,z,t,a)
{
    let s = x + 10*y+ 100*z + 1000*t + 10000*a
    print(s.toString(10))
}

zero()
one(123456789)
two('hello,',' world')
three('aaa','bbb','ccc')

let x = 111
let y = 222
let z = 333
let a = 666

four(x.toString(10),y.toString(10),z.toString(10),a.toString(10))
five(1,2,3,4,5)