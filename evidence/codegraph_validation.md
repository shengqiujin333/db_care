# CodeGraph Validation

Engine: installed @colbymchenry/codegraph CLI 1.6.0. All commands below exited 0. These are index usability checks, not product functional tests.

Initial pendingChanges (added / modified / removed): sensor 44 / 10 / 8; Android 35 / 12 / 0; relay 0 / 0 / 0. The first sensor explore reported stale symbols and source drift, so normal incremental sync was used:

- `codegraph sync "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output"`: Synced 62 changed files; Added 44, Modified 10, Removed 8.
- `codegraph sync "dengbei_care"`: Synced 47 changed files; Added 35, Modified 12.

Successful post-sync queries:

- `codegraph explore "sensor_decide_report gxht40_measure light_sample" --max-files 2 -p "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output"`: Found 14 symbols across 2 files; returned current gxht40.c and light.c source. Located sensor_decide_report at USER/src/fw_core.c:105, gxht40_measure at USER/src/gxht40.c:101, light_sample at USER/src/light.c:86, plus measure.c caller relationships and existing test references.
- `codegraph explore "decode_frame10 build_device_block" --max-files 2 -p "CH592EVT/EVT/EXAM/BLE/beiwov2"`: Found 14 symbols across 2 files; returned APP/bleencrypt.c and APP/feistel_al.c source. Located decode_frame10 at feistel_al.c:119 with caller app_um2006A.c, and build_device_block at bleencrypt.c:42.
- `codegraph explore "GatewayFrameCodec AlarmEvaluator" --max-files 2 -p "dengbei_care"`: Found 40 symbols across 2 files; located GatewayFrameCodec at protocol/GatewayFrameCodec.kt:16 and AlarmEvaluator at telemetry/AlarmEvaluator.kt:24, with MqtttService callers and existing test references.
- `codegraph explore "app/src/main/java/com/jinyuni/dengbei_care/protocol/GatewayFrameCodec.kt" --max-files 1 -p "dengbei_care"`: Found 22 symbols across 1 file, 1 file pinned; returned current numbered source including decodeAggregatedFrame:96 and decodeDeviceBlock:134.

Final `codegraph status --json "<root>"` results:

| Component | Files | Nodes | Edges | lastIndexed (UTC) |
| --- | ---: | ---: | ---: | --- |
| Sensor | 76 | 889 | 2132 | 2026-10-07T01:53:16.221Z |
| Relay | 35 | 509 | 1066 | 2026-10-02T12:17:46.875Z |
| Android | 136 | 3127 | 5881 | 2026-10-07T01:53:21.977Z |

All initialized=true, index.state=complete, pendingRefs=0, pendingChanges={added:0,modified:0,removed:0}, worktreeMismatch=null, reindexRecommended=false.

`git check-ignore -v <each root>/.codegraph/index.db` matched .gitignore:20:.codegraph/ for all three paths. `git ls-files -- ':(glob)**/.codegraph/**'` returned no files. Existing ignore rules required no changes. No large databases are tracked. Validation covers project-local static graphs, not cross-project or external vendor dependencies.
