# Copyright (c) 2021 Huawei Device Co., Ltd.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

debug:=no
log:=no
arm:=no
args:=
product:=
product_dir:=
qemu:=
compiler:=
compiler_args:=
stub_compiler:=
jsvm:=
root_dir=$(shell pwd)
ifeq ($(test), )
	test_name=helloaot
else
	test_name=$(test)
endif

ifeq ($(log), yes)
	args+= --log-compiled-methods=all
endif

ifeq ($(arm), no)
	product=hispark_taurus
	product_dir=$(root_dir)/out/$(product)
	ifeq ($(debug), no)
		export LD_LIBRARY_PATH=$(product_dir)/clang_x64/ark/ark:$(product_dir)/clang_x64/ark/ark_js_runtime:$(product_dir)/clang_x64/thirdparty/icu:$(root_dir)/prebuilts/clang/ohos/linux-x86_64/llvm/lib
		compiler=$(product_dir)/clang_x64/ark/ark_js_runtime/ark_aot_compiler
		stub_compiler=$(product_dir)/clang_x64/ark/ark_js_runtime/ark_stub_compiler
		jsvm=$(product_dir)/clang_x64/ark/ark_js_runtime/ark_js_vm
	else
		export LD_LIBRARY_PATH=$(product_dir)/clang_x64/exe.unstripped/clang_x64/ark/ark:$(product_dir)/clang_x64/exe.unstripped/clang_x64/ark/ark_js_runtime:$(product_dir)/clang_x64/lib.unstripped/clang_x64/ark/ark:$(product_dir)/clang_x64/lib.unstripped/clang_x64/ark/ark_js_runtime:$(product_dir)/clang_x64/lib.unstripped/clang_x64/test/test:$(product_dir)/clang_x64/lib.unstripped/clang_x64/thirdparty/icu:$(root_dir)/prebuilts/clang/ohos/linux-x86_64/llvm/lib
		compiler=$(product_dir)/clang_x64/exe.unstripped/clang_x64/ark/ark_js_runtime/ark_aot_compiler
		stub_compiler=$(product_dir)/clang_x64/exe.unstripped/clang_x64/ark/ark_js_runtime/ark_stub_compiler
		jsvm=$(product_dir)/clang_x64/exe.unstripped/clang_x64/ark/ark_js_runtime/ark_js_vm
	endif
else
	product=rk3568
	product_dir=$(root_dir)/out/$(product)
	qemu=qemu-aarch64
	compiler_args+= --target-triple=aarch64-unknown-linux-gnu
	ifeq ($(debug), no)
		export LD_LIBRARY_PATH=$(product_dir)/ark/ark/:$(product_dir)/ark/ark_js_runtime/:prebuilts/clang/ohos/linux-x86_64/llvm/lib/aarch64-linux-ohos/c++/:$(product_dir)/utils/utils_base/:$(product_dir)/thirdparty/icu:$(product_dir)/securec/thirdparty_bounds_checking_function/:$(product_dir)/hiviewdfx/hitrace_native/:$(product_dir)/clang_x64/ark/ark/:$(product_dir)/clang_x64/thirdparty/icu/:$(product_dir)/clang_x64/ark/ark_js_runtime
		compiler=$(product_dir)/clang_x64/ark/ark_js_runtime/ark_aot_compiler
		stub_compiler=$(product_dir)/clang_x64/ark/ark_js_runtime/ark_stub_compiler
		jsvm=$(product_dir)/ark/ark_js_runtime/ark_js_vm
	else
		export LD_LIBRARY_PATH=$(product_dir)/lib.unstripped/ark/ark/:$(product_dir)/lib.unstripped/ark/ark_js_runtime/:prebuilts/clang/ohos/linux-x86_64/llvm/lib/aarch64-linux-ohos/c++/:$(product_dir)/lib.unstripped/utils/utils_base/:$(product_dir)/lib.unstripped/thirdparty/icu:$(product_dir)/lib.unstripped/securec/thirdparty_bounds_checking_function/:$(product_dir)/lib.unstripped/hiviewdfx/hitrace_native/:$(product_dir)/lib.unstripped/clang_x64/ark/ark/:$(product_dir)/clang_x64/lib.unstripped/clang_x64/thirdparty/icu/:$(product_dir)/clang_x64/lib.unstripped/clang_x64/ark/ark/:$(product_dir)/clang_x64/lib.unstripped/clang_x64/ark/ark_js_runtime
		compiler=$(product_dir)/clang_x64/exe.unstripped/clang_x64/ark/ark_js_runtime/ark_aot_compiler
		stub_compiler=$(product_dir)/clang_x64/exe.unstripped/clang_x64/ark/ark_js_runtime/ark_stub_compiler
		jsvm=$(product_dir)/exe.unstripped/ark/ark_js_runtime/ark_js_vm
	endif
endif

test_dir=$(root_dir)/ark/js_runtime/test/aottest
ts2abc=$(product_dir)/clang_x64/obj/ark/ts2abc/ts2panda/build/src/index.js
out_dir=$(product_dir)/clang_x64/aottest
case_dir=$(out_dir)/$(test_name)
com_stub_args=--asm-interpreter=true --stub-file=$(out_dir)/stub.m

abc:
	mkdir -p $(case_dir)
	node --expose-gc $(ts2abc) $(test_dir)/$(test_name)/$(test_name).ts -o $(case_dir)/$(test_name).abc

stub:
	mkdir -p $(out_dir)
	$(stub_compiler) $(compiler_args) $(args) --stub-file=$(out_dir)/stub.m
	
aot:
	$(compiler) $(compiler_args) $(args) --aot-file=$(case_dir)/aot_file.m $(case_dir)/$(test_name).abc

aotd:
	$(gdb) $(compiler) $(compiler_args) $(args) --aot-file=$(case_dir)/aot_file.m $(case_dir)/$(test_name).abc

run:
	$(qemu) $(jsvm) $(args) --aot-file=$(case_dir)/aot_file.m $(com_stub_args) $(case_dir)/$(test_name).abc

rund:
ifeq ($(arm), no)
	gdb --args $(jsvm) $(args) --aot-file=$(case_dir)/aot_file.m $(com_stub_args) $(case_dir)/$(test_name).abc
else
	@echo "gdb-client start:   gdb-multiarch $(jsvm)"
	@echo "gdb-server connect: target remote:123456"
	$(qemu) -cpu max,sve=off -g 123456 $(jsvm) $(args) --aot-file=$(case_dir)/aot_file.m $(com_stub_args) $(case_dir)/$(test_name).abc
endif

int:
	$(qemu) $(jsvm) $(args) $(case_dir)/$(test_name).abc

intd:
ifeq ($(arm), no)
	gdb --args $(jsvm) $(args) $(case_dir)/$(test_name).abc
else
	@echo "gdb-client start:   gdb-multiarch $(jsvm)"
	@echo "gdb-server connect: target remote:123456"
	$(qemu) -cpu max,sve=off -g 123456 $(jsvm) $(args) $(case_dir)/$(test_name).abc
endif

asmint:
	$(qemu) $(jsvm) $(args) $(com_stub_args) $(case_dir)/$(test_name).abc

asmintd:
ifeq ($(arm), no)
	gdb --args $(jsvm) $(args) $(com_stub_args) $(case_dir)/$(test_name).abc
else
	@echo "gdb-client start:   gdb-multiarch $(jsvm)"
	@echo "gdb-server connect: target remote:123456"
	$(qemu) -cpu max,sve=off -g 123456 $(jsvm) $(args) $(com_stub_args) $(case_dir)/$(test_name).abc
endif

env:
	@echo "root_dir=$(root_dir)"
	@echo "test_name=$(test_name)"
	@echo "test_dir=$(test_dir)"
	@echo "case_dir=$(case_dir)"
	@echo "export LD_LIBRARY_PATH=$(LD_LIBRARY_PATH)"