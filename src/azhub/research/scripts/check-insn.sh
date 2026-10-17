#!/bin/bash
# Disassemble the stock and patched words to confirm the encodings.
. "$(dirname "$0")/kenv.sh" true
printf '\x1f\x04\x00\x71\x1f\x18\x00\x71' > ~/insn.bin
${CROSS_COMPILE}objdump -D -b binary -m aarch64 ~/insn.bin | tail -2
