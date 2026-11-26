#!/usr/bin/env python3
"""Read-only firmware release/test-build exclusion audit for E2E-JOINT-P1."""
import hashlib
import os
import re
import sys
import zipfile

ZIP = 'fw_release/fw_sensor_FD-002r8.zip'
UVPROJ = ('CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/'
          'gpio_input_output/MDK/Project.uvprojx')

print('== release package ==')
print('path:', ZIP)
print('size:', os.path.getsize(ZIP))
print('md5:', hashlib.md5(open(ZIP, 'rb').read()).hexdigest())
print('sha256:', hashlib.sha256(open(ZIP, 'rb').read()).hexdigest())

with zipfile.ZipFile(ZIP) as z:
    print('members:')
    for i in z.infolist():
        print('  %-40s %d' % (i.filename, i.file_size))
    hits = []
    for i in z.infolist():
        data = z.read(i.filename)
        for pat in (b'TINJ', b'SENSOR_TEST_INJECT', b'INJECT'):
            if pat in data:
                hits.append((i.filename, pat.decode()))
    print('test-string hits:', hits if hits else 'NONE')

print('\n== Keil project targets ==')
t = open(UVPROJ, encoding='utf-8', errors='replace').read()
targets = re.findall(r'<TargetName>([^<]*)</TargetName>', t)
print('targets:', targets)
defines = re.findall(r'<Define>([^<]*)</Define>', t)
print('Define entries:', defines)
for pat in ('SENSOR_TEST_INJECT', 'TINJ'):
    print('project contains %s:' % pat, pat in t)

print('\n== source scan ==')
root = ('CW32L010_StandardPeripheralLib_V1.0.5/Examples/sensor/'
        'gpio_input_output')
hitfiles = []
for dirpath, _, files in os.walk(root):
    for fn in files:
        if fn.endswith(('.c', '.h')):
            p = os.path.join(dirpath, fn)
            try:
                d = open(p, 'rb').read()
            except OSError:
                continue
            if b'TINJ' in d or b'SENSOR_TEST_INJECT' in d:
                hitfiles.append(p)
print('source files containing injection markers:', hitfiles if hitfiles else 'NONE')
