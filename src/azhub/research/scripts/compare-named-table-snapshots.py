#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Compare local C/Crate Digger TSV snapshots without printing private metadata."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path


def read(path):
    data = Path(path).read_bytes()
    rows = data.decode("ascii").splitlines()
    for row in rows:
        fields = row.split("\t")
        if len(fields) != 7:
            raise ValueError("Invalid column count")
        for field in fields[:6]:
            if not 0 <= int(field) <= 0xffffffff:
                raise ValueError("Invalid unsigned field")
        bytes.fromhex(fields[6]).decode("utf-8")
    return data, rows


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("c_snapshot")
    parser.add_argument("oracle_snapshot")
    args = parser.parse_args()
    actual, rows = read(args.c_snapshot)
    expected, oracle = read(args.oracle_snapshot)
    # Preserve duplicate multiplicity; playlist positions remain explicit
    # columns, so sorting records cannot conceal a changed playlist order.
    if Counter(rows) != Counter(oracle):
        raise SystemExit("FAIL: snapshots differ")
    print(json.dumps({
        "matched_rows": len(rows),
        "tables": dict(sorted(Counter(row.split("\t", 1)[0] for row in rows).items())),
        "c_sha256": hashlib.sha256(actual).hexdigest(),
        "oracle_sha256": hashlib.sha256(expected).hexdigest(),
        "byte_identical": actual == expected,
    }, indent=2))


if __name__ == "__main__":
    main()
