# 环境搭建和编译<a name="ZH-CN_TOPIC_0000001174215863"></a>

-   [环境配置](#section922419503415)
-   [代码编译](#section1166711064317)

## 环境配置<a name="section922419503415"></a>

Ubuntu版本要求18.04或20.04，详细环境搭建参考：

[搭建Ubuntu环境及编译（安装包方式）](https://gitee.com/openharmony/docs/blob/master/zh-cn/device-dev/quick-start/quickstart-standard-package-environment.md)

## 代码编译<a name="section1166711064317"></a>

1.  首次编译：

    ```
    ./build.sh --product-name Hi3516DV300
    ```

2.  首次编译后增量编译方舟运行时：

    ```
    ./build.sh --product-name Hi3516DV300 --build-target ark_js_host_linux_tools_packages  # arm平台和host端运行工具
    ```

3.  首次编译后增量编译方舟前端：

    ```
    ./build.sh --product-name Hi3516DV300 --build-target ark_ts2abc_build
    ```


方舟相关的二进制文件在如下路径：

```
out/ohos-arm-release/ark/ark/
out/ohos-arm-release/ark/ark_js_runtime/
out/ohos-arm-release/clang_x64/ark/ark/
out/ohos-arm-release/clang_x64/ark/ark_js_runtime
```

