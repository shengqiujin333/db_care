# 当前代码图入口

负责角色：代码智能。核对依据为当前 `file_manifest.txt`、`项目地址.txt`、`readme.txt`、三个子工程的 `codegraph.json` 和实际查询。产品容器根 `D:\mypro\beiwo2\AIP` 无 `.codegraph/`；应使用下列子工程的真实 projectPath。

本轮使用已安装的 `@colbymchenry/codegraph 1.6.0`，沿用三个现有索引并逐个增量同步，没有重新初始化、全量重建或安装软件。验证源码基线为 Git HEAD `e787681c348f5a6aeb30a55b8056e8b4c6a6f6bd`；启动时工作区干净，各子工程相对 HEAD 无差异。

查询证据：[codegraph_prepare_validation.json](../evidence/codegraph_prepare_validation.json)。`roots` 以 `name` 区分工程，记录真实路径、命令、UTC 起止时间、退出码、stderr、同步前后状态、查询摘要及有行号的源码节选。查询原始完整响应仅保留 SHA-256，避免展开整个源码；`indexed_source_hashes` 保存实际索引文件的当前内容摘要，便于确认来源版本。

## 传感器（sensor）

- projectPath：`D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output`
- indexPath：`D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output\.codegraph`
- 覆盖：`USER/`、`COMMON/`、`UM2005C/`、`rfconfig.h` 及本工程测试 C 源码/头文件；测试和 mock 位于 `test/`，仅降低排序优先级，不视作真实 MCU 驱动。
- 排除：`MDK/RTE/`、`MDK/output/`、`EWARM/Debug/`、`EWARM/Release/`、`obj/`、`build/`；子工程外的 CW32 SDK 与其它示例不在本索引中。
- 最近子工程提交：`ccf403113f25fe405f33c2ad972b5ac8327d290f`；当前已索引文件内容集合 SHA-256：`00b102336c11bf620d27d8030673a7c2d4a0c802a2ee9762e2f70b1ba5280e9b`（计算格式见证据中的逐文件摘要）。
- 同步结束：2026-11-10 07:34:28 UTC；状态检查结束：07:34:29 UTC；查询在 07:34:48 UTC 后执行，各次精确时间见 `roots[name=sensor].queries`。
- 验证：99 文件、1255 节点、3476 边；`gxht40_measure sensor_decide_report light_is_dark` 返回 `USER/src/gxht40.c:170`、`USER/src/fw_core.c:110` 和 `:94` 的当前源码，以及 `measure.c`、`light.c` 调用方；`measure_sample` 当前位于 `USER/src/measure.c:460`。
- UART 调试入口：`debug_trace_init` 查询只匹配到 `USER/inc/debug_trace.h` 的相关声明，不能据此证明存在同名初始化函数；补充固定文件查询 `USER/src/debug_trace.c` 已返回当前实现源码。初始化具体符号由消费角色继续查询。

最小查询：

```powershell
codegraph explore -p "D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output" --max-files 1 "sensor_decide_report"
```

## 网关（gateway）

- projectPath：`D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2`
- indexPath：`D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2\.codegraph`
- 覆盖：产品工程 `APP/` 和 `Profile/` 的 C 源码/头文件。目录虽位于厂商 EVT 内，已由项目入口明确为本项目网关；用于 433/BLE 接口上下文查询。
- 排除：`.settings/`、`.idea/`、`obj/`、`Debug/`、`Release/`、`build/`；上级 HAL、BLE 预编译库、WCH SDK、其它 EVT 示例和归档不在本索引中。
- 最近子工程提交：`4d101ae6459f89fa7f26f0d437d1d86a8be7a369`；当前已索引文件内容集合 SHA-256：`e74bb9c988f158851c23cbd1a0ed4531acf026449b8748fc5df01b0002adc2c1`。
- 同步结束：2026-11-10 07:34:29 UTC；状态检查结束：07:34:30 UTC；查询时间见 `roots[name=gateway].queries[0]`。
- 验证：34 文件、508 节点、1066 边；`decode_frame10` 返回 `APP/feistel_al.c:119` 当前源码，识别调用方 `APP/app_um2006A.c`。

最小查询：

```powershell
codegraph explore -p "D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2" --max-files 1 "decode_frame10"
```

## Android（android）

- projectPath：`D:\mypro\beiwo2\AIP\dengbei_care`
- indexPath：`D:\mypro\beiwo2\AIP\dengbei_care\.codegraph`
- 覆盖：本地 Gradle 工程可解析的 Kotlin/Java、Manifest、properties 等；含 `app/src/main/` 业务代码、宿主及设备测试、内嵌 Paho 客户端。测试目录和 `app/src/main/java/org/eclipse/paho/` 降低排序优先级。
- 排除：`iOS开发资料/`、`.idea/`、`.gradle/`、`build/`、`app/build/`；Android SDK、外部 Gradle 依赖、APK 和 iOS 资料不作为本轮代码根。
- 最近子工程提交：`4d101ae6459f89fa7f26f0d437d1d86a8be7a369`；当前已索引文件内容集合 SHA-256：`0d7523b963708a23bd3a01142a1ab65607b67179e948e0b196d8c4187a4641c1`。
- 同步结束：2026-11-10 07:34:31 UTC；状态检查结束：07:34:31 UTC；查询时间见 `roots[name=android].queries[0]`。
- 验证：88 文件、2396 节点、4770 边；`GatewayFrameCodec SensorCadence CloudUploadRepository` 返回当前源码，并识别三者在 `MqtttService.kt` 的调用方。相应源码位于 `protocol/GatewayFrameCodec.kt:16`、`telemetry/SensorCadence.kt:11`、`cloud/CloudUploadRepository.kt:28`，目录前缀均为 `app/src/main/java/com/jinyuni/dengbei_care/`。

最小查询：

```powershell
codegraph explore -p "D:\mypro\beiwo2\AIP\dengbei_care" --max-files 1 "GatewayFrameCodec"
```

## 状态、限制与后续使用

三个工程本轮 sync/status/explore/files 命令均退出 0，stderr 为空。同步均返回 Already up to date；同步后均为 `index.state=complete`、`pendingRefs=0`、added/modified/removed 均为 0、`worktreeMismatch=null`、`reindexRecommended=false`。旧 `lastIndexed` 时间不会因无变更同步而刷新，本次就绪结论依据实际同步和查询。

必需且未验证的代码根：无。未覆盖的范围为产品容器内其它厂商 SDK、示例、归档、生成目录、iOS 和硬件资料。当前资源入口未给出独立本地后端源码根，本轮未声称建立服务端索引，也未访问部署服务。代码图只覆盖可解析的静态符号和关系；宏、条件编译、外部库、无线/BLE/网络跨进程关系需由相应工程角色核对，图查询成功不代表编译、设备或业务测试通过。

交回实现角色核对的已观察位置：`SensorCadence.kt:7` 注释仍描述“下降 >0.9℃”，与当前需求“升温”及 `sensor_decide_report` 当前源码不同；这里只提供定位证据，不判定业务验收、不改实现。

专业消费者必须显式指定上列对应 projectPath；MCP 可使用 `codegraph_explore({projectPath: "对应绝对路径", query: "示例符号", maxFiles: 1})`。直接使用其返回源码与调用关系，不重复读取已返回文件。变更、切换版本或修改排除规则后依次执行：

```powershell
codegraph sync "<projectPath>"
codegraph status --json "<projectPath>"
codegraph explore -p "<projectPath>" --max-files 1 "<本次相关符号>"
```

确认同步、状态及当前源码查询均成功后才能沿用索引。需要查看实际覆盖文件时使用 `codegraph files -p "<projectPath>" --format flat --no-metadata`。三个 `.codegraph/` 已被现有 `.gitignore` 忽略，Git 跟踪文件检查为空；数据库留在本机，不提交大型数据库。
