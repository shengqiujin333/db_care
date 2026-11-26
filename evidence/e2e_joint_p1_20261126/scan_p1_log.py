#!/usr/bin/env python3
"""Scan captured logcat for E2E-JOINT-P1 signatures (read-only)."""
import re
import sys

path = sys.argv[1]
lines = open(path, encoding='utf-8', errors='replace').read().splitlines()

sig = {
    'tls_san_or_chain_errors': [
        'SSLHandshakeException', 'No subjectAltNames', 'Trust anchor',
        'PKIX', 'CertPathValidatorException', 'SSLPeerUnverifiedException',
        'CertificateException',
    ],
    'mqtt_connect': ['MqttException', 'CONNACK', 'not authorized', 'not_authorized',
                     'MqtttService', 'MqttService'],
    'ble_read': ['MTU', 'mtu', 'GwID', 'DATA_OK', 'link_state', 'READ_',
                 'valueSize', 'dev='],
    'reading': ['T=', 'H=', 'id=', 'TempDb', 'Save reading'],
    'upload': ['CloudUpload', 'upload gated off', 'uploaded '],
}

print('file:', path, 'lines:', len(lines))
print('first:', lines[0] if lines else '')
print('last:', lines[-1] if lines else '')
print()
for name, pats in sig.items():
    hit = [ln for ln in lines if any(p in ln for p in pats)]
    print('== %s: %d line(s)' % (name, len(hit)))
    for ln in hit[-40:]:
        print('  ', ln)
    print()
