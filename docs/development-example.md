# Development Example<a name="EN-US_TOPIC_0000001128096218"></a>

-   [HelloWorld](#section105987593810)
-   [Performing Test Case Test262](#section118471435115815)

This section describes how to develop and test ARK runtime.

## HelloWorld<a name="section105987593810"></a>

### Preparations

1.  Run the following command to compile ARK runtime:

	```
	./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages  # arm platform and host side running tool
	```

2.  Run the following command to compile the ARK frontend:

    x64：
	```
    ./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages --build-target ark_ts2abc_build  # arm平台和host端运行工具
    ```

	arm64：
	```
	./build.sh --product-name ohos_arm64 --build-target ark_js_vm --build-target ld-musl-aarch64.so.1
	```

	arm32:
	```
	./build.sh --product-name rk3568 --build-target libarkruntime --build-target ark_js_runtime --build-target ld-musl-arm.so.1
	```

**NOTE**:  Run the compilation commands in the project root directory.


### Running  **hello-world.js**

Create the  **hello-world.js**  file and write the following source code into the file:

```
 print("Hello World!!!");
```

Run the  **hello-world.js**  file.

1.  Use the ARK frontend to create the  **hello-world.abc**  file.

    ```
    node --expose-gc /your code path/out/rk3568/clang_x64/ark/ark/build/src/index.js hello-world.js
    ```

2.  Run the  **hello-world.abc**  file.
    1.  Set the search path.

        ```
        export LD_LIBRARY_PATH= out/rk3568/clang_x64/ark/ark:out/rk3568/clang_x64/ark/ark_js_runtime:out/rk3568/clang_x64/global/i18n_standard:prebuilts/clang/ohos/linux-x86_64/llvm/lib
        ```

    2.  Run  **ark\_js\_vm**.

        ```
        /your code path/out/rk3568/clang_x64/ark/ark_js_runtime/ark_js_vm hello-world.abc
        ```

        The execution result is as follows:

        ```
        Hello World!!!
        ```

**NOTE**:  In the preceding command,  _your code path_  indicates the source code directory.

### Disassembling  **hello-world.abc**

Run the following command to export the result to the  **output**  file:

```
./your code path/out/rk3568/clang_x64/ark/ark/ark_disasm hello-world.abc output
```

The output is as follows:

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
	builtin.idi "print", 0x0 // Load the print function.
	sta.dyn v3
	lda.str "Hello World!!!"  // Load the Hello World!!! string.
	sta.dyn v4
	builtin.tern3 v3, v4  // Call the print function.
	builtin.acc
}
```

## Performing Test Case Test262<a name="section118471435115815"></a>

### Preparations

1.  Run the following command to compile ARK runtime:

```
./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages
```

2.  Run the following command to compile the ARK frontend:

```
./build.sh --product-name rk3568 --build-target ark_ts2abc_build
```

**NOTE**:  Run the compilation commands in the project root directory.

### Running Test262

Run the  **run\_test262.py**  script to download and run the Test262 test case.

Command:

```
python3 test262/run_test262.py [options]
```

Run the script in  _Project root directory_**/ark/ts2abc**.

<a name="table11141827153017"></a>
<table><thead align="left"><tr id="row101462717303"><th class="cellrowborder" valign="top" width="50%" id="mcps1.1.3.1.1"><p id="p51552743010"><a name="p51552743010"></a><a name="p51552743010"></a>Option</p>
</th>
<th class="cellrowborder" valign="top" width="50%" id="mcps1.1.3.1.2"><p id="p11592710304"><a name="p11592710304"></a><a name="p11592710304"></a>Description</p>
</th>
</tr>
</thead>
<tbody><tr id="row2015172763014"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p171592710306"><a name="p171592710306"></a><a name="p171592710306"></a>--h, --help</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p13151527133011"><a name="p13151527133011"></a><a name="p13151527133011"></a>Displays help information.</p>
</td>
</tr>
<tr id="row1015527173015"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p1615182712308"><a name="p1615182712308"></a><a name="p1615182712308"></a>--dir  DIR</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p9556101593120"><a name="p9556101593120"></a><a name="p9556101593120"></a>Specifies the directory to test.</p>
</td>
</tr>
<tr id="row1015112763020"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p1815182733012"><a name="p1815182733012"></a><a name="p1815182733012"></a>--file  FILE</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1615627173019"><a name="p1615627173019"></a><a name="p1615627173019"></a>Specifies the file to test.</p>
</td>
</tr>
<tr id="row131515277307"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p111572716304"><a name="p111572716304"></a><a name="p111572716304"></a>--mode  [{1, 2, 3}]</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1820821404711"><a name="p1820821404711"></a><a name="p1820821404711"></a>Specifies the mode, which can be any of the following:</p>
<a name="ul136633170477"></a><a name="ul136633170477"></a><ul id="ul136633170477"><li><strong id="b12807202010471"><a name="b12807202010471"></a><a name="b12807202010471"></a>1</strong>: default</li><li><strong id="b16343325154719"><a name="b16343325154719"></a><a name="b16343325154719"></a>2</strong>: strict mode only</li><li><strong id="b19742163854610"><a name="b19742163854610"></a><a name="b19742163854610"></a>3</strong>: default and strict modes</li></ul>
</td>
</tr>
<tr id="row1815112753020"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p2151927193015"><a name="p2151927193015"></a><a name="p2151927193015"></a>--es51</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1715312588115"><a name="p1715312588115"></a><a name="p1715312588115"></a>Runs Test262 ES5.1.</p>
</td>
</tr>
<tr id="row1915182703012"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p17151527133017"><a name="p17151527133017"></a><a name="p17151527133017"></a>--es2015  [{all, only}]</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p536992675017"><a name="p536992675017"></a><a name="p536992675017"></a>Runs Test262 ES2015.</p>
<p id="p205288299503"><a name="p205288299503"></a><a name="p205288299503"></a><strong id="b2052812914503"><a name="b2052812914503"></a><a name="b2052812914503"></a>all</strong>: all cases</p>
<p id="p1392723585014"><a name="p1392723585014"></a><a name="p1392723585014"></a><strong id="b15128193544910"><a name="b15128193544910"></a><a name="b15128193544910"></a>only</strong>: only ES2015</p>
</td>
</tr>
<tr id="row10924204611109"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p18924846111013"><a name="p18924846111013"></a><a name="p18924846111013"></a>--esnext</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p15495042191410"><a name="p15495042191410"></a><a name="p15495042191410"></a>Runs <strong id="b13144135817502"><a name="b13144135817502"></a><a name="b13144135817502"></a>Test262-ES.next</strong>.</p>
</td>
</tr>
<tr id="row5161145010105"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p716125071020"><a name="p716125071020"></a><a name="p716125071020"></a>--engine  FILE</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p121612050181014"><a name="p121612050181014"></a><a name="p121612050181014"></a>Runs other engines and specifies binary files (such as d8, hermes, jsc, and qjs).</p>
</td>
</tr>
<tr id="row1325585931120"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p112561595112"><a name="p112561595112"></a><a name="p112561595112"></a>--babel</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p32561959111112"><a name="p32561959111112"></a><a name="p32561959111112"></a>Specifies whether to use Babel to convert code.</p>
</td>
</tr>
<tr id="row95230818126"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p12523158191210"><a name="p12523158191210"></a><a name="p12523158191210"></a>--timeout  TIMEOUT</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p65233871210"><a name="p65233871210"></a><a name="p65233871210"></a>Specifies the test timeout period in ms.</p>
</td>
</tr>
<tr id="row474911612120"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p1274912166123"><a name="p1274912166123"></a><a name="p1274912166123"></a>--threads  THREADS</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p4749121631210"><a name="p4749121631210"></a><a name="p4749121631210"></a>Specifies the number of concurrent threads.</p>
</td>
</tr>
<tr id="row561512363122"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p26152036191218"><a name="p26152036191218"></a><a name="p26152036191218"></a>--hostArgs  HOSTARGS</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p156151636161215"><a name="p156151636161215"></a><a name="p156151636161215"></a>Specifies the command line parameters sent to the eshost.</p>
</td>
</tr>
<tr id="row77091648111210"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p18709164871213"><a name="p18709164871213"></a><a name="p18709164871213"></a>--ark-tool  ARK_TOOL</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p16709194812126"><a name="p16709194812126"></a><a name="p16709194812126"></a>Specifies the binary tool of ARK runtime.</p>
</td>
</tr>
<tr id="row3767145231210"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p3767155201216"><a name="p3767155201216"></a><a name="p3767155201216"></a>--ark-frontend-tool  ARK_FRONTEND_TOOL</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p4767195251220"><a name="p4767195251220"></a><a name="p4767195251220"></a>Specifies the ARK front-end conversion tool.</p>
</td>
</tr>
<tr id="row753817001311"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p553870111318"><a name="p553870111318"></a><a name="p553870111318"></a>--libs-dir  LIBS_DIR</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p35384041313"><a name="p35384041313"></a><a name="p35384041313"></a>Specifies the set of .so dependency file paths, separated by colons (:).</p>
</td>
</tr>
<tr id="row08504716135"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p11851747161314"><a name="p11851747161314"></a><a name="p11851747161314"></a>--ark-frontend  [{ts2panda, es2panda}]</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1085144712137"><a name="p1085144712137"></a><a name="p1085144712137"></a>Specifies the frontend.</p>
</td>
</tr>
</tbody>
</table>

### Example

-   Run test case ES51.

    ```
     python3 test262/run_test262.py --es51
    ```

-   Run test case ES2015 only.

    ```
     python3 test262/run_test262.py --es2015 only
    ```

-   Run all ES2015 and ES51 test cases.

    ```
     python3 test262/run_test262.py --es2015 all
    ```

-   Run a test case.

    ```
     python3 test262/run_test262.py --file test262/data/test_es5/language/statements/break/12.8-1.js
    ```

-   Run all test cases in a directory.

    ```
     python3 test262/run_test262.py --dir test262/data/test_es5/language/statements
    ```


-   Use Babel to convert a test case into ES5 and then run the test case.

    ```
     python3 test262/run_test262.py  --babel --file test262/data/test_es5/language/statements/break/12.8-1.js
    ```


### Test Output

The results of all Test262 test cases are available in the  **_Project root directory_/ark/ts2abc/out**. The test result in the shell is as follows:

```
$python3 test262/run_test262.py --file test262/data/test_es2015/built-ins/Array/15.4.5.1-5-1.js

Wait a moment..........
Test command:
node
        test262/harness/bin/run.js
        --hostType=panda
        --hostPath=python3
        --hostArgs='-B test262/run_sunspider.py --ark-tool=/your code path/out/rk3568/clang_x64/ark/ark_js_runtime/ark_js_vm --ark-frontend-tool=/your code path/out/rk3568/clang_x64/ark/ark/build/src/index.js --libs-dir=/your code path/out/rk3568/clang_x64/ark/ark:/your code path/out/rk3568/clang_x64/global/i18n:/your code path/prebuilts/clang/ohos/linux-x86_64/llvm/lib/ --ark-frontend=ts2panda'
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

