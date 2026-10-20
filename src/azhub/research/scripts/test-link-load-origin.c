/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdio.h>
#define LF_SAFE_EJECT 1
#define EJECT_MAX 16
#include "../../linkfake/eject-ui-model.h"
#include "link-load-origin-probe.h"
int main(void) {
    struct eject_item original={.generation=100,.devnum=4,.usb="/sys/usb/2-1.3"};
    struct eject_item reserved=original;reserved.state=1;
    assert(probe_load_origin_matches(1,&original,&reserved,4,2));
    reserved.state=3;assert(probe_load_origin_matches(1,&original,&reserved,4,2));
    for(int state=0;state<5;state++) if(state!=1 && state!=3) {
        reserved.state=state;assert(!probe_load_origin_matches(1,&original,&reserved,4,2));
    }
    reserved.state=1;
    assert(!probe_load_origin_matches(0,&original,&reserved,4,2));
    assert(!probe_load_origin_matches(1,NULL,&reserved,4,2));
    assert(!probe_load_origin_matches(1,&original,NULL,4,2));
    assert(!probe_load_origin_matches(1,&original,&reserved,2,2));
    assert(!probe_load_origin_matches(1,&original,&reserved,~0u,-1));
    assert(!probe_load_origin_matches(1,&original,&reserved,~0u,8));
    /* Reconnect can keep every visible identifier, even wrapping devnum. */
    strcpy(original.label,"Same USB");strcpy(reserved.label,"Same USB");
    strcpy(original.disk,"sdc");strcpy(reserved.disk,"sdc");
    for(unsigned cycle=1;cycle<1000;cycle++) {
        reserved.generation=original.generation+cycle;
        reserved.devnum=cycle%127+1;
        assert(!probe_load_origin_matches(1,&original,&reserved,4,2));
    }
    reserved=original;reserved.state=1;reserved.devnum++;
    assert(!probe_load_origin_matches(1,&original,&reserved,4,2));
    reserved=original;reserved.state=1;strcpy(reserved.usb,"/sys/usb/2-1.4");
    assert(!probe_load_origin_matches(1,&original,&reserved,4,2));
    original.generation=0;reserved=original;reserved.state=1;
    assert(!probe_load_origin_matches(1,&original,&reserved,4,2));
    original.generation=100;original.devnum=0;reserved=original;reserved.state=1;
    assert(!probe_load_origin_matches(1,&original,&reserved,4,2));
    original.devnum=4;original.usb[0]=0;reserved=original;reserved.state=1;
    assert(!probe_load_origin_matches(1,&original,&reserved,4,2));
    puts("load-origin reservation tests passed");
}
