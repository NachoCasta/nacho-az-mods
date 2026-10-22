/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include "source-metadata.h"
static struct hub_source_properties value(const char *name,int color) {
    struct hub_source_properties p={.name_valid=1,.color_valid=1,.color=(unsigned char)color};
    snprintf(p.name,sizeof p.name,"%s",name);return p;
}
static void *replace_loop(void *unused) {
    (void)unused;
    for (int i=0;i<5000;i++) {
        hub_metadata_prepare(7,"/media/usb/racing","3-2.8");
        struct hub_metadata_ticket ticket=hub_metadata_capture(7);
        struct hub_source_properties p=value(i%2 ? "Aqua":"Yellow",i%2 ? 6:4);
        assert(hub_metadata_publish(7,&ticket,&p));
        hub_metadata_invalidate(7);
        assert(!hub_metadata_publish(7,&ticket,&p));
    }
    return NULL;
}
static void *read_loop(void *unused) {
    (void)unused;
    for (int i=0;i<20000;i++) {
        struct hub_source_properties p;
        hub_metadata_read(7,"/media/usb/racing","3-2.8",&p);
        if (p.name_valid) assert((!strcmp(p.name,"Aqua") && p.color==6) ||
                                (!strcmp(p.name,"Yellow") && p.color==4));
        else assert(!p.color_valid && !*p.name);
    }
    return NULL;
}
int main(void) {
    const char *root="/media/usb/sdc1",*route="3-2.4";
    struct hub_source_properties p=value("Nacho Chips",6),read;char name[128];
    hub_metadata_prepare(0,root,route);struct hub_metadata_ticket first=hub_metadata_capture(0);
    assert(!hub_metadata_complete(0,root,route));
    assert(hub_metadata_publish(0,&first,&p));
    assert(hub_metadata_complete(0,root,route));
    assert(!hub_metadata_complete(0,root,"3-2.7"));
    uint64_t revision=hub_metadata_read(0,root,route,&read);
    assert(revision && read.color==6 && !strcmp(read.name,"Nacho Chips"));
    hub_metadata_name(&read,"NACHO CHIPS",4,name,sizeof name);assert(!strcmp(name,"Nacho Chips"));
    assert(!hub_metadata_publish(0,&first,&p));
    assert(hub_metadata_read(0,root,route,&read)==revision);
    hub_metadata_prepare(1,"/media/usb/sdd1","3-2.2");struct hub_metadata_ticket neighbor=hub_metadata_capture(1);
    struct hub_source_properties other=value("Neighbor",4);assert(hub_metadata_publish(1,&neighbor,&other));
    hub_metadata_invalidate(0);assert(!hub_metadata_publish(0,&first,&p));
    assert(!hub_metadata_complete(0,root,route));
    assert(!hub_metadata_read(0,root,route,&read) && !read.name_valid);
    hub_metadata_prepare(0,root,route); /* Same path/route, new physical connection. */
    assert(!hub_metadata_publish(0,&first,&p));
    struct hub_metadata_ticket current=hub_metadata_capture(0);
    assert(current.generation!=first.generation);
    p=value("C\xc3\xa1mi",0);assert(hub_metadata_publish(0,&current,&p));
    assert(!hub_metadata_read(0,root,"3-2.7",&read) && !read.name_valid);
    assert(hub_metadata_read(0,root,route,&read) && !strcmp(read.name,"C\xc3\xa1mi"));
    assert(hub_metadata_read(1,neighbor.root,neighbor.route,&read) && !strcmp(read.name,"Neighbor") && read.color==4);
    memset(&read,0,sizeof read);hub_metadata_name(&read,"CAMI",4,name,sizeof name);assert(!strcmp(name,"CAMI"));
    hub_metadata_name(&read,"",4,name,sizeof name);assert(!strcmp(name,"USB 4"));
    hub_metadata_name(&read,NULL,8,name,sizeof name);assert(!strcmp(name,"USB 8"));
    assert(!hub_metadata_publish(8,&current,&p));assert(!hub_metadata_read(-1,root,route,&read));
    pthread_t writer,reader;
    assert(!pthread_create(&writer,NULL,replace_loop,NULL));
    assert(!pthread_create(&reader,NULL,read_loop,NULL));
    assert(!pthread_join(writer,NULL));assert(!pthread_join(reader,NULL));
    return 0;
}
