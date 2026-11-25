import re, sys
s = open(sys.argv[1], encoding='utf-8', errors='replace').read()
for m in re.finditer(r'text="([^"]{1,100})"', s):
    v = m.group(1).strip()
    if v:
        print(v)
