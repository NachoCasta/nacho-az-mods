#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Exercise actual loader/presence bodies with native-service stand-ins."""
import os,re,shlex,subprocess,tempfile
from pathlib import Path
root=Path(__file__).resolve().parent
source=(root/'linkfake.c').read_text();community=(root/'community.c').read_text()
def body(text,name):
 m=re.search(r"\b"+name+r"\([^;{}]*\)\s*\{",text);assert m,name
 depth=1
 for i in range(m.end(),len(text)):
  depth+=(text[i]=='{')-(text[i]=='}')
  if not depth:return text[m.end():i]
 raise AssertionError(name)
fake=source[source.index('struct fake {'):source.index('\n};',source.index('struct fake {'))+3]
harness=r"""
#define _GNU_SOURCE
#define LF_SAFE_EJECT 1
#include <assert.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "source-display.h"
#include "source-metadata.h"
#define MAX_FAKES 8
#define lf_log(...) ((void)0)
"""+fake+r"""
static struct fake fakes[MAX_FAKES];static int nfakes;
static unsigned hub_observed_address;static unsigned char hub_peer_ips[2]={1,1};
static int safe_eject_blocked_path(const char *path) {(void)path;return 0;}
static int hub_network_peers(unsigned address,unsigned char *peers) {(void)address;(void)peers;return 1;}
static void probe_udp_sender_add(int fd,int player) {(void)fd;(void)player;}
static const char *media_leaf(int slot) {return slot==3?"usb":slot==7?"usb2":slot==8?"usb3":"sd";}
"""
harness+='static int linkfake_load_config(void){'+body(source,'linkfake_load_config')+'}\n'
harness+='static void community_refresh_presence(const char *config){'+body(community,'community_refresh_presence')+'}\n'
harness+=r"""
static void config(const char *path,unsigned first,unsigned second,const char *route1,const char *route2) {
 FILE *f=fopen(path,"w");assert(f);
 if(first)fprintf(f,"#hub-port %u %s\n7 10.0.2.54 3 /media/usb/first A\n",first,route1);
 if(second)fprintf(f,"#hub-port %u %s\n7 10.0.2.54 7 /media/usb/second B\n",second,route2);
 fclose(f);
}
int main(void) {
 char path[]="/tmp/azhub-port-reconcile-XXXXXX";int fd=mkstemp(path);assert(fd>=0);close(fd);
 setenv("LINKFAKE_CONF",path,1);setenv("LINKFAKE_AUTODISCOVER","1",1);
 config(path,4,0,"2-1.4","");assert(linkfake_load_config()==1);
 assert(hub_source_display_number(fakes[0].player,fakes[0].slot)==4);
 fakes[0].srv_info=0xabc;fakes[0].tag_cache=0xdef;int original_socket=fakes[0].sock;
 config(path,4,1,"2-1.4","2-1.1");community_refresh_presence(path);assert(linkfake_load_config()==2);
 assert(hub_source_display_number(fakes[1].player,fakes[1].slot)==1);
 assert(fakes[0].sock==original_socket && fakes[0].srv_info==0xabc && fakes[0].tag_cache==0xdef);
 hub_metadata_prepare(0,fakes[0].dir,fakes[0].port_route);
 struct hub_metadata_ticket metadata=hub_metadata_capture(0);
 struct hub_source_properties properties={.name_valid=1,.color_valid=1,.color=6};
 strcpy(properties.name,"Configured name");assert(hub_metadata_publish(0,&metadata,&properties));
 /* Structural rank changes must not retire a source on the same physical route. */
 config(path,7,1,"2-1.4","2-1.1");community_refresh_presence(path);assert(linkfake_load_config()==2);
 assert(fakes[0].present && !fakes[0].unmount_pending);
 assert(hub_source_display_number(fakes[0].player,fakes[0].slot)==4);
 assert(hub_metadata_read(0,metadata.root,metadata.route,&properties) && properties.name_valid);
 /* Move that mount to physical port3: wait for old native unmount before admission. */
 config(path,3,1,"2-1.3","2-1.1");community_refresh_presence(path);
 assert(!fakes[0].present && fakes[0].unmount_pending);assert(linkfake_load_config()==2);
 assert(!hub_metadata_read(0,metadata.root,metadata.route,&properties));
 assert(!hub_metadata_publish(0,&metadata,&properties));
 fakes[2].srv_music=0x1234;fakes[2].srv_info=0x5678;
 fakes[0].unmount_pending=0;assert(linkfake_load_config()==3);
 assert(fakes[2].srv_music==0x1234 && fakes[2].srv_info==0x5678);
 assert(hub_source_display_number(fakes[2].player,fakes[2].slot)==3);
 assert(!fakes[0].present && fakes[1].present && fakes[2].present);
 assert(fakes[0].srv_info==0xabc && fakes[0].tag_cache==0xdef);
 /* Returning to an old route also waits for its outgoing incarnation to drain. */
 config(path,4,1,"2-1.4","2-1.1");community_refresh_presence(path);
 assert(!fakes[0].present && fakes[2].unmount_pending);
 fakes[2].unmount_pending=0;community_refresh_presence(path);
 assert(fakes[0].present && !fakes[2].present && fakes[1].present);
 for(int i=0;i<nfakes;i++)close(fakes[i].sock);
 unlink(path);puts("PASS: actual loader/presence bodies preserve ports, neighbor servers/cache, topology-shift labels and drain before moved-source admission");
}
"""
with tempfile.TemporaryDirectory(prefix='azhub-port-reconciliation-') as tmp:
 p=Path(tmp);(p/'test.c').write_text(harness)
 subprocess.run(shlex.split(os.environ.get('TEST_CC','cc'))+['-O2','-Wall','-Wextra','-pthread','-I'+str(root),str(p/'test.c'),'-o',str(p/'test')],check=True)
 subprocess.run([str(p/'test')],check=True)
