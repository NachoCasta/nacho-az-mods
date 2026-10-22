#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Locate module dependencies in GPL export declarations; not a runtime audit."""
import argparse
import json
from pathlib import Path
import re


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--kernel',type=Path,required=True)
    parser.add_argument('--undefined',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    if args.out.exists():parser.error('Refusing to overwrite prior evidence')
    symbols={line.split()[-1] for line in args.undefined.read_text().splitlines() if line.strip()}
    found={s:[] for s in symbols}
    pattern=re.compile(r'EXPORT_(?:PER_CPU_)?SYMBOL(?:_GPL|_GPL_FUTURE|_NOVERS)?\s*\(\s*(\w+)\s*\)')
    root=args.kernel.resolve()
    for path in root.rglob('*'):
        if path.suffix not in ('.c','.h','.S'):continue
        relative=path.relative_to(root)
        if relative.parts[0]=='arch' and relative.parts[1]!='arm64':continue
        text=path.read_text(errors='replace')
        if 'EXPORT_' not in text:continue
        for match in pattern.finditer(text):
            if match[1] in found:
                found[match[1]].append({'path':relative.as_posix(),'line':text.count('\n',0,match.start())+1})
    receipt={'scope':'Static GPL source export declarations only',
             'undefined_count':len(symbols),'source_exports_found':sum(bool(x) for x in found.values()),
             'missing':sorted(s for s,paths in found.items() if not paths),
             'declarations':dict(sorted(found.items())),
             'limits':['No proof that conditional source was compiled or symbols exported by the running target',
                       'No address, modversion, ABI, load or physical-device acceptance']}
    args.out.write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({k:v for k,v in receipt.items() if k!='declarations'}))


if __name__=='__main__':main()
