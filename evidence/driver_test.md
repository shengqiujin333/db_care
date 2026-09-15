# 驱动/逻辑测试证据（DT-001，L0 宿主机）

状态：固件实现证据（firmware_engineer.firmware_implementation）
依据：TD-001 §6.1.3 / FR-105/108/201/202/204/205。
运行：`test/build_test.sh` → `test/host_sensor_core_test.exe`，**56/56 通过，0 失败**。
被测：`USER/src/fw_core.c`（参数校验/换算/记录编解码）、`USER/src/history.c`（历史环/下降告警）。

## 1. 参数数值校验（FR-105，IC-001 §2）

合法向量 `[2.5,10,28,30,36,85]` 通过；以下全部拒绝：NaN、+Inf、-Inf、-0.0、temp_drop>20、hum_drop>100、temp_low<-40、temp_high>85、temp_low≥temp_high、hum_low≥hum_high；边界值（temp_drop=0、temp_low=-40）通过（共 13 例）。

## 2. float → x10 换算（FR-205）

`[2.5,10,28,30,36,85]` → `[25,100,280,300,360,850]`；`-25.0°C` → `-250`（int16 负温不回绕）。

## 3. NVM 记录编解码（FR-107/108）

build→parse 往返一致（txn=1，六值一致）；float 区 CRC 破坏拒绝；版本不符拒绝。Flash 擦写为硬件路径（cw32l010_flash），掉电中断写入用例（TD-OPT-016）留目标板验证。

## 4. 历史环与下降告警（FR-202/204）

- 25 次 push 后 count=20，idx0=最旧（回绕丢弃前 5 项，逻辑正确）；
- 下降告警：历史 30.0°C vs 当前 27.0°C、temp_drop=3.0 → 触发（`≥` 语义，恰等触发）；2.0°C 差 → 不触发；湿度 60.0%→50.0%、drop=10.0 → 触发。

## 5. 覆盖的 TD 用例（逻辑侧）

TD-SMP-007（20 样本回绕）、TD-SMP-008（下降告警边界 `≥`）、TD-SMP-009（整数域一致性由换算+判定单测联合保障）、TD-OPT-013（数值非法表驱动）。

## 6. 限制

AHT 读取失败（TD-SMP-003）、Hall 去抖（TD-TIM-004）、深睡保持（TD-TIM-003）、功耗（TD-PWR-*）等硬件相关用例需目标板验证（嵌入式测试执行能力承接）。
