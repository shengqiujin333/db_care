# 验收定义对齐证据 v2（ALIGN-002）

状态：ALIGNMENT_RECORDED（定义级证据；不是产品测试证据）
能力：`product_definition.acceptance_definition`
日期：2026-10-21（UTC）
基线：Git `759d9d3`（system_architecture 提交）；接手时工作区干净
对应验收：`artifacts/product_acceptance_v2.md`（PACC-002）

---

## 1. 绑定与版本

| 对象 | sha256 | 用途 |
|---|---|---|
| `artifacts/product_acceptance_v2.md` PACC-002 | `62e9507d7ac640e2cc238b7083f7bf00271421a9de46df15e1c87ce59d0f183f` | 本文件对应的验收定义 |
| `artifacts/product_requirements.md` PDR-001 | `7e2da2e889f9342ebc81fc1c88badd2d3d4321b7f3b350eb5fdfafb3ec346d2c` | 需求权威输入 |
| `artifacts/product_specification.md` PSPEC-001 | `9bd3119af50a6e45f2705e537ab6cf2dd69f97d4e0867497e9f7dd7f29f0bd68` | 行为/接口细化 |
| `artifacts/product_scope.md` PSCOPE-001 | `26337f9333655290ea20275f498f736a268ad7475e5100af559fdd77544c5819` | 范围/持续限制 |
| `artifacts/product_acceptance.md` PACC-001 | `47db684964db399b1ca523bb4ce083062d1145e7613bf27b41237033ae338bdf` | 前版验收（历史，已被 PACC-002 修正） |
| `artifacts/system_analysis.md` SA-001 | `4c9dd46bbc959d29953f418d76e1464a5c574d9b7d47c9c1a69ed94ef8a62327` | 冲突/未知/责任 |
| `artifacts/system_architecture.md` SARCH-001 | `463ad472adf9fca288233af08dc09d9ab9b9172af5248552d580ce82f54c6659` | AD03/AD05/AD06/AD08 等 |
| `artifacts/system_decomposition.md` SD-001 | `33540eeede9fd3551f52718a657acb3a507a0d43f88437c0dbf911c332aa581a` | 组件/边界/增量 |
| `artifacts/firmware_exploration.md` FEX-001 | `cb1c02d073ee339b7aa44a0ce20ff6f5fc138f2b068ae677dc50dd3c4eb10dc6` | 采集/光照/判据/节拍事实 |
| `artifacts/architecture_exploration.md` AEX-001 | `848f7fbc240b8d31e939ed1353ca610d81acbeb9414200714da07e7d4f75e713` | Q1..Q7 兼容/观测/限制 |

**读取范围**：上述 9 份工件，加上根 `readme.txt`、`file_manifest.txt` 的必要条目、`git log`/`git status`。未重读未变化的历史运行记录，未调用设备/网络/服务器。仓库根无 `.codegraph/` 索引；本核对不涉及代码定位。

---

## 2. 中间提交与输入变化（RESUME_SYNC）

| 提交 | 能力 | 对本验收的影响 |
|---|---|---|
| `3685beb` | firmware_feasibility_exploration | 新增 FEX-001：GXHT40 NACK、光照 785/不可分离风险、判据 3/14 偏差、轨迹 `p` 语义、逻辑仪映射反了 |
| `50dd01e` | architecture_feasibility_exploration | 新增 AEX-001：Q1 编码兼容、Q2 缓存不可辨识、Q3 降温覆盖缺口、Q4 160 B 承载、Q5 MQTT 顺序、Q7 标定 |
| `009e100` | system_analysis | 新增 SA-001：SA-X01..X12 冲突/未知与必要决定 |
| `759d9d3` | system_architecture | 新增 SARCH-001/SD-001：AD03（采样/发送分离、无跨周期队列）、AD05（BLE 有效前缀）、AD06（接收观察）、AD08（同键幂等） |

结论：架构/分析没有改变产品范围，但暴露了 PACC-001 的 6 处措辞缺陷、1 处暗可分离缺失和多个未决产品语义。PACC-002 必须修正验收定义，不能原样沿用。

---

## 3. PACC-001 问题与 PACC-002 修正

| # | PACC-001 问题 | 来源 | PACC-002 修正 | 验证 |
|---:|---|---|---|---|
| 1 | PA-001 把“0 值”当失败，可能误杀合法 0.0℃/0.0%RH | SA-X09/U08 | PA-001 改为有效域内 0 合法；新增 PA-026 | 静态检查 + 语义对照 |
| 2 | 5/5 与 PR-001“≥3”关系不清 | SA-X10 | PA-001 明确 3 最低/5 稳定性；3/3 仅条件通过 | 静态检查 |
| 3 | PA-012“非法帧全部拒绝”可能误拒固定 160 B 承载 | SA-X06/U04、SARCH AD05 | PA-012/PA-028 采用有效前缀 `16*(1+N)`；尾部不解析 | 静态检查 |
| 4 | PA-014“最近上报”与缓存复读冲突 | SA-X04/U03、AEX Q2 | PA-014/PA-027 区分接收观察/源样本；文案须产品决定 | 静态检查 |
| 5 | PA-016“无重复”可能被扩大为源样本恰一次 | SA-X08/U07 | PA-016/PA-029 限定同 `(dev_id,time)` 重试；不同接收秒可不同键 | 静态检查 |
| 6 | PA-015 未说明板端过滤导致降温不可见 | SA-X05/U05、AEX Q3 | PA-015/PA-030 仅对已收到样本回归；限制须文档化 | 静态检查 |
| 7 | PA-005 未验亮/暗可分离 | FEX-001 §4、SA-X03 | PA-005/PA-031 增加 `C_dark > 3L`；不可分离交产品 | 静态检查 |
| 8 | 未明确发送重试不得跨周期积压 | SARCH AD03/SD I01 | PA-010/PA-020 明确本周期内完成、用尽结束 | 静态检查 |
| 9 | MQTT 发布端未知未注明 | AEX Q5/SA-X07 | PA-013 只验 App 侧归因，不猜发布端 | 静态检查 |
| 10 | 逻辑仪通道标签与实际相反 | FEX-001 U7 | §3.3 标为可选旁证；使用前核对映射；伪 ACK 不作证据 | 静态检查 |

---

## 4. 静态检查结果

可复跑脚本：`evidence/acceptance_definition_check_v2.py`
输出：`evidence/acceptance_definition_check_v2.out`
退出码：0
结果：**68/68 检查通过**

检查覆盖：
- 9 份输入存在且 sha256 与绑定值一致；
- AD-01..10、PA-001..031、E2E-01..09、FB-01..20 全覆盖；
- SA-X01..X12 全部有落点；
- PR-001..014 全部被引用；
- readme 1..8 全部有追溯行；
- 关键语义：合法零、BLE 前缀、接收/源样本、同键幂等、报警覆盖限制、暗可分离、无跨周期队列、不猜 MQTT 发布端、不宣称源样本恰一次；
- 无 Runtime ID/绝对路径。

---

## 5. 定义级结论

| 项 | 结论 |
|---|---|
| PACC-002 定义完整性 | PASS（68/68 静态检查） |
| PDR/PSPEC/PSCOPE 与架构/分析对齐 | PASS（SA-X01..X12 全部映射；无范围扩张） |
| PACC-001 遗留歧义 | 6 处措辞歧义 + 暗可分离缺失已在 PACC-002 修正；旧版保留历史 |
| 产品验收 | **NOT ACCEPTED**：未执行任何实机/App/服务器/真实条件测试 |
| 阻断 F 级的条件 | 真实采集恢复、全暗 `C_dark`/可分离、服务器访问、网关配对、方向/新鲜度/幂等/覆盖的产品决定 |

---

## 6. 未执行与未覆盖

本能力**未执行**：

- 实板烧录/复位/COM43 采集/逻辑分析仪采集；
- ADB/App 安装/交互；
- 固件/Android 构建；
- MQTT/HTTP/数据库请求；
- 全暗标定、温度刺激、真实总功能测试。

因此：

- PA-001..PA-031 全部为 `NOT EXECUTED`（判据已定义，结果未产生）。
- AD-01..AD-10 仅为定义级静态检查，不是产品通过。
- FEX/AEX/EV-018 的观测只作输入事实，不冒充本轮验收证据。
- 旧 PACC-001 的“定义级通过”只描述旧版验收文档，不描述产品。

---

## 7. 交付边界

- 本能力只新增/更新验收定义与对齐证据；不修改 PDR/PSPEC/PSCOPE/SA/SARCH/SD/产品源码/环境配置/权限。
- 未关闭的产品语义（FB-04/13/14/15 等）以反馈形式保留；不代替产品/用户决定。
- 设备、服务器、暗环境、温度刺激由后续授权能力/用户提供；本能力不猜测地址、凭据或扩大授权。
- 无遗留后台任务；所有命令均有限等待。
