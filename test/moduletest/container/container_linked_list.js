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

var LinkedList = undefined;
if (globalThis["ArkPrivate"] != undefined) {
    LinkedList = ArkPrivate.Load(ArkPrivate.LinkedList);
    let list = new LinkedList();
    let testArray = []
    for(let i = 0; i<10; i++) {
        list.add(i)
        testArray.push(i)
    }
    print("test linkedlist has:",  list.has(8))
    print("test linkedlist not has:", list.has(2))
    print("test linkedlist getLastIndexOf:", list.getLastIndexOf(1) === 1)
    print("test linkedlist getIndexOf:", list.getIndexOf(5) === 5)
    
    list.removeByIndex(9)

    testArray.splice(9, 1)
    let res = true
    for(let i = 0; i < testArray.length; i++) {
        if (list[i] !== testArray[i]) {
            res = false
        }
    }
    print("test linkedlist removeByIndex:", res)

    const removeRes = list.remove(8)
    testArray.splice(8, 1)
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (list[i] !== testArray[i]) {
            res = false
        }
    }
    print("test linkedlist remove:", res)
    print("test linkedlist remove1:", removeRes)
    print("test linkedlist getFirst:", list.getFirst() === 0)
    print("test linkedlist getLast:", list.getLast() === 7)

    list.insert(3, 999)
    testArray.splice(3, 0, 999)
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (list[i] !== testArray[i]) {
            res = false
        }
    }
    print("test linkedlist insert:", res)

    list.set(5, 888)
    testArray[5] = 888
    res = true
    for(let i = 0; i < testArray.length; i++) {
        if (list[i] !== testArray[i]) {
            res = false
        }
    }
    print("test linkedlist set:", res)

    print("test linkedlist clone:", res)

    list.addFirst(1111)    
    print("test linkedlist addfirst:", list.getFirst() === 1111)

    const removefirstres = list.removeFirst()
    print("test linkedlist removeFirst:", removefirstres === 1111)

    res = true
    let i = 0
    for (const data of list) {
        if (data !== testArray[i]) {
            res = false
        }
        i++;
    }
    print("test linkedlist intertor:", res)

    let list1 = new LinkedList();
    let testArray1 = []
    for (let i = 0; i < 10; i++) {
        list1.add(i)
        testArray1.push(i)
    }

    res = true
    list1.forEach((i, d) => {
        if (d !== testArray1[i]) {
            res = false
        }
    })

    print("test linkedlist forEach:", res)
    list1.clear()
    print("test linkedlist clear:", list1.length === 0)
    print("test linkedlist get:", list.get(1232) === undefined)
    print("test linkedlist getLastIndexOf:", list.getLastIndexOf('abc') === -1)
    try {
        list.removeByIndex(99)
    } catch (error) {
        print("test linkedlist removeByIndex:", 'There is no such element to delete')
    }

    testArray.splice(5, 1)
    const resRemove = list.remove(888)
    print("test linkedlist remove:", resRemove)

    res = true
    const arr = list.convertToArray()
    for (let i = 1; i < arr.length; i++) {
        if (arr[i] !== testArray[i]) {
            res = false
        }
    }
    print("test linkedlist convertToArray:", res)
}

