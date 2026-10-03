# 协议测试证据（PROTO-002）

状态：固件实现证据（firmware_engineer.firmware_implementation）
本轮对象：任务项 **ITEM-008**（条件上报的 433 帧组包/加密）与 **ITEM-012**（网关固件兼容性核查）
被测实现：`USER/src/encrytogate.c`（传感器编码器，未修改）+ `CH592EVT/EVT/EXAM/BLE/beiwov2/APP/feistel_al.c`（网关解码器，未修改）
测试载体：`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test/host_rf_frame_check.c`

## 1. 目的

在宿主机上直接编译**真实的传感器编码器**与**真实的网关解码器**，验证 IC-002 §3/§4 的 433 空口约定：

```
uid_pick(4) | temperature_x10 int16 LE(2) | humidity_x10 uint16 LE(2) | crc16_ccitt LE(2)   --(8 轮 Feistel)--> 10 B
```

即：帧布局、字节序与 Feistel 加密在本次改动后保持不变，且传感器组包能被网关正确解出。

## 2. 运行

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
GW=../../../../../CH592EVT/EVT/EXAM/BLE/beiwov2/APP
gcc -std=c11 -Wall -Wextra \
    -Wno-unused-function -Wno-unused-const-variable -Wno-misleading-indentation \
    -I../USER/inc -I$GW \
    host_rf_frame_check.c ../USER/src/encrytogate.c $GW/feistel_al.c \
    -o host_rf_frame_check.exe && ./host_rf_frame_check.exe
```

结果：**14 passed, 0 failed**（编译 0 告警；最后三个 `-Wno-*` 仅屏蔽既有 `encrytogate.c`/`feistel_al.c` 的未用静态量与打印缩进告警）。

## 3. 检查明细

| 组 | 检查 | 结果 |
|---|---|---|
| T-L0i-01 向量 | `(-400,0)`、`(-100,0)`、`(0,0)`、`(250,550)`、`(350,1000)`、`(1250,1000)` 往返 uid/temp/hum 一致 | PASS ×6 |
| 边界/极值 | `(-450,0)`、`(1300,1000)`、`(32767,65535)`、`(-32768,0)` 往返一致 | PASS ×4 |
| 随机 | 128 组 LCG 随机 `(temp,hum)` 全部往返一致 | PASS ×1 |
| 字段位置/字节序 | `temp=-100 / hum=0x1234` 往返 -> 温度在 `p[4..5]` 小端、湿度在 `p[6..7]` 小端 | PASS ×1 |
| 完整性 | 10 个密文字节逐一翻转后 `decode_frame10` 均返回 false（CRC 拒绝） | PASS ×1 |
| 确定性 | 同一输入两次编码逐字节相同 | PASS ×1 |

往返同时隐含验证：传感器 `UID_IDX={0,1,2,3}` 选取的 4 字节 uid_pick 与网关 `decode_frame10` 原样输出的 uid_pick 一致（网关侧 `UID_IDX={1,3,7,9}` 仅用于其自身演示编码，不参与解码）。

## 4. 覆盖边界

- 本证据为**软件级协议互操作**：直接使用真实编码/解码实现，但**不涉及真实射频**（调制、前导/同步字、空中速率、丢包）与电气特性。
- 网关 BLE 侧每设备 8 B 记录顺序（`id | hum_be | temp_be`）与 Android `parsePlainFrame` 的 `u16be@4`/`s16be@6` 一致性已在 FD-002 §9 源码核对；其端到端验证属嵌入式测试（TD-002 T-L7/T-L2）。
- 真实 433 发射/接收与发送失败上限（TD-002 T-L6-03）需在真实硬件上完成。

## 5. 结论

传感器 `encode_frame10` 与网关 `decode_frame10` 对 142 组向量（14 类）逐位互操作一致，字段布局、小端序与 CRC/Feistel 加密保持不变；篡改必被 CRC 拒绝。ITEM-008 的“帧布局、字节序与 Feistel 加密保持不变”成立。

---

# ITEM-012：网关 → BLE → Android 兼容性核对

被测对象：网关 CH592 beiwov2 的解密/组帧链路与 Android `parsePlainFrame` 的对应关系

## 1. 结论：**一致，未修改任何网关/Android 代码**

| 环节 | 源码位置 | 行为 |
|---|---|---|
| 433 解密 | `APP/feistel_al.c: decode_frame10()` | `temp=p[4]\|p[5]<<8`（小端）、`hum=p[6]\|p[7]<<8`（小端）、CRC16 `p[8..9]` 小端 |
| 记录组装 | `APP/app_um2006A.c` | `sensorres[i*8+4..5]=resbf[6..7]`（湿度大端）、`[6..7]=resbf[4..5]`（温度大端） |
| 帧构造 | `APP/bleencrypt.c: build_device_block()` | `memcpy(out16,payload8,8)`（前 8 B 直拷）+ `[8..15]` pad；`build_padded_frame_blocks()` = Header(16)+DeviceBlock(16)×N |
| Android | `MqtttService.kt: parsePlainFrame()` | `p8=block[0..8)`；`humi=u16be(p8,4)/10`；`temp=s16be(p8,6)/10` |

即每设备 8 B 记录 = `id(4) | hum_be(2) | temp_be(2)`，与 IC-002 §4 一致；网关未按自身温度历史二次过滤。

## 2. 运行（真实网关解码器/帧构造器 + 真实传感器编码器）

```
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
GW=../../../../../CH592EVT/EVT/EXAM/BLE/beiwov2/APP
gcc -std=c11 -Wall -Wextra \
    -Wno-unused-function -Wno-unused-const-variable -Wno-misleading-indentation \
    -Wno-type-limits \
    -I../USER/inc -I$GW \
    host_rf_frame_check.c ../USER/src/encrytogate.c \
    $GW/feistel_al.c $GW/bleencrypt.c \
    -o host_rf_frame_check.exe && ./host_rf_frame_check.exe
```

结果：**22 passed, 0 failed**（编译 0 告警；`-Wno-*` 仅屏蔽既有网关/传感器文件的未用静态量、打印缩进、`dev_count>MAX_DEV_COUNT` 恒假告警）。

## 3. 新增检查（第 [6] 组）

| 检查 | 期望 | 结果 |
|---|---|---|
| 传感器 encode → 网关 decode | `temp=-100`/`hum=0x1234` 一致 | PASS |
| `build_device_block` | 前 8 B = payload8 直拷 | PASS |
| Android `devId` | `block[0..3]` == uid_pick | PASS |
| Android `humi` | `u16be@4` == 湿度 | PASS |
| Android `temp` | `s16be@6` == 温度（负温符号保持） | PASS |
| 整帧 | `build_padded_frame_blocks` 1 设备 → 32 B；Header `gwid6+devCount@6` | PASS |
| offset 16 解析 | id/hum/temp 全部一致 | PASS |

说明：`build_device_block` 未在 `bleencrypt.h` 声明，测试文件内以前向 `extern` 声明引用，**未修改网关头/源**。

## 4. 覆盖边界

- 网关 `app_um2006A.c` 的 4 行交换为按源码逐字复现（该文件含 TMOS/BLE 依赖，不能宿主机编译）；解码器与帧构造器为**真实网关代码**宿主机编译执行。
- 真实 433 收发、BLE 连接与 App 消费属嵌入式测试（TD-002 T-L2/T-L7）。
- CH592 实机构建需 WCH 工具链，本环境不具备；本轮网关源码 0 改动（`git status -- CH592EVT/` 为空）。

## 5. 结论（ITEM-012）

网关 `decode_frame10`（temp=p[4..5]、hum=p[6..7]）、每设备 8 B 记录 `id|hum_be|temp_be`（`build_device_block` 前 8 B 直拷）与 Android `parsePlainFrame`（`u16be@4`/`s16be@6`）三者逐位一致，无需对齐修改；已记录核对结论并保留可复现的映射自检。
