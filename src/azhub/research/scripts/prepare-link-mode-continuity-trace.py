"""Create an isolated forwarding-only emulator diagnostic; never deploy."""
from pathlib import Path
import argparse,hashlib,json,shutil

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--out',required=True,type=Path)
parser.add_argument('--source',type=Path,default=Path('src/azhub/linkfake'),help='Published 0.3.1 source tree, before continuity integration')
args=parser.parse_args()
root=args.out
baseline=(args.source/'link-mode-recovery.h').read_text(encoding='utf-8')
if 'probe_member_continuity_install' in baseline:
    raise SystemExit('Refusing duplicate hook claims: select the published 0.3.1 baseline with --source.')
assert not root.exists()
source=root/'src'
shutil.copytree(args.source,source,ignore=shutil.ignore_patterns('out','dist','__pycache__'))
header=source/'link-mode-recovery.h'
code=header.read_text(encoding='utf-8')
trace=r'''
/* Forwarding-only local lifecycle diagnostic. No suppression or state edits. */
static fn8 continuity_disconnect_original,continuity_reset_original;
static fn8 continuity_member_mode_original;
static fn8 continuity_clear_original;
static _Atomic unsigned continuity_trace_count;
static void continuity_trace(const char *event,uintptr_t a,uintptr_t b,uintptr_t caller) {
    if (__atomic_fetch_add(&continuity_trace_count,1,__ATOMIC_RELAXED)>=400) return;
    struct timespec ts;clock_gettime(CLOCK_MONOTONIC,&ts);
    lf_log("continuity trace t=%lld.%03ld event=%s a=%#lx b=%#lx caller=%#lx",
        (long long)ts.tv_sec,ts.tv_nsec/1000000,event,(unsigned long)a,
        (unsigned long)b,(unsigned long)caller);
}
static uint64_t continuity_disconnect(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    continuity_trace("disconnect",a,b?*(unsigned char *)b:255,
        (uintptr_t)__builtin_return_address(0));
    return continuity_disconnect_original(a,b,c,d,e,f,g,h);
}
static uint64_t continuity_reset(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    continuity_trace("network-reset",a,b,(uintptr_t)__builtin_return_address(0));
    return continuity_reset_original(a,b,c,d,e,f,g,h);
}
static uint64_t continuity_member_mode(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    continuity_trace("member-mode-begin",a,*(unsigned char *)(a+0x20),
        (uintptr_t)__builtin_return_address(0));
    uint64_t result=continuity_member_mode_original(a,b,c,d,e,f,g,h);
    continuity_trace("member-mode-end",a,0,0);
    return result;
}
static uint64_t continuity_clear(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uintptr_t begin=*(uintptr_t *)(a+0x380),end=*(uintptr_t *)(a+0x388);
    continuity_trace("member-clear-begin",a,b,(uintptr_t)__builtin_return_address(0));
    if (end>=begin && end-begin<=64*0x50) {
        for (uintptr_t row=begin;row<end;row+=0x50)
            continuity_trace("member-clear-row",*(unsigned char *)row,
                *(unsigned char *)(row+1),0);
    }
    uint64_t result=continuity_clear_original(a,b,c,d,e,f,g,h);
    continuity_trace("member-clear-end",a,0,0);
    return result;
}
'''
needle='static void probe_mode_recovery_install(void) {'
assert code.count(needle)==1
code=code.replace(needle,trace+'\n'+needle+'''
    continuity_disconnect_original=make_hook(0x13e0448,continuity_disconnect);
    continuity_reset_original=make_hook(0x13f96a8,continuity_reset);
    continuity_member_mode_original=make_hook(0x13e49b0,continuity_member_mode);
    continuity_clear_original=make_hook(0x13e18d8,continuity_clear);
''')
header.write_bytes(code.encode('utf-8'))
manifest=source/'azmod.json'
m=json.loads(manifest.read_text(encoding='utf-8'));m['version']='0.3.2'
m['summary']='Local emulator diagnostic: Link mode lifecycle traces.'
m['description']='Forwarding-only emulator diagnostic. Not a continuity fix or a release candidate. '+m['description']
manifest.write_bytes((json.dumps(m,indent=2)+'\n').encode())
(source/'out').mkdir()
(root/'preparation.json').write_text(json.dumps({'baseline':'0.3.1','diagnostic_only':True,'production_source_changed':False,'added_hooks':['0x13e0448','0x13f96a8','0x13e49b0','0x13e18d8'],'header_sha256':hashlib.sha256(header.read_bytes()).hexdigest()},indent=2),encoding='utf-8')
print(source)
