#define _GNU_SOURCE
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include "source-settings.h"
int main(int argc,char **argv) {
    assert(argc==2);unsigned char data[141],row[8],defaults[8]={1,1,1,1,1,1,1,1};
    FILE *f=fopen(argv[1],"rb");assert(f);size_t n=fread(data,1,sizeof data,f);fclose(f);
    assert(n==140 && hub_settings_decode(data,n,row));
    assert(row[1]==3 && row[4]==2); /* actual rekordbox export: RGB, bottom */
    assert(!hub_settings_decode(data,n-1,row));assert(!hub_settings_decode(data,n+1,row));
    data[114]^=1;assert(!hub_settings_decode(data,n,row));data[114]^=1;
    data[104]^=1;assert(!hub_settings_decode(data,n,row));data[104]^=1;
    unsigned char packet[1152]={0},output[1152],other[8]={1,1,1,1,1,1,1,1};
    hub_settings_write(packet,sizeof packet,3,row);hub_settings_observe(defaults,packet,sizeof packet);
    assert(defaults[1]==3 && defaults[4]==2);
    memset(packet,0,sizeof packet);hub_settings_observe(defaults,packet,sizeof packet);
    assert(defaults[1]==3 && defaults[4]==2); /* an empty deck cannot reset preferences */
    for(unsigned tick=0;tick<100;tick++) {
        memset(output,tick&1?0:0xa5,sizeof output); /* adversarial changing native deck */
        hub_settings_clear_foreign(output,sizeof output,defaults);
        hub_settings_write(output,sizeof output,3,row);
        hub_settings_write(output,sizeof output,7,other);
        hub_settings_write(output,sizeof output,8,row);
        assert(output[0xda]==3 && output[0xdd]==2);
        assert(output[0x469]==1 && output[0x46c]==1);
        assert(output[0x475]==3 && output[0x478]==2);
    }
    unsigned char empty[8]={0};hub_settings_merge(other,defaults,empty);
    assert(other[1]==3 && other[4]==2);
    assert(!hub_settings_write(output,0xda,3,row));assert(!hub_settings_write(output,sizeof output,6,row));
    unsigned char guard[8];memset(guard,0x55,8);hub_settings_clear_foreign(guard,8,defaults);
    for(unsigned i=0;i<8;i++)assert(guard[i]==0x55);
    puts("source settings: export CRC, invalid/truncated file, defaults, 100 alternating packets and independent USB1/2/3 passed");
}
