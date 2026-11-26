# CodeGraph handoff: sensor, gateway and Android alarm chain

Responsible role: code intelligence.

Scope confirmed from file_manifest.txt, readme.txt, project entry paths, prior codegraph_e2e_index.md, current Git history and per-root configurations. AIP is a product container with no root index; all three relevant child roots already have indexes. Current version declarations are empty and the engineering baseline is null, so actual Git and current file hashes establish this source snapshot. This does not identify installed firmware or APK versions.

Existing @colbymchenry/codegraph 1.6.0 was used for normal incremental sync, without initialization, rebuild, installation or upgrade. All three roots are queryable. No required local root remains unverified in this scope. Platform, cloud, database services and iOS are outside the current project goal. Hardware resources are not code roots.

Source HEAD: `43fe3b37a5562156223b817651e7e416f26d94e6`. Initial takeover worktree was clean; the collector records its own new untracked file. All three source subtrees have no differences against HEAD.

Validation UTC: 2026-11-26T14:12:08.768879+00:00 to 2026-11-26T14:12:17.699709+00:00.

[Validation evidence](../evidence/codegraph_alarm_chain_20261126.json) contains per-command times, exit codes, status before/after, queries, line-numbered source excerpts, call context and source hashes. [Bounded collector](../evidence/codegraph_alarm_chain_20261126.py) records how this evidence was obtained. No whole graph or complete source dump is included.

## sensor

- projectPath: `D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output`
- indexPath: `D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output\.codegraph` (local only).
- Source subtree: `d0535fee6ade459755b270a803887770a781978f`; latest root commit: `6a749d4eb3e911441e0af171262dd5b4554f389c`.
- Coverage: USER/, COMMON/, UM2005C/, rfconfig.h and local C tests. Vendor SDK outside this child root is excluded by scope; test/ is deprioritized.
- Configured exclusions: `MDK/RTE/`, `MDK/output/`, `EWARM/Debug/`, `EWARM/Release/`, `obj/`, `build/`.
- Sync finished UTC: 2026-11-26T14:12:09.848580+00:00; status checked: 2026-11-26T14:12:10.209735+00:00.
- 99 files, 1257 nodes, 3496 edges. State complete; pendingRefs=0; pending changes=0; worktreeMismatch=null; reindexRecommended=false. Sync: T  Syncing CodeGraph
|
Scanning files...
|
•  Already up to date
|
—  Done
- Indexed source set SHA-256: `9bb96e89455f6e8d66d95bc9d843fcb521d1769f07b5da806e46948f4a9f17f8`.
- Actual queries (evidence roots[name=sensor].queries):
  - `gxht40_measure sensor_decide_report light_is_dark` located current `USER/src/fw_core.c`; timestamps, excerpts and response hash are in the corresponding evidence entry.
  - `measure_sample` located current `USER/src/measure.c`; timestamps, excerpts and response hash are in the corresponding evidence entry.

Minimal query:

```powershell
codegraph explore -p "D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output" --max-files 1 "USER/src/fw_core.c"
```

## gateway

- projectPath: `D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2`
- indexPath: `D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2\.codegraph` (local only).
- Source subtree: `22c8a193daffea87408c971a53db566633f7c6ea`; latest root commit: `4d101ae6459f89fa7f26f0d437d1d86a8be7a369`.
- Coverage: APP/ and Profile/ C application and GATT code. Parent HAL, precompiled BLE libraries, other WCH examples and SDK trees are outside the root.
- Configured exclusions: `.settings/`, `.idea/`, `obj/`, `Debug/`, `Release/`, `build/`.
- Sync finished UTC: 2026-11-26T14:12:12.063997+00:00; status checked: 2026-11-26T14:12:12.368813+00:00.
- 34 files, 508 nodes, 1066 edges. State complete; pendingRefs=0; pending changes=0; worktreeMismatch=null; reindexRecommended=false. Sync: T  Syncing CodeGraph
|
Scanning files...
|
•  Already up to date
|
—  Done
- Indexed source set SHA-256: `2d7a74f42db9cbbd4a6db67aacc32ac3abb59125810fec441225e6a33408979d`.
- Actual queries (evidence roots[name=gateway].queries):
  - `decode_frame10` located current `APP/feistel_al.c`; timestamps, excerpts and response hash are in the corresponding evidence entry.
  - `app_um2006A_run ble_send_bf` located current `APP/app_um2006A.c`; timestamps, excerpts and response hash are in the corresponding evidence entry.

Minimal query:

```powershell
codegraph explore -p "D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2" --max-files 1 "APP/feistel_al.c"
```

## android

- projectPath: `D:\mypro\beiwo2\AIP\dengbei_care`
- indexPath: `D:\mypro\beiwo2\AIP\dengbei_care\.codegraph` (local only).
- Source subtree: `7caa53518f75d8ca8823e1909f37903090654c91`; latest root commit: `a83861fcf4b360520c56223b09446ca9508547bd`.
- Coverage: Parsable local Kotlin/Java, properties and selected XML including tests and embedded Paho. Tests and Paho are deprioritized. External Android/Gradle dependencies and APKs are not indexed.
- Configured exclusions: `iOS开发资料/`, `.idea/`, `.gradle/`, `build/`, `app/build/`.
- Sync finished UTC: 2026-11-26T14:12:14.134723+00:00; status checked: 2026-11-26T14:12:14.498421+00:00.
- 113 files, 3009 nodes, 6565 edges. State complete; pendingRefs=0; pending changes=0; worktreeMismatch=null; reindexRecommended=false. Sync: T  Syncing CodeGraph
|
Scanning files...
|
•  Already up to date
|
—  Done
- Indexed source set SHA-256: `8d1f283b1d9a025ab6e979b513708840e8e3fcbd3e6510d3d61d1be9f6820e40`.
- Actual queries (evidence roots[name=android].queries):
  - `app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayFrameCodec.kt` located current `app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayFrameCodec.kt`; timestamps, excerpts and response hash are in the corresponding evidence entry.
  - `app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt` located current `app/src/main/java/com/jinyuni/dengbei_care/MqtttService.kt`; timestamps, excerpts and response hash are in the corresponding evidence entry.
  - `app/src/main/java/com/jinyuni/dengbei_care/ui/home/HomeFragment.kt` located current `app/src/main/java/com/jinyuni/dengbei_care/ui/home/HomeFragment.kt`; timestamps, excerpts and response hash are in the corresponding evidence entry.
  - `app/src/main/java/com/jinyuni/dengbei_care/BaseActivity.kt` located current `app/src/main/java/com/jinyuni/dengbei_care/BaseActivity.kt`; timestamps, excerpts and response hash are in the corresponding evidence entry.
  - `app/src/main/java/com/jinyuni/dengbei_care/RingtonePlayer.kt` located current `app/src/main/java/com/jinyuni/dengbei_care/RingtonePlayer.kt`; timestamps, excerpts and response hash are in the corresponding evidence entry.
  - `app/src/main/java/com/jinyuni/dengbei_care/VibrationPlayer.kt` located current `app/src/main/java/com/jinyuni/dengbei_care/VibrationPlayer.kt`; timestamps, excerpts and response hash are in the corresponding evidence entry.

Minimal query:

```powershell
codegraph explore -p "D:\mypro\beiwo2\AIP\dengbei_care" --max-files 1 "app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayFrameCodec.kt"
```

## Alarm and guard navigation

Android supplementary_queries records `stopAlarm stopVibration` and `processTemperatureHumidityData startAlarm`. Current graph results identify BaseActivity.kt callers of RingtonePlayer.stopAlarm (line 50) and VibrationPlayer.stopVibration (line 62), and MqtttService.kt as caller of RingtonePlayer.startAlarm (line 21). MqtttService.processTemperatureHumidityData is located at line 1083. Exact-path queries also verify HomeFragment.kt for guard controls, GatewayFrameCodec.kt for received data, and both player files. Use returned source and caller relationships to investigate; static graph associations may be imprecise (for example generic start/stop methods). They do not prove cancellation or guard behavior.

Sensor queries locate gxht40_measure, sensor_decide_report, light_is_dark and measure_sample; gateway queries locate decode_frame10 and app_um2006A_run/ble_send_bf. These are navigation evidence, not device or end-to-end verification.

## Limits and refresh

All validation commands exited 0 without timeout. No empty indexes. The indexed file lists and individual raw-byte hashes are retained; source-set hash is SHA-256 of path-sorted path + TAB + lowercase file SHA-256 + LF in UTF-8. No database files are tracked; existing Git ignore rules retain .codegraph locally. No claim is made that these new files are committed, published or archived.

Android activity_main.xml, fragment_home.xml and fragment_notifications.xml are absent from the indexed file list; read those exact resource paths directly if needed. Conditional compilation, macros, external libraries and radio/BLE runtime behavior are not proven by a static graph. No product source, device configuration, permissions or workflow was changed. No hardware testing or defect repair was performed.

Consumers must specify the exact projectPath above; never query the AIP container as though it were one index. After code, version or exclusion changes, run:

```powershell
codegraph sync "<projectPath>"
codegraph status --json "<projectPath>"
codegraph explore -p "<projectPath>" --max-files 1 "<relevant symbol or exact file>"
```

Confirm source version, no pending changes and an actual current-source hit before reusing results. No-op sync preserves lastIndexed timestamps; this handoff relies on live queries, not those historical timestamps. If ranking misses the intended file, use the exact relative path. For MCP, pass the same absolute projectPath to codegraph_explore.

Manifest existence/readback checks are recorded under evidence.handoff_validation. The optional scripts/check_file_manifest.py is absent; a bounded manifest-path existence check was used instead.
