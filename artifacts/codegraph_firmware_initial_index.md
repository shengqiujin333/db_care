# 当前传感器固件代码图入口

负责角色：代码智能。检查日期：2026-11-13。源码基线：`dae8008d248ce11e3e3af5b7658c967f8b4d8bcb`；传感器工程Git tree：`d0535fee6ade459755b270a803887770a781978f`。接手工作区干净，已读取共享清单、最近提交、FWI-004 rev9、当前系统分析和相关测试范围记录。

本次范围仅为当前传感器固件根的索引同步与可查询性确认。FWI-004的“无产品代码改动交付”、重建/宿主自检和实板身份是上游记录，不是本次执行或产品通过证据。相对82a7e74，当前工程目录差异为测试脚本、验证C及mock头文件；不把测试资产变化混同于产品固件逻辑修复。

## 真实入口、覆盖和排除

- projectPath：`D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output`
- indexPath：`D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output\.codegraph`
- 产品容器根无`.codegraph/`；沿用传感器已有子索引，不在容器根建图。
- 工具：已安装`@colbymchenry/codegraph 1.6.0`及Windows bundle CLI；未初始化、全量重建、安装、升级或联网下载。
- 覆盖：USER/、COMMON/、UM2005C/、rfconfig.h和本工程可解析C测试/头文件；test/降低排序优先级，不作为真实驱动或测试通过的证明。
- 配置排除：MDK/RTE/、MDK/output/、EWARM/Debug/、EWARM/Release/、obj/、build/；不扩到工程外CW32 SDK、其他示例、归档和生成产物。test/build_test.sh未作为C源码被索引。

证据：[codegraph_firmware_initial_validation.json](../evidence/codegraph_firmware_initial_validation.json)。包含源码版本、配置、实际命令、UTC起止/限时/退出码/stderr、同步前后状态、当前源码查询节选及响应摘要、99个索引文件SHA-256。先前三根检查记录保留在Git历史；当前证据保留前次记录摘要，不把旧查询当成本次查询。

## 同步与源码查询

同步窗口2026-11-13 03:45:41.446–03:45:42.188 UTC（本地11:45）。正常增量同步5个被索引文件、处理131节点；工程差异另含未解析的测试shell脚本，因此与git差异6文件数量不同。同步后99文件、1257节点、3496边，state=complete、pendingRefs=0、added/modified/removed=0、worktreeMismatch=null、reindexRecommended=false。

查询结束时间03:45:42.769–03:45:43.385 UTC，均返回当前有行号源码：

| 查询 | 实际源码位置及用途 |
|---|---|
| light_is_dark | USER/src/fw_core.c:94；threshold_code参数、99行未启用门禁及102行比较；图列出light.c调用方 |
| sensor_decide_report | USER/src/fw_core.c:112；当前上报判定及measure.c调用上下文 |
| gxht40_measure | USER/src/gxht40.c:170；测量返回码、CRC与measure_sample调用上下文 |
| trace_uart_open | USER/src/debug_trace.c:115；UART_Init:151与轨迹调用上下文 |

这些是定位和调用关系证据，不批准阈值、不证明串口初始化实测或采样/发送行为通过。直接使用图返回源码，不重复打开已展示正文；若被裁剪则继续定向查询缺失符号。

最小查询：

```powershell
codegraph explore -p "D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output" --max-files 1 "sensor_decide_report"
```

MCP等价调用必须指定上述真实projectPath，可用query=light_is_dark、maxFiles=1。消费者需核对实际返回源码文件，不能仅依概览出现某符号推定读到实现。

## 人工范围决定与原失败记录边界

当前调用明确FW-LIGHT-DEF2-ENABLE为APPROVED_DEFERRED，保持NOT EXECUTED，不重新请求遮光、重测或将其作为固件返工依据。TD-002 rev9已单列排除该项的本阶段退出门槛；PRODUCT-ACCEPTANCE-RECHECK-002明确指出原检查者将该人工跳过项计入R4未满足/失败依据属于范围错误。需要原检查者按生效范围评价剩余目标，不能据原范围错误回填3590或虚构新代码修复任务。

BOT-01的REASSESS不是跳过、就绪或新增权限；其自动测试方式及现有采集证据由测试角色负责。本次没有执行或复判该测项。原失败报告中其他未执行项和真实缺陷保持各自证据/处理范围，本能力不撤销TEST_FAIL、不判断产品通过，也不承担测试、修复或流程重排职责。

## 身份、可用性和后续同步

99个索引文件集合SHA-256：`9bb96e89455f6e8d66d95bc9d843fcb521d1769f07b5da806e46948f4a9f17f8`。逐文件为原始字节SHA-256；集合为规范化相对路径字典序排序后，将`path + TAB + lowercase_sha256 + LF`拼接为UTF-8再SHA-256。文件摘要只识别本地源码，不认证板上镜像或发布包内容。

全部CLI/Git命令前台完成、exit0、stderr空、未超时；普通命令30s、同步45s上限，无遗留后台任务。git check-ignore和git ls-files确认本地.codegraph被忽略且数据库未被跟踪，不提交大型数据库。未访问硬件或服务、未构建或运行产品测试、未修改产品实现或用户资料。

本次必需且未验证代码根：无（声明范围仅当前传感器根）。已知网关与Android各有子索引，但本次不刷新其可用性结论；对应历史入口见原Git记录及artifacts/codegraph_android_index.md。硬件、iOS、其他SDK/示例/归档/生成目录和未提供本地源码根的后端不纳入。本图不证明条件编译、外部库、RF/BLE/HTTP跨进程关系或真实环境验收。

后续代码修改、切换版本或排除配置变化后执行：

```powershell
codegraph sync "<projectPath>"
codegraph status --json "<projectPath>"
codegraph explore -p "<projectPath>" --max-files 1 "<相关符号或文件>"
```

确认当前来源、同步状态和源码查询后再沿用。覆盖清单使用`codegraph files -p "<projectPath>" --format flat --no-metadata`。

本次仅更新入口、证据和文件清单。本次变更的两个清单路径均需在工作区读回确认存在；scripts/check_file_manifest.py当前不存在，未宣称执行全库机械检查。文件本地存在、Git已跟踪和已收入交付包是独立状态，本次不核验交付归档收录，也不自行Git提交。