#!/usr/bin/env python3
"""Bounded host-side TLS verification for E2E-JOINT-P0 platform stage.

Uses the project trust anchor jd-ca.crt, verifies the leaf IP SAN against
8.140.23.253, and records raw openssl output. No mutation, no --insecure.
"""
import re
import subprocess
import sys

OUT = r"D:\mypro\beiwo2\AIP\evidence\e2e_joint_p1_20261126"
CA = r"D:\mypro\beiwo2\AIP\dengbei_care\app\src\main\assets\jd-ca.crt"
HOST = "8.140.23.253"
PORT = "8883"


def run(argv):
    return subprocess.run(argv, stdin=subprocess.DEVNULL, capture_output=True,
                          timeout=60)


def save(name, data):
    with open(OUT + "\\" + name, "wb") as fh:
        fh.write(data)


def main():
    base = ["openssl", "s_client", "-connect", HOST + ":" + PORT,
            "-servername", HOST]

    verify = run(base + ["-CAfile", CA, "-verify_ip", HOST, "-verify_return_error"])
    save("24_host_tls_verify_ip.txt", verify.stdout + verify.stderr)
    print("verify_return_error_rc=%d" % verify.returncode)

    show = run(base + ["-showcerts"])
    save("24b_host_tls_showcerts.txt", show.stdout + show.stderr)

    text = (show.stdout + show.stderr).decode("utf-8", "replace")
    blocks = re.findall(
        r"-----BEGIN CERTIFICATE-----\s+.*?-----END CERTIFICATE-----\s*",
        text, re.S)
    print("cert_blocks=%d" % len(blocks))
    if blocks:
        with open(OUT + "\\leaf_8.140.23.253_p1.pem", "w", encoding="ascii") as fh:
            fh.write(blocks[0])
        fields = run(["openssl", "x509", "-in", OUT + "\\leaf_8.140.23.253_p1.pem",
                      "-noout", "-serial", "-fingerprint", "-sha256", "-subject",
                      "-issuer", "-dates", "-ext", "subjectAltName"])
        save("24c_host_leaf_fields.txt", fields.stdout + fields.stderr)

    plus = run(base + ["-CAfile", CA])
    save("24d_host_tls_plain.txt", plus.stdout + plus.stderr)
    print("done")


if __name__ == "__main__":
    sys.exit(main())
