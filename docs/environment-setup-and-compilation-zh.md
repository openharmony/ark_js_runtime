# 环境搭建和编译<a name="ZH-CN_TOPIC_0000001174215863"></a>

-   [环境配置](#section922419503415)
-   [代码编译](#section1166711064317)

## 环境配置<a name="section922419503415"></a>

Ubuntu版本要求18.04或20.04，详细环境搭建参考：

[搭建Ubuntu环境及编译（安装包方式）](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/quick-start/quickstart-standard-package-environment.md)

## 代码编译<a name="section1166711064317"></a>

1.  首次编译：

    ```
    ./build.sh --product-name rk3568
    ```

2.  首次编译后增量编译方舟运行时：

    编译x64版本：
	```
    ./build.sh --product-name rk3568 --build-target ark_js_host_linux_tools_packages --build-target ark_ts2abc_build  # arm平台和host端运行工具
    ```

	编译arm64版本：
	```
	./build.sh --product-name ohos_arm64 --build-target ark_js_vm --build-target ld-musl-aarch64.so.1
	```

	编译arm32版本:
	```
	./build.sh --product-name rk3568 --build-target libarkruntime --build-target ark_js_runtime --build-target ld-musl-arm.so.1
	```

3.  首次编译后增量编译方舟前端：

    ```
    ./build.sh --product-name rk3568 --build-target ark_ts2abc_build
    ```

**说明**：上述编译命令为release版本，且执行路径为项目根目录。编译debug版本需增加编译选项：--gn-args is_debug=true。

方舟相关的二进制文件在如下路径：

```
out/rk3568/ark/ark/
out/rk3568/ark/ark_js_runtime/
out/rk3568/clang_x64/ark/ark/
out/rk3568/clang_x64/ark/ark_js_runtime
```

