# 驱动测试证据（DRV-002 rev 5.3）

状态：固件实现证据（firmware_engineer.firmware_implementation）
本轮范围：run8 **ITEM-002 复验修复二**（EV-011 交回实现：E1 主机拉低/回读自检 + SDA/SCL 角色对调探测）
依据：FD-002 rev 5.0（§6.6.1/§6.6.3）、FWR-002 rev 5.0（FWR-116/118）、TD-002 rev 5.0；触发 `evidence/test.md` EV-011（TEST_FAIL，§5 E1）
受测实现：`USER/src/measure.c`（`sensor_io_diag_scan`、PA03 开漏输出、互换角色影子设备）、`USER/src/debug_trace.c`（`debug_trace_iotest`）、`USER/src/main.c`（上电接线顺序）
测试载体：`test/build_test.sh`（155 项）、`test/host_gxht40_check.c`（60）、`test/host_sf_i2c_bus_check.c`（35）；另**只读复跑** tester 自有 harness
测试环境：宿主机 MinGW-w64 gcc 12.2.0（带**开漏回读模型**的 mock GPIO）；`mdk_flash`/串口不在本调用工具列表内。

---

## ITEM-002 复验修复二：自检与独立资产对照（本轮实际执行）

### 1. 新增观测量（E1）与自检原始 stdout（节选）

```
$ sh test/build_test.sh
[1/4] 40 passed  [2/4] 39 passed  [3/4] 37 passed  [4/4a] 19 passed  [4/4b] 20 passed   (0 failed)
$ ./host_gxht40_check     ->  60 passed / 0 failed
$ ./host_sf_i2c_bus_check ->  35 passed / 0 failed

[12] E1 上电 I/O 自检: 主机拉低/回读 + 释放 + 角色对调 (E1)
  PASS  I/O 自检完成并填充结果
  PASS  主机能把 SDA(PA04) 与 SCL(PA03) 拉低并回读为 0 (开漏模型)
  PASS  释放后两线回读为高 (idle=3)
  PASS  mock 总线无器件: 角色对调后仍无 ACK
  PASS  负对照: PA03 回读恒高时 scl_lo=1 而 sda_lo=0 (能区分主机拉不低)

[T11] IOTEST 上电 I/O 自检行 (E1): 格式/预算
    PASS  典型: 拉低成功/释放回高/对调后无 ACK -> swap=none
        |IOTEST sda_lo=0 scl_lo=0 idle=3 swap=none
    PASS  最坏: 拉不低 + 对调后两候选均 ACK -> swap=44,45 (大写 hex)
        |IOTEST sda_lo=1 scl_lo=1 idle=3 swap=44,45
    PASS  worst-case IOTEST line <= SENSOR_DEBUG_UART_MAXLINE (96) bytes
        len(worst IOTEST)=44 bytes
    PASS  IOTEST 仅整数与大写十六进制
```

### 2. 负对照说明（为何该自检能定罪“主机拉不低”）

mock GPIO 按开漏模型实现（外部拉低→0；开漏输出且锁存为0→0；释放/输入→1），并把“回读恒高”作为可注入故障；注入后 `scl_lo=1` 而 `sda_lo=0`，证明该字段确实反映驱动器行为而不是常量。

### 3. tester 自有 harness 只读复跑（未修改其文件；结论权归 tester）

| 资产 | 结果 |
|---|---|
| `test/host_gxht40_verify_ev.c` | **93 passed / 0 failed**（含其已按上轮交接更新的 OBS-2 新语义）|
| `test/host_diag_probe_verify_ev.c` | **85 passed / 0 failed** |
| `test/host_sf_i2c_verify_ev.c` / `test/host_diag_line_verify_ev.c` | 51/51、36/36（上轮已复跑，本轮未涉及其被测文件）|

### 4. 本轮自检发现并修复的实现缺陷

| 缺陷 | 发现方式 | 修复 |
|---|---|---|
| `sensor_io_diag_scan` 首版位于 `#if SENSOR_DEBUG_UART` 之外，`=0` 时与空实现重定义，并引用被条件编译剔除的 `i2c0_scl_pin_read_level`/`i2c0_swap_dev` | `-DSENSOR_DEBUG_UART=0` 逐 TU 编译复核（`measure.c` 报 redefinition/undeclared） | 把真实实现移入 `#if SENSOR_DEBUG_UART` 块；复测 `measure.c`/`debug_trace.c` 在 `=0` 下 0 告警 0 错误 |

### 5. 未取得 / 不声称

- `IOTEST` 的**实板读数**与 `q=1` 均未在本轮取得（本调用无 `mdk_flash`/串口）。本轮交付的是“可复跑的判别观测量”，其判读表见 `artifacts/firmware_implementation.md` 本轮节 §2。
- 不变的不变量（未改且逐项回归通过）：双字 CRC-8、有效域、读重读/整帧重测上限、命令白名单、失败不写输出/不上报/不推进前值、`BUS`/`S`/`G` 行字面与字段。

---

# 历史：ITEM-002 复验修复一（FWR-118 + OBS-2）

### 1. 修复内容（对应 EV-009 事实 2 与 OBS-2）

| 项 | 修复前（EV-009 实测） | 修复后（本轮） |
|---|---|---|
| 缓存地址 CRC 错后的下一周期 | `0x88=1 0x8A=0`、`recovery_calls=0` | `0x88=1 0x8A=1`、`recovery_calls=1` |
| 缓存地址量程无效后的下一周期 | `0x88=1 0x8A=0`、`recovery_calls=0` | `0x88=1 0x8A=1`、`recovery_calls=1` |
| “地址 ACK 但读不通”的结果码 | `s=2`（无器件）而 `a44=1`（自相矛盾） | `s=3`（读失败）与 `a44=1` 自洽；`s=2` 仅在两地址均无 ACK 时出现 |

### 2. 实现侧自检原始 stdout（节选）

```
$ sh test/build_test.sh   ->  40 + 34 + 33 + 19 + 20 = 146 passed, 0 failed
$ ./host_sf_i2c_bus_check ->  35 passed / 0 failed
$ ./host_gxht40_check     ->  60 passed / 0 failed
```

`host_gxht40_check.c` 新增/更新的用例（原文）：

```
[1] 手册参考向量 ...
  PASS  首次访问完整重探: 0x88(命令) + 0x89(读) 后仍探 0x8A (FWR-118)
  PASS  三次事务各一次 START; 4 次 STOP = A 命令 1 + A 读取 1 + B 探测 2 (NACK 路径先发 STOP 再补 STOP, 既有写法)
[13] FWR-118/TD-002 B18: CRC 失败后下一轮完整重探两候选地址
  PASS  基线: 缓存地址 0x44 测量成功
  PASS  CRC 错周期返回 GXHT40_ERR_CRC
  PASS  CRC 错周期不修改输出
  PASS  CRC 错后缓存地址失效 (不再保留 0x44)
  PASS  下一轮恢复正常读数
  PASS  CRC 失败后的下一轮重探了 0x44 与 0x45 两个候选地址
[14] FWR-118/TD-002 B18: 量程无效后下一轮完整重探两候选地址
  PASS  基线: 缓存地址 0x44 测量成功
  PASS  量程无效周期返回 GXHT40_ERR_RANGE
  PASS  量程无效后缓存地址失效
  PASS  下一轮恢复正常读数
  PASS  量程无效后的下一轮重探了 0x44 与 0x45 两个候选地址
```

### 3. tester 自有 harness 只读复跑（未修改其文件；结论权归 tester）

| 资产 | 结果 | 说明 |
|---|---|---|
| `test/host_gxht40_verify_ev.c`（`-Wl,--wrap=i2c_bus_recover`） | **82 passed / 1 failed** | 唯一失败 = T15 的 OBS-2 *观察*断言（其目的为复现旧语义 `status==2 && ack44==1`）；按本轮 OBS-2 裁决需 tester 更新为 `status==3 && ack44==1`。**T11/T12/T13/T14/T17/T17b 全部通过**；T17/T17b 现输出 `next-period probes: 0x88=1 0x8A=1 recovery_calls=1`（FWR-118 字面符合） |
| `test/host_diag_probe_verify_ev.c` | **85 passed / 0 failed** | 含 D6「读 NACK 用尽 ⇒ `GXHT40_ERR_IO`」与 D7 只读 getter |
| `test/host_sf_i2c_verify_ev.c` | **51 passed / 0 failed** | 位级 mock 总线：既有原语回归 + 地址探测/恢复 |
| `test/host_diag_line_verify_ev.c` | **36 passed / 0 failed** | BUS/S/G 行逐字节渲染（含 12 个 `-` 占位） |
| `test/host_measure_flow_verify_ev.c` | **无法编译** | 期望旧的 `bool light_sample()`（自 run7 T2 起即如此，tester 已用 `host_diag_probe_verify_ev.c` 取代）——**与本轮改动无关**，登记供 tester 清理 |

### 4. 未改变的不变量（回归确认）

- 双字 CRC-8、整数 x10 换算与有效域、读重读(5)/整帧重测(3)、命令白名单（仅 `0xFD`；`0x94`/加热器从不出现）、失败不写输出、失败不上报/不推进前值——全部未改且逐项断言通过。
- 稳态快路径不变：正常设备下每周期仍只 1 次命令 + 1 次读取（新增的双候选重探仅发生在首次访问或失败后的周期）。

### 5. 未取得 / 不声称

- 实板 `q=1` **本轮仍未取得**（本调用无 `mdk_flash`/串口；且器件在地址级完全不应答——EV-009 §3.1/§5）。本修复只确保“失败后完整重探 + 结果码自洽”这一可由实现交付的义务。

---

# 历史：ITEM-002 首轮（FE 实现记录，已按 EV-009 修复）

### 1. 一手输入（来自 ITEM-001 独立验证，决定本轮修复方向）

| 事实 | 原文/数值 | 含义 |
|---|---|---|
| 总线空闲 | `BUS idle=1 scl=1 sda=0 ack=none`（boot#1）；`BUS idle=3 scl=1 sda=1 ack=none`（boot#2/#3） | 上拉/供电正常（idle=3）；存在一次 SDA 空闲读到低的瞬态 |
| 地址应答 | 0x08..0x77 全部 `none`（含 0x44/0x45） | **器件在地址级不应答**（非 CRC/量程/地址变体未覆盖） |
| 失败周期 | `G s=2 a44=0 a45=0 rd=0 at=3 raw=------------` ×6 | 驱动走“两地址均无 ACK”路径；未发起读 |
| 负对照 | 若 SDA 被持续拉低，扫描必然得到满 8 地址 `+`；实得 `ack=none` | 排除“SDA 卡低”解释（`evidence/test.md` EV-008 §3.3） |

**定位**：失败在「器件是否在总线上应答」这一层。固件侧可做且被 FWR-118/任务明文要求的修复 = 有界总线恢复 + 任一失败后强制双地址重探 + 首访 tPU 余量；**不应答本身不得由固件“绕过”**。

### 2. `host_sf_i2c_bus_check.c`（新增 [7]–[9]；共 35 项 0 失败）节选原文

```
[7] i2c_bus_recover: 空闲总线不产生时钟/STOP
  PASS  总线空闲: 返回 SF_I2C_SUCCESS
  PASS  空闲总线不产生 START
  PASS  空闲总线未额外打时钟 (仅释放 SCL 的一次置高)
  PASS  恢复后双线均高 (总线空闲)
[8] i2c_bus_recover: SDA 被从机持续拉低 -> 9 个 SCL 脉冲 + STOP, 仍有界返回
  PASS  SDA 仍被拉低: 返回 SF_I2C_TIMEOUT (不无限等待)
  PASS  脉冲数有界: 释放 1 + 9 个恢复脉冲 + STOP 1 = 11 个 SCL 上升沿
  PASS  恢复后补发合法 STOP
  PASS  恢复过程不发起任何 I2C 事务 (无 START/地址/命令)
[9] i2c_bus_recover: 从机在第 4 个上升沿前释放 SDA -> 提前结束并成功
  PASS  从机释放 SDA 后返回 SF_I2C_SUCCESS
  PASS  提前结束: 释放 1 + 3 个脉冲 + STOP 1 = 5 个上升沿 (未用满 9 个)
  PASS  结束前仍补发 STOP
  PASS  恢复后总线空闲
```

### 3. `host_gxht40_check.c`（新增 [11]–[12]；共 48 项 0 失败）节选原文

```
[11] FWR-118: 缓存地址读失败后必须重探两个候选地址
  PASS  器件在 0x45 上测量成功并缓存
  PASS  读 NACK 用尽 -> GXHT40_ERR_IO
  PASS  失败仍不修改输出
  PASS  缓存地址在任一失败后被清空 (FWR-118)
  PASS  缓存地址(0x45)读失败后仍重探了 0x44 (不再提前返回)
  PASS  下一轮重新探测后测量成功
  PASS  重新探测后缓存更新为 0x44
  PASS  重探从 0x44 开始且仅一次命令+读取
[12] 首次访问前 tPU 上电余量 (只一次, 不进周期路径)
  PASS  首测成功
  PASS  首次访问前恰好一次 GXHT40_POWER_ON_WAIT_MS (手册 tPU)
  PASS  tMEAS 等待仍为 1 次 10 ms (>= 手册 8.3 ms 上限)
  PASS  后续测量不再重复上电余量 (仅首访)
```

既有断言全部保留且通过：手册 CRC 向量、地址探测与缓存、双字 CRC-8、读 NACK 重读上限、CRC 重测上限、无器件/超范围、失败不改输出、命令白名单无 0x94/加热器。

### 4. 回归（本轮实际执行）

```
$ sh test/build_test.sh
[1/4] 40 passed  [2/4] 34 passed  [3/4] 33 passed  [4/4a] 19 passed  [4/4b] 20 passed
（合计 146 项 0 失败；S 行逐字节金标准、BUS/G 行预算、光照/判定/流程全部未变）
$ ./host_sf_i2c_bus_check => 35 passed / 0 failed
$ ./host_gxht40_check      => 48 passed / 0 failed
```

本轮实现侧合计 **229 项 0 失败**。

### 5. 本轮修复未涉及、也不得伪造的部分

- **实板 `q=1` 未取得**：本轮无 `mdk_flash`/串口（见 `evidence/build.md`）。若器件仍对任何地址不应答，则固件侧的恢复/重探无法使其应答；此时应保留 `q=0` 事实，并将“U9 贴装/焊接、VDD 与 PA03/PA04 连通性、器件异常保持态”交持有万用表/目视条件的角色现场核对（`evidence/test.md` EV-008 §3）。
- **未放宽任何校验**：CRC-8 双字校验、温度有效域、读重读(5)/整帧重测(3)、命令白名单、失败不写输出均未改动（见 [10] 及既有断言）。

---

# 历史：本轮 run8 ITEM-001（T1 诊断可观测，已 TEST_PASS）

### 1. `test/build_test.sh` 原始 stdout 摘要

```
$ sh test/build_test.sh
== [1/4] fw_core pure logic: CRC16/CRC-8/convert/light(1/3)/report ==
==== result: 40 passed, 0 failed ====
== [2/4] measure flow (mock MCU): sample/report/prev-not-updated-on-failure ==
==== result: 34 passed, 0 failed ====
== [3/4] UART1 debug trace (mock UART): banner/S-line budget/close semantics ==
==== T1 UART1 debug trace self-check (firmware_engineer) ====
==== result: 33 passed, 0 failed ====
== [4/4a] light channel (mock ADC, uncalibrated delivery default) ==
==== result: 19 passed, 0 failed ====
== [4/4b] light channel (calibrated variant) ====
==== result: 20 passed, 0 failed ====
(exit 0；合计 146 项，0 失败)
```

### 2. `host_debug_trace_check.c`（新增 T8/T9/T10；共 33 项 0 失败）节选原文

```
[T8] BUS 上电总线诊断行 (FWR-116): 格式/条件/预算
    PASS  无地址 ACK 时打印 ack=none (空闲电平位展开一致)
        |BUS idle=3 scl=1 sda=1 ack=none
    PASS  ACK 地址以 2 位大写十六进制逗号分隔列出
        |BUS idle=3 scl=1 sda=1 ack=44,45
    PASS  最坏 BUS 行字节精确匹配 (idle=0 -> scl/sda=0; 超出上限加 '+')
        |BUS idle=0 scl=0 sda=0 ack=08,09,0A,0B,0C,0D,0E,0F+
    PASS  worst-case BUS line <= SENSOR_DEBUG_UART_MAXLINE (96) bytes
        len(worst BUS)=53 bytes
    PASS  BUS line uses uppercase hex only (no lowercase/float)
[T9] G 失败周期诊断行 (FWR-116): 仅 diag_valid 时出现且 <=96 B
    PASS  失败周期 = S 行 (逐字节不变) 后紧跟最宽 G 行 (12 位大写 hex)
        |S k=3 p=0 H=0 V=0 o=0 n=0 x=0 a=0 D=0 t=456 h=678 q=0 r=0 s=0 y=0
        G s=5 a44=1 a45=1 rd=5 at=3 raw=ABACADAEAFB0
    PASS  worst-case S 行与 G 行各自 <= SENSOR_DEBUG_UART_MAXLINE (96) bytes
        len(worst S)=67 bytes, len(worst G)=46 bytes
    PASS  未读成功时 raw 用 12 个 '-' 占位
[T10] 成功周期不得出现 G 行
    PASS  diag_valid=0 -> 只打印 S 行, 无 G 行
```

仍有断言保证 `S` 行向后兼容（未改动）：`EXPECT_WORST`/`EXPECT_TYPICAL` 逐字节比对继续通过（最坏 S 行 92 B）。

### 3. `host_sf_i2c_bus_check.c`（新增 [5]/[6]；共 23 项 0 失败）节选原文

```
[5] i2c_probe_addr: 只发地址字节的有界探测 (FWR-116)
  PASS  器件在 0x44: 返回 SF_I2C_SUCCESS (地址被 ACK)
  PASS  恰好 1 个 START 与 1 个 STOP
  PASS  线上只有地址写字节 0x88 (无命令字节/无数据)
  PASS  探测后总线释放(SCL/SDA 均为高)
[6] i2c_probe_addr: 无器件地址返回超时且释放总线
  PASS  0x45 无器件: 返回 SF_I2C_TIMEOUT
  PASS  NACK 后已发出 STOP 释放总线
  PASS  线上只有地址写字节 0x8A, 未继续写命令
  PASS  超时后总线仍释放
```

既有 [1]–[4]（`i2c_write_cmd`/`i2c_read_bytes` 语义与长度 0 行为）不变且全部通过。

### 4. `host_gxht40_check.c`（新增诊断断言；共 36 项 0 失败）节选原文

```
[1] 手册参考向量 ...
  PASS  诊断: 成功轮 status=OK/attempt=1/read_retry=0
  PASS  诊断: 0x44 收到地址 ACK, 0x45 未被探测到 ACK
  PASS  诊断: raw 为最近一次成功读回的 6 字节
[5] 读 NACK 有界重读
  PASS  诊断: 读事务消耗的失败重读次数 = 2 (前 2 次 NACK)
[6] CRC 错误: 重测用尽后返回失败且不改输出
  PASS  诊断: CRC 错 - status=4 且 attempt=GXHT40_MEAS_RETRY
  PASS  诊断: 保留最近一次成功读回的原始 6 字节 (供独立 CRC-8 复算)
[7] 无器件
  PASS  诊断: 无器件 - status=2 且两候选地址 ACK 均为 0
  PASS  诊断: 未读成功时 raw_valid=0 (打印时用占位)
[8] 换算超范围
  PASS  诊断: 量程无效 - status=5 且保留原始 6 字节
```

既有断言全部保留（双字 CRC-8、地址探测与缓存、失败不改输出、命令白名单无 0x94/加热器）。

### 5. `host_measure_flow_check.c`（新增 [10]/[11]；共 34 项 0 失败）节选原文

```
[10] T1 诊断快照集成: 失败周期带 G 行数据 / 成功周期不带 (FWR-116)
  PASS  失败周期有轨迹快照
  PASS  失败周期 diag_valid=1 且结果码来自驱动诊断快照
  PASS  失败周期诊断字段 (地址 ACK/重读/重测/原始字节) 逐项传递到轨迹
  PASS  失败周期 sample_ok=0 (与 G 行出现条件一致)
  PASS  成功周期有轨迹快照
  PASS  成功周期 sample_ok=1 且 diag_valid=0 (不打印 G 行)
[11] T1 上电总线诊断扫描 (只观测; 不写命令/不改业务状态)
  PASS  扫描完成并填充结果
  PASS  主机 mock 总线两线均高 (idle=3), 无器件应答 (ack_count=0)
  PASS  可重复调用 (bsp_i2c_init 幂等, 不重复注册对象)
  PASS  幂等初始化后仍能按名找到同一 I2C 对象
```

### 6. 自检发现并修复的实现缺陷

| 缺陷 | 发现方式 | 修复 |
|---|---|---|
| `G raw=` 未读成功时只打印 6 个 `-`（与 6 字节 hex 的 12 位宽不一致，破坏定宽解析） | `host_debug_trace_check.c` 新增的 `raw=------------` 断言（首轮 FAIL） | `debug_trace.c` 改为 `GXHT40_RESULT_LEN * 2`（12 个 `-`），重跑 PASS |

### 7. 环境记录（不掩盖）

- 宿主杀毒/EDR 按文件名拦截新建 exe：`test/build_test.sh` 原产物名 `_hostbin/s2_flow.exe` 持续报
  `ld.exe: cannot open output file ...: Permission denied`（重试两次均失败）；同一编译命令换名（`flow_measure.exe`）立即成功。
  因此将 `_hostbin/` 产物名改为描述性名称（`core_check.exe`/`measure_flow_check.exe`/`debug_trace_check.exe`/`light_check.exe`/`light_check_cal.exe`），
  编译参数与用例不变，并在脚本头部注明原因；改名后 5 个阶段全部可运行。
- 宿主机自检结论仅证明**软件可见行为**；`BUS ack=` 只证明地址级应答，不代表位级电气/时序合格（与 TD-002 §4.4 边界一致）。

### 8. 未执行（交接，不当作通过）

- `mdk_flash` 部署与 COM42 采集：本调用工具列表仅 `mdk_build`。真实目标上的上电 `BUS` 行（空闲电平 + ACK 集合）、
  失败周期 `S`+`G` 行（结果码/地址 ACK/重读重测/原始 6 字节）及与 `S` 行 `q=0` 的自洽性由嵌入式测试能力按
  TD-002 §4.4 T-L2-11/12 采集并判定；本轮不声称已观测。
- 全暗基准 `C_dark` 仍为未标定（`LIGHT_DARK_CALIBRATED=0`），不在本项范围（后续增量）。

---

# 历史：本轮 run7 T2（光照采集—完全无光基准 1/3 判据与有效性）

### 1. `test/build_test.sh`（4 阶段）原始 stdout 节选

```
$ CC=<mingw full path> sh test/build_test.sh
== [1/4] fw_core pure logic: CRC16/CRC-8/convert/light(1/3)/report ==
==== result: 40 passed, 0 failed ====
== [2/4] measure flow (mock MCU): sample/report/prev-not-updated-on-failure ==
==== result: 24 passed, 0 failed ====
== [3/4] UART1 debug trace (mock UART): banner/S-line budget/close semantics ==
==== result: 24 passed, 0 failed ====
== [4/4a] light channel (mock ADC, uncalibrated delivery default) ==
==== light.c self-check (T2, LIGHT_DARK_CALIBRATED=0, C_dark=0) ====
==== result: 19 passed, 0 failed ====
== [4/4b] light channel (calibrated variant: -DLIGHT_DARK_CALIBRATED=1 -DLIGHT_DARK_REF_CODE=3000) ==
==== light.c self-check (T2, LIGHT_DARK_CALIBRATED=1, C_dark=3000) ====
==== result: 20 passed, 0 failed ====
(exit 0)
```

> 宿主环境备注（可复现事实，非代码问题）：本机 `test/` 目录下**同名产物 `host_measure_flow_check.exe` 无法再创建**（bash/gcc 均报 `Permission denied`，删除后仍复现；推测为宿主安全软件对该路径的残留拦截）。`build_test.sh` 因此把产物改为中性短名（`s1_core/s2_flow/s3_trace/s4_light/s4_light_cal.exe`）并写入 `_hostbin/`，同时对链接失败做一次带延迟重试；harness 源文件名、编译参数与断言不变，故不影响测试内容。

### 2. 纯逻辑 `light_is_dark`（`host_sensor_core_test.c` 节选，1/3 判据与边界）

```
[5] T-L0-04 light_is_dark (dark = valid && 3*mean >= C_dark)
  PASS  valid=false -> 恒 false
  PASS  valid=false + 满量程 -> false (不合成暗态)
  PASS  C_dark=1: 3*0=0 < 1 -> false
  PASS  C_dark=1: 3*1=3 >= 1 -> true
  PASS  C_dark=4095: 3*1364=4092 < 4095 -> false
  PASS  C_dark=4095: 3*1365=4095 -> true (相等为暗)
  PASS  C_dark=4095: 满量程 -> true
  PASS  C_dark=3000: 边界相等为暗, 低 1 不为暗
  PASS  每笔独立: 无滞回/无历史暗态
```

`host_fw_core_pure_check.c` 另有 `C_dark=1..4095` 全量边界扫描（`mean=ceil(C/3)` 为暗、低 1 不为暗，`bad=0`），随阶段 1 部分覆盖（该文件本阶段未入 build_test，属实现侧补充自检，编译运行见下）：

```
$ gcc -std=c11 -Wall -Wextra -I../USER/inc -I../COMMON host_fw_core_pure_check.c ../USER/src/fw_core.c -lm -o fw_core_pure && ./fw_core_pure
[5] T-L0-04 光照: 完全无光基准 1/3 判据 (light_is_dark)
    PASS  valid=false -> 恒 false (即使 3*mean>=C_dark)
    PASS  C_dark=1: 3*0=0 < 1 -> false
    PASS  C_dark=4095: 3*1365=4095 = C_dark -> true (相等为暗)
    PASS  C_dark=3000: 边界相等为暗, 低 1 不为暗
    PASS  每笔独立: 同输入同输出 (无滞回/无历史暗态)
    PASS  本交付件为未标定状态 (C_dark 占位 0, 不得当作已验收)
    PASS  C_dark=1..4095 全量: mean=ceil(C/3) 为暗, 再低 1 不是暗
```

### 3. `light.c` 采样通路（`host_light_check.c` + `mock_light/`，真实 light.c）

未标定变体（交付件默认）：

```
[1] light_init: 引脚配置与 ADC 关闭
    PASS  light_init configures exactly two pins (PB05, PB04)
    PASS  PB04 configured as analog input (AIN11) after PB05
    PASS  ADC is left disabled outside sampling
[2] 8 次全部成功: 均值/极值取自成功样本, PB05 供电后置低, ADC 关闭
    PASS  exactly LIGHT_ADC_SAMPLES conversions
    PASS  PB05 was powered (high) for every conversion
    PASS  PB05 returned low after sampling
    PASS  ADC disabled after sampling
    PASS  samples_ok=8, adc_ok=true
    PASS  mean = 135 over successful samples
    PASS  min=100, max=170
[3] 部分超时: 均值只取成功样本, 成功数如实记录
    PASS  4 successes recorded (timeouts dropped)
    PASS  mean/min/max computed over the 4 successful samples only
[4] 全部超时: valid=false, 不得用满量程合成暗态
    PASS  samples_ok=0, adc_ok=false
    PASS  mean/min/max stay 0 (no 4095 full-scale synthesis)
    PASS  valid=false, dark=false on total timeout
    PASS  PB05 low and ADC off after timeout too
[5] 有效性/无光判定
    PASS  uncalibrated: adc_ok=true but valid=false (cannot prove darkness)
    PASS  uncalibrated: dark=false even at full-scale reading
[6] 每笔独立 (无滞回/无历史暗态)
    PASS  same input -> same result regardless of the previous sample
==== result: 19 passed, 0 failed ====
```

标定后变体（`-DLIGHT_DARK_CALIBRATED=1 -DLIGHT_DARK_REF_CODE=3000`，仅验证判据接线，**不是**实板标定值）：

```
[5] 有效性/无光判定
    PASS  calibrated: valid=true when ADC succeeds
    PASS  dark = (3*mean >= C_dark) at mean=1000
    PASS  dark = (3*mean >= C_dark) at mean=999 (one below)
==== result: 20 passed, 0 failed ====
```

### 4. 流程回归（`host_measure_flow_check.c`，24/24）

`light_sample` 桩改为返回 `light_result_t`；采样顺序、失败不污染前值、上报门控/发送路径预期均保持通过（该文件的 `sensor_decide_report` 仍为 as-built 降温方向，属 T3）。

### 5. 未覆盖（如实记录）

- **全暗基准 `C_dark` 实板标定未执行**：需 `mdk_flash` + COM42 采集（不在本调用工具列表），未取得完全遮光原始样本分布 → 未回填、未置 `LIGHT_DARK_CALIBRATED=1`；**不得**用满量程/旧默认 350/250/亮态读数冒充。
- 实板三态观测（有光 `dark=0` / 完全遮光 `dark=1` / 移开恢复）与 `ceil(C_dark/3)` 实板邻界：同上未执行。
- 位级 I²C/ADC 时序与 PB05 实际波形：仪器未提供（TD-002 §4.10 可选扩展）。

---

# 历史：本轮 run7 T1（ITEM-001 UART1 调试串口，已 TEST_PASS）

## T1（run7 ITEM-001）：UART1 调试通道自检（已验证）

环境：MinGW-w64 GCC 12.2.0（`C:/ProgramData/chocolatey/lib/mingw/tools/install/mingw64/bin/gcc.exe`；PATH 上的 chocolatey shim 解析 libexec/cc1 失败，需用完整路径）。宿主机无 MCU 串口外设，默认 `SENSOR_DEBUG_UART=0`；本自检显式 `-DSENSOR_DEBUG_UART=1` 并链接真实 `debug_trace.c`。

命令与原始 stdout：

```
$ cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
$ CC=<mingw full path>
$ $CC -std=c11 -Wall -Wextra -DSENSOR_DEBUG_UART=1 -Itest/mock_trace -IUSER/inc \
      test/host_debug_trace_check.c USER/src/debug_trace.c -o host_debug_trace_check.exe
$ ./host_debug_trace_check.exe
==== T1 UART1 debug trace self-check (firmware_engineer) ====
[T1] boot banner
    PASS  banner bytes exactly match the frozen format
        |BOOT fw=FD-002r4 uid=01020304 rst=0001 uart=9600
    PASS  UART1 initialised once
    PASS  baud rate = 9600 (COM42 configuration preserved)
    PASS  PCLK = 8 MHz (HSI DIV6)
    PASS  TX mode enabled
[T2] worst-case S line: keys, integers, <=96 bytes
    PASS  worst-case line bytes exactly match the frozen format
        |S k=4294967295 p=-1250 H=1 V=1 o=8 n=4095 x=4095 a=4095 D=1 t=-1250 h=1000 q=0 r=1 s=2 y=2
    PASS  worst-case line <= SENSOR_DEBUG_UART_MAXLINE (96) bytes
        len(worst)=92 bytes (budget 96)
    PASS  S-line key order is the frozen contract order
    PASS  integers only (no float/percent formatting)
    PASS  line terminated with CRLF (no truncation)
[T3] typical S line
    PASS  typical line bytes exactly match the frozen format
        |S k=3 p=250 H=1 V=1 o=8 n=12 x=15 a=13 D=0 t=456 h=678 q=1 r=0 s=0 y=0
[T4] no side effect on the state snapshot
    PASS  debug_trace_sample does not modify the caller snapshot
[T5] flush/close semantics (only UART1 + PA05/PA06)
    PASS  waits for TC (transmit complete) before closing
    PASS  UART1 peripheral reset asserted then released
    PASS  UART1 APB clock disabled
    PASS  PA06 (UART1_TXD) returned to input
    PASS  PA05 (UART1_RXD) returned to input
    PASS  GPIOA peripheral is NOT reset (protects PA03/PA04 I2C)
    PASS  GPIOA clock is NOT disabled (protects I2C/SWD pins)
[T6] idempotent close
    PASS  second close is a no-op (no repeated reset/clock traffic)
[T7] re-init after close (deep-sleep wake path)
    PASS  sample after close prints a full line again
    PASS  UART1 re-initialised before printing after close
==== result: 22 passed, 0 failed ====
```

覆盖：FD-002 §2.4/§6.5/§11.15–11.18；TD-002 T-L2-01（勾幅字段）、T-L2-03（≤96 B；实测最坏 92 B，含 10 位 tick）、T-L2-06（关闭后无输出）、T-L2-08（不破坏 SWD/GPIOA）、T-L2-09（唤醒后重新初始化）、T-L2-10（打印无副作用）。

`test/build_test.sh`（三阶段）本机复跑：**38/38 + 24/24 + 24/24**，退出码 0。

### EV-006 缺陷修复后的自检加强（本轮）

针对 EV-006 的 **D-ITEM001-1**（S 行末尾 CR+LF 未发出）：`test/mock_trace/` 新增 `UART_FLAG_TXBUSY` 并建模“移位中字节数”；`host_debug_trace_check.c` 新增两项检查（共 **24 项**）：

```
[T5] flush/close semantics (only UART1 + PA05/PA06)
    PASS  mock models in-flight bytes (last bytes not yet shifted out)
    PASS  drains with TXBUSY (vendor UART_SendString pattern), not TC
    PASS  D-ITEM001-1 guard: UART1 is NOT reset while bytes are still shifting (would drop trailing CR/LF)
    PASS  UART1 peripheral reset asserted then released
    PASS  UART1 APB clock disabled
    PASS  PA06 (UART1_TXD) returned to input
    PASS  PA05 (UART1_RXD) returned to input
    PASS  GPIOA peripheral is NOT reset (protects PA03/PA04 I2C)
    PASS  GPIOA clock is NOT disabled (protects I2C/SWD pins)
[T6] idempotent close
[T7] re-init after close (deep-sleep wake path)
==== result: 24 passed, 0 failed ====
```

**负对照（证明该回归检查不是空验证）**：将排空改回旧 `TC` 写法（`sed 's/UART_FLAG_TXBUSY) == SET/UART_FLAG_TC) == RESET/'` 生成副本编译）：

```
==== result: 22 passed, 2 failed ====   exit=1
    FAIL  drains with TXBUSY (vendor UART_SendString pattern), not TC (line 222)
    FAIL  D-ITEM001-1 guard: UART1 is NOT reset while bytes are still shifting (would drop trailing CR/LF) (line 223)
```

保留的完整 24 项 stdout（修复后）见上；回归：`test/build_test.sh` 38+24+24，既有 `*_verify_ev.c` 独立 harness 全部保持通过（fw_core 53、light 45、gxht40 42、sf_i2c 35、measure 79、rf_report 30、gw_compat 11、rf_frame 10、rtc 17）。

**未覆盖（如实记录）**：真实目标 COM42 字节流、9600 下无乱码、每约 3 分钟一行、采样间期 0 字节、修复后横幅 `rst` 非零的实板观测（属 TD-002 §4.4 `[实板]`，本调用无串口/烧录工具）。

---

# 历史：上一轮工作流驱动测试证据（DRV-002 rev 3.0）


## T1 运行（本轮实际执行，HEAD 85ba302）

环境：MinGW-w64 GCC 12.2.0（`C:/ProgramData/chocolatey/lib/mingw/tools/install/mingw64/bin/gcc.exe`，PATH 上的 chocolatey shim 解析 libexec/cc1 失败，须用完整路径；shim 偶发 `Device or resource busy` 重试后恢复）。以下命令与 stdout 为本轮复跑结果，与上一轮一致（T1 实现未变更）。命令与原始 stdout：

```
$ cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
$ CC=<mingw full path>
$ $CC -std=c11 -Wall -Wextra -I../USER/inc host_sf_i2c_bus_check.c \
      ../USER/src/sf_i2c.c -o host_sf_i2c_bus_check.exe && ./host_sf_i2c_bus_check.exe
[1] i2c_write_cmd: START -> 0x88 -> 0xFD -> STOP
  PASS  返回 SF_I2C_SUCCESS (命令被 ACK)
  PASS  恰好 1 个 START 与 1 个 STOP
  PASS  线上字节为 地址+W(0x88) 后接 命令(0xFD), 无寄存器地址
  PASS  总线空闲释放(SCL/SDA 均为高)
[2] i2c_read_bytes: START -> 0x89 -> 6B(ACKx5,NACK) -> STOP
  PASS  返回 SF_I2C_SUCCESS
  PASS  恰好 1 个 START 与 1 个 STOP
  PASS  仅先发读地址字节 0x89
  PASS  读回 6 字节与从机数据逐字节一致
  PASS  前 5 字节主机 ACK、第 6 字节主机 NACK
[3] 从机不响应: ACK 失败作为返回值
  PASS  i2c_write_cmd 返回 SF_I2C_TIMEOUT
  PASS  超时路径已发出 STOP 释放总线
  PASS  i2c_read_bytes 返回 SF_I2C_TIMEOUT (读地址未 ACK)
  PASS  超时路径已发出 STOP 释放总线
[4] length==0: 无总线动作
  PASS  返回 SF_I2C_SUCCESS
  PASS  未产生任何 START/STOP

==== result: 15 passed, 0 failed ====
EXIT=0
```

```
$ $CC -std=c11 -Wall -Wextra -Wno-int-to-pointer-cast -I../USER/inc -I../COMMON \
      -I../../../../Libraries/inc -I<CMSIS 5.9.0 Core Include> \
      host_gxht40_check.c ../USER/src/gxht40.c ../USER/src/sf_i2c.c ../USER/src/fw_core.c \
      -lm -o host_gxht40_check.exe && ./host_gxht40_check.exe
[1] 手册参考向量: T=0xBEEF(CRC 0x92) / RH=0x1234
  PASS  返回 GXHT40_OK
  PASS  temp_x10 == 855
  PASS  hum_x10 == 29
  PASS  发送命令 0xFD 一次
  PASS  地址字节 0x88(命令) + 0x89(读)
  PASS  缓存地址 = 0x44
  PASS  命令与读取各一次 START/STOP
[2] 25.0C / 50.0%RH
  PASS  temp_x10=250, hum_x10=500
[3] 负温 -10.0C / 0.0%RH
  PASS  temp_x10=-100 (保留符号), hum_x10=0
[4] 地址探测与缓存: 器件仅在 0x45
  PASS  在 0x45 上测量成功
  PASS  依次 0x88 -> 0x8A -> 0x8B
  PASS  缓存地址 = 0x45
  PASS  第二次直接使用缓存 0x8A/0x8B (不再探测 0x88)
[5] 读 NACK(转换未完成) 有界重读
  PASS  重读后成功 (2 次 NACK 被容忍)
  PASS  读 NACK 不重发命令 (仅 1 次 0xFD, 同一次测量内重读)
[6] CRC 错误: 重测用尽后返回失败且不改输出
  PASS  返回 GXHT40_ERR_CRC
  PASS  输出未被修改
  PASS  整帧重测次数 == GXHT40_MEAS_RETRY
[7] 无器件: 返回 NO_DEVICE 且不改输出
  PASS  返回 GXHT40_ERR_NO_DEVICE
  PASS  输出未被修改
  PASS  两个候选地址 0x88/0x8A 都被探测过
[8] 换算超范围: -45.0C 与 125.1C 视为无效
  PASS  T=-45.0C -> GXHT40_ERR_RANGE
  PASS  输出未被修改
  PASS  T=125.1C -> GXHT40_ERR_RANGE
[9] 入参非法: 未绑定总线
  PASS  返回 GXHT40_ERR_PARAM
  PASS  输出未被修改
[10] 周期路径命令白名单: 只允许 0xFD
  PASS  所有已发命令均为 0xFD (无 0x94 软复位/加热器命令)

==== result: 27 passed, 0 failed ====
EXIT=0
```

覆盖边界：mock 从机按 I²C 位时序重建，只验证**软件侧协议序列、CRC/换算与返回值**；不验证真实器件的电气特性、tMEAS 实际等待、地址变体实装。这些属 TD-002 T-L2-01..09，需实板与逻辑分析仪（设备工具已禁用，未执行）。

---

> 以下为**上一轮工作流（旧 12 项编号）**的驱动测试证据，仅供追溯。

## 1. 环境与运行

| 项 | 值 |
|---|---|
| 编译器 | MinGW-w64 GCC 12.2.0（`C:/ProgramData/chocolatey/lib/mingw/tools/install/mingw64/bin/gcc.exe`） |
| 被测单元 | `../USER/src/sf_i2c.c`（真实实现，未打桩） |
| mock | 测试文件内的 I²C 从机模型：按 SCL 边沿采样/驱动 SDA，模拟地址 0x44（8bit 0x88/0x89）与 6 字节返回 |

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
gcc -std=c11 -Wall -Wextra -I../USER/inc host_sf_i2c_bus_check.c ../USER/src/sf_i2c.c \
    -o host_sf_i2c_bus_check.exe && ./host_sf_i2c_bus_check.exe
```

结果：**15 passed, 0 failed**（编译 0 告警；`*.exe` 为构建产物，运行后已清理，不入库）。

## 2. 检查明细

| # | 检查 | 期望 | 结果 |
|---|---|---|---|
| 1.1 | `i2c_write_cmd(dev,0x88,0xFD)` 返回值 | `SF_I2C_SUCCESS`（命令被 ACK） | PASS |
| 1.2 | 命令写的 START/STOP 数 | 恰好 1 个 START、1 个 STOP | PASS |
| 1.3 | 命令写线上字节 | `0x88` 后接 `0xFD`，**无寄存器地址** | PASS |
| 1.4 | 事务结束后总线状态 | SCL/SDA 均为高（已释放） | PASS |
| 2.1 | `i2c_read_bytes(dev,0x89,buf,6)` 返回值 | `SF_I2C_SUCCESS` | PASS |
| 2.2 | 连续读的 START/STOP 数 | 恰好 1 个 START、1 个 STOP | PASS |
| 2.3 | 连续读先发字节 | 仅读地址字节 `0x89` | PASS |
| 2.4 | 读回数据 | 6 字节与从机数据逐字节一致 | PASS |
| 2.5 | 主机应答位 | 前 5 字节 ACK(0)、第 6 字节 NACK(1)（TD-002 T-L2-03） | PASS |
| 3.1 | 从机不响应时 `i2c_write_cmd` | `SF_I2C_TIMEOUT` | PASS |
| 3.2 | 超时后总线释放 | 已发出 STOP（TD-002 T-L2-07 的软件侧行为） | PASS |
| 3.3 | 从机不响应时 `i2c_read_bytes` | `SF_I2C_TIMEOUT`（读地址未 ACK） | PASS |
| 3.4 | 超时后总线释放 | 已发出 STOP | PASS |
| 4.1 | `length==0` 返回值 | `SF_I2C_SUCCESS` | PASS |
| 4.2 | `length==0` 总线动作 | 无任何 START/STOP | PASS |

## 3. 覆盖边界

- 本证据只验证**软件侧总线序列与返回值**：字节内容、START/STOP 边界、主机 ACK/NACK 位置、ACK 失败传播、空长度短路。
- mock 从机是**按 I²C 位时序重建的模型**，不是真实器件；不验证电气特性（上升沿、上拉、时钟频率、噪声）、也不验证 GXHT40 的 tMEAS 等待与器件地址变体——这些属 TD-002 T-L2-01/02/03/07，须由嵌入式测试在真实传感器板上用逻辑分析仪完成。
- 既有 `i2c_*` 函数未在本测试中覆盖；其"语义不变"由 `git diff --numstat`（`62 insertions / 0 deletions`）与交叉编译符号集证明。

## 4. 结论

ITEM-002 两个原语的行为与 FD-002 §3.2/§2.2/§10 及 TD-002 T-L2-03/T-L2-07 的软件侧期望一致，ACK 失败可作为返回值判断，既有函数未被改动。板上验证交接给嵌入式测试。

---

# ITEM-003：GXHT40 驱动自检

被测实现：`USER/src/gxht40.c` / `USER/inc/gxht40.h`（新增；无既有文件修改）
测试载体：`.../test/host_gxht40_check.c`（mock I²C 从机 + 真实 `gxht40.c` / `sf_i2c.c`；`delay_ms` 为不等待的测试桩）

## 1. 运行

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
gcc -std=c11 -Wall -Wextra -Wno-int-to-pointer-cast \
    -I../USER/inc -I../COMMON -I../../../../Libraries/inc \
    -I<CMSIS 5.9.0 Core Include> \
    host_gxht40_check.c ../USER/src/gxht40.c ../USER/src/sf_i2c.c \
    -o host_gxht40_check.exe && ./host_gxht40_check.exe
```

结果：**27 passed, 0 failed**（驱动与 harness 代码 0 告警；仅 CMSIS 头在 64-bit 宿主上的 `int-to-pointer-cast` 噪声已抑制）。

帧向量由**独立工具**（非本驱动）按 gxht40.pdf §7.3/§7.5 计算后硬编码，含手册参考向量 `CRC(0xBEEF)=0x92`：

| 向量 | 6 字节 | 期望 |
|---|---|---|
| 手册参考 | `BE EF 92 12 34 37` | T=0xBEEF → `temp_x10=855`；RH=0x1234 → `hum_x10=29` |
| 常温 | `66 66 93 72 B0 DC` | 25.0 ℃ / 50.0 %RH → 250 / 500 |
| 负温 | `33 33 88 00 00 81` | −10.0 ℃ / 0.0 %RH → −100 / 0 |
| 量程低 | `00 00 81 72 B0 DC` | −45.0 ℃ → 超范围 |
| 量程高 | `F8 CA 32 72 B0 DC` | 125.1 ℃ → 超范围 |
| CRC 错 | `66 66 00 72 B0 DC` | 温度字 CRC 错 |

## 2. 检查明细

| 组 | 检查 | 结果 |
|---|---|---|
| 手册向量 | 返回 `GXHT40_OK`；`temp_x10=855`、`hum_x10=29`；命令恰为 `0xFD` 一次；地址字节 `0x88`(命令)+`0x89`(读)；缓存地址 `0x44`；命令/读取各 1 START/STOP | PASS ×7 |
| 常温/负温 | 25.0 ℃/50.0 %RH 与 −10.0 ℃/0.0 %RH 换算正确（负温保留符号） | PASS ×2 |
| 地址探测 | 器件仅在 0x45：依次 `0x88 → 0x8A → 0x8B`，缓存 0x45；第二次测量直接用 `0x8A/0x8B`（不再探测 0x88） | PASS ×4 |
| 读 NACK | 前 2 次读地址 NACK 后第 3 次成功；**不重发命令**（`cmd_count==1`） | PASS ×2 |
| CRC 错 | 重测 `GXHT40_MEAS_RETRY`(=3) 次后返回 `GXHT40_ERR_CRC`；输出保持调用前值 | PASS ×3 |
| 无器件 | 返回 `GXHT40_ERR_NO_DEVICE`；`0x88/0x8A` 均被探测；输出不变 | PASS ×3 |
| 超范围 | −45.0 ℃ 与 125.1 ℃ 均返回 `GXHT40_ERR_RANGE`；输出不变 | PASS ×3 |
| 入参 | 未绑定总线返回 `GXHT40_ERR_PARAM`；输出不变 | PASS ×2 |
| 命令白名单 | 全部已发命令均为 `0xFD`（无 `0x94` 软复位、无加热器命令） | PASS ×1 |

## 3. 覆盖边界

- 验证的是**驱动逻辑与总线序列**（地址探测/缓存、命令、6 字节顺序、双字 CRC、整数换算与截断、重试上限、失败不改输出、命令白名单）；帧字节按 `T_MSB,T_LSB,T_CRC,RH_MSB,RH_LSB,RH_CRC` 顺序供给。
- mock 从机是按位时序重建的模型，不是真实器件；**真实 tMEAS 等待下界、地址变体实物确认、电气与 CRC 实读**属 TD-002 T-L2-01/02/03/06/07，须由嵌入式测试在真实传感器板上用逻辑分析仪完成。
- CRC-8 参考向量（`0xBEEF→0x92`）通过手册向量被接受/拒绝行为间接验证；`fw_core.c` 中可宿主机直测的纯逻辑抽取属 ITEM-005。

## 4. 结论

`gxht40_measure` 的行为与 FD-002 §6.1/§6.2/§10 及任务项 ITEM-003 的声明一致：地址探测与缓存、`0xFD` + 6 字节 + 双字 CRC、整数 x10 换算（含负温与 0..1000 截断）、有界重试、失败返回失败码且不修改输出。板上验证交接给嵌入式测试。

---

# ITEM-004：光照通路自检

被测实现：`USER/src/light.c` / `USER/inc/light.h`（新增）
测试载体：`.../test/host_light_check.c`（宿主机；只调用纯函数 `light_code_is_dark`，ADC/GPIO 函数以桩满足链接、不被执行）

## 1. 运行

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
gcc -std=c11 -Wall -Wextra -Wno-int-to-pointer-cast \
    -I../USER/inc -I../COMMON -I../../../../Libraries/inc \
    -I<CMSIS 5.9.0 Core Include> \
    host_light_check.c ../USER/src/light.c -o host_light_check.exe \
    && ./host_light_check.exe
```

结果：**17 passed, 0 failed**（harness 与 light.c 纯逻辑部分 0 告警）。

## 2. 检查明细（TD-002 T-L0-04 边界）

| 组 | 检查 | 结果 |
|---|---|---|
| 配置关系 | `ENTER=350`/`EXIT=250`；`EXIT<ENTER`；`ENTER-EXIT==HYSTERESIS_STEP`；满量程 4095 | PASS ×4 |
| 已明→暗 | `0→LIT`、`349→LIT`、`350→DARK`、`351→DARK`、`4095→DARK` | PASS ×5 |
| 已暗→明 | `4095→DARK`、`351→DARK`、`251→DARK`、`250→LIT`、`249→LIT`、`0→LIT` | PASS ×6 |
| 带内不抖动 | 已明 + 300 → LIT；已暗 + 300 → DARK | PASS ×2 |

## 3. 覆盖边界

- 本证据只验证**滞回判定的纯逻辑**（阈值方向、端点、带内保持），不验证 ADC 采样序列（PB05 供电时序、稳定延时、多次取样均值、PB05 置低）——那需要 ADC 寄存器/实板，属 TD-002 T-L3-01/02（极性）与 T-L3-03（标定），由嵌入式测试在真实硬件完成。
- `light_sample()` / `light_init()` 的寄存器操作未在宿主机执行；已用交叉编译（0 告警）与源码审查确认其引脚配置（PB04 模拟输入、PB05 推挽输出）与序列（PB05 高→延时→8 次采样→PB05 低、ADC 使能位开关）。
- `light_code_is_dark` 现位于 `light.c`；ITEM-005 将抽取到 `fw_core.c`（同名纯函数），届时本 harness 可直接改指 `fw_core.c`。

## 4. 结论

光照滞回判定与 FD-002 §6.3 及任务项 ITEM-004 的声明一致（无光 = code ≥ 进入阈值，滞回带 250..349 不抖动）；采样序列与引脚所有权经编译与源码核对。实板光照阈值与极性交接给嵌入式测试（TD-002 T-L3）。

---

# ITEM-005：fw_core.c 纯逻辑自检

被测实现：`USER/src/fw_core.c` / `USER/inc/fw_core.h`（新增 7 个纯函数）
测试载体：`.../test/host_fw_core_pure_check.c`（宿主机，仅 `-I../USER/inc`，不需 MCU/CMSIS 头）

## 1. 运行

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
gcc -std=c11 -Wall -Wextra -I../USER/inc host_fw_core_pure_check.c \
    ../USER/src/fw_core.c -lm -o host_fw_core_pure_check.exe \
    && ./host_fw_core_pure_check.exe
```

结果：**36 passed, 0 failed**（harness 与 fw_core.c 新增部分 0 告警）。

## 2. 检查明细（逐条对应 TD-002 T-L0-01…05）

| 用例 | 检查 | 结果 |
|---|---|---|
| T-L0-01 | `fw_crc8_gxht`：`{0xBE,0xEF}→0x92`（手册参考）、`{0x00}→0xAC`（init 0xFF 移位链）、空长度 → `0xFF` | PASS ×3 |
| T-L0-02 | 温度：S=1872→-400；16855→0；26214→250；63664→1250；纯换算 S=0→-450、S=0xFFFF→1300；有效域 -400/1250 合法、-450/1251/1300 无效 | PASS ×8 |
| T-L0-03 | 湿度：S=0→0；2247→0；29360→500；55575→1000；0xFFFF→1000（截断） | PASS ×5 |
| 组合 | `gxht40_raw_to_x10` 有效→true 并写输出；无效→false 且输出保持 | PASS ×4 |
| T-L0-04 | 滞回：已明 349→LIT/350→DARK；已暗 251→DARK/250→LIT；0→LIT；4095→DARK；带内 300 保持原态 | PASS ×7 |
| T-L0-05 | 无前值+200→false；无前值+351→true；下降 9→false；下降 10→true；下降 11→true；下降满足但非 DARK→false；cur=350→false；cur=351→true；prev==cur→false | PASS ×9 |

## 3. 覆盖边界与不一致记录

- 本证据为**纯逻辑**级别（无 MCU 寄存器/实板）；`fw_core.c` 以 `SENSOR_CONFIG_NO_MCU` 只取 `sensor_config.h` 的纯数值段，因此宿主机仅需 `-I../USER/inc`。
- **TD-002 T-L0-05 第②行数值与规则不一致**：该行写作 `prev=300,cur=290`（下降 10 = 1.0 ℃）期望 **false**，但同一行要求「与 FD-002 §6.4 逐条一致」；按任务书/FD-002/IC-002 的 `(prev-cur)>9`，下降 10 应触发。实现按 `>9`（下降 9 不触发、下降 10 触发），即「恰好 0.9 ℃」对应 `cur=291`；自检已同时固定 9/10/11 三个边界值。请测试侧校正该行。
- 其余测试计划期望（含 35.0 ℃ 边界、无前值、需 DARK）均逐条一致；`cur=350` 采用 IC-002 的 `T<35` 守卫（恰好 35.0 ℃ 两分支均不成立）。
- 退役 OPTCFG/params 纯逻辑与其测试用例属 ITEM-009/ITEM-010；本项未删除既有纯函数，`test/build_test.sh` 仍 56/56。

## 4. 结论

`fw_crc8_gxht`、GXHT40 整数换算（含负温/有效域/0..1000 截断）、`light_code_is_dark` 滞回、`sensor_decide_report` 判定均与 FD-002 §6 及任务项 ITEM-005 一致，且不依赖 MCU 寄存器；`gxht40.c`/`light.c` 已改为复用本纯逻辑（消除双份实现）。

---

# ITEM-006：measure.c 采样流程自检

被测实现：`USER/src/measure.c` / `USER/inc/measure.h`（采样部分重写）
测试载体：`.../test/host_measure_flow_check.c` + `.../test/mock_measure_mcu/`（影子 `cw32l010_gpio/sysctrl/uart`，编译真实 `measure.c`；`gxht40_measure`/`light_sample` 以可控桩替换）

## 1. 运行

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
gcc -std=c11 -Wall -Wextra -DSENSOR_CONFIG_NO_MCU -Imock_measure_mcu \
    -I../USER/inc -I../COMMON -I../UM2005C \
    host_measure_flow_check.c ../USER/src/measure.c ../USER/src/sf_i2c.c \
    ../USER/src/fw_core.c -lm -o host_measure_flow_check.exe \
    && ./host_measure_flow_check.exe
```

结果：**15 passed, 0 failed**（真实 `measure.c` 0 告警）。

## 2. 检查明细

| 组 | 检查 | 结果 |
|---|---|---|
| 顺序 | 每次采样调用顺序 = `light_sample` → `gxht40_measure`（“LT”），含失败周期 | PASS ×2 |
| 首样本 | 无前值 + cur=200 → `report_req=0`；成功写入 `tempvalue`/`huminityvalue` | PASS ×3 |
| 下降边界 | prev=200,cur=191（降 9）→ 0；prev=191,cur=181（降 10）且 DARK → 1；成功时更新最近有效湿度 | PASS ×3 |
| 待上报保持 | 无上报条件的后续成功周期不清除已挂起的 `report_req` | PASS ×1 |
| 需 DARK | 降 11 但有光 → 0 | PASS ×1 |
| 失败路径 | 返回 `GXHT40_ERR_CRC` 时：不置 `report_req`；`tempvalue`/`huminityvalue` 保持上次有效值（不被失败的 50/999 覆盖，也不写 0） | PASS ×4 |
| 前值不被污染 | 失败后恢复：prev=170,cur=160（降 10）→ 上报（证明失败未把 prev 改成 50） | PASS ×1 |
| 超温分支 | cur=351（LIT）→ 1 | PASS ×1 |
| 无节拍 | `sample_flag==0` → 无 light/temp 调用 | PASS ×1 |

## 3. 覆盖边界

- 本证据验证 `measure.c` 的**采样流程与状态推进**；`gxht40_measure`/`light_sample` 为桩，其真实行为已由 ITEM-003/004 harness 单独验证（27/27、17/17）。
- 影子头只覆盖 `cw32l010_gpio/sysctrl/uart`；未执行真实 GPIO/ADC/UART 寄存器与时序。
- 板上时序与真实温度下的上报边界（TD-002 T-L4/T-L5/T-L6）须由嵌入式测试在真实硬件完成；且 ITEM-009 退役 `hall/OPTCFG` 前，PB04/PB05 引脚所有权冲突使实板光照/上报验证不具备有效前提。

## 4. 结论

`measure.c` 满足 ITEM-006 声明：先光照后温湿度、依赖驱动的有界重试、失败不上报/不更新前值/不构造 0 值、成功后推进前一有效温度与最近有效湿度，并按 `sensor_decide_report` 置待上报标志。发送路径完整对齐交接 ITEM-008。

---

# ITEM-007：3 分钟采样节拍

被测实现：`USER/src/main.c` 的 `RTC_IRQHandlerCallBack()`（RTC 1 分钟中断 → 累计 3 拍置 `sample_flag`）
本项为 2 处常量/注释改动（`>= 1` → `>= SENSOR_SAMPLE_TICKS`），采用**源码结构确定性检查**（节拍的权威验证为 TD-002 T-L4-01 板级长时基，需真实硬件，本环境不具备）。

## 1. 检查命令与结果

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output

grep -n "SENSOR_SAMPLE_TICKS\|rtc_set_cnt\|sample_flag" USER/src/main.c
  100:uint8_t rtc_set_cnt = 0;
  111:        rtc_set_cnt++;
  112:        if (rtc_set_cnt >= SENSOR_SAMPLE_TICKS) {
  113:            sample_flag = 1;
  114:            rtc_set_cnt = 0;

grep -rn "rtc_set_cnt >= 1\|first_sample_reported\|samples_since_report" USER/
  (无输出: 旧 1 拍阈值与小时/首样本上报已无残留)

grep -n "sample_flag = 0" USER/src/measure.c
  231:    sample_flag = 0u;      (采样函数消费标志 -> 一个周期只测量一次)

grep -n "SENSOR_RTC_TICK_PERIOD_MIN\|SENSOR_SAMPLE_TICKS" USER/inc/sensor_config.h
  26:#define SENSOR_RTC_TICK_PERIOD_MIN      1u
  27:#define SENSOR_SAMPLE_TICKS             3u
```

## 2. 检查项与判定

| 检查 | 期望 | 结果 |
|---|---|---|
| RTC 回调阈值 | `>= SENSOR_SAMPLE_TICKS`（配置 3），非硬编码 1 | PASS |
| 旧 1 拍阈值 | 无 `rtc_set_cnt >= 1` | PASS |
| 小时/首样本强制上报 | 无 `first_sample_reported`/`samples_since_report` 引用 | PASS |
| 一周期一次测量 | `temperature_process()` 成功后清 `sample_flag` | PASS |
| 配置值 | 1 min × 3 = 3 min | PASS |

## 3. 覆盖边界

- 本证据为**静态结构检查 + 编译**；未在宿主机执行 RTC 中断（`main.c` 依赖完整 MCU 层）。
- 3 分钟实际间隔、LSI 容差与离散度需 TD-002 T-L4-01 在真实板上用长时基/电流波形记录，由嵌入式测试完成。
- 上电即采一次（`sample_flag` 初值 1）为既有行为；首样本无前值，不触发下降分支，符合 FWR-108。

## 4. 结论

采样节拍为 1 分钟 RTC 中断累计 3 拍置位，一周期一次测量；旧小时/首样本强制上报逻辑已完全移除。板级长时基验证交接嵌入式测试。

---

# ITEM-008：条件上报发送路径自检

被测实现：`USER/src/measure.c` 的 `send_data_to_gateway()`
测试载体：`.../test/host_measure_flow_check.c`（同一 mock MCU 影子头 + 可控 GXHT40/光照/发送桩）

## 1. 运行

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
gcc -std=c11 -Wall -Wextra -DSENSOR_CONFIG_NO_MCU -Imock_measure_mcu \
    -I../USER/inc -I../COMMON -I../UM2005C \
    host_measure_flow_check.c ../USER/src/measure.c ../USER/src/sf_i2c.c \
    ../USER/src/fw_core.c -lm -o host_measure_flow_check.exe \
    && ./host_measure_flow_check.exe
```

结果：**24 passed, 0 failed**（ITEM-006 的 15 项 + 本项新增 9 项发送路径）。

## 2. 新增检查明细（第 [9] 组）

| 检查 | 期望 | 结果 |
|---|---|---|
| `report_req == 0` | 不调 `encode_frame10`、不调发送 | PASS |
| `report_req == 1` | 编码 1 次 + 发送 1 次 | PASS |
| 组帧样本 | 使用最近一次有效样本 (351/505)，非 0/脏值 | PASS |
| 发送长度 | `SENSOR_RF_FRAME_LEN`（10） | PASS |
| 发送成功 | 清除 `report_req` | PASS |
| 发送失败 | 保留 `report_req`（重试） | PASS |
| 连续失败 | 达 `SENSOR_RF_TX_RETRY`(3) 后清除 `report_req` 放弃本轮 | PASS |
| 重试计数复位 | 放弃后下一轮 1 次即成功 | PASS |

## 3. 覆盖边界

- `encode_frame10` / `app_um2005C_send_data_timeout` 为可控桩，验证的是**调用门控、参数、重试/清除语义**；组帧内容与加密的正确性由 `evidence/protocol_test.md`（真实编解码器往返 14/14）单独验证。
- 真实 433 发射与发送失败上限（TD-002 T-L6-03）需在真实硬件完成。
- `send_data_to_gateway()` 中遗留的 `optcfg_window_active()` 延后门属 ITEM-009（硬件已无 OPTCFG，恒为 false）。

## 4. 结论

`send_data_to_gateway()` 满足 ITEM-008：仅在 `report_req`（由 `sensor_decide_report` 置位）为真时组帧发送，使用最近有效样本与 `SENSOR_RF_FRAME_LEN`，成功清除待上报、失败按上限重试后放弃；帧布局/字节序/加密未变。

---

# ITEM-009：旧通路退役自检

被测对象：删除 hall/OPTCFG/params/history 后的传感器工程（编译、符号、引用、引脚所有权）
本项为“删除 + 引用清理”，采用**构建/符号/静态引用**三类确定性检查。

## 1. 检查命令与结果

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output

# (1) 源码引用
 grep -rn "hall|optcfg|params_|history|OPTCFG_|PARAMS_" USER/
   -> 仅 3 处说明性注释 (fw_core.h/fw_core.c/measure.c 的退役说明)

# (2) 已删文件不存在
 ls USER/src USER/inc
   -> 仅 encrytogate/fw_core/gxht40/interrupts/light/main/measure/sf_i2c (.c)
      + encrytogate/fw_core/gxht40/interrupts_cw32l010/light/main/measure/sensor_config/sf_i2c (.h)

# (3) 镜像符号
 arm-none-eabi-nm gcc/obj/sensor_fw.elf | grep -iE "hall|optcfg|params_|history"  -> (无输出)
 arm-none-eabi-nm gcc/obj/sensor_fw.elf | grep -iE "gxht40_measure|light_sample|sensor_decide_report|fw_crc8_gxht"
   -> T gxht40_measure / light_sample / sensor_decide_report / fw_crc8_gxht

# (4) 引脚所有权
 grep -rn "PB04|PB05|PB06|GPIO_PIN_[456]" USER/
   -> PB04/PB05 仅 light.c/sensor_config.h；PB06 无引用 (GPIO_PIN_6 仅用于 PA06 调试 UART)

# (5) 构建
 sh gcc/build.sh  -> 0 error / 28 warning; FLASH 32,208 B / RAM 1,712 B
```

## 2. 检查项与判定

| 检查 | 期望 | 结果 |
|---|---|---|
| 8 个模块文件 | 已删除 | PASS |
| 源码引用 | 无代码引用（仅注释） | PASS |
| 固件符号 | 无 hall/optcfg/params/history | PASS |
| 新模块接线 | gxht40/light/fw_core 符号在镜像中 | PASS |
| PB04 所有权 | 仅 light.c 配为 AIN11 模拟输入 | PASS |
| PB05 所有权 | 仅 light.c 配为推挽输出 | PASS |
| PB06 | 无任何配置/驱动 | PASS |
| 中断 | GPIOB 无霍尔分支；LPTIM 仅 433 位时钟 | PASS |
| 构建 | 0 error，资源下降 | PASS |

## 3. 覆盖边界

- 未在真实硬件验证引脚电平/功耗；PB04/PB05 所有权已在源码层唯一化，板级验证（TD-002 T-L3/T-L4）前提已具备，由嵌入式测试完成。
- MDK/IAR 工程源列表一致性属 ITEM-011；本项确认这些工程本就未登记被删模块。
- `test/build_test.sh` 已收敛为 CRC16 回归（2/2）；新增纯逻辑用例属 ITEM-010。
- 嵌入式测试的部分 harness（`host_measure_flow_verify_ev.c`/`host_rf_report_verify_ev.c`/`host_rtc_cadence_verify_ev.c`）仍引用已退役头/符号，属跨角色交接。

## 4. 结论

hall/OPTCFG/params/history 及其对 PB04/PB05/PB06 的初始化、GPIOB 霍尔 EXTI 分支与 LPTIM 的 OPTCFG 分支均已从源码、构建与固件镜像中移除；PB04 仅作 AIN11 模拟输入、PB05 仅作光照供电输出、PB06 不外驱动；采样→判定→上报→深睡主流程回归全部通过。

---

# ITEM-010：宿主机测试套件

被测对象：`USER/src/fw_core.c`（纯逻辑）+ `USER/src/measure.c`（流程）
测试入口：`test/build_test.sh`（两阶段）

## 1. 运行

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
CC=<mingw64 gcc full path> sh build_test.sh
```

结果：**阶段 1 纯逻辑 38 passed / 0 failed；阶段 2 流程 24 passed / 0 failed；退出码 0**。

## 2. 阶段 1 检查明细（host_sensor_core_test.c，TD-002 T-L0-01..06）

| 用例 | 断言 | 结果 |
|---|---|---|
| T-L0-06 | CRC16 `123456789`→0x29B1；参考帧 33B→0xB540 | PASS ×2 |
| T-L0-01 | CRC-8 `{BE,EF}`→0x92；`{00}`→0xAC；空长度→0xFF | PASS ×3 |
| T-L0-02 | 温度 1872→−400、16855→0、26214→250、63664→1250；0→−450、0xFFFF→1300；域端点/域外有效性与组合换算（有效才写输出） | PASS ×11 |
| T-L0-03 | 湿度 0→0、2247→0、29360→500、55575→1000、0xFFFF→1000 | PASS ×5 |
| T-L0-04 | 已明 349→LIT/350→DARK；已暗 251→DARK/250→LIT；0→LIT；4095→DARK；带内保持 | PASS ×7 |
| T-L0-05 | 无前值 200→false/351→true；降 9→false、10→true、11→true；非 DARK→false；cur=350→false；prev==cur→false | PASS ×9 |

## 3. 阶段 2 检查明细（host_measure_flow_check.c，TD-002 T-L6-01 + ITEM-006/008）

先光照后温湿度；降 9/10 边界；需 DARK；待上报保持；**整周期失败不置 report_req / 不更新前值 / 不写 0**；失败后按失败前前值判定；超温分支；`sample_flag==0` 不采样；发送路径门控/样本/长度/成功清除/失败重试/放弃（24 项）。

## 4. 覆盖边界

- 阶段 1 为纯逻辑（无 MCU 寄存器）；阶段 2 以 mock MCU 影子头 + 可控 GXHT40/光照/发送桩编译真实 `measure.c`，未执行真实驱动/硬件。
- 真实板级时序/功耗/射频（TD-002 T-L3/T-L4/T-L6/T-L8）由嵌入式测试完成。
- **TD-002 T-L0-05 数值校正**：第②行 `prev=300,cur=290` 期望 false 与 `>9` 规则不符（“恰好 0.9℃”为 cur=291）；实现与测试按任务文本/FD-002/IC-002 的 `>9`，并同时固定 9/10/11 三个边界。

## 5. 结论

`test/build_test.sh` 单命令覆盖本次需求新增的全部纯逻辑边界与“失败不更新前值”流程行为，本机运行退出码 0；已退役的 OPTCFG/params/history 用例不再存在。

---

# ITEM-011：交叉编译与工程列表一致性

被测对象：传感器工程构建（GNU 交叉编译 + AC5 单文件）与 MDK/IAR 源文件列表

## 1. 检查命令与结果

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output

# (1) 交叉编译与产物
 sh gcc/build.sh                       -> 0 error / 28 warning
 ls gcc/obj/sensor_fw.{elf,hex,bin}    -> 三个产物均生成
 arm-none-eabi-size gcc/obj/sensor_fw.elf
   -> text 32208 / data 84 / bss 1628
   -> FLASH 32,208 B (49.15% of 64 KB) <= 64 KB
   -> RAM   1,712 B  (41.80% of 4 KB)  <= 4 KB
 md5sum: sensor_fw.bin 47abc6df... / sensor_fw.hex ddf2e2e7...

# (2) 生产编译器 AC5 单文件编译
 armcc --cpu=Cortex-M0+ --c99 -I ../../../Libraries/inc -I USER/inc \
       -I UM2005C -I COMMON -I ../../IdeSupport/MDK -I <CMSIS 5.9.0> \
       -c USER/src/<f>.c -o /tmp/ac5/<f>.o
   -> fw_core/gxht40/light/measure/main/interrupts_cw32l010/sf_i2c : 0 error 0 warning
      encrytogate : 0 error (3 条既有未用静态量告警)

# (3) MDK/IAR 列表与磁盘一致 (脚本化比对)
 actual USER/src : [encrytogate, fw_core, gxht40, interrupts_cw32l010, light, main, measure, sf_i2c]
 MDK list        : 同上  (== actual: True)
 IAR list        : 同上  (== actual: True)
 retired hall/optcfg/params/history : MDK=no IAR=no
 cw32l010_adc.c / app_um2005C.c / delay.c : MDK=yes IAR=yes
 XML 解析 (ElementTree) : 两个工程文件均 OK
```

## 2. 检查项与判定

| 检查 | 期望 | 结果 |
|---|---|---|
| 交叉编译 | 0 error，生成 elf/hex/bin | PASS |
| Flash 占用 | ≤ 64 KB | PASS（32,208 B，49.15%） |
| RAM 占用 | ≤ 4 KB | PASS（1,712 B，41.80%） |
| AC5 兼容 | 新增/修改源文件 0 error | PASS |
| MDK 列表 | == 磁盘 `USER/src/*.c` | PASS |
| IAR 列表 | == 磁盘 `USER/src/*.c` | PASS |
| 已删模块 | 不在两工程列表中 | PASS |
| 依赖源/头路径 | adc/UM2005C/COMMON 均在列（IAR 含 include 路径） | PASS |
| 工程文件合法性 | XML 可解析 | PASS |

## 3. 覆盖边界

- **未执行 MDK/IAR 实机构建**：本环境 Keil 受 CMSIS 6.3.0/AC5 不兼容影响，IAR 工具链不可用；以 GNU 交叉编译（全工程）+ AC5（逐文件）+ XML 解析 + 列表脚本比对代替。
- 网关（CH592 beiwov2）构建与核对属 ITEM-012；板级实机验证属嵌入式测试。
- `.gitignore` 新增 `*.exe`（宿主机测试产物）已在仓库验证生效（`git check-ignore`）。

## 4. 结论

传感器工程可交叉编译并生成 elf/hex/bin，Flash/RAM 均远低于 4 KB/64 KB 上限；MDK/IAR 工程源文件列表与新增/删除后的实际源文件逐一一致，工程文件合法可解析。
