#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise the actual pre-announcement preparation with private-worker stand-ins."""
import os, re, shlex, subprocess, tempfile
from pathlib import Path

root = Path(__file__).resolve().parent
source = (root / 'hubservers.h').read_text(encoding='utf-8')
match = re.search(r'static int hub_prepare_announcement\([^{}]+\)\s*\{', source)
assert match
depth = 1
for end in range(match.end(), len(source)):
    depth += (source[end] == '{') - (source[end] == '}')
    if not depth:
        function = source[match.start():end + 1]
        break
else:
    raise AssertionError('Missing preparation body')

harness = r'''
#include <assert.h>
#include <time.h>
#include "source-metadata.h"
#define LF_SAFE_EJECT 1
#define lf_log(...) ((void)0)
struct fake {time_t mount_at;int mounted,srv_music,srv_info,metadata_probe_retired;char dir[256],port_route[96],mp[64];};
static struct fake fakes[8];
static int allowed=1,leases,queued,finish,retired;
static int alias_ready,alias_failure;
static int hub_alias_prepare(const char *root,const char *alias) {
    assert(!strcmp(root,"/media/usb/owned") && !strcmp(alias,"/media/player07/usb"));
    if(alias_failure)return -1;
    alias_ready=1;return 0;
}
static int safe_server_begin(int source) {(void)source;if(!allowed)return 0;leases++;return 1;}
static void safe_server_end(int source) {(void)source;leases--;}
static void hub_retire_metadata_mount(int source) {
    assert(leases==1 && fakes[source].mounted);retired++;
}
static int hub_queue_mount(int source) {
    assert(leases==1 && alias_ready);queued++;fakes[source].mounted=1;
    hub_metadata_prepare(source,fakes[source].dir,fakes[source].port_route);
    if(finish) {
        struct hub_metadata_ticket ticket=hub_metadata_capture(source);
        struct hub_source_properties p={0};
        /* Even a completed empty/invalid value must permit the fallback. */
        assert(hub_metadata_publish(source,&ticket,&p));
    }
    return 1;
}
''' + function + r'''
int main(void) {
    strcpy(fakes[0].dir,"/media/usb/owned");strcpy(fakes[0].port_route,"3-2.4");
    strcpy(fakes[0].mp,"/media/player07/usb");
    alias_failure=1;
    assert(!hub_prepare_announcement(0,90));assert(!queued && !leases && !alias_ready);
    alias_failure=0;
    assert(!hub_prepare_announcement(0,100));assert(!queued && !leases);
    assert(!hub_prepare_announcement(0,109));assert(hub_prepare_announcement(0,110));
    assert(fakes[0].mount_at==0 && fakes[0].metadata_probe_retired && !retired);
    fakes[0].metadata_probe_retired=0;fakes[0].srv_music=fakes[0].srv_info=1;
    allowed=0;assert(!hub_prepare_announcement(0,200));assert(!queued && !leases);
    allowed=1;finish=1;assert(hub_prepare_announcement(0,201));assert(queued==1 && !leases);
    assert(retired==1 && !fakes[0].mounted && !fakes[0].mount_at);
    assert(hub_prepare_announcement(0,202));assert(queued==1);
    hub_metadata_invalidate(0);fakes[0].mounted=0;fakes[0].mount_at=0;finish=0;
    fakes[0].metadata_probe_retired=0;
    assert(!hub_prepare_announcement(0,300));assert(queued==2 && !leases);
    assert(!hub_prepare_announcement(0,309));
    allowed=0;assert(!hub_prepare_announcement(0,310));assert(retired==1);
    allowed=1;assert(hub_prepare_announcement(0,311));
    assert(queued==2 && retired==2 && !fakes[0].mounted && !fakes[0].mount_at && !leases);
    assert(hub_prepare_announcement(0,312) && queued==2 && retired==2);
    puts("PASS: metadata capture retires premature mount before announcement, clears its deadline, does not repeat, bounds fallback and respects eject leases");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='azhub-announcement-') as directory:
    path = Path(directory)
    (path / 'test.c').write_text(harness, encoding='utf-8')
    subprocess.run(shlex.split(os.environ.get('TEST_CC', 'cc')) +
                   ['-O2', '-Wall', '-Wextra', '-Wno-unused-function', '-pthread',
                    '-I' + str(root), str(path / 'test.c'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
