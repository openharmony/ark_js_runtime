# 工具链使用<a name="ZH-CN_TOPIC_0000001128256014"></a>

方舟前端工具采用命令行交互方式，支持将JS代码转换为方舟字节码，使其能够在方舟运行时上运行。支持Windows/Linux/Mac平台。

## JS字节码编译工具概述

使用前端工具将JS文件转换为方舟字节码文件。方舟前端工具在linux平台上可通过全量编译或指定编译前端工具链获取。

构建编译：

```
$ ./build.sh --product-name hispark_taurus_standard --build-target ark_ts2abc_build
```

安装`node`和`npm`后, 使用前端工具：

```
$ cd out/hispark_taurus/clang_x64/ark/ark/build
$ npm install
$ node --expose-gc src/index.js [选项] file.js
```

<a name="table18706114344420"></a>
<table><thead align="left"><tr id="row19706343164411"><th class="cellrowborder" valign="top" width="15.29152915291529%" id="mcps1.1.6.1.1"><p id="p1970694310447"><a name="p1970694310447"></a><a name="p1970694310447"></a>选项</p>
</th>
<th class="cellrowborder" valign="top" width="6.830683068306829%" id="mcps1.1.6.1.2"><p id="p9548142314456"><a name="p9548142314456"></a><a name="p9548142314456"></a>缩写</p>
</th>
<th class="cellrowborder" valign="top" width="44.34443444344434%" id="mcps1.1.6.1.3"><p id="p170614318449"><a name="p170614318449"></a><a name="p170614318449"></a>描述</p>
</th>
<th class="cellrowborder" valign="top" width="26.01260126012601%" id="mcps1.1.6.1.4"><p id="p1841257144811"><a name="p1841257144811"></a><a name="p1841257144811"></a>取值范围</p>
</th>
<th class="cellrowborder" valign="top" width="7.520752075207521%" id="mcps1.1.6.1.5"><p id="p15894191313495"><a name="p15894191313495"></a><a name="p15894191313495"></a>默认值</p>
</th>
</tr>
</thead>
<tbody><tr id="row770684312444"><td class="cellrowborder" valign="top" width="15.29152915291529%" headers="mcps1.1.6.1.1 "><p id="p0706154312447"><a name="p0706154312447"></a><a name="p0706154312447"></a>--modules</p>
</td>
<td class="cellrowborder" valign="top" width="6.830683068306829%" headers="mcps1.1.6.1.2 "><p id="p1654810236457"><a name="p1654810236457"></a><a name="p1654810236457"></a>-m</p>
</td>
<td class="cellrowborder" valign="top" width="44.34443444344434%" headers="mcps1.1.6.1.3 "><p id="p12451427144913"><a name="p12451427144913"></a><a name="p12451427144913"></a>按照Module方式编译</p>
</td>
<td class="cellrowborder" valign="top" width="26.01260126012601%" headers="mcps1.1.6.1.4 "><p id="p6415710488"><a name="p6415710488"></a><a name="p6415710488"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="7.520752075207521%" headers="mcps1.1.6.1.5 "><p id="p389411314490"><a name="p389411314490"></a><a name="p389411314490"></a>-</p>
</td>
</tr>
<tr id="row8707114315446"><td class="cellrowborder" valign="top" width="15.29152915291529%" headers="mcps1.1.6.1.1 "><p id="p7707134344416"><a name="p7707134344416"></a><a name="p7707134344416"></a>--debug-log</p>
</td>
<td class="cellrowborder" valign="top" width="6.830683068306829%" headers="mcps1.1.6.1.2 "><p id="p15481123194512"><a name="p15481123194512"></a><a name="p15481123194512"></a>-l</p>
</td>
<td class="cellrowborder" valign="top" width="44.34443444344434%" headers="mcps1.1.6.1.3 "><p id="p1988112617501"><a name="p1988112617501"></a><a name="p1988112617501"></a>使能log信息</p>
</td>
<td class="cellrowborder" valign="top" width="26.01260126012601%" headers="mcps1.1.6.1.4 "><p id="p34135724819"><a name="p34135724819"></a><a name="p34135724819"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="7.520752075207521%" headers="mcps1.1.6.1.5 "><p id="p16894813204919"><a name="p16894813204919"></a><a name="p16894813204919"></a>-</p>
</td>
</tr>
<tr id="row1770734394411"><td class="cellrowborder" valign="top" width="15.29152915291529%" headers="mcps1.1.6.1.1 "><p id="p11549171604510"><a name="p11549171604510"></a><a name="p11549171604510"></a>--dump-assembly</p>
</td>
<td class="cellrowborder" valign="top" width="6.830683068306829%" headers="mcps1.1.6.1.2 "><p id="p1654815237456"><a name="p1654815237456"></a><a name="p1654815237456"></a>-a</p>
</td>
<td class="cellrowborder" valign="top" width="44.34443444344434%" headers="mcps1.1.6.1.3 "><p id="p1024452794916"><a name="p1024452794916"></a><a name="p1024452794916"></a>输出为可读文本格式的字节码文件</p>
</td>
<td class="cellrowborder" valign="top" width="26.01260126012601%" headers="mcps1.1.6.1.4 "><p id="p184145774817"><a name="p184145774817"></a><a name="p184145774817"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="7.520752075207521%" headers="mcps1.1.6.1.5 "><p id="p20894101319494"><a name="p20894101319494"></a><a name="p20894101319494"></a>-</p>
</td>
</tr>
<tr id="row17707643124417"><td class="cellrowborder" valign="top" width="15.29152915291529%" headers="mcps1.1.6.1.1 "><p id="p1270714432449"><a name="p1270714432449"></a><a name="p1270714432449"></a>--debug</p>
</td>
<td class="cellrowborder" valign="top" width="6.830683068306829%" headers="mcps1.1.6.1.2 "><p id="p1548172334510"><a name="p1548172334510"></a><a name="p1548172334510"></a>-d</p>
</td>
<td class="cellrowborder" valign="top" width="44.34443444344434%" headers="mcps1.1.6.1.3 "><p id="p32437275490"><a name="p32437275490"></a><a name="p32437275490"></a>携带debug信息</p>
</td>
<td class="cellrowborder" valign="top" width="26.01260126012601%" headers="mcps1.1.6.1.4 "><p id="p134185718488"><a name="p134185718488"></a><a name="p134185718488"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="7.520752075207521%" headers="mcps1.1.6.1.5 "><p id="p5894613104916"><a name="p5894613104916"></a><a name="p5894613104916"></a>-</p>
</td>
</tr>
<tr id="row14707184314419"><td class="cellrowborder" valign="top" width="15.29152915291529%" headers="mcps1.1.6.1.1 "><p id="p5707194311449"><a name="p5707194311449"></a><a name="p5707194311449"></a>--show-statistics</p>
</td>
<td class="cellrowborder" valign="top" width="6.830683068306829%" headers="mcps1.1.6.1.2 "><p id="p1954872319456"><a name="p1954872319456"></a><a name="p1954872319456"></a>-s</p>
</td>
<td class="cellrowborder" valign="top" width="44.34443444344434%" headers="mcps1.1.6.1.3 "><p id="p16224102744913"><a name="p16224102744913"></a><a name="p16224102744913"></a>显示字节码相关的统计信息</p>
</td>
<td class="cellrowborder" valign="top" width="26.01260126012601%" headers="mcps1.1.6.1.4 "><p id="p154195704813"><a name="p154195704813"></a><a name="p154195704813"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="7.520752075207521%" headers="mcps1.1.6.1.5 "><p id="p19894131304917"><a name="p19894131304917"></a><a name="p19894131304917"></a>-</p>
</td>
</tr>
<tr id="row768813216460"><td class="cellrowborder" valign="top" width="15.29152915291529%" headers="mcps1.1.6.1.1 "><p id="p20688113244612"><a name="p20688113244612"></a><a name="p20688113244612"></a>--output</p>
</td>
<td class="cellrowborder" valign="top" width="6.830683068306829%" headers="mcps1.1.6.1.2 "><p id="p1468863218469"><a name="p1468863218469"></a><a name="p1468863218469"></a>-o</p>
</td>
<td class="cellrowborder" valign="top" width="44.34443444344434%" headers="mcps1.1.6.1.3 "><p id="p1468814324461"><a name="p1468814324461"></a><a name="p1468814324461"></a>输出文件路径</p>
</td>
<td class="cellrowborder" valign="top" width="26.01260126012601%" headers="mcps1.1.6.1.4 "><p id="p12624825135117"><a name="p12624825135117"></a><a name="p12624825135117"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="7.520752075207521%" headers="mcps1.1.6.1.5 "><p id="p1989431344920"><a name="p1989431344920"></a><a name="p1989431344920"></a>-</p>
</td>
</tr>
<tr id="row6445636154611"><td class="cellrowborder" valign="top" width="15.29152915291529%" headers="mcps1.1.6.1.1 "><p id="p64451436124618"><a name="p64451436124618"></a><a name="p64451436124618"></a>--timeout</p>
</td>
<td class="cellrowborder" valign="top" width="6.830683068306829%" headers="mcps1.1.6.1.2 "><p id="p1445113611468"><a name="p1445113611468"></a><a name="p1445113611468"></a>-t</p>
</td>
<td class="cellrowborder" valign="top" width="44.34443444344434%" headers="mcps1.1.6.1.3 "><p id="p1044510362466"><a name="p1044510362466"></a><a name="p1044510362466"></a>超时门限</p>
</td>
<td class="cellrowborder" valign="top" width="26.01260126012601%" headers="mcps1.1.6.1.4 "><p id="p745572486"><a name="p745572486"></a><a name="p745572486"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="7.520752075207521%" headers="mcps1.1.6.1.5 "><p id="p7894111310494"><a name="p7894111310494"></a><a name="p7894111310494"></a>-</p>
</td>
</tr>
<tr id="row1978841614720"><td class="cellrowborder" valign="top" width="15.29152915291529%" headers="mcps1.1.6.1.1 "><p id="p0788141614716"><a name="p0788141614716"></a><a name="p0788141614716"></a>--help</p>
</td>
<td class="cellrowborder" valign="top" width="6.830683068306829%" headers="mcps1.1.6.1.2 "><p id="p207884169473"><a name="p207884169473"></a><a name="p207884169473"></a>-h</p>
</td>
<td class="cellrowborder" valign="top" width="44.34443444344434%" headers="mcps1.1.6.1.3 "><p id="p178821634716"><a name="p178821634716"></a><a name="p178821634716"></a>帮助提示</p>
</td>
<td class="cellrowborder" valign="top" width="26.01260126012601%" headers="mcps1.1.6.1.4 "><p id="p1341757114819"><a name="p1341757114819"></a><a name="p1341757114819"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="7.520752075207521%" headers="mcps1.1.6.1.5 "><p id="p208946134499"><a name="p208946134499"></a><a name="p208946134499"></a>-</p>
</td>
</tr>
<tr id="row14354103234714"><td class="cellrowborder" valign="top" width="15.29152915291529%" headers="mcps1.1.6.1.1 "><p id="p1235410329479"><a name="p1235410329479"></a><a name="p1235410329479"></a>--bc-version</p>
</td>
<td class="cellrowborder" valign="top" width="6.830683068306829%" headers="mcps1.1.6.1.2 "><p id="p63541832124712"><a name="p63541832124712"></a><a name="p63541832124712"></a>-v</p>
</td>
<td class="cellrowborder" valign="top" width="44.34443444344434%" headers="mcps1.1.6.1.3 "><p id="p14354832134715"><a name="p14354832134715"></a><a name="p14354832134715"></a>输出当前字节码版本</p>
</td>
<td class="cellrowborder" valign="top" width="26.01260126012601%" headers="mcps1.1.6.1.4 "><p id="p16410577487"><a name="p16410577487"></a><a name="p16410577487"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="7.520752075207521%" headers="mcps1.1.6.1.5 "><p id="p14894201364911"><a name="p14894201364911"></a><a name="p14894201364911"></a>-</p>
</td>
</tr>
<tr id="row246823515473"><td class="cellrowborder" valign="top" width="15.29152915291529%" headers="mcps1.1.6.1.1 "><p id="p1346883524711"><a name="p1346883524711"></a><a name="p1346883524711"></a>--bc-min-version</p>
</td>
<td class="cellrowborder" valign="top" width="6.830683068306829%" headers="mcps1.1.6.1.2 ">&nbsp;&nbsp;</td>
<td class="cellrowborder" valign="top" width="44.34443444344434%" headers="mcps1.1.6.1.3 "><p id="p17469123534711"><a name="p17469123534711"></a><a name="p17469123534711"></a>输出支持的最低字节码版本</p>
</td>
<td class="cellrowborder" valign="top" width="26.01260126012601%" headers="mcps1.1.6.1.4 "><p id="p195557124818"><a name="p195557124818"></a><a name="p195557124818"></a>-</p>
</td>
<td class="cellrowborder" valign="top" width="7.520752075207521%" headers="mcps1.1.6.1.5 "><p id="p11894141354919"><a name="p11894141354919"></a><a name="p11894141354919"></a>-</p>
</td>
</tr>
</tbody>
</table>

## 反汇编器工具概述

工具名称为ark\_disasm，用于将二进制格式的方舟字节码文件转换为文本格式的方舟字节码文件。

编译生成反汇编工具：

```
./build.sh --product-name rk3568 --build-target ark_host_linux_tools_packages
```

命令行格式：

```
ark_disasm [选项] 输入文件 输出文件
```

<a name="table125062517328"></a>
<table><thead align="left"><tr id="row125182553217"><th class="cellrowborder" valign="top" width="50%" id="mcps1.1.3.1.1"><p id="p175162514327"><a name="p175162514327"></a><a name="p175162514327"></a>选项</p>
</th>
<th class="cellrowborder" valign="top" width="50%" id="mcps1.1.3.1.2"><p id="p6512255324"><a name="p6512255324"></a><a name="p6512255324"></a>描述</p>
</th>
</tr>
</thead>
<tbody><tr id="row5511825103218"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p45172513326"><a name="p45172513326"></a><a name="p45172513326"></a>--debug</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1245695053215"><a name="p1245695053215"></a><a name="p1245695053215"></a>使能调试信息, 如果没有指定"--debug-file", 输出形式将会是标准输出。默认值是false</p>
</td>
</tr>
<tr id="row951112515321"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p451192515323"><a name="p451192515323"></a><a name="p451192515323"></a>--debug-file</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p175142583210"><a name="p175142583210"></a><a name="p175142583210"></a>调试信息输出文件路径，默认为std::cout</p>
</td>
</tr>
<tr id="row951112515321"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p451192515323"><a name="p451192515323"></a><a name="p451192515323"></a>--skip-string-literals</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p175142583210"><a name="p175142583210"></a><a name="p175142583210"></a>将字符串用对应的string_ID代替，可以减少输出文件的大小。默认值是false</p>
</td>
</tr>
<tr id="row951112515321"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p451192515323"><a name="p451192515323"></a><a name="p451192515323"></a>--quiet</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p175142583210"><a name="p175142583210"></a><a name="p175142583210"></a>打开所有--skip-*选项。默认值是false</p>
</td>
</tr>
<tr id="row45116253325"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p85116259328"><a name="p85116259328"></a><a name="p85116259328"></a>--help</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p1348135833214"><a name="p1348135833214"></a><a name="p1348135833214"></a>帮助提示</p>
</td>
</tr>
<tr id="row194197407327"><td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.1 "><p id="p154205401325"><a name="p154205401325"></a><a name="p154205401325"></a>--verbose</p>
</td>
<td class="cellrowborder" valign="top" width="50%" headers="mcps1.1.3.1.2 "><p id="p369871173312"><a name="p369871173312"></a><a name="p369871173312"></a>输出更多关于类和方法在文件中的信息。默认值是false</p>
</td>
</tr>
</tbody>
</table>

输入文件：二进制格式的方舟字节码

输出文件：文本格式的方舟字节码

