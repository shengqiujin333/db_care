# CodeGraph Index

Engine: installed @colbymchenry/codegraph CLI 1.6.0 (package repository: https://github.com/colbymchenry/codegraph). Source baseline: b38c0afbd36871c708651e5012ba77f66b103e0b; initial Git worktree clean.

| Component | Local index directory relative to repository | Files / nodes / edges |
| --- | --- | --- |
| CW32L010 sensor | CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/.codegraph/ | 76 / 889 / 2131 |
| CH592 gateway | CH592EVT/EVT/EXAM/BLE/beiwov2/.codegraph/ | 35 / 509 / 1066 |
| Android App | dengbei_care/.codegraph/ | 136 / 3127 / 5881 |

Scope confirmed from Project Goal, file_manifest.txt, readme.txt, 项目地址.txt, existing index artifacts, Git status/log and baseline diff. Sensor and Android are current product work; gateway remains relevant as the existing RF/BLE protocol reference, with no gateway source changes authorized by this capability. No new graphs for vendor SDKs, unrelated examples, archives, generated outputs, hardware or iOS reference material. Each graph covers its existing project-local scope, including indexed tests/support code; external SDK/HAL dependencies and cross-project links are not guaranteed.

All three existing indexes were reused. Sensor initial status showed two modified files, and explore of USER/src/main.c omitted source because of drift. Normal incremental codegraph sync processed two modified files; no init or full rebuild was performed. Gateway and Android required only status/explore checks.

All final statuses: initialized=true, index.state=complete, pendingRefs=0, pendingChanges={added:0,modified:0,removed:0}, worktreeMismatch=null, reindexRecommended=false. Existing .gitignore:20 ignores every index; no graph files are tracked.

## Query handoff

Run from the repository using codegraph explore "<query>" --max-files 1 -p "<component root>", or run within a component root. Use normal auto-sync; if status/query reports drift, run codegraph sync "<root>" and repeat the query.

| Concern | Query / entry point |
| --- | --- |
| Sensor startup, RTC sampling, print retargeting | USER/src/main.c; main:208, RTC_IRQHandlerCallBack:107, RTC_Configuration:127, __write:172 |
| UART1 configuration | UART1_Configure DebugUART_Close; USER/src/measure.c; UartGPIO_Configuration:141 |
| Sampling and RF reporting | temperature_process send_data_to_gateway; USER/src/measure.c:213 / :241 |
| GXHT40 and light | gxht40_measure light_sample; USER/src/gxht40.c:101 / USER/src/light.c:86 |
| Report decision | sensor_decide_report; USER/src/fw_core.c:105 |
| Gateway RF decoding | decode_frame10; APP/feistel_al.c:119; caller APP/app_um2006A.c |
| Android BLE decoding | GatewayFrameCodec; protocol/GatewayFrameCodec.kt:16; callers MqtttService.kt |

Current queried main.c:240-241 keeps Flash read protection calls commented out; UART1_Configure and example prints at :251-256 are also commented in the main loop. These are source navigation facts only; UART initialization/debug observability and requirement conformance remain downstream engineering work.

Evidence: evidence/codegraph_validation.md. Only the two declared handoff documents and file_manifest.txt were changed; fine-grained graphs remain local and ignored. Runtime owns capability commit/push.
