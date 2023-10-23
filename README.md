# CSDK for ABCDEGF

[![Github Actions](https://github.com/openLuat/luatos-soc-2023/actions/workflows/ci.yaml/badge.svg)](https://github.com/openLuat/luatos-soc-2023/actions/workflows/ci.yaml)

## 使用前须知

1. **强烈推荐使用git来下载本代码库**, 不会命令行操作git可以用 [tortoiseGit](https://tortoisegit.org/download/)
2. 本CSDK底层是没有AT指令的,不存在"调用某某AT指令"的情况
3. **由于依赖luatos主库，所以要在 luatos-soc-2023 所在同级目录 clone [LuatOS](https://gitee.com/openLuat/LuatOS) 仓库，否则编译报错**

## 文档中心

* [CSDK软件开发资料](https://doc.openluat.com/wiki/37?wiki_page_id=4544)

## 介绍
CSDK for ABCDEGF, 适用于所有基于ABCDEGF的合宙模组, 包括衍生型号及海外型号:

* Air780P   

## 目录说明

* PLAT 打包好的sdk, 含协议栈, 部分源码, 头文件, 打包工具
* project 项目文件, 每个不同的项目代表不同的demo或者turnkey方案
* xmake.lua 主编译脚本, 一般不需要修改

## 编译说明

1. 安装xmake, **选取加入PATH**；
   xmake下载地址：https://xmake.io/#/guide/installation
   windows下可以直接访问：https://github.com/xmake-io/xmake/releases ,下载系统对应的xmake-x.x.x-win32.exe或者xmake-x.x.x-win64.exe即可. 建议使用最新版.

   **注意：环境变量需重启电脑生效**

2. 如果编译example的话在本代码库的根目录执行`build.bat example`进行编译，**注意：默认编译ec718p，如需编译*ec718s*或 *ec716s*需要自行修改build.bat中的*CHIP_TARGET*变量**

4. 生成的binpkg位于`out`目录, 日志数据库文件位于`PLAT`目录

## 加入入口函数

* 加入头文件common_api.h
* 类似于`void main(void)`，但是可以任意名字，不能有入口参数和返回参数，同时允许多个入口
* 通过INIT_HW_EXPORT INIT_DRV_EXPORT INIT_TASK_EXPORT这3个宏修饰后，系统初始化的时候会调用function，不同的宏涉及到调用顺序不同
* 大优先级上`INIT_HW_EXPORT` > `INIT_DRV_EXPORT` > `INIT_TASK_EXPORT`
* 这3个宏有统一用法`INIT_HW_EXPORT(function, level)`，其中function就是入口函数，level是小优先级，填"0","1","2"...(引号要写的)，引号里数字越小，优先级越高。

* `INIT_HW_EXPORT`一般用于硬件初始化，GPIO之类的，可以没有
* `INIT_DRV_EXPORT`一般用于外设驱动初始化，初始化外部器件之类的，打开电源，提供时钟之类的，可以没有
* `INIT_TASK_EXPORT`一般用于初始化任务，用户代码基本上都是跑在任务里，原则上必须有

## 加入自己的项目

1. project新建目录，目录名称和项目名称一致，目录下新建xmake.lua，内容仿照着example写，核心是TARGET_NAME必须和项目名称一致
2. 代码路径当然不是限制的，在SDK的任何目录，甚至电脑上的其他地方，前提是你会改project里xmake.lua内的源码路径
3. 根目录执行build.bat 你的项目名称

## 在SDK外存放项目文件的编译方法

假设项目路径是 `D:\github\ABCDEGF-webabc` , 目录结构如下

```
D:\github
    - ABCDEGF-webabc
        - code
            - xmake.lua
            - src
                app_main.c
                task_mqtt.c
            - inc
                app_main.h
                RTE_Device.h
        - doc
            - README.md
```

编译方式:

```
set PROJECT_DIR=D:\github\ABCDEGF-webabc\code
build webabc
```

注意: code\xmake.lua里面的TARGET_NAME,对应命令行的项目名, `webabc`

恢复到默认项目查找逻辑, 之后就能恢复到 project/xxx 查找xmake.lua

```
set PROJECT_DIR=
build luatos
```



## 授权协议

[MIT License](LICENSE)
