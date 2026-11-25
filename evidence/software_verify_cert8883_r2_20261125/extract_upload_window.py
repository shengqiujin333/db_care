import re

SRC = "evidence/software_verify_cert8883_r2_20261125/20_logcat_capture.txt"
OUT = "evidence/software_verify_cert8883_r2_20261125/36_app_upload_window.txt"

TAGS = ("MqttService", "CloudUpload", "DataProcessor", "ZhuCeViewModel", "OkHttp")
lines = []
for raw in open(SRC, encoding="utf-8", errors="replace"):
    m = re.match(r"11-25 (16:[0-3][0-9]:[0-9]{2}\.[0-9]+) [A-Z]/([A-Za-z0-9_]+)", raw)
    if m and m.group(2) in TAGS:
        lines.append(raw.rstrip())
open(OUT, "w", encoding="utf-8").write("\n".join(lines) + "\n")
print("kept", len(lines), "lines ->", OUT)
