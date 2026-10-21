# 嵌入式验证证据（EV-022，ITEM-003 冻结帧与单周期 3 次发送）

能力：`embedded_tester.embedded_verification`
被测项：`TL-6c897e7b-38c9-4f2a-9d55-d216ddfaaff2-tasks-ITEM-003`
被测行为：门控触发时一次性构造并冻结 10 B 加密待发帧，帧内 ID、温度、湿度与触发样本一致；本周期最多 3 次、每次 200 ms 的发送全部发送同一字节缓冲，不重新编码、不重采样、不改值；成功或用尽都清除待发状态，不跨周期保留；`S.s` 为最终结果 0/1/2，`S.y` 保持现行失败计数器语义（成功/用尽后为 0）。
结论：**TEST_PASS（ITEM-003 本增量；宿主机独立 harness + 实板失败周期观测）**
日期：2026-10-21（本地 10:50–11:05）

> 范围声明：本结论只覆盖 ITEM-003 的冻结帧、单周期尝试上限、成功后/用尽清除、不跨周期与 `S.s/S.y` 语义。实板当前每周期 `q=0`（GXHT40 对 0x44/0x45 均无 ACK，U01 未闭合），无法在真实目标上构造触发周期，因此“3 次尝试逐字节相同、不重编码、`s=1/2`”由宿主机独立 harness 用**当前产品 `measure.c`** 证明；“非触发周期 `s=0`、`y=0`、不发送、不跨周期残留”同时有实板证据。**不**代表温湿度采集、光照标定、RF/网关/App/云链路或产品验收通过。

---

## 1. 受测件身份

| 项 | 值 | 来源 |
|---|---|---|
| Git HEAD | `09d4c52592d0a711e506d91bd4ecb6c0ccddc14a`（工作区干净） | `git_head.txt` |
| 被测源码 | `USER/src/measure.c` SHA-256 `0fe638320f04d6a5d41067b0a2313c8f0260ca56f13019bff9d5e38931769edf` | `source_sha256.txt` |
| 负向对照源码 | 父提交 `d4edf1f` 的 `measure.c` SHA-256 `67a8747f115171a2bb4bbe385ba3c9a196b425255f6f7a54e1a822cdf9ac7309` | `old_measure_pre_item003.c` |
| 构建 | Keil MDK `MDK/Project.uvprojx`，ARMCLANG V6.24，`mdk_build rebuild`：0 Error / 1 既有 warning（`main.c:245 while(k--);`），Code=20160 RO=644 RW=116 ZI=1676 | `build_rebuild.log` |
| 镜像 | `Project.axf` md5 `789a199e720436dff1de2d51f2cbd9cd`；`Project.hex` md5 `afc12baa6cca101578da15cbdec34e58` | `image_md5.txt` |
| 下载 | `mdk_flash`：`Erase Done. Programming Done. Verify OK. Application running ...`（10:50:33 本地） | `programmed_prototype.md`、`mdk_flash.log` |
| 目标板 | 传感器板（CW32L010F8 + GXHT40 + 光敏分压 + UM2005C）；本轮未在捕获窗口内复位，未重读 UID | — |
| 调试串口 | COM43，Prolific `067B:2303`，9600 8N1 | `serial_list_ports` 枚举、两次原始捕获 |
| 宿主机编译器 | MinGW-W64 gcc 12.2.0，`-std=c11 -Wall -Wextra`，链接 `-Wl,--wrap=encode_frame10` | `compile_current.err` / `compile_old.err` |

镜像 md5 与实现能力记录 `artifacts/firmware_implementation_v5.md` 完全一致。

---

## 2. 验证方法（独立于实现侧自检）

| 手段 | 文件 | 说明 |
|---|---|---|
| 测试能力自有 harness | `test/host_measure_freeze_verify_item003.c` | 直接编译**当前产品** `USER/src/measure.c`、`fw_core.c`、`sf_i2c.c`、`encrytogate.c`；影子 MCU 层 `test/mock_measure_ev/` 只替换寄存器/引脚与外设原语，不替换流程逻辑 |
| 编码调用计数 | 链接器 `--wrap=encode_frame10` | 真实产品编码器仍负责产生字节；wrapper 只计数，`__real_encode_frame10` 生成参考帧 |
| 独立参考模型 | `verify_measure_freeze_item003.py` | 只用打印出的刺激在 Python 中独立重算门控、期望尝试次数、最终结果，并逐字节比对每次尝试；不调用产品代码 |
| 旧实现负向对照 | `old_measure_pre_item003.c` + `harness_old.out` + `verify_old.out` | 同一 harness/同一刺激编译父提交 `d4edf1f` 的 `measure.c`，必须被拒绝 |
| 构建/下载复核 | `mdk_build rebuild`、`mdk_flash` | 当前源码可确定性重建并实际下载（`Verify OK`） |
| 实板观测 | `com43_capture_a.dat`、`com43_capture_b.dat` + `verify_com43_item003.py` | 下载后 COM43 原始字节；独立脚本解析并核对非触发周期不变量与节拍 |
| 回归参考 | `host_tests_regression.log` | 实现侧既有宿主机套件 5 组共 226 passed / 0 failed（回归参考，不作为本项独立判据） |

复现命令（在 `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/`）：

```sh
EV=D:/mypro/beiwo2/AIP/evidence/ev022_item003_verify
gcc -std=c11 -Wall -Wextra -I test/mock_measure_ev -I USER/inc -I UM2005C -I COMMON \
    -DVERIFY_LABEL='"current"' -Wl,--wrap=encode_frame10 \
    test/host_measure_freeze_verify_item003.c USER/src/measure.c \
    USER/src/fw_core.c USER/src/sf_i2c.c USER/src/encrytogate.c \
    -o $EV/harness_current.exe

gcc -std=c11 -Wall -Wextra -I test/mock_measure_ev -I USER/inc -I UM2005C -I COMMON \
    -DVERIFY_LABEL='"old_pre_item003"' -Wl,--wrap=encode_frame10 \
    test/host_measure_freeze_verify_item003.c $EV/old_measure_pre_item003.c \
    USER/src/fw_core.c USER/src/sf_i2c.c USER/src/encrytogate.c \
    -o $EV/harness_old.exe

cd $EV && ./harness_current.exe > harness_current.out && ./harness_old.exe > harness_old.out
python verify_measure_freeze_item003.py harness_current.out
python verify_measure_freeze_item003.py harness_old.out --expect-fail
python verify_com43_item003.py com43_capture_a.dat com43_capture_b.dat
```

编译 stderr 仅含产品 `encrytogate.c` 既有 `-Wextra` 告警；harness 本身无告警；两个进程退出码 0。

---

## 3. 宿主机刺激与逐周期结果

16 个事务：12 个有效样本（含 9 个触发周期）与 4 个失败周期。触发条件覆盖：首次无前值、升温 10、>35.0 ℃ 独立触发、降温、升温 1（≤9）、有光/光照无效、失败周期夹在中间。原始输出见 `harness_current.out`。

| # | 刺激（st/t/h/dark/lv） | RF 桩序列 | 触发 | enc | sends | 逐字节冻结 | s/y | 判定 |
|---:|---|---|---:|---:|---:|---|---|---|
| 0 | OK 250/500/1/1 | 111 | 否（首笔无前值） | 0 | 0 | — | 0/0 | PASS |
| 1 | OK 260/510/1/1 | 100 | 是（升温 10） | 1 | 1 | 单次=REF | 1/0 | PASS：第 1 次成功即停 |
| 2 | OK 270/520/1/1 | 000 | 是 | 1 | 3 | 3 帧完全相同=REF | 2/0 | PASS：用尽后 `s=2`，`y=0` |
| 3 | OK 280/530/1/1 | 001 | 是 | 1 | 3 | 3 帧完全相同=REF | 1/0 | PASS：第 3 次成功 |
| 4 | OK 280/530/1/1 | 111 | 否（无升温） | 0 | 0 | — | 0/0 | PASS：不编码/不发送 |
| 5 | OK 290/540/1/1 | 001 | 是 | 1 | 3 | 3 帧完全相同=REF | 1/0 | PASS：**重试期间改写 temp/hum=999/999，帧不变** |
| 6 | OK 351/550/0/0 | 100 | 是（>35.0 ℃） | 1 | 1 | =REF | 1/0 | PASS：光照无效仍独立触发 |
| 7 | CRC 999/999/1/1 | 111 | 否（q=0） | 0 | 0 | — | 0/0 | PASS |
| 8 | OK 15/620/1/1 | 111 | 否（自 351 降温） | 0 | 0 | — | 0/0 | PASS：降温不触发 |
| 9 | OK 25/630/1/1 | 010 | 是（升温 10） | 1 | 2 | 2 帧相同=REF | 1/0 | PASS：第 2 次成功 |
| 10 | OK 35/640/1/1 | 000 | 是 | 1 | 3 | 3 帧相同=REF | 2/0 | PASS：用尽清除 |
| 11 | OK 45/650/1/1 | 100 | 是 | 1 | 1 | =REF(45/650) | 1/0 | PASS：上一周期用尽后新触发不受影响，无残留帧 |
| 12 | IO 999/999/1/1 | 111 | 否（q=0） | 0 | 0 | — | 0/0 | PASS |
| 13 | OK 55/660/1/1 | 100 | 是（升温 10） | 1 | 1 | =REF(55/660) | 1/0 | PASS：失败周期未污染发送状态 |
| 14 | OK 55/660/0/0 | 111 | 否 | 0 | 0 | — | 0/0 | PASS |
| 15 | OK 56/661/1/1 | 111 | 否（升温 1 ≤9） | 0 | 0 | — | 0/0 | PASS |

独立参考模型汇总：**332 checks passed / 0 failed**（`verify_current.out`）。逐周期强制检查：

- 触发周期 `encode_frame10` 恰好 **1** 次；未触发周期 **0** 次（全轮 encoding 总计 9 次 = 9 个触发周期）。
- 每次尝试 `len=10`、`timeout=200`；尝试次数 = “首次脚本成功的位置”，无成功则恰好 3 次；成功即停止。
- 每次尝试的 10 B 与第 1 次**逐字节相同**，且等于 `encode(uid, 触发温度, 触发湿度)`（`REF` 行，由真实编码器生成）。
- 第 5 周期在第一次尝试后把 `tempvalue`/`huminityvalue` 改为 999/999（模拟重试期间到达新样本），第 2/3 次仍是触发样本 290/540 的编码 → 证明**不重编码、不重采样、不改值**。
- 每周期 `S.s` = 最终本地结果（0/1/2）；`S.y` 恒为 0；`report_req` 周期结束后为 0。
- 每周期结束再调用一次 `send_data_to_gateway()`：发送桩被调用 **0** 次 → 成功/用尽后待发状态确已清除，**无跨周期保留**。

### 3.1 负向对照（旧实现必须被拒绝）

`harness_old.out` 用同一 16 周期刺激运行父提交 `d4edf1f` 的 `measure.c`：

- 每次调用只做 **1** 次尝试，失败后 `report_req` 残留、`S.y=1`，下次调用继续重试（`extra_sends=1`）；
- 第 3/5/9 周期的最终结果被打印为第一次尝试的中间值 `s=2`，而不是脚本中最终成功的 `s=1`；
- 编码调用 12 次（每轮每轮尝试重新编码），而非 9 次。

独立模型判定：**22 项不符**，`NEGATIVE_CONTROL_OK`（`verify_old.out`）。这证明本用例针对的正是 ITEM-003 改变的语义，而不是“新旧都全过”的无效测试。

---

## 4. 实板观测（当前镜像已实际下载）

| 项 | 事实 |
|---|---|
| 下载 | `mdk_flash` 成功：`Erase Done. Programming Done. Verify OK. Application running ...` |
| 捕获 A | COM43 9600 8N1，300 s，112 B：1 个采样周期（`k=3`） |
| 捕获 B | COM43 9600 8N1，300 s，224 B：2 个采样周期（`k=6`、`k=9`），间隔 **180 s** |
| 周期内容 | 每个 `S` 后紧跟 1 条 `G`；`q=0 r=0 s=0 y=0`，`G s=2 a44=0 a45=0 rd=0 at=3` |
| 独立复核 | `verify_com43_item003.py`：**62 checks passed / 0 failed**（`verify_com43.out`） |

```text
S k=3 p=0 H=0 V=0 o=8 n=92 x=94 a=93 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
S k=6 p=0 H=0 V=0 o=8 n=90 x=92 a=90 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
S k=9 p=0 H=0 V=0 o=8 n=87 x=89 a=88 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
```

实板可证明的部分：

- 当前镜像已下载并运行，按 3 分钟节拍产生 `k=3/6/9`（相邻 `k` 增量 3）；
- 非触发周期 `s=0`、`y=0`：`S.s` 反映“本周期未发送”的最终结果，不是残留的中间值；
- 每周期恰一条 `S`，`G` 只随失败周期出现；相邻失败周期之间 `p/H/t/h` 不变，无跨周期状态残留；
- 每行含 CRLF 且 ≤96 B。

实板**不能**证明的部分（如实登记，不改为通过）：

- **触发周期**：本板 `G a44=0 a45=0`，GXHT40 对两个候选地址均无 ACK（U01 未闭合），所有周期 `q=0`，无法产生 `r=1`，因此冻结帧、3 次尝试、`s=1/s=2`、重试期间不改值只能在宿主机证明；上述实板行不构成这些行为的证据。
- 本次未捕获 `BOOT/VDD/IOTEST/IOSIG/BUS`（上电行在两次捕获窗口之外），未重读 UID；镜像身份由构建 md5 + `Verify OK` 下载日志确定。

---

## 5. 静态核对

`static_refs.txt` 显示（`USER/src/measure.c`）：

| 检查 | 结果 |
|---|---|
| 触发时一次性编码 | `512: encode_frame10(mcu_uid, tempvalue, huminityvalue, send_data);` 仅在触发分支，全文件唯一调用点 |
| 冻结标志 | `513: s_pending_frame_valid = 1u;`，发送路径只在 `616` 清除，重试中不重设 |
| 单周期循环 | `606-613`：`for (attempt=0; attempt<SENSOR_RF_TX_RETRY; attempt++)`，成功 `break`，循环体只传 `send_data` |
| 长度/超时 | `607-608`：`SENSOR_RF_FRAME_LEN`(10)、`SENSOR_RF_TX_TIMEOUT_MS`(200) |
| 成功/用尽清除 | `615-617`：`report_req=0`、`s_pending_frame_valid=0`、`report_retry=0`，在循环之后无条件执行 |
| 无待发不发送 | `592-595`：`report_req==0` 直接返回，`s_last_send_result=0` |
| `S.s/S.y` 来源 | `565-566`：`out->send=s_last_send_result`、`out->retry=report_retry` |

---

## 6. 未执行与局限

1. **GXHT40 无 ACK（U01）**：实板无法产生 `q=1`/触发周期，实板侧冻结与 3 次发送未执行；这是既有硬件/装配条件，不能由宿主机结果替代为实板通过。
2. **宿主 stub 边界**：harness 真实编译 `measure.c/fw_core.c/sf_i2c.c/encrytogate.c`；`light_sample`、`gxht40_measure`、`app_um2005C_send_data_timeout`、`gxht40_diag_fetch` 为脚本化原语。RF 空口时序/电气、网关、App 不由本项验证。
3. **“不重新编码”的证据形式**：以 `--wrap` 计数 + 重试期间改写 `tempvalue/huminityvalue` 后帧不变共同证明；`send_data` 为模块内静态/全局缓冲，未做越界写检查。
4. **未构造的实板故障**：真实 433 发送失败/丢包无法在不改变硬件与网关的条件下可靠构造；`s=2` 的实板路径未执行。
5. **主循环单次事务（ITEM-005）未实现**：`main.c` 仍每轮调用 `send_data_to_gateway()`；因本项在成功后/用尽后清 `report_req`，重复调用不会重复发送（harness 的 `AFTER` 检查与实板 `s=0` 一致），但“每机会一次完整事务”本身不属于本项。
6. 未做逻辑分析仪/示波器采集；本节结论只基于 UART 原始字节、下载日志与宿主机 harness。

---

## 7. 结论

- 当前受测源码（HEAD `09d4c52`，axf md5 `789a199e…`）在触发时只编码一次并冻结 10 B 缓冲；单次调用内最多 3 次、每次 200 ms，全部发送同一字节缓冲；重试期间外部样本值变化不影响帧；成功或用尽后清除待发状态，不跨周期保留；`S.s` 为最终结果、`S.y` 为 0。
- 独立 harness 332/332 通过；父提交负向对照在同组判据上被拒绝（22 项）。实现侧全量宿主机套件 226/226（回归参考）。
- 当前镜像已实际下载（`Verify OK`）；实板 3 个失败周期均 `s=0/y=0`、无残留、节拍 180 s，独立复核 62/62 通过。
- **ITEM-003 本增量：TEST_PASS**。范围与限制如 §1/§6 所述。
