# 系统需求与约束分析

负责角色：系统架构

## 1. 结论与检查范围

本次分析已完成，可以继续围绕现有工程进行增量修正、联调和验收。产品整体尚未完成：当前固件判据与明确需求冲突，真实温湿度采集历史失败未闭环，全暗标定缺失，手机 BLE 连接前提及服务端持久化/回读未完成当前候选验收。分析就绪不代表实现、设备或服务通过。

检查日期：2026-11-08（Asia/Shanghai）；仓库根：`D:\mypro\beiwo2\AIP`；接手 Git HEAD：`bf44dd779a4d6d9454fff7f89e4b3490b5aabe0b`；首次及写报告前 `git status --porcelain` 为空。已读取共享清单、最近五条提交、最近提交统计、需求和相关证据。没有预置的当前项失败历史；仓库内旧失败仍保留。未修改产品源码、接口契约、设备、部署或权限；未执行构建、安装、设备测试、网络探测或数据库访问。

根目录无 `.codegraph/`，但既有交接明确三个子工程索引；本次均先用 CodeGraph CLI explore 查询传感器判定/采样、网关编解码和 Android 权限/上传。CLI 均退出 0，返回带行号的当前磁盘源码；未建立或同步索引。宏配置未由图查询返回，随后定点读取 `sensor_config.h`，未重复读取图已展示的函数。图中的测试关联不是测试执行证据。当前授权无 MCP 工具，因此使用原生 shell CLI。

外部项目 Truth 的 `current_versions.yaml` 为 `current_versions: {}`，`engineering_baseline.yaml` 为 `current_verified_baseline: null`；requirements、architecture、interfaces、approved_artifacts 目录枚举未返回文件。本报告不据此否定仓库中真实存在的成果，也不将旧文档状态自动提升为现行批准或整体验收。

## 2. 用户目标与固定边界

唯一产品需求来源为根目录 `readme.txt` 的八项修改；领域参考用于理解，不覆盖原文。

| 编号 | 需求及约束 | 当前已存在成果 | 仍需工作与证据边界 |
|---|---|---|---|
| R1 | 温湿度器件改为 GXHT40 | GXHT40 驱动、双 CRC、有效域、重试及诊断已存在 | 历史实板 0/7 周期成功；恢复真实有效采集并复验，不能用宿主 mock 代替 |
| R2 | 使用已增加的光敏分压；硬件不改 | PB04 ADC、PB05 采样供电及结构化光照结果已存在 | 对应实际板和环境确认，不重做硬件设计 |
| R3 | 每 3 分钟采集 | 配置 1 分钟 RTC ×3；Android 180 s 节拍；历史串口 k=0,3,…,18 | 当前板、候选及手机长时节拍仍需复核；不解释成每 3 分钟必有上报 |
| R4 | 本次温度比上次升高严格超过 0.9℃且无光，或本次严格超过 35℃时上报 | 条件上报框架、前值状态和 RF 发送已存在 | 当前 `fw_core.c:106–122` 仍按降温判断，并把 35.0℃整体排除，必须交固件修正 |
| R5 | 使用项目提供的 broker/服务器资料 | 迁移记录、Android 云配置、上传待发箱已存在；用户确认服务器已部署 | 先核对和复用现有服务，当前远端版本、接口、持久化和回读尚未核验 |
| R6 | 同步 Android；iOS 暂不改；sensor 光通信/霍尔退役 | Android 解码、归因、SQLite v3、上传、文案改动及历史单测已存在；固件纯逻辑注释记录旧模块退役 | 正常启动权限、网关身份、真实 BLE、多设备归因、升级保留数据及端到端待验；本次未全面复核退役模块的工程排除情况 |
| R7 | 无光阈值采用完全无光基准的 1/3；当前环境有光 | `light_is_dark` 使用 `3*mean>=C_dark`，无效不判暗 | 当前 `LIGHT_DARK_REF_CODE=0`、`LIGHT_DARK_CALIBRATED=0`；需真实完全遮光数据，不能用亮态、满量程或历史默认值替代 |
| R8 | UART1 初始化并打印供调试 | `debug_trace.c:94–132` 初始化；BOOT/BUS/S/G 等打印及历史真实字节存在；配置 9600 | 当前 COM43 板上镜像及输出未确认；调试输出对睡眠/时序的影响待验证 |

固定范围：沿用 CR2032 焊接供电；不修改硬件、iOS、既有网关固件或网关配置。不新增心跳、传感器时间戳、协议版本或云平台重建需求。网关作为需兼容和实际联调的依赖保留在总测范围。

## 3. 组件、数据流与接口

```text
GXHT40 + 光敏分压 → CW32 采样/条件判定 → 433 MHz
→ 既有 CH592 网关接收/缓存/聚合 → BLE FFE0/FFE2
→ Android 解密/按 ID 归因 → 本地存储/既有告警/上传待发箱
→ 既有 EMQX 与 HTTP 服务 → MySQL 持久化与回读验证
```

此图表达依赖关系，不声称 CH592 有 MQTT 能力或线上所有路径已经跑通。迁移记录中的“传感器/网关→MQTT”是概括，不能替代真实生产者身份、设备顺序及部署链路核对。

| 边界 | 可复用约定与本次依据 | 未确认/风险 |
|---|---|---|
| GXHT40→传感器 | 配置 0x44/0x45；0xFD；6 字节 T/RH 各带 CRC；温度有符号 x10、湿度 x10；`measure.c:452–473` 成功才更新 | 器件当前应答、实际引脚电气和采集正确性未测 |
| 传感器→网关 | IC-002 v3：10 B 加密帧，明文 ID4/T s16LE/RH u16LE/CRC16LE；网关 `decode_frame10`、`build_device_block` 图查询确认既有函数 | 本次未运行往返测试或真实 RF；无 ACK/序号/采样时间，不能保证恰好一次 |
| 网关→Android | IC-002：16 B 头 + N×16 B 设备块；块内 ID4/RH u16BE@4/T s16BE@6；FFE0/FFE2，现有 AES；服务代码调用统一解码器 | 网关实物名称/地址、板上版本、GATT/MTU/缓存更新和多设备容量未验 |
| Android→本地 | `GatewayFrameCodec`、`ReadingAttribution`、SQLite v3 和 pending_uploads 既有成果 | 时间为接收时刻或 MQTT 载荷时间；缓存重复读不能证明新采样；本次未运行 SQLite 真机迁移 |
| Android→平台 | 既有 MQTT topic 保持；HTTP `POST /upload_data` 含 phone/mac/readings，读数 devId/time/temperature/humidity；2xx 视成功 | 服务器实际 schema、认证、响应语义、持久化及回读未知；不能以 2xx 单独证明落库 |

MQTT 无逐读数 ID 的兼容载荷依赖同一绑定上下文及设备顺序。数量检查不能发现顺序错配；IC-002 D-03 曾记录 BLE 转发只保留末笔的风险，本次未重新读取该转发分支，因此保留为待确认项，不能说仍未修或已经修复。联调须逐 ID 对照多设备数据。

## 4. 判定、时序与资源约束

沿用现有 IC-002 v3 对需求的解释：有效温湿度且 `((have_prev && int32(T)-int32(P)>9 && light_valid && dark) || T>350)`，单位 0.1℃。前值为上次有效采集而非上次上报；失败不造零、不推进前值；首样本仅能通过独立超温分支；重启 RAM 前值丢失。后四项是既有设计选择，非 readme 逐字要求，后续设计如调整须明确影响。

| 验收向量 | 应有结果 | 当前分析 |
|---|---|---|
| 30.0→30.9℃，暗 | 不上报 | 严格 >0.9℃，不能改成 >= |
| 30.0→31.0℃，暗 | 上报 | 当前代码计算 P−T，冲突 |
| 30.0→31.0℃，明/光照无效 | 不上报 | 未标定不能证明暗 |
| 31.0→30.0℃，暗 | 不上报 | 当前代码可能触发，冲突 |
| 34.0→35.0℃，暗 | 升温分支上报 | 当前 `T==350` 提前返回 false，冲突 |
| 无前值或明/光照无效，35.1℃且温湿度有效 | 上报 | 超温分支独立 |
| 温湿度采集失败 | 不生成有效读数/上报 | 历史 q=0 不得当成正常静默或业务通过 |

全暗阈值等于 `ceil(C_dark/3)`，相等算暗。原始分压码比例是既有接口的实现解释，不等于照度 lux 或光敏阻值线性比例。真实 C_dark 必须绑定板件、供电、遮光方式、ADC 配置、建立时间和原始分布；当前有光不提供该基准。

现有配置：光敏供电等待 100 ms、8 次 ADC、EOC guard=100000；GXHT40 等待 10 ms、读重试 5、整帧尝试 3；RF 单次 200 ms、尝试上限 3。Android 连接代码最多 5 次、单次 withTimeout 18000 ms、间隔 2 s，单轮连接预算可接近 98 s，不能把轮询常量当作完整端到端时延承诺。采样、RF 上报、BLE 缓存读取和云上传各有独立时序。

历史 Keil 结果 Code=20100、RO=644、RW=116、ZI=1676，即 Flash 20744 B、RAM 1792 B，相对历史工程 64 KB/4 KB 有余量；这不是本轮重建或当前栈峰值测量。CR2032 寿命、射频峰值电流、睡眠功耗、误报率和长时稳定性均无本轮实测，不新增臆造的量化指标。

新增静态关注：`debug_trace.c:33–38` 的 TXE 等待无超时计数，虽然注释称有界，固定行长只限制字符数，不能限制硬件标志永不就绪时的等待。交固件评估诊断失效是否阻塞采样/睡眠；本次未复现 UART 故障，不将其描述为已发生故障。

## 5. 既有候选成果确认记录

以下位置均相对上述仓库根。版本来自接手 HEAD 的 Git tree，覆盖已跟踪对象；未跟踪/忽略的构建物不在 tree 承诺内。附录逐文件 SHA-256 只确认本次读到的对象，不能证明远端或板上对象从未变化，也不能替代功能验收。

| 对象及实际位置 | 不可变版本/身份 | 已完成、原始证据 | 未核验项与复用方式 |
|---|---|---|---|
| 传感器工程 `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output` | Git tree `546162704fd33dd9c74bb7c0066c1aa10ccdab1a` @ 接手 HEAD | 本次图查询确认采样、光照、判据、调试；历史 `evidence/build.md`、`evidence/ev017/image_identity.txt` 记录 Keil 构建及镜像 MD5 `5fc16235ccd043f4edb48b16ee44da0b` | 当前未重建/烧录，板上身份未知；复用驱动和诊断，增量修正冲突及复验，不能重写报告声称修好固件 |
| 网关工程 `CH592EVT/EVT/EXAM/BLE/beiwov2` | Git tree `22c8a193daffea87408c971a53db566633f7c6ea` @ 接手 HEAD | 本次图查询定位 `APP/feistel_al.c:119`、`APP/bleencrypt.c:42/66` 的解码/组块；原有接口契约 | 无当前板上版本或空口证据；兼容依赖，不要求重新实现或刷写 |
| Android `dengbei_care` | Git tree `3c9feeb9ce399b3ba5ac359b31090af583ce4781` @ 接手 HEAD；源码 versionCode=9/versionName=1.8，minSdk26/target34 | 当前权限/连接/上传函数已核对；`evidence/software_test.md` 尾部 ST-014R 引用 `08f4592`，历史 188 tests/0 failure；保留 ST-010、ST-014 首次失败及复验记录 | 本次未构建、未核验 APK hash/签名、未安装；复用已有模块，先处理权限和网关身份再真实验证 |
| 实板历史捕获 `evidence/ev017/{r1,s1,s2,s3}.cap` | 2026-10-09 UTC 05:29:11–05:49:13；原文记录 uid=6A002C00，历史 COM42；本次重读原始字节并计算摘要 | 7 个 S 行 q=0；G 行 a44=0/a45=0/rd=0/at=3；BUS ack=none；VDD 3289 mV；原始镜像/窗口记录同目录 | 历史失败保留；当前登记 COM43，不沿用 COM42 操作；MCU 侧电压不能证明 U9 焊点供电、瞬态或所有供电故障已排除，更不能断言器件损坏 |
| Android 探索 `artifacts/android_feasibility.md` | Git blob 所属提交 `bf44dd7`；报告检查日期 2026-10-27；本次附录摘要 | 原始只读查询保存在报告 §2：Redmi8A/API29、BLE feature 存在；当时应用未安装、蓝牙/定位关闭、无默认网络；§3 源码权限门与固定 MAC | 均是旧时点事实，当前状态未查询；不能把模拟器/API34/旧1.7或仓库1.0 APK当作当前真机候选。报告中 Android 各版本平台规则未经本次外部核对，不作为新增验收事实 |
| 既有服务；本地资料 `服务器迁移记录.md` | 用户确认已部署；远端位置/版本未在本次登记核验；本地文件附录摘要 | 文档声明 EMQX 8883、HTTP5000、MySQL，以及 UNIQUE(dev_id,time)/INSERT IGNORE；记录历史测试 | 仅资料存在，不确认远端不可变版本。未连接服务或 DB；先由获准入口确认真实候选与回读，不安排默认重部署 |
| 接口设计 `artifacts/interface_contract.md` | IC-002 v3.0；附录摘要 | 定义升温、35℃、全暗比例和兼容布局，可复用规范与向量 | 文件含旧实现描述，不能当作当前实现状态；当前核对已证明判据仍冲突，光照结构化结果则已存在，不重复安排其重做 |

## 6. 环境、授权、安全与持续限制

环境及授权以本次注入的 Unified 汇总为准，来源分别为项目 Runtime 的 environment.yaml 和 permissions.yaml；本报告仅引用该快照，不建立第二套配置。配置已登记不等于设备在线或业务通过。本次角色为系统架构，不主动操作硬件；设备工具/外部动作/MCP 授权为空，环境自主修复关闭。

| 入口 | 当前条件 | 本次处理 |
|---|---|---|
| 传感器 COM43，9600 8N1 | 串口入口启用，当前能力缺 serial_capture | 未打开；COM26 是调试器串口；历史 COM42 不可替代 |
| Keil/CW-DAPLink | 编程入口登记但缺 mdk_flash、flash_device | 未构建/下载/复位；不由 shell 绕过授权 |
| Redmi 8A / 12f046690506 | 只读 ADB configured_unverified；操作测试缺 android_test | 本次无需刷新身份，未调用；安装、BLE/UI 和业务测试由获准责任方执行 |
| 网关 | 由 App 正常发现连接；身份未确认，无独立管理入口 | 固定 MAC 是实现事实而非授权或实物身份；不猜测地址、不修改网关 |
| FX2 被动采集 conn=2.31 | 缺 sigrok_capture、operate_test_equipment；本轮未枚举 | 未采集；D0=SCL/D1=SDA及共地为登记接线；历史地址2.19不可沿用，重插后需更新配置 |
| 已部署服务 | 无登记的管理/重部署/DB直连授权入口 | 文档内地址/凭据不自动形成可执行入口；只记录核验和配置缺项，不排除服务器总测 |

硬件资料/旧捕获称 I2C 位于 PA03/PA04，而仪器登记写 PB0/D0、PB1/D1；其是否指测试点/接线映射尚无本次证据。下次采集前由负责人员核对实际连接和配置，不能擅改固件引脚或猜测接线。数字捕获不能证明电平兼容、上拉质量或供电稳定。

保留安全事实：HTTP 5000 明文承载业务字段，迁移记录称 TLS 仅校验 CA 而不校验主机名；本次未做安全测试，也不扩大为协议重建。不得把这些描述成安全认证已通过。报告不复制密码、密钥或管理凭据。SQLite/服务器秒级(dev_id,time)幂等可抑制相同键重试，却不能识别不同接收秒的缓存复读，且同秒真实读数存在冲突风险。

持续限制：真实遮光、真实温度刺激和明确操作窗口仍需记录；有条件的真实整体验收不能用模拟替代。不终止占用设备的无关进程；后续每次候选变化均重新对应版本、板上/手机身份和测试结果。历史脚本含 COM42，仅作历史资产，不能据此选择本轮设备入口。

## 7. 可继续范围、必要决定与交接

| 优先事项 | 可继续的工作 | 必要条件/责任 |
|---|---|---|
| 固件需求冲突 | 修正 P−T 为 T−P，并取消 35℃对升温分支的整体排除；同步相关预期、命名/文档 | 固件负责实现；嵌入式测试独立检查边界，不因历史测试通过保留错误断言 |
| GXHT40 历史失败 | 利用既有 BUS/G/VDD 证据继续定位地址应答与有效采集；本次不重复提出已存在诊断的制作任务 | 固件/硬件相应责任方确认实际原因；未知原因不等于不可修复，不预先终止重试 |
| 无光分支 | 取得全暗原始分布和当前板标定值并绑定配置版本 | 真实遮光条件及获准采集；仅回填有证据的数值，不把0改成猜测常数 |
| Android 连接 | 处理 API<31 权限门与正常用户流程不一致；落实按正常发现/绑定确认网关身份 | Android 负责；具体 UI/绑定方案由其在现有产品边界内设计，本报告不擅自批准固定地址替代发现 |
| 服务复用 | 确认现有部署版本、上传接口/认证/幂等和可回读机制 | 需要登记且获准的核验入口；管理/重部署/数据库直连权限未知，不能从迁移资料推导 |
| 候选与验收身份 | 固件构建/镜像、APK hash/签名、实际网关身份、服务版本统一进入测试记录 | 对象变更后刷新对应记录和测试；新报告摘要本身不修复任何引用对象 |

总体集成目标始终保留：真实有效温湿度/光照 → 正确条件及静默 → RF 实收 → 正确网关缓存/BLE → 当前手机按 ID 显示/告警/本地落库 → MQTT/HTTP 上传 → 既有服务持久化与回读逐字段一致。至少覆盖升温阈值两侧、35℃边界、明/暗/无效、首笔/失败/重启、多设备、缓存重复读、网络中断重试、身份切换和手机息屏后的节拍。既有告警与传感器上报是不同语义，不能把一次上传当作报警正确。

已有宿主逻辑、编解码、上传待发箱、文案和测试设计可复用，不列重复制作任务；需专业责任方核对其是否仍对应当前对象，仅对变化和未测层补验。即使各组件单项都完成，仍必须完成上述真实集成链路和总测。这里列的是分析依赖与请求交接，不创建流程节点、不代替实现、验证或 Runtime 作路由决定。

## 8. 本次完成及限制

已完成：八项需求映射、组件/接口边界核对、当前源码冲突定位、历史失败及局部 PASS 的版本化保留、资源和授权缺口登记、可继续范围与端到端验收目标。未完成的产品工作逐项见上表；本次分析任务本身无未完成交付项。

本次仅做静态读取和证据核对，没有执行历史测试脚本，没有把历史错误成因判为唯一结论。只读命令均已完成，未启动后台任务或长时服务；不自行提交或推送。摘要首次追加时 PowerShell 的 AppendAllLines 重载绑定失败（进程退出码0但错误流非空），未写入摘要；已改用字符串追加重新完成，不以进程退出码掩盖该错误。最终文件检查只验证本次报告/清单与版本记录可读，不构成产品功能验证。

## 附录：本次确认对象的文件摘要

以下由本次本机 SHA-256 计算追加。检查时点随表记录；文件摘要标识本次确认记录，远端版本与设备镜像仍按各自未核验项处理。组件完整已跟踪版本以第5节 Git tree 为准。

检查时点：2026-11-08T11:10:51.6719216+08:00

| 文件 | SHA-256 |
|---|---|
| readme.txt | 487c1837f735c1e75b533665a16d6a9dd444bba2887c70e24ab4ff6a4fab7d61 |
| artifacts/interface_contract.md | 6af5f0a65c819033390e94099c77ede83af53b6a2750edf55debd9fe42d2ff22 |
| artifacts/android_feasibility.md | 657f9c5ebb16af3c95dea46f1d9a275ffd40d4c70cb01bd9da5def31cdce8ab3 |
| evidence/test.md | d09c537a6c8ce74e12f49d840d00942f6fe9300f53807f18293d40b425e00644 |
| evidence/build.md | 187d35a6d811e488b2d9e8cec77e392f8287e0c29a56ef1ad35ac426108ecb06 |
| evidence/software_test.md | 8fc9fffbf27bbb4f0518f987cd01a732febc9d6ef49ed40b604e4afcbd224671 |
| evidence/ev017/image_identity.txt | a19cadb1162e1c14baf6d4cd0b0b1f01fe0bb3f192ea0f10c421110166219d61 |
| evidence/ev017/summaries.txt | 13a79db6070bf85eb8d431f6d150aa6f4b23d4b47f897c6ea001fe8f757b2711 |
| evidence/ev017/r1.cap | 6c8e9324631782307333551a165707e0606495fe86eb86e35c5d40e46f41e188 |
| evidence/ev017/s1.cap | 870a1e3c7e94be1c7d60a8b459c1eef10b33d924319515e355dbe0bc9875d523 |
| evidence/ev017/s2.cap | 237b1e247901fcf8905ba9e46697609eb4f9d79b187f7d9e9e4b3b749f6f3da7 |
| evidence/ev017/s3.cap | 1976b7c54348cdc1602d0addbd702b58dc8ae9bf44e3f305f25df5a3f59455a4 |
| 服务器迁移记录.md | e883e4df73b109dd7ac08c862e350ed9b11372a9e073930dd7bcf9e4749811ed |
| sensor_hardware/pstxnet.dat | 5e0734950da222860ca55ef1e8dc7f5c6ac9ea25bd51c745d9c9afea7d6f19ac |
| sensor_hardware/MAIN_BOARD.BOM | 2dfc8ddd0fd74a5bfc4a1071aff00841f20ed15c4dd923ff51ab0b650c0f0f85 |
| CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/fw_core.c | 7d369ecf6143d2543282183679a208078c3688bb9ecc7a7d465e9eb4d1a3e4e3 |
| CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/measure.c | 8c7d64c415ad7ebe2db9628f4a9b9ffa607b9bccecb9f57326523c77f76b8dcc |
| CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/light.c | e45bd25f80ac2cc3678666dab5c4d4a1737ed65e70bbc3bbf69cc4586054dae8 |
| CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/debug_trace.c | 180325133b651cdc3139e3ffb4d661eac11b0123f8cad5cd8246a72587509573 |
| CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/inc/sensor_config.h | e4ed7c5b1c7ecd80ff152eef9a4ee86f39878eaf7457382d7bf53ad6c5f62aec |
| CH592EVT/EVT/EXAM/BLE/beiwov2/APP/feistel_al.c | b1c14ae82067a4ef5ba69d3c6bd04aa8fdec7524a07a58d28cf5c5df2d6ad300 |
| CH592EVT/EVT/EXAM/BLE/beiwov2/APP/bleencrypt.c | 982060bfab94ee6d93c3b91942c41802d84adc22cddf87f4196166af7903d2a6 |
| dengbei_care/app/build.gradle.kts | 9130aec418832e5859f57d1d8de98c3b86071bab87349cf50acde178f96bb75c |
| dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/MainActivity.kt | 1ba0302584f9cfb4db27eee0da678758219e2f38ae419d3a3a5b88d40209d849 |
| dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt | dd3847ff2fec80289f0e82282140fab77b8f0c78e85e59ff6a94040b934a88ac |
| dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt | c221de8da8d78ecc4b41e341daca69ece754fe8e18fd95920cdb065612b84a1c |
