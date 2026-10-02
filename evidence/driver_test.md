# 驱动测试证据（DRV-002）

状态：固件实现证据（firmware_engineer.firmware_implementation）
本轮对象：任务项 **ITEM-002**（`sf_i2c` 无寄存器地址总线原语）、**ITEM-003**（GXHT40 驱动）
被测实现：`USER/src/sf_i2c.c` / `USER/inc/sf_i2c.h`（ITEM-002）；`USER/src/gxht40.c` / `USER/inc/gxht40.h`（ITEM-003）
测试载体：`.../gpio_input_output/test/host_sf_i2c_bus_check.c`、`.../test/host_gxht40_check.c`（宿主机 mock 总线，不依赖 MCU 寄存器）

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
