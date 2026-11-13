#!/usr/bin/env python3
"""Bounded UI hierarchy text extractor for acceptance evidence (test-side only)."""
import re
import sys

path = sys.argv[1]
s = open(path, encoding='utf-8').read()
for m in re.finditer(r'<node[^>]*>', s):
    t = m.group(0)

    def g(k):
        mm = re.search(k + r'="([^"]*)"', t)
        return mm.group(1) if mm else ''

    txt = g('text')
    desc = g('content-desc')
    if txt or desc:
        print('text=%r desc=%r id=%s bounds=%s class=%s' % (
            txt, desc, g('resource-id'), g('bounds'), g('class')))
