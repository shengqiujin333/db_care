#!/usr/bin/env python3
"""Print resource-id/class/bounds for nodes whose id or class matches given keywords."""
import sys
import xml.etree.ElementTree as ET

src = sys.argv[1]
keywords = [k.lower() for k in sys.argv[2:]]
tree = ET.parse(src)
seen = set()
for node in tree.iter("node"):
    rid = node.get("resource-id") or ""
    cls = node.get("class") or ""
    hay = (rid + " " + cls).lower()
    if any(k in hay for k in keywords):
        row = (rid, cls, node.get("bounds"), node.get("text"), node.get("clickable"))
        if row not in seen:
            seen.add(row)
            print(row)
