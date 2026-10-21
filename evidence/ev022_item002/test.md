# 嵌入式验证证据（EV-022，ITEM-002 判定前快照）

能力：`embedded_tester.embedded_verification`
被测项：`TL-6c897e7b-38c9-4f2a-9d55-d216ddfaaff2-tasks-ITEM-002`
被测行为：每个采样事务开始时快照 `prev_for_decision/have_prev_for_decision`；温湿度有效时用快照判定，判定后才推进 `prev` 与最近有效湿度；失败周期不判定、不推进、不发送；`S` 行 `p/H` 来自判定前快照。
结论：**TEST_PASS（ITEM-002 本增量；宿主机逻辑 + 实板失败路径观测）**
日期：2026-10-21（UTC 02:21–02:37）

> 范围声明：本结论证明 ITEM-002 的判定前快照/推进/`S.p/H` 语义。实板当前每周期 `q=0`（GXHT40 地址无 ACK，U01 未闭合），因此“成功周期 `p` 为旧 `prev`、`t` 为本次值”只能在宿主机用真实 `measure.c` 证明；“失败周期 `q=0` 且 `prev` 不变”同时有宿主机与实板证据。**不**代表温湿度采集、光照标定、RF/网关/App/云链路或产品验收通过。

---

## 1. 受测件身份

| 项 | 值 | 来源 |
|---|---|---|
| Git HEAD | `31be53b20a23435e03aa2ef3d2f85fae1ab7f6fb`（工作区干净） | `git_head.txt` |
| 被测源码 | `USER/src/measure.c` SHA-256 `67a8747f115171a2bb4bbe385ba3c9a196b425255f6f7a54e1a822cdf9ac7309` | `source_sha256.txt` |
| 负向对照源码 | 父提交 `17a92ba` 的 `measure.c` SHA-256 `9f9f5722639ac0849585e42aa2653423af0c55d4dd8915b1100000386f20e139` | `old_measure_pre_item002.c` |
| 构建 | Keil MDK `MDK/Project.uvprojx`，ARMCLANG V6.24，`mdk_build rebuild`：0 Error / 1 既有 warning（`main.c:245 while(k--);`），Code=20168 RO=644 RW=116 ZI=1676 | `build_rebuild.log` |
| 镜像 | `Project.axf` md5 `e8c2ebcdb927930a3366761e4f2999c9`；`Project.hex` md5 `badfd8142dbf316390e258a8af389486` | `image_md5.txt` |
| 下载 | `mdk_flash`：`Erase Done. Programming Done. Verify OK. Application running ...`（10:26:35 本地） | 见 `programmed_prototype.md` |
| 目标板 | 传感器板（CW32L010F8 + GXHT40 + 光敏分压 + UM2005C）；本轮未重新读取 UID（BOOT 行在捕获窗口之外） | — |
| 调试串口 | COM43，Prolific `067B:2303`，9600 8N1 | `serial_list_ports` 枚举、两次原始捕获 |
| 宿主机编译器 | MinGW-W64 gcc 12.2.0，`-std=c11 -Wall -Wextra` | `compile_current.err` / `compile_old.err` |

镜像 md5 与实现能力记录 `artifacts/firmware_implementation_v4.md` 完全一致；`mdk_flash` 本次实际执行，`Verify OK` 证明下载校验，不证明业务功能。

---

## 2. 验证方法（独立于实现侧自检）

| 手段 | 文件 | 说明 |
|---|---|---|
| 测试能力自有 harness | `test/host_measure_snapshot_verify_item002.c` | 直接编译**当前产品** `USER/src/measure.c`、`fw_core.c`、`sf_i2c.c`、`encrytogate.c`；影子 MCU 层 `test/mock_measure_ev/` 仅替换寄存器/引脚与外设原语，不替换流程逻辑 |
| 独立参考模型 | `verify_measure_snapshot_item002.py` | 用独立 Python 重新表述“快照→判定→推进/不推进”，只读取 harness 原始逐周期输出，不调用产品代码 |
| 旧实现负向对照 | `old_measure_pre_item002.c` + `harness_old.out` | 同一 harness/同一刺激编译父提交 `measure.c`，必须被 p/H 判据拒绝，证明用例能区分新旧语义 |
| 构建复核 | `mdk_build rebuild` | 当前源码可确定性重建；资源与实现记录一致 |
| 实板观测 | `com43_capture_a.bin`、`com43_capture_b.bin` + `verify_com43_item002.py` | 下载后 COM43 原始字节；独立脚本解析并核对失败周期不变量 |
| 回归参考 | `host_tests_regression.log` | 实现侧既有宿主机套件 5 组 206 passed / 0 failed（回归参考，不作为本项独立判据） |

复现命令（在 `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/`）：

```sh
gcc -std=c11 -Wall -Wextra -I test/mock_measure_ev -I USER/inc -I UM2005C -I COMMON \
    -DVERIFY_LABEL='"current"' \
    test/host_measure_snapshot_verify_item002.c USER/src/measure.c \
    USER/src/fw_core.c USER/src/sf_i2c.c USER/src/encrytogate.c \
    -o D:/mypro/beiwo2/AIP/evidence/ev022_item002/harness_current.exe

gcc -std=c11 -Wall -Wextra -I test/mock_measure_ev -I USER/inc -I UM2005C -I COMMON \
    -DVERIFY_LABEL='"old_pre_item002"' \
    test/host_measure_snapshot_verify_item002.c \
    D:/mypro/beiwo2/AIP/evidence/ev022_item002/old_measure_pre_item002.c \
    USER/src/fw_core.c USER/src/sf_i2c.c USER/src/encrytogate.c \
    -o D:/mypro/beiwo2/AIP/evidence/ev022_item002/harness_old.exe

cd D:/mypro/beiwo2/AIP/evidence/ev022_item002
./harness_current.exe > harness_current.out
./harness_old.exe     > harness_old.out
python verify_measure_snapshot_item002.py harness_current.out
python verify_measure_snapshot_item002.py harness_old.out --expect-fail

python verify_com43_item002.py com43_capture_a.bin com43_capture_b.bin
```

`compile_current.err` / `compile_old.err` 无 harness 告警（仅有产品 `encrytogate.c` 既有 `-Wextra` 告警）；两个 harness 进程退出码 0。

---

## 3. 宿主机刺激与逐周期结果

16 个事务：10 成功、6 失败（CRC/IO/RANGE/NO_DEVICE），覆盖首次样本、连续成功、失败夹在成功之间、合法 0 值、负温、超温、光照无效/暗。原始输出见 `harness_current.out`。

| # | 刺激（st/t/h/dark/lv） | S.p/H | S.t/h | q/r/s | 判定 |
|---:|---|---|---|---|---|
| 0 | OK 250/500/1/1 | 0/0 | 250/500 | 1/0/0 | PASS：首次成功无前值，`H=0` |
| 1 | OK 260/510/1/1 | 250/1 | 260/510 | 1/1/1 | PASS：`p` 为旧 `prev`，`p!=t`，升温 10 触发 |
| 2 | CRC 999/999/1/1 | 260/1 | 260/510 | 0/0/0 | PASS：失败不推进、不判定、不发送；`t/h` 保留最近有效 |
| 3 | OK 270/520/1/1 | 260/1 | 270/520 | 1/1/1 | PASS：`p` 仍是失败前值而非推进后的 270 |
| 4 | OK 280/530/0/0 | 270/1 | 280/530 | 1/0/0 | PASS：成功但光照无效 → 不触发，仍推进 `prev` |
| 5 | IO 999/999/1/1 | 280/1 | 280/530 | 0/0/0 | PASS：失败周期零副作用 |
| 6 | OK 290/540/1/1 | 280/1 | 290/540 | 1/1/1 | PASS |
| 7 | OK 351/550/0/0 | 290/1 | 351/550 | 1/1/1 | PASS：>35.0 ℃ 独立触发 |
| 8 | CRC 999/999/1/1 | 351/1 | 351/550 | 0/0/0 | PASS |
| 9 | OK 0/0/1/1 | 351/1 | 0/0 | 1/0/0 | PASS：合法 0 值被判有效（`q=1`），不触发 |
| 10 | OK −10/600/1/1 | 0/1 | −10/600 | 1/0/0 | PASS：负温；`p` 为旧值 0 |
| 11 | RANGE 999/999/1/1 | −10/1 | −10/600 | 0/0/0 | PASS |
| 12 | OK 5/610/1/1 | −10/1 | 5/610 | 1/1/1 | PASS：升温 15 触发 |
| 13 | NO_DEVICE 999/999/1/1 | 5/1 | 5/610 | 0/0/0 | PASS |
| 14 | OK 15/620/1/1 | 5/1 | 15/620 | 1/1/1 | PASS |
| 15 | IO 999/999/1/1 | 15/1 | 15/620 | 0/0/0 | PASS |

独立参考模型汇总：**224 checks passed / 0 failed**（`verify_current.out`）。包含：

- 每个成功周期 `p == 判定前 prev`、`H == 判定前 have_prev`、`t == 本次值`；成功周期 `p != t` 的样本被实际覆盖。
- 每个失败周期 `q=0`、`p/H` 与失败前相同、`t/h` 保持最近有效值、`r=0`、`sends=0`、`report_req` 不变、`diag=1`。
- 失败周期后强制挂起上报再发送，10 B 帧与 `encode_frame10(uid, 失败前温度, 失败前湿度)` **逐字节相同**（`FRAME_AFTER_FAIL eq=1` 两例：280/530、−10/600），证明失败未推进 `prev` 与最近有效湿度。
- `sample_flag=0` 不采样、不产生轨迹、不修改轨迹输出；同一周期第二次 `sensor_trace_fetch()` 返回 0 且不写 `*out`。
- 每个事务恰好一次 `light_sample` + 一次 `gxht40_measure`；惰性初始化各一次。

### 3.1 负向对照（旧实现必须被拒绝）

`harness_old.out` 用同一 16 周期刺激运行父提交 `17a92ba` 的 `measure.c`：

- **11 项不符，全部落在 `p/H(pre-decision)`**：10 个成功周期打印推进后的值（`p == t`），首次成功周期 `H=1`；
- 失败周期的 `q=0`、`prev` 不变等检查在旧实现上同样成立（旧实现本就“成功后推进、失败不推进”）；
- 结论 `NEGATIVE_CONTROL_OK`（`verify_old.out`）。

这证明本用例针对的正是 ITEM-002 改变的语义，而不是“新旧都全过”的无效测试。

---

## 4. 实板观测（当前镜像已实际下载）

| 项 | 事实 |
|---|---|
| 下载 | `mdk_flash` 成功：`Erase Done. Programming Done. Verify OK. Application running ...` |
| 捕获 A | COM43 9600 8N1，300 s，115 B：1 个采样周期（`k=3`） |
| 捕获 B | COM43 9600 8N1，300 s，230 B：2 个采样周期（`k=6`、`k=9`），间隔 **180 s** |
| 周期内容 | 每个 `S` 后紧跟 1 条 `G`；`q=0 r=0 s=0 t=0 h=0`，`G s=2 a44=0 a45=0 rd=0 at=3` |
| 独立复核 | `verify_com43_item002.py`：**56 checks passed / 0 failed**（`verify_com43.out`） |

逐周期（原始字节见两份 `.bin`）：

```text
S k=3 p=0 H=0 V=0 o=8 n=117 x=119 a=117 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
S k=6 p=0 H=0 V=0 o=8 n=113 x=115 a=113 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
S k=9 p=0 H=0 V=0 o=8 n=109 x=111 a=109 D=0 t=0 h=0 q=0 r=0 s=0 y=0
G s=2 a44=0 a45=0 rd=0 at=3 raw=------------
```

实板可证明的部分：

- 当前镜像启动、按 3 分钟节拍运行（`k` 3→6→9）；
- 每个失败周期 `q=0`、`r=0`、`s=0`，`S.p/H` 在相邻周期保持 `0/0` 不变（失败不推进），`t/h` 保持最近有效值（本板从未有有效样本，故为 0）；
- 失败周期只追加 `G` 诊断，不产生上报/发送；
- 行格式固定、每行含 CRLF 且 ≤96 B。

实板**不能**证明的部分（如实登记，不改为通过）：

- 成功周期 `p` 为旧 `prev`、`t` 为本次值：本板 `G a44=0 a45=0`，GXHT40 对 0x44/0x45 均无 ACK（U01 未闭合），所有周期 `q=0`；该部分由 §3 的宿主机证据覆盖，不冒充实板结论。
- 本次未捕获 `BOOT/VDD/IOTEST/IOSIG/BUS`（上电行在两次捕获窗口之外），未重新读取 UID；镜像身份由构建 md5 + `Verify OK` 下载日志确定。

---

## 5. 静态核对

`static_refs.txt`（`rg` 原始输出）显示：

| 检查 | 结果 |
|---|---|
| 快照位置 | `measure.c:495-496` 在 `measure_sample()`（`502`）之前复制 `prev_for_decision/have_prev_for_decision` |
| 判定输入 | `measure.c:503` `sensor_gate_decide(true, prev_for_decision, have_prev_for_decision, ...)`，无 live `s_prev_temp_x10` |
| 推进位置 | `s_prev_temp_x10/s_have_prev/s_last_hum_x10` 仅在成功分支（`508-510`）赋值，失败分支（`513-517`）无赋值 |
| 轨迹来源 | `sensor_trace_fetch()` 的 `p/H` 读 `s_trace_prev_temp_x10/s_trace_have_prev`（`544-545`），不再读 live 值 |
| 失败副作用 | 失败分支只调用 `gxht40_diag_fetch()` 并置 `diag_valid`；不置 `report_req` |

---

## 6. 未执行与局限

1. **GXHT40 无 ACK（U01）**：实板无法产生 `q=1`，成功路径的实板判据（T-L2-06 的相邻成功行）未执行；这是既有硬件/装配条件，不是本增量缺陷，也不能由宿主机结果替代为实板通过。
2. **宿主 stub 边界**：harness 真实编译 `measure.c/fw_core.c/sf_i2c.c/encrytogate.c`；`light_sample`、`gxht40_measure`、`app_um2005C_send_data_timeout` 为脚本化原语。光照判据、GXHT40 驱动、RF 空口分别由其它用例负责；本项只验证流程快照语义。
3. **未标定**：`LIGHT_DARK_CALIBRATED=0`，实板 `V=0/D=0`；harness 直接脚本化 `light.valid/dark`，仅验证流程把这两个输入传给判定。
4. **范围外**：冻结帧/单周期 3 次发送（ITEM-003）、UART 有界（ITEM-004）、主循环/启动（ITEM-005）、标定（ITEM-006）、RF/网关/手机/云、功耗与电气均未在本项验证。
5. 未做逻辑分析仪采集；本次实板结论只基于 UART 原始字节与下载日志。
6. **“失败周期不发送”的含义**：本项证明失败周期不产生新的上报请求（`report_req` 不变、`s=0`、无发送调用）。若上一周期已置 `report_req` 且尚未发送成功，当前 `main.c` 会在后续循环（包括失败周期所在循环）继续调用 `send_data_to_gateway()` 重试该挂起帧；挂起帧的冻结/单周期 3 次语义属 ITEM-003，主循环单事务属 ITEM-005，本项未判其合格。

---

## 7. 结论

- 当前受测源码（HEAD `31be53b`，axf md5 `e8c2ebcd…`）的每个采样事务在任何测量前快照 `prev/have_prev`；成功周期用快照判定、判定后推进；失败周期不判定、不推进、不发送；`S.p/H` 来自判定前快照（`p != t` 合法）。
- 独立 harness 224/224 通过；旧实现负向对照恰在 `p/H` 判据上被拒绝（11 项）。
- 当前镜像已实际下载（`Verify OK`）；实板 3 个失败周期全部 `q=0` 且 `prev` 不变，独立复核 56/56 通过。
- **ITEM-002 本增量：TEST_PASS**。实板成功路径因 U01 未闭合未执行，结论范围如 §1/§6 所述。
