# CodeGraph Validation

- Gateway: `codegraph explore "APP_UARTInit BLE GATT received UM2006A 433 radio forwarding" --max-files 2 -p CH592EVT/EVT/EXAM/BLE/beiwov2` 成功，命中网关 radio、UART/应用事件等符号；索引状态 OK/up to date（35 files，509 nodes）。
- Sensor: `codegraph explore "sensor measurement sf_i2c sampling threshold upload" --max-files 3` 成功，命中 `measure.c` 与 `sf_i2c.c`（31 symbols）；索引状态 OK/up to date（40 files，392 nodes）。
- Android: `codegraph explore "MQTT publish and received BLE service device report" --max-files 2 -p dengbei_care` 成功，命中 MQTT publish 与 App report 相关符号；索引状态 OK/up to date（101 files，2,276 nodes）。
- `.gitignore` 已忽略 `.codegraph/`，工作成果只记录轻量索引位置与查询验证，不纳入图数据库。
