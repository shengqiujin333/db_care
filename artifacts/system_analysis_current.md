# 当前系统需求与约束分析

负责角色：系统架构。检查日期：2026-11-10（Asia/Shanghai）。

## 1. 分析结论与证据范围

分析就绪；可以继续基于现有成果的增量设计、实现和分阶段验证。产品整体尚未验收通过。本报告只完成系统需求与约束分析，不替代接口设计修订、产品实现、测试执行或用户的产品决定。

接手版本为 `4423f850273b4365f9205be0f5f883fc68e0ed5e`，初始工作区干净。已读取共享清单、最近五次提交、上游索引与相关既有报告，检查最新需求提交 `e787681c348f5a6aeb30a55b8056e8b4c6a6f6bd` 的 README 差异。旧分析保留在 Git 历史；本文件为当前入口。

本次未构建、运行产品测试、联网、访问手机或仪器、烧录、复位或修改产品源码。历史报告中的“本轮”均指原报告自己的检查窗口，不指本次分析。报告文件摘要只标识本次确认的本地记录，不能证明远端、板上或 APK 从未变化。

## 2. 当前需求与历史资料冲突

权威来源为本次用户目标及当前 `readme.txt`。最新 README 的明确变化：删除全暗基准 1/3 的表述和可配合遮光窗口；当前情况为有光；新增不能调温、不能改变光强、不能测功耗，且不得安排相关硬件测试设计与执行。

| 事项 | 当前有效约束 | 对旧成果的处理 |
|---|---|---|
| 光照判据 | 保留“无光”业务门，但当前输入没有规定数值阈值或替代算法 | 1/3 是旧契约和当前代码事实，不能继续声称是当前用户要求；删除文字不自动批准任何新阈值 |
| 光照现场 | 当前用户声明有光，不是照度测量 | 历史“无光/直射”仅对应各自窗口；不能覆盖当前状态 |
| 温度、光强、功耗测试 | 当前不安排相关硬件测试设计或执行 | 旧 T5 双态标定、调温触发、功耗任务不作为本阶段待执行安排；旧未执行记录和 FAIL 保留，不能改为 PASS |
| 上电日志 | 仅在有效权限和工具允许时评估先捕获再 reset；无法完成则记未执行与原因并继续其他可测项 | 不为日志反复修改固件；旧完整交付门禁需由负责设计/测试的角色按当前需求核对阶段适用性 |
| 跨链路 | 网关与手机安排在后续联调 | 不以尚未开始的手机联调反复退回固件实现；最终整链目标仍未完成 |
| 固定修改边界 | 只改传感器固件和 Android；不改硬件、iOS；复用既有网关和已部署服务 | 不新增刷网关、服务重部署、数据库迁移或新协议任务 |

旧 `interface_contract.md` §3、固件需求/设计/任务及测试设计仍以 1/3、全暗标定等为前提；它们是需要责任角色同步的历史设计，不凌驾于新需求。本报告只记录冲突，不代替其负责角色修改文件、降低断言或宣布旧失败已修复。清单中 readme 的旧 1/3 摘要本次同步更正。

## 3. 功能需求、已有成果和剩余工作

| 编号 | 当前需求 | 已存在的可复用成果 | 仍需工作与限制 |
|---|---|---|---|
| R1 | GXHT40 采集温湿度 | 驱动、CRC、整数换算、重试、诊断；EV-030 原报告 5 个有效周期及 I²C 双 CRC，EV-031 追加 2 周期 | 当前板上镜像与在线未重新确认；历史局部成功不是长期稳定性结论 |
| R2 | 使用已有光敏分压 | PB04/AIN11 采样、PB05 供电、结构化 valid/mean/dark；BOM R3=5M、J4=GL4539_2 | 无光判据数值未确定；当前不得安排改变光强的硬件测试，不能直接填入估算标定值 |
| R3 | 每 3 分钟采集 | SENSOR_SAMPLE_TICKS=3；Android SensorCadence=180 s；历史 k 步长 3 | 采样机会不等于固定上报；不能补造缺失样本或承诺每 180 s 有云数据 |
| R4 | 本次比上次升温 >0.9℃ 且无光，或温度 >35℃ 才上报 | sensor_decide_report 当前用本次减前次、严格 >9 / >350，冻结待发及前值快照已有 | 无光门未验收；真实触发链未完成；本阶段不安排调温/调光硬件实验。不能再次把已修升温方向列作待实现 |
| R5 | 同步 Android，复用 broker/服务器 | 统一 BLE 解码、按 ID 归因、本地库、告警、待发箱和 HTTP/MQTT 接线已有；历史 14 项复验及 188 项宿主通过记录 | AA-004 六项增量仍有待办；真机、实际网关、真实服务写入/回读未验 |
| R6 | 取消 sensor 光通信及霍尔 | fw_core 退役说明、既有工程清单与 EV-030 静态核查记录 | 本次没有重新全面审计每个编译分支；保留正常光敏 ADC，不混同光通信 |
| R7 | 当前有光 | 用户当前声明；历史 ADC 明暗变化记录可作分析输入 | 没有 lux 测量，不能据单一 ADC 码反推当前环境或器件完整健康 |
| R8 | UART1 初始化和打印 | debug_trace、UART1/9600/PA06 TX/PA05 RX 配置、有界轮询及 BOOT/S 等输出路径 | 历史完整 BOOT/VDD/IOTEST/IOSIG 与 uid 未取得；按用户安排记未执行，不能写为通过 |

既有成果不重复制作：GXHT40 驱动、纯逻辑判定、协议编解码、本地库和待发箱继续复用。分析完成不代表这些组件均已在当前真实环境通过。

## 4. 组件边界、接口和时序

系统路径：GXHT40/光敏 → CW32 采集与资格判定 → 433 MHz → 既有 CH592 接收与缓存 → BLE → Android 归因/显示/既有告警/本地存储/平台适配 → MQTT/HTTP 既有服务 → 持久化与回读。该路径描述接口依赖，不证明整链已运行，也不表示网关直接连接 MQTT。

| 接口 | 已有兼容约束 | 未知或持续限制 |
|---|---|---|
| GXHT40→MCU | 现有驱动 0x44/0x45 探测、0xFD、6 B 双 CRC；当前等待 20 ms | 契约旧 10 ms 描述已陈旧；器件 PDF 本次未独立重审；数字波形不证明电气兼容 |
| MCU→网关 | 现有 Feistel 10 B；明文 ID4、T s16LE、RH u16LE、CRC16LE | 无 ACK/序号/采样时间；本地发送完成不等于接收成功；不扩字段、心跳、密钥或帧版本 |
| 网关→Android | FFE0/FFE2；AES/ECB/NoPadding；16 B 头含网关 ID6 与 N@6，N×16 B 设备块；前 8 B=ID4、RH u16BE@4、T s16BE@6 | 实物名称、地址、镜像、实际 GATT/MTU、多设备窗口未核验；负温保留符号、单位仅除 10 一次 |
| Android→本地 | 无效帧不造读数；按 ID；接收 Unix 秒入库和待发箱 | 接收时刻不是采样时刻；缓存复读可能重复观测，同秒幂等键可能合并；不擅自按值去重 |
| Android→MQTT | 现有 topic 和 epoch:temps_csv:humis_csv 格式 | 无逐项 ID，绑定顺序必须一致；现有最后一笔覆盖及格式问题见 AA-004，尚待修复 |
| Android→HTTP | POST /upload_data；phone/mac/readings，逐笔 devId/time/temperature/humidity | 当前代码任意 2xx 即 ack；真实受理、部分成功、幂等、持久化尚未证实；不猜测鉴权头 |

当前规范中的业务组合为有效温湿度且 `((have_prev && current_x10-prev_x10>9 && light_valid && dark) || current_x10>350)`。此处 valid、上次有效采集、首笔和失败处理属于现有生命周期解释，不能冒称全部为用户原文。差值用 int32；采样失败不造零、不推进前值；有效采样即使未报也推进前值；首笔无前值仍可超温上报。光照失败仅关闭升温分支。重启后 RAM 前值消失。冻结待发与新采样分离，重试不偷换读数。

严格边界应保持：30→30.9 暗不报，30→31 暗可报、明不报；35.0℃仅不触发超温分支，34→35 暗仍可能满足升温分支；35.1℃有效温湿度可独立上报。以上可用现有宿主纯逻辑向量分析/回归，不授权相关硬件刺激。本次未运行这些测试。传感器上报资格与 App 既有下降报警是不同职责，不因升温上报擅自重写告警算法。

## 5. 无光判据：已知、未知和必要决定

当前代码 `fw_core.c:94` 为 `valid && 3*mean>=C_dark`；`sensor_config.h` 仍为 REF_CODE=0、CALIBRATED=0。因此当前样本光照 valid=false，不能证明无光，常温升温分支被抑制；有效温湿度 >35℃分支保留。不能把未标定时 V=0/D=0 解释为已辨识“有光”。

既有 FW-EXP-003 rev 6 的原始直射两行及延续一行，本次读到 ADC 均值 3061/3083/3095，均 q=1、V=0/D=0、r=s=y=0。旧 1/3 公式在 C_dark∈1..4095 时门限最高 1365，所以这些已记录受光码对任何合法基准都会判暗。这个算术矛盾是旧公式在已记录条件上的缺陷证据；不是当前实际运行已经误报（实际未标定且未报），也不是所有未来算法或所有光照条件均不可修复。

原报告房间灯码3514与同会话码3750、历史3914–3932及电阻扫描可作参考，但3750窗口未单独确认照明；温度/光强共同变化，反推阻值和温漂属于模型计算，不能当器件实测参数。码随照明变化不能单独排除错件、漏电、接触、遮挡等原因；本分析不承接旧报告中过度确定的硬件健康推断。

必要决定是“无光”的可验收定义和可用参数来源。当前删除 1/3 并未给出替代值；不得擅自采用中点、旧350/250、滞回/温补、取消光门或修改分压。可由后续负责设计的角色基于当前需求、已有原始数据和实际资料提出可审查判据，明确适用范围及未核验项；涉及产品语义的选择需有明确依据。没有新输入前该项保持未决，不阻止其他独立工作，不请求用户在本轮改变光强，也不把可修复问题判为不可修复。

## 6. 候选版本、检查时点和证据边界

本次本地检查窗口约为 2026-11-10 15:40–15:46 +08:00；准确摘要计算时点见附录。以下实际位置相对仓库根，目录身份采用当前 HEAD 下不可变 Git tree，证据文件摘要见附录。

| 对象 | 不可变身份/实际位置 | 已有原始证据与本次确认 | 未核验项 |
|---|---|---|---|
| sensor 源码候选 | CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output；tree `09bef6581f39ca50ba2ecfbaa5c77ef9accd68a5` | 当前目录相对 f978a01 的 git diff --stat 为空；当前图查询判据/采样/UART 源码 | 本次未构建、下载、读回；本地 AXF/HEX 未计算新摘要 |
| EV-030 受测镜像 | 原报告 f978a01；AXF MD5 `a3b08774e764f174eac318f6771d0c42`；HEX MD5 `546ce20a718fa69b40177cce0145f4b5` | evidence/ev030/tool_results.txt、capture_analysis.txt、4 个 cap、raw_i2c_capture.sr、decoded_i2c.txt；本次读取分析摘录 | 原 TEST_FAIL 保留；0E/1W、下载 Verify OK 及 218+498/0 是历史记录，不是本次复跑 |
| EV-031 追加观测 | evidence/ev031/serial_capture_0852Z.bin；原窗口2026-11-09 08:52:50–08:57:50Z | tool_results.txt：k33/36、q=1；原始文件摘要本次确认 | “同一启动会话”是原记录关联，不认证当前板上镜像 |
| 光敏探索候选记录 | artifacts/firmware_feasibility_current.md；evidence/firmware_exploration_current/ | 01:00:48–01:05:28Z 两行、01:13:11–01:18:12Z 一行原始 bin 本次读取；r3_sweep 输出本次查看 | 旧 WORK_FAILED 保留；未重采、未重跑脚本、未确认板上新版本 |
| 网关源码 | CH592EVT/EVT/EXAM/BLE/beiwov2；tree `22c8a193daffea87408c971a53db566633f7c6ea` | 当前图查询 decode_frame10 校验 CRC、ID/T/RH 映射 | 实物/板上版本、433/BLE 仍未知；不刷网关 |
| Android 源码 | dengbei_care；tree `3c9feeb9ce399b3ba5ac359b31090af583ce4781` | 当前图查询 GatewayFrameCodec、SensorCadence、CloudUploadRepository；ST-014R 历史188/0与14项复验记录 | APK 摘要/签名/安装版本、真机后台及 AA-004 六项增量不由旧 PASS 覆盖 |
| 已部署服务 | 原记录 abr.py 8949 B、mtime 2026-10-22；不可变部署版本/远端文件摘要未知 | BE-EXP-003；evidence/backend_exploration/ssh_readonly_inspection_g002.txt，2026-11-09 23:14:02–23:14:09Z，本次读取本地原始记录 | size/mtime/PID 不等于不可变版本；未新连接；无真实上传/持久化/回读证据 |
| 索引 | artifacts/codegraph_prepare_index.md、evidence/codegraph_prepare_validation.json | 上游基线 e787681；三根增量同步/状态/查询；当前按真实根 MCP 查询 | 根目录无图；图不证明编译、设备或跨进程集成 |

服务原始记录可支持当时 EMQX 5.8.0、unit active、6路由存在、OPTIONS 200、零当前连接；10-22 日志 401/401/200 中最后一次进入 mysql.connect，不证明提交事务或落库。无三次原始请求/响应体，不能断定 401 必为 token、归属或注册校验；即使合法身份仍401也不能仅据此证明需要某种凭证。旧本地桩不代表生产 MySQL。当前先复用，发现具体差异再提出依据与所缺授权，不默认重建或部署。

被引用对象版本变化时，应重新交付对应候选记录，联调/总测重新绑定该版本；不能仅改本报告来宣称修复了源对象。

## 7. 可继续范围、集成缺口和责任

| 事项 | 可继续范围与负责角色 | 完成边界 |
|---|---|---|
| 固件 | 固件工程复用已实现驱动、RTC、UART1、前值及冻结快照；处理当前光门语义未决项 | 只记录现有实板完成与未执行项；不重新安排被禁止的调温、调光、功耗硬件测试 |
| 设计/测试资料 | 各负责设计和测试角色依据最新 README 同步适用范围、阈值来源、阶段划分 | 本次不重写他人方案或测试资产；旧 FAIL、原始证据保持历史含义 |
| Android 六项增量 | Android 工程复用 AA-004：权限策略、正常发现绑定、完整批次/载荷格式、全零占位与合法0.0、失败分类/401可见与手动重试、升温文案 | 既有14项无需重做实现；新改动需对应源码/APK及软件验证；SensorCadence:7 当前仍写下降，CloudUploadRepository 仍返回 Boolean |
| 上电日志 | 获准嵌入式职责评估有界先采后 reset | 当前没有有效 pyocd reset 入口，不执行；EV-031已有 pyocd 0.36枚举记录，不能沿用“全机未安装”的绝对结论；没日志记未执行 |
| 服务复用与后续云验证 | 获准职责核对受理条件、真实请求/响应、同键重试、持久化与回读相关性 | 需隔离测试身份、明确写入/回读授权和数据处理策略；只读 SSH 不授权 POST 或 DB 直连；缺项记录未执行 |
| 后续跨链路联调 | 相关工程/测试角色绑定真实传感器镜像、网关身份、APK和服务候选 | RF实收→BLE完整批次→逐ID显示/本地/既有告警→MQTT/HTTP→持久化及回读仍未闭合；条件受限的触发段保持未执行，不能用模拟替代整体验收 |

专业交接记录不创建节点、不保证后续某能力一定执行，也不宣布其他角色无法修复。当前分析所需信息足以完成，不需要本轮要求人工操作或暂停分析。

## 8. 资源、安全和持续限制

来源为本次 Unified 环境与授权：正式分析不主动操作硬件。COM43/9600 8N1 的串口采集缺 serial_capture；烧录缺 mdk_flash/flash_device；FX2 conn=2.32 缺 sigrok_capture/operate_test_equipment；手机测试缺 android_test；服务只读 SSH 缺 inspect_existing_service_readonly。手机只读 ADB 是 configured_unverified，不等于在线或允许交互。本次全部未使用。环境修复关闭，不安装依赖、改授权文件、驱动、网络或绕道访问。

不使用 COM42、旧 FX2 2.19 或 COM26 代替传感器口；设备重插不猜地址。网关只经 App 正常发现/配对，没有独立管理/刷写权限。环境示例测试点 PB0/PB1 与既有工程 PA03/PA04 的对应仍未核验；BOM U7 CW32L010Y8M6 与配置芯片 CW32L010F8 名称差异保留，不能擅自改引脚/工程。

既有资源记录 Code+RO=26620 B、RW+ZI=1792 B，配置 IROM64KiB/IRAM4KiB；这些是历史构建量，不证明栈峰值、CR2032 峰值电流或续航。本次没有功耗定量结论，也不安排功耗测试。UART轮询 guard 是循环上界，不等于校准墙钟时限；180s采样、BLE轮询、网络退避、报警窗口分别计时，不能推导端到端固定延迟。历史 I²C SCL约1.628kHz与4MS/s仪器采样率不同，数字波形不证明电平兼容。

安全约束：不回显凭据、不将管理口令写入 App；保留原时间和待发数据，重试不丢行；AES/Feistel兼容不等于已证认证、抗重放或安全验收。旧明文HTTP、公网DB监听和共享资源情况只作为已记录风险，不新增无授权的运维改造。不从未知鉴权猜测 Bearer/token。没有授权不部署、不连数据库、不写生产数据。

本次完成：最新需求映射、现有候选身份确认、接口/时序/资源/安全分析、历史证据边界、冲突和剩余工作记录。产品未完成项保持开放。工具调用均已返回，未启动后台任务；不执行 Git 交接提交。

## 附录：本次确认对象 SHA-256

下列摘要由本次检查直接计算，路径相对仓库根。摘要认证本地记录内容，不证明引用结论已复测或远端未变。本报告自身版本由后续仓库提交标识，避免摘要自引用。

计算时点：2026-11-10T15:44:58.6546552+08:00

| 文件 | SHA-256 |
|---|---|
| `readme.txt` | `d9e18992097814d13fbf1cc4f184de9f9d020dea1752fd889a7254559d33bac4` |
| `artifacts/codegraph_prepare_index.md` | `a60184946df93635b9569bcbad762618ac2f769755a5eab2ee4af16b43c4e52c` |
| `evidence/codegraph_prepare_validation.json` | `ec4b40c4aa2829085f2bcb8a5bc2e025e9680f29402b8ae5ecff2582fdc4b239` |
| `artifacts/interface_contract.md` | `6af5f0a65c819033390e94099c77ede83af53b6a2750edf55debd9fe42d2ff22` |
| `artifacts/firmware_feasibility_current.md` | `c34850a7befe120065ef3ce09ff757b3b3f31c8157fd52737ddb5e89881f66bf` |
| `artifacts/firmware_implementation.md` | `d39e754aeb54ec42c17f5503764374ddc5fc3351b597678d661ee68421b7fa9d` |
| `artifacts/firmware_requirements.md` | `ea00a443c6ba329011238061254b5c240c3cadd0f99f527bf20465d6b485cf9a` |
| `artifacts/firmware_tasks.yaml` | `acfe0a4d61c1fc416796097461e36ac1943c93ee8277c050713fe8b83f3f5ec5` |
| `artifacts/test_design.md` | `62afe3e79d2d2ad56e22deb866320cbbac454eaf104c176a8e4b3e819330bf6a` |
| `artifacts/android_architecture.md` | `d565e1413f98824c34e5b6923f943d1b89225f0169e85252969bdc0ee3e4703c` |
| `artifacts/android_tasks.yaml` | `3fa069042fe7b0709e86d6dbbdcb0dbf3af276820c845d4bb6b430113b6a7f02` |
| `artifacts/backend_feasibility_current.md` | `cff541f353df1c61e1cc238f30849d4e20b1c2139699ae2e99b2b645e8aa0df9` |
| `evidence/test.md` | `7354164eedbf089078d7bd8977edf5f389b7ca51dd29ed09d9c8b7fd4a26733e` |
| `evidence/triage.md` | `53e4f89b405be6b051032b3b3cf7cceeecaef7525f83d8325ceb8b0ddfec3d75` |
| `evidence/software_test.md` | `8fc9fffbf27bbb4f0518f987cd01a732febc9d6ef49ed40b604e4afcbd224671` |
| `evidence/ev030/tool_results.txt` | `889de781c7ff1a52d4ee1aca2a02a1ab7c0fe31de38699b9d2a8c0552eed030b` |
| `evidence/ev030/capture_analysis.txt` | `e3da51c24946f6349c34636f3a75aeb5492b9024c21759b42fa8fd6a97077e09` |
| `evidence/ev030/raw_i2c_capture.sr` | `5ecd528e4c3f477070762620e0997f93d63d7f0efccbc66085a8fc03942d1812` |
| `evidence/ev030/decoded_i2c.txt` | `2de0907fcba477b120e5b592e41ff2f85f31ef33b1d5b26222275e75f49254e6` |
| `evidence/ev031/tool_results.txt` | `d3b298eae4c0b73893ec3f63ebb5a76d153dbf88d487a0a9a7b7499aa60277ce` |
| `evidence/ev031/serial_capture_0852Z.bin` | `81ee253db3b6f7c6cc96a474bb95485b0a508c0938227816fda6fb682e46d202` |
| `evidence/firmware_exploration_current/serial_capture_direct_280s.bin` | `31c7a6b370fc6071d2cf2d111b0ad00cb7adffc485a06538520d57ce1c129fcd` |
| `evidence/firmware_exploration_current/serial_capture_after_300s.bin` | `8acdff7c8282040392878d8e465e9df107c2aac344b6163165661081e554fbf8` |
| `evidence/firmware_exploration_current/r3_sweep_verification_20261110.txt` | `5a4bfdd587c0b1d4d08f49d195db9919c32b7172b7249670836727bf33f3ee8b` |
| `evidence/backend_exploration/ssh_readonly_inspection_g002.txt` | `cee54a3b2908755b8e8a599092b8212339aace83b997072c78720a7fd6e7d1d7` |
| `sensor_hardware/MAIN_BOARD.BOM` | `2dfc8ddd0fd74a5bfc4a1071aff00841f20ed15c4dd923ff51ab0b650c0f0f85` |
| `sensor_hardware/pstxnet.dat` | `5e0734950da222860ca55ef1e8dc7f5c6ac9ea25bd51c745d9c9afea7d6f19ac` |
| `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/inc/sensor_config.h` | `660ffd82989815e1fb31696b5d1905bdc6e3b719db543777bbcc404d41ada3ed` |
| `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/fw_core.c` | `780fe5126fe51f6cbf16a8d0b1e53caa14c927199e9a51f8e8e8f7743c515945` |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt` | `c221de8da8d78ecc4b41e341daca69ece754fe8e18fd95920cdb065612b84a1c` |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayFrameCodec.kt` | `76f669ad4ad88a786b3bec95d70bf76c1c8f865ff901938a626a34f84ccde464` |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/telemetry/SensorCadence.kt` | `fe863f0339bcf3f23c42423561557a62e28ae777adb9a9f9901902b72ff27e68` |

### 代码定位方法与限制

三个子工程均先调用现有 CodeGraph MCP（精确 projectPath），查询 sensor_decide_report/light_is_dark/measure_sample、decode_frame10、GatewayFrameCodec/SensorCadence/CloudUploadRepository 及 UART 轨迹。图返回的当前源码直接采用，没有重复读其已展示正文。sensor_config.h 两次精确查询未返回相关代码，改为对该已知头文件的限定读取。Android 宽泛 connect 查询命中过 Paho，未作为产品蓝牙结论；随后指定 MqtttService.kt 及 parsePlainFrame 查询，输出存在裁剪，六项完整缺口依据 AA-004 原报告，未冒称本次逐行独立全审。未建立/升级/同步图。所有调用已返回；无后台作业。
