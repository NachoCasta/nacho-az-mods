#!/usr/bin/env python3
"""Copy the hub research into an independent repository; never move originals."""
import argparse,hashlib,json,shutil,subprocess
from pathlib import Path

def digest(p): return hashlib.sha256(p.read_bytes()).hexdigest()

def main():
 a=argparse.ArgumentParser();a.add_argument('--source',type=Path,required=True);a.add_argument('--dest',type=Path,required=True);args=a.parse_args()
 source=args.source.resolve();dest=args.dest.resolve()
 if dest.exists():raise SystemExit('Destination already exists; refusing overwrite: '+str(dest))
 dest.mkdir(parents=True)
 src=source/'src/azhub';included=[];excluded=[]
 for p in sorted(src.rglob('*')):
  if not p.is_file():continue
  rel=p.relative_to(src);parts=rel.parts
  reason=None
  if parts[0]=='publishing':reason='publication preparation workspace / upstream SDK checkout'
  elif '__pycache__' in parts or p.suffix=='.pyc':reason='generated Python cache'
  elif p.name=='device-setting-serialize.c':reason='raw firmware decompilation; facts documented in GHIDRA-FINDINGS.md instead'
  elif p.name=='publishing-guide.html':reason='downloaded third-party web page; retained as a source URL in documentation'
  elif p.suffix in ('.so','.o') or (p.suffix=='.ko' and rel.as_posix()!='azhub.ko'):reason='rebuildable executable; tested mod binaries retained inside versioned dev archives'
  elif not p.suffix and p.name not in ('Makefile','.gitignore'):reason='generated executable; C sources retained'
  if reason:
   excluded.append({'path':str(p),'reason':reason,'size':p.stat().st_size});continue
  target=dest/'src/azhub'/rel;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,target)
  included.append({'path':target.relative_to(dest).as_posix(),'sha256':digest(p),'size':p.stat().st_size})
 for rel in ('LICENSE','src/azmod/azmod-mod.h'):
  p=source/rel;target=dest/rel;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,target)
  included.append({'path':rel,'sha256':digest(p),'size':p.stat().st_size})
 # Preserve the migration script itself for audit/repeatability.
 target=dest/'tools/prepare_private_repo.py';target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(Path(__file__),target)
 manifest={'source_remote':'https://github.com/Kyle-Hosman/xdj-az-mods','source_head':subprocess.check_output(['git','-C',str(source),'rev-parse','HEAD'],text=True).strip(),'source_workspace':str(source),'snapshot':'2026-10-17','copied':included,'excluded':excluded,'originals_untouched':True}
 p=dest/'docs/migration-inventory.json';p.parent.mkdir(parents=True,exist_ok=True);p.write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
 for item in included:
  assert digest(dest/item['path'])==item['sha256'],item['path']
 print(json.dumps({'dest':str(dest),'files_copied':len(included),'MiB':round(sum(x['size'] for x in included)/1048576,2),'excluded':len(excluded)},indent=2))
if __name__=='__main__':main()
