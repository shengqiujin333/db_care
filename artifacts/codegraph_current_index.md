# 当前 CodeGraph 入口

责任角色：代码智能。

本文件取代旧 `codegraph_dengbei11_index.md` 的当前入口用途；旧记录保留为历史。产品容器 `D:\mypro\beiwo2\AIP` 无根索引，消费者必须指定下面的子工程 projectPath，不能在容器根查询并推断子工程无索引。

本轮依据 `file_manifest.txt`、`readme.txt`、`项目地址.txt` 和已有子工程配置确定范围。使用现有 @colbymchenry/codegraph 1.6.0，未安装、升级或重建。接手时 Git 工作区干净，源码基线为 `56aba831fc74e5d416688558c44898d7b861afce`；各子工程相对 HEAD 无未提交差异。下述版本是索引验证时的源码版本，不是将来的交接提交。

## 网关

- projectPath：`D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2`
- 索引位置：`D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2\.codegraph`
- 覆盖 APP/、Profile/ 的 C 源码及头文件，共 34 文件、508 节点、1066 边。仅用于接收、解码和 BLE 兼容性查询；本轮没有修改网关。
- `codegraph.json` 排除 .settings/、.idea/、obj/、Debug/、Release/、build/；根外 SDK、HAL、预编译 BLE 库及其他示例不在此图内。
- 子工程最近提交：`4d101ae6459f89fa7f26f0d437d1d86a8be7a369`。实际查询定位 `APP/feistel_al.c:119` 的 `decode_frame10`，返回当前行号源码，并指出调用者位于 `APP/app_um2006A.c`。
- 同步完成：2026-11-09 05:00:55.548 UTC；查询完成：05:00:56 UTC 左右（精确时间见证据）。同步结果 Already up to date；lastIndexed 保持 2026-10-02T12:17:46.875Z，这不是本轮验证时间。
- 最小查询：`codegraph explore -p "D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2" --max-files 1 "decode_frame10"`
- 证据：`../evidence/codegraph_current_validation.json` 中 `roots[name=gateway]` 的 before/sync/after/explore/files/queried_source。

## 传感器

- projectPath：`D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output`
- 索引位置：`D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output\.codegraph`
- 覆盖 USER/、COMMON/、UM2005C/、rfconfig.h 和 test/ 的 C 源码及头文件，共 99 文件、1255 节点、3476 边。test/ 含 mock，与实板源码须按路径区分；配置将 test/ 降低查询优先级。
- 排除 MDK/RTE/、MDK/output/、EWARM/Debug/、EWARM/Release/、obj/、build/；整个外层标准外设 SDK、其他示例和归档不建立索引。此图不能替代 Keil/IAR 编译依赖闭包。
- 子工程最近提交：`ccf403113f25fe405f33c2ad972b5ac8327d290f`。实际查询定位 `USER/src/gxht40.c:170` 的 `gxht40_measure`，返回当前源码，关联 `USER/src/measure.c:460` 的 `measure_sample`；旧交接的该调用者行号为 452，不能沿用。
- 同步前 15 个 modified；正常 sync 更新 15 文件，未重建。同步完成：2026-11-09 05:00:56.080 UTC；lastIndexed：2026-11-09T05:00:55.777Z；查询随后完成，精确时间见证据。
- 最小查询：`codegraph explore -p "D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output" --max-files 1 "gxht40_measure"`
- 证据：`../evidence/codegraph_current_validation.json` 中 `roots[name=sensor]` 的 before/sync/after/explore/files/queried_source。

## Android

- projectPath：`D:\mypro\beiwo2\AIP\dengbei_care`
- 索引位置：`D:\mypro\beiwo2\AIP\dengbei_care\.codegraph`
- 覆盖 app/src/main/ Kotlin/Java、AndroidManifest.xml、测试及工具支持的 Gradle/属性文件，共 88 文件、2396 节点、4770 边。内嵌 Paho 库仍在图内，作为 MQTT 调用上下文；配置将测试及 Paho 路径降低优先级。
- 排除 iOS开发资料/、.idea/、.gradle/、build/、app/build/。未索引 iOS、外部 Android SDK、Gradle 依赖缓存或生成产物。
- 子工程最近提交：`4d101ae6459f89fa7f26f0d437d1d86a8be7a369`。实际查询定位 `app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt:28`，返回类及上传方法源码，并指出 MqtttService 调用者和相关测试。
- 同步完成：2026-11-09 05:00:55.589 UTC；查询随后完成，精确时间见证据。同步结果 Already up to date；lastIndexed 保持 2026-10-07T01:53:21.977Z。
- 最小查询：`codegraph explore -p "D:\mypro\beiwo2\AIP\dengbei_care" --max-files 1 "CloudUploadRepository"`
- 证据：`../evidence/codegraph_current_validation.json` 中 `roots[name=android]` 的 before/sync/after/explore/files/queried_source。

## 使用与证据边界

三个根的 sync、status、explore、files 均退出 0 且 stderr 为空；最终 index.state=complete、pendingRefs=0、pendingChanges 全零、worktreeMismatch=null、reindexRecommended=false。每个根均已取得能定位当前源码的 explore 结果，不以数据库存在或时间戳代替查询。JSON 保存命令、开始/结束时间、退出码、原始 stdout/stderr、查询源码 SHA-256/Git blob，以及按已索引文件路径和内容摘要排序后计算的集合摘要，供来源核对；并非全图导出。

已确认的三个本地相关代码根均完成验证，无遗漏的已知必需本地根。共享清单和入口资料未提供独立服务端源码根；服务器迁移说明属于配置资料，未据其地址访问远端或声称服务端代码已索引。若后续提供服务端源码根，应由代码智能按范围补充本地索引与查询证据。硬件资料、SDK/无关示例、归档和 iOS 不属于本轮索引范围。图不跨子工程自动连接 RF/BLE/HTTP 边界，不是业务、硬件、服务器运行或测试通过证据。

消费者优先调用 `codegraph_explore`，将 projectPath 设为上述精确路径，query 使用对应符号；也可运行最小 CLI 查询。直接使用返回源码和调用关系，不重复读取已返回文件。C 文件部分非 UTF-8 注释在 CLI 中显示替换字符，未修改原编码；语义核对以标识符、行号和源码摘要为依据。mock 调用和图中未发现测试不能分别等同实板路径或不存在测试。

源码变化后依次运行 `codegraph sync "<projectPath>"`、`codegraph status --json "<projectPath>"`，再执行该根最小 explore；确认 complete、无 pending/drift 并更新证据，不能直接沿用旧结果。新目录或排除项变化时另检查 `codegraph files -p "<projectPath>" --format flat --no-metadata`，按实际范围维护该根 codegraph.json，避免无故全量重建。所有命令本轮均设 60 秒上限并已退出，无遗留后台诊断任务。

三个 .codegraph/ 均被已有 Git 忽略规则覆盖；数据库仅保留本地，不提交。此次仅交付本入口文件、查询证据和清单更新，产品源码与已有配置未变。
