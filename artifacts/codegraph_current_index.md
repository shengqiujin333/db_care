# Current CodeGraph entry

Responsible role: Code intelligence.

Scope confirmed from file_manifest.txt, the registered resource index, root readme.txt and project entry document, the previous CodeGraph handoff and each subproject codegraph.json. The product container has no root .codegraph. All three existing subproject indexes were reused; no init, rebuild, install or upgrade was needed. No unrelated directory was indexed.

Repository baseline: `e6a6f151b2c86f5e512cffac7aa1f6947797519c`. Initial Git worktree was clean. Each source root has an empty diff against this HEAD. Existing CLI: @colbymchenry/codegraph 1.6.0. Incremental synchronization reported Already up to date for all three roots.

Evidence: [raw command results and source hashes](../evidence/codegraph_validation_20261109.json). The roots array is keyed by name; each entry includes projectPath, config, source_commit, source_diff, before/sync/after/explore/files, timestamps, exit codes and stderr. Prior validation files remain historical evidence.

## sensor

- projectPath: `D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output`
- Index: `D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output\.codegraph`
- Coverage: USER/, COMMON/, UM2005C/, rfconfig.h and test/ C source/header files. Tests and mocks are included with lower priority; outer MCU SDK, other examples and Keil/IAR dependency closure are outside the graph.
- Exclude: `["MDK/RTE/", "MDK/output/", "EWARM/Debug/", "EWARM/Release/", "obj/", "build/"]`. Deprioritize: `["test/"]`.
- Latest root commit: `ccf403113f25fe405f33c2ad972b5ac8327d290f`; indexed file-set SHA-256: `7539024ded41bd802365453194b2664af353ea6fc3643e32773be1bb805d6f77`.
- Files / nodes / edges: 99 / 1255 / 3476. All indexed paths resolved within this root and were hashed.
- Sync completed UTC: 2026-11-09T13:14:41.311516+00:00; query completed UTC: 2026-11-09T13:14:41.889393+00:00.
- Database lastIndexed UTC: 2026-11-09T05:00:55.777Z; this historical value is distinct from the current synchronization and verification time.
- Actual query: `gxht40_measure sensor_decide_report light_is_dark debug_trace_init measure_sample`, maxFiles=4. Evidence: roots[name=sensor].explore.
- Minimal query: `codegraph explore -p "D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output" --max-files 1 "gxht40_measure"`.
- MCP equivalent: codegraph_explore(projectPath above, query=gxht40_measure, maxFiles=1).

Returned current-source locations and relationships:

- `measure_sample` (USER/src/measure.c:460) — 1 caller in `USER/src/measure.c`; no tests found within 3 caller hops
- `gxht40_measure` (USER/src/gxht40.c:170) — 1 caller in `USER/src/measure.c`; no tests found within 3 caller hops
- `gxht40_init` (USER/src/gxht40.c:63) — 1 caller in `USER/src/measure.c`; no tests found within 3 caller hops
- `sensor_decide_report` (USER/src/fw_core.c:110) — 4 callers in `USER/src/measure.c`; tests: `test/host_fw_core_pure_check.c`, `test/host_fw_core_verify_ev.c`, `test/host_sensor_core_test.c`
- `light_init` (USER/src/light.c:41) — 1 caller in `USER/src/measure.c`; no tests found within 3 caller hops

Additional UART/config query: `debug_trace.c sensor_config.h`, maxFiles=2, completed 2026-11-09T13:15:47.428082+00:00. Evidence: roots[name=sensor].uart_config_explore. Returned paths: `USER/src/debug_trace.c`.

## gateway

- projectPath: `D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2`
- Index: `D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2\.codegraph`
- Coverage: APP/ and Profile/ C source/header files. External HAL, BLE binary libraries, outer WCH SDK and other examples are outside the graph. This root is retained only for RF/BLE compatibility context.
- Exclude: `[".settings/", ".idea/", "obj/", "Debug/", "Release/", "build/"]`. Deprioritize: `[]`.
- Latest root commit: `4d101ae6459f89fa7f26f0d437d1d86a8be7a369`; indexed file-set SHA-256: `4e1837de0b8fe53aab8db121a884d6eb1614280e5964fd60587de31ce7cb28f9`.
- Files / nodes / edges: 34 / 508 / 1066. All indexed paths resolved within this root and were hashed.
- Sync completed UTC: 2026-11-09T13:14:41.299259+00:00; query completed UTC: 2026-11-09T13:14:41.833576+00:00.
- Database lastIndexed UTC: 2026-10-02T12:17:46.875Z; this historical value is distinct from the current synchronization and verification time.
- Actual query: `decode_frame10`, maxFiles=4. Evidence: roots[name=gateway].explore.
- Minimal query: `codegraph explore -p "D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2" --max-files 1 "decode_frame10"`.
- MCP equivalent: codegraph_explore(projectPath above, query=decode_frame10, maxFiles=1).

Returned current-source locations and relationships:

- `decode_frame10` (APP/feistel_al.c:119) — 1 caller in `APP/app_um2006A.c`; no tests found within 3 caller hops

## android

- projectPath: `D:\mypro\beiwo2\AIP\dengbei_care`
- Index: `D:\mypro\beiwo2\AIP\dengbei_care\.codegraph`
- Coverage: App Kotlin/Java, manifest, tests, properties and supported Gradle files. Embedded Paho remains as lower-priority MQTT context. Android SDK, caches, generated outputs and iOS materials are excluded.
- Exclude: `["iOS开发资料/", ".idea/", ".gradle/", "build/", "app/build/"]`. Deprioritize: `["app/src/test/", "app/src/androidTest/", "app/src/main/java/org/eclipse/paho/"]`.
- Latest root commit: `4d101ae6459f89fa7f26f0d437d1d86a8be7a369`; indexed file-set SHA-256: `c28fb65ee5cd689341d5cf42da754b04a1246a4b2196febcf4cbe979b58225c8`.
- Files / nodes / edges: 88 / 2396 / 4770. All indexed paths resolved within this root and were hashed.
- Sync completed UTC: 2026-11-09T13:14:41.322237+00:00; query completed UTC: 2026-11-09T13:14:41.918910+00:00.
- Database lastIndexed UTC: 2026-10-07T01:53:21.977Z; this historical value is distinct from the current synchronization and verification time.
- Actual query: `CloudUploadRepository GatewayFrameCodec SensorCadence`, maxFiles=4. Evidence: roots[name=android].explore.
- Minimal query: `codegraph explore -p "D:\mypro\beiwo2\AIP\dengbei_care" --max-files 1 "CloudUploadRepository"`.
- MCP equivalent: codegraph_explore(projectPath above, query=CloudUploadRepository, maxFiles=1).

Returned current-source locations and relationships:

- `SensorCadence` (app/src/main/java/com/jinyuni/dengbei_care/telemetry/SensorCadence.kt:11) — 3 callers in `app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt`; tests: `app/src/test/java/com/jinyuni/dengbei_care/telemetry/SensorCadenceTest.kt`
- `CloudUploadRepository` (app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt:28) — 11 callers in `app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt`; tests: `app/src/test/java/com/jinyuni/dengbei_care/verification/CloudUploadItem009VerificationTest.kt`, `app/src/test/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepositoryTest.kt`
- `GatewayFrameCodec` (app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayFrameCodec.kt:16) — 11 callers in `app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt`; tests: `app/src/test/java/com/jinyuni/dengbei_care/verification/GatewayFrameCodecItem001LegacyParityTest.kt`, `app/src/test/java/com/jinyuni/dengbei_care/verification/GatewayFrameCodecItem001VerificationTest.kt`, `app/src/test/java/com/jinyuni/dengbei_care/protocol/GatewayFrameCodecTest.kt`
- `upload` (app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt:40) — 1 caller in `app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt`; tested via callers: `app/src/test/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepositoryTest.kt`, `app/src/test/java/com/jinyuni/dengbei_care/verification/CloudUploadItem009VerificationTest.kt`

## Validation and maintenance

All three required local roots returned current line-numbered source and caller relationships. Status is complete, pendingRefs=0, pendingChanges all zero, worktreeMismatch=null and reindexRecommended=false. Every recorded process exited successfully with empty stderr and a 60-second timeout; no diagnostic process remains. Git ignore checks confirmed all three .codegraph locations are ignored. Databases are local and are not delivery files.

There are no unverified known required local roots. No separate backend source root was supplied in the resource index or project entry materials; connection documentation is not a backend code root. iOS, hardware documents, archives and unrelated SDK/examples were not indexed. No remote access, hardware operations, business implementation or acceptance testing was performed. Query success is only evidence of code intelligence availability.

The graphs are separate: RF/BLE/HTTP relationships do not automatically cross project boundaries. Tests and mocks must be distinguished by path. Some original C comments use non-UTF-8 encodings and appear with replacement characters in CLI output; source encoding was preserved. A graph reporting no tests within caller hops does not establish absence of tests.

Consumers must specify the exact projectPath, use returned source/call relationships directly, and synchronize after subsequent source changes:

1. `codegraph sync "<projectPath>"`
2. `codegraph status --json "<projectPath>"`
3. Run that root's minimal explore query and confirm current source, complete state, no pending changes or drift.

Refresh evidence after changes. For scope changes use `codegraph files -p "<projectPath>" --format flat --no-metadata` and maintain the local codegraph.json exclusions. Rebuild only with a demonstrated reason. Any newly supplied relevant backend source root needs separate scope review and a successful current-source query.
