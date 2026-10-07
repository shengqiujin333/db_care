# CodeGraph Validation

## Latest verification: run7 baseline b38c0af (2026-10-07)

FULL_HANDOFF read manifest, Project Goal, latest readme, project-entry paths and prior indexing artifacts; inspected clean Git status, five recent commits, and HEAD diff (UART1 requirement added; Flash read-protection calls commented). Installed CLI version 1.6.0; local npm package metadata identifies github.com/colbymchenry/codegraph. No software installed.

Existing graphs reused. Initial status --json succeeded for all roots: sensor pendingChanges={added:0,modified:2,removed:0}; gateway and Android all zero. Initial sensor explore "USER/src/main.c" exited 0 but warned changed-on-disk and omitted source, so it was not accepted as validation.

codegraph sync "CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output" exited 0: Synced 2 changed files; Modified: 2 - 72 nodes. No init/full rebuild.

Successful queries (all exit 0, current numbered source, no source-drift warnings; --max-files 1 -p "<root>"):

| Component | codegraph explore query | Result |
| --- | --- | --- |
| Sensor | USER/src/main.c (after sync) | 11 symbols, one pinned file; current main:208 and commented Flash calls:240-241 |
| Sensor | sensor_decide_report gxht40_measure light_sample | 7 symbols; GXHT40 source and measure.c caller relationships; report/light entry points |
| Sensor | UART1_Configure DebugUART_Close | 12 symbols; current measure.c source, GPIO UART setup and main/interrupt references |
| Gateway | decode_frame10 | 9 symbols; APP/feistel_al.c:119 and app_um2006A.c caller |
| Android | GatewayFrameCodec | 17 symbols; codec source, MqtttService callers and existing test references |

Final status checks: sensor 76 files / 889 nodes / 2131 edges, lastIndexed 2026-10-07T14:44:46.119Z; gateway 35 / 509 / 1066; Android 136 / 3127 / 5881. All initialized=true, state=complete, pendingRefs=0, pendingChanges zero, worktreeMismatch=null, reindexRecommended=false.

git check-ignore -v <each root>/.codegraph/index.db matched .gitignore:20:.codegraph/ in all three roots. git ls-files -- ':(glob)**/.codegraph/**' returned no tracked files. No ignore edit needed.

Validation covers index usability only. No product implementation, build, functional test, hardware operation or gateway modification performed. Runtime owns commit/push.

## Latest invocation verification (2026-10-07, HEAD 4220a78)

Read the shared manifest, Project Goal, readme.txt, project entry paths and existing index/evidence artifacts. Git status --porcelain was empty; Git log and git show --stat for the previous indexing commit and updated requirements commit were inspected. The same three product roots remain relevant.

Installed `codegraph --version` returned 1.6.0. All three existing graphs were reused; no installation, init or rebuild was performed. Each `codegraph status --json "<root>"` exited 0: initialized=true, state=complete, pendingRefs=0, pendingChanges={added:0,modified:0,removed:0}, worktreeMismatch=null, reindexRecommended=false. File/node/edge counts remain sensor 76/889/2132, relay 35/509/1066, Android 136/3127/5881.

Each query below exited 0 and returned current numbered source and caller relationships, with no source-drift warning:

| Root | Query (with --max-files 1 -p "<root>") | Result |
| --- | --- | --- |
| Sensor | codegraph explore "sensor_decide_report" | 7 symbols; USER/src/fw_core.c:105; measure.c callers and test references |
| Relay | codegraph explore "decode_frame10" | 9 symbols; APP/feistel_al.c:119; app_um2006A.c caller |
| Android | codegraph explore "GatewayFrameCodec" | 17 symbols; protocol/GatewayFrameCodec.kt:16; MqtttService.kt callers and test references |

`git check-ignore -v <each root>/.codegraph/index.db` matched .gitignore:20:.codegraph/ for all three roots. `git ls-files '*/.codegraph/*'` returned no tracked files. Existing ignore rules required no changes. Validation is limited to local index usability; no product implementation or functional tests were performed. Runtime owns commit/push.

## Current invocation verification (2026-10-07, HEAD 5e74c93)

The initial worktree was clean. Read file_manifest.txt, Project Goal, 项目地址.txt, existing index/evidence documents, Git log and relevant commit summaries. The three product roots remain relevant; SDKs, unrelated examples, archives, hardware and iOS references were excluded from separate indexing.

Installed CLI `codegraph --version` returned 1.6.0. All three existing `.codegraph/` directories were reused. Initial `codegraph status --json "<root>"` commands exited 0 and reported the same file/node/edge counts and lastIndexed values as the table below; initialized=true, state=complete, pendingRefs=0, pendingChanges all zero, worktreeMismatch=null and reindexRecommended=false. No initialization or synchronization was necessary.

Each current query exited 0 and returned numbered on-disk source and caller relationships without source-drift warnings:

| Root | Query (with `--max-files 1 -p "<root>"`) | Result |
| --- | --- | --- |
| Sensor | `codegraph explore "sensor_decide_report"` | 7 symbols; USER/src/fw_core.c:105; caller relationships in USER/src/measure.c and test references |
| Relay | `codegraph explore "decode_frame10"` | 9 symbols; APP/feistel_al.c:119; caller in APP/app_um2006A.c |
| Android | `codegraph explore "GatewayFrameCodec"` | 17 symbols; protocol/GatewayFrameCodec.kt:16; callers in MqtttService.kt and test references |

`git check-ignore -v <each root>/.codegraph/index.db` matched .gitignore:20:.codegraph/ for all three roots. `git ls-files -- ':(glob)**/.codegraph/**'` returned no tracked files. No ignore change was needed. This validates local index usability only; no product functional tests or implementation were performed. Runtime owns commit/push.

## Previous invocation evidence

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
