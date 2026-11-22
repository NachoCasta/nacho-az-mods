#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Prepare old/fixed emulator-only hook-publication controls; never deploy."""
import argparse
from pathlib import Path
import shutil
import subprocess

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--unfixed',action='store_true')
a=parser.parse_args()
root=Path(__file__).resolve().parents[4]
out=a.output.resolve()
assert out.is_relative_to(root/'local') and not out.exists()
src=root/'src/azhub/linkfake'
out.mkdir(parents=True)
for p in src.iterdir():
    if p.is_file() and (p.suffix in ('.c','.h') or p.name in ('Makefile','azmod.json')):
        data=subprocess.check_output(['git','show','HEAD:'+p.relative_to(root).as_posix()],cwd=root) if a.unfixed else p.read_bytes()
        (out/p.name).write_bytes(data)
p=out/'linkfake.c'
s=p.read_text()
point='static void *make_hook(' if a.unfixed else 'static void make_hook('
s=s.replace(point,'#ifndef LF_PUBLIC_RUNTIME_EMULATOR\n#error "Startup window diagnostic is emulator-only"\n#endif\nstatic int startup_window;\n'+point,1)
needle='    return original;' if a.unfixed else '    return;\n#else'
window='    if (target==0x237f900) {\n        __atomic_store_n(&startup_window,1,__ATOMIC_RELEASE);\n        lf_log("STARTUP WINDOW open");\n        usleep(5000000);\n        lf_log("STARTUP WINDOW closed");\n        __atomic_store_n(&startup_window,0,__ATOMIC_RELEASE);\n    }\n'
assert needle in s
s=s.replace(needle,window+needle,1)
p.write_text(s)
p=out/'link-request-origin.h'
s=p.read_text()
needle='    uintptr_t caller=(uintptr_t)__builtin_return_address(0);'
start=s.index('static uint64_t probe_queue_pop(')
pos=s.index(needle,start)
s=s[:pos]+'    if (__atomic_load_n(&startup_window,__ATOMIC_ACQUIRE)) {\n        static int logged;\n        if (!__atomic_exchange_n(&logged,1,__ATOMIC_RELAXED))\n            lf_log("STARTUP WINDOW callback original=%p",(void *)probe_queue_pop_original);\n    }\n'+s[pos:]
p.write_text(s)
print(out)
