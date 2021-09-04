# Environment Setup and Compilation<a name="EN-US_TOPIC_0000001174215863"></a>

-   [Configuring the Environment](#section922419503415)
-   [Compilation](#section1166711064317)

## Configuring the Environment<a name="section922419503415"></a>

Use Ubuntu 18.04 or 20.04. For details about how to set up the environment, see:

[Setting Up Ubuntu Development Environment with Installation Package and Building Source Code](https://gitee.com/openharmony/docs/blob/master/en/device-dev/quick-start/quickstart-standard-package-environment.md)

## Compilation<a name="section1166711064317"></a>

1.  First compilation:

    ```
    ./build.sh --product-name Hi3516DV300
    ```

2.  Compile an ARK runtime after the first compilation:

    ```
    ./build.sh --product-name Hi3516DV300 --build-target ark_js_runtime
    ```

3.  Compile the ARK frontend after the first compilation:

    ```
    ./build.sh --product-name Hi3516DV300 --build-target ark_ts2abc_build
    ```


The binary files related to ARK are available in the following paths:

```
out/ohos-arm-release/ark/ark/
out/ohos-arm-release/ark/ark_js_runtime/
out/ohos-arm-release/clang_x64/ark/ark/
out/ohos-arm-release/clang_x64/ark/ark_js_runtime
```

