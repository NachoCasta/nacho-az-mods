#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compare C PCOB fields to unchanged Crate Digger; use explicit local fixtures."""
import hashlib
import json
from pathlib import Path
import subprocess
import sys


def main():
    assert len(sys.argv) >= 4, "READER JAVA_CLASSPATH ANALYSIS_FILE..."
    results = []
    for filename in sys.argv[3:]:
        raw = Path(filename).read_bytes()
        native = subprocess.run([sys.argv[1], filename], check=True, capture_output=True, timeout=15).stdout
        java = subprocess.run(["java", "-cp", sys.argv[2], "CueSectionControl", filename],
                              check=True, capture_output=True, timeout=15).stdout
        assert native == java, "Cue field mismatch"
        results.append({"sha256": hashlib.sha256(raw).hexdigest(), "bytes": len(raw),
                        "matched_entries": len(native.splitlines()),
                        "decoded_sha256": hashlib.sha256(native).hexdigest()})
    print(json.dumps({"fixtures": results, "matched": True}, indent=2))


if __name__ == "__main__":
    main()
