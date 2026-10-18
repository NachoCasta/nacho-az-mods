#!/usr/bin/env python3
"""Prepare a kernel-guarded localhost delayed-native-receipt overlay; never build or install.

Run from the repository root. Output stays in ignored local/diagnostics.
"""
from pathlib import Path
import shutil,json,hashlib
root=Path.cwd();src=root/'src/azhub/linkfake';dest=root/'local/diagnostics/loaded-late-completion/source';dest.mkdir(parents=True,exist_ok=True)
record=json.loads((root/'docs/acceptance/usb-hub-support-0.2.1-loaded-build.json').read_text(encoding='utf-8'))
for name,expected in record['source_snapshot'].items():
 p=root/name;assert hashlib.sha256(p.read_bytes()).hexdigest()==expected,name
 shutil.copy2(p,dest/p.name)
p=dest/'native-detach.h';s=p.read_text(encoding='utf-8');pos=s.index('static uintptr_t safe_native_interface')
s=s[:pos]+'''/* Deliberate localhost fault overlay: defer actual native receipts. */
static struct { uintptr_t owner; uint64_t media,serial; int valid; } test_late_native[HUB_DETACH_SLOTS];
static _Atomic int test_late_released;
'''+s[pos:]
a='    int index=hub_detach_complete(&safe_detach_pool,owner,media,serial);';b='''    int index=-1;
    if (__atomic_load_n(&test_late_released,__ATOMIC_ACQUIRE)) index=hub_detach_complete(&safe_detach_pool,owner,media,serial);
    else for (int i=0;i<HUB_DETACH_SLOTS;i++) {
        const struct hub_detach_ticket *ticket=&safe_detach_pool.slots[i];
        if (ticket->phase==HUB_DETACH_PENDING && ticket->submitted && ticket->owner==owner &&
            ticket->media==hub_detach_media(media) && serial>ticket->fence) {
            test_late_native[i].owner=owner;test_late_native[i].media=media;
            test_late_native[i].serial=serial;test_late_native[i].valid=1;
        }
    }''';assert a in s;s=s.replace(a,b,1)
s=s.replace('eject_clock_ms()+60000','eject_clock_ms()+1500',1)
a='''    for (;;) {
        int pending=0;''';b='''    for (;;) {
        if (!access("/tmp/azhub-detach-test-release",F_OK)) {
            pthread_mutex_lock(&safe_lock);
            __atomic_store_n(&test_late_released,1,__ATOMIC_RELEASE);
            for (int i=0;i<HUB_DETACH_SLOTS;i++) if (test_late_native[i].valid) {
                hub_detach_complete(&safe_detach_pool,test_late_native[i].owner,test_late_native[i].media,test_late_native[i].serial);
                test_late_native[i].valid=0;
            }
            pthread_mutex_unlock(&safe_lock);
        }
        int pending=0;''';assert a in s;s=s.replace(a,b,1);p.write_text(s,encoding='utf-8',newline='\n')
p=dest/'community.c';s=p.read_text(encoding='utf-8').replace('static int community_init(const az_core_v1 *core) {','''static int community_init(const az_core_v1 *core) {
    struct utsname test_u;
    if (uname(&test_u) || strcmp(test_u.release,"6.6.138-dirty")) return -92;''',1);p.write_text(s,encoding='utf-8',newline='\n')
(dest.parent/'provenance.json').write_text(json.dumps({'scope':'Deliberate emulator-only delayed receipt / timeout / retry overlay; not normal candidate acceptance','base_binary_sha256':record['selected_runtime']['binary_sha256'],'base_source_snapshot':record['source_snapshot'],'kernel_guard':'6.6.138-dirty before initialization','changes':['Forward native completion normally but defer its actual receipt to the pool','1.5 second test deadline','On explicit local release marker, deliver recorded actual owner/media/serial without repeating native callback','No direct native request or server logic replaced']},indent=2)+'\n')
print('Fault overlay prepared from frozen 9feec49c source; not compiled or installed')
