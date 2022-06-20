# ark_build
注意：目前版本部分内容还需完善，后续入库后会做具体说明

#### 介绍
方舟独立编译构建build

#### 软件架构和目录
独立编译目录结构

/ark/ 

├── js_runtime 

    ├── build     
           
├── runtime_core   
          
├── ts2abc         

├── third_patry         

#所有仓库需从openharmony开源项目下载


#### 安装教程

1.  独立编译拉取ark_js_runtime、ark_runtime_core、ark_ts2abc三个仓，执行[./build/prebuilts_download.sh] 下载相关的编译所需工具。
2.  之后执行[./js_runtime/build/compile_script/gen.sh ark]将.gn与.sh文件拿出来。
3.  执行[./gen.sh]命令编译目前独立编译支持的所有目标，执行[./gen.sh abc]命令生成abc文件，执行[./gen.sh .]命令执行abc文件

#### 使用说明

1.  独立编译build目录放在ark_js_runtime仓下，它和build仓不一样，是独立编译特有的
2.  所需三方库从openharmony开源项目下载，包含 :
        https://gitee.com/openharmony/third_party_bounds_checking_function,
        https://gitee.com/openharmony/third_party_icu,
        https://gitee.com/openharmony/third_party_jsoncpp,
        https://gitee.com/openharmony/third_party_zlib
3.  编译时需拉取三方库到ark/third_party/xxx

#### 参与贡献

1.  Fork 本仓库
2.  新建 Feat_xxx 分支
3.  提交代码
4.  新建 Pull Request
