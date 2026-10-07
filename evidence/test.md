# 嵌入式验证证据（EV-004 / ITEM-001 ↔ T1 GXHT40 温湿度采集通路）

状态：embedded_tester.embedded_verification 独立验证证据
本轮验证对象：任务队列 **ITEM-001（= 任务清单 T1：GXHT40 温湿度采集通路）**
受测提交：`f59b656`（firmware_engineer.firmware_implementation 的 IMPLEMENTED 提交；本轮开始时 tracked 工作树干净，仅有无意落盘的 MDK 构建日志，已在记录原始输出后删除）
验证基线：TD-002 **rev 3.0** `artifacts/test_design.md`（本能力设计，`n004`）
上游输入：`artifacts/firmware_implementation.md`（FWI-002 rev 3.0 / ITEM-001 节）、`artifacts/firmware_design.md`（FD-002 rev 3.0 §2.2/§6.1/§6.2/§10）、`artifacts/firmware_requirements.md`（FWR-101/107）、`artifacts/interface_contract.md`（IC-002 v3.0 §3）、`gxht40.pdf`
结论：**BLOCKED** —— 宿主机侧独立验证全部通过（含独立参考帧与定向变异负对照，未发现实现缺陷），但 TD-002 rev 3.0 §2.2 对 B1/T1 **强制要求**的真实目标观测（T-L2-01/02/03 实板 I²C 抓包；T-L2-04/05 参考温湿度计对照；T-L2-06/07/08/09 真实故障注入与地址变体）在本环境**不可执行**：无调试探针、无串口、无逻辑分析仪。按能力约定「required real-target work not performed is BLOCKED, not a pass with a handoff note」，本项不判 TEST_PASS，也不判 TEST_FAIL（未发现缺陷）。

> 说明：本能力工具面本轮新增 `mdk_build`/`mdk_flash`/`serial_*`。已按要求**重新核查**环境（不沿用上一轮的失败结论），核查结果见 §1：仍无探针/串口。

---

## 1. 环境与工具（本轮实际调用，原始输出）

| 工具调用 | 原始结果（节选） | 结论 |
|---|---|---|
| `serial_list_ports {}` | `{"tool":"serial_list_ports","success":true,"exit_code":0,"stdout":"[]", ...}` | **无任何串口**（配置候选 `COM43` 不在位） |
| `usb_list_devices {}` | `Chicony USB2.0 Camera`、`Logitech HID VID_046D:PID_C52F`、USB3 根集线器、`Intel Bluetooth VID_8087:PID_0033` | **无 SWD/调试探针**（无 CMSIS-DAP/J-Link/WCH-Link）、无 USB-TTL |
| `serial_capture {port:COM43,baudrate:9600,duration_sec:3}` | `RuntimeError: pyserial is required for serial_capture` | 串口采集不可用（双前置缺失：无端口 + 运行时缺 pyserial） |
| `mdk_build {action:build}` | `Using Compiler 'V6.24' ... 0 Error(s), 0 Warning(s)` | 工程可构建；但 `.axf` mtime=Oct 3（源码自 Oct 3 未变），需强制 rebuild 才有编译证据 |
| `mdk_build {action:rebuild}` | `Rebuild target 'Project' ... compiling gxht40.c ... compiling fw_core.c ... linking ... Program Size: Code=8498 RO-data=598 RW-data=76 ZI-data=1620 ... 0 Error(s), 0 Warning(s)` | **当前源码在 ARMCLANG V6.24 下全量重编译 0 错 0 警** |
| `mdk_flash {}` | `Load "...Project.axf"` → `Internal DLL Error` / `Error: Flash Download failed  -  Target DLL has been cancelled`（`success:false, exit_code:2, error_code:FLASH_LOG_ERROR`） | **无调试器，无法下载/回读校验**（T-L1-09/T-L1-12 BLOCKED） |

工具副作用：`mdk_build`/`mdk_flash` 在仓库根产生 `build.log`/`build_*.log`，已读入本证据后删除，未入库。

---

## 2. 选测项与理由

任务 ITEM-001 验收文字逐条拆成可观测行为，并按 TD-002 rev 3.0 选取手段。

| 任务验收行为 | TD-002 rev 3.0 用例 | 本轮实际执行（宿主机） | 强制真实目标（未执行） |
|---|---|---|---|
| 探测 0x44/0x45（8bit 0x88/0x89/0x8A/0x8B）带缓存 | T-L0-07 / T-L2-01 / T-L2-09 | 位级 mock 总线 + 真实驱动：两地址、探测顺序、缓存后不再重扫 | 真实器件变体、上拉、每字节 ACK（T-L2-01/09） |
| 发 0xFD 高重复率命令 | T-L0-07 / T-L2-01 | 命令白名单：历史全部命令字节必须为 0xFD | 真实线上字节（T-L2-01） |
| 按 tMEAS 上限等待后读 6 字节 | T-L0-07 / T-L2-02/03 | 断言一次 `GXHT40_MEASURE_WAIT_MS`(10 ms)；6 字节顺序、前 5 ACK/末 NACK | STOP→读真实时间差 ≥8.3 ms（T-L2-02） |
| 温度字/湿度字分别 CRC-8(0x31,init 0xFF) | T-L0-01 | 独立 Python oracle 复算 + mock 帧双字 CRC 校验路径 | 真实器件 CRC 行为（T-L2-01/03） |
| 温度 x10 有符号 −400..1250 | T-L0-02 | oracle 全量边界 + 驱动有效性端点（−40.0/125.0 ℃） | 与参考温湿度计一致（T-L2-04/05） |
| 湿度 x10 截断 0..1000 | T-L0-03 | raw 0xFFFF→1000、raw 0→0 | 同上（T-L2-04） |
| 读 NACK / CRC 错 / 无器件有界重试 | T-L0-07 / T-L6-06 / T-L2-06/08 | mock 注入 NACK/CRC/无器件，断言上限与有界 | 真实 NACK 时序、故障注入恢复（T-L2-06/07/08） |
| 整次失败返回失败码且不改输出 | T-L0-07 | 每类失败码 + 输出哨兵值不变 | 实板失败路径（T-L2-06/08） |

**为何宿主机仍不可替代真实目标**：mock 从机是按手册重建的模型，只能证明驱动软件逻辑；真实器件 ACK/NACK、tMEAS、电气时序与实测值只有实板能证。TD-002 §2.2 因此把 B1 的真实目标观测列为强制项。

---

## 3. 宿主机独立验证结果（命令与原始结果）

### 3.1 独立驱动 harness（真实驱动 + 位级 mock 总线）

```
$ cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
$ gcc -std=c11 -Wall -Wextra -Wno-int-to-pointer-cast -Itest/mock_mcu -IUSER/inc -ICOMMON \
      test/host_gxht40_verify_ev.c USER/src/gxht40.c USER/src/sf_i2c.c USER/src/fw_core.c \
      -o /tmp/ev004/gxht40_verify.exe && /tmp/ev004/gxht40_verify.exe
[T1] manual reference frame 0xBEEF/0x1234
    PASS  returns GXHT40_OK / temp_x10=855, hum_x10=29 (manual vector)
    PASS  exactly one command byte 0xFD / write 0x88 / read 0x89 / cache=0x44
    PASS  one tMEAS wait of the configured 10 ms
[T2] 25.0C/50%->250/500 ; -10.0C/0%->-100/0 (sign) ; hum 0xFFFF->1000 ; hum 0->0
[T3] device only at 0x45: probe 0x88 then 0x8A, read 0x8B, cache=0x45; 2nd measure uses cache
[T4] 2x read NACK -> success; command NOT re-sent; 3 read attempts; two 1 ms waits
[T5] NACK beyond bound -> GXHT40_ERR_IO; outputs untouched; attempts bounded by MEAS_RETRY / MEAS_RETRY*READ_RETRY
[T6] temp-word CRC bad -> GXHT40_ERR_CRC; hum-word CRC bad -> GXHT40_ERR_CRC; outputs untouched; re-measured==3
[T7] raw 0 (-45.0C) -> GXHT40_ERR_RANGE ; 125.1C -> GXHT40_ERR_RANGE ; outputs untouched
[T8] no device -> GXHT40_ERR_NO_DEVICE (both 0x88/0x8A probed); outputs untouched; bounded
[T9] NULL temp / NULL hum / unbound bus -> GXHT40_ERR_PARAM
[T10] every command byte ever sent was 0xFD (23 total)
==== result: 42 passed, 0 failed ====   exit=0
```

说明：实现 ITEM-005 之后 `gxht40.c` 改为调用 `fw_core.c` 的 `fw_crc8_gxht`/`gxht40_raw_to_x10`，故本 harness 链接目标必须补 `USER/src/fw_core.c`（缺少时链接器报 `undefined reference to fw_crc8_gxht/gxht40_raw_to_x10`）。已据此更新 harness 头部注释中的可复现构建命令。

### 3.2 负对照（定向变异，证明用例不是空跑）

```
$ sed 's/fw_crc8_gxht(&buf\[3\], 2u) != buf\[5\]/fw_crc8_gxht(&buf[3], 2u) != buf[2]/' USER/src/gxht40.c > /tmp/ev004/gxht40_mutA.c
$ diff USER/src/gxht40.c /tmp/ev004/gxht40_mutA.c
125c125
<             (fw_crc8_gxht(&buf[3], 2u) != buf[5])) {
---
>             (fw_crc8_gxht(&buf[3], 2u) != buf[2])) {
$ gcc ... test/host_gxht40_verify_ev.c /tmp/ev004/gxht40_mutA.c USER/src/sf_i2c.c USER/src/fw_core.c -o /tmp/ev004/mutA.exe
$ /tmp/ev004/mutA.exe ; echo exit=$?
==== result: 27 passed, 15 failed ====   exit=1
```
→ 湿度字 CRC 比较字节被定向改错后 harness 失败 15 项且退出码 1，说明 42/42 的通过不是空跑。

### 3.3 相邻独立 harness（T1 相关软件面）

```
$ gcc ... test/host_sf_i2c_verify_ev.c USER/src/sf_i2c.c -o sfi2c_verify.exe && ./sfi2c_verify.exe
==== result: 35 passed, 0 failed ====   exit=0   （命令写/连续读线上字节、START/STOP、主机 ACK/NACK 位置、超时释放、既有函数回归）

$ gcc -std=c11 -Wall -Wextra -Wno-int-to-pointer-cast -Itest/mock_mcu -IUSER/inc -ICOMMON \
      test/host_fw_core_verify_ev.c USER/src/fw_core.c -o fwcore_verify.exe && ./fwcore_verify.exe
==== result: 53 passed, 0 failed ====   exit=0   （其中 T1 相关：65536 温度字/65536 湿度字换算与量程、256x256 双字节 CRC-8 与独立 Python 参考哈希比对）
```

注：该 fw_core harness 的光照/上报用例当前仍按**rev 2.0 语义**（滞回、降温方向）断言，对应当前实现的 T2/T4 行为；本项只取其中的 **CRC-8/换算/量程** 作为 T1 证据，光照与上报留待 ITEM-002/ITEM-004 验证。

### 3.4 maker 自检复跑（交叉确认，不作为独立证据）

```
$ gcc ... test/host_gxht40_check.c USER/src/gxht40.c USER/src/sf_i2c.c USER/src/fw_core.c -o gxht40_check.exe && ./gxht40_check.exe
==== result: 27 passed, 0 failed ====
$ gcc ... test/host_sf_i2c_bus_check.c USER/src/sf_i2c.c -o sfi2c_check.exe && ./sfi2c_check.exe
==== result: 15 passed, 0 failed ====
$ sh test/build_test.sh
== [1/2] fw_core pure logic ==== result: 38 passed, 0 failed ====
== [2/2] measure flow (mock MCU) ==== result: 24 passed, 0 failed ====   exit=0
```

---

## 4. 当前交付件构建与静态核查

```
$ sh gcc/build.sh        -> 0 errors ; FLASH 32208 B / 64 KB (49.15%) ; RAM 1712 B / 4 KB (41.80%) ; elf/hex/bin 生成
$ arm-none-eabi-size gcc/obj/sensor_fw.elf
   text    data     bss     dec     hex filename
  32208      84    1628   33920    8480 gcc/obj/sensor_fw.elf
$ md5sum gcc/obj/sensor_fw.elf gcc/obj/sensor_fw.hex gcc/obj/sensor_fw.bin
24425d8d0f593ae3846fa9164c65c716 *sensor_fw.elf
ddf2e2e7bac47a80ea00177cb104afc9 *sensor_fw.hex
47abc6dfc798c837b2b68916f29db543 *sensor_fw.bin

$ arm-none-eabi-nm gcc/obj/{gxht40,sf_i2c,fw_core}.o | grep -cE '__aeabi_f|__aeabi_d|__float'
gxht40: 0 ; sf_i2c: 0 ; fw_core: 0        （T1 三个翻译单元无浮点依赖）
# 全 ELF 仍有浮点符号，按对象归因仅来自：um2005C.o(12)、cw32l010_uart.o(7)、cw32l010_adc.o(6)（既有厂商/驱动代码，非 T1）

$ mdk_build {action:rebuild}  -> Rebuild target 'Project' ... 0 Error(s), 0 Warning(s)
   Program Size: Code=8498 RO-data=598 RW-data=76 ZI-data=1620
$ md5sum MDK/output/exe/Project.axf -> c4861a3158a88f225d27ad915a27d918
$ arm-none-eabi-nm MDK/output/exe/Project.axf | grep -E 'gxht40_measure|fw_crc8_gxht|light_sample|i2c_read_bytes'
00001370 T fw_crc8_gxht ; 000013dc T gxht40_measure ; 000017ae T i2c_read_bytes ; 00001a68 T light_sample
```

交付件一致性：`git rev-parse HEAD` = `f59b656eb7d2f3f1b9719397869e726b3e28cbb1`；`git status --porcelain` 对 tracked 源码为空（仅本能力自己的 harness 注释修改），即被编译/验证的源码就是受测提交。

---

## 5. 强制真实目标项（本轮未执行，均为 BLOCKED）

| TD-002 用例 | 所需资源 | 本轮状态 | 阻塞证据 |
|---|---|---|---|
| T-L2-01 地址/0xFD/6B 线上时序 | 逻辑分析仪（PA03/PA04）| 未执行 | 无逻辑分析仪工具/仪器；无探针 |
| T-L2-02 tMEAS 下界 | 逻辑分析仪 | 未执行 | 同上 |
| T-L2-03 6 字节 ACK/NACK 时序 | 逻辑分析仪 | 未执行 | 同上 |
| T-L2-04 与参考温湿度计对照 | 可控温源 + 参考仪器 + 串口/钩子 | 未执行 | 无串口（`[]`）、无温源、钩子未接入 |
| T-L2-05 负温端到端 | 冷箱 + 网关/手机 | 未执行 | 同上 + 无网关/手机确认 |
| T-L2-06 CRC 错误路径 | 故障注入 + 逻辑分析仪 | 未执行 | 无仪器 |
| T-L2-07 总线卡死恢复 | 故障注入 | 未执行 | 无仪器 |
| T-L2-08 无器件 | 取下器件 + 电流表 | 未执行 | 无仪器；无法下载镜像 |
| T-L2-09 地址变体 | 两种变体实物 | 未执行 | 无实物 |
| T-L1-09 量产工具链下载回读 | 调试探针 | 未执行 | `mdk_flash` = `Flash Download failed - Target DLL has been cancelled` |
| T-L1-12 部署镜像=受测提交 | 调试探针 | 未执行 | 无法下载/回读 |

---

## 6. 判定、局限与交接

1. **判定**：T1 的宿主机可验证行为（地址探测与缓存、0xFD、tMEAS 等待、6 字节读取、双字 CRC-8、x10 换算与量程、读 NACK/CRC/无器件有界重试、失败不改输出、命令白名单）**未发现实现缺陷**；真实目标强制项不可执行，故本轮结果为 **BLOCKED**（不是 TEST_FAIL，也不是 TEST_PASS）。
2. **真实目标前置**（供后续执行）：调试探针 + 可下载环境（当前 `Target DLL cancelled`）、`COM43` 串口或等效、逻辑分析仪、可控温源/参考温湿度计、GXHT40 地址变体实物；另 `serial_capture` 仍需在配置运行时安装 `pyserial`（本轮报 `pyserial is required`）。
3. **观测钩子**：TD-002 §2 期望的 `SENSOR_TEST_TRACE` 打印点仍未接入；当前 T-L2-04/05 的内部读数只能靠空口/钩子，建议在具备实板时先接入观测点。
4. **后续任务项交接（不属本项）**：FD-002 rev 3.0 §1.1 的 D-01/D-02/D-03 仍未实现（光照 1/3 判据与有效性、升温方向上报、待上报冻结样本），对应任务项 ITEM-002/004/005；本项 T1 不受其影响（FD-002 rev 3.0 明确 T1 无变更）。
5. **跨项 harness 维护交接（后续 ITEM-003/004/005 验证时）**：`test/host_light_verify_ev.c`（需补 `fw_core.h`）、`test/host_measure_flow_verify_ev.c`、`test/host_rf_report_verify_ev.c`、`test/host_rtc_cadence_verify_ev.c` 仍引用已退役的 `optcfg.h`/`hall.h`/`params.h`/`history.h` 或旧语义；本轮属 T1，未改动它们。

---

# 历史记录（EV-003：上一轮工作流 WF-d5575697，基线 TD-002 rev 2.0；仅供追溯，勿与本轮混用）


状态：embedded_tester.embedded_verification 独立验证证据
本轮验证对象：任务队列 **ITEM-001（= 任务清单 T1：GXHT40 温湿度采集通路）**
被测提交：`cbb2b61`（firmware_engineer.firmware_implementation 的 IMPLEMENTED 提交，工作树干净）
验证基线：`dc5980b`（本能力自己的 TD-002 rev 2.0 测试设计）
结论：**BLOCKED** —— 宿主机侧独立验证全部通过（可复现、含独立 oracle 与负对照），但本项目 TD-002 §2.2 对 T1 强制要求的**真实目标观测（实板 I²C 抓包 / 参考温湿度计对照 / 真实器件 NACK 与 tMEAS）在设备工具禁用下未执行**。按验证能力约定「required real-target work not performed is BLOCKED, not a pass with a handoff note」，**本项不判 TEST_PASS，也不判 TEST_FAIL（未发现实现缺陷）**，阻塞原因是环境不可用。

上游输入：`artifacts/firmware_implementation.md`（FWI-002 rev 3.0 / T1 声明）、`artifacts/test_design.md`（TD-002 rev 2.0）、`artifacts/firmware_design.md`（FD-002 rev 2.0 §2.2/§6/§10）、`artifacts/firmware_requirements.md`（FWR-101/107）、`gxht40.pdf`、任务队列 ITEM-001 描述。
本文件新增内容为第 1–9 节；上一轮工作流的 EV-002 原文作为历史记录附于文末（Git 历史同样可追溯）。

---

## 1. 选测项与理由

任务 ITEM-001 的验收文字（队列描述）逐条拆成可观测行为，并按 TD-002 rev 2.0 §2.2 强制真实目标要求选取验证手段：

| 任务验收行为 | TD-002 用例 | 本轮选测（宿主机） | 强制真实目标（未执行） |
|---|---|---|---|
| 探测 0x44/0x45 地址（8bit 0x88/0x89/0x8A/0x8B）带缓存 | T-L0-07 / T-L2-01 / T-L2-09 | 位级 mock 总线 + 真实驱动，两地址与缓存路径 | 真实器件变体确认、空闲上拉、每字节 ACK（T-L2-01/09） |
| 发 0xFD 高重复率命令 | T-L0-07 / T-L2-01 | 命令白名单：全部命令字节必须为 0xFD | 真实线上字节（T-L2-01） |
| 按 tMEAS 上限等待后读 6 字节 | T-L0-07 / T-L2-02/03 | 断言一次 `GXHT40_MEASURE_WAIT_MS`(=10 ms) 延时；6 字节顺序、前 5 ACK/末 NACK | STOP→读的真实时间差 ≥8.3 ms（T-L2-02） |
| 温度字/湿度字分别 CRC-8(0x31,init 0xFF) | T-L0-01 | 独立 Python oracle 复算 CRC；mock 帧 CRC 校验路径 | 真实器件 CRC 行为（T-L2-01/03） |
| 温度 x10 有符号且 -400..1250 | T-L0-02 | oracle 全量边界 + 驱动有效性判定（-40.0/125.0 端点） | 真实读数与参考温湿度计一致（T-L2-04/05） |
| 湿度 x10 截断 0..1000 | T-L0-03 | raw 0xFFFF→1000、raw 0→0 | 同上（T-L2-04） |
| 读请求 NACK / CRC 错 / 无器件有界重试 | T-L0-07 / T-L6-06 / T-L2-06/08 | mock 注入 NACK/CRC/无器件，断言上限与有界 | 真实 NACK 时序、故障注入恢复（T-L2-06/07/08） |
| 整次失败返回失败码且不改输出 | T-L0-07 | 每类失败码 + 输出哨兵值不变 | 实板失败路径（T-L2-06/08） |

**为什么必须在宿主机做**：上述逻辑在真实目标上不可观测（无稳定周期日志，UART 仅活动期打开且无打印），TD-002 §1.2 已把「内部判定」的观测手段定为钩子或 mock；mock 可确定性地覆盖协议分支与边界，是唯一可重复的手段。
**为什么仍不能判通过**：mock 从机是按手册重建的模型，**不是真实器件**；它只能证明驱动软件逻辑，不能证明真实器件 ACK/NACK、tMEAS、电气时序与实测值。TD-002 §2.2 因此把 B1 的真实目标观测列为强制项。

## 2. 环境与工具（本轮实际调用）

| 工具 | 版本/路径 | 用途 |
|---|---|---|
| MinGW-w64 gcc | 12.2.0（`/c/ProgramData/chocolatey/bin/gcc`） | 宿主机编译驱动/harness/oracle 对照 |
| arm-none-eabi-gcc | 10.3.1 20210824 | 当前交付件交叉编译、浮点引用核查 |
| python | 3.10.9 | 独立参考 oracle（CRC-8/换算/量程） |
| git | 仓库版本 | 受测提交/工作树一致性 |
| SWD/J-Link、串口、逻辑分析仪、示波器、电源/电流表 | **未调用**：设备工具已禁用；宿主机仅暴露 read/bash/write 类能力，无 programmer/jlink/serial/logic_analyzer 工具；环境内也不存在 SEGGER J-Link（`JLink.exe` 为 OpenJDK 的 Java jlink）与逻辑分析仪 CLI | —— |

**结论：本轮不可能执行真实目标验证；未假定任何物理测试已执行。**

## 3. 独立验证方法（不采信 maker 自述）

1. **独立参考 oracle（我自己新写，不复用驱动代码）**：`/tmp/ev003/gxht40_ref.py` 按 gxht40.pdf 事实从头实现 CRC-8(poly 0x31,init 0xFF,MSB-first) 与两条换算公式，用于**独立复算** mock harness 里硬编码的 6 字节期望帧与所有边界值；harness 不参与生成期望。
2. **位级 mock 总线 + 真实驱动**：编译**真实** `USER/src/gxht40.c` + `USER/src/sf_i2c.c` + `USER/src/fw_core.c`，挂到按 I²C 位时序重建的 mock 从机（`test/host_gxht40_verify_ev.c`，上一轮本能力编写的独立 harness）。
3. **负对照（变异测试）**：把驱动复制到临时目录做两处定向变异，确认 harness 会失败——证明用例「有牙齿」而非空跑。
4. **相邻原语独立 harness**：`test/host_sf_i2c_verify_ev.c`（总线原语）、`test/host_fw_core_verify_ev.c`（CRC-8/换算穷举）。
5. **交叉编译当前交付件**：`gcc/build.sh` + T1 三个翻译单元的浮点符号核查。
6. **交付件与受测提交一致性**：`git status` / `git diff HEAD` 确认所编源码即 HEAD。

## 4. 结果明细（命令与原始结果）

### 4.1 独立参考 oracle（期望值独立复算）
```
$ python /tmp/ev003/gxht40_ref.py
manual reference CRC(0xBEEF)=0x92 (datasheet expected 0x92)
F_MANUAL S_T=0xBEEF S_RH=0x1234 T_CRC=0x92 H_CRC=0x37 -> temp=855  hum=29   validT=True
F_ROOM   S_T=0x6666 S_RH=0x72B0 T_CRC=0x93 H_CRC=0xDC -> temp=250  hum=500  validT=True
F_NEG    S_T=0x3333 S_RH=0x0000 T_CRC=0x88 H_CRC=0x81 -> temp=-100 hum=0    validT=True
F_CLAMP  S_T=0x6666 S_RH=0xFFFF T_CRC=0x93 H_CRC=0xAC -> temp=250  hum=1000 validT=True
F_HUM0   S_T=0x6666 S_RH=0x0000 T_CRC=0x93 H_CRC=0x81 -> temp=250  hum=0    validT=True
F_RLOW   S_T=0x0000 S_RH=0x72B0 T_CRC=0x81 H_CRC=0xDC -> temp=-450 hum=500  validT=False
F_RHIGH  S_T=0xF8CA S_RH=0x72B0 T_CRC=0x32 H_CRC=0xDC -> temp=1251 hum=500  validT=False
valid temp raw range: [0x073E..0xF8C2] -> [-400..1250]
  OK x11 ; REFERENCE RESULT: PASS ; exit=0
```
→ 手册参考向量 `CRC(0xBEEF)=0x92` 复现；harness 内所有期望帧的 CRC/换算全部与独立 oracle 一致；有效域端点为 raw `0x073E`(-40.0 ℃) 与 `0xF8C2`(125.0 ℃)，`raw=0`→-450 与 `0xFFFF`→1300 均无效。

### 4.2 独立驱动 harness（真实驱动 + 位级 mock 总线）
```
$ gcc -std=c11 -Wall -Wextra -Wno-int-to-pointer-cast -Itest/mock_mcu -IUSER/inc -ICOMMON \
      test/host_gxht40_verify_ev.c USER/src/gxht40.c USER/src/sf_i2c.c USER/src/fw_core.c \
      -o /tmp/ev003/gxht40_verify.exe && /tmp/ev003/gxht40_verify.exe
[T1] 0xBEEF/0x1234 -> OK, temp=855 hum=29; cmd=0xFD x1; wr 0x88; rd 0x89; cache=0x44; one 10 ms wait
[T2] 25.0C/50% -> 250/500 ; -10.0C/0% -> -100/0 (sign) ; hum raw 0xFFFF -> 1000 ; hum raw 0 -> 0
[T3] device only at 0x45: probe 0x88 then 0x8A, read 0x8B, cache=0x45; 2nd measure uses cache (no 0x88)
[T4] 2x read NACK -> success; command NOT re-sent; 3 read attempts; two 1 ms waits
[T5] NACK beyond bound -> GXHT40_ERR_IO; outputs untouched; attempts <= MEAS_RETRY / MEAS_RETRY*READ_RETRY
[T6] temp-word CRC bad -> GXHT40_ERR_CRC; hum-word CRC bad -> GXHT40_ERR_CRC; outputs untouched; re-measured == 3
[T7] raw 0 (-45.0C) -> GXHT40_ERR_RANGE ; 125.1C -> GXHT40_ERR_RANGE ; outputs untouched
[T8] no device -> GXHT40_ERR_NO_DEVICE (both 0x88/0x8A probed); outputs untouched; bounded
[T9] NULL temp / NULL hum / unbound bus -> GXHT40_ERR_PARAM
[T10] every command byte ever sent was 0xFD (23 total)
==== result: 42 passed, 0 failed ====   exit=0
```

### 4.3 负对照（变异测试，证明 harness 有效）
```
$ diff USER/src/gxht40.c /tmp/ev003/gxht40_mutA.c   # 湿度字 CRC 比较错字节 buf[5] -> buf[2]
125c125  < (fw_crc8_gxht(&buf[3], 2u) != buf[5])   > (fw_crc8_gxht(&buf[3], 2u) != buf[2])
$ /tmp/ev003/mutA.exe ; echo $?          -> ==== result: 27 passed, 15 failed ====   exit=1
$ diff USER/src/gxht40.c /tmp/ev003/gxht40_mutB.c   # 湿度原始字低字节 buf[4] -> buf[2]
131c131  < ... | (uint16_t)buf[4]);       > ... | (uint16_t)buf[2]);
$ /tmp/ev003/mutB.exe ; echo $?          -> ==== result: 39 passed, 3 failed ====    exit=1
```
→ 两处定向变异均被 harness 检出（非 0 退出、失败计数 >0），说明 42/42 不是空跑。

### 4.4 相邻原语独立 harness
```
$ gcc ... test/host_sf_i2c_verify_ev.c USER/src/sf_i2c.c -o sfi2c_verify.exe && ./sfi2c_verify.exe
  ... (命令写/连续读线上字节、START/STOP、主机 ACK/NACK 位置、超时释放、既有函数回归)
==== result: 35 passed, 0 failed ====   exit=0
$ gcc -DSENSOR_CONFIG_NO_MCU ... test/host_fw_core_verify_ev.c USER/src/fw_core.c -o fwcore_verify.exe && ./fwcore_verify.exe
  ... (65536 温度字/65536 湿度字/256x256 CRC/滞回/上报组合 与独立 Python 参考哈希比对)
==== result: 53 passed, 0 failed ====   exit=0
```

### 4.5 maker 自检 harness（复现，作为交叉确认，不作为独立证据）
```
$ ... test/host_gxht40_check.c ... -> ==== result: 27 passed, 0 failed ====   exit=0
$ sh test/build_test.sh -> [1/2] ==== result: 38 passed, 0 failed ====
                            [2/2] ==== result: 24 passed, 0 failed ====   exit=0
```

### 4.6 交叉编译当前交付件 + 浮点核查
```
$ sh gcc/build.sh
== link ==
Memory region         Used Size  Region Size  %age Used
           FLASH:       32208 B        64 KB     49.15%
             RAM:        1712 B         4 KB     41.80%
== done: obj/sensor_fw.elf/.hex/.bin ==      exit=0
$ arm-none-eabi-nm obj/gxht40.o obj/sf_i2c.o obj/fw_core.o | grep -iE "float|__aeabi_[fd]|soft"
(no output)  -> T1 三个翻译单元无浮点/soft-float 引用（换算为整数运算）
```

### 4.7 交付件与受测提交一致性
```
$ git status --short          -> (空)
$ git diff --stat HEAD -- .../USER/src/gxht40.c .../USER/inc/gxht40.h  -> (空)
```
→ 所编译/所测源码即受测提交 `cbb2b61` 的 HEAD 内容，未拿旧镜像或旧包充当当前交付件。

## 5. 覆盖矩阵与剩余缺口

| 验收行为 | 宿主机独立验证 | 真实目标验证 | 判定 |
|---|---|---|---|
| 地址探测 0x44/0x45 + 缓存 | 通过（含 0x45 顺序与缓存复用） | **未执行**（真实变体、上拉、ACK） | 部分 |
| 0xFD 高重复率命令（不发热/不复位） | 通过（白名单：全部发送命令均为 0xFD） | **未执行**（真实线上字节） | 部分 |
| tMEAS 等待 + 6 字节读取 | 通过（延时调用与字节顺序、ACK/NACK 位置） | **未执行**（真实时间差 ≥8.3 ms） | 部分 |
| 温度/湿度字 CRC-8 | 通过（独立 oracle + 注入坏 CRC） | **未执行** | 部分 |
| 温度 x10 有符号 + 量程 | 通过（端点 -400/1250、-450/1300 无效） | **未执行**（与参考温湿度计对照、负温实读） | 部分 |
| 湿度 x10 截断 0..1000 | 通过（0xFFFF→1000、0→0） | **未执行** | 部分 |
| 读 NACK / CRC / 无器件有界重试 | 通过（上限、次数、时序延时） | **未执行**（真实 NACK、故障注入恢复） | 部分 |
| 失败返回失败码且不改输出 | 通过（IO/CRC/RANGE/NO_DEVICE/PARAM 全分支） | **未执行** | 部分 |

## 6. 未执行的真实目标工作与阻塞原因

以下 TD-002 rev 2.0 用例对 T1 为强制项，本轮**均未执行**：

- T-L2-01 真实 I²C 事务抓包（地址字节、0xFD、6 字节、ACK 位置、空闲上拉）
- T-L2-02 tMEAS 真实时间差（≥8.3 ms，取 10 ms）
- T-L2-03 连续读时序与主机 ACK/NACK 位置的真实波形
- T-L2-04 与参考温湿度计的实测值一致性（≥3 温度点 / 2 湿度点）
- T-L2-05 负温端到端符号
- T-L2-06/07/08 真实故障注入（CRC 错、总线拉低卡死、无器件）
- T-L2-09 0x44/0x45 真实器件变体确认
- T-L1-09/T-L1-12 当前交付件烧录与回读一致性

**阻塞原因（环境不可用）**：项目执行环境声明「设备工具已禁用」；本能力实际可用工具仅 read/bash/write 类，无 programmer/jlink/serial/logic_analyzer/oscilloscope/power_cycle，环境内也没有 SEGGER J-Link 或逻辑分析仪 CLI，且无传感器板/网关板/可控温源/参考温湿度计/433 接收端。故以上必需的真实目标工作无法进行。

## 7. 结论

- **未发现实现缺陷**：宿主机独立验证（真实驱动 + 位级 mock 总线）、独立 oracle 复算与变异负对照全部通过，覆盖任务文字的每一个软件可判分支与边界；`gcc/build.sh` 对当前交付件交叉编译 0 错误、资源在预算内、T1 翻译单元无浮点。
- **但本项不能判 TEST_PASS**：TD-002 §2.2 对 T1 强制要求的真实目标观测（实板 I²C、真实器件 tMEAS/NACK、参考仪器对照、烧录回读一致性）在设备工具禁用下未执行；按能力约定这属于「required real-target work not performed」，必须返回阻塞类结果，且不得用 mock 成功或「后续再补」的说明替代。
- **也不判 TEST_FAIL**：阻塞不是实现缺陷；把未执行记成失败会误导后续修复方向。
- 因此本轮结果：**BLOCKED**（环境不可用，待设备工具解禁后按 TD-002 §4.4/§4.3 补做真实目标用例，并重跑本节宿主机证据）。

## 8. 交接

1. **设备解禁后**：先做 T-L1-09/T-L1-12（烧录当前提交编出的镜像并回读比对），再做 T-L2-01/02/03（I²C 抓包与 tMEAS），随后 T-L2-04/05（参考仪器）与 T-L2-06/07/08/09（故障注入、地址变体）。宿主机证据可用本轮命令原样复跑。
2. **观测钩子**：`SENSOR_TEST_TRACE` 默认关闭；实板做 T-L2-04 前建议按 TD-002 §6.2 打开以获得内部温湿度/重试计数（功耗用例必须关闭态）。
3. **不属本能力**：本项未发现需要固件实现修改的问题，故不产生实现侧改单。

---

# 历史记录（上一轮工作流 EV-002，旧 12 项编号；仅供追溯，勿与本轮 ITEM-001..007 混用）

# 嵌入式验证证据（EV-002 / ITEM-001）

状态：embedded_tester.embedded_verification 独立验证证据
本轮验证对象：任务项 **ITEM-001**（新增 `sensor_config.h` 作为唯一配置点；加入后工程仍可交叉编译）
被测提交：`ab5c5b2`（firmware_engineer.firmware_implementation）
验证基线：`9e90eb5`（本次实现之前，已含 TD-002 测试设计）
结论：**TEST_PASS**（ITEM-001 的声明行为全部独立复现；另发现 1 项与本项无关的既有 Keil 工具链环境问题，见 §6）

上游输入：`artifacts/firmware_implementation.md`（FWI-002 声明）、`artifacts/test_design.md`（TD-002）、`artifacts/firmware_design.md`（FD-002）、IC-002、任务清单 ITEM-001。

---

## 1. 验证范围与选测说明（为什么选这些）

| 选测项 | 理由 |
|---|---|
| 交付物存在性与常量值（TD-002 T-L1-05/T-L1-08 的配置点部分） | ITEM-001 的唯一验收物就是该头文件与其常量 |
| 头文件自包含 / 编译期断言 | 证明常量可被后续 TU 直接使用且取值正确，不靠人工目测 |
| GNU 交叉编译 + 与基线逐字节比对（TD-002 T-L1-02） | 直接检验「加入该头文件后工程仍可交叉编译」 |
| 生产编译器 ARM Compiler 5（Keil AC5）+ 工程 include 路径 | 交叉编译的真实量产工具链是 Keil MDK / IAR，需证明头文件对 AC5 与两个工程都可用 |
| 宿主机既有 L0 回归（TD-002 T-L0-06 的回归部分） | 本次改动不涉及被判定的既有纯逻辑，需确认无回归 |
| 变更范围审查（git） | 确认未夹带无关改动，且 .c 未被改动（支持"无行为变化"结论） |

**不适用（本轮未实现，按能力约定不要求、也不作为本项通过条件）**：TD-002 中 T-L0-01…T-L0-05（CRC-8/换算/光照/上报判定纯逻辑）、T-L0i（空口互操作）、T-L2/T-L3/T-L4/T-L5/T-L6/T-L7/T-L8（I²C、光照、节拍、条件上报、失败路径、端到端、功耗）——这些行为集中在后续 ITEM-002…ITEM-012，本轮不存在可测对象。仅对既有保留逻辑做回归。

## 2. 环境与工具（本轮实际调用）

| 工具 | 版本/路径 | 用途 |
|---|---|---|
| `arm-none-eabi-gcc` | 10.3.1 20210824（Cortex-M0+） | 工程交叉编译、编译期断言 TU |
| ARM Compiler 5 | `C:/Keil_v5/ARM/ARM_Compiler_5.06u7/Bin/armcc.exe` | 量产编译器单 TU 兼容性验证 |
| MinGW-w64 gcc | 12.2.0（`C:/ProgramData/chocolatey/.../mingw64/bin/gcc.exe`） | 宿主机 L0 回归 |
| `mdk_build`（Runtime 工具） | 配置的 Keil 工程 | 尝试真实 Keil 工程构建（结果见 §6） |
| 逻辑分析仪/示波器/J-Link/串口/烧录 | — | **本轮未使用**：ITEM-001 只新增宏定义，无可观测的板上行为；无需也不可能用仪器验证 |

## 3. 检查明细与结果

### 3.1 交付物存在与内容（通过）

`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/inc/sensor_config.h` 存在，135 行，含 6 组配置。独立核对：

| 要求组（ITEM-001 原文） | 实际定义 | 判定 |
|---|---|---|
| PB04/PB05 引脚 | `LIGHT_POWER_PORT=CW_GPIOB`、`LIGHT_POWER_PIN=GPIO_PIN_5`（PB05）；`LIGHT_ADC_PORT=CW_GPIOB`、`LIGHT_ADC_PIN=GPIO_PIN_4`（PB04） | PASS |
| ADC 通道 | `LIGHT_ADC_INPUT_CHANNEL=ADC_InputCH11`（=11）；`LIGHT_ADC_FULL_SCALE=4095`；`LIGHT_ADC_CLK_DIV=ADC_Clk_Div8`；`LIGHT_ADC_SAMPLE_TIME=ADC_SampTime390Clk`；`LIGHT_ADC_SAMPLES=8` | PASS |
| GXHT40 地址/命令/重试/等待 | 0x44/0x45；写读字节 0x88/0x89/0x8A/0x8B；`0xFD`（高重复率，另存备用 0xF6/0xE0 与软复位 0x94 并注明周期路径禁用）；`GXHT40_RESULT_LEN=6`；`GXHT40_MEASURE_WAIT_MS=10`；`GXHT40_READ_RETRY=5`+`1 ms`；`GXHT40_MEAS_RETRY=3`；`GXHT40_POWER_ON_WAIT_MS=2`；CRC8 poly 0x31/init 0xFF；换算常量 `-450/1750/-60/1250/65536/32768` 与量程 `-400..1250`、`0..1000` | PASS |
| 光照明暗阈值与滞回 | `LIGHT_DARK_ENTER=350`、`LIGHT_DARK_EXIT=250`、`LIGHT_DARK_HYSTERESIS_STEP=100`；`LIGHT_DARK_CALIBRATED=0`（明确标注"未标定，须实板标定后回填"） | PASS（并未把未标定默认值谎称已验收） |
| 3 分钟采样计数值 | `SENSOR_RTC_TICK_PERIOD_MIN=1`、`SENSOR_SAMPLE_TICKS=3` | PASS |
| 0.9 ℃ 下降 / 35.0 ℃ 超温上报常量 | `SENSOR_REPORT_DROP_X10=9`、`SENSOR_REPORT_HIGH_X10=350`，注释写明严格大于语义 | PASS |

附带核对：`SENSOR_RF_FRAME_LEN=10`、`SENSOR_RF_TX_TIMEOUT_MS=200`、`SENSOR_RF_TX_RETRY=3`、调试钩子 `SENSOR_TEST_TRACE` 默认 0。

**独立交叉核对（来源可信度）**：
- PB04 = AIN11 = `ADC_InputCH11`：由厂商库头 `CW32L010_StandardPeripheralLib_V1.0.5/Libraries/inc/cw32l010_adc.h` 的注释与宏独立确认（`ADC_InputCH11 = 0x0B`，注释「通道11输入PB04」），非仅引用设计文档。
- PB04 模拟功能宏 `PB04_ANALOG_ENABLE()` 存在于 `cw32l010_gpio.h`。

### 3.2 头文件自包含 + 编译期断言（通过）

自写独立验证 TU（`sensor_config.h` 为唯一项目头包含，另仅 `_Static_assert`），43 条断言覆盖上表全部常量与关系（`EXIT<ENTER`、`ENTER-EXIT==STEP`、`MEASURE_WAIT_MS>=9`、地址派生、通道=11 等）：

```
arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -Wall -Wextra -std=c11 \
  -I USER/inc -I COMMON -I UM2005C -I ../../../Libraries/inc \
  -I <CMSIS 5.9.0 Core Include> -fsyntax-only <verify_tu.c>
=> ASSERT_TU_OK: all 43 static assertions compiled cleanly（0 警告，0 错误）
```

结论：头文件自包含（仅 `<stdint.h>`/`<stdbool.h>` + CW32 库头），所有断言值正确。

### 3.3 唯一配置点（通过，局部）

- 在目标工程范围内（`USER/`、`UM2005C/`、`COMMON/`、`test/`、`gcc/`、`MDK/`、`EWARM/`）检索 `GXHT40_*`/`LIGHT_*`/`SENSOR_RTC_*`/`SENSOR_SAMPLE_*`/`SENSOR_REPORT_*`/`SENSOR_RF_*`/`SENSOR_TEST_*` 的 `#define`：**除 `sensor_config.h` 外为 0 处**，无重复定义/冲突。
- 实际被构建使用（非死文件）：预处理确认 `USER/src/main.c` 与 `USER/src/interrupts_cw32l010.c` 经 `main.h` 包含该头文件；`measure.c` 目前未包含（本次任务未要求，后续 ITEM 接入）。
- 说明：`SENSOR_*` 同名前缀在仓库其它无关示例（`Examples/sensoraiwrite/...`）与 CH592 官方库中存在，但不在本工程构建范围内，不构成冲突。

### 3.4 GNU 交叉编译 + 与基线逐字节比对（通过，核心）

命令：`sh gcc/build.sh`（工程自带脚本，Cortex-M0+，`-O1 -Wall -Wextra`，全工程 TU）。

| 项 | HEAD（ab5c5b2） | 基线（9e90eb5，独立 git worktree 构建） |
|---|---|---|
| 编译/链接 | 0 error | 0 error |
| warning 数 | 29（全部来自既有代码/厂商库） | （同源代码，未变） |
| sensor_config.h 相关告警 | **0** | 不适用 |
| FLASH | 33,896 B（51.72%） | 33,896 B（51.72%） |
| RAM used | 1,960 B（47.85%） | 1,960 B（47.85%） |
| `arm-none-eabi-size` text/data/bss | 33896 / 84 / 1876 | 33896 / 84 / 1876 |
| `sensor_fw.bin` MD5 | `46727d6446edb4adb0c87d26c7933f01` | `46727d6446edb4adb0c87d26c7933f01` |
| `sensor_fw.hex` MD5 | `0da7d963b50d16c2e1bb4fc5d80ce9da` | `0da7d963b50d16c2e1bb4fc5d80ce9da` |

基线为在临时 worktree（`git worktree add --detach <tmp> 9e90eb5`）中独立构建，未复用实现者产物。**固件二进制逐字节相同**，独立证明「加入该头文件后工程可交叉编译且不产生任何代码/数据变化」。

### 3.5 生产编译器与工程 include 路径（通过）

- Keil MDK 工程 include 路径含 `..\USER\inc`（`MDK/Project.uvprojx: <IncludePath>...;..\USER\inc;..\UM2005C;..\COMMON`）。
- IAR 工程 include 路径含 `$PROJ_DIR$\..\USER\inc`（`EWARM/*.ewp`）。
- 用 ARM Compiler 5.06u7 + 工程固定的 CMSIS 5.9.0 单独编译实际解析该头文件的 TU：

```
armcc --cpu=Cortex-M0+ --c99 -I ../../../Libraries/inc -I USER/inc -I UM2005C -I COMMON \
      -I ../../IdeSupport/MDK -I <CMSIS 5.9.0 Core Include> -c <file>
USER/src/main.c                 : 0 error（经 main.h 包含 sensor_config.h）
USER/src/interrupts_cw32l010.c  : 0 error（经 main.h 包含 sensor_config.h）
USER/src/sf_i2c.c               : 0 error
USER/src/measure.c              : 0 error, 1 warning（既有代码）
```

结论：新头文件对量产 ARM Compiler 5 兼容，未破坏既有 TU 编译。

### 3.6 宿主机 L0 既有回归（通过）

```
CC=<mingw64 gcc 12.2.0> sh test/build_test.sh
==== result: 56 passed, 0 failed ====
```

本项未改动 `fw_core.c`/`history.c`/`host_sensor_core_test.c`，56/56 与基线一致。

### 3.7 变更范围审查（通过）

`git show --stat ab5c5b2`：仅 5 个文件 —— `USER/inc/sensor_config.h`（新增）、`USER/inc/main.h`（+1 行 include，位于既有 include 段）、`artifacts/firmware_implementation.md`、`evidence/build.md`、`file_manifest.txt`。**无任何 .c/.ld/工程文件被修改**，无夹带无关改动，与"仅新增配置头"的声明一致。

## 4. 反证与反向检查

| 检查 | 结果 |
|---|---|
| 是否存在与声明不符的"假验证"（例如头文件未被任何 TU 使用） | 否：`main.c`/`interrupts_cw32l010.c` 预处理确认包含 |
| 是否把未标定阈值当作已验收 | 否：`LIGHT_DARK_CALIBRATED=0` 且注释明确要求实板标定后回填（FWR-OPEN-1） |
| 是否引入新的编译告警 | 否：0 条告警指向该头文件；交叉编译告警均来自既有代码/厂商库 |
| 是否存在同值常量重复定义（破坏"唯一配置点"） | 本工程范围内为 0 |
| 二进制是否真的无变化 | 是：与基线 `.bin`/`.hex` MD5 完全相同 |

## 5. 覆盖边界声明（本轮未验证的内容）

- 未在真实传感器板上做任何 I²C/光照/节拍/上报/功耗验证（无非硬件可测对象，且本项不涉及行为）。
- 未烧录、未使用串口/逻辑分析仪/示波器；`mdk_flash` 未调用（无需下载，且本项无板上行为可观测）。
- Keil 完整工程构建未成功（见 §6），因此"MDK 工程端到端编译通过"**未**被证明；已用 GNU 交叉编译 + AC5 单 TU 编译替代覆盖，并如实标注。
- 光照阈值、GXHT40 驱动、条件上报判定等属后续 ITEM，本证据不作为其通过依据。

## 6. 发现的非本项问题（环境/工具链，登记为交接，不计入本项缺陷）

`mdk_build`（action=build）在**当前环境**下失败，日志摘录：

```
*** Using Compiler 'V5.06 update 7 (build 960)'
compiling system_cw32l010.c...
C:\Users\kason\AppData\Local\Arm\Packs\ARM\CMSIS\6.3.0\CMSIS\Core\Include\cmsis_compiler.h(279): error: #35: #error directive: Unknown compiler.
".\output\exe\Project.axf" - 13 Error(s) ...
```

定位（独立复现，证明与 ITEM-001 无关）：

| 复现 | 结果 |
|---|---|
| 未改动的厂商文件 `Libraries/src/system_cw32l010.c` + CMSIS **6.3.0** 头 | 同样 `Unknown compiler` 错误 |
| 同一文件 + 工程记录的固定包 CMSIS **5.9.0** 头 | 0 错误，产出 .o |
| 本工程 `git show HEAD:MDK/output/exe/Project.build_log.htm`（导入基线，早于本次改动） | 上次成功构建使用 `ARM.CMSIS.5.9.0`，`0 Error(s)`；本次 UV4 解析到了 6.3.0 |

即：Keil RTE/pack 解析在本机选到 CMSIS 6.3.0，而 AC5 不被 CMSIS ≥6 支持，导致**所有** MDK 构建失败，与 `sensor_config.h` 无关。
处置建议（交接，非本能力执行）：在 Keil 工程中固定 CMSIS 5.9.0 包，或改用 ARM Compiler 6（AC6）并同步工程选项；修复前 `mdk_flash`/`mdk_build` 不能作为量产构建证据。

**仓库卫生**：`mdk_build` 生成的构建产物（`MDK/output/exe/*` 被跟踪文件）与 `MDK/build.log`、`test/host_sensor_core_test.exe` 已在验证后清理/还原，验证结束时 `git status` 为空。

## 7. 结论

ITEM-001 的声明行为经独立复现全部成立：配置头存在且取值正确、自包含、对 AC5 与 GNU 交叉编译均可用、为工程实际包含、无重复配置点，且固件二进制与改动前**逐字节一致**。既有 L0 回归无退化。

**判定：TEST_PASS**。残留项为 §6 的既有 Keil/CMSIS 工具链环境问题（已登记交接，不影响本项判定）与后续 ITEM 的行为验证（不在本项范围）。

---

# ITEM-002 验证（`sf_i2c` 无寄存器地址总线原语）

验证对象：任务项 **ITEM-002**（命令写 + 连续读原语，ACK 失败可判断；既有 i2c 函数语义与调用方式不变）
被测提交：`306c63d`；源码基线：`5a6210a`
结论：**TEST_PASS**

## A. 选测说明

| 选测项 | 理由 |
|---|---|
| 源码 diff 纯增量审查 | 直接检验「既有 i2c 函数语义与调用方式不变」 |
| 独立 mock 总线协议测试（自研，不复用实现者测试） | 检验「命令写 / 连续读」的线上字节、START/STOP 边界、主机 ACK/NACK 位置与返回值 |
| 既有函数机器码逐指令比对（改动前后两个 elf） | 比 diff 更强的回归证据：证明既有函数编译结果未被改变 |
| GNU 交叉编译 + 符号/体积差分 | 可编译、增量只来自新增函数、无既有代码膨胀 |
| ARM Compiler 5（AC5）单 TU 编译 | 量产工具链兼容性（sf_i2c.c 本次被修改） |
| 宿主机既有 L0 回归 | 无附带回归 |

不适用（后续 ITEM 才存在可测对象）：GXHT40 驱动、光照、节拍、条件上报、端到端与功耗用例。真实电气/时序（上升沿、上拉、时钟频率）、GXHT40 tMEAS 等待与器件地址变体属 TD-002 T-L2-01/02/03/07，须在真实传感器板上用逻辑分析仪完成，本轮**不具备**该条件，不在此判定。

## B. 源码变更范围（通过）

`git show --numstat 306c63d`：`USER/src/sf_i2c.c` **62 insertions / 0 deletions**；`USER/inc/sf_i2c.h` **8 / 0**。即对既有函数无任何行修改，仅在文件尾部新增 `i2c_write_cmd` / `i2c_read_bytes` 及声明。

## C. 独立 mock 总线协议测试（通过，核心）

自研位级 mock 总线 + mock 从机，直接链接**未打桩的** `USER/src/sf_i2c.c`（与实现者的 `test/host_sf_i2c_bus_check.c` 相互独立）。harness 入库于 `.../gpio_input_output/test/host_sf_i2c_verify_ev.c`。

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
<mingw64 gcc> -std=c11 -Wall -Wextra -I USER/inc test/host_sf_i2c_verify_ev.c \
              USER/src/sf_i2c.c -o <tmp>/i2c_verify.exe && <tmp>/i2c_verify.exe
=> ==== result: 35 passed, 0 failed ====   (编译 0 警告)
```

原始线上转录（本 harness 输出）：

```
T1  i2c_write_cmd(0x88,0xFD) 有从机 : START TX(88) ACK TX(FD) ACK STOP
T2  i2c_write_cmd 无从机应答       : START TX(88) NACK STOP            -> SF_I2C_TIMEOUT
T6b 地址 ACK、命令字节 NACK         : START TX(88) ACK TX(FD) NACK STOP STOP -> SF_I2C_TIMEOUT
T3  i2c_read_bytes(0x89,6)         : START TX(89) ACK RX(61) M-ACK RX(9C) M-ACK RX(2B) M-ACK RX(8A) M-ACK RX(44) M-ACK RX(75) M-NACK STOP
T4  i2c_read_bytes 无从机应答       : START TX(89) NACK STOP            -> SF_I2C_TIMEOUT
T6  i2c_read_bytes 传写形式地址 0x88: START TX(89) ACK RX(11) M-NACK STOP
T7  回归 i2c_write_multi_byte       : START TX(70) ACK TX(AC) ACK TX(33) ACK TX(00) ACK STOP
T8  回归 i2c_read_multi_byte        : START TX(70) ACK TX(71) ACK START TX(71) ACK RX(A0) M-ACK ... RX(A4) M-NACK STOP
```

| 断言组 | 结果 |
|---|---|
| 命令写：1×START/1×STOP、线上 `0x88`→`0xFD`（无寄存器地址）、两字节均 ACK、从机仅收到命令字节、无读字节、结束后总线空闲高 | PASS |
| 命令写失败：地址 NACK → `SF_I2C_TIMEOUT`、已发 STOP、**命令字节不再发送**（快速失败）、总线释放 | PASS |
| 命令写失败：命令字节 NACK → `SF_I2C_TIMEOUT`、总线释放 | PASS |
| 连续读：1×START/1×STOP、首字节 `0x89`、6 字节与从机数据逐字节一致、**前 5 字节主机 ACK、末字节 NACK**、总线释放 | PASS |
| 连续读失败：读地址 NACK → `SF_I2C_TIMEOUT`、无数据字节、已发 STOP（不卡总线） | PASS |
| `length==0`：返回 SUCCESS 且**无任何总线动作** | PASS |
| 地址形式：`I2C_READ()` 掩码使写形式 0x88 与读形式 0x89 均得 0x89；与 `sensor_config.h` 的 `GXHT40_ADDR_READ_A=0x89` 兼容 | PASS |
| 回归：既有 `i2c_write_multi_byte` / `i2c_read_multi_byte`（含重复 START）字节序列与末字节 NACK 不变 | PASS |

实现者自带 harness 也被独立执行确认：`gcc ... test/host_sf_i2c_bus_check.c USER/src/sf_i2c.c` → **15 passed, 0 failed**（不以其自报为唯一依据）。

## D. 既有函数机器码等价（通过，最强回归证据）

在临时 worktree 中独立构建基线 `5a6210a`，与 HEAD 构建产物逐函数比对（`arm-none-eabi-objdump -d --disassemble=<fn>`，归一化重定位地址后逐指令比较）：

| 既有函数 | 结果 |
|---|---|
| `i2c_init` / `i2c_start` / `i2c_stop` / `i2c_write_byte` / `i2c_read_byte` | 指令完全相同 |
| `i2c_write_multi_byte` / `i2c_read_multi_byte` | 指令完全相同 |
| `i2c_write_multi_byte_16bit` / `i2c_read_multi_byte_16bit` / `i2c_obj_find` | 指令完全相同（仅重定位目标地址随链接位置变化） |

`arm-none-eabi-nm --print-size` 对照：上述函数**大小逐一相等**；新增符号只有 `i2c_write_cmd`（0x32=50 B）与 `i2c_read_bytes`（0x72=114 B）。

## E. 构建与体积（通过）

| 项 | 基线 5a6210a | HEAD 306c63d |
|---|---|---|
| 交叉编译 | 0 error / 29 warning | 0 error / 29 warning（无新增） |
| FLASH | 33,896 B | 34,060 B（**+164 B = 50+114，恰为两个新函数**） |
| RAM | 1,960 B | 1,960 B（不变） |
| `sf_i2c.c` 相关告警 | 2 条既有 `err may be used uninitialized`（`i2c_write_multi_byte`/`_16bit`，line 295/417，非本次引入） | 同左，行号与数量不变 |

AC5（`armcc --cpu=Cortex-M0+ --c99`，工程固定 CMSIS 5.9.0）单 TU 编译：`sf_i2c.c` 0 错误、`main.c` 0 错误、`measure.c` 0 错误（1 既有告警）。宿主机 L0 回归 `test/build_test.sh` → **56 passed, 0 failed**。

## F. 观察项（非缺陷，记录备查）

1. **NACK 失败路径会出现两个 STOP**（`i2c_wait_ack` 超时内已发一次，调用方再发一次；T6b 转录 `... NACK STOP STOP`）。I²C 总线上空闲态重复 STOP 无副作用，且与既有 `i2c_write_multi_byte` 的写法一致；本项不判缺陷，后续如需精简可随 GXHT40 驱动一并评估。
2. **`length==0` 短路返回 `SF_I2C_SUCCESS` 且不发总线**：任务未定义该边界，实现选择明确且可测；记录为行为约定。
3. 调用方需注意 `i2c_read_bytes` 的地址参数可用写/读形式任一种（内部掩码），与 `sensor_config.h` 的 `0x88/0x89` 常量兼容——后续 ITEM-003 接入时不应出现地址形式错用。

## G. ITEM-002 判定

「命令写（START→地址+W→命令→STOP）」「连续读（START→地址+R→N 字节→末字节 NACK→STOP）」「从机 ACK 失败作为可判断返回值」「既有 i2c 函数语义与调用方式不变」四项声明均经独立复现成立，且既有函数机器码逐指令未变。

**判定：TEST_PASS**（真实板电气/时序与 GXHT40 器件交互不在本项范围，交接 TD-002 T-L2-01/02/03/07）。

---

# ITEM-003 验证（GXHT40 驱动 `gxht40.c/.h`）

验证对象：任务项 **ITEM-003**（地址探测 0x44/0x45、`0xFD` 高重复率测量、tMEAS 等待、读 6 字节、温度/湿度字分别 CRC-8 校验、x10 换算与截断、失败返回失败码且不修改输出）
被测提交：`3bbd292`；源码基线：`1712162`
结论：**TEST_PASS**

## A. 选测说明与变更范围

| 选测项 | 理由 |
|---|---|
| 新增文件 diff 审查 | `git show --numstat 3bbd292`：`gxht40.h` 48/0、`gxht40.c` 183/0、`test/host_gxht40_check.c` 308/0，**均为纯新增，无既有源码修改** |
| 独立参考帧生成（Python 独立实现 CRC/换算） | 不让被测驱动的 CRC/换算参与构造期望值，避免同源验证 |
| 自研 mock I²C + mock GXHT40 器件（链接真实 `gxht40.c`/`sf_i2c.c`） | 独立复现地址探测/命令/帧顺序/双字 CRC/重试/失败语义 |
| 失败不改输出（逐失败码） | 任务硬性要求，需直接断言 |
| GNU 交叉编译 + AC5 单 TU 编译 | 可编译性与量产编译器兼容（gxht40.c 尚未加入 Keil/IAR 源列表，属 ITEM-011） |

不适用（后续 ITEM 才有可测对象）：光照通路、采样节拍、条件上报判定、端到端与功耗。**真实 tMEAS 等待下界、地址变体实物确认、电气与真实器件 CRC 实读**属 TD-002 T-L2-01/02/03/06/07，本轮无逻辑分析仪/实物板，不在此判定。

## B. 独立参考帧与 CRC 变体确认（通过）

用独立 Python 实现（poly 0x31、init 0xFF、MSB-first、无反转、xorout 0x00）生成期望值：

```
CRC-8({0xBE,0xEF}) = 0x92   （与 gxht40.pdf / FD-002 §6.1 参考向量一致）
CRC-8("123456789") = 0xF7   （即 CRC-8/NRSC-5 标准校验值，确认变体无歧义）
换算：raw_t=0x0000 -> -450（超范围）; 0xFFFF -> 1300（超范围）; raw_h=0 -> 0; 0xFFFF -> 1190 -> 截断 1000
```

用于 mock 器件的 6 字节帧（**由上述独立脚本生成后硬编码，harness 不调用驱动的 CRC/换算**）：

| 向量 | 6 字节 | 期望结果 |
|---|---|---|
| 手册参考 0xBEEF/0x1234 | `BE EF 92 12 34 37` | OK，temp=855，hum=29 |
| 常温 25.0 ℃/50 %RH | `66 66 93 72 B0 DC` | OK，250 / 500 |
| 负温 −10.0 ℃/0 % | `33 33 88 00 00 81` | OK，−100 / 0 |
| 湿度越界（raw 0xFFFF） | `66 66 93 FF FF AC` | OK，hum 截断 1000 |
| 湿度 raw 0 | `66 66 93 00 00 81` | OK，hum 0 |
| 量程低（raw_t 0） | `00 00 81 72 B0 DC` | `GXHT40_ERR_RANGE`（换算 −450） |
| 量程高 125.1 ℃ | `F8 CA 32 72 B0 DC` | `GXHT40_ERR_RANGE`（换算 1251） |
| 温度字 CRC 错 | `66 66 00 72 B0 DC` | `GXHT40_ERR_CRC` |
| 湿度字 CRC 错 | `66 66 93 72 B0 00` | `GXHT40_ERR_CRC` |

字节序独立验证：若温度字字节序颠倒，0xEFBE 会得到 1189 而非 855；若湿度字颠倒，0x3412 会得到 194 而非 29——实际得到 855/29，证明 `T_MSB,T_LSB,T_CRC,RH_MSB,RH_LSB,RH_CRC` 顺序正确。

## C. 独立 mock 器件功能测试（通过，核心）

harness 入库于 `.../gpio_input_output/test/host_gxht40_verify_ev.c`（链接**未打桩的** `gxht40.c` + `sf_i2c.c`；`delay_ms` 为计数测试桩以断言 tMEAS 等待）。

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
<mingw64 gcc> -std=c11 -Wall -Wextra -Wno-int-to-pointer-cast \
  -I../USER/inc -I../COMMON -I../../../../Libraries/inc -I<CMSIS 5.9.0 Core Include> \
  host_gxht40_verify_ev.c ../USER/src/gxht40.c ../USER/src/sf_i2c.c -o gxht40_verify && ./gxht40_verify
=> ==== result: 42 passed, 0 failed ====   (0 编译告警)
```

| 组 | 独立断言 | 结果 |
|---|---|---|
| 手册向量与帧封装 | OK；855/29；命令恰为 0xFD 一次；写地址 `0x88`、读地址 `0x89`；探测地址缓存 0x44；每次测量一次 10 ms tMEAS 等待 | PASS |
| 换算 | 25.0 ℃/50 %→250/500；−10.0 ℃/0 %→−100/0（符号保留）；hum raw 0xFFFF→1000；raw 0→0 | PASS |
| 地址探测与缓存 | 器件仅在 0x45：先 `0x88`（无应答）再 `0x8A`（成功）、读 `0x8B`；缓存 0x45；第二次测量直接用 `0x8A`，**不再探测 0x88** | PASS |
| 读 NACK 重试 | 前 2 次读地址 NACK、第 3 次成功；**命令未重发**（转换中）；三次读尝试；两次 1 ms 重试等待 | PASS |
| 读 NACK 用尽 | `GXHT40_ERR_IO`；输出不变；命令尝试 ≤ `GXHT40_MEAS_RETRY`；读尝试 ≤ `MEAS_RETRY×READ_RETRY` | PASS |
| CRC 错（温度字/湿度字） | 均 `GXHT40_ERR_CRC`；输出不变；**恰好重测 `GXHT40_MEAS_RETRY`(=3) 次** | PASS |
| 超范围（低/高） | 均 `GXHT40_ERR_RANGE`；输出不变 | PASS |
| 无器件 | `GXHT40_ERR_NO_DEVICE`；`0x88` 与 `0x8A` 均被探测；输出不变；探测次数有界 | PASS |
| 入参 | NULL 温度指针 / NULL 湿度指针 / 未绑定总线 均 `GXHT40_ERR_PARAM` | PASS |
| 命令白名单 | 全程共 23 次已发命令**全部为 `0xFD`**（无 `0x94` 软复位、无 `0x39` 加热器、无 `0xE0/0xF6` 低/中重复率） | PASS |

实现者自带 harness 也被独立执行确认：`host_gxht40_check.c` → **27 passed, 0 failed**（不以其自报为唯一依据）。

## D. 失败不修改输出（逐失败码，通过）

每类失败均先写入哨兵值（`temp=0x1111`、`hum=0x2222`，或 `0x7F7F`）再调用：`ERR_IO`、`ERR_CRC`（温度字/湿度字）、`ERR_RANGE`（低/高）、`ERR_NO_DEVICE`、`ERR_PARAM` 全部保持哨兵值不变。代码层核对：输出指针仅在完全成功分支末尾写入。

## E. 构建与对现网固件的影响（通过）

| 项 | 结果 |
|---|---|
| GNU 交叉编译（`gcc/build.sh`，含 `USER/src/gxht40.c`） | 0 error；29 warning（与基线同数，**无一条指向 `gxht40.c`**） |
| `gxht40.o` | text 460 B / data 0 / bss 5；符号 `gxht40_init`、`gxht40_measure`、`gxht40_detected_addr7`（`gxht40_crc8`、`gxht40_start_and_read` 为 static） |
| 固件镜像 | FLASH 34,060 B、RAM 1,960 B；`sensor_fw.bin` MD5 **与 ITEM-002 构建完全相同**（`67917abd…`）——模块尚未被调用，链接器 `--gc-sections` 丢弃，对现网固件零影响 |
| AC5（`armcc --cpu=Cortex-M0+ --c99`，CMSIS 5.9.0） | `gxht40.c` 0 error；`main.c` 0 error |
| 宿主机 L0 回归 | `test/build_test.sh` → 56 passed, 0 failed（未涉及，回归确认） |

## F. 观察项与交接

1. **CRC/换算实现会重复（交 ITEM-005）**：`gxht40.c` 内 `gxht40_crc8` / `gxht40_conv_*` 为 static；而 ITEM-005 要求在 `fw_core.c` 提供可宿主机直测的 `fw_crc8_gxht` / `gxht40_raw_to_x10` 等纯函数。若两份实现并存且不互调，存在后续漂移风险。建议 ITEM-005 完成后让 `gxht40.c` 复用 `fw_core.c` 的纯函数（或至少由 ITEM-005 的宿主测试锁定两者一致性）。**本项不因此判失败**（当前单份实现自洽且经独立验证）。
2. **Keil/IAR 源文件列表尚未包含 `gxht40.c`**（`Project.uvprojx`/`project.ewp` 中 `gxht40` 匹配数为 0）。这属 ITEM-011（“MDK/IAR 工程源文件列表与新增/移除源文件保持一致”）的范围，故在 ITEM-003 不判缺陷；但意味着**当前 Keil/IAR 构建不会编译该驱动**，ITEM-011 必须补上。
3. **无器件时的探测次数**：缓存失效后每次外层尝试会依次试 0x44/0x45，最坏 3×2=6 次命令（每次 10 ms 等待）≈ 60 ms 额外活动；有界且仅发生在器件缺失/故障场景，记录备查。
4. **真实 tMEAS 等待与地址变体**未在实物验证（无板/无逻辑分析仪）：软件侧已确认“命令→等待 10 ms→读”顺序与“读 NACK 不重发命令”，但实测下界（≥8.3 ms）与 0x44/0x45 实物确认仍需 TD-002 T-L2-01/02/03。

## G. ITEM-003 判定

地址探测（0x44/0x45，8 位 0x88/0x8A 写、0x89/0x8B 读）、`0xFD` 高重复率测量与 tMEAS 等待、6 字节读取与温度/湿度字分别 CRC-8（poly 0x31/init 0xFF）校验、x10 整数换算（含负温、0..1000 截断、−40.0..125.0 ℃ 有效域）、有界重试、失败返回失败码且不修改输出——全部经**独立于被测实现的参考帧与 mock 器件**复现成立。

**判定：TEST_PASS**。

---

# ITEM-004 验证（光照通路 `light.c/.h`）

验证对象：任务项 **ITEM-004**（采样时 PB05 输出高 → 稳定延时 → PB04/AIN11 多次取样求均值 → 阈值/滞回输出 DARK/LIT（无光 = 读数 ≥ 进入阈值）→ 采样结束 PB05 置低）
被测提交：`8e91e08`；源码基线：`d91f89c`
结论：**TEST_PASS**（带一项已登记的后续 ITEM 依赖，见 F.1）

## A. 选测说明与变更范围

| 选测项 | 理由 |
|---|---|
| 新增/修改 diff 审查 | `light.h` 44/0、`light.c` 136/0、`test/host_light_check.c` 74/0 新增；`sensor_config.h` **+2/0**（仅新增 `LIGHT_ADC_EOC_GUARD`），无既有行为被改 |
| **阴影头 mock MCU 层**（自研，链接真实 `light.c`） | 实现者的测试只覆盖纯滞回函数（ADC/GPIO 全为不执行的空桩），**采样序列/均值/超时回退未被覆盖**；本轮用仿真 MCU 层把真实 `light.c` 跑起来 |
| 滞回真值表直接调用真实 `light_code_is_dark` | 需求硬性语义（无光 = code ≥ ENTER） |
| 交叉编译 + AC5 + 对现网镜像影响 | 可编译性与回归 |

不适用（后续 ITEM）：采样节拍接入（ITEM-006）、上报判定（ITEM-008）、旧通路退役（ITEM-009）、纯逻辑抽取（ITEM-005）、板级电气/时序（TD-002 T-L3-01/03/04/05，需暗箱/照度计/示波器，本轮无仪器）。

## B. 独立仿真 MCU 层验证（通过，核心）

自研阴影头（`test/mock_mcu/cw32l010_{gpio,adc,sysctrl}.h` + `mock_cw32.h`）置于 include 路径最前，使**真实的 `light.c`** 针对仿真 MCU 层编译；`delay_ms` 为计数桩，ADC 样本序列可脚本化（含 EOC 永不置位 = 转换超时）。

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
<mingw64 gcc> -std=c11 -Wall -Wextra -I test/mock_mcu -I USER/inc -I COMMON \
  test/host_light_verify_ev.c USER/src/light.c -o light_verify && ./light_verify
=> ==== result: 45 passed, 0 failed ====   (0 编译告警；未引用任何厂商头)
```

| 组 | 独立断言 | 结果 |
|---|---|---|
| 引脚所有权与 ADC 配置 | PB05 = 推挽输出且初始低；PB04 = 模拟输入；未配置其它引脚；ADC_Init 仅 1 次且通道 = `ADC_InputCH11`(AIN11)、`ADC_Clk_Div8`、`ADC_SampTime390Clk`、单次模式；init 后 ADC 关闭 | PASS ×10 |
| 采样序列（LIT 场景） | PB05 先高、后低（每次采样恰好 1 高 1 低、无其它引脚写）；稳定延时 = `LIGHT_SETTLE_MS`(100 ms) 在采样前；恰好 `LIGHT_ADC_SAMPLES`(8) 次转换；ADC 使能/关闭各 1 次且事后关闭 | PASS ×10 |
| DARK 场景 | code=400 → DARK | PASS |
| 均值 | 4×0 + 4×4095 → `light_last_code()`=2047（整数均值），判定用均值 | PASS ×3 |
| **滞回状态机（经真实 `light_sample`）** | 400→DARK；300（带内）保持 DARK；250(≤EXIT)→LIT；300(<ENTER) 保持 LIT；350(=ENTER)→DARK；349(>EXIT) 保持 DARK | PASS ×6 |
| 部分转换超时 | 2 次超时 + 6×1000 → 均值 1000（仅对成功样本求均值）；仍尝试 8 个时隙 | PASS ×3 |
| **全部转换超时（分压开路）** | 回退满量程 4095 → DARK（倾向上报、不阻塞）；PB05 仍置低；ADC 仍关闭；**轮询次数 800008，严格落在 8×GUARD…8×(GUARD+1) 内（有界，无死等）** | PASS ×5 |
| `light_reset_state()` | 复位后首样本用 ENTER 阈值（300→LIT），随后 350→DARK | PASS ×2 |
| 滞回真值表（直接调用） | 0/349→LIT；350/4095→DARK；251 保持/250→LIT；带内保持原状态 | PASS ×4 |

实现者自带 harness 也被独立执行确认：`host_light_check.c` → **17 passed, 0 failed**。

## C. 配置点与引脚所有权（通过，带依赖）

- `sensor_config.h` 仍为唯一配置点：本轮仅 +2 行（`LIGHT_ADC_EOC_GUARD`），光照阈值/引脚/采样数/稳定延时等常量未重复定义。
- `light.c` 内部：PB05 → `GPIO_MODE_OUTPUT_PP`（初始 `RESET`）；PB04 → `GPIO_MODE_ANALOG`（即 AIN11）；**非采样期 PB05 低、ADC 关闭**（低功耗约束）。
- **整个固件层面的引脚所有权尚未唯一（后续 ITEM 依赖，见 F.1）**。

## D. 构建与对现网镜像的影响（通过）

| 项 | 结果 |
|---|---|
| GNU 交叉编译 | 0 error；29 warning（与基线同数，无一条指向 `light.c`） |
| `light.o` | text 364 B / data 0 / bss 4；符号 `light_init`、`light_sample`、`light_last_code`、`light_code_is_dark`、`light_reset_state`；需 `__aeabi_uidiv`（32 位整数除法，无浮点）与 `memset` |
| 固件镜像 | FLASH 34,060 B、RAM 1,960 B；`sensor_fw.bin` MD5 与 ITEM-003 构建**完全相同**（模块尚未被调用，`--gc-sections` 丢弃） |
| AC5（`--c99`，CMSIS 5.9.0） | `light.c` 0 error；`main.c` 0 error |
| 宿主机 L0 回归 | `test/build_test.sh` → 56 passed, 0 failed |

## E. 观测与边界

- 仿真层验证的是**软件序列与判定逻辑**：PB05 高/低时序、延时值、转换次数、均值、滞回、超时回退与有界轮询。
- **未验证**（属板级）：PB05 高电平的实际建立时间与分压 RC/光敏响应（TD-002 T-L3-05）、实际暗/亮照度下的码值与阈值标定（T-L3-03）、ADC 真实 EOC 时序与 5 MΩ 源阻抗下的采样精度（T-L3-04）、PB04 电压/引脚真实电平（T-L3-01）。本轮无暗箱/照度计/示波器。

## F. 依赖与交接

1. **【重要・交 ITEM-009】整机引脚所有权冲突仍未消除**：`main.c` 目前仍调用 `optcfg_init(); hall_init();`（第 245/246 行），而 `optcfg.h` 定义 `OPT_IN_PIN = GPIO_PIN_5`（PB05 数字输入）、`F2_PWR_EN_PIN = GPIO_PIN_6`（PB06）、`hall.c` 将 `HALL_IN_PIN = GPIO_PIN_4`（PB04）配为带 EXTI 的数字输入。即：**在 ITEM-009 退役 hall/optcfg/params/history 之前，PB04/PB05/PB06 仍会被旧模块重新配置，光照通路在真实板上不会按设计工作**。这不属于 ITEM-004 的实现缺陷（ITEM-004 只交付 `light.c/.h`，且任务清单已把退役列入 ITEM-009），但意味着：在 ITEM-009 完成前，TD-002 的 T-L3-01/03/04/05 板级验证**不具备有效前提**，不得据此判光照功能通过。
2. **接线（交 ITEM-006）**：`light_init()` 与 `light_sample()` 尚未被主循环调用（因此镜像零变化）。ITEM-006 需在采样流程中先 `light_sample()` 后测温湿度，并注意 `light_sample()` 自带 100 ms 阻塞延时（RTA-002 时间预算已计入）。
3. **滞回纯函数重复（交 ITEM-005）**：`light_code_is_dark(code, prev_dark)` 已在 `light.c` 中为可宿主机直测的全局函数；ITEM-005 将其抽取到 `fw_core.c` 时，应保持签名/语义一致并让 `light.c` 复用，避免两份实现漂移。

## G. ITEM-004 判定

“PB05 输出高 → 稳定延时 → PB04/AIN11 多次取样求均值 → 阈值+滞回输出 DARK/LIT（无光=读数≥进入阈值）→ 采样结束 PB05 置低”以及“转换超时不阻塞、回退满量程（无光）”全部经**真实 `light.c` + 独立仿真 MCU 层**复现成立，既有配置点与现网镜像无回归。

**判定：TEST_PASS**（F.1 的整机引脚冲突为已登记的后续 ITEM 依赖，不影响本项模块级判定）。

---

# ITEM-005 验证（`fw_core.c` 纯逻辑：CRC-8 / 换算 / 滞回 / 上报判定）

验证对象：任务项 **ITEM-005**（不依赖 MCU 寄存器的纯逻辑：`fw_crc8_gxht` 满足 `CRC(0xBEEF)=0x92`；GXHT40 原始字→x10 整数换算含负温与 0–100%RH 截断；`light_code_is_dark` 滞回；`sensor_decide_report`）
被测提交：`7f8381e`；源码基线：`7342368`
结论：**TEST_PASS**（含 1 项需需求方确认的 35.0 ℃ 边界语义分歧，见 F.1；实现与已验证上游契约 IC-002 一致）

## A. 选测说明与变更范围

| 选测项 | 理由 |
|---|---|
| 变更范围审查 | `fw_core.c` +95/0、`fw_core.h` +26/2、`sensor_config.h` +22/−14（拆出 `SENSOR_CONFIG_NO_MCU` 纯数值段）、`gxht40.c` +5/−42（删本地静态 CRC/换算）、`light.c` +1/−11（删本地滞回）、`light.h` −8（声明移走） |
| **独立参考穷举校验** | 这是本项目最易穷举的纯逻辑：对**全部 65536 个温度原始字、65536 个湿度原始字、65536 个两字节 CRC、4096×2 滞回输入、572 组上报组合**与独立参考逐位比对 |
| 重构后行为不变（回归） | 重跑 ITEM-003/ITEM-004 的独立 harness 与实现者 harness，确认删掉本地副本后驱动/光照行为不变 |
| 依赖检查 | 证明 `fw_core.c` 真不依赖 MCU 头/外设符号；GNU + AC5 可编译 |

不适用（后续 ITEM）：采样节拍接入（ITEM-006）、条件上报接入（ITEM-008）、旧通路退役（ITEM-009）、MDK/IAR 源列表（ITEM-011）、板级时序/电气（TD-002 T-L2/L3 需仪器）。

## B. 独立参考与穷举校验（通过，核心）

期望值由**独立 Python 参考**生成：换算用精确有理数 `-450 + floor(1750·raw/65536 + 1/2)`（round-half-up，与 C 实现不同的写法），CRC 用从零写的位算法，判定用直接按 IC-002 写的布尔式。harness 对结果计算 FNV-1a 32 位哈希并与参考常量比对。

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
<mingw64 gcc> -std=c11 -Wall -Wextra -I USER/inc -I COMMON \
  test/host_fw_core_verify_ev.c USER/src/fw_core.c -lm -o fw_core_verify && ./fw_core_verify
=> ==== result: 53 passed, 0 failed ====   (0 编译告警)
```

| 穷举域 | 哈希（实测 = 参考） | 结论 |
|---|---|---|
| CRC-8：全部 256×256 个两字节输入 | `0x7CC4B9C5` | 65536 个 CRC 全部一致 |
| 温度换算：全部 65536 个原始字 | `0xBB0C5ABB` | 含舍入方向完全一致 |
| 湿度换算：全部 65536 个原始字 | `0x5F674BC5` | 含 0..1000 截断一致 |
| 滞回：全部 4096 个码值 × 2 种前态 | `0x19DB6FE6` | 8192 个判定一致 |
| 上报：13×11×(have,dark) = 572 组合 | `0xF7D66E3E` | 与 IC-002 布尔式一致 |

向量与边界（期望值均来自独立参考）：

- **CRC-8**：`{0xBE,0xEF}→0x92`（手册参考向量）；`{0x00}→0xAC`；`"123456789"→0xF7`（CRC-8/NRSC-5 标准校验值，独立确认变体）；GXHT40 帧字节 `{0x66,0x66}→0x93`、`{0x72,0xB0}→0xDC`；空长度→`0xFF`。
- **温度**：`0x0000→-450`；`0x073E→-400`（−40.0 ℃ 域下界）；`0x41C2→0`；`0x6666→250`；`0xF89D→1250`（125.0 ℃ 域上界）；`0xBEEF→855`；`0xF8CA→1251`（域上界外）；`0xFFFF→1300`；**恰好 0.5 的舍入平分点** `0x4000→-12`、`0xC000→863`（round-half-up）。
- **湿度**：`0x0000→0`（原始 −60 截断）；`0x0C64→1`；`0x1234→29`；`0x72B0→500`；`0xD8FD→1000`；`0xFFFF→1000`（原始 1190 截断）。
- **组合换算 `gxht40_raw_to_x10`**：有效对→`true` 且写输出；温度越域（−450 / 1251）→`false` **且不写输出**；域端点 −400/1250 接受；`NULL` 输出指针不崩且仍返回有效性。
- **滞回**：`0/349→LIT`；`350/4095→DARK`；已暗时 `251` 保持、`250→LIT`；带内保持前态（不抖动）。
- **上报**：无前值 cur=200→false、cur=350→false、cur=351→true；下降恰好 9→false、10→true；下降满足但 LIT→false；prev==cur→false；升温→false；负温（−10.0→−12.0 ℃，降 2.0 ℃）→true；int16 极值不溢出（差值以 32 位算）。

## C. 重构一致性与回归（通过）

- **重复实现已消除**：`fw_crc8_gxht`、`light_code_is_dark` 在源码树中各只有 **1 处定义**（均在 `fw_core.c`）；`gxht40.c` 中已无本地 `gxht40_crc8/conv_*`。
- 重跑 **ITEM-003 独立 harness**（`host_gxht40_verify_ev.c`，链接重构后 `gxht40.c`+`fw_core.c`）：**42/42**；**ITEM-004 独立 harness**（`host_light_verify_ev.c`）：**45/45**。
- 实现者 harness：`host_fw_core_pure_check.c` **36/36**、`host_gxht40_check.c` **27/27**、`host_light_check.c` **17/17**；宿主机 L0 回归 `test/build_test.sh` **56/56**。
- 说明：`light_code_is_dark` 的声明从 `light.h` 移到了 `fw_core.h`；已有调用方（`light.c`）已包含 `fw_core.h`。本轮据此同步更新了我方 ITEM-004 harness 的包含（否则会出现隐式声明告警），属测试侧适配。

## D. 依赖与构建检查（通过）

| 检查 | 结果 |
|---|---|
| `fw_core.c` 不依赖 MCU 头 | 预处理输出中 `cw32l010` 匹配数 = **0**（`SENSOR_CONFIG_NO_MCU` 纯数值段生效） |
| `fw_core.o` 无外设符号 | `nm` 中无 `ADC_*`/`GPIO*`/`CW_*` 引用 |
| GNU 交叉编译 | 0 error；29 warning（与基线同数，无一条指向 `fw_core.c`/`gxht40.c`/`light.c`） |
| 镜像 | FLASH 34,060→**34,352 B**（+292 B，为 `fw_core.o` 新增纯逻辑）；RAM 1,960 B 不变；`.bin` 变化（`fw_core.o` 已被引用，符合预期） |
| AC5（`--c99`，CMSIS 5.9.0） | `fw_core.c` / `gxht40.c` / `light.c` / `main.c` 均 **0 error** |
| 新增符号 | `fw_crc8_gxht`、`gxht40_temp_raw_to_x10`、`gxht40_hum_raw_to_x10`、`gxht40_temp_x10_valid`、`gxht40_raw_to_x10`、`light_code_is_dark`、`sensor_decide_report`（`fw_core.o` text 1588 B） |

## E. 观测边界

- 本项为纯逻辑，宿主机穷举即完整覆盖其输入域；不涉及板上时序。
- **未验证**（后续 ITEM）：这些纯函数是否被采样/上报流程正确调用（ITEM-006/008）、真实温度下报行为（TD-002 T-L5 边界矩阵，需实板/温箱）。

## F. 发现与交接

1. **【需需求方确认・非实现缺陷】恰好 35.0 ℃ 的下降分支语义分歧**：
   - ITEM-005 任务文本与 FD-002 §6.4 的代码片段为 `((prev−cur)>9 且 DARK) 或 (cur>350)`，**无 `cur<350` 条件**；在「`cur` 恰好 =350 且下降 >9 且 DARK」时会判 true。FD-002 §6.4 还注释说 IC-002 的 `T<35` 项“属冗余”。
   - 已验证上游契约 **IC-002 §2** 写的是 `(T[n] < 35.0) AND (下降) AND DARK OR T[n] > 35.0`；**FWR-104** 的边界行也写“恰好 35.0 ℃ 不触发”。即在该点 IC-002/FWR-104 与任务文本/设计代码不一致（FD-002 的“冗余”判断在恰好 35.0 ℃ 处不成立）。
   - 实现选择了 **IC-002/FWR-104**（`cur == 350` 直接返回 false），并在代码中以注释标注依据。
   - 本轮先按任务文本写出参考式，穷举时命中该分歧点；按 IC-002 修正参考式后哈希完全一致（`0xF7D66E3E`）。**精确分歧集仅为**：`have_prev=true, dark=true, cur=350, prev∈{1000,32767}`（共 2 点/572 组合）。
   - 结论：实现与已验证契约一致，不判实现缺陷；但**任务文本/设计注释需同步澄清**（建议需求方确认“恰好 35.0 ℃ 时下降分支是否允许上报”，并修正 FD-002 §6.4 的“冗余”注释）。此为本能力边界外的文档/需求变更，登记为交接。
2. **接口位置变更（交下游调用方）**：`light_code_is_dark` 现仅在 `fw_core.h` 声明；后续 ITEM-006/008 接入时应包含 `fw_core.h`。
3. **MDK/IAR 源列表仍未包含 `gxht40.c`/`light.c`**（匹配数 0），属 ITEM-011；`fw_core.c` 本就在列表中。

## G. ITEM-005 判定

四个纯逻辑均已在 `fw_core.c` 实现且不依赖 MCU 寄存器/头文件，经**独立参考穷举**逐位一致；重构后驱动与光照行为无回归（独立 harness 42/42、45/45），GNU 与 AC5 均 0 error。

**判定：TEST_PASS**（F.1 为需需求方确认的边界语义分歧，实现侧与已验证契约 IC-002/FWR-104 一致）。

---

# ITEM-006 验证（`measure.c` 采样流程改造）

验证对象：任务项 **ITEM-006**（每个周期先光照后温湿度；GXHT40 读 NACK/CRC 错按配置上限有界重试；整周期失败不上报、不更新前一有效温度、不构造 0 值；成功后更新前一有效温度与最近有效湿度）
被测提交：`1a0ba51`；源码基线：`9de0946`
结论：**TEST_PASS**

## A. 选测说明与变更范围

| 选测项 | 理由 |
|---|---|
| 变更范围审查 | `measure.c` **+154/−249**（重写采样部分）、`measure.h` −3（删 `samples_since_report`/`first_sample_reported` 声明）、`fw_core.c` +2（`SENSOR_CONFIG_NO_MCU` 改为可外部预定义，供宿主机 harness 使用）、新增 `host_measure_flow_check.c` + `mock_measure_mcu/` |
| **独立仿真 MCU 层流程验证** | ITEM-006 的新逻辑就是“周期流程 + 状态推进/不推进”；用自研阴影头编译**真实 `measure.c`**，把光/温湿度原语脚本化，用**参考模型逐步比对** |
| 旧序列/旧调度清理检查 | 确认 AHT21 序列与旧上报调度已移除且无残留引用 |
| 构建 + 回归 | GNU/AC5 可编译，既有 harness 无回归 |

不适用（后续 ITEM）：3 分钟节拍（ITEM-007，当前仍为 RTC 1 分钟一拍）、上报发送路径对齐（ITEM-008）、旧通路退役（ITEM-009）、板级时序/上报矩阵（TD-002 T-L2/L3/L5，需仪器）。

## B. 独立仿真 MCU 层流程验证（通过，核心）

自研阴影头（`test/mock_measure_ev/cw32l010_{gpio,adc,sysctrl,uart}.h` + `mock_measure_hw.h`）置于 include 路径最前，编译**真实 `measure.c`**；`light_sample`/`gxht40_measure` 脚本化（含 `ERR_IO`/`ERR_CRC`/`ERR_RANGE`），并链接真实 `fw_core.c`（`sensor_decide_report`）、`sf_i2c.c`、`encrytogate.c`。

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
<mingw64 gcc> -std=c11 -Wall -Wextra -Wno-unused-function -I test/mock_measure_ev \
  -I USER/inc -I UM2005C -I COMMON test/host_measure_flow_verify_ev.c USER/src/measure.c \
  USER/src/fw_core.c USER/src/sf_i2c.c USER/src/encrytogate.c -lm -o measure_verify && ./measure_verify
=> ==== result: 79 passed, 0 failed ====
```

方法：harness 内维护一个**参考模型**（与 measure.c 无关的流程重述），逐周期比对 `tempvalue`、`huminityvalue`、`report_req` 与“本周期是否新置上报”。14 个脚本周期：`(dark, gxht, temp, hum)` =
`(0,OK,250,500) (0,OK,240,510) (1,OK,220,600) (1,ERR_IO,-,-) (1,OK,210,610) (1,ERR_CRC,-,-) [待上报中失败] (0,OK,351,700) (0,OK,350,710) (1,OK,340,720) (0,OK,330,730) (1,OK,300,740) (1,ERR_CRC,-,-) (0,OK,280,750)`。

| 组 | 独立断言 | 结果 |
|---|---|---|
| 逐周期状态（14 周期×4） | `tempvalue`/`huminityvalue` 等于最近一次成功值；`report_req` 与参考模型一致；“新置上报”与参考判定逐周期一致；`sample_flag` 被消费 | PASS ×56 |
| 惰性初始化 | `light_init`/`gxht40_init`/I2C 物理层各只初始化 1 次；`i2c_obj_find("i2c0")` 绑定成功 | PASS ×3 |
| 采样顺序 | 每周期恰好 1 次 light + 1 次 gxht，且**恒为 light 先于 gxht**（含失败周期） | PASS ×3 |
| 失败不构造 0 值 | 4 个失败周期后 `tempvalue`=280（最后一次成功值）、`huminityvalue`=750，从未变为 0 | PASS ×3 |
| **失败不推进前值** | 周期 3（ERR_IO，前值 240）失败后，周期 4 测 210（降 30>9，暗）→ **必须上报**；若失败污染前值（如置 0）则降为负、不会上报 | PASS ×2 |
| 失败不清除待上报 | 周期 6 前手动置 `report_req=1`，失败后仍为 1，且状态不变 | PASS ×2 |
| 当前周期光照不泄露 | 周期 11（暗+降温）上报；周期 12（暗但失败）不上报；周期 13（亮+降温）不上报 | PASS ×3 |
| 35.0 ℃ 边界（与 ITEM-005 一致） | 周期 7（351，亮）上报；周期 8（350，亮）不上报 | 已由逐周期判定覆盖 |
| 休眠门控 | 待上报/待采样时**不**进深睡；空闲时进深睡 1 次 | PASS ×3 |
| 节拍门控 | `sample_flag==0` 时无 light/gxht 调用、无状态变化 | PASS ×2 |
| 低功耗 | 采样路径不配置调试 UART（`UART_Init` 调用数 = 0） | PASS |

实现者自带 harness 也被独立执行确认：`host_measure_flow_check.c`（按其在文件头记录的 `-DSENSOR_CONFIG_NO_MCU -Imock_measure_mcu` 命令）→ **15 passed, 0 failed**。

## C. 需求逐条对照

| ITEM-006 声明 | 实现位置 | 独立证据 |
|---|---|---|
| 每周期先取光照再取温湿度 | `measure_sample()`：`light_sample()` → `gxht40_measure()` | B.S2（逐周期调用顺序） |
| 读 NACK / CRC 错按配置上限有界重试 | `gxht40_measure()` 内部（ITEM-003 已验证）；流程每周期只调一次 | B.S1（每周期 gxht 调用数 = 1） |
| 整周期失败不上报/不更新前值/不构造 0 值 | `measure_sample()` 失败即返回，不写 `tempvalue`/`huminityvalue`；`temperature_process()` 仅成功分支判定与推进 | B.S3/S4/S5 |
| 成功后更新前一有效温度与最近有效湿度 | `s_prev_temp_x10`/`s_have_prev`/`s_last_hum_x10` 仅成功分支更新；判定用**更新前**的前值 | B 逐周期比对（含降 1.0/3.0 ℃ 正确触发） |

补充确认：AHT21 序列（`0x70/0xAC/0x71/0x33`）已从 `measure.c` 完全移除（仅注释提到“不再使用 AHT21”）；`first_sample_reported`/`samples_since_report` 已无任何代码引用（仅 `main.c` 一处旧注释提到该名字，属文档残留）；首周期无前值时不上报（符合 FWR-108）。

## D. 回归与构建（通过）

| 项 | 结果 |
|---|---|
| 既有独立 harness | `host_fw_core_verify_ev` 53/53；`host_gxht40_verify_ev` 42/42；`host_light_verify_ev` 45/45 |
| 宿主机 L0 | `test/build_test.sh` 56/56 |
| GNU 交叉编译 | 0 error；28 warning（比上一版少 1 条，无一条指向 `measure.c`） |
| AC5（`--c99`，CMSIS 5.9.0） | `measure.c` **0 error / 1 warning**；`main.c`/`gxht40.c`/`light.c`/`fw_core.c`/`sf_i2c.c` 均 0 error |
| 接线 | `measure.o` 引用 `light_init/light_sample/gxht40_init/gxht40_measure/sensor_decide_report`；三者在 `sensor_fw.elf` 中均已被链接（模块首次真正进入固件） |
| 资源 | FLASH 34,352→**35,884 B**（+1,532，模块接入）；RAM 1,960→**1,896 B**（−64，旧测量缓冲/参数结构体删除） |
| 镜像 | `sensor_fw.bin` MD5 `fff8aa9c…`（较 ITEM-005 变化，因驱动/光照模块接入，符合预期） |

## E. 观察与交接

1. **`s_last_hum_x10` 赋值后从未被读（AC5 `#550-D`）**：当前 `send_data_to_gateway()` 直接用 `huminityvalue` 组帧，未用该变量。任务要求“成功后更新最近有效湿度”，所以赋值本身是合规的；但**ITEM-008 应对齐发送路径（使用该变量或删除）**，以免遗留死存储与告警。
2. **发送路径与睡眠门控仍依赖旧模块**：`send_data_to_gateway()` 仍走 `optcfg_window_active()`/旧有界重试，`go_to_sleep()` 仍查 `hall_event_pending()`——属 ITEM-008（发送对齐）/ITEM-009（旧通路退役），本项未越界。
3. **`main.c` 旧注释**仍提到已删除的 `samples_since_report`（纯注释）；建议 ITEM-007 一并清理。
4. **上电即采一次**：`sample_flag` 初值为 1，故上电后立即执行一个采样周期；无前值时不强制上报（已验）。若产品不希望上电立即测量，属需求变更（FWR-108 仅约束“不强制上报首样本”）。
5. **3 分钟节拍未在本项**（当前 `RTC_IRQHandlerCallBack` 仍为 1 分钟一拍），属 ITEM-007；本项不做判定。

## F. ITEM-006 判定

“先光照后温湿度”“失败不上报/不推进前值/不构造 0 值/不清除待上报”“成功推进前值与最近有效湿度”“每周期一次有界重试调用”均经**真实 `measure.c` + 独立仿真 MCU 层与参考模型**逐周期复现成立；旧 AHT21 序列已清除；GNU 与 AC5 均可编译，既有回归全部通过。

**判定：TEST_PASS**。

---

# ITEM-007 验证（3 分钟采样节拍）

验证对象：任务项 **ITEM-007**（RTC 1 分钟中断累计到配置的 3 次才置采样标志；一个采样周期只执行一次测量；移除小时上报与首样本强制上报逻辑）
被测提交：`fd65dcf`；源码基线：`2ea1431`
结论：**TEST_PASS**

## A. 选测说明与变更范围

| 选测项 | 理由 |
|---|---|
| 变更范围审查 | 仅 `main.c` **+12/−11**：`RTC_IRQHandlerCallBack` 阀值 `>= 1` → `>= SENSOR_SAMPLE_TICKS`；删除未用全局量 `temp_cnt`/`work_period_flag`；注释更新 |
| **真实 `main.c` 的 RTC 回调行为验证** | 用自研仿真 MCU 层直接驱动 `RTC_IRQHandlerCallBack()` 与 `RTC_Configuration()` |
| **与基线的全函数反汇编差异** | 证明本次改动**仅**是节拍阀值 + 2 个已删全局量，无其它代码变化 |
| 旧上报逻辑残留检查 | 确认小时上报/首样本强制上报已无实现（仅注释提及） |
| 回归 | 既有独立 harness + 宿主机 L0 + 构建 |

不适用（后续 ITEM）：发送路径对齐（ITEM-008）、旧通路退役（ITEM-009）、MDK/IAR 源列表（ITEM-011）、板级长时基实测（TD-002 T-L4-01/02，需逻辑分析仪/电流波形）。

## B. 独立 RTC 节拍验证（通过，核心）

自研阴影头（`test/mock_main_ev/`，共 11 个头）置于 include 路径最前，编译**真实 `main.c`**（为避免与本 harness 的 `main` 冲突，编译时用 `-Dmain=firmware_main_entry`，并用 `--gc-sections` 丢弃未引用函数）；harness 直接调用 `RTC_Configuration()` 与 `RTC_IRQHandlerCallBack()`。

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
<mingw64 gcc> -c -ffunction-sections -fdata-sections -Dmain=firmware_main_entry \
  -I test/mock_main_ev -I USER/inc -I UM2005C -I COMMON USER/src/main.c -o main_ev.o
<mingw64 gcc> -c -ffunction-sections -fdata-sections -I test/mock_main_ev -I USER/inc \
  -I UM2005C -I COMMON test/host_rtc_cadence_verify_ev.c -o rtc_ev.o
<mingw64 gcc> -Wl,--gc-sections main_ev.o rtc_ev.o -o rtc_verify && ./rtc_verify
=> ==== result: 17 passed, 0 failed ====
```

| 组 | 独立断言 | 结果 |
|---|---|---|
| RTC 间隔 | `RTC_Configuration()` 传给 `RTC_SetInterval` 的值 == `RTC_INTERVAL_EVERY_1M`；并由厂商头 `cw32l010_rtc.h` 独立确认该宏 = `0x03`（1 分钟档，同表还有 0.5s/1s/1h/1d/1month）；RTC 初始化/中断使能/NVIC/LSI 各 1 次 | PASS ×4 |
| 无中断挂起 | `RTC_GetITState` 返回 RESET 时回调不动计数器也不置标志 | PASS |
| 第一周期 | 第 1/2 拍：无标志、计数器 1→2；第 3 拍：**置采样标志且计数器归零** | PASS ×3 |
| 后续周期 | 第 4/5 拍无标志，第 6 拍再次置标志 | PASS ×3 |
| 长时行为 | 连续 30 拍：**恰好 10 次采样机会，且落在第 3/6/9/…/30 拍**；计数器始终 < 3（有界） | PASS ×3 |
| 配置关系 | `SENSOR_RTC_TICK_PERIOD_MIN × SENSOR_SAMPLE_TICKS = 3`（即 3 分钟） | PASS ×2 |

“一个周期只测一次”由两端共同保证：ISR 每 3 拍才置一次 `sample_flag`（本 harness），而 `temperature_process()` 在完成一次测量后清除该标志（**已在 ITEM-006 独立 harness 79/79 中验证**）。

## C. 与基线的机器码差异（通过，最强回归证据）

在临时 worktree 中独立构建基线 `2ea1431`，对全二进制反汇编做归一化逐指令比对：

```
RTC_IRQHandlerCallBack:
  baseline:  cmp r3, #0   ; beq.n ...      (rtc_set_cnt >= 1)
  HEAD:      cmp r3, #2   ; bls.n ...      (rtc_set_cnt >= 3)
```

除该阀值外，全部差异仅为**字面量池地址偏移**（因删除 2 个 .bss 全局量导致地址平移 4 字节）与文件路径行；**没有任何其它指令变化**。基线 ELF 中 `temp_cnt`(0x200004cd)、`work_period_flag`(0x200004cc) 存在，HEAD ELF 中已不存在。

## D. 旧上报逻辑清理（通过）

- 小时上报（旧 `samples_since_report >= 60`）与首样本强制上报（旧 `first_sample_reported`）**已无任何实现或引用**；全树仅两处注释提到该词：`main.c:105`（说明“已无小时/首样本上报”）与 `main.c:260` 的**陈旧注释** `/* 立即/小时/首样本上报 (FR-203) */`。后者是文档级残留，建议 ITEM-008/010 清理（不影响行为）。
- ELF 中无 `first_sample*`/`samples_since*`/`temp_cnt`/`work_period_flag` 符号。

## E. 构建与回归（通过）

| 项 | 结果 |
|---|---|
| GNU 交叉编译 | 0 error；28 warning（与基线同数；`main.c` 的 3 条 warning 均为模板/既有：`__write` 未用参数、`main` 返回类型、`assert_failed` 未用参数） |
| AC5（`--c99`，CMSIS 5.9.0） | `main.c` **0 error / 0 warning** |
| 资源 | FLASH 35,884 B、RAM 1,896 B（与基线**完全相同**，仅指令常量与 2 字节全局量变化）；`.bin` MD5 `2f6efa92…`（基线 `fff8aa9c…`） |
| 既有独立 harness | fw_core 53/53、gxht40 42/42、light 45/45、measure 流程 79/79 |
| 宿主机 L0 | `test/build_test.sh` 56/56 |

## F. 观测与交接

1. **节拍精度未实测**：RTC 时钟源为 LSI，3 分钟为近似值；需求未给精度指标。实际间隔/离散度需板级长时基测量（TD-002 T-L4-01），本轮无逻辑分析仪/电流波形，**未验证**。
2. `main.c:260` 陈旧注释（“立即/小时/首样本上报”）建议后续清理。
3. 采样仍与光照/温湿度模块以及 `send_data_to_gateway`（旧路径，ITEM-008）在同一主循环内，每次 RTC 唤醒都会进一次循环体（无采样时立即回睡）；低功耗实测仍属板级（TD-002 T-L4-04/T-L8）。

## G. ITEM-007 判定

“RTC 1 分钟中断累计 3 次才置采样标志”“一个周期只测一次”“移除小时上报与首样本强制上报”均经**真实 `main.c` + 独立仿真 MCU 层**验证，并与基线逐指令比对确认无其它副作用；既有回归全部通过。

**判定：TEST_PASS**。

---

# ITEM-008 验证（条件上报的 433 发送路径）

验证对象：任务项 **ITEM-008**（满足 `sensor_decide_report` 时 `encode_frame10` 组 10 字节帧并经 `app_um2005C_send_data_timeout` 有界发送；帧布局/字节序/Feistel 不变；仅在发送成功后清除待上报；失败按上限重试后放弃本轮）
被测提交：`6a69601`；源码基线：`cdbc8cb`
结论：**TEST_PASS**

## A. 选测说明与变更范围

| 选测项 | 理由 |
|---|---|
| 变更范围审查 | `measure.c` **+12/−19**：删除手工 `send_data[0..7]` 预组装（`encode_frame10` 会写满 10 字节，属死代码）；改传 `s_prev_temp_x10`/`s_last_hum_x10`（触发上报的样本）；`len` 改用 `SENSOR_RF_FRAME_LEN`；`main.c` 仅注释；**`encrytogate.c`/`feistel_al.c` 未修改** |
| **发送路径独立验证（真实 `measure.c`）** | 用脚本化发送结果驱动成功/失败/用尽/窗口分支，验证“仅成功后清除”与“有界重试后放弃” |
| **空口帧互操作（真实编码器 + 真实网关解码器）** | 验证帧布局/字节序/加密不变；并证明被删的手工组帧确为死代码 |
| 回归与构建 | 既有 harness + GNU/AC5 |

不适用（后续 ITEM）：旧通路退役（ITEM-009）、MDK/IAR 源列表（ITEM-011）、网关兼容性正式核对（ITEM-012）、**真实 433 射频与丢包/失败上限实板验证**（TD-002 T-L6-03/T-L7，需真实射频与仪器）。

## B. 发送路径独立验证（通过，核心）

真实 `measure.c` + 仿真 MCU 层（`test/mock_measure_ev/`），脚本化光/温湿度与 `app_um2005C_send_data_timeout` 返回值，并用**真实采样周期**产生待上报标志。

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
<mingw64 gcc> -std=c11 -Wall -Wextra -Wno-unused-function -Wno-misleading-indentation \
  -Wno-unused-const-variable -I mock_measure_ev -I ../USER/inc -I ../UM2005C -I ../COMMON \
  host_rf_report_verify_ev.c ../USER/src/measure.c ../USER/src/fw_core.c \
  ../USER/src/sf_i2c.c ../USER/src/encrytogate.c -lm -o rf_report_verify && ./rf_report_verify
=> ==== result: 28 passed, 0 failed ====
```

| 组 | 独立断言 | 结果 |
|---|---|---|
| 无待上报 | `report_req==0` 时不调用发送器 | PASS |
| 成功路径 | 真实周期（暗+降 2.0 ℃）置起待上报 → 恰好 1 次发送；`len==SENSOR_RF_FRAME_LEN==10`；超时 == `SENSOR_RF_TX_TIMEOUT_MS`；**成功后清除待上报** | PASS ×5 |
| **发送帧内容** | 交给发送器的 10 字节 == `encode_frame10(uid, 触发样本温度, 触发样本湿度)`；且发送时刻 `tempvalue/huminityvalue` 就是该样本，**与旧代码路径编码逐字节相同**（无帧变化） | PASS ×4 |
| 失败不假成功 | 连续失败时 `report_req` 保持 1（不提前清除）；第 3 次成功才清除 | PASS ×4 |
| **有界重试后放弃** | 持续失败 → 恰好 `SENSOR_RF_TX_RETRY`(3) 次尝试后清除待上报并放弃；此后不再发送 | PASS ×3 |
| 新轮次 | 放弃后新的待上报重新发送，重试计数重置（第 2 次成功） | PASS ×2 |
| 配置窗口 | `optcfg_window_active()` 为真 → 不发送且保留待上报；窗口关闭后补发 | PASS ×2 |
| 失败周期 | 测量失败不置待上报、不发送 | PASS ×2 |

## C. 空口帧与网关互操作（通过）

自研 harness 直接链接**真实传感器编码器**（`USER/src/encrytogate.c`）与**真实网关解码器**（`CH592EVT/.../beiwov2/APP/feistel_al.c`）——两份独立实现互证。

```
<mingw64 gcc> -std=c11 -Wall -Wextra -Wno-unused-function -Wno-unused-const-variable \
  -Wno-misleading-indentation -I ../USER/inc -I <gateway APP> host_rf_frame_verify_ev.c \
  ../USER/src/encrytogate.c <gateway APP>/feistel_al.c -o rf_frame_verify && ./rf_frame_verify
=> ==== result: 10 passed, 0 failed ====
```

| 组 | 独立断言 | 结果 |
|---|---|---|
| 往返 | 12 组向量（含 -100/−400/−450/1250/1300/32767/−32768、湿度 0/1000/65535）逐位往返一致（uid[0..3]、temp、hum） | PASS |
| 字段/字节序 | 温度在 `p[4..5]` 小端、湿度在 `p[6..7]` 小端、id = `uid[0..3]`；负温符号保留 | PASS ×3 |
| **全 10 字节写入** | 同一输入写入预填 0xAA 与清零缓冲区结果相同 → **证明 ITEM-008 删除的手工组帧确为死代码** | PASS ×3 |
| 确定性 | 同输入两次编码逐字节相同 | PASS |
| 完整性 | 10 个密文字节逐一翻转均被网关 CRC 拒绝 | PASS |
| 网关→Android 映射 | 解码值按 `id | hum_be | temp_be` 组记录后用 `u16be@4`/`s16be@6` 读回，温度/湿度/id 归属正确（与 `app_um2006A.c` 与 `MqtttService.kt` 源码核对一致） | PASS |

实现者自带 harness 也被独立执行确认：`host_rf_frame_check.c` **14/14**、`host_measure_flow_check.c` **24/24**。

## D. 构建与回归（通过）

| 项 | 结果 |
|---|---|
| GNU 交叉编译 | 0 error；28 warning（无一条指向 `measure.c`） |
| AC5（`--c99`，CMSIS 5.9.0） | `measure.c` **0 诊断**——**ITEM-006 遗留的 `s_last_hum_x10` set-but-unused 告警已消除**（该变量现被 `encode_frame10` 使用） |
| 资源 | FLASH 35,884→**35,856 B**（−28，删除死代码）；RAM 1,896 B 不变；`.bin` MD5 `9b72933e…` |
| 符号 | `measure.o` 引用 `encode_frame10` 与 `app_um2005C_send_data_timeout`；`s_last_hum_x10` 已定义并使用 |
| 既有独立 harness | fw_core 53/53、gxht40 42/42、light 45/45、measure 流程 79/79 |
| 宿主机 L0 | `test/build_test.sh` 56/56 |

## E. 观察与交接

1. **真实射频未验证**：调制/前导/空中速率/丢包以及“发送失败上限”的实板行为需真实 433 链路与接收机（TD-002 T-L6-03/T-L7）；本轮仅验证软件侧调用、参数与状态机。
2. **待上报期间主循环空转**：`go_to_sleep()` 在 `report_req!=0` 时不进深睡，故发送失败重试期间 CPU 保持活跃（次数有界：≤3 次 × ≤200 ms 超时）；能量影响属功耗实测（TD-002 T-L8）。
3. `send_data_to_gateway()` 仍依赖 `optcfg_window_active()`（旧配置窗口），属 ITEM-009 退役范围。
4. 网关 BLE 记录顺序与 Android 解析本轮已做**源码核对 + 宿主机映射验证**；网关固件正式核对属 ITEM-012。
5. 本轮顺带清除了 ITEM-007 登记的 `main.c` 陈旧注释（“立即/小时/首样本上报”已改为条件上报）。

## F. ITEM-008 判定

“条件上报（`report_req` 门控）”“`encode_frame10` 组 10 字节帧 + 有界发送”“帧布局/字节序/Feistel 不变”“仅成功后清除待上报”“失败按上限重试后放弃本轮”均经**真实 `measure.c` 发送路径**与**真实编码器↔真实网关解码器互操作**独立验证；AC5 告警消除，既有回归全部通过。

**判定：TEST_PASS**。

---

# ITEM-009 验证（旧通路退役：hall / OPTCFG / params / history）

验证对象：任务项 **ITEM-009**（从工程与构建中移除 hall/OPTCFG/params/history 及其对 PB04/PB05/PB06 的初始化；移除 GPIOB 霍尔 EXTI 分支与 LPTIM 中 OPTCFG 分支；PB04 仅作 AIN11 模拟输入、PB05 仅作光照供电输出、PB06 不外驱动）
被测提交：`fef788f`；源码基线：`c556196`
结论：**TEST_PASS**

## A. 变更范围

| 类别 | 内容 |
|---|---|
| 删除 | `USER/src/{hall,optcfg,params,history}.c`、`USER/inc/{hall,optcfg,params,history}.h`（共 8 个文件全部删除） |
| 头文件 | `main.h` 移除 4 个 include；`fw_core.h` 移除 params/OPTCFG 声明与常量（保留 `fw_crc16_ccitt`） |
| 源码 | `main.c` 移除 4 个 `*_init()` 与 hall/optcfg 主循环分支；`interrupts_cw32l010.c` 移除 GPIOB 霍尔分支与 LPTIM OPTCFG 分支；`measure.c` 移除 optcfg/hall 依赖（`send_data_to_gateway` 不再延后，`go_to_sleep` 仅看 report/sample）；`fw_core.c` 移除退役纯逻辑 |
| 测试 | `host_sensor_core_test.c` 删除退役用例（−278/+10）、`build_test.sh` 不再链接 `history.c`、`host_measure_flow_check.c` 调整 |

## B. 退役完整性（静态，通过）

- `USER/src`、`USER/inc` 目录中已无任何退役文件（现仅 encrytogate/fw_core/gxht40/interrupts/light/main/measure/sf_i2c）。
- 全树检索 `hall_/optcfg_/OPTCFG/params_/history_/HALL_/OPT_IN_PIN/F2_PWR_EN`：**无任何代码引用**，仅 3 处注释说明“已随 ITEM-009 退役”。
- 工程文件（`MDK/Project.uvprojx`、`EWARM/*.ewp`、`gcc/build.sh`）中退役源文件引用数为 0；`gcc/build.sh` 以通配符编译 `USER/src/*.c`，退役后不再产生对应目标文件。
- ELF 中无 `hall_*`/`optcfg_*`/`params_*`/`history_*` 符号；`fw_core.o` 仅剩 `fw_crc16_ccitt` + 本次需求新增的 7 个纯函数（params/OPTCFG 纯逻辑已消失）。

## C. 中断分支移除（机器级，通过）

对编译产物反汇编核对（比源码审阅更硬）：

```
GPIOB_IRQHandler:
  38c2:  4770   bx lr            <-- 空处理，霍尔 EXTI 分支已彻底移除
LPTIM_IRQHandler 调用的函数集合:
  bl  <app_gtimer_count_irq>     <-- 仅 433 位时钟，OPTCFG 采样分支已移除
```

另外：全树已无 `GPIO_IT_FALLING/RISING/BOTH` 等 EXTI 配置（仅 `GPIO_IT_NONE`），也无 GPIOB 的 NVIC 使能。

## D. 整机引脚所有权唯一（通过）

逐条列出 USER/src 中**全部** GPIO 配置/驱动调用点：

| 端口 | 引脚 | 模块 | 用途 |
|---|---|---|---|
| GPIOB | PB05 | `light.c` | `GPIO_Init(OUTPUT_PP)` + `GPIO_WritePin`（采样高/空闲低） |
| GPIOB | PB04 | `light.c` | `GPIO_Init(ANALOG)`（AIN11） |
| GPIOA | PA03/PA04 | `measure.c` | 软 I²C SCL/SDA |
| GPIOA | PA05/PA06 | `measure.c` | 调试 UART（采样路径不配置，已由 ITEM-006/008 harness 验证 `UART_Init` 调用数 = 0） |

即：**PB04 仅作 AIN11 模拟输入、PB05 仅作光照供电输出、PB06 全树无任何引用（不驱动）**；除 light.c 外无任何模块碰触端口 B。此结果同时解除了 ITEM-004 登记的“整机引脚冲突”依赖（当时 hall/optcfg 仍会重配 PB04/PB05/PB06）。

## E. 构建与资源（通过）

| 项 | 结果 |
|---|---|
| GNU 交叉编译 | 0 error；28 warning（无新增） |
| 退役目标文件 | `gcc/obj` 中无 hall/optcfg/params/history 目标文件 |
| 资源 | FLASH 35,856→**32,208 B**（−3,648）；RAM 1,896→**1,712 B**（−184）；`.bin` MD5 `47abc6df…` |
| AC5（`--c99`，CMSIS 5.9.0） | `main.c`/`interrupts_cw32l010.c`/`measure.c`/`fw_core.c`/`gxht40.c`/`light.c`/`sf_i2c.c` 均 **0 error / 0 warning**；`encrytogate.c` 0 error（3 条既有未用静态量告警） |

## F. 回归（通过）

| 独立 harness | 结果 |
|---|---|
| `host_fw_core_verify_ev`（纯逻辑穷举） | 53/53 |
| `host_gxht40_verify_ev` | 42/42 |
| `host_light_verify_ev` | 45/45 |
| `host_measure_flow_verify_ev` | 79/79 |
| `host_rf_report_verify_ev` | 30/30（T6 已按退役更新） |
| `host_rf_frame_verify_ev`（传感器编码器↔网关解码器） | 10/10 |
| `host_rtc_cadence_verify_ev` | 17/17 |
| 实现者 `host_measure_flow_check.c` | 24/24 |
| 工程自带 `test/build_test.sh` | 2/2（见 G.1） |

## G. 观察与交接

1. **【交 ITEM-010】`test/build_test.sh` 目前只跑 2 项**（`host_sensor_core_test.c` 保留的 CRC16 回归）：退役时删除了 278 行旧用例，而本次需求新增的纯逻辑用例位于单独的 `host_fw_core_pure_check.c`（36 项）**未并入脚本**。任务清单已把“更新 `host_sensor_core_test.c` 与 `build_test.sh` 覆盖 CRC-8/换算/滞回/上报判定”列为 **ITEM-010**，故本轮不判缺陷；但需注意：TD-002 T-L1-01 对 `build_test.sh` 的覆盖期望要等 ITEM-010 完成后才成立。**本轮验证强度不受影响**：我自己的 `host_fw_core_verify_ev` 已对相同纯逻辑做了穷举比对（53 项）。
2. **【交 ITEM-011】MDK/IAR 源文件列表仍不完整**：`Project.uvprojx` 仅列 `main.c/interrupts_cw32l010.c/sf_i2c.c/measure.c/encrytogate.c`（缺 `fw_core.c`/`gxht40.c`/`light.c`），IAR 仅列 `main.c/interrupts_cw32l010.c`。因此**生产工具链链接会因缺符号失败**（`measure.c` 现在引用 `light_*`/`gxht40_*`/`sensor_decide_report`）。这属 ITEM-011 明确范围；退役文件本来就不在列表中，故 ITEM-009 未引入新缺口。本环境 Keil 另受 CMSIS 6.3.0/AC5 不兼容影响（ITEM-001 已登记），因此本轮以 GNU 交叉编译 + AC5 逐文件编译为构建证据。
3. **测试侧适配（非产品改动）**：为反映退役，我更新了 3 个自有 harness —— `host_rf_report_verify_ev` 的 T6 由“配置窗口延后发送”改为“无遗留窗口延后 + 休眠门控仅看 report/sample”；`host_measure_flow_verify_ev`/`host_rtc_cadence_verify_ev` 删除已不再被引用的退役桩。
4. **板级回归未做**：TD-002 T-L9-02（磁铁不产生唤醒、PB06 无输出）需实板 + 示波器；本轮无仪器，仅以源码/机器码证明 EXTI 与 PB06 驱动已不存在。

## H. ITEM-009 判定

hall/OPTCFG/params/history 已从源码、头文件、构建与工程引用中彻底退役；GPIOB 霍尔 EXTI 分支与 LPTIM OPTCFG 分支在**机器码层面**消失（空处理 / 仅 433 位时钟）；整机引脚所有权变为唯一（PB04 仅 AIN11、PB05 仅光照供电、PB06 不驱动）；资源明显下降且既有功能回归全部通过。

**判定：TEST_PASS**。

---

# ITEM-010 验证（宿主机 L0 测试套件更新）

验证对象：任务项 **ITEM-010**（更新 `test/host_sensor_core_test.c` 与 `test/build_test.sh`：覆盖 CRC-8 参考向量、换算边界（负温、0%/100% 截断）、光照阈值与滞回、上报判定边界（恰好 0.9 ℃、恰好 35.0 ℃、无前值、非暗、失败不更新）；移除已退役的 OPTCFG/params 用例；脚本在本机运行通过）
被测提交：`2ab13d8`；源码基线：`b6277fd`
结论：**TEST_PASS**

## A. 选测说明与变更范围

| 选测项 | 理由 |
|---|---|
| 变更范围审查 | `host_sensor_core_test.c` **+116/−23**（删退役用例、增本次需求纯逻辑用例）；`build_test.sh` **+10/−1**（改为两阶段入口） |
| 实际运行 | 脚本必须在本机通过且可重复（TD-002 T-L1-01） |
| **期望值独立性核对** | 防止“测试只自证实现”——用独立 Python 参考逐条复核测试中的每一个期望常量 |
| **门禁有效性** | 验证脚本在用例失败时确实返回非零（而不是永远绿） |
| 退役用例清理 | 确认无 OPTCFG/params/history/Manchester 残留 |

## B. 实际运行（通过）

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
CC=<mingw64 gcc> sh build_test.sh
```

| 阶段 | 内容 | 结果 |
|---|---|---|
| [1/2] | `host_sensor_core_test.c` + `fw_core.c`（纯逻辑） | **38 passed, 0 failed** |
| [2/2] | `host_measure_flow_check.c` + `measure.c`/`sf_i2c.c`/`fw_core.c` + `mock_measure_mcu`（采样/上报流程） | **24 passed, 0 failed** |
| 退出码 | `set -e` 两阶段均通过 | **0** |
| 编译告警 | 两次编译均为 `-Wall -Wextra` | **0 warning** |
| 重复运行 | 连续两次 | 结果一致（确定性） |

覆盖对照 TD-002：T-L0-06（CRC16 回归）→ 阶段 1 [1]；T-L0-01（CRC-8 参考向量/单字节/空长度）→ [2]；T-L0-02（温度换算与有效域，含负温与域外无效）→ [3]/[3b]；T-L0-03（湿度 0..1000 截断）→ [4]；T-L0-04（光照阈值与滞回）→ [5]；T-L0-05（上报边界：恰好 0.9 ℃、恰好 35.0 ℃、无前值、非暗、prev==cur）→ [6]；“**失败不更新前值/不构造 0 值**”属采样流程行为，由**阶段 2** 覆盖（[5]/[6]）。

## C. 期望值独立性核对（通过）

用独立 Python 参考（精确有理数 round-half-up + 从零写的 CRC + 直接按 IC-002 写的布尔式）逐条复核测试文件中的**全部期望常量**，30/30 一致：

| 组 | 复核内容 | 结果 |
|---|---|---|
| CRC16 | `"123456789"→0x29B1`、33 字节参考帧→`0xB540` | 一致 |
| CRC-8 | `{0xBE,0xEF}→0x92`、`{0x00}→0xAC`、空长度→`0xFF` | 一致 |
| 温度 | 1872→−400、16855→0、26214→250、63664→1250、0→−450、0xFFFF→1300 | 一致 |
| 湿度 | 0→0、2247→0、29360→500、55575→1000、0xFFFF→1000 | 一致 |
| 滞回 | 349/350（已明）、251/250（已暗）、0/4095、300（带内两种前态） | 一致 |
| 上报 | 无前值 200/351、下降 9/10/11、非暗、恰好 350、prev==cur | 一致 |

即：测试断言的是**需求/手册/契约推导出的正确值**，而非“把实现输出当期望”的自证式测试。

## D. 门禁有效性（通过，未修改仓库文件）

将 `test/` 与 `USER/`、`COMMON/`、`UM2005C/` 复制到临时目录，在副本中注入预期错误后运行 `build_test.sh`：

| 注入 | 退出码 | 输出 |
|---|---|---|
| 无（基线） | **0** | 38/0 与 24/0 |
| 阶段 1：CRC-8 期望 0x92→0x93 | **1** | `FAIL {0xBE,0xEF} -> 0x92: got 146 expect 147`，阶段 1 `37 passed, 1 failed`，且 `set -e` 使阶段 2 未执行 |
| 阶段 2：首个 `report_req` 期望反转 | **1** | 阶段 1 `38/0` 后阶段 2 `23 passed, 1 failed`，并打印对应 FAIL |
| 恢复后 | **0** | 38/0 与 24/0 |

证明该脚本是一个**真实门禁**（用例不满足即失败），且阶段间失败传播正确。

## E. 退役用例清理（通过）

- 测试文件与脚本中无 `params_*`/`optcfg_*`/`history_*`/`Manchester`/`decoder_t` 等退役用例（仅一处注释说明“ITEM-009 已移除 OPTCFG/params/history 用例”）。
- 脚本不再链接 `history.c`；仅依赖 `USER/inc`、`USER/src/{fw_core,measure,sf_i2c}.c`、`COMMON`、`UM2005C`、`test/mock_measure_mcu`（均已入库）。

## F. 观察与交接

1. **仓库卫生小项**：`build_test.sh` 运行后会在 `test/` 留下 `host_sensor_core_test.exe`、`host_measure_flow_check.exe`，而 `.gitignore` 未包含 `*.exe` → 会在 `git status` 中显示为未跟踪文件（本轮已清理）。建议后续把 `*.exe` 加入 `.gitignore` 或在脚本末尾清理；属卫生问题，不影响功能与判定。
2. `host_fw_core_pure_check.c`（36 项）与综合套件内容重叠但保留；当前 `build_test.sh` 只跑综合套件 + 流程套件。保留重叠无功能风险（互为交叉验证）。
3. 阶段 2 使用 `mock_measure_mcu` 阴影头（实现侧）；我方另有独立阴影头与独立 harness（`host_measure_flow_verify_ev` 79 项）作为交叉验证，两者结论一致。
4. 本轮完成后，TD-002 T-L1-01（“运行 `test/build_test.sh` 且覆盖新纯逻辑”）**已成立**（ITEM-009 登记的缺口已关闭）。

## G. ITEM-010 判定

`host_sensor_core_test.c` 已覆盖 CRC-8 参考向量、温度/湿度换算边界（含负温与 0/100% 截断）、光照阈值与滞回、上报判定边界（含恰好 0.9 ℃、恰好 35.0 ℃、无前值、非暗）；“失败不更新前值”由脚本阶段 2 覆盖；退役用例已彻底移除；`build_test.sh` 在本机两阶段 38+24 全部通过、退出码 0、零编译告警、可重复，且经注入验证为**真实门禁**；测试期望值经独立参考逐条确认正确。

**判定：TEST_PASS**。

---

# ITEM-011 验证（交叉编译与 MDK/IAR 工程源文件列表）

验证对象：任务项 **ITEM-011**（执行 `gcc/build.sh` 完成编译与链接并生成 elf/hex/bin；RAM/Flash 不超过 4 KB/64 KB；MDK/IAR 工程源文件列表与新增、移除的源文件保持一致）
被测提交：`eada262`；源码基线：`74f663f`
结论：**TEST_PASS**

## A. 变更范围

| 文件 | 变化 |
|---|---|
| `.gitignore` | **+1**：新增 `*.exe`（关闭 ITEM-010 登记的“测试脚本留下未跟踪 .exe”卫生项） |
| `EWARM/project.ewp` | **+64**：User 组补齐 8 个 `USER/src`；新增 UM2005C/COMMON 组与对应 include 路径；Driver 组补齐 adc/digitalsign/lptim/rtc/uart |
| `MDK/Project.uvprojx` | **+20**：User 组补 `fw_core.c`/`gxht40.c`/`light.c`；Driver 组补 `cw32l010_adc.c` |
| **源码/头文件** | **无任何改动**（`git show --numstat` 中 `USER/(src\|inc)/` 计数 = 0） |

## B. 交叉编译（通过）

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
sh gcc/build.sh
```

| 项 | 结果 |
|---|---|
| 退出码 | **0** |
| 编译/链接 | **0 error**；28 warning（全部为既有：encrytogate 未用静态量/缩进、main 模板参数与返回类型、sf_i2c maybe-uninitialized；**无一条指向 fw_core/gxht40/light/measure**） |
| 产物 | `gcc/obj/sensor_fw.elf`（156,508 B）、`sensor_fw.hex`（90,878 B）、`sensor_fw.bin`（32,292 B）均生成 |
| 目标 | Cortex-M0+（`-mcpu=cortex-m0plus -mthumb`） |

## C. 内存预算（通过）

| 区域 | 占用 | 限额 | 判定 |
|---|---|---|---|
| FLASH（text+data） | **32,208 B**（链接器报告 49.15%；`size` text 32,208 + data 84 = 32,292） | 64 KB = 65,536 B | **PASS**（余量约 50%） |
| RAM（data+bss） | **1,712 B**（链接器报告 41.80%；data 84 + bss 1,628） | 4 KB = 4,096 B | **PASS**（余量约 58%） |

## D. MDK/IAR 工程源文件列表一致性（通过，含功能完整性）

路径按各自工程目录解析（MDK 的 `FilePath` 相对于 `MDK/`，IAR 的 `$PROJ_DIR$` 相对于 `EWARM/`）。

| 检查 | 结果 |
|---|---|
| 条目数 | MDK 25、IAR 25 |
| **每条路径存在** | 全部存在（无悬空引用） |
| 两列表集合 | **完全相同**（唯一差异是工具链各自的 startup：`IdeSupport/MDK/startup_cw32l010.s` vs `IdeSupport/EWARM/startup_cw32l010.s`） |
| `USER/src` 一致性 | 实际 8 个 .c == 列表 8 个（encrytogate/fw_core/gxht40/interrupts/light/main/measure/sf_i2c） |
| 新增文件 | `fw_core.c`/`gxht40.c`/`light.c` 在**两个**列表中均存在 |
| 退役文件 | `hall/optcfg/params/history` 在列表中**零出现** |
| 新增依赖 | MDK 补 `cw32l010_adc.c`（light.c 需要）；IAR 补 `UM2005C`/`COMMON` 的 include 路径（gxht40.c 需要 `delay.h`，measure.c 需要 `app_um2005c.h`） |
| **功能完整性（链接验证）** | 用 GNU 工具链编译两列表的 25 个文件（工具链专用 startup 以 `gcc/startup_cw32l010.S` 等价替代）→ 均 **0 error**，链接**无未定义符号**；MDK 集合 FLASH 32,300 B / RAM 1,696 B，IAR 集合 FLASH 32,300 B / RAM 1,704 B |

即：列表不仅“名字对齐”，而且**足以完整链接出固件**（无缺文件）。

> 说明：Keil/IAR 专用 startup 使用 ARMASM / IAR 语法，无法用 GNU `as` 汇编（报 `bad instruction`），这是**预期**而非缺陷；`gcc/build.sh` 使用 `gcc/startup_cw32l010.S`。

## E. .gitignore 与仓库卫生（通过）

运行 `test/build_test.sh`（38/38 + 24/24，exit 0）后 `git status` **为空**——`*.exe` 已入 `.gitignore`，ITEM-010 登记的卫生项已关闭。

## F. 回归（通过）

| 项 | 结果 |
|---|---|
| `test/build_test.sh` | 38/38（纯逻辑）+ 24/24（采样/上报流程），exit 0 |
| 独立 harness 抽查（本项未改源码，结论沿用） | fw_core 53/53、measure 流程 79/79、空口帧互操作 10/10 |

## G. 观察与交接

1. **两套构建的编译集合不完全相同（非缺陷）**：`gcc/build.sh` 以通配符编译 `Libraries/src/*.c`（**23 个**），而 MDK/IAR 只列实际使用的 **9 个**库文件；未使用的库代码在 GNU 链接时被 `--gc-sections` 丢弃，因此内存数字仍只反映实际使用代码。验收要求的是“工程列表与新增/移除的源文件一致”，项目自有源码集合（USER/src/UM2005C/COMMON）两列表完全一致，已满足；此差异仅记录备查。
2. **Keil 全量构建仍不可用**：本机 CMSIS 解析到 6.3.0 与 AC5 不兼容（ITEM-001 已登记定位），`mdk_build` 会报 `Unknown compiler`；因此 `mdk_flash`/`mdk_build` 不能作为构建证据（无 Verify OK 不得判成功）。本轮构建证据 = GNU 交叉编译 + AC5 逐文件编译（ITEM-009/010 已做）+ 上述“列表功能完整性链接”。
3. **板级验证未做**：烧录/运行/串口/波形观测本轮无设备确认（无 SWD 探针枚举、无实测环境），属 TD-002 T-L2/L4/L8 范围。

## H. ITEM-011 判定

`gcc/build.sh` 编译链接通过并生成 elf/hex/bin，FLASH 32,208 B（≤ 64 KB）与 RAM 1,712 B（≤ 4 KB）均在预算内；MDK 与 IAR 工程源文件列表已补齐新增源文件、无退役残留、路径全部存在、与 `USER/src` 实际集合一致，且两列表均可完整链接（零未定义符号）；`.gitignore` 补齐 `*.exe`。

**判定：TEST_PASS**。

---

# ITEM-012 验证（网关 CH592 beiwov2 兼容性核对）

验证对象：任务项 **ITEM-012**（确认 `decode_frame10` 取 `temp=p[4..5]`、`hum=p[6..7]`；BLE 每设备 8 字节记录为 `id|hum_be|temp_be` 且与 Android `parsePlainFrame` 的 `u16be@4`/`s16be@6` 一致；一致则不改代码并记录结论）
被测提交：`6769d79`（仅改传感器侧测试与文档；**未改网关/Android**）
结论：**一致，无需修改；TEST_PASS**

## A. 选测说明

本项是**只读跨组件核对**（网关 CH592 固件 + Android 解析），不属传感器固件改动。本轮采用三种独立手段：
1. **逐段源码核对**（每个字节操作的出处）；
2. **独立端到端 harness**：链接**真实传感器编码器**与**真实网关解码器**，并忠实复刻网关的组帧/打包与 Android 的解析代码，跑完整链路；
3. **变更历史核对**（确认网关/Android 未被本工作流改动）。

## B. 逐段源码核对（通过）

| 环节 | 源码位置 | 实际行为 |
|---|---|---|
| 传感器组帧 | `USER/src/encrytogate.c:encode_frame10` | `uid[0..3] | temp_LE(2) | hum_LE(2) | crc16_LE(2)`，8 轮 Feistel |
| 空口 | 433 MHz 10 B | 不变（IC-002 §3） |
| 网关解密 | `APP/feistel_al.c:decode_frame10` | `uid=p[0..3]`；**`temp = p[4] | p[5]<<8`（小端 int16）；`hum = p[6] | p[7]<<8`（小端 uint16）**；crc16 校验覆盖 p[0..7] |
| 网关组 resbf | `APP/app_um2006A.c:169..172` | `resbf[4..5]=temp 大端`、`resbf[6..7]=hum 大端` |
| **网关记录交换** | `APP/app_um2006A.c:192..195` | `sensorres[4..5]=resbf[6..7]`（湿度大端）、`sensorres[6..7]=resbf[4..5]`（温度大端）→ **每设备 8 B = `id | hum_be | temp_be`** |
| 设备归因 | `APP/app_um2006A.c:180` | 仅当 `bind_device_id[i][0..3] == resbf[0..3]` 才写入该设备槽 → 按 ID 归因，不靠顺序 |
| 打包 | `APP/bleencrypt.c` | `FRAME_BLOCK_SIZE=16`、`DEV_PAYLOAD_LEN=8`；头部块 `gwid6[0..5] + devCount[6] + pad9`；设备块 `payload8` 原样置于 `out16[0..7]` + pad8；帧长 `16*(1+devCount)` |
| BLE 加密 | `APP/app_um2006A.c:71..79, 209` | `encrypt_frame_ecb_inplace(..., APP_AES_KEY16)`，按 16 B 块 `LL_Encrypt`（AES-128 ECB），再 `SimpleProfile_SetParameter(SIMPLEPROFILE_CHAR2,...)` |
| Android 解密 | `MqtttService.kt:501` | `Cipher.getInstance("AES/ECB/NoPadding")`，要求密文 16 B 对齐 |
| Android 解析 | `MqtttService.kt:parsePlainFrame` | `gwid6=plain[0..5]`、`devCount=plain[6]`、`expected=16*(1+devCount)`、`off` 从 16 起步进 16、`p8=block[0..7]`、**`humi=u16be(p8,4)`、`temp=s16be(p8,6)`** |

逐段对应：传感器 `temp/hum` 小端 → 网关解密后转大端存入 resbf → 记录内交换为“湿度在前” → Android 按 `u16be@4`（湿度）/`s16be@6`（温度）读取。**全链一致。**

关键常量独立比对：**AES 密钥 16 字节与 Android 逐字节相同**（`91 4E 03 B7 C2 5A 88 1D F4 60 7B 2E A9 17 6C 55`）；帧长规则、记录 8 B、块 16 B 两边一致；网关无对自身温度历史的二次过滤（`tempervalue`/`humivalue` 仅用于组帧）。

## C. 独立端到端 harness（通过）

链接**真实** `encrytogate.c` + **真实** `feistel_al.c`，并**逐字复刻**网关 `bleencrypt.c` 的 `build_header_block`/`build_device_block`/`build_padded_frame_blocks`、`app_um2006A.c` 的记录交换，以及 Android 的 `parsePlainFrame`/`u16be`/`s16be`。

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
<mingw64 gcc> -std=c11 -Wall -Wextra -Wno-unused-function -Wno-unused-const-variable \
  -Wno-misleading-indentation -I ../USER/inc -I <gateway APP> host_gw_compat_verify_ev.c \
  ../USER/src/encrytogate.c <gateway APP>/feistel_al.c -o gw_compat_verify && ./gw_compat_verify
=> ==== result: 11 passed, 0 failed ====
```

| 组 | 独立断言 | 结果 |
|---|---|---|
| 全链向量 | 8 组值（含 −100/−400/1250/351/0/289/350 ℃ 与 0/7/100/500/999/1000 %RH）经“传感器编码 → 网关解密 → 网关记录 → 打包 → Android 解析”后 id/温度/湿度均一致 | PASS |
| 多设备 | 4 设备一帧：帧长 = 16*(1+devCount)、头部 `gwid6[0..5]`+`devCount[6]`、每设备 id/温/湿归因正确、设备块前 8 B 与记录逐字节相同 | PASS ×5 |
| **负对照** | 故意按旧顺序 `id|temp_be|hum_be` 组记录 → Android 读出错位值（未通过）→ 证明上述正向结论非空验 | PASS |
| 帧长规则 | devCount 0..5 时网关 `16*(1+N)` == Android `expected` | PASS |
| ID 透传 | 空口 4 B == 编码器 `uid[0..3]` == 记录 4 B（网关原样拷贝，不改序） | PASS ×3 |

实现者自带 harness 也被独立执行确认（需额外链接网关 `bleencrypt.c`，见 G.1）：`host_rf_frame_check.c` → **22 passed, 0 failed**。

## D. 变更历史核对（通过）

| 对象 | 结论 |
|---|---|
| 网关 `CH592EVT/.../beiwov2/` | 仅 `c3baaaa`（**本工作流基线 `b32ea8a` 之前**）修改过 `app_um2006A.c` 的记录顺序（交换为 `id|hum_be|temp_be`，与 IC-001/IC-002 一致）；自 `b32ea8a` 起的本工作流**未触碰**网关任何文件 |
| Android `dengbei_care/` | 自基线导入 `cc6c6f7` 以来**零改动** |
| 本项实现提交 | `6769d79` 仅改传感器侧 `test/host_rf_frame_check.c` 与文档，**无网关/Android 改动**——符合“一致则不改代码”的任务要求 |

## E. 观察与交接

1. **实现者证据的复现命令缺一个源文件**：`evidence/protocol_test.md` 的 ITEM-012 命令未包含网关 `bleencrypt.c`，按原文链接会报 `undefined reference to build_device_block / build_padded_frame_blocks`（我已实测复现）；补上后 22/22 通过。属**证据文档级**问题（非产品缺陷），建议补全命令。
2. **既有“超时无传感器”路径会发布全 0 记录**：网关在 `timeoutcnt` 用尽后 `memset(sensorres,0,...)` 并发布（`id=00000000`）。这是**基线已有、本工作流未改动**的行为；因零 ID 不匹配任何绑定设备，不会把真实传感器数据错配。但 IC-002 §5 提到“不将未收到数据解释为零值”——建议 App/服务器侧确认对零 ID 记录的现有处理（登记为产品级备注，非本项缺陷）。
3. **设计文档措辞与实际 ID 取值不同（无功能影响）**：FD-002 §8.1 写“`mcu_uid` 取 `ptr[0],[3],[6],[8]`”，而 `encrytogate.c` 的 `UID_IDX={0,1,2,3}` 使空口 ID 实际为 `uid[0..3]`（`measure.c` 里写入 `send_data[0..3]` 的代码被 `encode_frame10` 覆盖，ITEM-008 已证为死代码）。网关与 Android 均原样透传这 4 字节，**功能一致**；建议更正设计文档措辞。
4. **真实射频/BLE 空中链路未实测**：本项为软件级一致性核对；调制/前导/空中速率/丢包、BLE 实际收发与 App 读数需真实硬件（TD-002 T-L7、T-L2）。

## F. ITEM-012 判定

`decode_frame10` 的 `temp=p[4..5]`/`hum=p[6..7]` 与 BLE 每设备 `id|hum_be|temp_be`、Android `u16be@4`/`s16be@6` **逐段一致**；AES 密钥与 ECB 块处理一致；网关无二次过滤；网关与 Android 自本工作流基线以来未被修改，且本项实现未改其代码——**符合“一致则不改代码并记录结论”**。

**判定：TEST_PASS**。
