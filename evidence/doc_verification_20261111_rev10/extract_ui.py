"""Extract visible text/content-desc and bounds from an Android uiautomator XML dump.

Usage: python extract_ui.py <xml-file>
Read-only; used by the documentation-verification evidence run.
"""
import sys
import xml.etree.ElementTree as ET

def main(path):
    root = ET.parse(path).getroot()
    for node in root.iter("node"):
        text = node.get("text") or ""
        desc = node.get("content-desc") or ""
        if text or desc:
            label = text if text else desc
            rid = node.get("resource-id") or ""
            print(f'{node.get("bounds")} | {rid} | {label}')

if __name__ == "__main__":
    main(sys.argv[1])
