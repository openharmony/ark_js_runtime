declare function print(str:any):string;



function sum(a:number, b:number):number {
      return a + b;
    }

const handler = {
 apply: function(target:any, thisArg:any, argumentsList:any[]) {
       print(`Calculate sum: ${argumentsList}`);
       // expected output: "Calculate sum: 1,2"

       return target(argumentsList[0], argumentsList[1]) * 10;
   }
};

const proxy1 = new Proxy(sum, handler);

print(sum(1, 2));
// expected output: 3
print(proxy1(1, 2));
