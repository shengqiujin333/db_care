# Driver/Logic Self-Test Evidence — ITEM-006

状态：实现侧自检（**不表示实板通过**；未烧录/未接目标）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
被测源码：当前工作树 `sensor_config.h`、`debug_trace.c`、实现侧测试；父基线 `8f9f86c`
编译器：MinGW-w64 gcc 12.2.0；Keil ARMCLANG V6.24

## 1. 标定配置编译门禁

`test/check_calib_config.sh` 使用真实 `sensor_config.h`、`-DSENSOR_CONFIG_NO_MCU`：
结果 **6 passed / 0 failed**（`calib_cfg.log`）。

| 配置 | 期望 | 实测 |
|---|---|---:|
| 默认（CALIBRATED=0, REF=0） | 编译通过 | PASS |
| CALIBRATED=1, REF=1 | 编译通过 | PASS |
| CALIBRATED=1, REF=4095 | 编译通过 | PASS |
| CALIBRATED=2 | 编译失败 | PASS（拒绝） |
| CALIBRATED=1, REF=0 | 编译失败 | PASS（拒绝） |
| CALIBRATED=1, REF=4096 | 编译失败 | PASS（拒绝） |

## 2. 未标定/边界行为（既有实现侧套件）

- `host_light_check` 默认未标定变体：ADC 成功时 `adc_ok=true`、`valid=false`、`dark=false`；全超时 `valid=false/dark=false`，不回退 4095。
- `host_light_check` 标定变体（`-DLIGHT_DARK_CALIBRATED=1 -DLIGHT_DARK_REF_CODE=3000`）：`mean=1000`（ceil(3000/3)）判暗，`mean=999` 判亮。
- `host_fw_core_pure_check` 的 `C_dark=1..4095` 全量：`mean=ceil(C/3)` 为暗、`mean-1` 为亮。
- 以上均在 `host_tests_full.log` 的 53/28/29 组中通过。

## 3. BOOT 构建标识

`host_debug_trace_check` 使用 `SENSOR_FW_BUILD_ID` 构造期望：
实测 `BOOT fw=RUN10-FDS003 uid=01020304 rst=0001 uart=9600\r\n`，**51 passed / 0 failed**（`debug_trace_item006.log`）。
字段顺序/行长/S/G/TXE/TXBUSY 用例均保持。

## 4. 汇总

| 测试 | 结果 | 证据 |
|---|---:|---|
| 标定配置编译门禁 | 6 passed / 0 failed | `calib_cfg.log` |
| `host_debug_trace_check`（含 BOOT ID） | 51 passed / 0 failed | `debug_trace_item006.log` |
| `test/build_test.sh` 全量 5 组 | 233 passed / 0 failed | `host_tests_full.log` |

## 5. 限制与交接

- 未烧录/采集 COM43；不是实板、独立验证或验收。
- `LIGHT_DARK_REF_CODE=0/CALIBRATED=0` 仍为未标定状态；本轮不完成全暗标定（U02）。
- 测试能力 `host_diag_line_verify_ev.c` 硬编码旧 `BOOT fw=FD-002r4`，需测试能力更新为 `SENSOR_FW_BUILD_ID`；本实现未修改该测试资产。
