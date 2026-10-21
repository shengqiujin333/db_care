# 嵌入式验证证据 EV-029 — 固件实现完整交付独立验证（FirmwareImplementation）

- 能力：`embedded_tester.embedded_verification`
- 项目：dengbei_run10
- 验证范围：`complete_delivery`（n012 `firmware_engineer.firmware_implementation` 的完整交付，ITEM-001..008）
- 受测输入：`artifacts/firmware_implementation_v10.md`
- 结论：**`TEST_FAIL`**
  - **失败项**：PA-001（真实温湿度采集）在实板上完全不可用：同一启动会话中观测到的 4 个采样机会 **4/4 `q=0`**（k=0/3/15/18；窗口之间的 k=6/9/12 未采集），`G s=2 a44=0 a45=0`，`BUS ack=none`，未达到“≥3 连续周期 `q=1`”的最低要求。
  - **已通过**：本轮可执行的宿主独立验证（门控/快照/冻结发送/UART 有界/主流程/标定门禁/驱动/软 I2C/空口/网关/静态）全部通过；实板 `S/G/BUS` 结构、行长、节拍、失败不伪造均符合现行契约。
  - 本证据**不**断言 GXHT40 器件已损坏或固件不可修；根因未确定，按失败交回实现/硬件，并给出最小区分实验。

> 完整交付不能仅凭宿主测试或下载成功通过。被测产品的主功能（采集被窝温湿度）在真实目标上未产生任何有效样本，因此本能力不能给出 `TEST_PASS`。

---

## 1. 受测身份（版本绑定）

| 项 | 值 |
|---|---|
| 验证时 Git HEAD | `690d2ee496a1981b5a57d2ca47ccc16204b2096c`（工作区验证开始时干净） |
| 固件交付提交 | `c176d02dd0e6220a605c90bf34e84ac7a929565b`（n012） |
| 产品源码一致性 | 6 个 `USER/**` 源文件 SHA-256 与 `evidence/ev027_item008/source_sha256.txt` **逐项一致**；n012 之后仅 Android 设计文档提交，未改固件 |
| 工程 | `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/Project.uvprojx` |
| 编译器 | Keil MDK ARMCLANG `V6.24` |
| 构建 | `rebuild` **0 Error / 1 既有 warning**（`main.c:252` 空循环）；Code=19544 RO=644 RW=116 ZI=1684 |
| `Project.axf` md5 | `9cb97918e8832811dfcb00a93646fba1`（261,832 B） |
| `Project.hex` md5 | `3af11e7adb21ba43004fc827649adb44`（57,185 B） |
| 目标 | CW32L010F8 传感器板（Keil Device 字段）；调试/烧录为工程保存的 CMSIS-DAP + CW32L010 Flash 算法 |
| 下载 | `Erase Done. Programming Done. Verify OK. Application running ...`（2026-10-21 13:22:57 本地） |
| 源码 SHA-256 | `source_sha256.txt` |
| 原始日志 | `build_rebuild.log`、`mdk_flash.log`、`run_context.txt` |

`Verify OK` 仅证明下载校验通过；本次受测件身份由“源码哈希一致 + 构建产物 md5 + 下载日志 + 实板 `S.k` 递增”共同绑定。

---

## 2. 方法与证据

### 2.1 实板（真实条件）

| 项目 | 实际执行 |
|---|---|
| 构建/下载 | `mdk_build rebuild`、`mdk_flash`（原始日志见上） |
| UART1 | `COM43` 9600 8N1，两个 300 s 原始字节窗口（`com43_capture_window_a.bin` 257 B、`window_b.bin` 226 B），解析脚本 `verify_com43_full.py` |
| 逻辑分析仪 | **未取得数据**：`sigrok_capture` 两次失败（`INVALID_CAPTURE`），见 §6；未写 EEPROM、未全局扫描 |
| 遮光/加温/电流表 | 未提供，未执行（L3/L5 物理刺激未覆盖） |

### 2.2 宿主独立验证（真实产品 `.c` + 影子 MCU/位级 mock）

| 证据 | 覆盖 | 原始结果 |
|---|---|---|
| `verify_gate.out`（ITEM-001 harness + 独立 Python 参考） | 14 规范向量、1,936 点网格、4 参兼容、边界 | **14/14、1936 点全一致，RESULT PASS** |
| `snapshot.out` / `verify_snapshot.out`（ITEM-002） | 16 周期 p/H 判定前快照、失败零副作用、无冻结帧不发送 | **224 passed / 0 failed** |
| `freeze.out` / `verify_freeze.out`（ITEM-003） | 触发一次编码、1..3 次同字节、成功/用尽清除、不跨周期 | **332 passed / 0 failed（16 周期、9 触发）** |
| `uart.out` / `verify_uart.out`（ITEM-004） | TXE/TXBUSY 卡死有界、关闭动作、S/G 独立预算、业务不变 | **88 passed / 0 failed** |
| `mainflow.out` / `verify_mainflow.out`（ITEM-005） | 每机会一次事务、BOOT 顺序、UID 零深睡、无空转 | **26 passed / 0 failed** |
| `diagline.out` | 诊断行逐字节与 96 B 预算 | **36 passed / 0 failed（worst S=92 B, G=46 B）** |
| `calib_delivery.out` / `calib_3000.out` / `calib_gate.out`（ITEM-006） | 未标定 `V=0/D=0`、1/3 边界、编译门禁 | **48/0、43/0、11/0** |
| `gxht40.out` | 真实驱动 + 位级 mock：地址探测、0xFD、CRC、重试、总线恢复、无 0x94 | **93 passed / 0 failed** |
| `sfi2c.out` | 软 I2C 原语与恢复线序 | **51 passed / 0 failed** |
| `diagprobe.out` | BUS 扫描/general call/诊断快照传播、失败不伪造 | **106 passed / 0 failed** |
| `rfframe.out` / `gwcompat.out` | 10 B 帧、Feistel、网关/Android 端序与归因、篡改拒绝 | **10/0、11/0** |
| `vdd.out` / `rtc.out` | 供电轨测量公式、3 分钟节拍 | **31/0、17/0** |
| `host_tests_build_test.log` | 实现侧套件回归（参考，不作为独立判据） | **7 组 295/0** |
| `static_audit.out` | 退役通路/NVM/心跳/RF 字段/引脚/中断/行长/源码身份 | **23 passed / 0 failed** |

宿主结论只证明逻辑与流程在影子环境下符合 FDS-003/IC-003，不替代实板结论。

---

## 3. 实板原始观测

### 3.1 原始字节（逐行）

窗口 A（`window_a`，下载复位后立即开始；257 B）：

```text
BUS idle=3 scl=1 sda=1 ack=none
S k=0 p=0 H=0 V=0 o=8 n=41 x=44 a=42 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
S k=3 p=0 H=0 V=0 o=8 n=41 x=44 a=42 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
```

窗口 B（`window_b`，下载后约 13 分钟起；226 B）：

```text
S k=15 p=0 H=0 V=0 o=8 n=42 x=45 a=43 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
S k=18 p=0 H=0 V=0 o=8 n=42 x=45 a=43 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
```

### 3.2 结构判据（`verify_com43_full.out`）

- 79 项结构检查通过：纯 ASCII、每个 LF 前有 CR、无空行、每行 ≤96 B（最大 67 B）、`S` 15 字段顺序固定、`G` 紧随失败 `S`、`G` 字段自洽、`k∈{0,3,15,18}` 且均为 3 的倍数、相邻窗口 `k` 单调递增（无复位）、4 个采样机会各恰一条 `S`、周期之间无额外字节。
- 失败样本 `t=0 h=0 r=0 s=0 y=0`：**未伪造样本、未上报**，符合 B12/FWR-312。
- 光照通路确实执行：`o=8`、`a=42/43`、`n<x`、`a≤x`；未标定交付配置下 `V=0/D=0`，符合 ITEM-006 的未标定语义。
- 上电立即采样（`k=0`、`H=0`）与随后 3  tick（180 s）节拍存在；4 个机会全部为同一失败形态。

### 3.3 未入窗项（证据缺口，不是“未实现”）

`BOOT / VDD / IOTEST / IOSIG` 是上电一次性输出。`mdk_flash` 返回 `Application running` 后串口工具才获得端口，采集起点离首行已晚约数百毫秒，因此只捕获到其后的 `BUS`。该四行的字节格式、顺序与 `BOOT fw=RUN10-FDS003` 由 `diagline.out`（36/0，逐字节）与 `uart.out`（88/0，含 VDD 复算）在宿主独立验证；**实板窗口内未见**如实记为缺口。

---

## 4. 验收映射（与 PACC-002 / TD-003）

| 验收/用例 | 期望 | 实测 | 结论 |
|---|---|---|---|
| **PA-001** | 当前构建实板 ≥3 连续周期 `q=1`，t/h 在有效域；交付门禁 5/5 | 4/4 周期 `q=0`；`G a44=0 a45=0`；`BUS ack=none` | **FAIL** |
| PA-002 | CRC/换算/无效不写出 | 宿主 `gxht40` 93/0、`fw_core` 57/0 | PASS（增量级，宿主） |
| PA-003 | 失败不推进/不上报；恢复后 `q=1` | 失败不推进/不上报已证；**恢复后 `q=1` 未出现** | FAIL（恢复未闭合） |
| PA-004 | PB05 采样期供电、多次转换均值 | `o=8`、均值字段存在；PB05 波形属可选仪器 | 部分通过（实板字段 + 宿主） |
| PA-005/031 | 全暗标定与可分离性 | 未提供遮光窗口；`CALIBRATED=0` | NOT EXECUTED（条件缺失，不冒充通过） |
| PA-007 | ≥5 周期、每 3 tick、无额外采样 | 4 个机会 `k` 增量均为 3；无心跳/额外采样；字节无时间戳故不判绝对容差 | 部分通过（结构级） |
| PA-008 | 14 向量/严格边界 | `verify_gate` 14/14、1936 网格 | PASS（宿主） |
| PA-009 | 真实升温/暗光触发与端到端 | 无有效样本，无法构造升温 | NOT EXECUTED |
| PA-010 | 冻结帧、同字节重试、不跨周期 | `freeze` 332/0；`rfframe` 10/0 | PASS（宿主） |
| PA-017 | BOOT→VDD→IOTEST→IOSIG→BUS、每周期 S、失败 G、≤96 B | 周期 `S/G` 与 `BUS` 符合；上电四行未入窗 | 部分通过（实板）+ 宿主逐字节 |
| PA-018 | 退役通路/引脚/NVM | 静态 23/0 | PASS |
| PA-019 | 非采样期静默/深睡 | 周期之间 0 字节；电流未测 | 部分通过 |
| PA-026 | 合法零值、失败不伪造 | 宿主 + 实板失败路径 `t=h=0` 且 `q=0` | PASS（语义） |
| PA-020/012/013 | 故障有界、帧布局、归因 | 宿主相关套件全通过 | PASS（增量级） |

---

## 5. 失败分析（交回实现/硬件）

### 5.1 复现

1. 用当前工作树 `rebuild`，`mdk_flash` 下载（`Verify OK`）。
2. `COM43` 9600 8N1 采集任意 ≥300 s 窗口。
3. 每个采样机会均得到 `S ... q=0` 紧跟 `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------`；`BUS ... ack=none`。

### 5.2 期望 vs 实际

- 期望（FDS-003/FWR-302、PACC-002 PA-001）：地址 0x44 或 0x45 应答，`q=1`，`t∈−400..1250`、`h∈0..1000`，连续 ≥3 个机会。
- 实际：两个地址均不应答；驱动在 3 次帧尝试后按“无器件”结束；不伪造、不推进、不上报（这部分符合失败语义）。

### 5.3 竞争假设（均未被本证据排除）

| # | 假设 | 支持/反对 |
|---|---|---|
| H1 | GXHT40 未贴装/虚焊/器件损坏 | 支持：4/4 无 ACK；`a44=a45=0`。历史多轮同形态 |
| H2 | GXHT40 供电/去耦异常 | 支持：无 ACK 可由未上电导致；本次未测器件 VDD（无电流表/示波器） |
| H3 | I2C 从器件侧电气/上拉问题 | 部分反对：主机可驱动并读回空闲高（`BUS scl=1 sda=1`），112 地址扫描无异常 ACK；但上升沿/容性负载未测 |
| H4 | 固件探测时序/电平与真实器件不匹配 | 支持有限：位级 mock 93/0；但 mock 不能证明真实边沿、上拉强度与器件时序 |

固件在本轮变更的 6 个文件**不含** `gxht40.c` / `sf_i2c.c`，且该失败形态早于 n012 已存在；因此不是本轮改动引入的回归，但它仍然阻断完整交付的主功能验收。未知根因**不**等于硬件已损坏或软件不可修。

### 5.4 最小区分实验（建议顺序）

1. **测器件供电**：在采样瞬间测量 GXHT40 VDD（U9 供电脚）与去耦，区分 H1/H2。
2. **换件/换板交叉**：同一镜像 + 已知良好 GXHT40（或已知良好传感器板）复测；若 `q=1`，指向本板器件/装配（H1/H2）；若仍 `q=0`，转向 H4（固件/总线时序）。
3. **总线旁证/边沿**：修好逻辑仪采集输出路径后抓取 `IOTEST`/周期探测的 SCL/SDA 原始波形；或临时降低软 I2C 速率做对照（仅测试夹具，不改交付判据）。
4. 若 H1/H2 成立，修复后必须重新执行 PA-001 的 3/5 连续周期与后续 PA-009/端到端；不得以“已诊断”替代“已恢复”。

---

## 6. 测试资产修复（本能力自有，未改产品源码）

执行中发现的 4 处**测试期望过期**已按现行契约修正，并用负向/正向断言复核；没有降低产品判据：

| 文件 | 问题 | 修正 |
|---|---|---|
| `evidence/ev024_item004_verify/verify_uart_guard_item004.py` | 硬编码 `BOOT fw=FD-002r4`（ITEM-006 前的旧标识） | 从 `sensor_config.h` 读取 `SENSOR_FW_BUILD_ID`（当前 `RUN10-FDS003`）；缺失即报错，无静默回退 |
| `test/host_measure_snapshot_verify_item002.c` + `evidence/ev022_item002/verify_measure_snapshot_item002.py` | 仍用“手动置 `report_req` 后重发旧帧”的 ITEM-002 旧模型；ITEM-003 已改为触发时冻结 | 改为断言“无冻结帧时不得发送”（T-L6-04）；p/H 快照检查全部保留；冻结逐字节由 ITEM-003 harness 覆盖 |
| `test/host_diag_probe_verify_ev.c` | F2 断言 `S.p` 为推进后的值（ITEM-002 前语义） | 改为判定前快照：首个成功周期 `H=0/p=0`；第二成功周期 `p=前值` |
| `evidence/ev028_item008_verify/static_audit_item008.py` | 硬编码 Git HEAD `c176d02`（后续非固件提交使 HEAD 前移） | 以 n012 交付的 6 个源码 SHA-256 判定受测身份，并记录当前 HEAD |

上述修改仅位于测试资产；产品源码、Keil 工程、网关、Android、硬件与 `runtime/*.yaml` 未改。

---

## 7. 环境限制（不影响本轮主判定）

1. **逻辑分析仪不可用（工具缺陷）**：`sigrok_capture` 两次返回 `INVALID_CAPTURE`；`sigrok-cli` 报
   `Error saving zipfile: Failure to create temporary file: No error`。会话输出路径长 258 字符，
   srzip 临时文件名超过 Windows `MAX_PATH` 是最可能原因（未修改配置/驱动）。原始 stderr 与 metadata 已保存
   （`sigrok_capture_attempt1_metadata.json`、`attempt2_metadata.json`、`sigrok_capture_stderr_head.txt`）。
   该仪器在 PACC-002 中为可选旁证，不作为通过前提。
2. **BOOT 窗口竞争**：`serial_capture` 在 `mdk_flash` 返回后才启动，上电四行已发完；只捕获 `BUS`。
3. **无遮光/安全加温/电流表**：PA-005/031、PA-009、PA-019 的物理条件未具备，只能记 `NOT EXECUTED`/部分。
4. 未执行端到端（网关/手机/服务器）；不属于本能力本次范围。

---

## 8. 结论

- 本能力对 n012 固件实现的**完整交付**判定为 **`TEST_FAIL`**：
  - 唯一的阻断项是 **PA-001 真实温湿度采集**（以及依赖它的 PA-003 恢复、PA-009、端到端）；
  - 固件的全部可宿主验证行为（门控、快照、冻结、UART 有界、主流程、标定门禁、驱动、总线恢复、帧兼容、静态边界）均通过独立验证；
  - 实板诊断与失败语义正确，未伪造数据。
- 不宣称硬件损坏、不宣称固件不可修；请实现/硬件按 §5.3/§5.4 继续定位，修复后在当前受测件上重跑 PA-001 与完整回归。
- 本证据不构成产品验收；U01（GXHT40 真实 ACK）、U02（全暗标定）、端到端与真实总功能仍未闭合。
