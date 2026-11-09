# 当前系统需求与约束分析

负责角色：系统架构。检查日期：2026-11-09（Asia/Shanghai）。

## 1. 判断与证据边界

分析就绪，可继续现有候选的增量实现、标定与分阶段验证；产品整体尚未通过。接手 HEAD 为 `6575d220caf2ca5da0466ef4f0a941eb4954d6d2`，首次 `git status --porcelain` 为空。已读共享清单、最近五条提交及最近提交统计、当前需求、上游 CodeGraph 交接、接口契约、实现和最新失败证据。当前调用没有附加失败历史，但仓库中的 EV-028 TEST_FAIL、EV-029 归因和旧失败均必须保留。

本报告取代 `system_analysis.md` 的当前分析入口；旧报告保留在原位置作为历史。旧报告曾记录降温判据、UART 无界等待和 GXHT40 失败，这些不能直接沿用为当前缺陷。当前源码已改为升温判据、有界 UART 等待，并有后续有效实板采样证据。报告更新不修复任何被引用产品对象。

本次只进行静态代码、版本和既有证据核对，没有构建、运行测试、连接设备、安装、复位、烧录、访问远端服务或修改产品源码。局部历史 PASS 不升级为本次测试通过；原始捕获重读只确认已有字节，不是新采集。后附摘要仅标识检查时对象，不证明设备和远端从未变化。

## 2. 需求、完成成果与剩余工作

权威产品来源：根目录 `readme.txt` 八点及补充条件1–4。现有设计解释保留来源，不冒充用户逐字要求。

| 需求 | 当前存在且可复用的成果 | 尚需工作及边界 |
|---|---|---|
| R1 GXHT40 温湿度采集 | 驱动、双 CRC、有效域、重试和诊断；EV-028 7条、EV-029 2条原始 S 均 q=1 | 当前板在线和镜像未查；保留早期失败、后续修复与局部成功，不重复制作驱动或断言永久稳定 |
| R2 使用新增光敏分压，硬件不改 | 既有 PB04 ADC/PB05 供电采样及结构化结果；旧实板 o=8 | 实际板件、分压与供电条件绑定；不重做硬件 |
| R3 每3分钟采集 | RTC 1分钟×3拍设计、串口 k 步长3及既有 Android 节拍 | 名义180s与实测周期分开记录；非每3分钟必有上报，不补点 |
| R4 升温严格>0.9℃且无光，或温度严格>35℃ | 当前 fw_core.c:110–123 已实现 T−P>9 或 T>350；35℃未整体排除 | 当前候选边界与真实触发/静默待验，不再安排修正已消除的方向错误 |
| R5 复用已部署服务器 | 迁移资料、云配置、上传待发箱；用户确认服务已部署 | 当前服务版本、接口、身份、持久化和回读未验；先确认复用，不默认重建/重部署 |
| R6 同步Android；iOS不改；取消sensor光通信/霍尔 | 统一解码、按ID归因、SQLite/待发箱/既有告警；历史188单测；固件退役记录 | Android四项增量尚不能认定实现完成；真机升级/数据保留、权限、发现绑定、多设备和后台链路待验；退役工程排除本次未全面重审 |
| R7 全暗基准1/3 | light_is_dark:94–100 为有效时 3×mean>=C_dark；未标定不判暗 | 配置 REF_CODE=0/CALIBRATED=0，全暗标定未做；用户可配合遮光是条件意愿，不是测量证据 |
| R8 UART1初始化与打印 | trace_uart_open:115–154；9600；TXE/TXBUSY guard；BOOT/VDD/IOTEST/IOSIG/BUS及S/P诊断资产 | 完整上电五行和uid缺失；本次未测UART故障和当前输出；不为取得日志反复改固件 |

固定边界：CR2032焊接供电保持，不改硬件、iOS、既有网关固件或配置。既有网关是兼容与联调依赖，不新增网关改造任务。不新增心跳、RF序号、采样时间字段、服务协议或告警算法。App既有下降报警与sensor升温上报是不同职责。

## 3. 组件及接口约束

数据流：GXHT40/光敏 → CW32采样及判定 → 433MHz → 既有CH592接收/缓存/聚合 → BLE → Android解码/归因/本地/告警/待发箱 → 既有MQTT与HTTP服务 → 持久化及回读。此为依赖关系，不证明每一段已运行，也不推断CH592具有MQTT实现。

| 边界 | 当前约定与来源 | 限制、未知及验收 |
|---|---|---|
| GXHT40→MCU | IC-002 §3、现有驱动：0x44/0x45，0xFD，6B双CRC；T有符号x10、RH x10 | 图索引未覆盖外层SDK/全部构建依赖；实际器件时序/电气不能由代码或串口完全证明 |
| MCU→网关 | IC-002 §4：10B加密帧；明文ID4/T s16LE/RH u16LE/CRC16LE；现有Feistel | 本次图定位decode_frame10；无ACK/序号/时间，RF本地成功不等于网关收到或端到端恰好一次 |
| 网关→Android | IC-002 §5：16B头+N×16B，ID4/RH u16BE@4/T s16BE@6；AES/ECB/NoPadding；FFE0/FFE2 | 图定位build_device_block及App统一解码入口；实物名称/地址/镜像、GATT/MTU、多设备容量未核验 |
| Android→本地 | GatewayFrameCodec/ReadingAttribution、本地库和待发箱既有实现 | 按ID归因，负温只缩放一次；拒绝坏帧/错数量；升级保留数据未真机验；接收时间不是物理采样时间 |
| Android→平台 | IC-002 §6：既有topic；MQTT epoch:temp列表:hum列表；HTTP POST /upload_data，phone/mac/readings，读数devId/time/temperature/humidity | MQTT无逐读数ID，顺序和绑定上下文必须一致；数量相同仍可错配；BLE完整批次转发/零值需独立验证 |
| HTTP→存储/回读 | CloudUploadRepository持久待发/退避，成功后ack；迁移资料声明UNIQUE(dev_id,time)/INSERT IGNORE | 客户端2xx不证明落库；秒级幂等不能识别跨秒缓存复读，亦可能合并同秒真实读数；服务端版本及回读入口未知 |

温度/湿度失败不造零，不推进前值；上次指上次有效采集而非上次上报；光照失败不触发升温分支，超温分支仍独立；首样本仅可独立超温；重启RAM前值丢失。这些来自既有IC-002及当前measure.c:460–532，不是新增需求。待发冻结样本与新采样并存，不能把本周期值替换尚未发出的旧样本；重试耗尽行为和拥塞丢弃由实现/测试据现有机制核验，不扩大为新队列产品需求。

规范向量（单位℃）：30→30.9暗不报；30→31暗报；30→31明或光无效不报；31→30暗不报；34→35暗可由升温分支报；35无前值不报；35.1有效温湿度在明/光无效/无前值时可报。温湿度无效均不生成有效遥测。必须区分判定r、冻结P、本地发送s、网关实收及App显示/告警。

全暗定义沿用IC-002的原始ADC码解释：阈值ceil(C_dark/3)，等于算暗；不是lux比例。标定必须记录实际板件/供电/ADC配置/建立时间/遮光方式/原始分布/C_dark及候选版本；不用满量程、亮态或旧延时版本光照读数替代。温湿度有效但无标定，只能验证超温分支，不能宣布无光分支通过。

## 4. 当前候选与原始证据登记

所有位置相对仓库 `D:\mypro\beiwo2\AIP`，检查时点及文件SHA-256见附录。Git tree标识接手HEAD完整已跟踪对象，不包含忽略的产物。对远端和板上身份未知的对象不提供伪造版本。版本变化后须重新出具当前确认记录，联调和总测重新对应候选。

| 对象/位置 | 不可变版本或身份 | 已有成果/原始证据 | 未核验项 |
|---|---|---|---|
| sensor工程 `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output` | tree `09bef6581f39ca50ba2ecfbaa5c77ef9accd68a5` @ 接手HEAD；本次git diff bee3d8f..HEAD -- USER为空 | FWI-002 rev6.4；EV-028独立重建AXF MD5 `a3b08774e764f174eac318f6771d0c42`、HEX `546ce20a718fa69b40177cce0145f4b5`；`evidence/ev028/flash_1.log`、`flash_2.log`及cap01..05原始字节 | 本次未重建/核验现存产物或当前板上镜像；tree含测试及工程资料，不等于下载镜像 |
| 网关 `CH592EVT/EVT/EXAM/BLE/beiwov2` | tree `22c8a193daffea87408c971a53db566633f7c6ea` | 图查询decode_frame10/build_device_block；IC-002兼容布局 | 未核验实物身份、镜像、RF/BLE；不重新实现或刷写 |
| Android `dengbei_care` | tree `3c9feeb9ce399b3ba5ac359b31090af583ce4781` | 既有实现记录、software_test.md末尾ST-014R，188/0历史结果；版本9/1.8记录 | 当前APK摘要/签名、真机安装升级、真实权限/连接/云端未验；旧PASS不覆盖新四项增量 |
| EV-028实板原始 `evidence/ev028/cap01_k3_postflash.bin`、`cap02_k6_k9.bin`、`cap03_reset_bus_k0_k3.bin`、`cap04_k9_after_reset.bin`、`cap05_k18.bin` | 原始字节附录摘要；历史2026-11-08，COM43；受测bee3d8f | 7条S q=1/0G；cap03首样本H=0 p=0，后条p=263=t(k0)；BUS ack44；capture_analysis.txt及tool_results.txt | uid/完整BOOT等上电段未取得；静默r/s/y=0不证明真实触发或传输 |
| EV-029 `evidence/ev029/serial_capture_1204Z.bin` | 原始158B；MD5 `6522407d2058a1f3ced6edef9662c314`；历史2026-11-08 12:04:51–12:09:51UTC | 本次重读：k27 t272、k30 p272 t277；2条q1，V0/D0/r0/s0/y0；capture_analysis.txt/tool_results.txt | 会话连续性是旧证据的关联判断，不能充当当前镜像的不可变身份认证 |
| D2/E1历史修复证据 `evidence/test.md`内EV-024/026、`evidence/ev026/raw_i2c_capture.sr`、decoded_i2c.txt/capture_metadata.json/capture_analysis.txt | 旧候选与采样参数按原报告；原始对象附录摘要 | 慢I2C后27/27成功；delay恢复后STOP→读START32.525ms、双CRC；失败采样点后移/单独延长配置等待曾无效 | 不是本次硬件复验；数字采样不能证明电平/供电；不重做已否定修复，不断言根因已永久排除 |
| 接口/设计 `artifacts/interface_contract.md` IC-002 v3；android_architecture.md/firmware_design.md | 附录摘要 | 判据/布局/参数化标定和设计可复用 | IC-002包含旧实施交接：D-01、结构化光照等部分已完成，不能当现行缺陷清单 |
| 已部署服务；本地 `服务器迁移记录.md` | 本地摘要见附录；远端版本/可执行核验入口未确认 | 用户确认部署，资料载接口与持久化声明 | 本地文档摘要不认证远端版本；未连接，不从地址/凭据推导管理或DB授权 |
| 上游索引 `artifacts/codegraph_current_index.md`、`evidence/codegraph_current_validation.json` | 接手HEAD及附录摘要；上游2026-11-09检查记录 | 三子工程已同步/查询；本次MCP按精确projectPath查询当前磁盘源码 | 容器无根索引，图不跨RF/BLE/HTTP证明集成；无独立服务端源码根，不臆造服务端索引 |

保留证据演进：早期GXHT40失败 → D2慢时钟成功 → E1恢复delay等待 → F-1前值观测修复 → EV-028/029局部成功、整体仍TEST_FAIL。旧证据中“排除器件故障/排除新驱动缺陷”等表述只按其观测窗口理解；无失败签名不等于证明所有潜在原因均已排除。`failure_resolution.md`的历史拒接/流程意见不是本次授权、不可修复结论或新流程节点。

## 5. 时序、资源、安全与环境限制

当前配置实读：I2C_DELAY_LOOPS=300，GXHT40_MEASURE_WAIT_MS=20，LIGHT_ADC_SAMPLES=8，EOC_GUARD=100000，UART_TX_GUARD=200000，UART9600。代码图对sensor_config.h未返回相关源码，因此只对该已知文件定点读取宏；没有据此重建索引。delay循环历史实测约1.63倍名义等待，当前作为下限等待保留；这不是重新校准的毫秒时基。3个RTC分钟拍不保证精确180.000s，旧实测周期/首间隔偏差按原证据保留。Android每次连接18s、最多5次、间隔2s，单轮约98s预算；不能以采样常量承诺端到端延时。

EV-028历史构建Code25964/RO656/RW116/ZI1676，即Flash26620B、静态RAM1792B；按历史64KB/4KB工程有余量，但栈峰值、CR2032续航、射频峰值电流、睡眠功耗、误报率与长时稳定性未在本次测量，不新增量化指标。

唯一环境和授权依据是本次Unified汇总，以下是其引用而非第二份配置：

| 入口/条件 | 当前状态及限制 |
|---|---|
| COM43/9600 8N1 | 配置启用，本能力缺serial_capture；COM26非传感器串口，旧COM42不沿用；未打开 |
| Keil/CW-DAPLink | 缺mdk_flash及flash_device；未烧录/复位；pyocd只是readme允许评估的建议，未登记授权入口，不自动执行 |
| Redmi8A，12f046690506 | 只读ADB configured_unverified；交互缺android_test；正式分析不主动操作硬件，本次未查询手机 |
| 网关 | 通过App正常发现/配对确认身份；无独立管理/刷写授权，不从源码固定MAC认定实物 |
| FX2 conn=2.32，D0=SCL/D1=SDA/GND | 本次当前登记值；缺sigrok_capture/operate_test_equipment，本次未枚举；EV-028/029失败地址2.27为历史，不能报告2.32也失败或已修好 |
| 已部署服务 | 管理/重部署/DB直连及当前功能核验入口未确认；迁移资料不授权网络访问；保留总测要求 |
| 环境修复 | 当前关闭；不改权限、系统包、驱动、网络、网关配置，不换通道绕过缺项 |

实际I2C固件/旧网表为PA03/PA04，仪器登记描述含PB0/PB1映射，两者的测试点对应仍待接线责任方核实；不据描述自行修改引脚。SDA下降沿触发不等于限定SCL高的START识别，采样率不是总线速度。数字捕获不证明电平兼容。真实遮光/温度刺激需明确操作窗口；readme“现在夜间”不自动延续为本次时点事实。

安全持续限制：报告不复制密钥/凭据；HTTP明文及TLS主机名校验的旧描述只作现状风险，未经本次安全验证，不擅自重设协议。保护既有本地记录和待发箱；重试保留原time，设备ID/网关身份/topic上下文切换需防错配。无外部安全标准研究或认证结论。不得终止无关占用进程；后续设备操作有界等待并保存原始字节/波形及参数。

## 6. 可继续范围、必要决定与总测目标

| 工作 | 当前可以继续的范围 | 完成条件/责任边界 |
|---|---|---|
| Android四项增量 | 复用android_tasks.yaml/android_architecture.md既有设计：权限请求与状态、正常发现绑定、完整BLE批次转发/零值、升温文案 | Android实施后由软件测试核对当前源码/APK/真机；当前checkBluetoothPermission:552–557仍API<31检查定位，connect:582等可静默返回，不认定四项已做；本报告不重复制作这些任务或以平台规则外推替代验证 |
| 全暗标定 | 获准责任方先测当前光敏原始码，与用户约定遮光/恢复窗口；真全暗数据绑定板件及配置 | 固件据实测回填C_dark并重建，随后明/暗/无效及1/3边界复验；用户已愿配合，不再把“不能提供遮光”作为既定事实 |
| 上电日志 | 在获准探索/测试职责内评估已登记工具是否支持先开串口后复位；需新入口时明确配置/授权缺项 | 当前无可执行pyocd入口；按readme暂记未执行及原因并继续其他有条件测试，不能标通过，不为捕获反复改固件 |
| 固件阶段与整体验收 | 保留EV-028完整交付TEST_FAIL；固件阶段分别报告已测和未测，跨链路后置 | readme补充3/4与TD-002 complete_delivery退出规则存在阶段适用冲突，需验收责任方明确分阶段适用和总测汇合；本分析不改断言或降低最终要求 |
| 服务复用与核验 | 先确认既有部署和业务核验入口、认证/幂等及可回读机制 | 获准责任方记录实际版本/上传/持久化/回读；需要管理变更时说明依据及授权缺项；不排除服务器，不默认重部署 |
| 真实集成与候选身份 | 固件镜像摘要、板件/网关实物、APK摘要/签名和服务版本对应同一验证窗口 | 从真实采样到云端回读按同笔ID/T/RH/时间语义比对；候选任一变化重新关联记录与必要回归 |
| 测试资产维护 | 复用现有harness和计划；仅修陈旧host_rf_report_verify_ev.c/host_light_verify_ev.c等资产及未覆盖层 | 嵌入式测试负责，旧API编译失败不计入PASS计数；不修改产品源码或降低断言来通过 |

必要决定是确定阶段验收适用范围、合法采集编排和服务核验入口；这些不妨碍当前静态分析交付。没有必须由用户立即回答才能完成本报告的问题，不在分析阶段申请硬件操作或停住其他有条件工作。

总体最终目标保持：真实有效温湿度/光照 → 条件上报与正确静默 → RF实收 → 网关缓存/BLE → 手机逐ID显示/既有告警/本地落库 → MQTT/HTTP → 服务持久化/回读一致。覆盖升温及35℃严格边界、明/暗/无效、首笔/失败/重启、冻结重试、多设备/零值/顺序、缓存复读、网络断续和后台节拍。各组件单项完成仍不替代此链路；无光原因不能由下游帧反推，上报成功不等于报警正确。

本次已完成需求映射、当前与历史分离、边界/接口/资源/安全/时序分析、可复用候选确认及依赖缺项记录。分析本身无未交付项；产品待办不因分析完成而宣称完成。本次没有后台任务或受管理长时作业；工具均已返回，未自行提交/推送。

## 附录：本次确认的文件身份

以下摘要由本机对指定对象计算。原始证据、旧报告及配置资料各自只证明其内容，设备/远端不可变版本未核验项仍有效。

检查时点：2026-11-09T13:09:01.9410282+08:00

| 文件 | SHA-256 |
|---|---|
| readme.txt | c6f19954d54c29136db95d75dcc28816d84d5ce57ea449714a0c58ba80a1e6dd |
| artifacts/system_analysis.md | bd05fa40e8c1520d0376bc95cc6d9bacdaa9f25ccf00bf287d490ed47929f7e4 |
| artifacts/interface_contract.md | 6af5f0a65c819033390e94099c77ede83af53b6a2750edf55debd9fe42d2ff22 |
| artifacts/android_architecture.md | 09784fd5fe59118436c04db9c23040e033ee26ea2e9f701df72107c94910e7af |
| artifacts/android_tasks.yaml | 20882c891c27b6c60c88b92c7130e5f2b690b3923725683d5890893d04231449 |
| artifacts/firmware_design.md | c6e5e73d501f58356af11f7f87ba1148cfdcbdbbcd6d51ffa1e6f4048dfb826b |
| artifacts/firmware_implementation.md | b7e5733f85157c5b6f115e281709b62a28b7ac5dd7f364877d872b50e568d757 |
| artifacts/failure_resolution.md | 406a18e366f14f35f29f5cf01917a30bfcf90ee5b98e46e2cdd2072186d59535 |
| artifacts/codegraph_current_index.md | c8151ec82d9650ace4a49836d471bc5ff87870495bb80a71fdb854351de473d8 |
| evidence/codegraph_current_validation.json | 8f50ccca2115b108db655ffb322f26744b0e5df4b8ba3468c834161a3edeacca |
| evidence/test.md | 11fa5393ad5f248e7f9db874ce1a11bfb242793ae9147e02e5c7c22956879b38 |
| evidence/triage.md | 241e7bea5415f64425e2935f20cfbcf28de52a05b2a56cb8395ebf456aac0788 |
| evidence/software_test.md | 8fc9fffbf27bbb4f0518f987cd01a732febc9d6ef49ed40b604e4afcbd224671 |
| evidence/ev028/cap01_k3_postflash.bin | d2fe1166ae5e673743b9f18531343f139f4dfa2c800aeb186ee4406a52cd6f7b |
| evidence/ev028/cap02_k6_k9.bin | 49cc0ea20dc930e850932610bf77667a3ad12647240ea4c2746e9a7e6bcbd05d |
| evidence/ev028/cap03_reset_bus_k0_k3.bin | b29de14602846c306c17027773aec0d20baa7fa30cd1e916ede7e37ee68d8fe6 |
| evidence/ev028/cap04_k9_after_reset.bin | d443f37ea2cf4b34245b592fc4ec8419a289f3f259845e97471d83fcf40ae05f |
| evidence/ev028/cap05_k18.bin | c2c829c48e6b80ecc861bf21d43439efc1748b0e55e495d405fb6c2cadd788d8 |
| evidence/ev028/capture_analysis.txt | 4dd73d5eab40930a9512fd33628217fc92ca229a5eac46885e2e0e1963089a0f |
| evidence/ev028/tool_results.txt | 64298760f5723ec25c3ac963ff3a5381d0111dc1469aaf4dde3759134be196bc |
| evidence/ev028/flash_1.log | bba7a868a069e9ceab2e75d450f7bc0d866abf2dd94e028bc2ffa91b6741c079 |
| evidence/ev028/flash_2.log | bc37aa495b7aaa6461f1f2d7e84a4ecec4f9af73fbce680757f00255b6a2f635 |
| evidence/ev029/serial_capture_1204Z.bin | 3d330daad35782dd0be771965f3909720b8a5ecf632f706f83c886154dfd032e |
| evidence/ev029/capture_analysis.txt | 2c70da8f6d551591dc0270b2f6628fee7ac099f572c440e33bad7f588a8e85cf |
| evidence/ev029/tool_results.txt | 5baa8fd733fca7075971b668c5cf22e343cba64640212261b1b27856de5ef736 |
| evidence/ev026/raw_i2c_capture.sr | d71f8e82e6048c29764420460bb3fa0d35c2e0f49e06b2bf980a51f822eb3ef0 |
| evidence/ev026/decoded_i2c.txt | d2aa9965b3b2afdd4c6ea599a1cf61cf304d2403b985e2a1febd8d0b0d04816e |
| evidence/ev026/capture_metadata.json | c5f9859f1ca82687164535ac8de984d1137b61df933d04d6be636da07d8eb98c |
| evidence/ev026/capture_analysis.txt | 6a9fdb70f2f4e0cd6ff37dab256cba96b0c9d785e548fdcdafdeec966f04e37a |
| 服务器迁移记录.md | e883e4df73b109dd7ac08c862e350ed9b11372a9e073930dd7bcf9e4749811ed |
| CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/inc/sensor_config.h | 660ffd82989815e1fb31696b5d1905bdc6e3b719db543777bbcc404d41ada3ed |
