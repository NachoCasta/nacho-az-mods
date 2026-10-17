#!/bin/bash
# calls-in.sh <start_hex> <end_hex> <regex>: calls in EP147's disassembly within an address range
# whose target name matches regex.
python3 - "$@" <<'EOF'
import re, sys, os
lo, hi, rx = int(sys.argv[1], 16), int(sys.argv[2], 16), re.compile(sys.argv[3])
line_re = re.compile(r'^\s*([0-9a-f]+):\s+(bl|b)\s+([0-9a-f]+) <([^>]+)>')
with open(os.path.expanduser('~/ep147.dis')) as f:
    for line in f:
        m = line_re.match(line)
        if m and lo <= int(m.group(1), 16) < hi and rx.search(m.group(4)):
            print(m.group(1), m.group(2), m.group(4))
EOF
