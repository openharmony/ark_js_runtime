# 概述<a name="ZH-CN_TOPIC_0000001174295771"></a>

方舟\(ARK\)是华为自研的统一编程平台，包含编译器、工具链、运行时等关键部件，支持高级语言在多种芯片平台的编译与运行，并支撑OpenHarmony标准操作系统及其应用和服务运行在手机、个人电脑、平板、电视、汽车和智能穿戴等多种设备上的需求。本次开源的ARK-JS提供的能力是在OpenHarmony操作系统中编译和运行JavaScript语言\(本文后面简称JS\)。

本次开源的ARK-JS分成两个部分，分别是JS编译工具链与JS运行时。JS工具链将JS源码编译成方舟字节码\(ARK Bytecode\)，JS运行时负责执行生成的方舟字节码\(后续如无特殊说明，字节码特指方舟字节码\)。

图 1 JS编译工具链架构
![](figures/zh-cn_image_0000001197967897.png)

ARK-JS的源码编译器接收JS源码的输入，再由ts2abc（将JavaScript文件转换为字节码的工具）转化为abc文件。

图 2 JS运行时（Runtime）架构

![](figures/zh-cn_image_0000001196789343.png)

ARK-JS Runtime以方舟字节码文件作为输入并直接运行字节码文件，实现对应的JS语义逻辑。

ARK-JS Runtime主要由四个部分组成：

-   Core Runtime

    Core Runtime主要由语言无关的基础运行库组成，包括承载字节码的ARK File组件、支持Debugger的Tooling组件、负责对应系统调用的ARK Base组件等。

-   Execution Engine

    执行引擎目前包含执行字节码的解释器、缓存隐藏类和内联缓存、以及剖析记录运行时类型的Profiler。

-   ECMAScript Runtime

    ECMAScript Runtime则包含了各种JS对象的分配器、垃圾回收器、以及用以支撑ECMAScript规范的内部运行库。

-   AFFI \(ARK Foreign Function Interface\)

    AFFI是ARK-JS运行时的C++语言外部函数接口。


未来规划：

-   高性能TypeScript支持

OpenHarmony目前选用了TS（TypeScript）作为主要开发语言之一，而TS简单地概括就是具有静态类型的JS。业界通用的执行方式是把TS转化为JS，再使用JS运行时来执行生成的JS代码。

ARK-JS规划原生支持TS。在ts2abc编译TS源码时，会推导分析TS的类型信息并传递给ARK-JS运行时，运行时直接利用类型信息静态生成内联缓存从而加速字节码执行。

TS AOT \(Ahead of Time\) Compiler，利用ts2abc传递的类型信息，直接编译生成高质量的机器码，使得应用可以直接以机器码形式运行，提升运行性能。

-   轻量级Actor并发模型

ECMAScript没有提供并发规范，业界JS引擎的实现中常用Actor并发模型。此模型下执行体之间不共享任何数据，通过消息机制进行通信。业界当前实现的JS Actor模型（web-worker）有启动速度慢、内存占用高这些缺陷。为了利用设备的多核能力获得更好的性能提升，ARK-JS需要提供启动快、内存占用少的Actor实现。

ARK-JS规划在Actor内存隔离模型的基础上，共享actor实例中的不可变或者不易变的对象、内建代码块、方法字节码等，来提升JS Actor的启动性能和节省内存开销，达到实现轻量级Actor并发模型的目标。

