# 嵌入式验证证据：运行中固件身份核查（不改板、不烧录）

- 证据编号：`EV-EMB-IDENT-20261126`
- 对应工作项：`verify_running_firmware_identity_without_flashing`（本能力的“先核对实际运行固件版本”步骤）
- 负责角色：嵌入式测试
- 结论：**本项 `TEST_PASS`（仅限“运行固件身份与当前产品源/发布镜像一致”这一核查项）**
- 判定时间：2026-11-26T15:15Z–15:27Z（本地 UTC+8）
- 未烧录、未复位、未断电；板件运行状态未被本能力改变。

> 本文件只判定“板子当前跑的是什么固件”。传感器→网关→手机显示/报警/停止/守护等业务功能、App 安装与身份、平台/云/数据库/iOS 均**不在本项内**，未执行、未判定为通过（见 §6）。

---

## 1. 被测目标与工具身份（本轮重新枚举）

| 项 | 值 | 来源 |
|---|---|---|
| 传感器板 / MCU | CW32L010F8（Vendor WHXY），Keil Target `Project` | `MDK/Project.uvprojx` |
| 传感器调试串口 | **COM43** = Prolific USB-to-Serial `VID:PID=067B:2303`，`LOCATION=1-9.4.2`，9600 8N1 | `serial_list_ports` 2026-11-26T15:15:02Z |
| 下载/调试探针 | CW-DAPLink `VID:PID=C251:F001`，SN `87094109484987710672FF50`（本轮未使用、未访问） | `usb_list_devices` 2026-11-26T15:15:02Z |
| 逻辑分析仪 | Cypress FX2 `VID:PID=04B4:8613` 在位（本轮未探测、未采集） | 同上 |
| 手机（仅环境枚举） | Redmi 8A `12f046690506` 在线；`pm list packages` 无 `com.jinyuni.dengbei_care` | `adb devices -l` / `pm list packages` |
| 工具授权 | 仅本项目已授权的 `serial_list_ports` / `usb_list_devices` / `serial_capture` 与只读 ADB 枚举 | 本能力工具清单 |

> 设备枚举只证明连接/身份在位，不证明业务功能。COM26 是调试器 CDC，不是传感器调试口。

---

## 2. 仓库与镜像基线（只读复算）

| 项 | 值 |
|---|---|
| 仓库 HEAD | `635ab5a63e76187719a512ba72fc5d03460966ea`（2026-11-26 23:11:15 +0800）；开工 `git status --porcelain` 为空 |
| 固件产品源 | 相对实现提交 `82a7e74`，`git diff --stat 82a7e74..HEAD -- <sensor>/{USER,MDK,COMMON,UM2005C}` **为空**（82a7e74 之后只有 `test/` 宿主测试文件变化） |
| 发布包 | `fw_release/fw_sensor_FD-002r8.zip` 134,791 B，MD5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`，SHA-256 `16ad3f69a9c83fc4984cf6efa2d9c0f43b5c5498716b5f253df090ef955bf4d0`（本轮复算一致） |
| 包内 axf | `fw_sensor_FD-002r8_b6a15fed.axf` 297,380 B，MD5 `b6a15fed2ed5e3d0c4adc51ac87a4830` |
| 包内 hex | `fw_sensor_FD-002r8_4d728dec.hex` 75,275 B，MD5 `4d728dec9b0d28a352c46436b7586c72` |
| 包内 `PACKAGE.txt` | 3,202 B，MD5 `a58f67fcf5b760767ff5b65888c73dfb` |
| 上一次已确认下载（历史证据 EV-041） | 2026-11-23T08:13:11.758Z–08:13:24.274Z，`mdk_flash {}`：`Erase Done. Programming Done. Verify OK. Application running ...`；下载镜像即上述 axf；同窗口 `BOOT fw=FD-002r4 uid=6A002C00 rst=0240 uart=9600` |

> `BOOT fw=FD-002r4` 是源码内冻结横幅字符串（`USER/src/debug_trace.c:214`），不是版本号；发布修订号由包名 `FD-002r8` + 镜像摘要共同标识。历史 `Verify OK` 只证明当时下载内容回读一致。

---

## 3. 本轮实际刺激与观测

本轮**不烧录、不复位、不断电**。方法：在 COM43 9600 8N1 上做两段有界被动采集，读取固件每 3 分钟主动输出的 `S` 行，并与当前源码的格式/语义、以及上次已验证下载后的连续运行时间做交叉核对。

| 窗口 | 端口/参数 | 起止（UTC） | 收到字节 | 原始文件 |
|---|---|---|---|---|
| W1 | COM43 9600 8N1，300 s | 15:17:08.477–15:22:08.692 | 162 B（2 条 `S`） | `evidence/embedded_verify_identity_20261126/capture_w1_300s.cap`（SHA-256 `51548965a2ea00569625f2228d2a2928cc8a4d2da7f9c6446409c403698cc452`） |
| W2 | COM43 9600 8N1，200 s | 15:23:19.919–15:26:40.127 | 81 B（1 条 `S`） | `evidence/embedded_verify_identity_20261126/capture_w2_200s.cap`（SHA-256 `e67ab2fd2741ab57d12bd449e947f341d0e7354b3bd6cbdaa7a7769dfbf76f23`） |

原始字节（逐字节，含 `\r\n`）：

```text
W1: S k=4752 p=364 H=1 V=0 o=8 n=3947 x=3950 a=3948 D=0 t=350 h=242 q=1 r=0 s=0 y=0
    S k=4755 p=350 H=1 V=0 o=8 n=3947 x=3951 a=3948 D=0 t=341 h=251 q=1 r=0 s=0 y=0
W2: S k=4758 p=341 H=1 V=0 o=8 n=3945 x=3950 a=3947 D=0 t=336 h=258 q=1 r=0 s=0 y=0
```

解析与计算可由 `evidence/embedded_verify_identity_20261126/parse_identity_capture.py` 复跑（输出已存 `capture_analysis.txt`）。工具原始返回、时间戳与发布包复算见同目录 `tool_results.txt`。

---

## 4. 判据与逐项结果

| # | 判据（本核查项） | 期望 | 实际 | 结论 |
|---|---|---|---|---|
| C1 | 目标板在线且在运行固件并采样 | 采样间期静默、每 180 s 一条 `S` | 两个窗口共 3 条 `S`；`k` 步长 4752→4755→4758，均为 **+3**；行间静默 | **PASS** |
| C2 | `S` 行格式与当前源码一致 | `USER/src/debug_trace.c` 冻结字段顺序 `k p H V o n x a D t h q r s y` | 三行逐字段一致；行长 81 B（≤96 B） | **PASS** |
| C3 | 采样有效性 | 当前板件/接线应 `q=1` | 三行均 `q=1`、`o=8`（=`LIGHT_ADC_SAMPLES`）、`h` 在 10.0–100.0 %RH、`H=1` | **PASS** |
| C4 | 判定语义与当前源码一致（默认交付配置） | `LIGHT_DARK_THRESHOLD_CODE=0` ⇒ `V=0 D=0`；`t>350` 才 `r=1`（严格大于，恰好 350 不报） | `V=0 D=0`；`t=350 → r=0`、`t=341 → r=0`、`t=336 → r=0`；`s=0 y=0`；无 `P`/`G` | **PASS** |
| C5 | 连续运行、无中途复位/重烧 | 连续采样时 `k` 递增且相邻样本 `p` 等于上一样本 `t` | `p` 链：`364→t350`、`p350→t341`、`p341→t336` 完全衔接；`k` 连续 +3 | **PASS** |
| C6 | 运行会话与“最后一次已验证下载”时间一致 | 自 EV-041 下载复位起连续运行，`k`≈下载后经过分钟数（RTC LSI 容差内） | EV-041 下载结束 2026-11-23T08:13:24Z 至 W1 窗口结束经过 **4748.74 min**；首条 `k=4752`，偏差 **+3.26 min / +0.069 %** | **PASS** |
| C7 | 运行镜像与当前发布包一致 | 无证据显示 82a7e74/FD-002r8 之后有产品源或镜像变化；无重烧/复位事件 | 产品源相对 82a7e74 无差异；发布包哈希复算一致；`k` 连续说明 EV-041 之后无复位（任何重烧/下载都会复位并把 `rtc_tick_total` 清零） | **PASS（见 §5 证据边界）** |
| C8 | 本轮不改变板件状态 | 不烧录、不复位 | 未调用 `mdk_build/mdk_flash`；未改工程/权限；板件连续运行 | **PASS** |

---

## 5. 证据边界（明确写出，避免过度声明）

1. **“运行镜像字节”不是本轮直接回读得到的。** 本轮无授权的 Flash 回读入口（`mdk_flash` 会先编译再下载，属烧录，本项明确不做），且主动复位才能取得一次性 `BOOT/uid`；因此身份结论由三部分交叉支撑：①最后一次 `Verify OK` 下载的镜像摘要（EV-041 历史原始日志）；②`rtc_tick_total` 是从 0 开始的 RAM 计数器，`k=4752` 与下载后经过时间在 LSI 容差内吻合，说明此后无复位/重烧；③`S` 行格式、`V/D` 禁用、严格 `>350`、`o=8`、3 分钟节拍与当前源码逐项一致。
2. **`BOOT/uid` 本轮未重新取得**：不再复位即无法打印上电五行；板件 `uid=6A002C00` 沿用 EV-041 同一次未中断运行的记录，非本轮新读。
3. **`t`/`h` 只作采样事实记录**：真实环境温湿度，不构成被窝场景或物理量校准结论；`V=0/D=0` 仅表示判据未启用，不代表“已识别有光/无光”。
4. **串口只证明固件侧采样/判定/打印**；不证明 433 发射成功、网关接收、手机显示或报警。
5. **未使用逻辑分析仪**：本项不依赖 I²C 波形；不把未采集写成“总线正常”。

---

## 6. 本项未执行 / 不属于本项的部分

| 项 | 状态 | 原因 |
|---|---|---|
| 传感器→网关→手机显示、正常不误报、异常报警、反复报警复现 | NOT EXECUTED（本项） | 属后续真实设备用例节点；且当前交付固件在“有光、常温、`LIGHT_DARK_THRESHOLD_CODE=0`”下不会自然产生 433 上报（详见 `artifacts/embedded_test_design_alarm_chain.md` §3.2、§4） |
| 点“停止”后铃声/震动停止、停止守护、重启守护后新事件报警 | NOT EXECUTED（本项） | 同上；还需 App 侧测试与注入/强制上报测试构建 |
| App 安装与已安装字节身份 | NOT EXECUTED（本项） | 由独立节点 `verify_installed_android_identity_without_installing` 负责；本轮只记录“未安装”这一环境事实 |
| 主动复位取 `BOOT/VDD/IOTEST/IOSIG/BUS` 与 `uid` | NOT EXECUTED | 本项要求“不改板、不烧录”，且授权工具中没有独立的复位入口；为保住连续运行证据（`k`）也不应复位 |
| Flash 逐字节回读 | NOT EXECUTED | 当前授权工具无读回入口；不另选未登记的 shell/调试器入口访问目标 |
| 平台/云上传/数据库/iOS | NOT EXECUTED | 明确不在项目范围 |
| 调温、调光、功耗测量 | NOT EXECUTED | 用户明确当前不能提供这些条件 |

---

## 7. 结论与后续依赖

- **本核查项结论：`TEST_PASS`** —— 目标板当前运行会话与最后一次经 `Verify OK` 下载的发布镜像 **FD-002r8**（axf MD5 `b6a15fed…`，产品源提交 `82a7e74`，发布包 MD5 `5df0c6d4…`）一致；运行行为与当前源码逐项吻合；未发现运行未知/旧固件的迹象；本轮未烧录、未复位。
- **不扩展为产品通过**：显示、报警、重复报警、停止、守护、完整链路均未在本项执行，属后续真实设备用例与 App 验证；不得据本文件声称这些功能已通过。
- **后续用例的执行依赖（事实记录，不代为执行）**：要在现有“有光、常温、判据未启用”条件下覆盖正向链路与报警，需要固件实现提供默认关闭、可从正式构建排除的测试专用注入/强制上报构建（`artifacts/embedded_test_design_alarm_chain.md` §4）；并在真机上安装被测 App。若后续节点未取得该测试构建，相关用例应记 `NOT EXECUTED`，不得写成通过。

---

## 8. 原始证据索引

| 文件 | 内容 |
|---|---|
| `evidence/embedded_verify_identity_20261126/tool_results.txt` | 全部工具原始返回、时间戳、仓库/发布包身份、未执行操作清单 |
| `evidence/embedded_verify_identity_20261126/capture_w1_300s.cap` | W1 原始串口字节（162 B） |
| `evidence/embedded_verify_identity_20261126/capture_w2_200s.cap` | W2 原始串口字节（81 B） |
| `evidence/embedded_verify_identity_20261126/capture_analysis.txt` | 解析、节拍、`p` 链、运行时间一致性、判据签名 |
| `evidence/embedded_verify_identity_20261126/parse_identity_capture.py` | 可复跑解析脚本（只读） |
