# CodeGraph Index

Engine: installed @colbymchenry/codegraph CLI 1.6.0; installed package repository is https://github.com/colbymchenry/codegraph. Current source baseline: 1dd8e4b322123ce8c2a2b1fe10afe44fcf7fbe62. Initial Git worktree was clean. This handoff supersedes the earlier b38c0af index handoff; no prior workflow queue/session is resumed.

| Component | Local index directory relative to repository | Files / nodes / edges |
| --- | --- | --- |
| CW32L010 sensor | CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/.codegraph/ | 88 / 995 / 2376 |
| CH592 gateway | CH592EVT/EVT/EXAM/BLE/beiwov2/.codegraph/ | 35 / 509 / 1066 |
| Android App | dengbei_care/.codegraph/ | 136 / 3127 / 5881 |

Scope selected from Project Goal, current readme.txt, file_manifest.txt, Git status/history, baseline change summary and the previous index handoff. Sensor and Android are current product code; the unchanged gateway is relevant for RF/BLE decoding and interoperability queries. Existing graph scopes include project-local support code and tests. Vendor SDKs/HAL outside these roots, unrelated examples, archives, generated outputs, hardware and iOS receive no separate index. External dependencies and cross-root call links are not guaranteed.

All three graphs already existed. Sensor status reported 12 added and 12 modified files; the first explore reported source drift. Incremental codegraph sync processed those 24 files; no init or full rebuild was needed. Gateway and Android required only status/query checks. Final status for every root: initialized=true, index.state=complete, pendingRefs=0, pendingChanges={added:0,modified:0,removed:0}, worktreeMismatch=null, reindexRecommended=false.

The existing .gitignore rule .codegraph/ ignores all three graph directories, and git ls-files reports no graph files tracked. No ignore-rule change was needed. Fine-grained graphs remain local; this document is a navigation handoff, not a graph export.

## Query handoff

Run codegraph explore -p "<component root>" --max-files 1 "<query>" from the repository. Reuse normal synchronization; when status/query reports pending changes or drift, run codegraph sync "<root>" and repeat the query. Do not routinely run init/index on these initialized graphs.

| Concern | Query / entry point |
| --- | --- |
| Sensor startup and RTC sampling | USER/src/main.c |
| UART1 initialization, BOOT/S output, drain/close | USER/src/debug_trace.c; debug_trace_boot, debug_trace_sample |
| Sampling and RF reporting | USER/src/measure.c; temperature_process, send_data_to_gateway, sensor_trace_fetch |
| GXHT40 acquisition | gxht40_measure; USER/src/gxht40.c:101; caller USER/src/measure.c |
| Light acquisition and dark decision | light_sample; USER/src/light.c; light_is_dark at USER/src/fw_core.c:94 |
| Report decision | sensor_decide_report at USER/src/fw_core.c:106; callers in USER/src/measure.c |
| Gateway RF decoding | decode_frame10 at APP/feistel_al.c:119; caller APP/app_um2006A.c |
| Android BLE decoding | GatewayFrameCodec at app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayFrameCodec.kt:16; callers MqtttService.kt |

Line numbers describe this baseline and may change after later edits. Index availability proves navigation usability only. GXHT40 q=0 investigation, current-build UART observation and physical dark calibration remain downstream responsibilities; historical test evidence is not current validation. The baseline report-decision source still contains a temperature-drop expression, while current readme requires a rise; this source-navigation observation is handed off without a product change or acceptance judgment.

Validation: evidence/codegraph_validation.md. Only this document, its declared evidence and their file_manifest.txt descriptions are changed. Runtime owns capability commit/push.
