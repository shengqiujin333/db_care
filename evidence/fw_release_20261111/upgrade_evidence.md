# 发布包与升级证据（RELEASE-PKG-001 rev 3）

负责角色：固件工程
时点：2026-11-13（Asia/Shanghai；工具时间戳 UTC）；rev 2 = 整包自查与证据重绑，rev 3 = 按当前人工范围与 EV-039 重发（包字节不变）
证据类别：**发布包身份 + 可复现构建 + 完整性校验 + 下载/回退说明**。不含烧录执行、不含实板功能结论。
对应发布记录：`artifacts/firmware_release.md`（FW-REL-001 **rev 3**）；整包自查：`evidence/fw_release_20261111/package_selfcheck.md`（SELFCHECK-REL-002）；本轮原始证据：`evidence/fw_release_20261113/`。

> rev 3 增量：包字节与成员摘要**未变**（§1 数字与 rev 1/2 相同）；新增 EV-038（`HIL_PASS`）与 **EV-039（`TEST_PASS`，TD-002 rev 9.0 范围）** 的同身份绑定（§3）；重跑身份复算与包完整性校验（`evidence/fw_release_20261113/package_verify_r3.txt`）；按人工范围决定显式单列 `FW-LIGHT-DEF2-ENABLE` = NOT EXECUTED（不作为返工依据）；补记产品级门禁未对齐（`FR-006`）。
> rev 2 增量（历史）：新增 EV-036/EV-037 同身份绑定与随包手册 18 类事实对照（自查文件）。


---

## 1. 包内容与摘要

| 成员 | 大小 | MD5 |
|---|---:|---|
| `fw_sensor_FD-002r8_b6a15fed.axf` | 297,380 B | `b6a15fed2ed5e3d0c4adc51ac87a4830` |
| `fw_sensor_FD-002r8_4d728dec.hex` | 75,275 B | `4d728dec9b0d28a352c46436b7586c72` |
| `PACKAGE.txt` | 3,202 B | `a58f67fcf5b760767ff5b65888c73dfb` |
| `fw_release/fw_sensor_FD-002r8.zip`（整体） | 134,791 B | `5df0c6d45ecddc8aae8bddbbcfc9cfdf` |

`evidence/fw_release_20261111/md5_list.txt` 为同一组摘要的可校验清单（`md5sum -c` 可用；其中两个 Keil 构建产物不入库，可由 `mdk_build rebuild` 复现）。

打包约定：zip 使用固定成员时间戳（1980-01-01）压缩，**连续两次打包摘要相同**；`fw_release/` 不保留散装 `.axf/.hex`（仓库 `.gitignore` 忽略这两种扩展名），二进制只随 zip 交付；目录名避开 `.gitignore` 的 `**/Release/` 规则（Windows 大小写不敏感，实测 `release/` 会被忽略）。

## 2. 完整性校验（已执行）

`verify_release_package.py` → `package_verify.txt`：

```text
zip        : fw_release\fw_sensor_FD-002r8.zip (134791 bytes)
zip md5    : 5df0c6d45ecddc8aae8bddbbcfc9cfdf (expected 5df0c6d45ecddc8aae8bddbbcfc9cfdf) OK
crc test   : OK
members    : ['fw_sensor_FD-002r8_b6a15fed.axf', 'fw_sensor_FD-002r8_4d728dec.hex', 'PACKAGE.txt']
  PACKAGE.txt                                        3202  a58f67fcf5b760767ff5b65888c73dfb  OK
  fw_sensor_FD-002r8_4d728dec.hex                   75275  4d728dec9b0d28a352c46436b7586c72  OK
  fw_sensor_FD-002r8_b6a15fed.axf                  297380  b6a15fed2ed5e3d0c4adc51ac87a4830  OK
keil Project.axf : b6a15fed2ed5e3d0c4adc51ac87a4830  OK
keil Project.hex : 4d728dec9b0d28a352c46436b7586c72  OK
RESULT: PASS
```

结论：压缩包 CRC 通过；包内三个成员 MD5 与期望一致；包内 axf/hex 摘要等于本轮重建产物（解包后可直接与 `MDK/output/exe/` 比对）。

## 3. 可复现构建（同一源版本的三次独立重建）

| 轮次 | 时间（UTC） | 构建结果 | axf / hex MD5 | 证据 |
|---|---|---|---|---|
| 固件实现 | 2026-11-10T08:39:52–08:39:53 | 0E/1W，`25964/656/116/1676` | `b6a15fed…` / `4d728dec…` | `evidence/fw_impl_20261110/mdk_build_lightdef2.out.json` |
| 嵌入式测试 EV-035 | 2026-11-11T04:38:11–04:38:13 | 0E/1W，`25964/656/116/1676` | `b6a15fed…` / `4d728dec…` | `evidence/ev035/image_identity.txt`、`mdk_rebuild.log` |
| 嵌入式 HIL EV-036 | 2026-11-11（`firmware_package_check`） | 0E/1W，`25964/656/116/1676` | `b6a15fed…` / `4d728dec…` | `evidence/test.md`（EV-036）、`evidence/ev036/` |
| 嵌入式复验 EV-037 | 2026-11-11（`firmware_check`） | 0E/1W，`25964/656/116/1676` | `b6a15fed…` / `4d728dec…` | `evidence/test.md`（EV-037）、`evidence/ev037/` |
| 嵌入式 HIL EV-038 | 2026-11-12（`firmware_package_check`） | 0E/1W，`25964/656/116/1676` | `b6a15fed…` / `4d728dec…` | `evidence/ev038/EV-038_section.md` |
| 嵌入式复验 EV-039 | 2026-11-13（`firmware_check`，TD-002 rev 9 范围） | 0E/1W，`25964/656/116/1676` | `b6a15fed…` / `4d728dec…` | `evidence/ev039/EV-039_section.md` |
| 本轮发布 rev 3 | 2026-11-13T06:58:04–06:58:07 | 0E/1W，`25964/656/116/1676` | `b6a15fed…` / `4d728dec…` | `evidence/fw_release_20261113/mdk_build_release_r3.out.json` |
| 发布 rev 1 | 2026-11-11T05:08:38–05:08:40 | 0E/1W，`25964/656/116/1676` | `b6a15fed…` / `4d728dec…` | `evidence/fw_release_20261111/mdk_build_release.out.json` |

GNU 交叉构建（辅助验证，不入发布包）：`gcc/build.sh` exit 0、0 error / 27 既有 warning、FLASH 34840 B / RAM 1816 B（`gcc_build_release.log`）。

## 4. 源版本固定

| 项 | 值 |
|---|---|
| 传感器产品源提交 | `82a7e74e5d6bae855a573e1f23f83936ba8db049` |
| 打包时 HEAD | `9f7817f`（rev 1）；rev 2 自查时 `13e737e`；rev 3 重发时 `9224053`；三次均确认产品源相对 `82a7e74` 零差异 |
| 产品源在 `82a7e74..HEAD` 的差异 | 空（`USER/MDK/EWARM/UM2005C/COMMON` 五路径逐项核对；rev 1/2/3 各核一次） |
| 工程/构建配置 | `MDK/Project.uvprojx` 未改（未出现在 `git status`） |
| 工具链固定 | ARMCLANG V6.24（Keil 保存配置）；`arm-none-eabi-gcc`（GNU 辅助验证，版本见日志） |

## 5. 升级路径说明（本产品没有 OTA）

本产品固件通过 **Keil + CW-DAPLink 有线下载**发布，不存在 OTA/Bootloader 通道，因此"升级"= 用同一工程保存配置重新烧录镜像。

- 升级前：核对 axf `b6a15fed2ed5e3d0c4adc51ac87a4830`（或包内 hex）。
- 升级方式：`mdk_flash {}`（工程保存的调试器与 Flash 算法；先编译再下载），**不得改芯片/调试器/算法**。
- 升级后判定：日志须含 `Erase Done.Programming Done.Verify OK.Application running ...`；随后按 COM43 9600 8N1 观测上电五行与周期 `S` 行。`Verify OK` 只证明回读一致。
- 回退参考：上一历史交付镜像 axf `a3b08774e764f174eac318f6771d0c42` / hex `546ce20a718fa69b40177cce0145f4b5`（源提交 `f978a01`；EV-030 记录，早于 LIGHT-DEF-2 形制）。回退属变更动作，须由获授权角色按其变更流程执行；本证据不指示在共享仓库切换提交。
- 非目标：不得用本包烧录既有网关（CH592 `beiwov2`）；网关不属本次发布范围。

## 6. 随包必须披露的未执行/延期项

| 项 | 状态 | 依据 |
|---|---|---|
| `FW-LIGHT-DEF2-ENABLE`（无光判据启用与被动双态取证） | **人工确认未执行：`APPROVED_DEFERRED` / `SKIP_CONFIRMED`（2026-11-11T03:50:42Z，「无法准备，先跳过」）**；`T` 保持 0，不写成通过，**不作为返工依据** | `evidence/test.md` EV-035/036/037/039；Runtime 人工确认记录 |
| 真实升温 `>0.9℃` 且无光 / 稳定 `>35℃` 触发与 433 实发 | 未执行（用户禁止调温；跨链路属后续联调阶段） | 同上；产品验收 N-2 |
| 平台 `/upload_data` 真实写入与回读 | 未执行（无写入授权/隔离身份） | 产品验收 N-4（不属固件域） |
| 固件发布包与构建的独立核验 | 已有 EV-036/EV-038（`HIL_PASS`）与 EV-037/**EV-039**（`TEST_PASS`，同身份） | `evidence/test.md`、`evidence/ev036`、`evidence/ev037`、`evidence/ev038`、`evidence/ev039` |
| `E2E-04` CR2032 峰值/深睡电流 | 已批准 `DEFERABLE`，未测（无电流表/SMU/示波器） | 同上 |
| 长期稳定性/失败率 | 未覆盖（EV-036/037/039 观测窗 4–7 个周期、0 条 `G`） | 同上 |
| 产品级验收/交付门禁 | **未对齐**（`FR-006`：`REPLAN_REQUIRED`；产品验收仍 `TEST_FAIL`/`EXTERNAL_CONDITION_UNEXECUTED`） | `artifacts/failure_resolution_product_stage_delivery_gate.md`、`evidence/product_acceptance_20261112_scope_recheck/PRODUCT-ACCEPTANCE-RECHECK-002.md` |

因此**交付固件中需求 4 的"升温且无光"分支不生效**（判据未启用），仅严格 `>35.0℃` 分支可用；本包不能被解释为"无光功能已完成"或"产品已通过验收"。

## 7. 边界

1. 本能力未执行任何烧录/复位/串口采集；包内二进制来自构建，与板上当前镜像是否一致需由烧录方按其流程核对（EV-036/037/038/039 已对**同一摘要**做过下载校验，但那属测试角色的证据，不是本能力的行为）。
2. 本证据不改变 `evidence/test.md`、产品验收报告或文档包的结论与范围，也不把未执行项迁移为通过。
3. 包内不包含密钥、账号、服务器凭据或 Android/iOS 产物。
4. 整包自查（含 18 类文档事实对照、身份冻结规则与文档/验收侧转交）见 `evidence/fw_release_20261111/package_selfcheck.md`；rev 3 重跑身份复算与包完整性校验（`evidence/fw_release_20261113/`）结论不变，因此未改包字节。
