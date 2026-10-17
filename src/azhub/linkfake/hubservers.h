/* SPDX-License-Identifier: GPL-3.0-only */
/* EP147 2.00 emulator experiment. Factory returns a nontrivial C++ pointer
 * through x8; a 24-byte C aggregate uses that same indirect-result register.
 * Objects are intentionally retained until process exit. */
struct hub_result { uint64_t object, unused[2]; };
typedef struct hub_result (*hub_factory_fn)(void *, void *, void *, void *);
static hub_factory_fn hub_factory_original;
static fn8 hub_holder_original;
static uint64_t hub_music_vt[MAX_FAKES][42], hub_info_vt[MAX_FAKES][42];
static uint64_t hub_owners[16];
static unsigned hub_owner_counts[16];
static int hub_slot_code(int player, int slot) {
    if (!getenv("LINKFAKE_HUBSRV")) return 0;
    for (int i=0; i<nfakes; i++) if (fakes[i].player==player && fakes[i].slot==slot) return 0xe0+i;
    return 0;
}
static __thread uint64_t hub_matching_message, hub_matching_server;
static uint64_t hub_matches(uint64_t server, uint64_t message) {
    uint32_t dmst=((uint32_t (*)(uint64_t,int))0xf05680)(message,0);
    for (int i=0; i<nfakes; i++)
        if (fakes[i].present && (server==fakes[i].srv_music || server==fakes[i].srv_info))
            if (((dmst>>8)&255)==(unsigned)(0xe0+i)) {
                hub_matching_message=message; hub_matching_server=server;
                return 1;
            }
    return 0;
}
static uint64_t hub_accept(uint64_t server, uint16_t command) {
    uint64_t native_vt=0;
    for (int i=0;i<nfakes;i++) {
        if (server==fakes[i].srv_music) native_vt=0x2bb1268;
        if (server==fakes[i].srv_info) native_vt=0x2cafeb8;
    }
    if (!native_vt) return 0;
    uint64_t accepted=((uint64_t (*)(uint64_t,uint16_t))((uint64_t *)native_vt)[9])(server,command);
    if (hub_matching_server==server && hub_matching_message) {
        uint64_t message=hub_matching_message;
        hub_matching_message=hub_matching_server=0;
        if (accepted & 255) {
            uint64_t begin=*(uint64_t *)(message+0x28),end=*(uint64_t *)(message+0x30);
            if (begin && end>=begin+0x30 && *(uint32_t *)begin==1) {
                uint32_t *dmst=(uint32_t *)(begin+8);
                *dmst=(*dmst & ~0xff00u) | 0x300;
            }
        }
    }
    return accepted;
}
/* File-identity lookup is a separate service path from dbserver dispatch. */
static __thread uint64_t hub_info_library;
static fn8 hub_identity_lookup_original;
static uint64_t hub_identity_lookup(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
                                    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    if (hub_info_library) {
        uint64_t *handle=(uint64_t *)a;
        handle[0]=hub_info_library+0x160; /* IFileIdentityInfoGetter */
        handle[1]=0; /* direct process-lifetime object; no mapped-object lease */
        return 1;
    }
    return hub_identity_lookup_original(a,b,c,d,e,f,g,h);
}
static void hub_info_process(uint64_t server,uint64_t message,uint32_t connection) {
    uint64_t previous=hub_info_library;
    for (int i=0;i<nfakes;i++) if (fakes[i].srv_info==server)
        hub_info_library=fakes[i].srv_music;
    ((void (*)(uint64_t,uint64_t,uint32_t))0x19802c8)(server,message,connection);
    hub_info_library=previous;
}
static void hub_override(uint64_t server, uint64_t *vt) {
    memcpy(vt,*(uint64_t **)server-2,42*sizeof(uint64_t));
    vt[2+0x80/8]=(uint64_t)hub_matches;
    vt[2+0x48/8]=(uint64_t)hub_accept;
    *(uint64_t **)server=vt+2;
}
static void hub_create_music(int i) {
    if (fakes[i].srv_music) return;
        uint64_t nm=0;
        uint8_t id=2,alternate=0;
        char label[32];
        snprintf(label,sizeof label,"HubLib%d",i);
        ((void (*)(void *,const char *))0x24876f0)(&nm,label);
        /* The factory's vector contains per-source mutable mount caches, NOT
         * global services. Reusing stock vec[0] couples the new library to USB1.
         * Reproduce the in-place shared object built at 0x1380e9c. Retain it. */
        uint64_t *cache=calloc(1,0x50);
        cache[0]=0x2bb6040; cache[1]=0x100000001ULL;
        cache[2]=0x2bb5f00; cache[3]=0x2bb5f58;
        uint64_t entry[2]={(uint64_t)(cache+2),(uint64_t)cache};
        uint64_t ownvec[3]={(uint64_t)entry,(uint64_t)(entry+2),(uint64_t)(entry+2)};
        struct hub_result extra=hub_factory_original(ownvec,&nm,&id,&alternate);
        fakes[i].srv_music=extra.object;

        lf_log("hub factory player %d music=%#lx",fakes[i].player,extra.object);
}
__attribute__((used,noinline)) static void hub_factory_c(void *vec,void *name,void *sdid,void *flag,uint64_t *out) {
    struct hub_result stock=hub_factory_original(vec,name,sdid,flag);
    if (*(uint8_t *)sdid==2) for (int i=0;i<nfakes;i++) hub_create_music(i);
    *out=stock.object;
}
/* The stock unique_ptr result occupies only 8 bytes. Never return the 24-byte
 * ABI shim aggregate directly to it: capture x8 and write exactly one word. */
__asm__(".text\n.align 2\n.type hub_factory_entry,%function\nhub_factory_entry:\n"
        "stp x29,x30,[sp,#-16]!\nmov x29,sp\nmov x4,x8\n"
        "bl hub_factory_c\nldp x29,x30,[sp],#16\nret\n");
extern void hub_factory_entry(void);
static void hub_finish_servers(uint64_t common,uint64_t timer) {
    for (int i=0; i<nfakes; i++) {
        uint64_t s=fakes[i].srv_music;
        if (!s || fakes[i].srv_info) continue;
        uint8_t id=2,alternate=0;
        void *tag=calloc(1,0x18);
        ((void (*)(void *,void *))0x1384ae8)(tag,&id);
        uint64_t *vt=*(uint64_t **)s;
        ((void (*)(uint64_t,void *))vt[0x88/8])(s,tag);
        ((void (*)(uint64_t,uint64_t))vt[0x90/8])(s,common);
        ((void (*)(uint64_t,uint64_t))vt[0x98/8])(s,timer);
        *(uint64_t *)(s+0x150)=((uint64_t (*)(uint64_t))0x1385378)(s);
        hub_override(s,hub_music_vt[i]);
        ((void (*)(uint64_t))0x137ef60)(s);
        uint64_t nm=0;
        char label[32];
        snprintf(label,sizeof label,"HubInfo%d",i);
        ((void (*)(void *,const char *))0x24876f0)(&nm,label);
        s=(uint64_t)calloc(1,0x210);
        ((void (*)(uint64_t,void *,void *,void *))0x1974f50)(s,&nm,&id,&alternate);

        fakes[i].srv_info=s;
        *(uint64_t *)(s+0x150)=((uint64_t (*)(uint64_t))0x1385378)(s);
        hub_override(s,hub_info_vt[i]);
        hub_info_vt[i][2+0x50/8]=(uint64_t)hub_info_process;
        ((void (*)(uint64_t,int,int))0x7d58b8)(s,5,0);
        lf_log("hub initialized player %d music=%#lx info=%#lx",fakes[i].player,fakes[i].srv_music,s);
    }
}
static uint64_t hub_holder(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
                           uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uint64_t result=hub_holder_original(a,b,c,d,e,f,g,h);
    uint64_t *holder=(uint64_t *)a;
    hub_finish_servers(holder[0x3a],holder[0x27]);
    return result;
}
/* Register on the dispenser thread, before it traverses the list. */
static void hub_register(uint64_t owner) {
    unsigned count=nfakes;
    int k;
    for (k=0; k<16 && hub_owners[k] && hub_owners[k]!=owner; k++) {}
    if (k==16 || !count || hub_owner_counts[k]>=count) return;
    unsigned registered=hub_owner_counts[k];
    uint64_t stock_music=0,stock_info=0;
    uint64_t *scanhead=(uint64_t *)(owner+0x2c0);
    for (uint64_t *n=(uint64_t *)scanhead[0];n!=scanhead;n=(uint64_t *)n[0]) {
        uint64_t s=n[2],vt=*(uint64_t *)s;
        if (*(uint8_t *)(s+0xd0)!=2) continue;
        if (vt==0x2bb1268) stock_music=s;
        if (vt==0x2cafeb8) stock_info=s;
    }
    if (!stock_music || !stock_info) return;
    if (getenv("LINKFAKE_LATE_SERVERS")) {
        uint64_t commands=*(uint64_t *)(stock_music+0x1d0);
        uint64_t component=commands ? *(uint64_t *)(commands+0x30) : 0;
        uint64_t nested=component ? *(uint64_t *)(component+0x78) : 0;
        uint64_t common=component ? *(uint64_t *)(component+0xa0) : 0;
        uint64_t timer=nested ? *(uint64_t *)(nested+0x60) : 0;
        if (!common || !timer) return;
        for (unsigned i=registered;i<count;i++) hub_create_music(i);
        hub_finish_servers(common,timer);
        lf_log("hub late construction common=%#lx timer=%#lx",common,timer);
    }
    if (!fakes[0].srv_info) return;
    for (unsigned i=registered;i<count;i++) {
        uint64_t targets[2]={fakes[i].srv_music,fakes[i].srv_info};
        uint64_t sources[2]={stock_music,stock_info};
        for (int j=0;j<2;j++) {
            uint64_t s=targets[j],stock=sources[j];
            /* Shared system services only: each server keeps its own database,
             * request dispatcher, queue, mounted media ID and thread. */
            memcpy((void *)(s+0xd8),(void *)(stock+0xd8),24);
            uint64_t service_key=*(uint64_t *)(s+0xe0);

            lf_log("hub services kind %d provider=%#lx registry_key=%#lx sender_key=%#lx",j,
                   *(uint64_t *)(s+0xd8),service_key,*(uint64_t *)(s+0xe8));
        }
        uint64_t s=targets[0];
        memcpy((void *)(s+0x190),(void *)(stock_music+0x190),0x38);

        uint64_t *vt=*(uint64_t **)s;
        uint64_t commands=*(uint64_t *)(stock_music+0x1d0);
        uint64_t reply=commands ? *(uint64_t *)(commands+0xd0) : 0;
        uint64_t sender=reply ? *(uint64_t *)reply : 0;
        ((void (*)(uint64_t,uint64_t))vt[0x58/8])(s,sender);
        lf_log("hub reply sender copied %#lx",sender);
    }
    hub_owners[k]=owner;
    uint64_t *head=(uint64_t *)(owner+0x2c0);
    for (unsigned i=registered; i<count; i++) {
        uint64_t objects[2]={fakes[i].srv_music,fakes[i].srv_info};
        for (int j=0; j<2; j++) {
            uint64_t *node=malloc(24);
            node[0]=(uint64_t)head; node[1]=head[1]; node[2]=objects[j];
            ((uint64_t *)head[1])[0]=(uint64_t)node;
            head[1]=(uint64_t)node;
            head[2]++;
        }
    }
    hub_owner_counts[k]=count;
    lf_log("hub registered %u servers with %#lx (new %u)",count*2,owner,(count-registered)*2);
}
static void hub_mount_poll(void) {
    static time_t started, ready_at;
    static int waiting=-1;
    time_t now=time(NULL);
    if (!started) started=now;
    if (waiting>=0 && !fakes[waiting].present) {
        waiting=-1; ready_at=now+3;
    }
    /* Native callbacks enqueue unmount on each owned server's thread. Keep
     * objects alive and serialize firmware mailbox writes with announcements. */
    for (int i=0;i<nfakes;i++) {
        struct fake *f=&fakes[i];
        if (f->unmount_pending==1 && waiting<0 && now>=ready_at) {
            uint16_t id=0x102;
            if (f->srv_music) ((void (*)(uint64_t,void *,void *))0x1373928)(f->srv_music,&id,NULL);
            if (f->srv_info) ((void (*)(uint64_t,void *,void *))0x1975468)(f->srv_info,&id,NULL);
            FILE *box=fopen("/proc/udev_nfs","w");
            if (!box) continue;
            fprintf(box,"umount %s nfs",f->mp); fclose(box);
            f->unmount_pending=2; f->removed_at=now;
            f->mounted=0; f->mount_at=0;
            ready_at=now+3;
            lf_log("hub removal queued player %d slot %d path=%s",f->player,f->slot,f->dir);
            break;
        }
        if (f->unmount_pending==2 && now-f->removed_at>=3) {
            char command[192];
            snprintf(command,sizeof command,"! mountpoint -q %s || umount %s",f->mp,f->mp);
            if (!system(command)) {
                f->unmount_pending=0;
                lf_log("hub removal alias released player %d slot %d",f->player,f->slot);
            }
        }
    }
    if (waiting>=0) {
        struct fake *f=&fakes[waiting];
        if (__atomic_load_n(&f->queried_at,__ATOMIC_ACQUIRE)>=f->announced_at) {
            lf_log("hub announcement acknowledged player %d slot %d",f->player,f->slot);
            waiting=-1; ready_at=now+3;
        } else if (now-f->announced_at>=12) {
            lf_log("hub announcement retry player %d slot %d",f->player,f->slot);
            __atomic_store_n(&f->announced_at,0,__ATOMIC_RELEASE);
            waiting=-1; ready_at=now+1;
        }
    }
    if (waiting<0 && now>=ready_at && now-started>=20) {
        for (int i=0;i<nfakes;i++) {
            struct fake *f=&fakes[i];
            if (!f->present || f->unmount_pending || !__atomic_load_n(&f->notify_pending,__ATOMIC_ACQUIRE) || access(f->dir,R_OK)) continue;
            char command[768];
            snprintf(command,sizeof command,
                "mkdir -p %s && { mountpoint -q %s || mount -o bind,ro '%s' %s; }",
                f->mp,f->mp,f->dir,f->mp);
            struct stat source_stat,alias_stat;
            if (!stat(f->dir,&source_stat) && !stat(f->mp,&alias_stat) &&
                (source_stat.st_dev!=alias_stat.st_dev || source_stat.st_ino!=alias_stat.st_ino)) {
                /* A prior EP147 run may have assigned this alias to another
                 * stick. Never let an old bind point playback at that stick. */
                snprintf(command,sizeof command,
                    "mkdir -p %s && { ! mountpoint -q %s || umount %s; } && mount -o bind,ro '%s' %s",
                    f->mp,f->mp,f->mp,f->dir,f->mp);
            }
            int rc=system(command);
            if (rc) { lf_log("hub alias failed player %d slot %d rc=%d",f->player,f->slot,rc); continue; }
            __atomic_store_n(&f->announced_at,now,__ATOMIC_RELEASE);
            FILE *box=fopen("/proc/udev_nfs","w");
            if (!box) { __atomic_store_n(&f->announced_at,0,__ATOMIC_RELEASE); continue; }
            fprintf(box,"mount %s nfs",f->mp);
            fclose(box);
            waiting=i;
            lf_log("hub announce player %d slot %d path=%s",f->player,f->slot,f->mp);
            break;
        }
    }
    for (int i=0;i<nfakes;i++) {
        struct fake *f=&fakes[i];
        if (!f->present || f->unmount_pending || !f->srv_info || f->mounted || !f->mount_at || now<f->mount_at) continue;
        if (access(f->mp,R_OK)) continue;
        /* Each owned server/cache uses its constructed SDID 2, generation 1.
         * Registry lookup translates this private context to remote identity. */
        uint16_t id=0x102;
        uint64_t path=0;
        ((void (*)(void *,const char *))0x24a1180)(&path,f->dir);
        ((void (*)(uint64_t,void *,void *,int,void *))0x1373ca8)(f->srv_music,&id,&path,0,NULL);
        ((void (*)(uint64_t,void *,void *))0x1974ff8)(f->srv_info,&id,NULL);
        ((void (*)(void *))0x2469aa0)(&path);
        f->mounted=1;
        lf_log("hub mount queued player %d slot %d id=%#x path=%s",f->player,f->slot,id,f->dir);
    }
}
static fn8 hub_usb2_original,hub_usb3_original;
static uint64_t hub_available(int slot,fn8 original,uint64_t a,uint64_t b,uint64_t c,uint64_t d,
                              uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    if (getenv("LINKFAKE_SLOT_CAPS") && b>=0xff0000000000ULL && b<0x1000000000000ULL) {
        /* LinkMemberID: kind 2=SinglePlayer, next byte=zero-based player index.
         * Do not change the AZ's own members or real/unconfigured sources. */
        uint32_t member=*(uint32_t *)b;
        for (int i=0;i<nfakes;i++)
            if (fakes[i].slot==slot && member==((unsigned)(fakes[i].player-1)<<8|2u))
                return 1;
    }
    return original(a,b,c,d,e,f,g,h);
}
static uint64_t hub_usb2(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    return hub_available(7,hub_usb2_original,a,b,c,d,e,f,g,h);
}
static uint64_t hub_usb3(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    return hub_available(8,hub_usb3_original,a,b,c,d,e,f,g,h);
}
static void hub_install(void) {
    hub_usb2_original=make_hook(0x1869b48,hub_usb2);
    hub_usb3_original=make_hook(0x1869e98,hub_usb3);
    hub_identity_lookup_original=make_hook(0x1981560,hub_identity_lookup);
    /* Streaming service activation is unrelated to USB initialization. */
    if (getenv("LINKFAKE_LATE_SERVERS")) {
        hub_factory_original=(hub_factory_fn)0x137bc60;
    } else {
        hub_factory_original=(hub_factory_fn)make_hook(0x137bc60,hub_factory_entry);
        hub_holder_original=make_hook(0x13809f0,hub_holder);
    }
}
