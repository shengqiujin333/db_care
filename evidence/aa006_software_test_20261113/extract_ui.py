import re
import sys
import xml.etree.ElementTree as ET

path = sys.argv[1]
tree = ET.parse(path)
root = tree.getroot()


def attrs_of(node):
    out = []
    for key in ("text", "resource-id", "class", "content-desc", "clickable", "enabled", "bounds", "focused"):
        value = node.attrib.get(key)
        if value not in (None, "", "false"):
            out.append("%s=%s" % (key, value))
    return out


def walk(node, depth=0):
    a = attrs_of(node)
    if a:
        print("%s%s" % ("  " * depth, " | ".join(a)))
    for child in node:
        walk(child, depth + 1)


walk(root)
