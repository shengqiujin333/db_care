# 接口契约一致性检查记录：IC-002 v4.0

负责角色：系统架构。日期：2026-11-10。范围：文档和独立算术自检；不是产品测试、独立下游验收或真实链路通过。

## 1. 同步与输入

恢复此前系统分析上下文，读取最新file_manifest.txt、git status、最近五次log及597b3e6提交统计。初始工作区干净，HEAD=597b3e6c591547ec5c3d2c674da40107f5854c93。中间提交仅新增后端探索报告和原始证据等，没有产品源码变化。读取BE-EXP-004及其原始只读输出/本地桩摘要、当前系统分析、最新README和旧v3.0接口/自检。

版本身份（HEAD下Git tree）：sensor=`09bef6581f39ca50ba2ecfbaa5c77ef9accd68a5`；网关=`22c8a193daffea87408c971a53db566633f7c6ea`；Android=`3c9feeb9ce399b3ba5ac359b31090af583ce4781`。实际根分别为CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output、CH592EVT/EVT/EXAM/BLE/beiwov2、dengbei_care。未确认板上/手机安装镜像；服务无不可变部署版本。

## 2. 定向代码检查

按三个实际根先用现有CodeGraph MCP。查询gxht40_measure/measure_sample、encode_frame10、light_sample、temperature_send（定位send_data_to_gateway）；网关app_um2006A_run/build_padded_frame_blocks、simpleProfile_ReadAttrCB及头文件；Android ApiService/SensorReading/UploadPayload、GatewayFrameCodec。沿用前一轮已展示的fw_core/CloudUploadRepository等未变源码，不重复打开其已返回正文。

部分图返回裁剪，GATT读回509–602正文未展示，随后只补读该缺失区段。sensor_config.h和bleencrypt.h宏未由相关图查询返回，定点读取必要宏。图的gattprofile.h显示位置缺include，尝试Profile/gattprofile.h失败；限定Profile目录rg --files找到真实Profile/include/gattprofile.h，使用真实文件核对与摘要。该失败读取不作成功证据，后续均完成；未扩展全库搜索、建图或升级工具。

| 对象 | 当前源码事实 | 契约处理 |
|---|---|---|
| sensor gxht40.c:170、measure.c:460/489/590 | 双CRC成功才写值、前值与冻结槽、失败最多3次后释放 | 更新旧“尚需修升温/快照”的过期任务；不保证事件必达 |
| sensor light.c:147、fw_core.c:94及sensor_config.h | ADC与分类有效性分离；旧1/3、0/0未标定 | 保留UNKNOWN，数值判据待定；不恢复350/250或估算基准 |
| sensor encrytogate.c:119 | ID4/T_LE/RH_LE/CRC_LE、10B密文 | 线上不扩字段，给出负温明文向量 |
| 网关APP/app_um2006A.c:161–224 | 按绑定ID交换温湿为RH_BE/T_BE；失败计数路径清零 | 区分本笔CRC拒绝与随后占位，不把静默当失败计数 |
| 网关APP/bleencrypt.c:12/42/66 | 头16+设备16N、自定义填充 | 布局保持，不新增接收侧填充认证假设 |
| 网关Profile/gattprofile.c:509–600、Profile/include/gattprofile.h:50 | 160B属性、offset>0不支持、min(maxLen,160)、读取不清缓存 | 推导≤9设备静态上界，明确MTU/截断交接，不能把255当实用容量 |
| Android GatewayFrameCodec:96/134 | N/长度拒绝、允许额外尾块、RH_BE/T_s16BE | 负温/单位保持，不伪称现有解码器强制恰好长度 |
| Android ApiService:42–54、UploadPayload:31/57 | 字段类型、逐笔time，注释“真实采样时间”与BLE来源不一致 | 保持字段，要求同步来源注释，不发明采样时钟 |
| Android CloudUploadRepository | Boolean成功、2xx整批ack、失败统一退避 | 区分现状与AA-004待实现错误分类，真实部分成功风险保留 |

## 3. 确定性自检结果（本次实际执行）

使用工具运行独立JavaScript纯算术，没有导入或运行产品代码、安装依赖、连接设备或网络：

- CRC16逐字节/逐位独立计算，RF前8B 01 02 03 04 CE FF 58 02→CRC=0x2821，完整10B尾部21 28。
- 穷举65536个16位温度字，经LE拆字节→BE重组→二补码恢复，65536/65536保持同一有符号值；负温向量=-50→-5.0℃，RH=600→60.0%。这证明转换算术，不证明产品实现全路径。
- 12个资格向量通过：升温8/9/10、明/暗/光无效、降温、35.0℃双分支、首笔35.0/35.1℃与独立高温。外层温湿度无效必须关闭新资格；该不变量由代码/契约核对，未假称这12向量覆盖全部异常生命周期。
- GXHT40原始65 7B B1 4E 1B CC，独立CRC8得到177/204（B1/CC）；整数换算T_x10=244、RH_x10=321，与既有捕获解释一致；未重新采集。
- 静态容量算术floor(160/16)-1=9；N=1/9/10语义长度32/160/176B。未执行真实MTU协商、9设备或超容量绑定测试。
- 三条模型记录：同ID同秒相同值重试一次及不同秒同值一次，按(devId,time)集合保留2行；只是键语义示例，不证明MySQL UNIQUE/IGNORE。

## 4. 对新探索结论的消费与收窄

BE-EXP-004原始窗口2026-11-10T07:47:50Z–07:47:57Z，21命令/7.2s；本次只读本地留档，未SSH。记录支持当时端口/unit/EMQX/路由及元数据，不证明源码不可变、真实受理或提交。桩13用例/11×2xx/2×4xx/538行仅对应本地Flask/SQLite。

契约保留服务复用和D1–D6关闭条件，但不采用“已注册仍401必需凭证”或“两次2xx即可排除鉴权”的过强推断，不要求未证实的inserted响应字段。容量480行/日收窄为单180s读取路径的模型，不作为多来源/多客户端/补传情况下硬上界。缓存读不清零与接收秒入库的观察已纳入规范；保持原观测语义，不自行添加无来源的缓存代次或按值去重。

## 5. 当前边界和完成条件

最新需求没有1/3数值要求，也未给替代判据。契约明确UNKNOWN和光门失败语义，留下有来源判据的专业交接；没有进行产品语义批准、调光/调温/功耗硬件测试设计。旧EV-030整体FAIL、FW-EXP-003失败和历史局部PASS均未重写。

本能力缺serial_capture、mdk_flash/flash_device、android_test、sigrok_capture/operate_test_equipment和服务只读动作授权；未使用设备/联网入口，环境修复关闭，未安装依赖或修改权限。没有执行pyocd、复位、应用安装或生产写入。未启动后台任务；已调用工具均返回。只修改接口契约、此记录和清单，不自行提交Git。

结论：已知接口、兼容布局、错误与时间语义及未决项可直接消费；CONTRACT_READY仅指本契约交付。光门分类、真实BLE/MTU/绑定与平台写入/幂等/回读均未通过本次检查。本记录不能替代产品验收或整体发布结论。

## 附录：当前本地对象摘要

文件摘要只确认本地候选/原始记录身份，不认证远端未变化。检查时点与SHA-256如下；本记录自身由后续Git提交标识，不自引用摘要。

计算时点：2026-11-10T15:59:59.0350811+08:00

| 文件 | SHA-256 |
|---|---|
| `artifacts/interface_contract.md` | `9eeda5acc435559d9fdac3c96bd0c72ca9d5f4a9efd1e2e3a81dded6cab10171` |
| `readme.txt` | `d9e18992097814d13fbf1cc4f184de9f9d020dea1752fd889a7254559d33bac4` |
| `artifacts/system_analysis_current.md` | `672d3f51ff960dd79a098efbd6253e4a08a760cb43d2f5f88948efff6959ef5f` |
| `artifacts/backend_feasibility_current.md` | `c4d1d9a18a0e8e7e352d2380dc9e59263facaf99aacbe6d097b265466dbd889b` |
| `evidence/backend_exploration/ssh_readonly_inspection_g003.txt` | `7d4f7e3872a4dac0b5d5c1e020b8f0ff3283e754b110a154a337df16b044f870` |
| `evidence/backend_exploration/upload_contract_observations_g003.txt` | `429395cbe5d6f396659aa9e3bf2dcf9c437f2d3d85c9a4fa6e06476f0ab1813c` |
| `artifacts/codegraph_prepare_index.md` | `a60184946df93635b9569bcbad762618ac2f769755a5eab2ee4af16b43c4e52c` |
| `evidence/codegraph_prepare_validation.json` | `ec4b40c4aa2829085f2bcb8a5bc2e025e9680f29402b8ae5ecff2582fdc4b239` |
| `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/measure.c` | `aaa82d4bd42d168d4e660f0444868ae20e620b9b417b4d53b852e8ef2e97f7e7` |
| `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/light.c` | `e45bd25f80ac2cc3678666dab5c4d4a1737ed65e70bbc3bbf69cc4586054dae8` |
| `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/gxht40.c` | `250fa66939838f28bb16748a036deab3fd00d7e5a6b07f03c1c734fb538356e9` |
| `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/src/encrytogate.c` | `c88265618b91278b36981838d424d23931fddebed5bbbdeacb4365b30d7106d0` |
| `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/USER/inc/sensor_config.h` | `660ffd82989815e1fb31696b5d1905bdc6e3b719db543777bbcc404d41ada3ed` |
| `CH592EVT/EVT/EXAM/BLE/beiwov2/APP/app_um2006A.c` | `b351b44161635c0339b9be1ab3c21213d50a4520660d3c557f41d29ebabba78a` |
| `CH592EVT/EVT/EXAM/BLE/beiwov2/APP/bleencrypt.c` | `982060bfab94ee6d93c3b91942c41802d84adc22cddf87f4196166af7903d2a6` |
| `CH592EVT/EVT/EXAM/BLE/beiwov2/APP/bleencrypt.h` | `12179453f7f34f4019d77e6c2d2c320bc2869962f985f63346fd9fe654e57f8f` |
| `CH592EVT/EVT/EXAM/BLE/beiwov2/Profile/gattprofile.c` | `1678efa31d36b662b619a791d2ebd5c902e454c2c4f743ac372995d08b269790` |
| `CH592EVT/EVT/EXAM/BLE/beiwov2/Profile/include/gattprofile.h` | `83215c5e4f1be55b642cf304857380b92485528a1f6d1f20f5d168f0a0bd82c7` |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayFrameCodec.kt` | `76f669ad4ad88a786b3bec95d70bf76c1c8f865ff901938a626a34f84ccde464` |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/cloud/UploadPayload.kt` | `c0fff0ed06404895c52b216d4ce0c31657c2c4a6a509f29d015e6d579a6d98a9` |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt` | `c221de8da8d78ecc4b41e341daca69ece754fe8e18fd95920cdb065612b84a1c` |
| `dengbei_care/app/src/main/java/com/jinyuni/dengbei_care/ui/zhuce/ApiService.kt` | `7d9fd0d15994174f13857fff29db880edd697f156fbfc4bbc4316237aab7b444` |
