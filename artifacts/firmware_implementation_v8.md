# 固件实现记录（FWI-008，ITEM-006）

状态：`IMPLEMENTED`（仅 ITEM-006 标定配置编译门禁、未标定门禁保持、BOOT 构建标识；**未烧录、未实板验证、未产品验收**）
能力：`firmware_engineer.firmware_implementation`
项目：dengbei_run10
父基线：Git `8f9f86c`（ITEM-005 已提交；本 item 开始时工作区干净）
实现范围：`artifacts/firmware_tasks_v3.yaml` 第 6 项
设计依据：FDS-003 DD-06/§7/§9、FWR-303/304/309、IC-003 IF01/IF08、PACC-002 PA-005/031
构建证据：`evidence/ev025_item006/build.md`
自测证据：`evidence/ev025_item006/driver_test.md`

---

## 1. 本次改动

| 文件 | 改动 |
|---|---|
| `CW32.../USER/inc/sensor_config.h` | 新增编译期门禁：`LIGHT_DARK_CALIBRATED` 只允许 0/1；已标定时 `LIGHT_DARK_REF_CODE` 必须 1..4095，否则 `#error` |
| `CW32.../USER/inc/sensor_config.h` | 新增 `SENSOR_FW_BUILD_ID "RUN10-FDS003"`；注释说明只是日志标识，镜像身份由提交/axf md5 绑定 |
| `CW32.../USER/src/debug_trace.c` | BOOT 改为 `trace_puts(SENSOR_FW_BUILD_ID)`，不再硬编码 `FD-002r4` |
| `CW32.../test/host_debug_trace_check.c` | `EXPECT_BOOT` 使用 `SENSOR_FW_BUILD_ID` |
| `CW32.../test/check_calib_config.sh` | 新增编译门禁自检（默认/1/4095 通过；2/0/4096 拒绝） |

未修改：光照算法（`LIGHT_SETTLE_MS=100`、`LIGHT_ADC_SAMPLES=8`、整数均值）、门控、发送、UART 有界、主循环、协议、引脚、硬件、网关。`LIGHT_DARK_REF_CODE=0`、`LIGHT_DARK_CALIBRATED=0` 保持未标定。

完整源码 SHA-256：`evidence/ev025_item006/source_sha256.txt`。
构建产物 md5：axf `9cb97918e8832811dfcb00a93646fba1`，hex `3af11e7adb21ba43004fc827649adb44`。

---

## 2. 行为实现

```c
#if (LIGHT_DARK_CALIBRATED != 0) && (LIGHT_DARK_CALIBRATED != 1)
#error "LIGHT_DARK_CALIBRATED must be 0 or 1"
#endif
#if LIGHT_DARK_CALIBRATED
#if (LIGHT_DARK_REF_CODE < 1u) || (LIGHT_DARK_REF_CODE > 4095u)
#error "LIGHT_DARK_REF_CODE must be 1..4095 when LIGHT_DARK_CALIBRATED=1"
#endif
#endif
#define SENSOR_FW_BUILD_ID "RUN10-FDS003"
```

- 未标定：`light.valid = adc_ok && (CALIBRATED != 0)` → false；`dark=false`；升温分支关闭（既有 `light.c`，本 item 不改）。
- 标定：`valid=adc_ok`；`dark = valid && 3*mean >= C_dark`；边界相等为暗。
- 非法配置在编译期拒绝，不进入固件。
- BOOT 打印 `fw=RUN10-FDS003`；字段顺序/行长不变。该标识不单独证明 Flash 身份。

---

## 3. 验证

### 3.1 编译门禁

`check_calib_config.sh`：**6 passed / 0 failed**（`calib_cfg.log`）：
- 默认 0/0、标定 1/1、标定 1/4095：编译通过；
- `CALIBRATED=2`、`CALIBRATED=1/REF=0`、`CALIBRATED=1/REF=4096`：编译失败（按预期拒绝）。

### 3.2 未标定与边界

`host_light_check` 默认/标定变体、`host_fw_core_pure_check` C_dark=1..4095：
- 未标定 ADC 成功仍 `valid=false/dark=false`；全超时不合成 4095；
- `mean=ceil(C_dark/3)` 判暗、减一判亮；C_dark=3000 时 1000/999 边界通过。

### 3.3 BOOT

`host_debug_trace_check`：`BOOT fw=RUN10-FDS003 uid=01020304 rst=0001 uart=9600\r\n`，**51/0**。

### 3.4 汇总/构建

| 项 | 结果 |
|---|---:|
| 全量 `build_test.sh` 5 组 | **233 passed / 0 failed** |
| Keil `build`/`rebuild` | 0 Error；唯一既有 `main.c(252) while(k--);` |
| Code/RO/RW/ZI | 19544 / 644 / 116 / 1684 |

未烧录、未复位、未连接 COM43；不宣称实板通过。

---

## 4. 未完成/交接

| 项 | 状态 |
|---|---|
| 全暗 `C_dark` 实测回填 | 未完成；依赖 U02 现场窗口；当前保持未标定 |
| 测试能力 `host_diag_line_verify_ev.c` | 仍硬编码旧 `BOOT fw=FD-002r4`，需测试能力改用 `SENSOR_FW_BUILD_ID`；本实现未修改该资产 |
| 实板 BOOT/标定/升温分支 | 未执行 |
| ITEM-007 测试资产全面同步/ITEM-008 构建静态核对 | 未完成 |

## 5. 结论

ITEM-006 的标定配置编译门禁、未标定 `valid/dark=false` 保持、1/3 边界、BOOT 构建标识已实现，并通过 6/0 编译门禁、51/0 BOOT 自检、233/0 全量宿主机自检。结论仅覆盖该 item；全暗标定、实板与产品验收未通过。
