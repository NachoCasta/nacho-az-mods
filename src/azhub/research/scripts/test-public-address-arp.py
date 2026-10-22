#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Run an explicit ARP component binary on an isolated Linux veth pair.

Creates one fresh owned namespace with no physical interfaces, addresses or
routes. Removes only that namespace on exit. No AZ or external LAN access.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import uuid

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--binary',type=Path,required=True)
args=p.parse_args()
assert os.name=='posix' and os.geteuid()==0
binary=args.binary.resolve();assert binary.is_file()
namespace='azhub-acd-'+uuid.uuid4().hex[:10]
assert namespace not in subprocess.check_output(['ip','netns','list'],text=True)
created=False
def run(*command):
    return subprocess.run(command,check=True,capture_output=True,text=True,timeout=20)
try:
    run('ip','netns','add',namespace);created=True
    prefix=('ip','netns','exec',namespace)
    run(*prefix,'ip','link','add','azpubt0','type','veth','peer','name','azpubt1')
    for name,mac in [('azpubt0','02:00:00:00:33:01'),('azpubt1','02:00:00:00:33:02')]:
        run(*prefix,'ip','link','set',name,'address',mac)
        run(*prefix,'ip','link','set',name,'up')
    result=run(*prefix,str(binary),'--isolated-veth')
    print(result.stdout,end='')
finally:
    if created:
        assert not run('ip','netns','pids',namespace).stdout.strip(),'Unexpected live namespace process'
        run('ip','netns','del',namespace)
        print(json.dumps({'owned_namespace_removed':True,'external_interface_used':False}))
