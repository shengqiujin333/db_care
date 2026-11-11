"""Print visible/clickable nodes from a uiautomator XML dump (bounded, read-only)."""
import sys
import xml.etree.ElementTree as ET


def walk(e, d=0):
    a = e.attrib
    txt = a.get("text", "")
    desc = a.get("content-desc", "")
    cid = a.get("resource-id", "")
    clk = a.get("clickable", "")
    en = a.get("enabled", "")
    if txt or desc or clk == "true":
        cls = a.get("class", "").split(".")[-1]
        print(f"{'  ' * d}{cls} id={cid} text={txt!r} desc={desc!r} "
              f"clickable={clk} enabled={en} bounds={a.get('bounds', '')}")
    for c in e:
        walk(c, d + 1)


if __name__ == "__main__":
    tree = ET.parse(sys.argv[1])
    walk(tree.getroot())
