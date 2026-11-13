import glob
import os
import xml.etree.ElementTree as ET

RESULT_DIR = r"D:/mypro/beiwo2/AIP/dengbei_care/app/build/test-results/testDebugUnitTest"
OUT = r"D:/mypro/beiwo2/AIP/evidence/aa006_software_test_20261113/unit_test_summary.txt"

files = sorted(glob.glob(os.path.join(RESULT_DIR, "TEST-*.xml")))
tot = fail = err = skip = 0
rows = []
for f in files:
    root = ET.parse(f).getroot()
    t = int(root.get("tests", 0))
    fa = int(root.get("failures", 0))
    er = int(root.get("errors", 0))
    sk = int(root.get("skipped", 0))
    tot += t
    fail += fa
    err += er
    skip += sk
    rows.append((root.get("name"), t, fa, er, sk))

lines = ["suites=%d tests=%d failures=%d errors=%d skipped=%d" % (len(files), tot, fail, err, skip)]
for n, t, fa, er, sk in rows:
    lines.append("%s\t%d\t%d\t%d\t%d" % (n, t, fa, er, sk))
text = "\n".join(lines)
with open(OUT, "w", encoding="utf-8") as fh:
    fh.write(text + "\n")
print(text)
