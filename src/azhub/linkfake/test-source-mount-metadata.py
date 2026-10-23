#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise the actual mount enqueue body with native queue stand-ins."""
import os, re, shlex, subprocess, tempfile
from pathlib import Path

root = Path(__file__).resolve().parent
source = (root/'hubservers.h').read_text()
match = re.search(r'static int hub_queue_mount\(int source\)\s*\{', source)
assert match
depth = 1
for end in range(match.end(), len(source)):
    depth += (source[end] == '{') - (source[end] == '}')
    if not depth:
        function = source[match.start():end+1]
        break
for address, stub in [('0x24a1180', 'make_path'), ('0x1373ca8', 'mount_music'),
                      ('0x1974ff8', 'mount_info'), ('0x2469aa0', 'free_path')]:
    function = function.replace(address, '(uintptr_t)'+stub)
harness = r'''
#include <assert.h>
#include "source-metadata.h"
#define LF_SAFE_EJECT 1
#define lf_log(...) ((void)0)
static pthread_mutex_t safe_source_mutex=PTHREAD_MUTEX_INITIALIZER;
static struct fake {int mounted,present,unmount_pending;uint64_t srv_music,srv_info;
 char dir[256],port_route[96];} fakes[8];
static int music_calls,info_calls;
static void make_path(void *p,const char *path) {*(uintptr_t *)p=(uintptr_t)path;}
static void free_path(void *p) {(void)p;}
static void mount_music(uint64_t server,void *id,void *path,int flag,void *listener) {
 assert(server==11 && *(uint16_t *)id==0x102 && !flag && !listener);
 assert(!strcmp((char *)*(uintptr_t *)path,fakes[0].dir));music_calls++;
}
static void mount_info(uint64_t server,void *id,void *listener) {
 assert(server==12 && *(uint16_t *)id==0x102 && !listener);info_calls++;
}
''' + function + r'''
int main(void) {
 fakes[0].present=1;fakes[0].srv_music=11;fakes[0].srv_info=12;
 strcpy(fakes[0].dir,"/media/usb/owned");strcpy(fakes[0].port_route,"3-2.4");
 assert(hub_queue_mount(0));
 struct hub_metadata_ticket first=hub_metadata_capture(0);
 struct hub_source_properties props={.name="Configured",.color=6,.name_valid=1,.color_valid=1},out;
 assert(hub_metadata_publish(0,&first,&props));
 fakes[0].mounted=0;assert(hub_queue_mount(0));
 struct hub_metadata_ticket second=hub_metadata_capture(0);
 assert(first.generation==second.generation);
 assert(hub_metadata_read(0,fakes[0].dir,fakes[0].port_route,&out));
 assert(!strcmp(out.name,"Configured") && out.color==6 && out.name_valid);
 assert(music_calls==2 && info_calls==2);
 assert(hub_queue_mount(0) && music_calls==2);
 hub_metadata_invalidate(0);fakes[0].mounted=0;
 assert(hub_queue_mount(0));
 second=hub_metadata_capture(0);assert(second.generation!=first.generation);
 assert(!hub_metadata_complete(0,fakes[0].dir,fakes[0].port_route));
 fakes[0].mounted=0;fakes[0].unmount_pending=1;
 assert(!hub_queue_mount(0) && music_calls==3 && info_calls==3);
 puts("PASS: actual mount queue preserves captured metadata across remount, invalidates retired generations and refuses pending removal");
}
'''
with tempfile.TemporaryDirectory(prefix='azhub-mount-metadata-') as directory:
    path = Path(directory)
    (path/'test.c').write_text(harness)
    subprocess.run(shlex.split(os.environ.get('TEST_CC', 'cc'))+
                   ['-O2', '-Wall', '-Wextra', '-Wno-unused-function', '-pthread',
                    '-I'+str(root), str(path/'test.c'), '-o', str(path/'test')], check=True)
    subprocess.run([str(path/'test')], check=True)
