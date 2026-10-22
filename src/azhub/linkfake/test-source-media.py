#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise the actual media encoder and its metadata snapshot boundary."""
import os,re,shlex,subprocess,tempfile
from pathlib import Path
root=Path(__file__).resolve().parent
source=(root/'linkfake.c').read_text(encoding='utf-8')
def body(name):
    m=re.search(r'\b'+name+r'\([^;{}]*\)\s*\{',source);assert m,name
    depth=1
    for i in range(m.end(),len(source)):
        depth+=(source[i]=='{')-(source[i]=='}')
        if not depth:return source[m.end():i]
    raise AssertionError(name)
fake=source[source.index('struct fake {'):source.index('\n};',source.index('struct fake {'))+3]
harness=r'''
#define _GNU_SOURCE
#define LF_SAFE_EJECT 1
#include <assert.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include "source-name.h"
#include "source-display.h"
#include "source-metadata.h"
'''+fake+r'''
static struct fake fakes[8];
static int nfakes=8, sends;
static pthread_mutex_t safe_source_mutex=PTHREAD_MUTEX_INITIALIZER;
static const char MAGIC[]="Qspt1WmJOL",*MODEL="CDJ-3000";
static unsigned char wire[0xc0];
static void to_loopback(struct fake *f,int port,const void *data,size_t size) {
    assert(f>=fakes && f<fakes+8 && port==50002 && size==sizeof wire);memcpy(wire,data,size);sends++;
}
'''
harness+='static uint64_t hub_media_fields(struct fake *f,char *name,size_t size,unsigned char *color){'+body('hub_media_fields')+'}\n'
harness+='static void send_media(struct fake *f,int slot){'+body('send_media')+'}\n'
harness+='static struct fake *fake_by_ip(struct in_addr a){'+body('fake_by_ip')+'}\n'
harness+='static void hub_media_refresh(struct fake *f,uint64_t *sent){'+body('hub_media_refresh')+'}\n'
harness+=r'''
static void ascii_name(const char *name) {
    size_t i=0;for (;name[i];i++) assert(!wire[0x2c+i*2] && wire[0x2d+i*2]==(unsigned char)name[i]);
    assert(!wire[0x2c+i*2] && !wire[0x2d+i*2]);
}
int main(void) {
    unsetenv("LINKFAKE_NORB");fakes[0].player=7;fakes[0].slot=3;
    strcpy(fakes[0].dir,"/media/usb/first");strcpy(fakes[0].port_route,"3-2.4");
    strcpy(fakes[0].name,"HUB_LIB");assert(hub_source_port_reserve(7,3,4));
    send_media(fakes,3);ascii_name("HUB_LIB");assert(!wire[0xa8]);
    unsigned char baseline[sizeof wire];memcpy(baseline,wire,sizeof wire);
    hub_metadata_prepare(0,fakes[0].dir,fakes[0].port_route);
    struct hub_metadata_ticket ticket=hub_metadata_capture(0);
    struct hub_source_properties p={.name_valid=1,.color_valid=1,.color=6};strcpy(p.name,"Nacho Chips");
    assert(hub_metadata_publish(0,&ticket,&p));send_media(fakes,3);ascii_name("Nacho Chips");
    assert(wire[0xa8]==6 && wire[0x27]==7 && wire[0x2b]==3);
    for (size_t i=0;i<sizeof wire;i++) if (!(i>=0x2c && i<0x6c) && i!=0xa8) assert(wire[i]==baseline[i]);
    p.name_valid=0;assert(hub_metadata_publish(0,&ticket,&p));send_media(fakes,3);
    ascii_name("HUB_LIB");assert(wire[0xa8]==6);
    hub_metadata_invalidate(0);assert(!hub_metadata_publish(0,&ticket,&p));
    fakes[0].name[0]=0;send_media(fakes,3);ascii_name("USB 4");assert(!wire[0xa8]);
    /* One transport readiness event must refresh every independent sibling. */
    uint64_t sent[8]={0};sends=0;
    for (int i=0;i<8;i++) {
        fakes[i].ip.s_addr=htonl(i/4+1);fakes[i].present=1;
        snprintf(fakes[i].dir,sizeof fakes[i].dir,"/media/usb/source%d",i);
        snprintf(fakes[i].port_route,sizeof fakes[i].port_route,"3-2.%d",i+1);
        hub_metadata_prepare(i,fakes[i].dir,fakes[i].port_route);
    }
    for (int i=0;i<8;i++) hub_media_refresh(fakes+i,sent+i);
    assert(!sends);
    fakes[0].link_ready=1;fakes[4].link_ready=1;
    for (int i=0;i<8;i++) hub_media_refresh(fakes+i,sent+i);
    assert(!sends); /* Prepared libraries cannot advertise before native admission. */
    for (int i=0;i<8;i++) fakes[i].announced_at=1;
    for (int i=0;i<8;i++) hub_media_refresh(fakes+i,sent+i);
    assert(sends==8);
    for (int i=0;i<8;i++) hub_media_refresh(fakes+i,sent+i);
    assert(sends==8);
    hub_metadata_invalidate(3);fakes[3].present=0;
    hub_media_refresh(fakes+3,sent+3);assert(sends==8);
    fakes[3].present=1;
    hub_metadata_prepare(3,fakes[3].dir,fakes[3].port_route);
    hub_media_refresh(fakes+3,sent+3);assert(sends==9);
    puts("PASS: actual media encoder prefers configured name/color, preserves transport bytes and rejects retired metadata");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='azhub-media-') as directory:
    d=Path(directory);(d/'test.c').write_text(harness,encoding='utf-8')
    subprocess.run(shlex.split(os.environ.get('TEST_CC','cc'))+['-O2','-Wall','-Wextra','-Wno-unused-function','-pthread','-I'+str(root),str(d/'test.c'),'-o',str(d/'test')],check=True)
    subprocess.run([str(d/'test')],check=True)
