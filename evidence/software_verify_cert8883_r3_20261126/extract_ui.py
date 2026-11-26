#!/usr/bin/env python3
"""Extract text/clickable nodes from a uiautomator XML dump (read-only)."""
import re
import sys

path = sys.argv[1]
text = open(path, encoding="utf-8", errors="replace").read()
for m in re.finditer(r"<node[^>]*>", text):
    tag = m.group(0)
    def attr(name):
        x = re.search(r'\b%s="([^"]*)"' % name, tag)
        return x.group(1) if x else ""
    t = attr("text")
    rid = attr("resource-id")
    desc = attr("content-desc")
    if t or desc or attr("clickable") == "true":
        print("text=%r rid=%r desc=%r clickable=%s bounds=%s" % (
            t, rid, desc, attr("clickable"), attr("bounds")))
