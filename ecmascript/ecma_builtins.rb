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

def ExtBuiltins.load_ecma_builtins(data)
  ecma_fname = File.expand_path(File.join(File.dirname(__FILE__), 'ecma_runtime.yaml'))
  builtins = YAML.load_file(ecma_fname)
  builtins = JSON.parse(builtins.to_json, object_class: OpenStruct)
  builtins.intrinsics.each do |intr|
    new_intr = intr.signature.clone

    if !intr.signature['sig']
      new_intr['sig'] = intr.method_name
      v_idx = 1
      imm_idx =2 # first imm is subcode, so other begin at 2
      new_intr.args.each_index do |i|
        case new_intr.args[i]
        when "any"
          new_intr['sig'] << " v" + v_idx.to_s + ":in:any"
          v_idx += 1
        when "acc"
          next
        when "u8", "i8", "u16", "i16", "u32", "i32"
          new_intr['sig'] << " imm" + imm_idx.to_s + ":i32"
          imm_idx += 1
        when "u64", "i64"
          new_intr['sig'] << " imm" + imm_idx.to_s + ":i64"
          imm_idx += 1
        when "string_id"
          new_intr['sig'] << " string_id"
        when "method_id"
          new_intr['sig'] << " method_id"
        else
          raise "unknown the type in " + intr.method_name
        end
      end
    end

    if new_intr['sig'].include?('v1:') && !new_intr['sig'].include?('v2:')
      new_intr['sig'].sub!(/v1+?/, 'v')
    end

    if !intr.signature['acc']
      if new_intr.ret == "any" then
        new_intr['acc'] = "out:any"
      elsif new_intr.ret == "void" then
        new_intr['acc'] = "none"
      else
        raise "unknown the ret in " + intr.method_name
      end
    end

    if intr['exception'] == true
      new_intr['exception'] = true
    end

    new_intr['space'] = intr.space

    new_intr['name'] = intr.name

    data["builtins"] << new_intr
  end
end