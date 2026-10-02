# CodeGraph Index

本地 CodeGraph 已复用并验证以下当前需求相关代码根（索引数据库均留在对应的 `.codegraph/`，且由仓库 `.gitignore` 忽略）：

- `CH592EVT/EVT/EXAM/BLE/beiwov2/` — CH592 网关固件（433MHz 接收与 BLE 转发）。
- `CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/gpio_input_output/` — CW32L010 传感器固件（采样、温湿度/I²C 与上报判断）。
- `dengbei_care/` — Android App（BLE 数据接收与 MQTT 上报）。

三处索引均已存在；CodeGraph 1.6.0 的 `status` 对三处都返回 `[OK] Index is up to date`。无需全量重建。
