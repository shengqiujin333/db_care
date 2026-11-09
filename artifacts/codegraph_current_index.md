# Current CodeGraph entry

Responsible role: Code intelligence.

Scope was checked against the complete root readme.txt including supplementary test arrangements, file_manifest.txt, the registered three source roots, project entry documentation and each root codegraph.json. The product container D:\mypro\beiwo2\AIP has no root index: consumers must select one of the exact projectPath values below. No container-wide indexing was performed.

Source baseline: `2e3c2df6fc25b7eb0388975e5096502e5f750740`. Initial Git worktree was clean; all three source roots have no differences from HEAD. Existing @colbymchenry/codegraph 1.6.0 was reused. All incremental sync operations returned Already up to date. No init, rebuild, install or upgrade was performed. Previous evidence remains at evidence/codegraph_current_validation.json; the previous entry is preserved in Git history.

## gateway

- projectPath: `D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2`
- Index: `D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2\.codegraph`
- Coverage: APP/ and Profile/ C sources and headers. External HAL, BLE binary libraries, SDKs and other examples are outside this graph.
- Exclude: `[".settings/", ".idea/", "obj/", "Debug/", "Release/", "build/"]`. Deprioritize: `[]`.
- Files / nodes / edges: 34 / 508 / 1066. Every path from files was checked and hashed; indexed_files_sha256 identifies the sorted path/content set without exporting the graph.
- Latest source-root commit: `4d101ae6459f89fa7f26f0d437d1d86a8be7a369`. Queried source SHA-256: `b1c14ae82067a4ef5ba69d3c6bd04aa8fdec7524a07a58d28cf5c5df2d6ad300`.
- Sync finished UTC: 2026-11-09T06:17:39.489136+00:00. Explore finished UTC: 2026-11-09T06:17:39.947544+00:00.
- Database lastIndexed: 2026-10-02T12:17:46.875Z. This is not the current verification time.
- Actual current-source result: APP/feistel_al.c:119 (decode_frame10), caller in APP/app_um2006A.c.
- Minimal query: `codegraph explore -p "D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2" --max-files 1 "decode_frame10"`.
- MCP equivalent: codegraph_explore with the exact projectPath above, query=decode_frame10, maxFiles=1.
- [Raw evidence](../evidence/codegraph_verified_validation.json): roots[name=gateway], including before/sync/after/explore/files, source_commit/source_diff, queried_source and indexed_files_sha256.

## sensor

- projectPath: `D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output`
- Index: `D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output\.codegraph`
- Coverage: USER/, COMMON/, UM2005C/, rfconfig.h and test/ C sources and headers. Tests include mocks: distinguish their paths from production sources. Outer SDKs, other examples and archives are outside this graph. This is not the Keil/IAR build dependency closure.
- Exclude: `["MDK/RTE/", "MDK/output/", "EWARM/Debug/", "EWARM/Release/", "obj/", "build/"]`. Deprioritize: `["test/"]`.
- Files / nodes / edges: 99 / 1255 / 3476. Every path from files was checked and hashed; indexed_files_sha256 identifies the sorted path/content set without exporting the graph.
- Latest source-root commit: `ccf403113f25fe405f33c2ad972b5ac8327d290f`. Queried source SHA-256: `250fa66939838f28bb16748a036deab3fd00d7e5a6b07f03c1c734fb538356e9`.
- Sync finished UTC: 2026-11-09T06:17:39.511474+00:00. Explore finished UTC: 2026-11-09T06:17:39.990145+00:00.
- Database lastIndexed: 2026-11-09T05:00:55.777Z. This is not the current verification time.
- Actual current-source result: USER/src/gxht40.c:170 (gxht40_measure), related measure_sample at USER/src/measure.c:460.
- Minimal query: `codegraph explore -p "D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output" --max-files 1 "gxht40_measure"`.
- MCP equivalent: codegraph_explore with the exact projectPath above, query=gxht40_measure, maxFiles=1.
- [Raw evidence](../evidence/codegraph_verified_validation.json): roots[name=sensor], including before/sync/after/explore/files, source_commit/source_diff, queried_source and indexed_files_sha256.

## android

- projectPath: `D:\mypro\beiwo2\AIP\dengbei_care`
- Index: `D:\mypro\beiwo2\AIP\dengbei_care\.codegraph`
- Coverage: App Kotlin/Java, AndroidManifest.xml, tests and supported Gradle/properties files. Embedded Paho provides MQTT context. External Android SDKs, dependency caches and generated artifacts are excluded.
- Exclude: `["iOS开发资料/", ".idea/", ".gradle/", "build/", "app/build/"]`. Deprioritize: `["app/src/test/", "app/src/androidTest/", "app/src/main/java/org/eclipse/paho/"]`.
- Files / nodes / edges: 88 / 2396 / 4770. Every path from files was checked and hashed; indexed_files_sha256 identifies the sorted path/content set without exporting the graph.
- Latest source-root commit: `4d101ae6459f89fa7f26f0d437d1d86a8be7a369`. Queried source SHA-256: `c221de8da8d78ecc4b41e341daca69ece754fe8e18fd95920cdb065612b84a1c`.
- Sync finished UTC: 2026-11-09T06:17:39.537675+00:00. Explore finished UTC: 2026-11-09T06:17:40.045192+00:00.
- Database lastIndexed: 2026-10-07T01:53:21.977Z. This is not the current verification time.
- Actual current-source result: app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudUploadRepository.kt:28, related MqtttService and CloudUploadRepositoryTest.
- Minimal query: `codegraph explore -p "D:\mypro\beiwo2\AIP\dengbei_care" --max-files 1 "CloudUploadRepository"`.
- MCP equivalent: codegraph_explore with the exact projectPath above, query=CloudUploadRepository, maxFiles=1.
- [Raw evidence](../evidence/codegraph_verified_validation.json): roots[name=android], including before/sync/after/explore/files, source_commit/source_diff, queried_source and indexed_files_sha256.

## Validation and maintenance

All three known relevant local roots returned current line-numbered source and caller relationships through explore. Each recorded command exited 0 with empty stderr. Each index is complete, pendingRefs=0, pendingChanges all zero, worktreeMismatch=null, reindexRecommended=false. Each subprocess had a 60-second timeout and has exited; no diagnostic background process remains. Existing Git ignore rules cover all three local .codegraph directories; databases are not delivered.

There are no unverified known required local roots. No separate server source root was supplied in the resource index or project entry documents, so server code is not indexed. Server connection documentation is not source code. Hardware materials, iOS, unrelated SDKs/examples and archives are out of scope. No remote access, device operation or product-source changes were performed. This evidence does not establish firmware, Android, upload idempotency, persistence readback or server runtime acceptance.

Graphs do not automatically join RF/BLE/HTTP boundaries across projects. Mock paths are not evidence of hardware execution. Some non-UTF-8 C comments render as replacement characters; original source encoding was not changed. Use returned source and relationships directly. A graph reporting no tests does not prove tests are absent.

After source changes, run `codegraph sync "<projectPath>"`, then `codegraph status --json "<projectPath>"`, then the minimal explore query for that exact root. Confirm complete state with no pending changes or drift and refresh the evidence before reuse. After scope changes inspect `codegraph files -p "<projectPath>" --format flat --no-metadata` and maintain that root codegraph.json as needed; do not rebuild without a reason. A subsequently supplied backend source root needs its own scoped index and source-returning query.
