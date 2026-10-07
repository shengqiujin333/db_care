# ContractValidationEvidence：IC-002 v2.0

范围：system_architect.interface_architecture 自检；不是下游软件/嵌入式测试报告。结论：结构与现有线上字段一致；上报/无光语义未冻结，REPLAN_REQUIRED。

## 接手与来源

- FULL_HANDOFF 已读取完整 file_manifest.txt、git status --short、git log -5、git show --stat -1。接手 HEAD=16c0025，初始 status 无条目。
- Project Truth current_versions={}、current_verified_baseline=null；interfaces/architecture/requirements/approved_artifacts 目录没有提供可读现行文件。
- 已验证 CodeGraphIndexArtifact 指向 artifacts/codegraph_index.md，确认三个组件索引。按索引先执行 sensor_decide_report/gxht40_measure/send_data_to_gateway/encode_frame10/light_sample、relay decode_frame10/build_device_block/app_um2006A_run、Android GatewayFrameCodec/UploadPayload/ReadingAttribution/CloudConfig 定向 explore，再读取必要的未展示源码段与网表/BOM。无需索引重建；未修改本地图或产品源码。
- 名称查询曾包含 iOS开发资料 下的参考副本；正式契约 Android 事实以 app/src/main 路径的 GatewayFrameCodec、ReadingAttribution、CloudConfig 和 MqtttService 为准，没有将参考副本当作活跃实现。

## 可复核来源定位

传感器根：CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output。
中继根：CH592EVT/EVT/EXAM/BLE/beiwov2。
Android 根：dengbei_care。

| 事实 | 依据 |
|---|---|
| 双 CRC/成功才写温湿度 | sensor USER/src/gxht40.c:101–145 |
| 成功推进前值、首值状态 | sensor USER/src/measure.c:117–125、183–230 |
| 35℃整体排除、严格下降 >9 | sensor USER/src/fw_core.c:105–121 |
| UID 前4B、小端 T/RH、前8B CRC、8轮 Feistel | sensor USER/src/encrytogate.c:13、20、119–135 |
| RF 解密后 CRC及字段读取 | relay APP/feistel_al.c:119–131 |
| BLE 头和16B设备块 | relay APP/bleencrypt.c:12–89 |
| RF 温湿度换位组装 | relay APP/app_um2006A.c 的 FR-401 组装段 |
| hum大端/有符号temp大端、帧长拒绝 | Android app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayFrameCodec.kt:16–145 |
| MQTT严格数量拒绝 | Android app/src/main/java/com/jinyuni/dengbei_care/telemetry/ReadingAttribution.kt:69–115 |
| HTTP 单位、time、空集 | Android app/src/main/java/com/jinyuni/dengbei_care/cloud/UploadPayload.kt:9–18、31–59 |
| 门控、待发箱、2xx成功 | Android app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt:79–142、200–205 |
| 新端点 | Android app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudConfig.kt:18–30；服务器迁移记录.md |
| BLE转MQTT只保留末个设备 | Android app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt 中 bleTempData=listOf(temperature)、bleHumiData=listOf(humidity) |
| 光敏引脚/器件 | sensor_hardware/pstxnet.dat 的 LIGHT_ADC/LIGTHT_POWER；MAIN_BOARD.BOM 的 U9 |
| 阈值未标定、通道配置 | sensor USER/inc/sensor_config.h LIGHT_DARK_CALIBRATED=0、ENTER=350、EXIT=250、ADC_InputCH11 |
| ADC超时当暗 | sensor USER/src/light.c:111–120 |

## 确定性自检结果

已在宿主 Python 运行 struct 解码和独立 CRC16 复算（非硬件运行）：

- RF `01 02 03 04 CE FF 58 02` 解码 T=-50、RH=600；CRC16=0x2821，完整明文 `01 02 03 04 CE FF 58 02 21 28`，长度10。
- BLE `01 02 03 04 02 58 FF CE` 解码 RH=600、T=-50；ID相同；除10一次为60.0%RH、-5.0℃。
- 聚合设备数0/1/2对应长度16/32/48；0设备由Android拒绝，长度计算不代表0设备有效遥测。
- 对比现有上报式与“严格下降且不额外限制35℃”候选：prev=360/cur=350/DARK，现有false，候选true；prev=300/cur=291，两者false；prev=300/cur=290，两者true。该运算仅证明额外35℃限制的差异，不确认0.9℃需求。
- 工件必需内容检查：R-01..R-04、REPLAN_REQUIRED、433、BLE、180、/upload_data 均存在。
- git diff --check 通过；仅换行格式提示，无空白错误。

## 未闭合条件与停止理由

R-01：原始自然语言未唯一规定0.9℃比较式，已有设计/源码的 >9 不是需求批准。
R-02：LIGHT_DARK_CALIBRATED=0，无物理无光标定证据。
R-03：光ADC故障是否允许当DARK没有明确产品决策。
R-04：单笔MQTT转发和多设备绑定归因仍有兼容风险；数量检查不能证明顺序一致。

没有运行实板、BLE、broker/HTTP或数据库测试，没有外部连接/部署；没有验证GXHT40手册全部规格、实际TLS主机名或服务器可用性。仅完成本能力工件自检。接口候选已明确来源、稳定布局、未决项和下游交接；不能把局部结构通过报告为CONTRACT_READY。

本节点只修改 artifacts/interface_contract.md、evidence/interface_contract_validation.md 和清单，不写 Runtime result.yaml，不实现其他能力。由 Runtime 接受 Outcome 后创建并 push 必需提交。
