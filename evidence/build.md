# 构建证据（BUILD-002 rev 5.5）

状态：固件实现证据（firmware_engineer.firmware_implementation）
本轮范围：run8 **ITEM-002 复验修复四**（用户重焊 U9 后重做授权；上电一次性 I²C general call 复位尝试 `0x00`+`0x06` 与 `gc` 可观测字段）
依据：FD-002 rev 5.0、FWR-002 rev 5.0（FWR-116/118）、TD-002 rev 5.0；`gxht40.pdf` §7.7；触发 `evidence/test.md` EV-015 与 operator_update（重焊授权）
受测提交：`5b07982`（RESUME_SYNC 接手时工作区干净）+ 本轮改动
测试环境：GNU 交叉编译（`arm-none-eabi-gcc`，Cortex-M0+）与 Keil MDK（ARMCLANG V6.24，`mdk_build`）；`mdk_flash`/串口不在本调用工具列表内。

---

## ITEM-002 复验修复四：交叉编译 + Keil MDK 构建（本轮实际执行）

```
$ sh gcc/build.sh
Memory region         Used Size  Region Size  %age Used
           FLASH:       34120 B        64 KB     52.06%
             RAM:        1808 B         4 KB     44.14%
== done: obj/sensor_fw.elf/.hex/.bin ==
(exit 0；告警 27 条，均为既有类别；无一条指向本轮 measure.c/debug_trace.c 新增代码)

$ md5sum gcc/obj/sensor_fw.elf gcc/obj/sensor_fw.bin
9533f40d746fc9e42427dd72a63577ad  gcc/obj/sensor_fw.elf
a092df240b773cbfbb0cfc902128c1b5  gcc/obj/sensor_fw.bin

$ mdk_build {"action":"rebuild"}   (授权工具，经 hardware-verification MCP/CLI)
*** Using Compiler 'V6.24' ...
../USER/src/main.c(238): warning: while loop has empty body [-Wempty-body]   (既有 `while(k--);`)
Program Size: Code=19144 RO-data=644 RW-data=116 ZI-data=1676
".\output\exe\Project.axf" - 0 Error(s), 1 Warning(s).

$ md5sum MDK/output/exe/Project.axf
ff49029608f3315ee76964c131d06821  MDK/output/exe/Project.axf

$ arm-none-eabi-strings MDK/output/exe/Project.axf | grep -E "gc=|IOSIG|IOTEST"
@IOSIG scl=
@IOTEST sda_lo=
 gc=
(证明 general call 恢复尝试的 gc 字段已编入本轮交付件)

$ arm-none-eabi-gcc -mcpu=cortex-m0plus -O1 -Wall -Wextra -DSENSOR_DEBUG_UART=0 -c ... USER/src/{measure,debug_trace}.c
(0 告警 0 错误)
```

- 相对上一轮（GNU 34,004 B / RAM 1,808 B；Keil Code=18,888 RO=644 RW=116，axf md5 `5c2fdb58…`）：GNU **+116 B**；Keil **Code +256 B**（新增 general call 探测/命令写入分支与 `gc` 渲染 + 三个配置常量）；RAM 不变。均在预算内（Code+RO ≈ 19.8 KB / 64 KB；RAM ≈ 1.79 KB / 4 KB）。
- **未执行**：`mdk_flash` 与 COM42 采集（本调用工具列表仅含 `mdk_build`）⇒ 实板 `gc` 读数与 `q=1` 复判均未取得（由嵌入式测试能力执行）。
- 工具在仓库根产生的 `build*.log` 已读入本证据后删除（`.gitignore` 的 `/build*.log` 覆盖）。

---

# 历史：ITEM-002 复验修复三（E1b IOSIG）

```
$ sh gcc/build.sh
Memory region         Used Size  Region Size  %age Used
           FLASH:       34004 B        64 KB     51.89%
             RAM:        1808 B         4 KB     44.14%
== done: obj/sensor_fw.elf/.hex/.bin ==
(exit 0；告警 27 条，均为既有类别；无一条指向本轮 measure.c/debug_trace.c 新增代码)

$ md5sum gcc/obj/sensor_fw.elf gcc/obj/sensor_fw.bin
d4543806318f8ec86c6e9b0f40cafd7e  gcc/obj/sensor_fw.elf
16633799923f8c2a7b4f1eea9d9d906a  gcc/obj/sensor_fw.bin

$ mdk_build {"action":"rebuild"}   (授权工具，经 hardware-verification MCP/CLI)
*** Using Compiler 'V6.24' ...
../USER/src/main.c(238): warning: while loop has empty body [-Wempty-body]   (既有 `while(k--);`)
Program Size: Code=18888 RO-data=644 RW-data=116 ZI-data=1676
".\output\exe\Project.axf" - 0 Error(s), 1 Warning(s).

$ md5sum MDK/output/exe/Project.axf
5c2fdb58d0fc4ef7dac8a21e343082a3  MDK/output/exe/Project.axf

$ arm-none-eabi-strings MDK/output/exe/Project.axf | grep -E "IOSIG|IOTEST"
@IOSIG scl=
@IOTEST sda_lo=
(证明 IOSIG/IOTEST 诊断行确实编入本轮交付件)

$ arm-none-eabi-gcc -mcpu=cortex-m0plus -O1 -Wall -Wextra -DSENSOR_DEBUG_UART=0 -c ... USER/src/{measure,debug_trace,main}.c
(measure.c / debug_trace.c：0 告警 0 错误; main.c：仅既有告警)
```

- 相对上一轮（GNU 33,564 B / RAM 1,808 B；Keil Code=17,808 RO=636 RW=116，axf md5 `bcfc14dc…`）：GNU **+440 B**、Keil **Code +1,080 B / RO +8 B**（新增 `sensor_io_sig_scan` 20 个采样点的位序列与签名拼装、`debug_trace_iosig` 渲染、`IOSIG` 字面；ARMCLANG 不合并等价路径，故增量大于 GNU）；RAM 不变。均在预算内（Code+RO ≈ 19.5 KB / 64 KB；RAM ≈ 1.79 KB / 4 KB）。
- **告警行号更正**：上轮 `evidence/build.md` 写 `main.c(224)`；Tester 在 EV-012 §1 指出实际为 231（行号随新增代码位移）。本轮如实记录为 `main.c(238)`（同一既有告警）。
- **未执行**：`mdk_flash` 与 COM42 采集（本调用工具列表仅含 `mdk_build`）⇒ `IOSIG` 实板读数与 `q=1` 复判均未取得。
- 工具在仓库根产生的 `build*.log` 已读入本证据后删除（`.gitignore` 的 `/build*.log` 覆盖）。

---

# 历史：ITEM-002 复验修复二（E1）

```
$ sh gcc/build.sh
Memory region         Used Size  Region Size  %age Used
           FLASH:       33564 B        64 KB     51.21%
             RAM:        1808 B         4 KB     44.14%
== done: obj/sensor_fw.elf/.hex/.bin ==
(exit 0；告警 27 条，均为既有类别；无一条指向本轮 measure.c/debug_trace.c 新增代码)

$ md5sum gcc/obj/sensor_fw.elf gcc/obj/sensor_fw.bin
c166e26384dffae6968acdf4d9191ec1  gcc/obj/sensor_fw.elf
fb6196f666dcb4e658a9a8c63ee3365c  gcc/obj/sensor_fw.bin

$ mdk_build {"action":"rebuild"}   (授权工具，经 hardware-verification MCP/CLI)
*** Using Compiler 'V6.24' ...
../USER/src/main.c(224): warning: while loop has empty body [-Wempty-body]   (既有 `while(k--);`)
Program Size: Code=17808 RO-data=636 RW-data=116 ZI-data=1676
".\output\exe\Project.axf" - 0 Error(s), 1 Warning(s).

$ md5sum MDK/output/exe/Project.axf
bcfc14dc27ee9afeb02df2e0e7b4b586  MDK/output/exe/Project.axf

$ arm-none-eabi-strings MDK/output/exe/Project.axf | grep -E "IOTEST|swap=|sda_lo"
@IOTEST sda_lo=
 swap=
(证明 IOTEST 诊断行确实编入本轮交付件)

$ arm-none-eabi-gcc -mcpu=cortex-m0plus -O1 -Wall -Wextra -DSENSOR_DEBUG_UART=0 -c ... USER/src/{measure,debug_trace,main}.c
(measure.c / debug_trace.c：0 告警 0 错误; main.c：仅既有告警)
```

- 相对上一轮（GNU 33,076 B / RAM 1,768 B；Keil Code=16,572，axf md5 `e309bac2…`）：GNU **+488 B / +40 B**（新增 `sensor_io_diag_scan` 204 B、`debug_trace_iotest` 168 B、PA03 开漏输出/影子设备等；`i2c0_swap_dev` 占 40 B RW-data）；Keil Code **+1,236 B**（同一批新增代码，ARMCLANG 不合并等价路径，故增量大于 GNU）；RAM/Flash 均在预算内（Code+RO ≈ 18.4 KB / 64 KB，RAM ≈ 1.79 KB / 4 KB）。
- **未执行**：`mdk_flash` 与 COM42 采集（本调用工具列表仅含 `mdk_build`）⇒ `IOTEST` 实板读数、`q=1` 复判均未取得，由嵌入式测试能力执行。
- 工具在仓库根产生的 `build*.log` 已读入本证据后删除（`.gitignore` 的 `/build*.log` 覆盖）。

---

# 历史：ITEM-002 复验修复一（FWR-118 + OBS-2）

```
$ sh gcc/build.sh
Memory region         Used Size  Region Size  %age Used
           FLASH:       33076 B        64 KB     50.47%
             RAM:        1768 B         4 KB     43.16%
== done: obj/sensor_fw.elf/.hex/.bin ==
(exit 0；告警 27 条，均为既有类别，无一条指向本轮 gxht40.c 改动)

$ md5sum gcc/obj/sensor_fw.elf gcc/obj/sensor_fw.bin
23cb6f6f4e269627631eaf0bbc5b5b89  gcc/obj/sensor_fw.elf
c6d132146afadba963336b3d6537fe6c  gcc/obj/sensor_fw.bin   (33,164 B)

$ mdk_build {"action":"rebuild"}   (授权工具，经 hardware-verification MCP/CLI)
*** Using Compiler 'V6.24' ...
../USER/src/main.c(224): warning: while loop has empty body [-Wempty-body]   (既有 `while(k--);`)
Program Size: Code=16572 RO-data=624 RW-data=76 ZI-data=1676
".\output\exe\Project.axf" - 0 Error(s), 1 Warning(s).

$ md5sum MDK/output/exe/Project.axf
e309bac2a34fd210ee966d6da9001625  MDK/output/exe/Project.axf

$ arm-none-eabi-gcc -mcpu=cortex-m0plus -O1 -Wall -Wextra -DSENSOR_DEBUG_UART=0 -c ... USER/src/gxht40.c
(0 告警 0 错误)
```

- 相对上一轮（GNU 33,016 B；Keil Code=16,488，axf md5 `196b0040…`）：GNU **+60 B**、Keil **Code +84 B**（新增：CRC/量程分支清缓存、0x45 局部帧缓冲与双候选完整重探、结果码一致性收紧）；RAM 不变。仍在 64 KB/4 KB 预算内。
- **未执行**：`mdk_flash`（部署/回读校验）与 COM42 采集 —— 本调用工具列表仅含 `mdk_build`。实板 `q=1` 仍受限于器件地址级不应答（EV-009 §3.1/§5），需现场动作。
- 工具在仓库根产生的 `build*.log` 已读入本证据后删除（`.gitignore` 的 `/build*.log` 已覆盖）。

---

# 历史：ITEM-002 首轮（已按 EV-009 判定修复，见上）

```
$ sh gcc/build.sh
Memory region         Used Size  Region Size  %age Used
           FLASH:       33016 B        64 KB     50.38%
             RAM:        1768 B         4 KB     43.16%
== done: obj/sensor_fw.elf/.hex/.bin ==
(exit 0；告警 27 条，均为既有类别，无一条指向本轮改动的 gxht40.c / sf_i2c.c 新增函数)

$ md5sum gcc/obj/sensor_fw.elf gcc/obj/sensor_fw.bin
b90246dd7bdf82047c38cc362fab46fe  gcc/obj/sensor_fw.elf
11558a770775b92adb7191cca4496c04  gcc/obj/sensor_fw.bin   (33,104 B)

$ mdk_build {"action":"rebuild"}   (授权工具，经 hardware-verification MCP/CLI)
*** Using Compiler 'V6.24' ...
compiling gxht40.c / sf_i2c.c / measure.c / debug_trace.c / main.c ...
../USER/src/main.c(224): warning: while loop has empty body [-Wempty-body]   (既有 `while(k--);`)
Program Size: Code=16488 RO-data=624 RW-data=76 ZI-data=1676
".\output\exe\Project.axf" - 0 Error(s), 1 Warning(s).

$ md5sum MDK/output/exe/Project.axf
196b0040d5378eaf2edcd6a3d5fdf9a5  MDK/output/exe/Project.axf
```

- 相对 ITEM-001（GNU 32,840 B / RAM 1,768 B；Keil Code=16,316 ZI=1,676）：GNU **+176 B**、Keil **Code +172 B**（新增 `i2c_bus_recover`（含 9 脉冲与 STOP）与 `gxht40_acquire` 重探分支、首访 tPU 余量）；RAM 不变。仍远在预算内。
- **`SENSOR_DEBUG_UART=0` 编译路径复核**（`-mcpu=cortex-m0plus -O1 -Wall -Wextra -DSENSOR_DEBUG_UART=0`）：`gxht40.c` / `measure.c` **0 告警 0 错误**（本轮改动与调试开关无关）；`sf_i2c.c` 仅 `i2c_write_multi_byte`/`i2c_write_multi_byte_16bit` 的**改动前既有** `err may be used uninitialized`（未触碰的旧函数）。默认（ARM，`SENSOR_DEBUG_UART=1`）`gxht40.c` 0 告警。
- **未执行**：`mdk_flash`（部署/回读校验）与 COM42 采集 —— 本调用工具列表仅含 `mdk_build`。T2 的实板验收（每周期 `S` 行 `q=1`、`t/h` 在有效域且与环境相符、上报时手机显示一致）由嵌入式测试能力执行（TD-002 §4.4 T-L2-14）；若仍 `ack=none`，按 `evidence/test.md` EV-008 §3 的一手结论交现场硬件核对。
- 工具在仓库根产生的 `build*.log` 已读入本证据后删除；`.gitignore` 已由 `/build.log` 扩为 `/build*.log`（覆盖实测变体）。

---

# 历史：本轮 run8 ITEM-001（T1 诊断可观测）

```
$ sh gcc/build.sh
(编译全部 USER/COMMON/UM2005C/Libraries 源 + 链接)
Memory region         Used Size  Region Size  %age Used
           FLASH:       32840 B        64 KB     50.11%
             RAM:        1768 B         4 KB     43.16%
== done: obj/sensor_fw.elf/.hex/.bin ==
(exit 0；告警 27 条，均为既有类别：main.c 既有 `int32_t main`/未用形参/空体 while、sf_i2c 既有
 `i2c_write_multi_byte*` 的 `err may be used uninitialized`、COMMON/convert.c、UM2005C/app_gtimer.c；
 **无一条指向本轮新增的 gxht40.c 诊断/getter、debug_trace.c BUS/G 行、measure.c 扫描与幂等初始化**)

$ md5sum gcc/obj/sensor_fw.elf gcc/obj/sensor_fw.bin
5020cba32f64df4299a9879224752651  gcc/obj/sensor_fw.elf
4ef1675d725ae1ffab189e49e052b2c4  gcc/obj/sensor_fw.bin   (32,924 B)

$ mdk_build {"action":"rebuild"}   (授权工具，经 hardware-verification MCP/CLI)
*** Using Compiler 'V6.24' ...
compiling measure.c / debug_trace.c / gxht40.c / sf_i2c.c / main.c ...
../USER/src/main.c(224): warning: while loop has empty body [-Wempty-body]   (既有 `while(k--);`)
Program Size: Code=16316 RO-data=624 RW-data=76 ZI-data=1676
".\output\exe\Project.axf" - 0 Error(s), 1 Warning(s).

$ md5sum MDK/output/exe/Project.axf
c39bf314851a0988a5f347c73604d105  MDK/output/exe/Project.axf
```

- 相对本轮基线 `1dd8e4b` 的 run7 T2 快照（GNU FLASH 31,952 B / RAM 1,736 B；Keil Code=13,976 ZI=1,652）：
  GNU **+888 B / +32 B**，Keil **Code +2,340 B / ZI +24 B**。增量来自新增地址探测原语、有界地址扫描、BUS/G 行格式化、
  GXHT40 诊断快照与 getter（Keil 侧优化等级较 GNU 保守，故增量更大）；仍远在 64 KB/4 KB 预算内（Code+RO ≈ 16.9 KB，RAM 43%）。
- 目标镜像字符串核验（证明诊断行确实编入**本轮交付件**，而非宿主自检）：

```
$ arm-none-eabi-strings MDK/output/exe/Project.axf | grep -E "BOOT|BUS idle| ack=| raw=|"
@BOOT fw=FD-002r4 uid=
@BUS idle=
 ack=
 raw=
```

- **`SENSOR_DEBUG_UART=0` 编译路径复核**（逐个编译本轮触碰的 TU，`-mcpu=cortex-m0plus -O1 -Wall -Wextra -DSENSOR_DEBUG_UART=0`）：
  `measure.c` / `debug_trace.c` / `gxht40.c` **0 告警 0 错误**（扫描实现被 `#if SENSOR_DEBUG_UART` 编译为空实现，无 UART/I2C 探测依赖）；
  `main.c` 与 `sf_i2c.c` 仅上文既有告警。ARM 默认（不传宏 → `__arm__` 判为 1）：`measure.c` / `debug_trace.c` 0 告警。
- **未执行**：`mdk_flash`（部署/回读校验）与 COM42 采集 —— 本调用工具列表仅含 `mdk_build`。真实目标上的
  `BUS` 行、失败周期 `S`+`G` 行观测与总线身份核对由嵌入式测试能力执行（`evidence/test.md` / TD-002 §4.4 T-L2-11/12）。
- 工具在仓库根产生的 `build.log` 已读入本证据后删除；本轮已将 `/build.log` 加入 `.gitignore`（避免后续构建工具在共享仓库根留下未跟踪文件）。

---

# 历史：本轮 run7 T2（ITEM-002 光照 1/3 判据）

```
$ sh gcc/build.sh
Memory region         Used Size  Region Size  %age Used
           FLASH:       31952 B        64 KB     48.75%
             RAM:        1736 B         4 KB     42.38%
(exit 0；27 条告警，均为既有厂商库/既有代码，无一条指向 light.c/fw_core.c/measure.c/main.c 本轮改动)

$ mdk_build {"action":"rebuild"}
*** Using Compiler 'V6.24' ...
compiling light.c / fw_core.c / measure.c / debug_trace.c ...
Program Size: Code=13976 RO-data=620 RW-data=76 ZI-data=1652
".\output\exe\Project.axf" - 0 Error(s), 1 Warning(s).
（唯一告警为 main.c 既有空体 `while(k--);`）

$ md5sum MDK/output/exe/Project.axf
3ee3830814ec61384785998556a4461e  MDK/output/exe/Project.axf
```

- 相对 T1（FLASH 32,020 B / Code=14048）：FLASH **31,952 B**、Code **13,976**（−72/−72 B，删滞回/回退并改用结构化结果）。
- 本提交**未标定**：`LIGHT_DARK_CALIBRATED=0`、`LIGHT_DARK_REF_CODE=0`（占位），属设计允许的中间状态，**不得**当作无光判定已验收。
- **未执行**：`mdk_flash`（部署/回读校验）与 COM42 采集 → 全暗基准 `C_dark` 实板标定与三态观测未执行（本项返回 BLOCKED 的原因）。
- 工具在仓库根产生的 `build.log` 已读入本证据后删除。

---

# 历史：本轮 run7 T1（ITEM-001 UART1 调试串口）

## T1（run7 ITEM-001）：交叉编译 + Keil MDK 构建（已 TEST_PASS，EV-007）

### 1. GNU 交叉编译（`gcc/build.sh`）

```
$ cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
$ sh gcc/build.sh
== compile ==
== link ==
Memory region         Used Size  Region Size  %age Used
           FLASH:       32000 B        64 KB     48.83%
             RAM:        1736 B         4 KB     42.38%
== done: obj/sensor_fw.elf/.hex/.bin ==
(exit 0)

$ arm-none-eabi-size gcc/obj/sensor_fw.elf
   text    data     bss     dec     hex filename
  32000      84    1652   33736    83c8 gcc/obj/sensor_fw.elf
```

- **0 错误**，共 27 条告警，全部为既有厂商库/既有代码（`cw32l010_iwdt/uart/spi/i2c/gtim/btim`、`COMMON/convert.c`、`UM2005C/app_gtimer.c`、`encrytogate.c`、`sf_i2c.c`、`main.c` 既有 3 条），**无一条指向 `debug_trace.c`/`measure.c`/`light.c`**。
- 相对 run6 基线（FLASH 32,208 B / RAM 1,712 B）：FLASH −208 B（移除旧 UART 配置与 printf 重定向），RAM +24 B（轨迹快照 20 B + 统计 4 B）。均在预算内。
- T1 涉及的翻译单元无浮点引用：

```
$ for o in debug_trace light measure main fw_core gxht40 sf_i2c; do
    echo -n "$o: "; arm-none-eabi-nm gcc/obj/$o.o | grep -cE "__aeabi_[df]|__float"; done
debug_trace: 0
light: 0
measure: 0
main: 0
fw_core: 0
gxht40: 0
sf_i2c: 0
```

### 2. Keil MDK / ARMCLANG（`mdk_build {"action":"rebuild"}`）

```
*** Using Compiler 'V6.24', folder: 'C:\Keil_v5\ARM\ARMCLANG\Bin'
Rebuild target 'Project'
...
compiling debug_trace.c...
...
Program Size: Code=14000 RO-data=620 RW-data=76 ZI-data=1644
".\output\exe\Project.axf" - 0 Error(s), 1 Warning(s).
Build Time Elapsed:  00:00:03
```

- **0 Error / 1 Warning**：唯一告警为 `main.c:209 while loop has empty body`（既有 `while(k--);`，非本轮新增）。`debug_trace.c` 已纳入构建。
- 产物 `MDK/output/exe/Project.axf`（md5 `f4aec36c80576958dfbc331c3da350cc`）含字符串 `BOOT fw=FD-002r4 uid=`，证明**Keil 目标构建下 `SENSOR_DEBUG_UART=1`**（宿主机默认 0，需显式 `-DSENSOR_DEBUG_UART=1`）。
- 工具在仓库根产生的 `build.log` 已读入本证据后删除，未入库。

### 3. 未执行（如实记录，不当作通过）

- `mdk_flash`（编译+下载+回读校验）与 COM42 轨迹采集未在本调用工具列表内 → TD-002 §4.4 T-L2-01..09 的真实目标观测**未执行**，由嵌入式测试能力执行。
- 下载校验成功也不等于业务功能通过；本轮不以构建/下载代替实板观测。

### 4. EV-006 缺陷修复后的重建（本轮）

修复点：`debug_trace.c` 排空由 `TC` 改为「等 `TXE` → 等 `TXBUSY` 清零」（D-ITEM001-1）；`main.c` 在 `main()` 首条语句锁存复位标志（D-ITEM001-2）。重建原始结果：

```
$ sh gcc/build.sh
Memory region         Used Size  Region Size  %age Used
           FLASH:       32020 B        64 KB     48.86%
             RAM:        1736 B         4 KB     42.38%
(exit 0; 27 warnings, 无一条指向 debug_trace/measure/light/main 新增代码)

$ mdk_build {"action":"rebuild"}
*** Using Compiler 'V6.24' ...
compiling debug_trace.c...
Program Size: Code=14048 RO-data=620 RW-data=76 ZI-data=1644
".\output\exe\Project.axf" - 0 Error(s), 1 Warning(s).
（唯一告警仍为 main.c 既有空体 `while(k--);`）

$ md5sum MDK/output/exe/Project.axf
956dd6790bf11ff8cb584f584bc006d2  MDK/output/exe/Project.axf

$ arm-none-eabi-objdump -d MDK/output/exe/Project.axf | sed -n '/<debug_trace_flush_close>:/,/^$/p'
    1aac:  bl  1498 <UART_GetFlagStatus>     <- 等 TXE (最后一字节进入移位器)
    1ab8:  bl  1498 <UART_GetFlagStatus>     <- 等 TXBUSY 清零 (移位器排空)
    1ac6:  bl  11ac <SYSCTRL_APBPeriphReset1> <- 排空后才复位 UART1
    1ace:  bl  11ac <SYSCTRL_APBPeriphReset1>
    1ad6:  bl  1140 <SYSCTRL_APBPeriphClk_Enable1>
```

- 交付的 Keil 镜像中确实为「两次状态轮询后再复位 UART1」，即 D-ITEM001-1 修复已进入真实目标产物（不再是单次 `TC` 轮询）。
- `mdk_flash`/COM42 复验仍未执行（工具不在本调用列表）；复验归嵌入式验证能力。
- 工具在仓库根产生的 `build.log` 已读入本证据后删除。

---

# 历史：上一轮工作流构建证据（BUILD-002 rev 3.0）

## T1（ITEM-001）：交叉编译与宿主机回归（本轮实际执行）

环境：
- 交叉编译器：`arm-none-eabi-gcc 10.3.1 20210824`（GNU Arm Embedded Toolchain，Cortex-M0+）
- CMSIS-Core：ARM CMSIS 5.9.0（本机 Arm Packs）
- 宿主机编译器：MinGW-w64 GCC 12.2.0（`C:/ProgramData/chocolatey/lib/mingw/tools/install/mingw64/bin/gcc.exe`，需用完整路径）
- 构建脚本：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/gcc/build.sh`

命令与原始结果：

```
$ cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output
$ sh gcc/build.sh
== compile ==
...
== link ==
Memory region         Used Size  Region Size  %age Used
           FLASH:       32208 B        64 KB     49.15%
             RAM:        1712 B         4 KB     41.80%
== done: obj/sensor_fw.elf/.hex/.bin ==

$ arm-none-eabi-size gcc/obj/sensor_fw.elf
   text    data     bss     dec     hex filename
  32208      84    1628   33920    8480 gcc/obj/sensor_fw.elf
```

- **0 错误**；告警仅来自既有厂商库（`Libraries/src/*`）与既有代码，无一条指向 T1 的 `gxht40.c`/`sf_i2c.c`/`fw_core.c`。
- 资源在预算内：FLASH 32,208 B ≤ 64 KB；RAM 1,712 B ≤ 4 KB。

Keil MDK 量产工具链构建（本能力本轮已授权 `mdk_build`）：

```
$ mdk_build {"action":"build"}      # 工具 CLI / MCP 同名工具，工程由配置决定
{"tool":"mdk_build","success":true,"exit_code":0,"stdout":"build_log:\n*** Using Compiler 'V6.24', folder: 'C:\\Keil_v5\\ARM\\ARMCLANG\\Bin'\nBuild target 'Project'\n\".\\output\\exe\\Project.axf\" - 0 Error(s), 0 Warning(s).\nBuild Time Elapsed:  00:00:01"}
```

- 工程 `MDK/Project.uvprojx` 用 ARMCLANG V6.24 构建通过：**0 Error / 0 Warning**，产物 `MDK/output/exe/Project.axf`（构建产物被 `.gitignore` 忽略）。
- 工具在仓库根产生的 `build.log` 已读入本证据后**删除**，未入库。
- 说明：下载/回读校验（`mdk_flash`）与烧录后业务观测属 TD-002 `[实板]` 项，本轮未授权、未执行。

浮点依赖核查（区分 T1 路径与既有厂商库）：

```
$ for o in gxht40 sf_i2c fw_core; do arm-none-eabi-nm gcc/obj/$o.o | grep -E "__aeabi_[df]|__float"; done
(no float/soft-float references)          # T1 三个翻译单元均无浮点引用

$ arm-none-eabi-nm gcc/obj/sensor_fw.elf | grep -E "__aeabi_[df]" | head
__aeabi_d2f / __aeabi_dadd / __aeabi_dmul / ...   # soft-double 符号
```

- T1 翻译单元（`gxht40.o`、`sf_i2c.o`、`fw_core.o`）**无**浮点/soft-float 引用（换算为整数运算）。
- 链接后的 ELF 仍含 soft-double 符号，来源为既有厂商对象 `cw32l010_adc.o`、`cw32l010_uart.o`、`um2005C.o`，属**改动前既有**依赖，不由 T1 引入。TD-002 T-L1-03 针对「换算/判定路径」，T1 已满足；若要求全镜像无浮点，属独立整改项（交接给后续轮次/需求方，不在 T1 范围）。

宿主机回归（T1 相关）：

```
$ CC=<mingw full path> sh test/build_test.sh
== [1/2] fw_core pure logic: CRC16/CRC-8/convert/light/report ==
==== result: 38 passed, 0 failed ====
== [2/2] measure flow (mock MCU): sample/report/prev-not-updated-on-failure ==
==== result: 24 passed, 0 failed ====
```

T1 专项 harness 的原始输出见 `evidence/driver_test.md`（T1 节）。

---

> 以下为**上一轮工作流（旧 12 项编号）**的构建证据，仅供追溯；其 ITEM 编号与本轮 T1–T7 / ITEM-001..007 不对应。

## 1. 环境

- 交叉编译器：`arm-none-eabi-gcc 10.3.1 20210824`（GNU Arm Embedded Toolchain，Cortex-M0+）
- CMSIS-Core：ARM CMSIS 5.9.0（`core_cm0plus.h`，本机 Arm Packs）
- 宿主机编译器：MinGW-w64 GCC 12.2.0（`C:/ProgramData/chocolatey/lib/mingw/tools/install/mingw64/bin/gcc.exe`；PATH 上的 chocolatey shim `bin/gcc.exe` 无法解析 libexec/cc1，需用完整路径）
- 构建脚本：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/gcc/build.sh`
- 产物：`gcc/obj/*.o`、`gcc/obj/sensor_fw.elf/.hex/.bin`（被 `.gitignore` 忽略，不入库）

## 2. 交叉编译结果（本轮）

```
== compile ==   USER/src + COMMON + UM2005C + Libraries/src 全部翻译单元
== link ==
Memory region   Used Size   Region Size   %age Used
FLASH:          33,896 B    64 KB         51.72%
RAM:             1,960 B    4 KB          47.85%
== done: obj/sensor_fw.elf/.hex/.bin ==
arm-none-eabi-size: text 33896 / data 84 / bss 1876 / dec 35856
```

- **0 错误**。告警全部来自既有代码与厂商库（`encrytogate.c` 未用函数、`main.c` 返回类型/未用参数、`sf_i2c.c` 可能未初始化、`Libraries/src/*` 库宏等），**无任何告警指向 `sensor_config.h`**。
- 与改动前基线对比：FLASH 33,896 B、RAM 1,960 B **完全一致**（配置头只含宏，不产生代码），证明「加入该头文件后工程仍可交叉编译」。

## 3. 配置头编译期断言检查

命令（不进入固件构建，使用临时宿主机侧 .c）：

```
arm-none-eabi-gcc -mcpu=cortex-m0plus -mthumb -Wall -Wextra \
  -I USER/inc -I COMMON -I UM2005C -I ../../../Libraries/inc \
  -I <CMSIS Core Include> -fsyntax-only /tmp/cfg_check.c
```

断言与结果：

| 断言 | 期望 | 结果 |
|---|---|---|
| `GXHT40_ADDR_WRITE_A/READ_A/WRITE_B/READ_B` | 0x88 / 0x89 / 0x8A / 0x8B | PASS |
| `GXHT40_ADDR_7BIT_A/B` | 0x44 / 0x45 | PASS |
| `GXHT40_CMD_MEASURE_HIGH_REP` / `GXHT40_RESULT_LEN` | 0xFD / 6 | PASS |
| `GXHT40_MEASURE_WAIT_MS` | ≥ 9（覆盖 tMEAS.H max 8.3 ms） | PASS（=10） |
| `LIGHT_ADC_INPUT_CHANNEL` | 11（PB04=AIN11） | PASS |
| `LIGHT_POWER_PIN` / `LIGHT_ADC_PIN` | `GPIO_PIN_5` / `GPIO_PIN_4` | PASS |
| `LIGHT_ADC_FULL_SCALE` | 4095 | PASS |
| `LIGHT_DARK_EXIT < LIGHT_DARK_ENTER` 且差 = `LIGHT_DARK_HYSTERESIS_STEP` | 成立 | PASS |
| `SENSOR_RTC_TICK_PERIOD_MIN × SENSOR_SAMPLE_TICKS` | 3 | PASS |
| `SENSOR_REPORT_DROP_X10` / `SENSOR_REPORT_HIGH_X10` | 9 / 350 | PASS |
| `SENSOR_RF_FRAME_LEN` | 10 | PASS |
| `SENSOR_TEST_TRACE` 默认 | 0 | PASS |

结论：**CONFIG HEADER CHECK OK（全部断言通过）**。

## 4. 宿主机既有 L0 测试（回归）

```
CC=<minGW full path> sh test/build_test.sh
==== result: 56 passed, 0 failed ====
```

本项未改动 `fw_core.c`/`history.c`/`host_sensor_core_test.c`，56/56 与改动前一致，属回归确认（新的纯逻辑用例属后续任务项）。

## 5. ITEM-002 增量（sf_i2c 原语）

改动：`USER/inc/sf_i2c.h`（+8 行声明）、`USER/src/sf_i2c.c`（+62 行实现，**0 行删除**）；新增测试载体 `test/host_sf_i2c_bus_check.c`（不进入固件构建）。

```
== compile ==  全部翻译单元
== link ==
Memory region   Used Size   Region Size   %age Used
FLASH:          34,060 B    64 KB         51.97%
RAM:             1,960 B    4 KB          47.85%
== done: obj/sensor_fw.elf/.hex/.bin ==
arm-none-eabi-size: text 34060 / data 84 / bss 1876 / dec 36020
```

- **0 错误**。相对 ITEM-001（FLASH 33,896 B / RAM 1,960 B）：FLASH **+164 B**（两个原语），RAM 不变。
- 告警：无新增。`sf_i2c.c` 仍只有改动前既有的 2 条 `err may be used uninitialized`（位于未触碰的 `i2c_write_multi_byte` / `i2c_write_multi_byte_16bit`）。
- 链接符号（`arm-none-eabi-nm obj/sensor_fw.elf`）：`i2c_write_cmd 0x46b2 T`、`i2c_read_bytes 0x46e4 T` 已进入镜像；既有 `i2c_write_byte`/`i2c_read_byte`/`i2c_read_multi_byte`/`i2c_write_multi_byte`/`i2c_*_16bit`/`i2c_start`/`i2c_stop`/`i2c_init`/`i2c_obj_find` 全部保留。
- 变更范围：`git diff --numstat` = `62 insertions / 0 deletions`（sf_i2c.c），既有函数未被改动。
- 驱动级自检：`test/host_sf_i2c_bus_check.c` → **15 passed / 0 failed**（详见 `evidence/driver_test.md`）。
- 宿主机 L0 回归：`test/build_test.sh` → 56 passed / 0 failed。

## 6. ITEM-003 增量（GXHT40 驱动）

改动：新增 `USER/inc/gxht40.h`、`USER/src/gxht40.c`；新增测试载体 `test/host_gxht40_check.c`（不进入固件构建）。无既有文件修改。

```
== compile ==  全部翻译单元（含 gxht40.c）
== link ==
Memory region   Used Size   Region Size   %age Used
FLASH:          34,060 B    64 KB         51.97%
RAM:             1,960 B    4 KB          47.85%
== done: obj/sensor_fw.elf/.hex/.bin ==

arm-none-eabi-size obj/gxht40.o : text 460 / data 0 / bss 5
arm-none-eabi-nm   obj/gxht40.o :
  T gxht40_init / gxht40_measure / gxht40_detected_addr7
  t gxht40_crc8 / gxht40_start_and_read
  b s_dev / s_addr7
  U delay_ms / i2c_write_cmd / i2c_read_bytes
```

- **0 错误，无任何告警指向 `gxht40.c`**。
- 工程 FLASH/RAM **与 ITEM-002 相同（34,060 / 1,960 B）**：`gxht40.o` 已编译但尚未被引用，链接器未拉入（接线由 ITEM-006 完成）；因此本项不改变镜像内容，仅新增可编译、可单测的驱动模块。
- 依赖面：`gxht40.c` 仅依赖 `sensor_config.h`（配置）、`sf_i2c`（ITEM-002 原语）与 `delay_ms`；无直接寄存器访问，无浮点。
- 驱动级自检：`test/host_gxht40_check.c` → **27 passed / 0 failed**（详见 `evidence/driver_test.md`）。
- 宿主机 L0 回归：`test/build_test.sh` → 56 passed / 0 failed。
- 工程文件注册（MDK `Project.uvprojx` / IAR `project.ewp` 的源文件列表加入 `gxht40.c`）属 ITEM-011 范围，本项未改工程文件；`gcc/build.sh` 通过通配自动包含。

## 7. ITEM-004 增量（光照通路）

改动：新增 `USER/inc/light.h`、`USER/src/light.c`；`USER/inc/sensor_config.h` +2 行（`LIGHT_ADC_EOC_GUARD`）；新增测试载体 `test/host_light_check.c`。

```
== compile ==  全部翻译单元（含 light.c）
== link ==
Memory region   Used Size   Region Size   %age Used
FLASH:          34,060 B    64 KB         51.97%
RAM:             1,960 B    4 KB          47.85%
== done: obj/sensor_fw.elf/.hex/.bin ==

arm-none-eabi-size obj/light.o : text 364 / data 0 / bss 4
arm-none-eabi-nm   obj/light.o : T light_code_is_dark / light_init / light_last_code /
                                 light_reset_state / light_sample ; b s_dark_state / s_last_code
```

- **0 错误，无任何告警指向 `light.c`**；总告警 29 条，与基线同数。
- 工程 FLASH/RAM **与 ITEM-003 相同（34,060 / 1,960 B）**：`light.o` 已编译但尚未被引用，链接器未拉入（接线由 ITEM-006 完成）。
- 依赖面：`light.c` 仅依赖 `sensor_config.h`（配置）、`delay_ms` 与 CW32 标准外设库（GPIO/ADC/SYSCTRL）；无浮点。
- 纯逻辑自检：`test/host_light_check.c` → **17 passed / 0 failed**（详见 `evidence/driver_test.md`）。
- 宿主机 L0 回归：`test/build_test.sh` → 56 passed / 0 failed。
- 工程文件注册（MDK/IAR 源列表加入 `light.c`）属 ITEM-011；`gcc/build.sh` 通过通配自动包含。

## 8. ITEM-005 增量（fw_core.c 纯逻辑）

改动：`USER/inc/fw_core.h`、`USER/src/fw_core.c` 新增 7 个纯函数；`USER/inc/sensor_config.h` 拆为纯数值段 + `SENSOR_CONFIG_NO_MCU` 守卫的 MCU 段；`gxht40.c`/`light.c`/`light.h` 改为复用纯函数；新增 `test/host_fw_core_pure_check.c`。

```
== compile ==  全部翻译单元
== link ==
Memory region   Used Size   Region Size   %age Used
FLASH:          34,352 B    64 KB         52.42%
RAM:             1,960 B     4 KB         47.85%
== done: obj/sensor_fw.elf/.hex/.bin ==
```

- **0 错误；告警 29 条，与基线同数**（初版曾因注释内 `/*` 产生 17 条 `-Wcomment`，已修正注释文本）。
- 工程 FLASH 34,352 B（较 ITEM-004 **+292 B**）：`fw_core.o` 原本已随 `params.c` 入镜像，新增纯函数随之被链接；RAM 不变。`gxht40.o`/`light.o` 仍未被引用（接线属 ITEM-006）。
- `fw_core.c` 仍**不依赖 MCU 头**：仅含 `fw_core.h`/`params.h` 与 `sensor_config.h`（定义 `SENSOR_CONFIG_NO_MCU`），因此 `test/build_test.sh` 的 `-I../USER/inc` 无需追加 MCU/CMSIS 路径。
- 纯逻辑自检：`test/host_fw_core_pure_check.c` → **36 passed / 0 failed**（详见 `evidence/driver_test.md`）。
- 回归自检：`host_gxht40_check.c` 27/27、`host_light_check.c` 17/17、`host_sf_i2c_bus_check.c` 15/15。
- 既有宿主机 L0：`test/build_test.sh` → 56 passed / 0 failed。

## 9. ITEM-006 增量（measure.c 采样流程）

改动：`USER/src/measure.c` 采样部分重写（AHT21 步进状态机 → `light_sample()` + `gxht40_measure()`；新增前一有效温度/最近有效湿度状态）；`USER/inc/measure.h` 移除废弃声明；`fw_core.c` 的 `SENSOR_CONFIG_NO_MCU` 加 `#ifndef` 守卫；新增 `test/host_measure_flow_check.c` + `test/mock_measure_mcu/`。

```
== compile ==  全部翻译单元
== link ==
Memory region   Used Size   Region Size   %age Used
FLASH:          35,884 B    64 KB         54.75%
RAM:             1,896 B     4 KB         46.29%
== done: obj/sensor_fw.elf/.hex/.bin ==

arm-none-eabi-nm obj/sensor_fw.elf :
  T gxht40_measure / light_sample / sensor_decide_report  (已入镜像)
```

- **0 错误；告警 28 条**（基线 29：旧 AHT21 序列移除后少 1 条，无新增；无一条指向 `measure.c`）。
- FLASH 34,352 → **35,884 B**（+1,532 B）：`gxht40.o`/`light.o`/新增纯函数因 `measure.c` 引用而入镜像；`history.o` 不再被引用。RAM 1,960 → **1,896 B**。
- 采样流程自检：`test/host_measure_flow_check.c`（mock MCU 影子头 + 可控 GXHT40/光照桩，编译真实 `measure.c`）→ **15 passed / 0 failed**。
- 回归：`host_fw_core_pure_check.c` 36/36、`host_gxht40_check.c` 27/27、`host_light_check.c` 17/17、`host_sf_i2c_bus_check.c` 15/15、`test/build_test.sh` 56/56。
- 工程文件注册（MDK/IAR 源列表）属 ITEM-011；`gcc/build.sh` 通配自动包含。

## 10. ITEM-007 增量（3 分钟节拍）

改动：`USER/src/main.c` 的 `RTC_IRQHandlerCallBack()` 阈值 `rtc_set_cnt >= 1` → `>= SENSOR_SAMPLE_TICKS`（=3）；删除未使用的 `temp_cnt`/`work_period_flag`；修正陈旧注释。

```
== compile ==  全部翻译单元
== link ==
Memory region   Used Size   Region Size   %age Used
FLASH:          35,884 B    64 KB         54.75%
RAM:             1,896 B     4 KB         46.29%
```

- **0 错误；告警 28 条**（与 ITEM-006 同数；`main.c` 仅改动前既有 4 条）。FLASH/RAM 不变（节拍常量不产生代码）。
- 源码结构确定性检查（见 `evidence/driver_test.md`）：阈值 = `SENSOR_SAMPLE_TICKS`；无 `rtc_set_cnt >= 1`/`first_sample_reported`/`samples_since_report` 残留；`sample_flag = 0` 消费；配置 1 min × 3 = 3 min。
- 回归：`host_fw_core_pure_check.c` 36/36、`host_measure_flow_check.c` 15/15、`host_gxht40_check.c` 27/27、`host_light_check.c` 17/17、`host_sf_i2c_bus_check.c` 15/15、`test/build_test.sh` 56/56。
- 板级 3 分钟长时基（含 LSI 容差）属 TD-002 T-L4-01，需嵌入式测试；本环境无硬件。

## 11. ITEM-008 增量（条件上报发送路径）

改动：`USER/src/measure.c` `send_data_to_gateway()`（删除被 `encode_frame10` 覆盖的冗余组帧、改用最近有效样本、长度用 `SENSOR_RF_FRAME_LEN`）；`USER/src/main.c` 陈旧注释；新增 `test/host_rf_frame_check.c`；扩展 `test/host_measure_flow_check.c`（发送路径 9 项）。

```
== compile ==  全部翻译单元
== link ==
Memory region   Used Size   Region Size   %age Used
FLASH:          35,856 B    64 KB         54.71%
RAM:             1,896 B     4 KB         46.29%
```

- **0 错误；告警 28 条**（与 ITEM-007 同数；`measure.c` 0 告警，`main.c` 仅既有 4 条）。FLASH 35,884 → **35,856 B**（−28 B，删除冗余组帧代码）；RAM 不变。
- 空口往返（协议级）：`test/host_rf_frame_check.c`（真实传感器 `encrytogate.c` ↔ 真实网关 `feistel_al.c`）→ **14 passed / 0 failed**（详见 `evidence/protocol_test.md`）。
- 发送路径（流程级）：`test/host_measure_flow_check.c` → **24 passed / 0 failed**（含门控/样本/长度/成功清除/失败重试/放弃）。
- 回归：`host_fw_core_pure_check.c` 36/36、`host_gxht40_check.c` 27/27、`host_light_check.c` 17/17、`host_sf_i2c_bus_check.c` 15/15、`test/build_test.sh` 56/56。
- 板级 433 发射/接收与发送失败上限属 TD-002 T-L6-03，需嵌入式测试。

## 12. ITEM-009 增量（退役旧通路）

改动：删除 `hall.c/.h`、`optcfg.c/.h`、`params.c/.h`、`history.c/.h`（8 个）；`main.h`/`main.c`/`interrupts_cw32l010.c`/`measure.c`/`fw_core.c/.h` 移除引用与旧逻辑；`test/host_sensor_core_test.c`/`build_test.sh` 收敛为 CRC16 回归；`host_measure_flow_check.c` 移除不再需要的桩。

```
== compile ==  全部翻译单元
== link ==
Memory region   Used Size   Region Size   %age Used
FLASH:          32,208 B    64 KB         49.15%
RAM:             1,712 B     4 KB         41.80%

arm-none-eabi-nm : hall/optcfg/params/history 符号 = 0；
                   T gxht40_measure / light_sample / sensor_decide_report / fw_crc8_gxht 均在
```

- **0 错误；告警 28 条**（与 ITEM-008 同数，均为既有模板/库告警）。FLASH 35,856 → **32,208 B**（−3,648 B）；RAM 1,896 → **1,712 B**（−184 B）。
- 退役引用检查：`USER/` 中仅 3 处说明性注释提到旧模块名；MDK/IAR 工程文件本就未登记这些模块。
- 引脚所有权：PB04/PB05 仅出现在 `light.c`/`sensor_config.h`（owner），PB06 无引用。
- 回归：`test/build_test.sh` 2/2、`host_fw_core_pure_check.c` 36/36、`host_measure_flow_check.c` 24/24、`host_gxht40_check.c` 27/27、`host_light_check.c` 17/17、`host_sf_i2c_bus_check.c` 15/15、`host_rf_frame_check.c` 14/14。
- MDK/IAR 源列表一致性（补 gxht40.c/light.c/fw_core.c）属 ITEM-011；新增纯逻辑用例属 ITEM-010。

## 13. ITEM-010 增量（宿主机测试更新）

改动：`test/host_sensor_core_test.c` 重写为综合纯逻辑套件（保留 CRC16，新增 CRC-8/换算/光照/上报判定）；`test/build_test.sh` 扩展为两阶段（纯逻辑 + 采样/上报流程）。

```
== [1/2] fw_core pure logic ==   result: 38 passed, 0 failed
== [2/2] measure flow (mock) ==  result: 24 passed, 0 failed
build_test.sh exit = 0

== 交叉编译 ==  0 error / 28 warning
FLASH: 32,208 B / RAM: 1,712 B   (测试不进入固件, 与 ITEM-009 相同)
```

- 覆盖 TD-002 T-L0-01…T-L0-06（含温度/湿度边界、光照滞回、上报判定 0.9℃/35.0℃/无前值/非暗）与 T-L6-01（失败不更新前值，阶段 2）。
- 回归：`host_fw_core_pure_check.c` 36/36、`host_measure_flow_check.c` 24/24、`host_gxht40_check.c` 27/27、`host_light_check.c` 17/17、`host_sf_i2c_bus_check.c` 15/15、`host_rf_frame_check.c` 14/14。
- 本机依赖：MinGW-w64 gcc（PATH 上的 chocolatey shim 损坏，需完整路径 `CC=<full path>`）。

## 14. ITEM-011 增量（交叉编译与 MDK/IAR 工程列表）

改动：`MDK/Project.uvprojx`、`EWARM/project.ewp` 源文件列表补齐/对齐；`.gitignore` 新增 `*.exe`。

```
== gcc/build.sh ==
== compile ==  全部翻译单元
== link ==
Memory region   Used Size   Region Size   %age Used
FLASH:          32,208 B    64 KB         49.15%
RAM:             1,712 B     4 KB         41.80%
== done: obj/sensor_fw.elf/.hex/.bin ==

arm-none-eabi-size: text 32208 / data 84 / bss 1628
sensor_fw.bin MD5 = 47abc6dfc798c837b2b68916f29db543
sensor_fw.hex MD5 = ddf2e2e7bac47a80ea00177cb104afc9
```

- **0 错误、28 条告警**；FLASH 32,208 B ≤ 64 KB、RAM 1,712 B ≤ 4 KB（均满足）。
- 生产编译器 AC5（`armcc --cpu=Cortex-M0+ --c99`，CMSIS 5.9.0）：`USER/src` 8 个文件 0 error（`encrytogate.c` 3 条既有告警）。
- MDK/IAR `USER/src` 列表 == 磁盘 `USER/src/*.c`（8 个）；已删模块不在列表中；两工程均含 `cw32l010_adc.c`、`UM2005C`、`COMMON`；两工程文件 XML 解析通过。
- 宿主机测试：`test/build_test.sh` 38/38 + 24/24，退出码 0。
- MDK/IAR 实机构建未执行（环境：Keil CMSIS 6.3.0 与 AC5 不兼容，IAR 不可用）。

## 15. ITEM-012 增量（网关兼容性核对，无固件改动）

改动：仅扩展测试 `test/host_rf_frame_check.c`（新增第 [6] 组映射检查）；**传感器/网关/Android 产品代码 0 改动**（`git status -- CH592EVT/ dengbei_care/` 为空）。

```
== gcc/build.sh ==  0 error / 28 warning
FLASH: 32,208 B (49.15%) / RAM: 1,712 B (41.80%)   (与 ITEM-011 相同)
== test/build_test.sh ==  exit 0, 38/38 + 24/24
== host_rf_frame_check.c (真实传感器编码器 + 真实网关解码器/帧构造器) ==
   result: 22 passed, 0 failed
```

- 核对结论：网关 `decode_frame10`（temp=p[4..5]、hum=p[6..7]）、每设备 8 B 记录 `id|hum_be|temp_be`（`build_device_block` 前 8 B 直拷）与 Android `parsePlainFrame`（`u16be@4`/`s16be@6`）逐位一致，**无需对齐修改**。
- 详见 `evidence/protocol_test.md`（ITEM-012 节）。

## 16. 限制与交接

- 本证据为 GNU 交叉编译 + 宿主机回归；量产构建走 Keil MDK / IAR EWARM（既有工程，`USER/inc` 已在两者 include 路径中，故新头文件无需改工程文件列表），未在本环境复编译 MDK/IAR。
- 网关（CH592 beiwov2）需 WCH 工具链，本轮未涉及。
- PATH 上默认 `gcc` shim 损坏导致 `test/build_test.sh` 直接调用失败；需用完整路径或修正 PATH。这是环境问题，非代码问题，已在上面记录调用方式。
