#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Prepare an emulator-only overrun control from current mod source.

Copies public source into a new ignored local directory. Never builds, installs,
connects to a controller, edits the core, or changes the SDK timing policy.
The diagnostic header refuses compilation without LF_PUBLIC_RUNTIME_EMULATOR.
"""
import argparse,hashlib,json,shutil
from pathlib import Path

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--trigger',choices=('startup','open'),default='open')
args=parser.parse_args()
root=Path(__file__).resolve().parents[4]
out=args.output.resolve()
if not out.is_relative_to(root/'local'):
    parser.error('Output must stay in the ignored local directory')
if out.exists():
    parser.error('Use a new output directory; existing experiments are never overwritten')
source=root/'src/azhub/linkfake'
out.mkdir(parents=True)
for path in source.iterdir():
    if path.is_file() and (path.suffix in ('.c','.h') or path.name in ('Makefile','azmod.json')):
        shutil.copy2(path,out/path.name)
path=out/'eject-ui.h'
text=path.read_text()
guard='''#if !defined(LF_PUBLIC_RUNTIME_EMULATOR)
#error "Eject input fault injection is emulator-only; never package this build"
#endif
'''
text=text.replace('#define HUB_EJECT_UI_H','#define HUB_EJECT_UI_H\n'+guard,1)
armed='1' if args.trigger=='startup' else '0'
text=text.replace('static struct hub_input_health eject_input_health;',
    'static struct hub_input_health eject_input_health;\n'
    'static _Atomic unsigned eject_fault_test_armed='+armed+';')
for name in ('eject_raw_keys','eject_filter_keys'):
    start=text.index('static int '+name+'(')
    point=text.index('    if (bus!=AZ_BUS_MIXER_IN || length<=34) return AZ_FRAME_OK;',start)
    end=point+len('    if (bus!=AZ_BUS_MIXER_IN || length<=34) return AZ_FRAME_OK;')
    injection='''
    /* Diagnostic only: let the unmodified SDK quarantine this callback. */
    static unsigned fault_calls;
    if (__atomic_load_n(&eject_fault_test_armed,__ATOMIC_ACQUIRE) && fault_calls++<8)
        usleep(2000);
'''
    text=text[:end]+injection+text[end:]
if args.trigger=='open':
    text=text.replace('    eject_focus_owned=1;',
        '    eject_focus_owned=1;\n'
        '    __atomic_store_n(&eject_fault_test_armed,1,__ATOMIC_RELEASE);\n'
        '    lf_core->log(EJECT_ID,"eject-input fault injection armed after selector open");',1)
text=text.replace('eject-ui active; hold=%u ms; mode=%s',
    'eject-ui EMULATOR FAULT CONTROL; hold=%u ms; mode=%s',1)
path.write_bytes(text.encode())
receipt={'scope':'Emulator-only source control; not a distributable package',
    'trigger':args.trigger,'source_header_sha256':hashlib.sha256((source/'eject-ui.h').read_bytes()).hexdigest(),
    'diagnostic_header_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),
    'mechanism':'Each mod callback deliberately sleeps2ms up to eight times; the real SDK retains its original budget and quarantine',
    'core_changed':False,'installed':False,'build_target':'sharing-runtime-emulator',
    'hardware_compile_guard':True}
(out/'fault-control.json').write_bytes((json.dumps(receipt,indent=2)+'\n').encode())
print(json.dumps(receipt,indent=2))
