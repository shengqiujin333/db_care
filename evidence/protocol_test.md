# 协议测试证据（PROTO-002）

状态：固件实现证据（firmware_engineer.firmware_implementation）
本轮对象：任务项 **ITEM-008**（条件上报的 433 帧组包/加密）
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
