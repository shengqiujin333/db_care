#!/usr/bin/env python3
"""Bounded uiautomator XML parser for product acceptance recheck.

Prints visible nodes that carry text/content-desc, and every clickable node with
its bounds, so a reviewer can verify reachability without re-reading the raw XML.
No device access; read-only file parsing.
"""
import sys
import xml.etree.ElementTree as ET


def parse_bounds(b):
    # "[x1,y1][x2,y2]"
    try:
        left, rest = b.split("][")
        x1, y1 = left.lstrip("[").split(",")
        x2, y2 = rest.rstrip("]").split(",")
        return int(x1), int(y1), int(x2), int(y2)
    except Exception:
        return None


def main():
    src = sys.argv[1]
    pkg_filter = sys.argv[2] if len(sys.argv) > 2 else None
    tree = ET.parse(src)
    print("== text / content-desc nodes ==")
    for node in tree.iter("node"):
        pkg = node.get("package") or ""
        if pkg_filter and pkg and pkg != pkg_filter:
            continue
        text = (node.get("text") or "").strip()
        desc = (node.get("content-desc") or "").strip()
        if not text and not desc:
            continue
        b = node.get("bounds")
        rect = parse_bounds(b) if b else None
        print(
            "text=%r desc=%r class=%s id=%s bounds=%s clickable=%s enabled=%s"
            % (
                text,
                desc,
                node.get("class"),
                node.get("resource-id"),
                b,
                node.get("clickable"),
                node.get("enabled"),
            )
        )
        if rect:
            print("    rect=(%d,%d)-(%d,%d)" % rect)
    print("== clickable nodes ==")
    for node in tree.iter("node"):
        if node.get("clickable") == "true":
            print(
                "class=%s id=%s text=%r desc=%r bounds=%s enabled=%s"
                % (
                    node.get("class"),
                    node.get("resource-id"),
                    (node.get("text") or "").strip(),
                    (node.get("content-desc") or "").strip(),
                    node.get("bounds"),
                    node.get("enabled"),
                )
            )


if __name__ == "__main__":
    main()
