# 当前三工程代码图入口

负责角色：代码智能。本次依据 file_manifest.txt、项目地址.txt、readme.txt、旧索引交接、backend_feasibility_current.md、Git 和各 codegraph.json 核对范围。AIP 是产品容器，根没有索引，三个相关子工程均已有 .codegraph。使用已安装的 @colbymchenry/codegraph 1.6.0 正常增量同步，没有初始化、全量重建、安装或升级。

接手 Git 工作区为空；采集时新增了本次采集脚本。源码 HEAD：`6e98b727484c2de344ff5e8b63ae15dc89d1ac60`。各子工程相对 HEAD 无差异。

[验证证据](../evidence/codegraph_refresh_20261123.json) 按 roots.name 区分工程，保存命令、UTC起止、超时上限、退出码、stderr、前后状态、当前源码行号节选、调用上下文、响应摘要及逐文件原始字节SHA-256。[采集脚本](../evidence/codegraph_refresh_20261123.py) 保留有界采集方法。不展开整个源码或全图。

验证时间：2026-11-23T07:15:12.535830+00:00 至 2026-11-23T07:15:59.321343+00:00（UTC）。

## 传感器 (sensor)

- projectPath：`D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output`
- indexPath：`D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output\.codegraph`，数据库仅留本地。
- 配置：该根 codegraph.json；覆盖：USER/、COMMON/、UM2005C/、rfconfig.h 及本工程 C 测试；test/降低排序优先级。子根之外 CW32 SDK 与其他示例未纳入。
- 配置排除：`MDK/RTE/`、`MDK/output/`、`EWARM/Debug/`、`EWARM/Release/`、`obj/`、`build/`。
- HEAD子树：`d0535fee6ade459755b270a803887770a781978f`；最近子工程提交：`6a749d4eb3e911441e0af171262dd5b4554f389c`。
- 同步结束：2026-11-23T07:15:13.930187+00:00；状态检查结束：2026-11-23T07:15:14.322694+00:00；查询时间详见该根 queries。
- 状态：99文件、1257节点、3496边。
- 索引内容集合SHA-256：`9bb96e89455f6e8d66d95bc9d843fcb521d1769f07b5da806e46948f4a9f17f8`。
- 查询证据：gxht40_measure sensor_decide_report light_is_dark 查询返回 USER/src/gxht40.c:170、USER/src/fw_core.c:112 等当前源码与 measure.c 调用上下文；measure_sample 命中 USER/src/measure.c:460。

最小查询：

```powershell
codegraph explore -p "D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output" --max-files 1 "sensor_decide_report"
```

## 网关 (gateway)

- projectPath：`D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2`
- indexPath：`D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2\.codegraph`，数据库仅留本地。
- 配置：该根 codegraph.json；覆盖：APP/、Profile/的 C 源码和头文件。项目入口明确其为产品工程；上级 HAL、BLE 预编译库、WCH SDK 和其他 EVT 例程未纳入。
- 配置排除：`.settings/`、`.idea/`、`obj/`、`Debug/`、`Release/`、`build/`。
- HEAD子树：`22c8a193daffea87408c971a53db566633f7c6ea`；最近子工程提交：`4d101ae6459f89fa7f26f0d437d1d86a8be7a369`。
- 同步结束：2026-11-23T07:15:16.730265+00:00；状态检查结束：2026-11-23T07:15:17.116024+00:00；查询时间详见该根 queries。
- 状态：34文件、508节点、1066边。
- 索引内容集合SHA-256：`2d7a74f42db9cbbd4a6db67aacc32ac3abb59125810fec441225e6a33408979d`。
- 查询证据：decode_frame10 命中 APP/feistel_al.c:119，调用方为 APP/app_um2006A.c；app_um2006A_run ble_send_bf 查询返回 APP/app_um2006A.c 当前源码。

最小查询：

```powershell
codegraph explore -p "D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2" --max-files 1 "decode_frame10"
```

## Android (android)

- projectPath：`D:\mypro\beiwo2\AIP\dengbei_care`
- indexPath：`D:\mypro\beiwo2\AIP\dengbei_care\.codegraph`，数据库仅留本地。
- 配置：该根 codegraph.json；覆盖：本地 Gradle 工程可解析的 Kotlin/Java、Manifest、properties 等，含测试与内嵌 Paho。app/src/test/、app/src/androidTest/、app/src/main/java/org/eclipse/paho/降低排序优先级。外部 Android SDK、Gradle 依赖、APK 和 iOS 工程未纳入。
- 配置排除：`iOS开发资料/`、`.idea/`、`.gradle/`、`build/`、`app/build/`。
- HEAD子树：`7caa53518f75d8ca8823e1909f37903090654c91`；最近子工程提交：`a83861fcf4b360520c56223b09446ca9508547bd`。
- 同步结束：2026-11-23T07:15:19.947270+00:00；状态检查结束：2026-11-23T07:15:20.598903+00:00；查询时间详见该根 queries。
- 状态：113文件、3009节点、6565边。
- 索引内容集合SHA-256：`8d1f283b1d9a025ab6e979b513708840e8e3fcbd3e6510d3d61d1be9f6820e40`。
- 查询证据：CloudUploadRepository 命中 cloud/CloudUploadRepository.kt:28，调用方 MqtttService.kt；GatewayFrameCodec、GatewayReadState 的符号查询选中关联文件，随后精确路径查询均返回目标源码。原尝试保留，current_source_located=false 不算目标源码验证。路径前缀为 app/src/main/java/com/jinyuni/dengbei_care/。

最小查询：

```powershell
codegraph explore -p "D:\mypro\beiwo2\AIP\dengbei_care" --max-files 1 "app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayFrameCodec.kt"
```

## 验证边界与后续使用

传感器、网关 sync 均为 Already up to date；Android 同步新增3文件，由110更新为113文件。无变更同步不刷新 lastIndexed，本次结论依据实际同步和当前源码查询。三根均 complete、pendingRefs=0、待新增/修改/删除均0、worktreeMismatch=null、reindexRecommended=false。全部验证命令退出0、stderr为空，没有超时；采集子进程均已退出，无后台诊断作业。三个索引均被现有规则忽略，git ls-files 未发现受跟踪数据库；细粒度图仅保留本地，不提交大型数据库。本文不声称已提交、发布或收入交付包。

indexed_source_hashes 保存各根实际索引文件的原始字节摘要，数量与 fileCount 相符。集合摘要算法为路径排序后，将 path+TAB+lowercase_sha256+LF 的UTF-8拼接再求SHA-256。

已登记的三个本地代码根均已验证。平台代码根未验证：当前资源与清单未提供独立本地服务端源码工程；远端运行报告不代替源码版本或查询，本地桩也不是平台源码。本次没有远程读取、复制或索引服务端。如需平台符号图，请资料负责人提供当前源码根和版本，再依据当时有效授权准备。本次就绪仅适用于上列三个本地根，不声明平台索引就绪；没有发现其他已确认必须索引但遗漏验证的本地根。

Android 的 app/src/main/res/layout/activity_main.xml、fragment_home.xml、fragment_notifications.xml 仍未列入113文件列表，布局覆盖限制保留。宏、条件编译、链接脚本、外部库及跨设备433/BLE/HTTP链路不能只靠静态图证明。语言列表不代表所有资源均覆盖。SDK、归档、生成目录、硬件及无关iOS资料未扩大索引。

专业消费者必须指定上述真实 projectPath；MCP 示例：codegraph_explore({projectPath: "对应绝对路径", query: "相关文件或符号", maxFiles: 1})。直接使用返回的当前源码与调用关系；排序偏离目标时补用精确路径查询。图未命中的明确文件按职责有界读取。

源码修改、版本切换或排除配置变化后依次执行：

```powershell
codegraph sync "<projectPath>"
codegraph status --json "<projectPath>"
codegraph explore -p "<projectPath>" --max-files 1 "<本次相关文件或符号>"
```

确认来源版本、同步状态和当前源码命中后再沿用结果。覆盖查询使用 codegraph files -p "<projectPath>" --format flat --no-metadata。未修改产品实现、测试、设备配置、权限或工作流，未执行后续业务测试。

清单核对1876条路径无缺失，本次三个新增文件均已读回确认。仓库没有 scripts/check_file_manifest.py，使用同格式的有界路径存在性检查；证据见 handoff_validation。
