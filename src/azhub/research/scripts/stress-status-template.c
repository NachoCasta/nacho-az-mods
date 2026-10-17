/* Emulator-only adversarial input: alternate captured transport settings.
 * Does not edit the native settings manager or source files. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
int main(int argc,char **argv) {
 if(argc!=3)return 2;
 char path[128],line[512];snprintf(path,sizeof path,"/proc/%d/maps",atoi(argv[1]));
 FILE *maps=fopen(path,"r");if(!maps)return 3;
 unsigned long base=0,lo,hi,off;char perms[8];
 while(fgets(line,sizeof line,maps))if(strstr(line,"/hub-libraries.so") && sscanf(line,"%lx-%lx %7s %lx",&lo,&hi,perms,&off)==4){base=lo-off;break;}
 fclose(maps);if(!base)return 4;
 snprintf(path,sizeof path,"/proc/%d/mem",atoi(argv[1]));int fd=open(path,O_RDWR);if(fd<0)return 5;
 unsigned long target=base+strtoull(argv[2],0,16);
 for(unsigned tick=0;tick<200;tick++) {
  unsigned char row[8]={1,3,1,2,2,1,2,1};if(tick&1)memset(row,0,8);
  const unsigned offsets[]={0xd9,0xf9,0x468,0x474};
  for(unsigned i=0;i<4;i++)if(pwrite(fd,row,8,target+offsets[i])!=8)return 6;
  usleep(100000);
 }
 close(fd);puts("200 alternating RGB/bottom and empty deck templates injected (20s)");return 0;
}
