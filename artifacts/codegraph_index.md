# CodeGraph Index

Engine: installed @colbymchenry/codegraph CLI 1.6.0, launched by C:/Users/kason/AppData/Roaming/npm/codegraph.ps1. All three existing graphs were reused; no init or full rebuild was needed.

| Product component | Local index path, relative to repository | Files / nodes / edges |
| --- | --- | --- |
| CW32L010 sensor | CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/.codegraph/ | 76 / 889 / 2132 |
| CH592 relay | CH592EVT/EVT/EXAM/BLE/beiwov2/.codegraph/ | 35 / 509 / 1066 |
| Android App | dengbei_care/.codegraph/ | 136 / 3127 / 5881 |

Scope was determined from Project Goal, file_manifest.txt, the registered project-entry material, previous index artifacts, and Git handoff (HEAD 73778cf; initially clean worktree). The sensor owns GXHT40/light sampling and reporting, the relay provides the 433MHz-to-BLE protocol path, and Android provides decoding, alarms and platform connectivity. No additional graphs were created for SDKs, unrelated examples, archives, hardware or iOS reference material. Graphs cover code inside each project root; external vendor libraries/HAL and precompiled BLE libraries are outside this scope. Cross-project relationships are not guaranteed.

The first sensor query reported stale indexed symbols. Queries did not clear pending changes automatically in this session. Normal incremental `codegraph sync` updated the sensor and Android graphs; the relay had no pending changes. If `codegraph status --json "<root>"` shows pendingChanges and a query reports source drift, run `codegraph sync "<root>"` before querying again; do not repeat init.

Query usage: `codegraph explore "symbol names or relative source path" --max-files 2 -p "<root>"`.

All final statuses: initialized=true, index.state=complete, pendingRefs=0, pendingChanges={added:0,modified:0,removed:0}, worktreeMismatch=null, reindexRecommended=false. Repository .gitignore:20 ignores .codegraph/ at all three roots; no graph files are Git-tracked. Fine-grained data stays locally in these directories.

Evidence: evidence/codegraph_validation.md. Only indexes and their handoff documents were updated; product source was unchanged. Runtime owns commit/push.
