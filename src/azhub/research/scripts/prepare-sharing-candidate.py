#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Stage and validate a private sharing candidate; never install or publish."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tarfile
import zipfile


def digest(path):return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    for name in ('out','runtime','helper','macvlan-build','packer'):
        parser.add_argument('--'+name,type=Path,required=True)
    parser.add_argument('--version',required=True)
    args=parser.parse_args();root=Path.cwd().resolve();out=args.out.resolve()
    assert out.is_relative_to(root/'local/publishing') and not out.exists()
    assert re.fullmatch(r'(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)',args.version)
    for path in (args.runtime,args.helper,args.packer,args.macvlan_build/'usb_hub_macvlan.ko'):
        assert path.is_file()
    assert digest(args.macvlan_build/'macvlan.c')=='2aa65660418fa6121ba2b63b16c9540426bc43f83ffd0baf000c0d21662df54a'
    assert digest(args.macvlan_build/'usb_hub_macvlan.ko')=='4d59439e1d080f0c2c2dd49b0e41524747d6632f1e6261d18a54a2819784c667'
    src=root/'src/azhub/linkfake';mod=out/'mod'
    (mod/'out').mkdir(parents=True);(mod/'assets/macvlan-source').mkdir(parents=True)
    for name in ('icon.png','LICENSE'):shutil.copyfile(src/name,mod/name)
    for name in ('azhub.ko','azhub-LICENSE.txt','mount-hub.sh'):shutil.copyfile(src/'assets'/name,mod/'assets'/name)
    shutil.copyfile(args.runtime,mod/'out/usb-hub-support.so')
    shutil.copyfile(args.helper,mod/'assets/usb-hub-support-network-helper.so')
    shutil.copyfile(root/'src/azhub/research/scripts/public-dhcp-event-hook.sh',mod/'assets/public-dhcp-event-hook.so')
    shutil.copyfile(args.macvlan_build/'usb_hub_macvlan.ko',mod/'assets/usb_hub_macvlan.ko')
    source=mod/'assets/macvlan-source'
    for name in ('macvlan.c','Makefile','build-receipt.json'):shutil.copyfile(args.macvlan_build/name,source/name)
    shutil.copyfile(src/'assets/azhub-LICENSE.txt',source/'LICENSE')
    shutil.copyfile(root/'src/azhub/research/scripts/build-sharing-macvlan.py',source/'build-sharing-macvlan.py')
    shutil.copyfile(root/'docs/HUB-LINK-TRANSPORT-BUILD.md',source/'BUILD.md')
    shutil.copyfile(root/'src/azhub/azhub.c',mod/'assets/azhub-source.c')
    manifest=json.loads((src/'azmod.json').read_text())
    assert tuple(map(int,args.version.split('.')))>tuple(map(int,manifest['version'].split('.')))
    manifest.update(version=args.version,summary='Experimental PRO DJ LINK sharing for hub USB sticks.',
        description='Local candidate for separately authorized physical AZ/CDJ trials. Browse and load hub USB libraries locally and through independent PRO DJ LINK software clients. Preserve native USB sources and four local decks with Link off, two with Link on. Use USB STOP to select a stick or hold to eject the hub. External players handle source loss according to their own cache. Actual-CDJ interoperability and target MACVLAN runtime are not validated. Exact firmware2.00 ep147 only; core1.2.3 or newer.')
    (mod/'azmod.json').write_text(json.dumps(manifest,indent=2)+'\n')
    # Snapshot an explicit source allowlist, never local diagnostics/firmware/music.
    inputs=set(src.glob('*.c'))|set(src.glob('*.h'))
    inputs.update(src/name for name in ('Makefile','azmod.json','LICENSE','icon.png'))
    inputs.update(src/'assets'/name for name in ('azhub.ko','azhub-LICENSE.txt','mount-hub.sh'))
    inputs.update(root/'src/azhub'/name for name in ('azhub.c','Makefile'))
    scripts=root/'src/azhub/research/scripts'
    inputs.update(p for p in scripts.iterdir() if p.is_file() and p.suffix in ('.py','.sh','.c','.h'))
    inputs.update((root/'src/azhub/research/link-peer/src/main/java').glob('*.java'))
    inputs.update((root/'docs').glob('HUB-LINK-*.md'))
    source_files={p.relative_to(root).as_posix():p.read_bytes() for p in sorted(inputs)}
    source_files['candidate/azmod.json']=(mod/'azmod.json').read_bytes()
    for p in sorted(source.iterdir()):source_files['candidate/macvlan-source/'+p.name]=p.read_bytes()
    with tarfile.open(out/'source.tar','w',format=tarfile.USTAR_FORMAT) as archive:
        for name,data in sorted(source_files.items()):
            entry=tarfile.TarInfo(name);entry.size=len(data);entry.mode=0o644
            archive.addfile(entry,io.BytesIO(data))
    commands=[[sys.executable,str(args.packer),'package',str(mod)],
              [sys.executable,str(args.packer),'validate',str(mod/'dist'/('usb-hub-support-'+args.version+'.azmod'))]]
    for i,command in enumerate(commands):
        result=subprocess.run(command,capture_output=True,text=True)
        (out/('validation-'+str(i)+'.log')).write_text(result.stdout+result.stderr)
        assert result.returncode==0,result.stdout+result.stderr
    package=mod/'dist'/('usb-hub-support-'+args.version+'.azmod')
    with zipfile.ZipFile(package) as archive:
        members={name:{'bytes':len(archive.read(name)),'sha256':hashlib.sha256(archive.read(name)).hexdigest(),
                      'mode':oct((archive.getinfo(name).external_attr>>16)&0o777)} for name in archive.namelist()}
        for name in ('assets/usb-hub-support-network-helper.so','assets/public-dhcp-event-hook.so'):
            assert members[name]['mode']=='0o755'
    receipt={'scope':'Local candidate build/validation, not physical acceptance or publication',
        'package_sha256':digest(package),'source_tar_sha256':digest(out/'source.tar'),
        'runtime_input_sha256':digest(args.runtime),'helper_input_sha256':digest(args.helper),
        'packer_sha256':digest(args.packer),'commands':commands,'members':members,
        'source_members':{name:hashlib.sha256(data).hexdigest() for name,data in source_files.items()},
        'physical_AZ_touched':False,'published':False}
    (out/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps({key:receipt[key] for key in ('package_sha256','source_tar_sha256','runtime_input_sha256','helper_input_sha256')}))


if __name__=='__main__':main()
