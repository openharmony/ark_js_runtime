# 方舟JS运行时组件<a name="ZH-CN_TOPIC_0000001183610495"></a>

-   [目录](#section161941989596)
-   [约束](#section119744591305)
-   [编译构建](#section137768191623)
    -   [接口说明](#section175841548124517)
    -   [使用说明](#section129654513264)

-   [相关仓](#section1371113476307)

### 简介<a name="section190813718209"></a>

**方舟JS运行时（ARK JavaScript Runtime）**是OpenHarmony上JS应用使用的运行时。包含JS对象的分配器以及垃圾回收器（GC）、符合ECMAScript规范的标准库、用于运行ARK前端组件生成的方舟字节码（ARK Bytecode，abc）的解释器、用于存储隐藏类的内联缓存、方舟JS运行时对外的函数接口（AFFI）等模块。

**方舟JS运行时组件架构图：**

![](docs/figures/zh-cn_image_0000001196712959.png)

## 目录<a name="section161941989596"></a>

```
/ark/js_runtime
├─ ecmascript             # 方舟JS运行时实现，包括ECMAScript标准库、解释器、内存管理等
│   ├─ base               # 基础帮助类
│   ├─ builtins           # ECMAScript标准库
│   ├─ class_linker       # 字节码预处理模块
│   ├─ compiler           # JS编译器
│   ├─ hprof              # 内存分析工具
│   ├─ ic                 # 内联缓存模块
│   ├─ interpreter        # JS解释器
│   ├─ jobs               # 微任务队列
│   ├─ js_vm              # 命令行工具
│   ├─ mem                # 内存管理模块
│   ├─ napi               # C++接口模块
│   ├─ regexp             # 正则引擎模块
│   ├─ snapshot           # 快照模块
│   ├─ tests              # 单元测试用例
│   ├─ thread             # 线程池
│   ├─ tooling            # JS调试器
│   └─ vmstat             # 运行时profiling工具
└─ test                   # 模块测试用例
```

## 约束<a name="section119744591305"></a>

* 仅支持运行方舟JS前端工具链\(ts2abc\)生成的方舟字节码文件
* 只支持ES2015标准和严格模式（use strict)
* 不支持通过字符串动态创建函数(比如new Function("console.log(1);"))

## 编译构建<a name="section137768191623"></a>

./build.sh --product-name Hi3516DV300 --build-target ark\_js\_runtime

### 接口说明<a name="section175841548124517"></a>

NAPI接口说明参考[NAPI组件](https://gitee.com/openharmony/ace_napi/blob/master/README_zh.md)

### 使用说明<a name="section129654513264"></a>

JS生成字节码参考[工具链使用](docs/工具链使用.md)

字节码执行

cd out/release

LD\_LIBRARY\_PATH=clang\_x64/ark/ark:clang\_x64/global/i18n:../../prebuilts/clang/ohos/linux-x86\_64/llvm/lib/ ./clang\_x64/ark/ark\_js\_runtime/ark\_js\_vm helloworld.abc \_GLOBAL::func\_main\_0

## 相关仓<a name="section1371113476307"></a>

[方舟运行时子系统](docs/方舟运行时子系统.md)

[ark/runtime\_core](https://gitee.com/openharmony/ark_runtime_core/blob/master/README_zh.md)

**[ark/js\_runtime](README_zh.md)**

[ark/ts2abc](https://gitee.com/openharmony/ark_ts2abc/blob/master/README_zh.md)

