# Environment Setup and Compilation

## Environment Configuration

Use Ubuntu 18.04 or 20.04. For details about how to set up the environment, see:

[Setting Up Ubuntu Development Environment with Installation Package and Building Source Code](https://gitee.com/openharmony/docs/blob/master/en/device-dev/quick-start/quickstart-standard-package-environment.md)

## Compilation

1.  First compilation:

    ```
    ./build.sh --product-name Hi3516DV300
    ```

2.  Compile an ARK runtime after the first compilation:

    ```
    ./build.sh --product-name Hi3516DV300 --build-target ark_js_host_linux_tools_packages  # Arm platform and host runtime tool
    ```

3.  Compile the ARK frontend after the first compilation:

    ```
    ./build.sh --product-name Hi3516DV300 --build-target ark_ts2abc_build
    ```


The binary files related to ARK are available in the following paths:

```
out/hi3516dv300/ark/ark/
out/hi3516dv300/ark/ark_js_runtime/
out/hi3516dv300/clang_x64/ark/ark/
out/hi3516dv300/clang_x64/ark/ark_js_runtime
```
