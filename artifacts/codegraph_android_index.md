# Android 当前代码图入口

负责角色：代码智能。验证日期：2026-11-13。

本次依据共享清单、最新提交及 Android 实现记录中的 AA-006 变更同步已有索引，只声明 Android 源码查询可用。接手时 Git 工作区干净；源码基线为 `a12d12b570d541a9d348a1b6d6846275d871c7d4`，Android 子树为 `65de834726ec9103248cf70480e9485125fbf222`。上游实现自检及 APK 声明不作为本次产品验收结果。

## 入口与范围

- projectPath：`D:\mypro\beiwo2\AIP\dengbei_care`
- indexPath：`D:\mypro\beiwo2\AIP\dengbei_care\.codegraph`，数据库保留本地，不提交大型数据库。
- 产品容器 AIP 不作为索引根；沿用 Android 子工程已有索引，使用已安装的 colbymchenry/codegraph 1.6.0，没有初始化、重建、安装或升级。
- 配置来源：`dengbei_care/codegraph.json`。排除 `iOS开发资料/`、`.idea/`、`.gradle/`、`build/`、`app/build/`；测试目录和内置 Paho 源码降低排序优先级，仍可查询。覆盖实际纳入图的 Kotlin、Java、properties 及部分 XML。
- 未扩大到厂商 SDK、无关示例、归档、生成目录、iOS、硬件或服务器。传感器与网关已有独立子索引，本次未刷新验证；历史入口见 `artifacts/codegraph_firmware_initial_index.md`，不得将其旧状态视为本次就绪。

## 同步与查询证据

完整命令、UTC起止、限时、退出码、stderr、源码摘要与查询节选见 [验证证据](../evidence/codegraph_android_validation.json)。首次正常同步更新时间为 2026-11-13 04:37:28 UTC；最终复核同步为 04:39:16 UTC，返回 Already up to date。最终状态为110文件、2949节点、6345边；state=complete、pendingRefs=0、待新增/修改/删除均为0、worktreeMismatch=null、reindexRecommended=false。

本次查询均指定上述 projectPath、精确相对路径和 max-files=1，核对目标源码标题及带行号正文：

| app/src/main/java/com/jinyuni/dengbei_care/ 下的目标 | 可消费上下文 |
|---|---|
| protocol/GatewayReadState.kt | 读取终态与 DATA_OK、READ_INCOMPLETE、READ_INVALID 等状态 |
| report/ReportRegenerateFeedback.kt | 报告生成反馈的纯逻辑入口 |
| ui/notifications/NotificationsFragment.kt | 权限结果、待恢复扫描及扫描入口 |
| ui/gateway/GatewayBleSettingsDialog.kt | 网关扫描设置与回调入口 |

四次查询均成功返回当前磁盘源码；证据保存行号节选、文件 SHA-256 和完整响应摘要，不导出全图。直接使用返回源码和调用上下文，缺少符号时再定向查询，避免重复读取已返回正文。图中未展示的跨文件调用链不可据此推断已验证。

110个实际索引文件的集合 SHA-256：`2fc70aff89da5dd4660f79eeb4f6ba353d6983eaf24ce77cefa1443f0541c709`。证据含逐文件原始字节 SHA-256；集合算法为按规范化相对路径排序后，将 `path + TAB + lowercase_sha256 + LF` 的 UTF-8 拼接再计算 SHA-256。

采集过程中纠正了配置路径、explore 不支持的 --no-metadata 参数及源码行号匹配器；失败原因如实保留于证据。最终命令退出0、stderr为空，全部子进程已退出，无后台任务。普通命令上限30秒，同步45秒。

## 覆盖限制与后续使用

`app/src/main/res/layout/activity_main.xml`、`fragment_home.xml`、`fragment_notifications.xml` 仍未列入当前索引文件集合。语言列表包含XML不代表这些布局已覆盖。涉及布局时在图未命中后对这些明确路径有界读取，并由相应专业角色完成验证。当前没有其他本轮必需但未验证的代码根；上述布局限制保留，不宣称完整 Android 资源覆盖。

最小查询：

```powershell
codegraph explore "app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayReadState.kt" -p "D:\mypro\beiwo2\AIP\dengbei_care" --max-files 1
```

代码变更、切换版本或调整排除配置后，先执行 `codegraph sync "D:\mypro\beiwo2\AIP\dengbei_care"`，再执行 `codegraph status --json "D:\mypro\beiwo2\AIP\dengbei_care"` 并对相关文件重做 explore；确认版本、待变更状态及当前源码命中后再沿用结果。MCP消费者同样必须指定此 projectPath。

Git检查确认索引被忽略且没有受跟踪的数据库文件。本地产物存在、Git提交及交付包收录是独立状态，本次不声称已发布或入包。未修改产品实现、测试、手册、权限或工作流，也未执行构建、设备或服务操作。

人工确认的 FW-LIGHT-DEF2-ENABLE 保持 NOT EXECUTED，不作为本次返工依据；BOT-01 的重新评估不代表跳过或通过。原检查者应按已确认范围重评，其他真实缺陷仍由对应专业角色处理。本次索引就绪不关闭产品失败报告，不代替独立复验。
