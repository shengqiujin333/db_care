# 固件发布（FW-REL-001 rev 1）

负责角色：固件工程
时点：2026-11-11（Asia/Shanghai；工具时间戳为 UTC）
发布对象：蹬被报警器**传感器**固件（CW32L010，Keil MDK 工程）
本次范围：核对源版本与构建配置 → 固定工具链重建同一源版本 → 打包发布候选（二进制 + 身份 + 下载说明 + 限制）→ 绑定实际版本的测试记录。
**不负责烧录、不复位设备、不做独立实板验收**；`CANDIDATE_READY` 只表示"包可交付"，不表示功能合格，也不表示已部署。

依据：`artifacts/firmware_implementation.md`（FWI-003 rev 8）、`artifacts/firmware_design.md`（FD-002 rev 8）、`artifacts/firmware_requirements.md`（FWR-002 rev 8）、`artifacts/test_design.md`（TD-002）、`evidence/test.md`（EV-035）、`evidence/ev035/`、`CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/Project.uvprojx`（工程保存配置）。

---

## 1. 发布物

| 项 | 值 |
|---|---|
| 发布包 | `fw_release/fw_sensor_FD-002r8.zip`（134,791 B） |
| 包 MD5 | `5df0c6d45ecddc8aae8bddbbcfc9cfdf`（固定成员时间戳的确定性打包；连续两次打包同摘要） |
| 包完整性 | `evidence/fw_release_20261111/package_verify.txt`：zip CRC 测试 OK、三成员 MD5 与期望一致、且与 Keil 重建产物（`b6a15fed…`/`4d728dec…`）一致，`RESULT: PASS` |
| 成员 1 | `fw_sensor_FD-002r8_b6a15fed.axf`（297,380 B，MD5 `b6a15fed2ed5e3d0c4adc51ac87a4830`） |
| 成员 2 | `fw_sensor_FD-002r8_4d728dec.hex`（75,275 B，MD5 `4d728dec9b0d28a352c46436b7586c72`） |
| 成员 3 | `PACKAGE.txt`（3,202 B，MD5 `a58f67fcf5b760767ff5b65888c73dfb`）：包内自述的源提交、构建命令、下载说明与限制 |
| 发布说明（本文件） | `artifacts/firmware_release.md` |
| 打包证据 | `evidence/fw_release_20261111/`（`md5_list.txt`、`make_release_package.py`、`verify_release_package.py`、`package_verify.txt`、`mdk_build_release.out.json`、`gcc_build_release.log`、`args/mdk_build_release.json`） |

说明（两点工程约定）：

1. 工程构建目录 `MDK/output/` 被现有 `.gitignore` 忽略（`*.axf`/`*.hex`），因此**包内二进制以 zip 形式入库**，可被 Git 跟踪、可被测试/烧录角色直接解压取用，不依赖本机残留的构建目录；`fw_release/` 下**不保留**散装 `.axf/.hex`（否则会与清单不一致）。
2. 目录名用 `fw_release/` 而非 `release/`：仓库现有 `.gitignore` 的 `**/Release/` 规则在 Windows 大小写不敏感文件系统上会同时忽略 `release/`（`git check-ignore` 实测），改名后包可正常入库。

## 2. 源版本与构建身份

| 项 | 值 | 证据 |
|---|---|---|
| 打包时仓库 HEAD | `9f7817f`（工作树干净） | `git status --porcelain` 为空 |
| 传感器产品源提交 | `82a7e74e5d6bae855a573e1f23f83936ba8db049`（FWI-003 rev 8） | `git log -1 -- <sensor>/USER <sensor>/MDK <sensor>/EWARM` |
| 产品源自该提交是否变化 | **否**：`git diff --stat 82a7e74 HEAD -- <sensor>/USER <sensor>/MDK <sensor>/EWARM <sensor>/gcc` 为空 | 本轮实际执行，输出为空 |
| 工程/构建配置 | `MDK/Project.uvprojx`（工程保存的 Device/优化/调试器/Flash 算法），**未改** | `git status` 未列工程文件；`mdk_build` 仅做备份 |
| 编译器 | Keil MDK / ARMCLANG **V6.24**（`C:\Keil_v5\ARM\ARMCLANG\Bin`） | 构建日志首行 |
| 构建命令 | `mdk_build {"action":"rebuild"}` | `args/mdk_build_release.json` |
| 构建结果 | **0 Error / 1 Warning**（既有 `main.c(254) while(k--);` 空循环体，非本轮引入） | `mdk_build_release.out.json`（2026-11-11T05:08:38.059–05:08:40.182Z） |
| Program Size | `Code=25964 RO-data=656 RW-data=116 ZI-data=1676` | 同上 |
| GNU 交叉构建（辅助验证，不作为烧录产物） | `gcc/build.sh` exit 0，0 error / 27 既有 warning，FLASH 34840 B / RAM 1816 B | `gcc_build_release.log` |
| 目标器件 | CW32L010（工程 Device `CW32L010F8`；BOM U7 = `CW32L010Y8M6`） | 工程文件、`MAIN_BOARD.BOM`；差异保留不擅改 |

**可复现性**：同一源版本在三个独立轮次重建得到**同一** axf/hex 摘要——固件实现轮（`evidence/fw_impl_20261110/build_evidence.md`）、嵌入式测试 EV-035（`evidence/ev035/image_identity.txt`）、本轮发布（`mdk_build_release.out.json`）。

## 3. 下载说明（由具备烧录授权的角色执行；本能力不执行）

1. 使用**工程保存配置**：Keil 工程 `MDK/Project.uvprojx` + 工程保存的 CW-DAPLink 调试器 + 工程保存的 CW32L010 Flash 算法，调用 `mdk_flash {}`（先编译再下载）。**不得另选芯片、调试器、Flash 算法，不得修改工程文件**。
2. 下载日志须含 `Erase Done.Programming Done.Verify OK.Application running ...`。`Verify OK` **只证明下载内容回读校验一致**，不证明任何功能。
3. 若需与发布包逐字节对齐，可在下载前核对：axf `b6a15fed2ed5e3d0c4adc51ac87a4830`、hex `4d728dec9b0d28a352c46436b7586c72`（或解压发布包 `fw_release/fw_sensor_FD-002r8.zip` 后使用包内文件）。
4. 下载后观测（COM43 / 9600 8N1）：上电段 `BOOT→VDD→IOTEST→IOSIG→BUS` 各一次，随后每 3 分钟一条 `S` 行；交付默认（`T=0`）时 `S` 行应为 `V=0 D=0`、`q=1`。
5. **不得烧录既有网关**（CH592 `beiwov2`），不属本次发布范围；Android 侧按其自身交付物处理。

## 4. 测试记录与版本绑定（引用实际版本，不宣称功能合格）

绑定对象：`evidence/test.md` 的 **EV-035**（验证时 HEAD `cfe98a1`；其声称产品源 `git diff 82a7e74 HEAD` 为空，与本轮核对一致）；该轮独立重建并复核到**同一** axf/hex 摘要，且以 `mdk_flash {}` 下载校验 `Verify OK` 后在真实目标上采集。

| 任务/项 | EV-035 结果 | 与本发布的关系 |
|---|---|---|
| FW-REG-ASBUILT | 通过（独立重建 0E/1W，摘要逐字符一致；宿主 222/0、测试侧 harness 609/0、静态 21/21） | 支持"构建身份可复现" |
| FW-REG-REPORT | 通过（T1/T2/T3 向量） | 支持判定/冻结/有界等待行为 |
| FW-LIGHT-DEF2-MECH | 通过（形制、`T=0` 未启用等价性） | 本发布的实质变更 |
| FW-OBS-PASSIVE | 通过（6 个采样行 `q=1`、`V=0 D=0`、`k` 步长 3、行 ≤96 B、间期无输出） | 支持"默认未启用"在板上的表现 |
| FW-DIAG-BOOT | 通过（先开端口 + pyocd HW reset，同一窗口取得五行与首条 `S k=0`） | 支持上电诊断行可用 |
| FW-LIGHT-DEF2-ENABLE | **未执行（人工确认 `SKIP_CONFIRMED`，2026-11-11T03:50:42Z，「无法准备，先跳过」）** | **本交付的未执行项**；`T` 保持 0，绝不写成通过 |
| 真实升温/`>35.0℃` 触发、跨链路 | 未执行（用户禁止调温；跨链路属后续联调阶段） | 未验证项 |
| E2E-04 功耗 | 已批准 `DEFERABLE`，**未测**（无电流表/SMU/示波器） | 未测项 |

总体判定（引用 EV-035 原文）：本交付范围内已执行的必需用例 `TEST_PASS`，**无已测固件功能缺陷**；完整目标仍未通过，原因是上述未执行/延期项。本发布**不改变**该判定，也不把未执行项写成通过。

## 5. 限制与未决项（必须随包披露）

1. **需求 4 的"无光"主路径在本交付固件中不生效**：`LIGHT_DARK_THRESHOLD_CODE = 0`（判据未启用）⇒ `valid=false`、`dark=false`，升温且无光的上报分支不触发；只有**严格 >35.0℃** 分支可用。这是设计 §6.2 的安全默认，不是缺陷，但意味着当前固件**不满足**需求 4 的完整语义。
2. **判据启用前置未具备**：需要①判据形制/数值的接受记录（`artifacts/interface_contract.md` §4.2 尚未同步）；②同板件/同构建/同摆放的受光与覆盖被动双态证据。用户已明确当前不能改变光强并确认该项跳过。分离度风险（实测受光/覆盖仅约 146 码经验间隔）已在设计 §6.2(4) 记录。
3. **无 OTA / Bootloader**：本产品以有线 Keil 下载发布，包内不含空中升级路径；"升级"仅指替换镜像重新烧录。
4. **未验证**：真实升温/超温触发与 433 实发、网关/手机同笔一致性、CR2032 峰值与深睡电流、长期稳定性与失败率（EV-035 观测窗 6 个周期、0 条 `G`）。
5. **标识说明**：`BOOT fw=FD-002r4` 是字节冻结的既有横幅字符串（验证脚本按此解析），不代表版本号；本发布对应 FD-002 **rev 8** 源码，版本以源码提交与产物摘要为准。
6. **环境入口**：`pyocd 0.36.0` 已在主机安装并被 EV-035 用于 HW reset，但**未登记**在 `runtime/environment.yaml`；登记属配置所有者动作，本能力只提出建议，不自行改配置。
7. **BOM/器件命名差异**：工程 Device `CW32L010F8` 与 BOM `CW32L010Y8M6` 并存，未擅自改工程或网表。

## 6. 复现步骤

```text
# 1) 取得同一源版本（仓库当前分支；产品源提交 82a7e74，HEAD 9f7817f 对其无改动）
git diff --stat 82a7e74 HEAD -- CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER \
    CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK \
    CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/EWARM   # 期望：空

# 2) 用保存配置重建（工程为 cwd 的 mdk_build，或 Keil 手工 rebuild 同一工程）
mdk_build {"action":"rebuild"}
# 期望：0 Error / 1 既有 Warning；Code=25964 RO=656 RW=116 ZI=1676

# 3) 核对产物身份
md5sum CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/MDK/output/exe/Project.axf
# 期望：b6a15fed2ed5e3d0c4adc51ac87a4830

# 4) 重新打包并自检（可选）
python evidence/fw_release_20261111/make_release_package.py
python evidence/fw_release_20261111/verify_release_package.py
```

## 7. 交付边界与后续

- 本能力交付"候选包 + 身份 + 下载说明 + 限制"，`CANDIDATE_READY` **不等于**已烧录、已实板验收或产品合格。
- 烧录、板上复测、跨链路联调与功耗测量由对应职责按既有授权执行；本文件第 4、5 节列出必须随包披露的未执行/延期项。
- 判据启用（`T` 回填）属后续实现动作，前置为判据接受记录与被动双态证据；未具备时保持 `T=0`，本包不得被解释为"无光功能已完成"。
