#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build an ignored research-only MACVLAN module; never install or deploy it."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

CONFIG_SHA='f7f4f8aafbc9cede5be22dc36dac06e934c442fc834d7c6e5e30b0578755b3f3'
SOURCE_SHA='2aa65660418fa6121ba2b63b16c9540426bc43f83ffd0baf000c0d21662df54a'


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--kernel',required=True,type=Path)
    parser.add_argument('--compiler-prefix',required=True)
    parser.add_argument('--out',required=True,type=Path)
    args=parser.parse_args()
    kernel=args.kernel.resolve();out=args.out.resolve()
    if not out.is_relative_to((Path.cwd()/'local').resolve()) or out.exists():
        parser.error('Expected a new output directory under the checkout local/ directory')
    config=kernel/'.config';source=kernel/'drivers/net/macvlan.c'
    if digest(config)!=CONFIG_SHA or digest(source)!=SOURCE_SHA:
        parser.error('Prepared target config/source differs from pinned research input')
    release=(kernel/'include/generated/utsrelease.h').read_text().strip()
    if release!='#define UTS_RELEASE "4.4.194"':parser.error('Wrong prepared kernel release')
    compiler=subprocess.check_output([args.compiler_prefix+'gcc','--version'],text=True)
    if '6.3.1 20170404' not in compiler:parser.error('Expected pinned Linaro6.3.1 toolchain')
    out.mkdir(parents=True)
    shutil.copyfile(source,out/'macvlan.c')
    # Enable MACVLAN header helpers only for this external module. The target
    # .config and generated headers are never edited by this wrapper.
    (out/'Makefile').write_text('obj-m += usb_hub_macvlan.o\nusb_hub_macvlan-y := macvlan.o\nccflags-y += -DCONFIG_MACVLAN_MODULE=1\n')
    command=['make','-C',str(kernel),'M='+str(out),'ARCH=arm64',
             'CROSS_COMPILE='+args.compiler_prefix,'CC='+args.compiler_prefix+'gcc',
             'HOSTCFLAGS=-std=gnu11 -fcommon -w','modules']
    result=subprocess.run(command,capture_output=True)
    (out/'build.stdout').write_bytes(result.stdout);(out/'build.stderr').write_bytes(result.stderr)
    if result.returncode:raise SystemExit('Build failed; inspect local logs')
    module=out/'usb_hub_macvlan.ko'
    info=subprocess.check_output([args.compiler_prefix+'readelf','-p','.modinfo',str(module)],text=True)
    if 'vermagic=4.4.194 SMP preempt mod_unload aarch64' not in info:
        raise SystemExit('Unexpected module vermagic')
    (out/'module-info.txt').write_text(info)
    (out/'undefined-symbols.txt').write_bytes(subprocess.check_output([args.compiler_prefix+'nm','-u',str(module)]))
    unstripped=digest(module)
    subprocess.run([args.compiler_prefix+'strip','--strip-debug',str(module)],check=True)
    if digest(config)!=CONFIG_SHA:raise SystemExit('Target config changed unexpectedly')
    receipt={'scope':'Local build only; not runtime or physical acceptance',
             'config_sha256':CONFIG_SHA,'driver_source_sha256':SOURCE_SHA,
             'kernel_release':'4.4.194','compiler':compiler.splitlines()[0],
             'module_unstripped_sha256':unstripped,'module_stripped_sha256':digest(module),
             'module_bytes':module.stat().st_size,
             'symbol_versions_available':(kernel/'Module.symvers').exists(),
             'limits':['No symbol-export or load acceptance follows from compilation',
                       'MACVLAN disabled in target config; core/helper assumptions need review',
                       'No firmware compatibility expansion, packaging, installation or deployment']}
    (out/'build-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(receipt))


if __name__=='__main__':main()
