# 非官方社区修复版

原项目与作者：[aloofbit/comfyatmosphere](https://github.com/aloofbit/comfyatmosphere)。
本分支由cwozycz-sudo维护，不代表原作者发布或背书。
保留原版权声明及完整[GNU GPL v3](LICENSE)，修改说明见[NOTICE.community.md](NOTICE.community.md)。

修复雨滴、雨雾、建筑水面的判别缓存；约5MiB日志轮转；增加慢帧分项记录；阴影动画匹配分帧处理。
所有物件共用约1毫秒每帧的检索预算，保留近邻搜索、全局粗搜、精细搜索和动画时钟。
这不是所有渲染开销的上限，一次不可抢占的评价可能超时；重新同步可能需要多帧。
未删除高清模型、鱼、蝙蝠或光影效果，也未降低画质。

CPU原生回归和独立DXVK资源/Reset检查通过；一套Windows/NVIDIA环境的用户反馈帧数明显稳定。
尚无严格的修补后FPS对照，不能承诺所有硬件、客户端和场景均无卡顿。

安装前退出游戏、备份旧DLL。已有用户仅替换DLL，保留个人ini和插件配置。
首次安装按原README配置加载器和addon/ComfyAtmosphere。
发布包不含游戏客户端、MPQ、DXVK、其他加载器、个人配置、日志、账号或服务器资料。
若启动器强制管理此文件，应通过该服务的维护方更新。
回退时退出游戏后恢复事先备份的DLL。

面对太阳出现黑底亮斑时，先输入 `/console comfyDebugView 0` 关闭调试视图。
本版未实现每次启动自动关闭已保存的调试视图。

编译需要Visual Studio 2022、C++工具链/Windows SDK和CMake，架构Win32。
运行 `./scripts/build-community.ps1`，或按[英文说明](COMMUNITY.md)执行CMake与CTest命令。
完整对应源码、测试、编译脚本包含在源码包中，不需要本机目录外的文件。
上游基线：73bebaaf2439aa17cc7766bb99bf8513f9aa6ade。
