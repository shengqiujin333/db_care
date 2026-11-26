#!/usr/bin/env python3
"""Read the MySQL server greeting (no auth, no SQL) for service identity."""
import socket

HOST, PORT = "8.140.23.253", 3306
OUT = r"D:\mypro\beiwo2\AIP\evidence\e2e_joint_p0_platform_20261126\46_mysql_greeting.txt"

with socket.create_connection((HOST, PORT), timeout=10) as s:
    data = s.recv(256)

lines = []
lines.append("peer=%s:%d bytes=%d" % (HOST, PORT, len(data)))
lines.append("hex=" + data.hex())
# 3-byte little-endian packet length, 1-byte sequence id, then protocol version
if len(data) > 6:
    plen = int.from_bytes(data[0:3], "little")
    seq = data[3]
    proto = data[4]
    ver = data[5:].split(b"\x00", 1)[0].decode("ascii", "replace")
    lines.append("packet_length=%d" % plen)
    lines.append("sequence_id=%d" % seq)
    lines.append("protocol_version=%d" % proto)
    lines.append("server_version=%s" % ver)
with open(OUT, "w", encoding="utf-8") as fh:
    fh.write("\n".join(lines) + "\n")
print("\n".join(lines))
