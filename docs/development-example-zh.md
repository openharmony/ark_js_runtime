# 开发实例<a name="ZH-CN_TOPIC_0000001128096218"></a>

- [开发实例<a name="ZH-CN_TOPIC_0000001128096218"></a>](#开发实例)
  - [HelloWorld<a name="section105987593810"></a>](#helloworld)
    - [运行前准备](#运行前准备)
    - [运行hello-world.js](#运行hello-worldjs)
    - [反汇编hello-world.abc](#反汇编hello-worldabc)
  - [运行Test262测试用例<a name="section118471435115815"></a>](#运行test262测试用例)
    - [运行前准备](#运行前准备-1)
    - [运行Test262](#运行test262)
    - [测试运行示例](#测试运行示例)
    - [测试输出](#测试输出)

本章节将介绍基于方舟运行时的开发测试实例。

## HelloWorld<a name="section105987593810"></a>

### 运行前准备

1.  编译方舟运行时，编译命令：

```
./build.sh --product-name Hi3516DV300 --build-target ark_js_host_linux_tools_packages  # arm平台和host端运行工具
```

2.  编译方舟前端，编译命令：

```
./build.sh --product-name Hi3516DV300 --build-target ark_ts2abc_build
```

**说明**：编译命令执行路径为项目根目录。

### 运行hello-world.js

新建hello-world.js文件，写入以下源码：

```
 print("Hello World!!!");
```

运行步骤：

1.  通过方舟前端生成hello-world.abc文件，编译命令：

    ```
    node --expose-gc /your code path/out/hi3516dv300/clang_x64/ark/ark/build/src/index.js hello-world.js
    ```

2.  执行hello-world.abc文件：
    1.  设置搜索路径：

        ```
        export LD_LIBRARY_PATH= out/hi3516dv300/clang_x64/ark/ark:out/hi3516dv300/clang_x64/ark/ark_js_runtime:out/hi3516dv300/clang_x64/global/i18n_standard:prebuilts/clang/ohos/linux-x86_64/llvm/lib
        ```

    2.  执行ark\_js\_vm：

        ```
        /your code path/out/hi3516dv300/clang_x64/ark/ark_js_runtime/ark_js_vm hello-world.abc
        ```

        执行结果如下：

        ```
        Hello World!!!
        ```



**说明**：此处“_your code path_”为源码目录路径。

### 反汇编hello-world.abc

执行如下命令，结果输出到output文件中：

```
./your code path/out/hi3516dv300/clang_x64/ark/ark/ark_disasm hello-world.abc output
```

hello-world.abc反汇编结果如下：

```
#
# source binary: hello-world.abc
#

# ====================
# LITERALS

# ====================
# RECORDS

.record _ESAnnotation <external>

.record _ESModuleMode {
	u8 isModule
}

# ====================
# METHODS

.function any func_main_0_any_any_any_any_(any a0, any a1, any a2) <static> {
	mov.dyn v2, a2
	mov.dyn v1, a1
	mov.dyn v0, a0
	builtin.acc
	sta.dyn v5
	builtin.idi "print", 0x0 // 加载print函数
	sta.dyn v3
	lda.str "Hello World!!!"  // 加载Hello World!!!字符串
	sta.dyn v4
	builtin.tern3 v3, v4  // 调用print函数
	builtin.acc
}
```

## 运行Test262测试用例<a name="section118471435115815"></a>

### 运行前准备

1.  编译方舟运行时，编译命令：

```
./build.sh --product-name Hi3516DV300 --build-target ark_js_host_linux_tools_packages
```

1.  编译方舟前端，编译命令：

```
./build.sh --product-name Hi3516DV300 --build-target ark_ts2abc_build
```

**说明**：编译命令执行路径为项目根目录。

### 运行Test262

运行run\_test262.py脚本，下载及运行Test262用例。

命令行格式：

```
python3 test262/run_test262.py [options]
```

执行路径为：项目根目录/ark/ts2abc。

<a name="table11141827153017"></a>
<table><thead align="left"><tr id="row101462717303"><th class="cellrowborder" valign="top" width="50%" id="mcps1.1.3.1.1"><p id="p51552743010"><a name="p51552743010"></a><a name="p51552743010"></a>选项</p>
</th>
<th class="cellrowborder" valign="top" width="50%" id="mcps1.1.3.1.2"><p id="p11592710304"><a name="p11592710304"></a><a name="p11592710304"></a>描述</p>
</th>
</tr>
</thead>
<tbody><tr id="row2015172763014"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p171592710306"><a name="p171592710306"></a><a name="p171592710306"></a>--h，--help</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p13151527133011"><a name="p13151527133011"></a><a name="p13151527133011"></a>帮助提示</p>
</td>
</tr>
<tr id="row1015527173015"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p1615182712308"><a name="p1615182712308"></a><a name="p1615182712308"></a>--dir  DIR</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p9556101593120"><a name="p9556101593120"></a><a name="p9556101593120"></a>选定要测试的目录</p>
</td>
</tr>
<tr id="row1015112763020"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p1815182733012"><a name="p1815182733012"></a><a name="p1815182733012"></a>--file  FILE</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1615627173019"><a name="p1615627173019"></a><a name="p1615627173019"></a>选定要测试的文件</p>
</td>
</tr>
<tr id="row131515277307"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p111572716304"><a name="p111572716304"></a><a name="p111572716304"></a>--mode  [{1, 2, 3}]</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1655718513105"><a name="p1655718513105"></a><a name="p1655718513105"></a>模式选择，1：仅默认值；2：仅严格模式；3：默认模式和严格模式</p>
</td>
</tr>
<tr id="row1815112753020"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p2151927193015"><a name="p2151927193015"></a><a name="p2151927193015"></a>--es51</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1715312588115"><a name="p1715312588115"></a><a name="p1715312588115"></a>运行Test262 ES5.1版本</p>
</td>
</tr>
<tr id="row1915182703012"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p17151527133017"><a name="p17151527133017"></a><a name="p17151527133017"></a>--es2015  [{all, only}]</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p15761152983113"><a name="p15761152983113"></a><a name="p15761152983113"></a>运行Test262 ES2015版本，all：包含的所有用例；only：仅包括ES2015</p>
</td>
</tr>
<tr id="row10924204611109"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p18924846111013"><a name="p18924846111013"></a><a name="p18924846111013"></a>--esnext</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p15495042191410"><a name="p15495042191410"></a><a name="p15495042191410"></a>运行Test262-ES.next</p>
</td>
</tr>
<tr id="row5161145010105"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p716125071020"><a name="p716125071020"></a><a name="p716125071020"></a>--engine  FILE</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p121612050181014"><a name="p121612050181014"></a><a name="p121612050181014"></a>运行测试的其他引擎，指定二进制文件（如:d8,hermes,jsc,qjs...）</p>
</td>
</tr>
<tr id="row1325585931120"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p112561595112"><a name="p112561595112"></a><a name="p112561595112"></a>--babel</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p32561959111112"><a name="p32561959111112"></a><a name="p32561959111112"></a>是否使用Babel转换</p>
</td>
</tr>
<tr id="row95230818126"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p12523158191210"><a name="p12523158191210"></a><a name="p12523158191210"></a>--timeout  TIMEOUT</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p65233871210"><a name="p65233871210"></a><a name="p65233871210"></a>设置测试超时时间（以毫秒为单位）</p>
</td>
</tr>
<tr id="row474911612120"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p1274912166123"><a name="p1274912166123"></a><a name="p1274912166123"></a>--threads  THREADS</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p4749121631210"><a name="p4749121631210"></a><a name="p4749121631210"></a>设置并行运行线程数</p>
</td>
</tr>
<tr id="row561512363122"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p26152036191218"><a name="p26152036191218"></a><a name="p26152036191218"></a>--hostArgs  HOSTARGS</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p156151636161215"><a name="p156151636161215"></a><a name="p156151636161215"></a>传递给eshost主机的命令行参数</p>
</td>
</tr>
<tr id="row77091648111210"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p18709164871213"><a name="p18709164871213"></a><a name="p18709164871213"></a>--ark-tool  ARK_TOOL</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p16709194812126"><a name="p16709194812126"></a><a name="p16709194812126"></a>方舟运行时的二进制工具</p>
</td>
</tr>
<tr id="row3767145231210"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p3767155201216"><a name="p3767155201216"></a><a name="p3767155201216"></a>--ark-frontend-tool  ARK_FRONTEND_TOOL</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p4767195251220"><a name="p4767195251220"></a><a name="p4767195251220"></a>方舟前端转换工具</p>
</td>
</tr>
<tr id="row753817001311"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p553870111318"><a name="p553870111318"></a><a name="p553870111318"></a>--libs-dir  LIBS_DIR</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p35384041313"><a name="p35384041313"></a><a name="p35384041313"></a>依赖so的路径集合，通过“:”分割</p>
</td>
</tr>
<tr id="row08504716135"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p11851747161314"><a name="p11851747161314"></a><a name="p11851747161314"></a>--ark-frontend  [{ts2panda, es2panda}]</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1085144712137"><a name="p1085144712137"></a><a name="p1085144712137"></a>指定前端</p>
</td>
</tr>
</tbody>
</table>

### 测试运行示例

-   运行ES51测试用例：

    ```
     python3 test262/run_test262.py --es51
    ```

-   仅运行ES2015测试用：

    ```
     python3 test262/run_test262.py --es2015 only
    ```

-   运行ES2015和ES51所有测试用例：

    ```
     python3 test262/run_test262.py --es2015 all
    ```

-   运行单一测试用例：

    ```
     python3 test262/run_test262.py --file test262/data/test_es5/language/statements/break/12.8-1.js
    ```

-   运行某目录下所有测试用例：

    ```
     python3 test262/run_test262.py --dir test262/data/test_es5/language/statements
    ```


-   使用\`babel\`把单个测试用例转换成es5后再运行：

    ```
     python3 test262/run_test262.py  --babel --file test262/data/test_es5/language/statements/break/12.8-1.js
    ```


### 测试输出

Test262所有用例的测试结果位于项目根目录/ark/ts2abc/out下。shell中测试输出结果如下：

```
$python3 test262/run_test262.py --file test262/data/test_es2015/built-ins/Array/15.4.5.1-5-1.js

Wait a moment..........
Test command:
node
        test262/harness/bin/run.js
        --hostType=panda
        --hostPath=python3
        --hostArgs='-B test262/run_sunspider.py --ark-tool=/your code path/out/hi3516dv300/clang_x64/ark/ark_js_runtime/ark_js_vm --ark-frontend-tool=/your code path/out/hi3516dv300/clang_x64/ark/ark/build/src/index.js --libs-dir=/your code path/out/hi3516dv300/clang_x64/ark/ark:/your code path/out/hi3516dv300/clang_x64/global/i18n:/your code path/prebuilts/clang/ohos/linux-x86_64/llvm/lib/ --ark-frontend=ts2panda'
        --threads=15
        --mode=only strict mode
        --timeout=60000
        --tempDir=build/test262
        --test262Dir=test262/data
        --saveCompiledTests
        test262/data/test_es5/language/statements/break/12.8-1.js

PASS test262/data/test_es2015/built-ins/Array/15.4.5.1-5-1.js (strict mode)
Ran 1 tests
1 passed
0 failed
used time is: 0:01:04.439642
```

