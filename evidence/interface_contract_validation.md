# ContractValidationEvidence：IC-002 v3.0

范围：system_architect.interface_architecture 自检，非下游测试。结论：CONTRACT_READY；实板标定/链路/部署未验收，源码尚有偏差。本版替代旧v2.0证据。

## Project Truth 与来源

FULL_HANDOFF 已读取 file_manifest.txt、git status --short、git log -5；初始工作区干净，HEAD=367628c。git show --stat 1120524 与 git show 4220a78 -- readme.txt 核对前一契约和需求更新。Project Truth current_versions={}、current_verified_baseline=null；interfaces/architecture/requirements/approved_artifacts 目录无现行文件。

已验证上游索引指向 artifacts/codegraph_index.md。三个子工程已有CodeGraph，先执行对应根的定向explore：sensor_decide_report/light_sample、gxht40_measure/encode_frame10；relay decode_frame10/build_device_block；Android GatewayFrameCodec.kt/CloudConfig.kt/ReadingAttribution.kt/MqtttService.kt。未初始化或重建索引。sensor_config.h定向查询未返回后补读其配置；仅补读必要网表/BOM、服务器迁移段和MqtttService行。Android依据app/src/main，未使用iOS参考副本。

当前UserInput与readme一致：升温严格超过0.9℃且无光，或严格超过35℃；新增全暗阈值1/3。旧“小于”歧义已消除，不用旧降温实现反推需求。

## 可复核来源

sensor根：CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output。
relay根：CH592EVT/EVT/EXAM/BLE/beiwov2。
Android前缀：dengbei_care/app/src/main/java/com/jinyuni/dengbei_care。

| 事实或差距 | 来源 |
|---|---|
| 旧降温式及35℃整体排除 | sensor USER/src/fw_core.c:105–121 |
| 双CRC、成功才写输出 | sensor USER/src/gxht40.c:101–145 |
| 成功推进前值 | sensor USER/src/measure.c:213–230 |
| ADC全超时合成暗态 | sensor USER/src/light.c:111–120 |
| 未标定350/250、PB04 AIN11/PB05 | sensor USER/inc/sensor_config.h |
| RF字段/CRC/10B | sensor USER/src/encrytogate.c:119–135 |
| RF接收与BLE设备块 | relay APP/feistel_al.c:119–131、APP/bleencrypt.c:12–89 |
| BLE负温与大端 | Android protocol/GatewayFrameCodec.kt:134–145 |
| MQTT顺序、数量检查 | Android telemetry/ReadingAttribution.kt:68–125 |
| 新端点 | Android cloud/CloudConfig.kt:18–30、服务器迁移记录.md |
| BLE转MQTT只保留末设备 | Android MqtttService.kt:751–752 |
| FFE0/FFE2与topic | Android MqtttService.kt:122、683–686、1007–1009 |
| 光敏拓扑/器件 | sensor_hardware/pstxnet.dat:200–221与GND中J4.2；MAIN_BOARD.BOM R3=5M、J4、U9=GXHT40 |

## 确定性自检

宿主Python独立运算通过，非产品运行：
- RF前8B 01 02 03 04 CE FF 58 02，小端T=-50/RH=600；CRC16=0x2821，完整明文10B尾部21 28。
- BLE设备前8B 01 02 03 04 02 58 FF CE，大端RH=600/T=-50；单位除10一次，ID保持。
- 10个上报向量：差值8/9/10、降温、明态、光照无效、35.0℃升温分支、首笔35.0/35.1℃均通过。契约外层温湿度有效性门控始终必需。
- C_dark=1..4095的4095组整数边界均通过：ceil(C_dark/3)为暗，前一码非暗；4095→1365，4000→1334。这些不是实测标定值。
- RF字段4+2+2+2=10B；BLE为16+N×16B，温湿换位、字节序转换与负温符号保持，无光照/原因扩展。
- 最终git diff --check通过；修改范围仅契约、此证据与file_manifest.txt。工具曾因Python管道编码导致替换失败，未写文件；之后以直接补丁写入并检查最终内容。

## 决策与交接

冻结180s采样与条件上报分离，T−P>9且有效暗态或T>350；前值为上次有效采集；全暗码1/3每笔判断，光照失败不合成暗态。兼容RF/BLE及平台schema。

契约明确区分用户要求与本能力的生命周期/异常决策：启动采样、RAM前值、失败不污染温湿度保留；光照内部新增valid，不保留绕过1/3边界的旧滞回。

D-01交固件修正方向、35℃排除、旧命名和测试；D-02交固件/实板能力获取全暗有效基准、验证当前有光及全暗分类与建立时间，未标定不宣称无光已验收；D-03交Android保证批次顺序与数量、避免末设备单值错配并同步升温规则文案。

未执行实板、BLE、broker/HTTP、MySQL或全面器件手册规格验证；TLS主机名/服务在线状态未验证。参数化接口已就绪，实现、标定与集成验收属于后续负责能力。本节点没有修改产品代码、硬件、iOS、服务器或Runtime文件，没有commit/push；由Runtime接受Outcome后执行。
