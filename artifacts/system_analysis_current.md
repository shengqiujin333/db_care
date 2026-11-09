# 当前系统需求与约束分析

负责角色：系统架构。检查日期：2026-11-09（Asia/Shanghai）。

## 1. 判断与证据边界

分析就绪，可继续现有候选的增量实现、标定与分阶段验证；产品整体尚未通过。接手 HEAD 为 `a41a7a74d7294bec0c76cfb80fdb0625992a80ed`，首次 `git status --porcelain` 为空。已读共享清单、最近五条提交及最近提交统计、完整根目录 readme（八点与四项补充）、当前上游 CodeGraph 交接、接口契约、既有分析和最新失败证据。本次新增核对后端探索与服务失败协调记录；EV-028 TEST_FAIL、EV-029 归因及后端历史 WORK_FAILED 均保留，不改写为通过。

本报告取代 `system_analysis.md` 的当前分析入口；旧报告保留在原位置作为历史。旧报告曾记录降温判据、UART 无界等待和 GXHT40 失败，这些不能直接沿用为当前缺陷。当前源码已改为升温判据、有界 UART 等待，并有后续有效实板采样证据。报告更新不修复任何被引用产品对象。

本次只进行静态代码、版本和既有证据核对，没有构建、运行测试、连接设备、安装、复位、烧录、访问远端服务或修改产品源码。局部历史 PASS 不升级为本次测试通过；本次定点读取EV-028解析原文；原始捕获本次计算摘要确认对象身份，不是新采集或重新执行独立验证。后附摘要仅标识检查时对象，不证明设备和远端从未变化。

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
| EV-029 `evidence/ev029/serial_capture_1204Z.bin` | 原始158B；MD5 `6522407d2058a1f3ced6edef9662c314`；历史2026-11-08 12:04:51–12:09:51UTC | 既有分析记录、此次沿用：k27 t272、k30 p272 t277；2条q1，V0/D0/r0/s0/y0；capture_analysis.txt/tool_results.txt | 会话连续性是旧证据的关联判断，不能充当当前镜像的不可变身份认证 |
| D2/E1历史修复证据 `evidence/test.md`内EV-024/026、`evidence/ev026/raw_i2c_capture.sr`、decoded_i2c.txt/capture_metadata.json/capture_analysis.txt | 旧候选与采样参数按原报告；原始对象附录摘要 | 慢I2C后27/27成功；delay恢复后STOP→读START32.525ms、双CRC；失败采样点后移/单独延长配置等待曾无效 | 不是本次硬件复验；数字采样不能证明电平/供电；不重做已否定修复，不断言根因已永久排除 |
| 接口/设计 `artifacts/interface_contract.md` IC-002 v3；android_architecture.md/firmware_design.md | 附录摘要 | 判据/布局/参数化标定和设计可复用 | IC-002包含旧实施交接：D-01、结构化光照等部分已完成，不能当现行缺陷清单 |
| 已部署服务；本地 `服务器迁移记录.md` | 本地摘要见附录；远端位置与版本本次未认证 | 用户确认部署；定点核对资料中 /upload_data、字段、UNIQUE及INSERT IGNORE声明；资料有SSH说明 | 当前已登记只读SSH入口，但本能力不满足其动作授权；未连接；管理变更与DB直连未授权。文档摘要不认证远端版本 |
| 后端探索 `artifacts/backend_feasibility.md`；原始 `evidence/backend_exploration/upload_contract_observations.txt` 和 upload_contract_stub.py | 所属提交 `06f9c59`，本次文件摘要见附录 | 历史WORK_FAILED；契约层无需服务端改动；仅本地Flask/SQLite桩13请求、538行，不是真实服务测试 | 不证明MySQL行为、线上鉴权、幂等或持久化。其“无登记入口”是旧配置事实，本次已增加只读SSH入口；原失败不删除，不断言后续能力不能修复 |
| 服务协调 `artifacts/failure_resolution_backend_service.md` | 所属提交 `2e3c2df`；本次文件摘要见附录 | 历史REPLAN_REQUIRED，保留服务运行期未证与测试身份/回读缺项 | 报告中旧授权与流程判断不覆盖本次输入；本次明确只读探索前置、真实写入/幂等/持久化回读在端到端阶段执行，缺条件记未执行 |
| 上游索引 `artifacts/codegraph_current_index.md`、`evidence/codegraph_verified_validation.json` | 接手HEAD及附录摘要；上游2026-11-09检查记录，源码基线2e3c2df | 三子工程已同步/查询；本次MCP按精确projectPath查询当前磁盘源码 | 容器无根索引，图不跨RF/BLE/HTTP证明集成；无独立服务端源码根，不臆造服务端索引；旧current_validation不是本次上游原始证据入口 |

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
| 已部署服务 | existing_service_ssh_inspection已登记启用，连接来源为服务器迁移记录.md，30s上限；当前能力缺inspect_existing_service_readonly，故unavailable_for_current_capability。只许获准责任方查看本项目服务状态/端口/必要近期日志，不用sudo，不变更、不写接口、不直连DB。本次未连接；重部署/DB管理未授权 |
| 环境修复 | 当前关闭；不改权限、系统包、驱动、网络、网关配置，不换通道绕过缺项 |

实际I2C固件/旧网表为PA03/PA04，仪器登记描述含PB0/PB1映射，两者的测试点对应仍待接线责任方核实；不据描述自行修改引脚。SDA下降沿触发不等于限定SCL高的START识别，采样率不是总线速度。数字捕获不证明电平兼容。真实遮光/温度刺激需明确操作窗口；readme“现在夜间”不自动延续为本次时点事实。

安全持续限制：报告不复制密钥/凭据；HTTP明文及TLS主机名校验的旧描述只作现状风险，未经本次安全验证，不擅自重设协议。保护既有本地记录和待发箱；重试保留原time，设备ID/网关身份/topic上下文切换需防错配。无外部安全标准研究或认证结论。不得终止无关占用进程；后续设备操作有界等待并保存原始字节/波形及参数。

## 6. 可继续范围、必要决定与总测目标

| 工作 | 当前可以继续的范围 | 完成条件/责任边界 |
|---|---|---|
| Android四项增量 | 复用android_tasks.yaml/android_architecture.md既有设计：权限请求与状态、正常发现绑定、完整BLE批次转发/零值、升温文案 | Android实施后由软件测试核对当前源码/APK/真机；当前checkBluetoothPermission:552–557仍API<31检查定位，connect:582等可静默返回，不认定四项已做；本报告不重复制作这些任务或以平台规则外推替代验证 |
| 全暗标定 | 获准责任方先测当前光敏原始码，与用户约定遮光/恢复窗口；真全暗数据绑定板件及配置 | 固件据实测回填C_dark并重建，随后明/暗/无效及1/3边界复验；用户已愿配合，不再把“不能提供遮光”作为既定事实 |
| 上电日志 | 在获准探索/测试职责内评估已登记工具是否支持先开串口后复位；需新入口时明确配置/授权缺项 | 当前无可执行pyocd入口；按readme暂记未执行及原因并继续其他有条件测试，不能标通过，不为捕获反复改固件 |
| 固件阶段与整体验收 | 保留EV-028按当时complete_delivery规则得出的TEST_FAIL；当前依readme补充3/4，分别报告实板已测与未测，手机跨链路留后续联调 | 无串口采集期间reset能力时上电日志记未执行及原因，继续其他有条件项；不为日志反复改固件，不因尚未开始手机联调反复退回固件实现；测试文档阶段适用需由测试责任方对应本次要求维护，最终集成要求保留 |
| 服务复用与前置探索 | 由获准后端责任方从指定迁移资料读取连接信息，经已登记SSH只读核对项目服务状态、端口、必要近期日志及现有接口资料 | 本能力缺外部动作，不执行；后续需检查当次有效授权而非沿用本能力结论。记录实际连接结果/超时/运行版本及脱敏证据；版本无法取得则保留未知，不默认重部署 |
| 服务端到端验收 | 真实上传、同键重试/冲突幂等、持久化与回读，安排在端到端阶段 | 执行前落实实际测试入口、隔离测试身份、写入与回读授权；缺条件的具体子项记未执行及原因。2xx不替代落库，不从SSH只读授权推导写接口/DB直连/清理权限 |
| 真实集成与候选身份 | 固件镜像摘要、板件/网关实物、APK摘要/签名和服务版本对应同一验证窗口 | 从真实采样到云端回读按同笔ID/T/RH/时间语义比对；候选任一变化重新关联记录与必要回归 |
| 测试资产维护 | 复用现有harness和计划；仅修陈旧host_rf_report_verify_ev.c/host_light_verify_ev.c等资产及未覆盖层 | 嵌入式测试负责，旧API编译失败不计入PASS计数；不修改产品源码或降低断言来通过 |

本次输入已明确前置只读服务探索与端到端真实写入的阶段分工，不再把取得真实上传结果当作本次系统分析或前置探索的完成条件。仍需落实合法采集编排、获准服务检查责任方以及端到端测试身份/入口/回读条件。这些不妨碍静态分析交付；不在分析阶段请求设备操作或停住其他有条件工作。

总体最终目标保持：真实有效温湿度/光照 → 条件上报与正确静默 → RF实收 → 网关缓存/BLE → 手机逐ID显示/既有告警/本地落库 → MQTT/HTTP → 服务持久化/回读一致。覆盖升温及35℃严格边界、明/暗/无效、首笔/失败/重启、冻结重试、多设备/零值/顺序、缓存复读、网络断续和后台节拍。各组件单项完成仍不替代此链路；无光原因不能由下游帧反推，上报成功不等于报警正确。

本次已完成需求映射、当前与历史分离、边界/接口/资源/安全/时序分析、可复用候选确认及依赖缺项记录。分析本身无未交付项；产品待办不因分析完成而宣称完成。本次没有后台任务或受管理长时作业；工具均已返回，未自行提交/推送。

## 7. 本次增量检查与判断来源

本次按上游登记的三个精确projectPath调用CodeGraph，未建索引或升级工具。查询返回当前磁盘行号和源码：sensor_decide_report（fw_core.c:110–123）确认升温及严格边界；measure_sample/temperature_process（measure.c:460–532）确认有效采集、冻结样本与前值推进；light_sample（light.c:147–205）确认标定有效门禁；trace_wait_flag/trace_putc/trace_uart_open（debug_trace.c:37–154）确认UART等待计数和初始化。网关decode_frame10（feistel_al.c:119–131）确认解密后CRC、ID及小端温湿；build_device_block（bleencrypt.c:42–60）确认8B载荷复制与16B设备块。App GatewayFrameCodec:96–145确认块长度与大端换算，ReadingAttribution:68–125确认按保存顺序归因和数量检查；CloudUploadRepository:86–142确认入队、异步上传、成功删待发、失败退避。

本次另查MqtttService:552–635，当前API<31仍以ACCESS_FINE_LOCATION作连接门禁，缺权限/蓝牙关闭/占用时记录日志后返回，5次×18s及4次×2s构成98s名义连接预算；实际进程调度与平台行为未测。零值连续计数分支仍见754–760。此为源码事实，不是本次Android平台规则认证或真机复现；Android既有四项增量继续交其负责角色处理，不把旧188项PASS当作四项完成。图裁剪未返回的代码不补造；宏仅定点读取已知sensor_config.h。

版本核对：本次git rev-parse得到第4节三棵tree；git diff bee3d8f HEAD -- sensor工程/USER无输出且退出0，确认该生产源码范围未变。产品源码没有新增改动；上游索引更新为当前verified_validation入口。不能由相同源码推导设备仍运行该镜像。

后端历史探索提出的缓存复读与接收时间风险继续保留：条件上报减少RF发送，并不自动使App每180s缓存读取都成为新样本；单手机正常路径每天最多480次轮询只是180s的算术上界，不是实测吞吐或云容量承诺。当前待发箱批上限50、退避60s至1800s、最多20次，达到上限保留行而不再自动重试；由源码支持，恢复/积压/身份切换与MQTT回环影响仍须测试。服务端UNIQUE与INSERT IGNORE目前仅为资料声明；桩内first-wins、13请求和538行不能外推MySQL事务、精度、鉴权或部分成功行为。若2xx实际未完整落库，当前客户端可能删去待发项，是端到端必须区分的风险。未发现必须新增服务字段或重建平台的需求依据。

旧协调报告中的“重派无效”“非实现缺陷”等为其旧时点判断，本报告不据此判断他方不可修复或终止重试；当前授权和需求已变化，按真实新检查结果交对应负责角色。读档、图查询和版本诊断均已结束，无后台诊断进程；未执行SSH、HTTP、数据库或设备操作。本次报告/清单变更和摘要可读性检查只验证交付文件，不构成产品功能验收。

## 附录：本次确认的文件身份

以下SHA-256由本次本机对指定文件重新计算。未重新运行文件中的实验；历史报告与原始文件仍按原窗口、版本、权限及未核验项解释。本地对象摘要不能证明远端/板上对象从未变化。三子工程已跟踪对象整体版本见第4节Git tree。

检查时点：2026-11-09T14:22:46.5936353+08:00

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
| artifacts/codegraph_current_index.md | 7cb5505123078a282e2de3453d64b675f90371ca4ac95b026c0a2b49961c2c81 |
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
| artifacts/backend_feasibility.md | a1ccb119544102ed428035f3d633515d1d8c63e1f9732f21c132c3f0be51e203 |
| artifacts/failure_resolution_backend_service.md | 226f58ad233a68da9982bf08f643ebb9dc7421a7fe9d0f45d4ae725afd3d354a |
| evidence/backend_exploration/upload_contract_observations.txt | 8f200994dcc3b338aff1f74cf4ed5b81faa8a046aa5a62c45cd50027a3e153e1 |
| evidence/backend_exploration/upload_contract_stub.py | 5134c6200ba42fed4ee43adcac2e28c1c7454db0042d3c5ff97bdacef4629a8a |
| evidence/codegraph_verified_validation.json | c867d218e1fd625ccefd85a886bf592980f485ba0a94b9c2a2257b63164c346b |
| CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/fw_core.c | 780fe5126fe51f6cbf16a8d0b1e53caa14c927199e9a51f8e8e8f7743c515945 |
| CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/measure.c | aaa82d4bd42d168d4e660f0444868ae20e620b9b417b4d53b852e8ef2e97f7e7 |
| CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/light.c | e45bd25f80ac2cc3678666dab5c4d4a1737ed65e70bbc3bbf69cc4586054dae8 |
| CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/debug_trace.c | 7800991c14589f8ec936a119392ee99e707d7c01ef95f3e57f19295a945708e9 |
| CH592EVT/EVT/EXAM/BLE/beiwov2/APP/feistel_al.c | b1c14ae82067a4ef5ba69d3c6bd04aa8fdec7524a07a58d28cf5c5df2d6ad300 |
| CH592EVT/EVT/EXAM/BLE/beiwov2/APP/bleencrypt.c | 982060bfab94ee6d93c3b91942c41802d84adc22cddf87f4196166af7903d2a6 |
| dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt | dd3847ff2fec80289f0e82282140fab77b8f0c78e85e59ff6a94040b934a88ac |
| dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt | c221de8da8d78ecc4b41e341daca69ece754fe8e18fd95920cdb065612b84a1c |
| dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayFrameCodec.kt | 76f669ad4ad88a786b3bec95d70bf76c1c8f865ff901938a626a34f84ccde464 |
| dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/telemetry/ReadingAttribution.kt | 8b31db2523b5416c74e03297659d49ad7384d5d2b5446adc817edb388416306a |
