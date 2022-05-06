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
args:=
root_dir=$(shell pwd)
ifeq ($(test), )
	test_name=helloaot
else
	test_name=$(test)
endif

ifeq ($(log), yes)
	args+= --log-compiled-methods=all
endif

ifeq ($(debug), no)
	export LD_LIBRARY_PATH=$(root_dir)/out/hi3516dv300/clang_x64/ark/ark:$(root_dir)/out/hi3516dv300/clang_x64/ark/ark_js_runtime:$(root_dir)/out/hi3516dv300/clang_x64/common/common:$(root_dir)/prebuilts/clang/ohos/linux-x86_64/llvm/lib
	bin_dir=$(root_dir)/out/hi3516dv300/clang_x64/ark/ark_js_runtime
else
	export LD_LIBRARY_PATH=$(root_dir)/out/hi3516dv300/clang_x64/exe.unstripped/clang_x64/ark/ark:$(root_dir)/out/hi3516dv300/clang_x64/exe.unstripped/clang_x64/ark/ark_js_runtime:$(root_dir)/out/hi3516dv300/clang_x64/lib.unstripped/clang_x64/ark/ark:$(root_dir)/out/hi3516dv300/clang_x64/lib.unstripped/clang_x64/ark/ark_js_runtime:$(root_dir)/out/hi3516dv300/clang_x64/lib.unstripped/clang_x64/test/test:$(root_dir)/out/hi3516dv300/clang_x64/lib.unstripped/clang_x64/common/common:$(root_dir)/prebuilts/clang/ohos/linux-x86_64/llvm/lib
	bin_dir=$(root_dir)/out/hi3516dv300/clang_x64/exe.unstripped/clang_x64/ark/ark_js_runtime
endif

ts2abc=$(root_dir)/out/hi3516dv300/clang_x64/obj/ark/ts2abc/ts2panda/build/src/index.js
test_dir=$(root_dir)/ark/js_runtime/test/aottest
out_dir=$(root_dir)/out/hi3516dv300/clang_x64/aottest
case_dir=$(out_dir)/$(test_name)
com_stub_args=--enable-stub-aot=1 --asmInter=1 --bc-stub-out=$(out_dir)/bc_stub.m --com-stub-out=$(out_dir)/com_stub.m

abc:
	mkdir -p $(case_dir)
	node --expose-gc $(ts2abc) $(test_dir)/$(test_name)/$(test_name).ts -o $(case_dir)/$(test_name).abc

stub:
	mkdir -p $(out_dir)
	cd $(out_dir) && $(bin_dir)/ark_stub_compiler $(args) --com-stub-out=com_stub.m --bc-stub-out=bc_stub.m

aot:
	cd $(case_dir) && $(bin_dir)/ark_aot_compiler $(args) $(test_name).abc

aotd:
	cd $(case_dir) && gdb --args $(bin_dir)/ark_aot_compiler $(args) $(test_name).abc

run:
	cd $(case_dir) && $(bin_dir)/ark_js_vm $(args) --enable-ts-aot=1 $(com_stub_args) $(test_name).abc

rund:
	cd $(case_dir) && gdb --args $(bin_dir)/ark_js_vm $(args) --enable-ts-aot=1 $(com_stub_args) $(test_name).abc
	
int:
	cd $(case_dir) && $(bin_dir)/ark_js_vm $(args) $(test_name).abc

intd:
	cd $(case_dir) && gdb --args $(bin_dir)/ark_js_vm $(args) $(test_name).abc
	
asmint:
	cd $(case_dir) && $(bin_dir)/ark_js_vm $(args) $(com_stub_args) $(test_name).abc

asmintd:
	cd $(case_dir) && gdb --args $(bin_dir)/ark_js_vm $(args) $(com_stub_args) $(test_name).abc

env:
	@echo "root_dir=$(root_dir)"
	@echo "test_name=$(test_name)"
	@echo "test_dir=$(test_dir)"
	@echo "case_dir=$(case_dir)"
	@echo "LD_LIBRARY_PATH=$(LD_LIBRARY_PATH)"