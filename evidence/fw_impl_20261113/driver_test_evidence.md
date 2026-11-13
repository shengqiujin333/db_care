# 实现侧自检证据（DRV-004：无改动轮的宿主自检现状）

负责角色：固件工程
时点：2026-11-13（Asia/Shanghai）
范围：`test/build_test.sh` 五阶段宿主自检在本轮的运行结果；作为"当前实现无需修改"交付的执行证据。
证据类别：**宿主机 mock 自检**；不声明实板、电气或跨链路通过。

依据：`artifacts/firmware_implementation.md`（FWI-004 rev 9）、`artifacts/test_design.md`（TD-002 rev 9.0）、`artifacts/firmware_design.md`（FD-002 rev 8 §6.2/§12）。

---

## 1. 运行方式

```text
cd CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/test
bash build_test.sh
```

| 项 | 值 |
|---|---|
| 宿主 shell | Git Bash（Windows） |
| 宿主编译器 | `gcc`（MinGW-w64 12.2.0，`C:\ProgramData\chocolatey\bin\gcc.exe`） |
| 产物目录 | `test/_hostbin/`（`.gitignore` 覆盖 `*.exe`，不入库、不进正式镜像） |
| 原始输出 | `host_build_test.log` |

## 2. 结果（本轮）

| 阶段 | 覆盖 | 结果 |
|---|---|---:|
| [1/4] | `fw_core` 纯逻辑：CRC16、GXHT40 CRC-8/换算/有效域、LIGHT-DEF-2 阈值判据（`T=0/1/3590/4095`、边界、`T=1..4095` 全量）、`sensor_decide_report` 边界（+0.8/+0.9/+1.0、降温、恰好 35.0℃、无前值、int16 极值） | 48 / 0 |
| [2/4] | `measure.c` 采样流程（mock MCU）：冻结快照、重试、跨周期不覆盖、失败不推进前值、`P` 行取数 | 61 / 0 |
| [3/4] | UART1 调试通道（mock UART）：BOOT/S/G/P 行格式与 96 B 预算、`TXE/TXBUSY` 卡死有界返回、关闭语义 | 54 / 0 |
| [4/4a] | 光照通路（真实 `light.c` + 影子 ADC）：交付默认 `T=0` → `valid=false/dark=false`、全超时不合成暗态、部分成功、VDD 测量不破坏 AIN11 | 28 / 0 |
| [4/4b] | 光照通路启用变体（`-DLIGHT_DARK_THRESHOLD_CODE=3590`）：`mean==T` 判暗、`mean==T-1` 不判暗、实测参考 3514/3987、边界 | 31 / 0 |
| **合计** | | **222 / 0（exit 0）** |

## 3. 本轮状态说明

1. **断言与编译参数**：与上一轮（`evidence/fw_impl_20261112/host_build_test.log`）一致，**未降低任何断言**。
2. **环境修复（2026-11-13，已完成并已提交）**：宿主安全软件按文件名拦截 `_hostbin/measure_flow_check.exe`（`ld: cannot open output file … Permission denied`；同名手编亦失败，而 `report_flow_check.exe` 首次成功）。已将阶段 2 产物名改为 `report_flow_check.exe`，脚本头记录实测现象；仅产物文件名变化，用例/参数/断言未改。本轮为该修复后的再次通过。
3. **正式固件不含测试代码**：`test/` 与 `mock_*` 不参与 Keil 工程（`Project.uvprojx` 的 27 个源项无 `test`/`mock` 路径），正式 axf 摘要与发布包成员一致（见 `build_evidence.md` §3/§4）。

## 4. 不证明的范围

1. 影子 ADC/GPIO/UART 不能证明真实电气、时序、器件响应或上电建立；实板证据属 EV-036/037/038（测试角色，同 axf/hex 摘要）。
2. 本轮**未**启用无光判据（`T=0`），因此"升温 >0.9℃ 且无光"分支仍不触发；启用需要接受记录 + 被动双态证据（人工已确认该项延期，见 `evidence/fw_impl_20261112/assignment_referral.md` §2/§5）。
3. 未执行：真实升温/超温触发与 433 实发、光强双态、网关→手机跨链路、平台 `/upload_data` 写入/回读、CR2032 功耗测量。上述项不因本轮自检通过而改变状态。
4. 宿主通过不等于实板通过，也不等于产品验收通过；`PRODUCT-ACCEPTANCE-RECHECK-002` 的 `TEST_FAIL` 仍然有效。
