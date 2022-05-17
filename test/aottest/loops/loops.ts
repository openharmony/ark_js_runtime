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

declare function print(n:number):string;

for (var i:number = 0; i < 10; i++) {
    for (var j:number = 0; j < 10; j++) {
      if (i == j) {
        print(i);
      }
    }
}

for (var i:number = 0; ; i++) {
    if (i > 100) {
        print(i);
        break;
    }
}

for (var i:number = 0; i < 100; i++) {
    if (i < 99) {
        continue;
    }
    print(i);
}

var i:number = 0;
var sum:number = 0;
while (i < 10) {
    i++;
    sum += i;
}
print(sum);

var j:number = 0;
while (j < 100) {
    j++;
    if (j == 50) {
        print(j);
        break;
    }
}

var k:number = 0;
while (k < 100) {
    k++;
    if (k < 99) {
        k++;
        continue;
    }
    print(k);
}

i = 0;
sum = 0;
do {
    sum += i;
    i++;
} while (i < 100);
print(sum);

j = 0;
do {
    j++;
    if (j > 100) {
        print(j);
        break;
    }
} while (true);

k  = 0;
do {
    k++;
    if (k < 100) {
        continue;
    }
    print(k);
} while (k < 100);

var index:number = 0;
i = 0;
if (index == 0) {
    for (i = 0; ; i++) {
        if (i > 100) {
            print(i);
            break;
        }
    }
} else if (index == 1) {
    while (i < 100) {
        i++;
        if (i < 99) {
            i++;
            continue;
        }
        print(i);
    }
} else {
    do {
        i++;
        if (i > 100) {
            print(i);
            break;
        }
    } while (true);
}

index = 1;
i = 0;
if (index == 0) {
    for (i = 0; ; i++) {
        if (i > 100) {
            print(i);
            break;
        }
    }
} else if (index == 1) {
    while (i < 100) {
        i++;
        if (i < 99) {
            i++;
            continue;
        }
        print(i);
    }
} else {
    do {
        i++;
        if (i > 100) {
            print(i);
            break;
        }
    } while (true);
}

index = 2;
i = 0;
if (index == 0) {
    for (i = 0; ; i++) {
        if (i > 100) {
            print(i);
            break;
        }
    }
} else if (index == 1) {
    while (i < 100) {
        i++;
        if (i < 99) {
            i++;
            continue;
        }
        print(i);
    }
} else {
    do {
        i++;
        if (i > 100) {
            print(i);
            break;
        }
    } while (true);
}
