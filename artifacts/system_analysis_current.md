# 当前系统需求与约束分析

负责角色：系统架构。检查日期：2026-11-10（Asia/Shanghai）。

## 1. 判断与范围

分析就绪，可继续既有候选的增量实现、实测标定和分阶段验收；产品整体尚未通过。接手 HEAD 为 `e16cf5994685e4d79484e11835873d362e74626d`，首次及写入前 `git status --porcelain=v1` 均为空。已读取项目清单、最近五条提交、最近提交统计、根 readme 八点及四项测试补充、上游索引、既有系统分析、接口契约，以及固件、Android、后端和最新失败报告的相关部分。本报告更新当前分析入口，旧版本保留在 Git 历史。

本次仅静态分析和文件身份核对，未构建、运行产品测试、访问远端、连接设备、安装应用、复位或烧录。CodeGraph 按三个子工程的实际 projectPath 返回当前磁盘源码，仓库根无索引；未新建或同步索引。图不跨无线、蓝牙和 HTTP 证明集成。硬件手册未重新审阅，规格解释引用既有设计/接口资料，不当作本次器件规格独立核验。

本次相对旧分析的实质更新：纳入 BE-EXP-002 真实只读服务记录；纳入 EV-030/031 正常采集与整体失败边界；Android 待办采用 AA-004 的六项现有增量；撤销“服务从未核验”等过时描述，但不将旧在线状态写为当前在线。不重复安排制作已有驱动、解析器、待发箱或已修复的升温判定。

## 2. 需求、现有成果与剩余工作

需求来源：当前用户目标及 `readme.txt` 修改点1–8、补充安排1–4。既有设计解释单列来源，不冒充用户原话。

| 编号/需求 | 已存在、可复用的成果 | 仍需工作及证据边界 |
|---|---|---|
| R1 换 GXHT40，采集温湿度 | gxht40 驱动、CRC、换算、重试与诊断；EV-030 5/5周期 q=1，EV-031另2/2，双字CRC实测匹配 | 本次板上身份/在线未确认；局部旧成功不证明长时稳定；保留早期失败与后续修复 |
| R2 使用现有光敏分压，不改硬件 | BOM含U9 GXHT40、R3 5M；网表PB04 LIGHT_ADC、PB05 LIGTHT_POWER；代码ADC与采样供电实现 | 实际板件/受光路径/建立时间和受光低码需实测；不得恢复退役光通信来代替光敏 |
| R3 采集周期3分钟 | sensor RTC分钟节拍及既有三拍机制；Android SensorCadence=180s；历史串口k步长3 | 180s是采样机会，不是固定上报周期；实际周期、息屏轮询及端到端延时分别验 |
| R4 升温>0.9℃且无光，或温度>35℃才上报 | fw_core.c:110–123 当前方向和严格边界正确；冻结待发样本、前值快照已存在 | 真实暗态升温/超温触发与静默待验；不把历史整体失败直接归为待修判据 |
| R5 broker资料已提供、复用已部署服务 | CloudConfig与迁移资料；BE-EXP-002在2026-11-09确认服务/端口/路由存在 | 历史401机制、当前部署身份、真实上传/幂等/持久化回读未确定；不默认重建或部署 |
| R6 同步Android、iOS不改，取消sensor光通信/霍尔 | 统一BLE解码、ID归因、本地库/待发箱；历史14项实现及188用例；固件退役记录与EV-030静态核查 | AA-004六项增量未完成（见第5节）；真实安装升级/权限/发现/后台/跨链路待验；本次未全面审计工程排除项 |
| R7 无光阈值取全暗基准1/3 | light_is_dark 为有效时3×mean>=C_dark；结构化光照结果已实现 | REF_CODE=0、CALIBRATED=0；缺受控全遮光及受光双态原始数据，不能用估算/满量程/旧暗场码回填 |
| R8 UART1初始化与打印 | UART1轨迹及有界等待；历史S/BUS原始字节；BOOT/VDD/IOTEST/IOSIG/BUS设计资产 | 完整上电段及uid未取得；按补充3暂记未执行和原因，继续其他可测项，不为日志反复改固件 |

固定边界：CR2032焊接供电保持；不改硬件、iOS和既有网关。网关是协议兼容与联调依赖，不新增刷写/改造任务。不增加心跳、RF序号/采样时间字段、光照线上字段、服务协议或告警算法。传感器升温上报和App既有温湿度下降报警分别承担职责。

## 3. 组件、接口与语义

链路为 GXHT40/光敏 → CW32采样与门控 → 433MHz → 既有CH592接收/缓存 → BLE → Android归因/本地/告警/平台适配 → 既有MQTT/HTTP服务 → 存储及回读。此为依赖关系，不表明CH592具有MQTT客户端或整链已运行。

| 边界 | 已有契约/代码核对 | 持续限制及未知 |
|---|---|---|
| GXHT40→MCU | IC-002、当前measure_sample：0x44/0x45、0xFD、6字节双CRC，T有符号x10、RH x10；当前配置测量等待20ms | 契约旧正文10ms是历史值，当前配置优先；数字波形不证明电气兼容 |
| MCU→网关 | IC-002：加密10B；明文ID4/T s16LE/RH u16LE/CRC16LE；沿用Feistel | 无ACK/序号/采样时间；本地发送完成不等于网关实收或端到端恰好一次 |
| 网关→Android | 图查询build_device_block、GatewayFrameCodec：16B头+N×16B块；设备前8B为ID4/RH u16BE@4/T s16BE@6；AES/ECB/NoPadding、FFE0/FFE2 | 实物名称/地址/镜像、真实MTU、多设备和配对未确认；未知ID不应归因另一设备 |
| Android→本地 | GatewayFrameCodec拒绝不完整/无设备帧；按ID归因、负温保留、单位仅缩放一次；既有SQLite与待发箱 | 读缓存时刻是接收时间，不是物理采样时间；全零占位和合法零值当前仍混淆 |
| Android→MQTT | IC-002：既有topic及`epoch:temps_csv:humis_csv` | 无逐读数ID，绑定顺序需一致；当前BLE只保留最后一笔，不能代表整批 |
| Android→HTTP→存储 | POST /upload_data，phone/mac/readings；每笔devId/time/temperature/humidity；2xx后ack、失败退避 | 2xx不能证明落库；UNIQUE(dev_id,time)/INSERT IGNORE仅资料声明；具体401门禁及回读入口未证 |

规范判据沿用用户目标和IC-002：`valid_T_RH && ((have_prev && current_x10-prev_x10>9 && light_valid && dark) || current_x10>350)`，差值用int32。上次指上次有效采集而非上次上报；温湿度失败不造零、不推进前值；光照失败只关闭升温分支；无前值首笔仍可独立超温；重启RAM前值丢失。冻结待发快照与新采样分离，重试不能换成别笔。这些是现有接口生命周期解释。

规范向量：30→30.9暗不报，30→31暗报，30→31明/光无效不报，31→30暗不报；34→35暗仍可由升温分支报；35无前值不报；35.1在有效温湿度且无前值/明/光无效时可报。严格35℃仅不触发超温分支，不能整体排除35℃。区分判定r、冻结P、本地发送s、网关实收、手机告警。

全暗比例沿用既有IC-002解释：阈值ceil(C_dark/3)，等于算暗，原始ADC码比例不是lux比例。拓扑为PB05→R3 5M→PB04→光敏→GND，暗态码升高。C_dark须绑定板件/供电/ADC参考/建立时间/取均值策略、遮光方式和原始分布。缺标定时保持invalid，不用满量程合成暗态。近满量程旧码3990量级不能单独证明当前全暗标定合格。

缓存复读策略沿用AA-004：按每次有效观测接收时间处理，不擅自按值去重；跨秒复读可能成为多条记录，秒级键亦可能合并同秒读数。若改变去重/压缩或部分设备批次协议，需要明确产品决定，当前任务不扩大范围。

## 4. 当前候选、版本与原始证据

以下路径相对仓库根，检查基线为本报告接手HEAD；附录SHA-256标识本次检查对象。文件摘要不证明设备或远端从未变化，版本改变后需重新出具确认记录并使后续联调/总测对应当前候选。

| 对象/位置 | 不可变版本/身份及已有原始证据 | 本次核对与未核验项 |
|---|---|---|
| sensor工程 `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output` | tree `09bef6581f39ca50ba2ecfbaa5c77ef9accd68a5`；FWI-002 rev7；`evidence/fw_impl_20261109/mdk_rebuild.result.txt`、host_build_test.log | 当前USER相对受测f978a01无差异；未重建、未确认板上镜像；当前本地构建产物未验 |
| EV-030受测sensor | 提交f978a01；报告记AXF MD5 `a3b08774e764f174eac318f6771d0c42`，HEX `546ce20a718fa69b40177cce0145f4b5`；`evidence/ev030/tool_results.txt`、4个cap、raw_i2c_capture.sr、decoded_i2c.txt、capture_analysis.txt | 原报告0E/1W、下载Verify OK；5/5 q=1、CRC匹配；本次读取解析原文并为原始对象算摘要，没有重新执行其脚本或采集；下载成功不是功能合格 |
| EV-031追加观测 | `evidence/ev031/serial_capture_0852Z.bin`、tool_results.txt；旧窗口2026-11-09 08:52:50–08:57:50UTC，原报告MD5 `8559c0278503afcb2563d871b975797e` | 报告称同启动会话新增k33/36、2/2 q=1；会话连续性是旧证据关联，不是当前板上身份认证；未重算业务验证 |
| 网关 `CH592EVT/EVT/EXAM/BLE/beiwov2` | tree `22c8a193daffea87408c971a53db566633f7c6ea`；图查询APP/bleencrypt.c；IC-002兼容布局 | 源码可复用；实物身份、部署镜像、RF/BLE及全零占位真实窗口未核验；不刷写 |
| Android `dengbei_care` | tree `3c9feeb9ce399b3ba5ac359b31090af583ce4781`；AIMPL-002、`evidence/software_test.md`历史14项及188/0、版本9/1.8记录 | 当前源码查询仍见权限门/最后一笔/零值门槛；历史PASS不覆盖AA-004六项增量；APK摘要、签名、真机安装升级和后台未验 |
| 已部署服务 | BE-EXP-002旧窗口2026-11-09 06:24–06:28UTC；原始 `evidence/backend_exploration/ssh_readonly_inspection*.txt`、http_route_existence.txt、http_request_history.txt | 旧记录EMQX 5.8.0、unit active、6路由405；日志10-22上传401/401/200。本次读原始版本/历史请求行；未新连接，服务应用不可变版本/源码摘要未知，mtime和大小不是不可变版本 |
| 后端本地桩 | `evidence/backend_exploration/upload_contract_observations_rerun.txt`及旧报告 | 13用例/538行仅本地Flask/SQLite桩记录，不证明MySQL或线上幂等 |
| 需求/设计/接口与计划 | readme、IC-002 v3、FWI/FD/FWR rev7、AA-004 v3、现有测试计划与任务；附录摘要 | 分析不替职责方实现或验收；IC-002旧D-01/D-02的已实现项不再作为缺陷；部分正文时序/旧交接已陈旧 |
| 索引 | `artifacts/codegraph_current_index.md`、`evidence/codegraph_validation_20261110.json` | 上游三根覆盖/排除与查询证据可用；本次精确根MCP查询当前源码；不覆盖iOS、SDK全量或远端服务 |

失败历史必须保留：早期GXHT40失败→D2慢I2C→E1恢复delay→F-1前值观测修复→EV-028/029局部正常→EV-030完整交付TEST_FAIL→EV-031归因补充正常窗口。最新整体TEST_FAIL没有被撤销；归因“未发现新失败签名”不证明无缺陷或不可修复。FR-003的拒接/流程意见是历史记录，不是本报告批准终止、指定路由或创建节点。

## 5. 缺口、可继续范围及必要决定

| 事项 | 当前可继续工作与责任 | 完成条件/不能替代的证据 |
|---|---|---|
| Android六项既有增量 | Android工程复用AA-004及android_tasks.yaml：权限策略与可见状态、正常发现绑定、完整BLE批次转发、占位/合法零值、上传失败分类与401阻塞/手动重试、升温文案 | 当前MqtttService:552–557在旧API仍以定位作直连门；:748–752仅最后一笔；:743–760零值需3次；CloudUploadRepository上传接口仍Boolean；SensorCadence:7仍写下降。由软件测试对变更后的源码/APK/真机独立验收，既有14项不重做实现 |
| 光敏标定 | 获准固件/嵌入式职责先测当前码、确认当次照明，再与用户约定受光/完全遮光/恢复窗口，保存原始分布 | 用户愿配合不是实测；旧报告用户曾更正无光与本轮目标写有光存在时点差异，不能从旧回复判当前状态。取得合法C_dark后固件回填并重建，对新候选复验；不得估算 |
| 上电日志 | 获准职责评估先采集后reset方案 | EV-030串口晚开0.208s；EV-031说明另有pyocd0.36 CLI可枚举但无CW32目标定义，不能笼统写全机未安装；当前无登记pyocd入口，不能执行。按用户补充3记未执行及原因并继续，不为日志反复改固件 |
| 固件阶段分工 | 固件/嵌入式测试保留已完成实板成果与未执行项；测试责任方按当前用户补充核对验收阶段适用性 | 旧TD complete_delivery与用户补充3/4存在阶段冲突；本报告不改测试断言，不将旧整体FAIL改PASS；不以尚未开始手机联调反复退回固件实现，联调目标仍保留 |
| 服务复用 | 复用BE-EXP-002在线/路由证据；后续必要刷新仅由获准职责经登记入口执行 | 当前分析缺inspect_existing_service_readonly；不联网补测。旧401不充分证明Bearer机制，不猜测请求头；真实身份/请求/响应/服务日志相关性需合法测试窗口 |
| 端到端云验证 | 现有E2E计划包含真实上传、同键重试/冲突、幂等、持久化回读与断网补传 | 执行前落实隔离测试身份、写入/回读入口及权限、清理策略；只读SSH不授予POST或数据库直连；2xx与本地桩不替代回读 |
| 测试资产 | 测试责任方按当前API维护历史harness并复用现有有效用例 | host_light_verify_ev.c等旧API问题只属测试资产；不修改产品或降低断言让验证通过；本次未编译，编译现状按报告引用 |
| 硬件/资源 | 获准职责确认实际板件、接线、功耗工具及测量窗口 | 网表器件标签CW32L010Y8M6与Keil Device CW32L010F8不一致，后者是当前配置身份来源；PA03/PA04 I2C与环境PB0/PB1测试点文字对应未核实。不能据此擅自改引脚/工程/硬件 |

这些缺口不妨碍完成系统分析，不需要本轮请求用户做遮光或设备操作。记录交接不保证后续能力一定执行，执行安排由Runtime负责。

## 6. 时序、资源、安全及总测边界

当前配置静读：I2C_DELAY_LOOPS=300、GXHT40_MEASURE_WAIT_MS=20、LIGHT_ADC_SAMPLES=8、EOC_GUARD=100000、UART_TX_GUARD=200000、UART9600；光敏标定0/0。EV-030旧数字捕获记录STOP→读START32.528ms、SCL周期614.25μs（约1.628kHz），与4MS/s采样率不同；这是旧窗口测量，不能推断电平。SDA下降沿触发不等于限定SCL高的START识别。

Keil当前Device=CW32L010F8，IROM64KiB/IRAM4KiB。旧构建Code25964+RO656=26620B，RW116+ZI1676=1792B；静态空间有余量不证明栈峰值、峰值电流、续航、误报率或长期稳定性。UART等待是循环guard，不等于独立校准的墙钟时限。Android当前每次连接18s、最多5次、间隔2s，约98s单轮等待预算，不承诺端到端时延。

唯一环境授权来源为本次Unified汇总：COM43/9600 8N1缺serial_capture；下载缺mdk_flash/flash_device；Redmi8A序列号12f046690506只读ADB configured_unverified，交互缺android_test；FX2 conn=2.32缺sigrok_capture/operate_test_equipment；服务只读SSH缺inspect_existing_service_readonly。正式分析不主动操作硬件，以上均未使用。COM26不是传感器串口，不使用旧COM42或2.19；设备重插后不能猜地址。网关只经App正常发现/配对确认，无独立管理入口。环境修复关闭，不改permissions/environment，不安装系统包、改驱动/网络或借其他通道绕过。

安全与持续限制：不复制密钥/口令，不把SSH/数据库凭据写进App；保护本地记录和待发箱，重试保留原time。旧后端记录有明文HTTP、数据库公网监听等现状风险，未获准改变服务协议、防火墙或部署；本分析不新增安全整改范围，也不把风险写成安全通过。已有TLS/密钥/UUID/topic/schema兼容面保持，鉴权未知不靠猜测实现。目标设备占用时报告具体占用，不终止无关进程。

最终集成目标仍为：真实有效温湿度及光照→正确触发/静默→RF实收→网关缓存/BLE→手机逐ID显示/既有告警/本地落库→MQTT/HTTP→真实持久化及回读一致。必须关联固件镜像、板件/网关、APK摘要/签名、服务版本和验证窗口；各组件局部完成不替代这条链。覆盖严格边界、首笔/失败/重启、冻结重试、多设备/合法零值/占位/顺序、缓存复读、断网与后台。光照和上报原因不在帧内，下游不能反推无光；上报不等于报警。无现成定量续航/时延验收值时保留未知，不自行制定阈值。

本次已完成需求映射、当前代码与历史证据分离、接口/时序/资源/安全分析、版本登记、剩余工作及阶段冲突记录。产品未完成项保持开放；分析完成不代表整体通过。未修改产品源码和他人测试资产，未运行后台任务或提交Git。一次定点读取不存在的 `evidence/ev031/capture_analysis.txt` 返回失败，未将其作为证据，已改用实际存在的tool_results.txt、原始bin及triage报告；其余所引用读取与图查询已返回。

## 附录：本次对象摘要

以下为检查时文件SHA-256，路径均相对仓库根。摘要仅确认本地对象身份，不认证远端版本或重新验证业务结论。

检查时点：
2026-11-10T07:03:55.7876243+08:00

| 对象 | SHA-256 |
|---|---|
| `readme.txt` | `c6f19954d54c29136db95d75dcc28816d84d5ce57ea449714a0c58ba80a1e6dd` |
| `artifacts/codegraph_current_index.md` | `f86b9716c99a7bd907a31085f234b01f9dc5cf63c15f88ecdc524f45bfd5909a` |
| `evidence/codegraph_validation_20261110.json` | `d8f7047612c8d18d9c80d8b91cfe0428878c4aa37fbd157920f4b9fdab121787` |
| `artifacts/interface_contract.md` | `6af5f0a65c819033390e94099c77ede83af53b6a2750edf55debd9fe42d2ff22` |
| `artifacts/firmware_implementation.md` | `d39e754aeb54ec42c17f5503764374ddc5fc3351b597678d661ee68421b7fa9d` |
| `artifacts/firmware_feasibility.md` | `295f12e5df5222081742c94e63f30f91447a2d39132932e097475fde98a7d618` |
| `artifacts/firmware_design.md` | `ed23e41e598c0a5aa4b85b2b25e5b5cfe6a06ca3f1baa04e335d16f674b11447` |
| `artifacts/firmware_requirements.md` | `ea00a443c6ba329011238061254b5c240c3cadd0f99f527bf20465d6b485cf9a` |
| `artifacts/firmware_tasks.yaml` | `acfe0a4d61c1fc416796097461e36ac1943c93ee8277c050713fe8b83f3f5ec5` |
| `artifacts/android_architecture.md` | `d565e1413f98824c34e5b6923f943d1b89225f0169e85252969bdc0ee3e4703c` |
| `artifacts/android_tasks.yaml` | `3fa069042fe7b0709e86d6dbbdcb0dbf3af276820c845d4bb6b430113b6a7f02` |
| `artifacts/android_implementation.md` | `df7992cb1dd4f701cdab937437dcc94f7832e0031fe79902e520ca8bd6f0d1ba` |
| `artifacts/backend_feasibility.md` | `b775ec915c72d5ae3bb6cedb62e594e047b2c9666789438810a0ee358b393fd2` |
| `artifacts/failure_resolution.md` | `330aef55af04619d2390ac33bf7363399b24ddcb804e0fddac8bd8252f8af870` |
| `artifacts/software_e2e_plan.md` | `dad411e0b662834593855ce133ecf100826f1ecdc7a3c8e65979f5328e49d284` |
| `artifacts/software_test_design.md` | `85858e3f10ab7f414b71024b9003838a4937a5adb85d402c3911cd106f246383` |
| `artifacts/test_design.md` | `62afe3e79d2d2ad56e22deb866320cbbac454eaf104c176a8e4b3e819330bf6a` |
| `evidence/test.md` | `7354164eedbf089078d7bd8977edf5f389b7ca51dd29ed09d9c8b7fd4a26733e` |
| `evidence/triage.md` | `53e4f89b405be6b051032b3b3cf7cceeecaef7525f83d8325ceb8b0ddfec3d75` |
| `evidence/software_test.md` | `8fc9fffbf27bbb4f0518f987cd01a732febc9d6ef49ed40b604e4afcbd224671` |
| `evidence/ev030/tool_results.txt` | `889de781c7ff1a52d4ee1aca2a02a1ab7c0fe31de38699b9d2a8c0552eed030b` |
| `evidence/ev030/capture_analysis.txt` | `e3da51c24946f6349c34636f3a75aeb5492b9024c21759b42fa8fd6a97077e09` |
| `evidence/ev030/cap01_boot_bus_k0.cap` | `b084d97ef8199de5bea4a998788ffa1e5de47f934b3ea7cb259cc3c3336f0711` |
| `evidence/ev030/cap02_k6.cap` | `403c700557c055d3aa32d70e05763f6b6a980f3d8c5a2c335a472cf37b903db7` |
| `evidence/ev030/cap03_k9_k12.cap` | `31835aebc970f1ac952ec1205fa4bae41c6ff506730b51ef1fb97e54fea107f2` |
| `evidence/ev030/cap04_k18.cap` | `b96bbed8e6dd1f2bb935f196ac60bfe7f0e672e20fe7499d03694b031cf1fc3e` |
| `evidence/ev030/raw_i2c_capture.sr` | `5ecd528e4c3f477070762620e0997f93d63d7f0efccbc66085a8fc03942d1812` |
| `evidence/ev030/decoded_i2c.txt` | `2de0907fcba477b120e5b592e41ff2f85f31ef33b1d5b26222275e75f49254e6` |
| `evidence/ev031/serial_capture_0852Z.bin` | `81ee253db3b6f7c6cc96a474bb95485b0a508c0938227816fda6fb682e46d202` |
| `evidence/ev031/tool_results.txt` | `d3b298eae4c0b73893ec3f63ebb5a76d153dbf88d487a0a9a7b7499aa60277ce` |
| `evidence/fw_impl_20261109/mdk_rebuild.result.txt` | `a7c109e3b36149945850955acaa78fd411b645a6a674f9abbf079dc6d9598d43` |
| `evidence/fw_impl_20261109/host_build_test.log` | `6e07da5e7f32caf8e3ce76194dbff1441992f8713cc9700b319450d65bad4e40` |
| `evidence/backend_exploration/ssh_readonly_inspection.txt` | `cea6dd4da0363cd892efa9ba9eb96e12143490df49b88f9e2e39ea5ad38e66be` |
| `evidence/backend_exploration/ssh_readonly_inspection_2.txt` | `051055532ef507560244e80231cacf11fb4c7a1866dc335b4e0d27f13001dda3` |
| `evidence/backend_exploration/ssh_readonly_inspection_3.txt` | `13f87c3faa662664c3ecdb227e66d98811abdf8d250821c0e3153b37f3b59b28` |
| `evidence/backend_exploration/http_route_existence.txt` | `6c037a66849fb03d6a7021564e0d3256200dd52d677b4eab202db3d063256a77` |
| `evidence/backend_exploration/http_request_history.txt` | `85e2f7752012b8efb86926adabc1ad2297e0b2ca32321deba56c84e437ae78d6` |
| `evidence/backend_exploration/upload_contract_observations_rerun.txt` | `2001d7d8d68dc548d21ad7d399aa75d14139e6a6075645ee438b5f1da483d9ad` |
| `sensor_hardware/MAIN_BOARD.BOM` | `2dfc8ddd0fd74a5bfc4a1071aff00841f20ed15c4dd923ff51ab0b650c0f0f85` |
| `sensor_hardware/pstxnet.dat` | `5e0734950da222860ca55ef1e8dc7f5c6ac9ea25bd51c745d9c9afea7d6f19ac` |
| `服务器迁移记录.md` | `e883e4df73b109dd7ac08c862e350ed9b11372a9e073930dd7bcf9e4749811ed` |
| `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/Project.uvprojx` | `60ec74e0594bb2fa6e006a44010a5201b465eb4042375ea16f56ae9c81cd6fcd` |
| `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/inc/sensor_config.h` | `660ffd82989815e1fb31696b5d1905bdc6e3b719db543777bbcc404d41ada3ed` |
| `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/fw_core.c` | `780fe5126fe51f6cbf16a8d0b1e53caa14c927199e9a51f8e8e8f7743c515945` |
| `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/measure.c` | `aaa82d4bd42d168d4e660f0444868ae20e620b9b417b4d53b852e8ef2e97f7e7` |
| `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/light.c` | `e45bd25f80ac2cc3678666dab5c4d4a1737ed65e70bbc3bbf69cc4586054dae8` |
| `CH592EVT/EVT/EXAM/BLE/beiwov2/APP/bleencrypt.c` | `982060bfab94ee6d93c3b91942c41802d84adc22cddf87f4196166af7903d2a6` |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt` | `dd3847ff2fec80289f0e82282140fab77b8f0c78e85e59ff6a94040b934a88ac` |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt` | `c221de8da8d78ecc4b41e341daca69ece754fe8e18fd95920cdb065612b84a1c` |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayFrameCodec.kt` | `76f669ad4ad88a786b3bec95d70bf76c1c8f865ff901938a626a34f84ccde464` |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/telemetry/SensorCadence.kt` | `fe863f0339bcf3f23c42423561557a62e28ae777adb9a9f9901902b72ff27e68` |
