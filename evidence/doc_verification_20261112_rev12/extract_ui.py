#!/usr/bin/env python3
"""Extract visible app text nodes (text/content-desc/bounds/class) from a uiautomator dump."""
import sys
import xml.etree.ElementTree as ET

src, dst = sys.argv[1], sys.argv[2]
tree = ET.parse(src)
rows = []
for node in tree.iter("node"):
    pkg = node.get("package", "")
    if pkg != "com.jinyuni.dengbei_care":
        continue
    text = node.get("text", "")
    desc = node.get("content-desc", "")
    if not text and not desc:
        continue
    rows.append(
        f"text={text!r} desc={desc!r} class={node.get('class')} "
        f"clickable={node.get('clickable')} bounds={node.get('bounds')}"
    )
with open(dst, "w", encoding="utf-8") as fh:
    fh.write("\n".join(rows) + "\n")
print(f"wrote {len(rows)} nodes to {dst}")
