/* SPDX-License-Identifier: GPL-3.0-only */
#define LF_SAFE_EJECT 1
#define EJECT_MAX 8
#include <assert.h>
#include <stdio.h>
#include "eject-presentation.h"
static struct eject_item row(int number,const char *label,unsigned generation) {
    struct eject_item item={.display_number=number,.generation=generation,.devnum=generation};
    snprintf(item.label,sizeof item.label,"%s",label);
    snprintf(item.usb,sizeof item.usb,"/sys/usb/%u",generation);
    return item;
}
int main(void) {
    struct eject_inventory live={.count=6},shown={0};
    live.items[0]=row(4,"Four",4); live.items[1]=row(2,"Two",2);
    live.items[2]=row(0,"Zulu",6); live.items[3]=row(1,"One",1);
    live.items[4]=row(0,"Alpha",5); live.items[5]=row(3,"Three",3);
    hub_eject_sort(&live);
    for (int i=0;i<4;i++) assert(live.items[i].display_number==i+1);
    assert(!strcmp(live.items[4].label,"Alpha") && !strcmp(live.items[5].label,"Zulu"));
    hub_eject_refresh(&shown,&live,0,-1);
    int selected=2; struct eject_item physical=shown.items[selected];
    /* Shuffled observation order and changed label preserve physical target. */
    struct eject_item temp=live.items[0]; live.items[0]=live.items[2]; live.items[2]=temp;
    strcpy(live.items[0].label,"Changed");
    selected=hub_eject_refresh(&shown,&live,0,selected);
    assert(selected==2 && hub_eject_same_row(&shown.items[selected],&physical));
    /* Removing an earlier source moves the same selection to its new row. */
    live.items[2]=live.items[--live.count];
    selected=hub_eject_refresh(&shown,&live,0,selected);
    assert(selected==1 && hub_eject_same_row(&shown.items[selected],&physical));
    /* Replacing the selected connection at the same USB path is not retention. */
    live.items[0].generation++;
    assert(hub_eject_refresh(&shown,&live,0,selected)==-1);
    assert(hub_eject_step(-1,5,1)==0 && hub_eject_step(-1,5,-1)==4);
    assert(hub_eject_step(-1,1,-1)==0 && hub_eject_step(0,1,1)==0);
    assert(hub_eject_step(0,3,-1)==2 && hub_eject_step(2,3,1)==0);
    assert(hub_eject_step(-1,0,1)==-1);
    return 0;
}
