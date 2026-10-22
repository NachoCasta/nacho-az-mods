/* SPDX-License-Identifier: GPL-3.0-only */
/* Emulator-only identity control. Creates one generated64-byte fixture, drops
 * this VM's inode caches and removes the fixture. Never a physical AZ tool. */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/utsname.h>
#include "../../linkfake/public-pdb-source.h"
int main(int argc,char **argv) {
 struct utsname kernel;
 if(argc!=2 || strcmp(argv[1],"--emulator-only") || uname(&kernel) || strcmp(kernel.release,"6.6.138-dirty")) return 126;
 char path[]="/media/usb/sdc1/.azhub-inode-probe-XXXXXX";
 int fd=mkstemp(path);if(fd<0){perror("fixture");return 1;}
 unsigned char content[64]={0};assert(write(fd,content,sizeof content)==sizeof content);close(fd);
 struct hub_public_root root;assert(!hub_public_root_init(&root));assert(!hub_public_root_attach(&root,"/media/usb/sdc1",1));
 const char *relative=strrchr(path,'/')+1;
 struct hub_public_pdb_source source;assert(!hub_public_pdb_source_init(&source,&root,1,relative,strlen(relative)));
 for(unsigned pin=0;pin<2;++pin) {
  int anchor=pin ? open(path,O_RDONLY|O_CLOEXEC):-1;
  struct stat before,after;assert(!stat(path,&before));sync();
  int cache=open("/proc/sys/vm/drop_caches",O_WRONLY|O_CLOEXEC);
  if(cache<0){perror("emulator cache control");unlink(path);return 1;}
  assert(write(cache,"2\n",2)==2);close(cache);assert(!stat(path,&after));
  unsigned char byte;errno=0;int result=hub_public_pdb_source_read(&source,0,&byte,1),error=errno;
  printf("pin=%u inode_before=%llu inode_after=%llu pdb_read=%d errno=%d\n",pin,(unsigned long long)before.st_ino,(unsigned long long)after.st_ino,result,error);
  if(anchor>=0)close(anchor);
  assert(!hub_public_pdb_source_init(&source,&root,1,relative,strlen(relative)));
 }
 hub_public_root_withdraw(&root);assert(!hub_public_root_destroy(&root));assert(!unlink(path));
 return 0;
}
