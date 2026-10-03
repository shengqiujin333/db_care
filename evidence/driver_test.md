# 驱动测试证据（DRV-002）

状态：固件实现证据（firmware_engineer.firmware_implementation）
本轮对象：任务项 **ITEM-002**（`sf_i2c` 无寄存器地址总线原语）、**ITEM-003**（GXHT40 驱动）、**ITEM-004**（光照通路）、**ITEM-005**（`fw_core.c` 纯逻辑）、**ITEM-006**（`measure.c` 采样流程）、**ITEM-007**（3 分钟节拍）
被测实现：`USER/src/sf_i2c.c` / `USER/inc/sf_i2c.h`（ITEM-002）；`USER/src/gxht40.c` / `USER/inc/gxht40.h`（ITEM-003）；`USER/src/light.c` / `USER/inc/light.h`（ITEM-004）；`USER/src/fw_core.c` / `USER/inc/fw_core.h`（ITEM-005）；`USER/src/measure.c` / `USER/inc/measure.h`（ITEM-006）
测试载体：`.../test/host_sf_i2c_bus_check.c`、`.../test/host_gxht40_check.c`（mock 总线）、`.../test/host_light_check.c`（纯逻辑 + 硬件桩）、`.../test/host_fw_core_pure_check.c`（纯逻辑）、`.../test/host_measure_flow_check.c` + `.../test/mock_measure_mcu/`（mock MCU 影子头）

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
