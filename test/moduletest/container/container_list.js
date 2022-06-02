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

var List = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    List = ArkPrivate.Load(ArkPrivate.List);
    let list = new List();
    const testArray = []
    for(let i = 0; i < 10; i++) {
        list.add(i)
        testArray.push(i)
    }

    print("test list get 1:", list.get(1) === 1)
    print("test list has:",  list.has(8))
    print("test list not has:", list.has(123) === false)

    let list1 = new List();
    const testArray2 = []
    for(let i = 0; i < 10; i++) {
        list1.add(i)
        testArray2.push(i)
    }
    
    print("test list equal:", list.equal(list1))
    list.add(10)
    testArray.push(10)
    print("test list equal:", list.equal(list1) === false)
    print("test list getLastIndexOf:", list.getLastIndexOf(1) === 1)
    print("test list getIndexOf:", list.getIndexOf(5) === 5)

    list.removeByIndex(10)
    testArray.splice(10, 1)
    let res = true
    for(let i = 0; i < testArray.length; i++) {
        if (list[i] !== testArray[i]) {
            res = false
        }
    }
    print("test list removeByIndex:", res)

    list.remove(9)
    testArray.splice(9, 1)
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (list[i] !== testArray[i]) {
            res = false
        }
        testArray[i] = testArray[i] * 2
    }
    print("test list remove:", res)

    list.replaceAllElements((item, index) => {
        return item * 2
    })
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (list[i] !== testArray[i]) {
            res = false
        }
    }
    print("test list replaceAllElements:", res)
    print("test list getFirst:", list.getFirst() === 0)
    print("test list getLast:", list.getLast() === 16)
    list.insert(999, 3)
    testArray.splice(3, 0, 999)
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (list[i] !== testArray[i]) {
            res = false
        }
    }
    print("test list insert:", res)
    
    list.set(5, 888)
    testArray[5] = 888
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (list[i] !== testArray[i]) {
            res = false
        }
    }
    print("test list set:", res)

    let list2 = new List();
    list2.add(4);
    list2.add(3);
    list2.add(1);
    list2.add(2);
    list2.add(0);
    list2.sort((a,b) => a-b);
    res = true
    for (let i = 0; i < 5; i++) {
        if (list2[i] !== i) {
            res = false
        }
    }
    print("test list sort:", res)

    res = true
    let subList = list.getSubList(1, 3)
    const newtestArray = testArray.slice(1, 3)
    for(let i = 0; i < subList.length; i++) {
        if (newtestArray[i] !== subList[i]) {
            res =  false
        }
    }
    print("test list getSubList:", res)

    res = true
    const arr = list.convertToArray()
    for (let i = 0; i < arr.length; i++) {
        if (arr[i] !== testArray[i]) {
            res = false
        }
    }
    print("test list convertToArray:", res)

    res = true
    let i = 0
    for (const data of list) {
        if (data !== testArray[i]) {
            res = false
        }
        i++;
    }
    print("test list itertor:", res)

    res = true
    list1.forEach((i, d) => {
        if (d !== testArray2[i]) {
            res = false
        }
    })
    print("test list forEach:", res)
    list2.clear()
    print("test list clear:", list2.length === 0)
    print("test list get:", list1.get(200) === undefined)
    print("test list getLastIndexOf:", list1.getLastIndexOf('abc') === -1)
    try {
        list1.removeByIndex(99)
    } catch (error) {
        print("test list removeByIndex:", 'There is no such element to delete')
    }
    res = list1.remove(888)
    print("test list remove:", res)
}

