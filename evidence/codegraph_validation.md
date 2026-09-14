# CodeGraph Validation Evidence

Validated on 2026-09-14 with CodeGraph CLI 1.6.0.

| Product graph | Status result | Explore query | Result |
|---|---|---|---|
| Sensor firmware | complete; 257 nodes; 482 edges; 0 pending changes | `main measure sensor measurement flow` | Located `main`, `temperature_process`, `send_data_to_gateway`, and their call flow. |
| Relay firmware | complete; 509 nodes; 1,066 edges; 0 pending changes | `peripheral_main EV1527 radio receive BLE report` | Located `Peripheral_Init`, EV1527 decode symbols, sources, and callers. |
| Android application | complete; 2,276 nodes; 3,810 edges; 0 pending changes | `DeviceParamsBook parameter storage MainActivity MQTT` | Located `DeviceParamsBook`, `MqtttService`, dependent callers, and current source. |

All three `codegraph status -j <root>` and `codegraph explore -p <root> --max-files 3 <query>` commands exited successfully. `git check-ignore -v` confirmed that each `.codegraph/` database is covered by the repository `.gitignore` rule.
