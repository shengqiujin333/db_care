# IC-001 契约校验证据

结论：PASS（接口架构自检；尚未进行硬件/固件/App 实现测试）。

## 溯源检查

| 检查项 | 证据 | 结果 |
|---|---|---|
| 用户需求六参数 | `readme.txt` 明确列出温降、湿降、温低、湿低、温高、湿高 | PASS |
| 1 分钟采样、约 20 分钟缓存、越界立即/平时 1 h 上报 | `readme.txt` | PASS |
| 传感器现有 433 帧 | `USER/src/encrytogate.c:118-135`：4 B ID + temp LE + hum LE + CRC LE + Feistel | PASS |
| 传感器调用一致性 | `USER/src/measure.c:315` 参数与 `encode_frame10(temp, hum)` 签名相反；契约已显式标为实现必修问题 | PASS |
| 中继器现有解码 | `APP/feistel_al.c:119-131`：解密、CRC、temp/hum LE | PASS |
| 中继器 BLE 服务 | `Profile/include/gattprofile.h:35-50`：FFE0、FFE1..FFE5、CHAR2 长 160 | PASS |
| Android BLE 使用 | `MqtttService.kt:682-687` 读取 FFE0/FFE2；`:1092-1128` 写 FFE3 | PASS |
| 绑定载荷 | Android `buildWritePayload` / `buildAllSavedIdsA1SinglePacket` 与中继器 `APP/app_um2006A.c:230-249` 使用 A1、数量、4 B ID | PASS |

## 契约一致性检查

- 参数字段数=6，payload 长度=`6*4=24 B`，OPTCFG/1 总长=`2+1+1+1+4+24+2=35 B`：PASS。
- 20 项历史采用两个 16-bit 定点值，RAM=`20*4=80 B`：PASS。
- BLE 绑定最大 50 个 ID 时载荷=`2+50*4=202 B`，小于 Android 请求 MTU 240 的 ATT 有效载荷 237 B；仍要求以协商 MTU 分片：PASS WITH IMPLEMENTATION CONDITION。
- 433 明文=4+2+2+2=10 B，与现有加密块宽一致：PASS。
- OPTCFG/1 具有 magic、版本、类型、定长、transaction ID、CRC、范围/关系检查、原子提交和幂等规则：PASS。
- 单向光学链路无确认能力已在契约和 UI 交接中明确：PASS。

## 已知风险/后续验证点

1. 40 ms 位宽、±25% 容差和三次发送需在目标光敏电路及 Android 机型上做示波器/真机验证；若电路带宽或相机 API 限制不满足，应通过新协议版本调整，不可静默改变编码。
2. 当前自定义 Feistel 与硬编码系统密钥不是强设备认证；本节点保持兼容，安全加固需单独工作项。
3. CH592 FFE2 固定 160 B 与 Android 请求 MTU 240 并不等价；当前读取路径必须以实际特征值长度为界，后续实现测试需覆盖最大设备数及 AES 块对齐。
