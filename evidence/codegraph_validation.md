# CodeGraph Validation Evidence

Validation date: 2026-10-08. Source HEAD: 1dd8e4b322123ce8c2a2b1fe10afe44fcf7fbe62. Startup git status --short was empty; git log -5 and git show --stat HEAD established the current baseline. Installed CLI version was 1.6.0; local package metadata identifies @colbymchenry/codegraph and https://github.com/colbymchenry/codegraph. No software installation occurred.

## Incremental synchronization

All three component .codegraph directories existed. Initial sensor status: 76 files / 889 nodes / 2131 edges; pendingChanges added=12, modified=12, removed=0. Initial sensor explore surfaced drift in fw_core.c, measure.c and light.c. Command codegraph sync CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output exited 0 and reported: Synced 24 changed files; Added: 12, Modified: 12 - 283 nodes in 211ms. No full initialization/rebuild occurred. Gateway/Android initial status already had no pending changes.

## Successful current-source queries

Every command below exited 0 and returned line-numbered on-disk source. Post-sync queries had no drift warning.

| Component | Command | Observed result |
| --- | --- | --- |
| Sensor | codegraph explore -p CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output --max-files 4 "sensor_decide_report gxht40_measure light_is_dark debug_trace" | 44 symbols across 4 files; gxht40_measure caller measure.c, sensor_decide_report callers measure.c, light_is_dark callers light.c, sensor_trace_fetch caller main.c |
| Sensor UART | codegraph explore -p CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output --max-files 1 USER/src/debug_trace.c | 10 symbols, 1 pinned file; current UART1 initialization and BOOT/S output source returned |
| Gateway | codegraph explore -p CH592EVT/EVT/EXAM/BLE/beiwov2 --max-files 1 decode_frame10 | 9 symbols across 1 file; APP/feistel_al.c:119 and caller APP/app_um2006A.c |
| Android | codegraph explore -p dengbei_care --max-files 1 GatewayFrameCodec | 17 symbols across 1 file; codec at line 16, 11 callers in MqtttService.kt and indexed tests |

## Final status and Git exclusion

codegraph status --json "<root>" exited 0 for each root after the queries.

| Root | Files | Nodes | Edges | Last indexed (UTC) |
| --- | --- | --- | --- | --- |
| CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output | 88 | 995 | 2376 | 2026-10-08T02:03:14.069Z |
| CH592EVT/EVT/EXAM/BLE/beiwov2 | 35 | 509 | 1066 | 2026-10-02T12:17:46.875Z |
| dengbei_care | 136 | 3127 | 5881 | 2026-10-07T01:53:21.977Z |

All final JSON statuses: initialized=true; index.state=complete; pendingRefs=0; pendingChanges added=0, modified=0, removed=0; worktreeMismatch=null; reindexRecommended=false; extraction version 25 matches the CLI.

Git check-ignore returned all three <root>/.codegraph/index.db paths. Git ls-files '*/.codegraph/*' returned no paths. Existing .gitignore covers all indexes; it was not modified.

This evidence establishes local index usability, not firmware, Android, hardware or calibration acceptance. Product code was not modified, compiled, flashed or tested. Old workflow evidence is background only. Runtime owns the mandatory commit/push.
