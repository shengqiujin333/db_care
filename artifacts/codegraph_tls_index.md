# CodeGraph handoff for the 8883 certificate investigation

Responsible role: Code intelligence. Result: WORK_FAILED. The three existing application/firmware indexes are usable within their documented coverage. The new certificate script root is not usable: initialization and sync completed, but indexed zero files and both explore queries returned no relevant code. Do not describe this whole scope as ready.

Scope was checked against file_manifest.txt, readme.txt, project address declarations, artifacts/codegraph_e2e_index.md, the latest Git commit and evidence/e2e_p0/P0_backend_repair_scope.md. AIP is a product container without a root index. Only scripts was newly initialized; existing child indexes were incrementally synchronized. No SDK, unrelated example, archive, generated directory or evidence tree was indexed. No product implementation, certificate, trust anchor, permission or workflow was changed. No remote or device operation was performed.

Source HEAD: `5237cdb3c45f86f2583c24f954d4c99728aa5eac`. Initial worktree was clean. Installed CLI: @colbymchenry/codegraph 1.6.0. Detailed UTC sync/validation times, per-root subtree versions, latest source commits, exact commands, exit codes, source excerpts, call contexts, hashes and limitations are recorded in [validation](../evidence/codegraph_tls_20261124.json), under `roots`. The [collector](../evidence/codegraph_tls_20261124.py) records bounded queries; credential-related lines were omitted from delivered excerpts. Source excerpts were compared with current workspace line contents. The JSON is evidence, not a full graph export.

## Validated roots

Each indexPath is exactly projectPath plus `\.codegraph`. All databases remain local and Git-ignored. All three syncs reported already up to date; no pending source changes or references were reported. Source differences against HEAD were empty. Per-file source hashes and aggregate hashes are in the evidence.

| Evidence root | Actual projectPath | Source subtree at HEAD | Coverage and exclusions | Verified query |
|---|---|---|---|---|
| roots[0], sensor | D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output | d0535fee6ade459755b270a803887770a781978f | 99 files: USER, COMMON, UM2005C, local headers/tests; excludes MDK/RTE, MDK/output, EWARM Debug/Release, obj/build; tests deprioritized. Parent SDK and other examples excluded. | sensor_decide_report: USER/src/fw_core.c:112; callers in measure.c |
| roots[1], gateway | D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2 | 22c8a193daffea87408c971a53db566633f7c6ea | 34 files: APP/Profile C sources/headers; excludes .settings/.idea/obj/Debug/Release/build. Parent HAL, BLE binary libraries and SDK examples excluded. | decode_frame10: APP/feistel_al.c:119; caller in APP/app_um2006A.c |
| roots[2], android | D:\mypro\beiwo2\AIP\dengbei_care | 7caa53518f75d8ca8823e1909f37903090654c91 | 113 files: parseable local Kotlin/Java/properties/XML, tests and embedded Paho. Excludes iOS material, .idea/.gradle/build/app/build; tests/Paho deprioritized. External SDK/dependencies/APK excluded. | CloudConfig.kt; setupSSL in MqtttService.kt; CloudUploadRepository |

Minimal commands, one per usable root:

```powershell
codegraph explore -p "D:\mypro\beiwo2\AIP\CW32L010_StandardPeripheralLib_V1.0.5\Examples\sensor\gpio_input_output" --max-files 1 "sensor_decide_report"
codegraph explore -p "D:\mypro\beiwo2\AIP\CH592EVT\EVT\EXAM\BLE\beiwov2" --max-files 1 "decode_frame10"
codegraph explore -p "D:\mypro\beiwo2\AIP\dengbei_care" --max-files 1 "app/src/main/java/com/jinyuni/dengbei_care/cloud/CloudConfig.kt"
```

CloudConfig locates the current connection address and 8883/5000 constants. setupSSL locates the trust-manager and connection-option construction, with calling context. CloudUploadRepository locates upload handling and MqtttService callers. Refer to actual returned source/call relationships; static graph relationships do not establish runtime TLS, MQTT, HTTP, persistence or end-to-end success. Asset certificates and resource XML must not be assumed covered merely because XML appears in the language list. Previously reported layout coverage gaps remain.

## Incomplete roots and follow-up

`roots[3]`: projectPath `D:\mypro\beiwo2\AIP\scripts`; indexPath `D:\mypro\beiwo2\AIP\scripts\.codegraph`; source subtree `4cabeaf4760bb35ff49523cca9b7c05d7ca6537f`. Scope consists only of emqx_8883_cert_replace.sh and emqx_8883_cert_verify.sh. No custom codegraph.json. `codegraph init --yes` returned “No files found to index”. Sync/status then showed zero files, nodes and edges; lastIndexed is null. Both file queries failed to locate source. Exit 0 and index state complete do not establish usability.

Reproduce the missing coverage with:

```powershell
codegraph explore -p "D:\mypro\beiwo2\AIP\scripts" --max-files 1 "emqx_8883_cert_replace.sh"
codegraph explore -p "D:\mypro\beiwo2\AIP\scripts" --max-files 1 "emqx_8883_cert_verify.sh"
```

Next preparation step: establish actual Shell parsing support/configuration for the installed tool in the authorized project environment, then sync and repeat both source queries. No upgrade or global installation was attempted. The empty local index and failure evidence are retained. Native bounded reading can support downstream work but does not count as successful CodeGraph verification.

No independent local source root for deployed abr.py/EMQX was supplied or verified. Runtime reports and local stubs cannot stand in for that source index. No remote code was fetched. This handoff makes no claim about remote code graph readiness, CA signing availability or certificate repair.

Consumers must specify the real projectPath in CLI or MCP `codegraph_explore`. After source/config/version changes, run `codegraph sync "<projectPath>"`, `codegraph status --json "<projectPath>"`, then a relevant `codegraph explore -p "<projectPath>" --max-files 1 "<file or symbol>"`. Confirm current source/version and source hits before reusing results. Use returned source directly; narrow to exact paths when symbol ranking selects other files. Preserve language, macro, external-library and cross-device limitations.

Formal outputs are local files; local existence does not prove publication or archive delivery. Manifest read-back results are in validation.handoff_validation. No separate Git commit was created.
