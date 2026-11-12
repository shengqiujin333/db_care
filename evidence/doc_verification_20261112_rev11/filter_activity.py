#!/usr/bin/env python3
"""Read `dumpsys activity activities` from stdin; print resumed/top activity lines."""
import sys

keep = ("mResumedActivity", "topResumedActivity", "ResumedActivity", "realActivity")
for line in sys.stdin:
    if any(k in line for k in keep):
        sys.stdout.write(line)
