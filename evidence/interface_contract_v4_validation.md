# IC-003 v4.0 接口一致性核对

结论：接口设计静态核对完成，39/39通过。性质：system_architect.interface_architecture作者自检，不是独立产品测试/设备协议验证或验收通过。

## 1. 对象与方法

基线Git `619433e5dd3f41d69dfae9bfdf303273e506e099`，开始工作区干净；同步新增PACC-002/ALIGN-002，使用SA/SD/SARCH/AEX已验证输入。根目录无CodeGraph索引，限定文件阅读/rg；无全库遍历、索引创建或设备探测。

正式契约：`artifacts/interface_contract_v4.md`。它替代旧IC-002 v3.0在本轮的地位；旧契约和旧validation文件保留历史。PACC-002为现行验收，PACC-001不再作为下游判据。未改产品源码、需求、验收、其他专业工件、环境或权限。

复跑：`C:/Users/kason/anaconda3/python.exe evidence/interface_contract_v4_check.py`。脚本只读取限定项目文件、做本地字段/示例算术核对，生成配套JSON；不编译/执行固件或Kotlin，不构造设备连接、不调用ADB/tool_cli/MQTT/HTTP/SQL。唯一子命令为git rev-parse HEAD，timeout=10s；本次退出0，stderr为空。脚本执行本身约1秒，退出0，无后台任务。

## 2. 本次输出与证据

| 文件 | 作用 |
|---|---|
| `interface_contract_v4_check.py` | 可复跑的静态schema/示例核对脚本；不是接口实现或专业测试harness |
| `interface_contract_v4_check.json` | 当前基线、UTC检查时刻、30个所用源码/工件/脚本SHA-256及39项逐项结果 |
| `interface_contract_v4_fixtures.json` | 3组RF/BLE明文例子、4个承载长度、暗阈值邻界、14门控向量、UART行与HTTP字段例子 |

本次原始终端摘要：

```text
STATIC CHECKS 39/39
RF fixture: 01020304CEFF58022128
UART bytes: 90 46
```

这里RF是按契约CRC参数本地计算的明文，不是线上密文或发射捕获；UART字节数只描述构造的两行，不是当前固件全部输出的实测结果。

检查内容：IF01..IF08覆盖、PACC-002及PA-026..031引用、UTF-8与代码围栏；CRC8 BEEF→92、CRC16标准123456789→29B1；3组RF/BLE符号/端序与合法零例子；N=1/2/9/10的32/48/160/176B；C_dark4000的1333/1334邻界；PDR14个门控结果；S/G含CRLF不超96B；HTTP字段集合与整数time；现有light_result_t API、固定160B属性、codec前缀接受和上传DTO存在性。逐项结果见JSON，不将检查计数扩大为实现覆盖率。

## 3. 代码与契约偏差交接

| 核对发现 | 契约处理/后续责任 |
|---|---|
| fw_core方向反且35.0提前返回；measure p/H推进后打印 | IF02/08保留规范，固件修正并独立验证；本次没有修改或复测实现 |
| light.h已是结构返回、无旧滞回；配置仍未标定 | IF01使用当前API与真实标定前提，不沿用旧契约的bool/350/250现状描述 |
| 固定GATT属性与有效长度不同 | IF04按PACC-002/AD05接受完整承载前缀，不因尾部误拒；真实MTU/长读仍待验证 |
| BLE isZeroValue/bleZeroCount从0起忽略前两次连续全零读数，第三次起放行 | IF04交Android按合法零要求处理；静态发现不是已修复 |
| UART TXE/TXBUSY while无显式超时 | IF08/10交固件处理诊断故障隔离；本次不虚构已有限等待或代定超时常量 |
| y成功/用尽重置0 | 明确为既有状态计数快照，不能当总尝试数；不擅改线上诊断含义 |
| SensorReading.time注释称真实采样 | IF06/07明确接收/载荷原时间；交Android同步注释，字段不变 |
| outbox false兼有已存在与存储错误 | IF07禁止将false一概解释为成功去重；失败证据必须保留 |
| MQTT本机发布与批量订阅格式不同、发布端未知 | IF05不猜桥接、不自行发布改格式，保留U06/FB-18 |

## 4. 限制及未执行

本次没有执行硬件采集/烧录/复位、手机安装/交互、真实RF/AES/GATT、Kotlin/驱动运行、服务连接、SQL、全暗标定或温度刺激；没有任何PA产品项的新通过结论。CRC/字段例子只证明契约内部一致，源代码存在性检查不证明执行语义正确。AEX局部真实C实验和真实手机只读响应仅按其版本/范围引用，没有升级为当前独立验收。

真实GXHT40采集根因、C_dark/可分离性、网关身份/版本/长读、MQTT发布上下文、服务器运行/唯一键、产品方向/接收时间文案与报警覆盖决定均保留为明确依赖。CONTRACT_READY表示接口工件完整，不表示这些条件已关闭或风险已接受。无遗留本次诊断任务。
