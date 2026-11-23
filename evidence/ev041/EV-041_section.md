# 嵌入式实板验证证据（EV-041 / E2E-JOINT-P0 嵌入式段）

负责角色：嵌入式测试
被测对象：当前传感器固件（产品源 = 实现提交 `82a7e74` 的 `USER/MDK/COMMON/UM2005C` 目录，未改动）；发布包 `fw_release/fw_sensor_FD-002r8.zip`
用例：`E2E-JOINT-P0`（身份、手机联网与接收就绪）——**本文件只判定其中“嵌入式”段**，对应设计 `artifacts/e2e_joint_test_design.md` §4.1 / §8 与 `artifacts/test_design.md` TD-002 rev 10.0 §5.1（EM-01）、§6.1（J-00）的嵌入式行。
本轮方式：重新枚举端口/USB → 独立重建 → 工程保存配置下载（`mdk_flash {}`，Verify OK）→ 端口先开后下载复位 → COM43 9600 8N1 采集上电五行与采样行。
原始证据：`evidence/ev041/`（`tool_results.txt`、`ev041_boot_capture.cap/.log`、`ev041_serial_capture_k3.cap`、`capture_analysis.txt`、可复跑脚本）。

## 0. 结论（限定本段）

**嵌入式段 `TEST_PASS`**（仅限：当前固件身份 + 上电诊断五行 + `uid4` + 采样闸门字段）。

| 判据（E2E-JOINT-P0 嵌入式行） | 实际 | 结论 |
|---|---|---|
| COM43 9600 8N1 | COM43 = Prolific `067B:2303`（LOCATION=1-9.4.2）；8N1@9600 | PASS |
| 有序 `BOOT/VDD/IOTEST/IOSIG/BUS` | 下载复位后同一窗口按序各一次 | PASS |
| `uid4` 可读 | `BOOT … uid=6A002C00` | PASS |
| 至少一条 `S q=1 r=0 s=0` 且无 `P` | `S k=0`、`S k=3` 均 `q=1 r=0 s=0 y=0`；无 `P`/`G` | PASS |
| 记录当次构建与下载身份 | 重建 axf `b6a15fed…`/hex `4d728dec…` 与发布包逐字节一致；`mdk_flash` `Verify OK` | PASS |

**明示未执行（不属于本能力，不得写成通过）**：手机段（设备/APK 摘要、Wi-Fi `ZTE_607157`、蓝牙/定位权限、守护、网关发现/绑定、0xA1/读取）、平台段（只读基线/回读）、N1 与 P1 全用例。本文件不把它们判为通过，也不把本段 PASS 扩大为整用例或产品通过。

## 1. 身份与环境（本轮实际执行）

| 项 | 值 | 来源 |
|---|---|---|
| 仓库 HEAD（开工） | `808107d`；`git status --porcelain` 仅 `?? evidence/ev041/` | `tool_results.txt` §1 |
| 固件产品源 | `git diff --stat 82a7e74..HEAD -- <sensor>/{USER,MDK,COMMON,UM2005C}` 为空 | 同上 |
| 目标器件 | CW32L010F8（Vendor WHXY），Keil Target `Project` | `Project.uvprojx` |
| 调试串口 | COM43 Prolific USB-to-Serial `VID:PID=067B:2303`，LOCATION=1-9.4.2 | `serial_list_ports` |
| 下载探针 | CW-DAPLink `VID:PID=C251:F001`，SN `87094109484987710672FF50` | `usb_list_devices` |
| 烧录入口 | Keil MDK + 工程保存的 CMSIS-DAP / `FlashCW32L010.FLM`；`mdk_flash {}` | 工程 uvoptx |

> 说明：工程 `BOOT` 横幅内的 `fw=FD-002r4` 是 `USER/src/debug_trace.c:214` 编译进去的固件标识串（EV-039/EV-040 同值）；发布包名 `FD-002r8` 是打包/发布修订号。两者来自同一未改动的产品源 `82a7e74`，不是两套固件。

## 2. 构建、下载与镜像一致性

| 项 | 结果 |
|---|---|
| 工具链 | Keil ARMCLANG **V6.24**，工程保存配置 |
| 增量构建（下载前） | 0 Error / 0 Warning（未触发重新编译） |
| **独立全量重建** | `Rebuild target 'Project'`：**0 Error / 1 Warning**（既有 `USER/src/main.c(254): while(k--);` 空循环体）；`Program Size: Code=25964 RO-data=656 RW-data=116 ZI-data=1676` |
| axf 身份 | `b6a15fed2ed5e3d0c4adc51ac87a4830`（297,380 B） |
| hex 身份 | `4d728dec9b0d28a352c46436b7586c72`（75,275 B） |
| 与发布包比对 | 与 `fw_release/fw_sensor_FD-002r8.zip` 内 axf/hex 成员**逐字节一致**（包 md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf`） |
| 下载 | `mdk_flash {}` 2026-11-23T08:13:11.758Z–08:13:24.274Z：`Erase Done.Programming Done.Verify OK.Application running ...` |
| 重建后可复现性 | 重建（08:18:56Z–08:18:58Z）后 axf/hex md5 仍为上述值 |

> `Verify OK` 只证明下载内容回读校验一致，不表示业务功能合格。

### 2.1 工具级瞬态（如实记录，非固件缺陷）

首次 `mdk_flash {}`（08:12:16.728Z–08:12:18.952Z，exit 2）在加载调试器驱动阶段失败：

```
Cannot load driver 'C:\Keil_v5\ARM\BIN\CMSIS_AGDI.dll'.
Error: Flash Download failed  -  C:\Keil_v5\ARM\BIN\CMSIS_AGDI.dll
```

`CMSIS_AGDI.dll` 实际存在（2,410,776 B，可读），无 UV4 实例占用。**未修改任何工程/配置/权限**，同参数重试第 2 次即成功；错误未复现。按测试环境有界自修复处理（共 2 次尝试，未超 3 次上限），根因未确定，仅记录为工具级瞬态。

## 3. COM43 上电诊断与采样（端口先开、下载复位触发）

测试侧夹具 `ev041_boot_capture.py` 先打开 COM43（08:12:13.553Z 起，120.062 s，**只接收、不发送**），随后 `mdk_flash` 在同一窗口完成下载和复位。原始字节 `ev041_boot_capture.cap`（451 B，md5 `6ea1307a62609aa3bc224e1d5e04d8da`）。

### 3.1 下载复位后的完整上电五行（08:13:24.247Z 起）

| 顺序 | 原始行 |
|---|---|
| 1 | `BOOT fw=FD-002r4 uid=6A002C00 rst=0240 uart=9600` |
| 2 | `VDD ok=1 code=1480 bgrmv=1189 mv=3289` |
| 3 | `IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none gc=0` |
| 4 | `IOSIG scl=95555 sda=30300` |
| 5 | `BUS idle=3 scl=1 sda=1 ack=44` |

- VDD 复算：`4095*1189//1480 = 3289`，与 `mv=3289` 一致（源码公式 `debug_trace.c:357`）。
- `BUS ack=44` 为 GXHT40 地址；`gc=0` 表示未用 general call 复位器件。
- 首次下载连接时的引导（08:13:14.059Z，`rst=0040`）被随后编程停顿截断在 `IOTEST` 行中段（原始字节中可见 `…IOTEST sd` 直接接下一次 `BOOT`）。这是下载编排的正常现象（编程期间核被 halt），**不是固件缺陷**；完整有序五行取自上电/复位完成后的最终启动。

### 3.2 采样行与闸门

| 时点（UTC） | 原始 `S` 行 | 说明 |
|---|---|---|
| 08:12:59.651 | `S k=14367 p=322 H=1 V=0 o=8 n=3743 x=3750 a=3747 D=0 t=315 h=266 q=1 r=0 s=0 y=0` | 下载前板上旧运行状态（连续运行约 10 天）；用于确认采集链路 |
| 08:13:25.686 | `S k=0 p=0 H=0 V=0 o=8 n=3748 x=3754 a=3750 D=0 t=315 h=267 q=1 r=0 s=0 y=0` | 下载复位后首个采样机会 |
| 窗口 08:14:52.736–08:17:52.958（MCP `serial_capture`，180 s） | `S k=3 p=315 H=1 V=0 o=8 n=3756 x=3763 a=3759 D=0 t=312 h=269 q=1 r=0 s=0 y=0` | 下一采样机会（`p=315` 与 `k=0` 的 `t=315` 链一致），原始字节 `ev041_serial_capture_k3.cap`（78 B，md5 `bcd41f957f25c82607bcfa2fb2952665`） |

- 两个新窗口采样均 `q=1`、`r=0`、`s=0`、`y=0`；无 `P`（冻结/上报）行、无 `G`（周期失败）行。
- `V=0 D=0` 仅表示交付默认 `LIGHT_DARK_THRESHOLD_CODE=0` 判据未启用，**不得**解释为“已识别有光”。
- 未施加温度/光照刺激；未调温、未改光强、未遮光。

## 4. 断言结果

`analyze_ev041.py` 对原始字节独立解析（不引用固件常量），结果见 `capture_analysis.txt`：

- 完整上电组 `BOOT<VDD<IOTEST<IOSIG<BUS` 各一次且有序：PASS。
- `BOOT` 字段、`fw=FD-002r4`、`uid=6A002C00`、`uart=9600`：PASS。
- `VDD ok=1` 且 `mv==4095*bgrmv//code`：PASS。
- `BUS ack=44`：PASS。
- 采样行 `q=1`、`r=0`、`s=0`（k=14367、k=0、k=3）：PASS。
- 无 `P`、无 `G`；最长行 80 B ≤ 96 B+CRLF：PASS。
- **RESULT failures=0**。

## 5. 未执行与边界（不得写成通过）

| 项 | 状态 | 原因/归属 |
|---|---|---|
| 手机段：设备/API、APK SHA-256、蓝牙/定位/权限、Wi-Fi `ZTE_607157`、守护、网关发现/绑定、0xA1/读取 | **NOT EXECUTED** | 非本能力；由手机段执行能力按 §4.1 执行 |
| 平台段：只读服务/端口/日志/连接数基线、行回读 | **NOT EXECUTED** | 非本能力；由平台段执行能力执行 |
| N1 负向（≥3 机会、手机/平台无新上报） | **NOT EXECUTED** | 独立用例，本能力仅完成 P0 嵌入式段 |
| P1 受控注入（测试构建/TINJ/冻结/发送） | **NOT EXECUTED** | 依赖固件实现提供测试构建；当前产品源无 `SENSOR_TEST_INJECT*`/`TINJ` |
| 客观 L3 位级 I²C | NOT EXECUTED | 可选补充；本轮未用于判定 |
| 真实升温/>35℃、改变光强、功耗、433 电气 | NOT EXECUTED | 用户不可提供的条件/已批准延期；不属 P0 |

## 6. 关联键（供后续段引用）

| 键 | 值 |
|---|---|
| 传感器 `uid4` | `6A002C00` |
| 固件镜像 | axf `b6a15fed2ed5e3d0c4adc51ac87a4830` / hex `4d728dec9b0d28a352c46436b7586c72` |
| 发布包 | `fw_sensor_FD-002r8.zip` md5 `5df0c6d45ecddc8aae8bddbbcfc9cfdf` |
| 采集窗口 | 2026-11-23T08:12:13.553Z–08:14:13.619Z；08:14:52.736Z–08:17:52.958Z |
| 首样本 / 次样本 | `S k=0 t=315 h=267`；`S k=3 t=312 h=269` |

## 7. 证据索引

| 文件 | 内容 |
|---|---|
| `evidence/ev041/tool_results.txt` | 端口/USB、构建/下载原始日志（含首次失败）、夹具与 MCP 采集原文、哈希 |
| `evidence/ev041/ev041_boot_capture.cap` / `.log` | 451 B 原始串口字节与逐块时间戳（跨下载复位） |
| `evidence/ev041/ev041_serial_capture_k3.cap` | MCP `serial_capture` 取得的 `S k=3` 原始 78 B |
| `evidence/ev041/capture_analysis.txt` | 独立解析断言（0 失败） |
| `evidence/ev041/ev041_boot_capture.py` / `analyze_ev041.py` | 可复跑测试夹具与解析脚本（测试侧，不入发布包） |

## 8. 结论与交接

- **嵌入式段 `TEST_PASS`**：身份闭合、构建/下载一致、上电五行有序、`uid4` 可读、采样闸门 `q=1/r=0/s=0` 无 `P`。
- 未发现本段固件功能缺陷。首次下载的调试器驱动加载失败为工具级瞬态，重试即成功，已如实记录，未改产品/工程/配置。
- 后续：手机段按 §4.1 完成设备/APK/Wi-Fi/权限/守护/网关绑定与读取；平台段完成只读基线。本文件不代替这些段，也不把本段 PASS 扩大为整用例或产品通过。
