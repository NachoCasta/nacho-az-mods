/* Read-only emulator diagnostic: locate native settings manager, sample sources. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
int main(int argc,char **argv) {
 if(argc<2)return 2;
 char path[128],line[512];snprintf(path,sizeof path,"/proc/%d/maps",atoi(argv[1]));
 FILE *maps=fopen(path,"r");if(!maps)return 3;
 snprintf(path,sizeof path,"/proc/%d/mem",atoi(argv[1]));int fd=open(path,O_RDONLY);if(fd<0)return 4;
 uint64_t object=argc>2?strtoull(argv[2],0,16):0;
 uint64_t *buf=malloc(1048576);if(!buf)return 5;
 while(!object && fgets(line,sizeof line,maps)) {
  unsigned long lo,hi;char perm[8];
  if(sscanf(line,"%lx-%lx %7s",&lo,&hi,perm)!=3 || perm[0]!='r' || perm[1]!='w' || strchr(line,'/') || strstr(line,"[stack]") || hi-lo>(512UL<<20))continue;
  for(unsigned long at=lo;at<hi && !object;at+=1048576) {
   size_t len=hi-at<1048576?hi-at:1048576;ssize_t got=pread(fd,buf,len,at);
   for(ssize_t i=0;i+16<=got;i+=8)if(buf[i/8]==0x2c0fbd0 && buf[i/8+1]==0x2c0fcc8) {object=at+i;break;}
  }
 }
 fclose(maps);free(buf);if(!object){puts("manager not found");return 6;}
 printf("manager=%#lx\n",object);
 unsigned char color[1752],pos[1752];
 for(unsigned tick=0;tick<30;tick++) {
  if(pread(fd,color,sizeof color,object+0x98)!=sizeof color || pread(fd,pos,sizeof pos,object+0x770)!=sizeof pos)return 7;
  printf("%u",tick);
  for(unsigned member=0;member<73;member++)for(unsigned k=0;k<4;k++) {
   unsigned kind=(unsigned[]){0,1,2,8}[k];if(member>0 && color[member*24+kind]==1 && pos[member*24+kind]==1)continue;printf(" m%u/k%u=%u,%u",member,kind,color[member*24+kind],pos[member*24+kind]);
  }
  puts("");fflush(stdout);usleep(200000);
 }
 close(fd);return 0;
}
