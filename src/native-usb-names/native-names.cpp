/* SPDX-License-Identifier: GPL-3.0-only */
#include "azmod-mod.h"
#include <stdint.h>
#include <string.h>
#include <sys/utsname.h>
#include <atomic>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include "name-policy.h"
#include "volume-label.h"
#include "source-refresh.h"
#define MOD_ID "nachocasta-native-usb-names"
static const az_core_v1 *api;
/* Nontrivial return types preserve the native AAPCS64 indirect x8 ABI. */
struct NativeString {
    uintptr_t text;
    NativeString() { ((void (*)(void *))0x247c9b0)(this); }
    explicit NativeString(const char *value) { ((void (*)(void *,const char *))0x24a1180)(this,value); }
    NativeString(const NativeString &v) { ((void (*)(void *,const void *))0x247c9d0)(this,&v); }
    ~NativeString() { ((void (*)(void *))0x2469aa0)(this); }
    const char *utf8() const { return text ? (const char *)text : ""; }
};
struct NativePath { NativeString path; uint32_t flags; };
typedef NativeString (*NameFn)(uintptr_t,uintptr_t,const unsigned char *);
typedef void (*MountFn)(uintptr_t,const unsigned char *,uintptr_t,uintptr_t,uintptr_t,uintptr_t);
static NameFn original_name;
static MountFn original_mount;
static void (*original_information_task)(uintptr_t);
/* Requests are plain values, never retained provider or PropertyTable pointers.
 * UI readers use trylock; the worker performs all filesystem access unlocked. */
struct Slot {
    uint64_t epoch,done;
    uintptr_t manager;
    unsigned char media[8];
    char root[256],label[128];
};
static Slot slots[2];
static std::atomic<uint64_t> epochs[2],pending_refresh[2],applied_epoch[2],notified_epoch[2];
static std::atomic<unsigned> refresh_delay[2];
static std::atomic<bool> active(false);
static pthread_mutex_t state_lock=PTHREAD_MUTEX_INITIALIZER;
static void refresh(void *arg) {
    if(!active.load())return;
    unsigned port=(unsigned)(uintptr_t)arg;
    uint64_t pending=pending_refresh[port].load();
    if(!pending || notified_epoch[port].load()==pending || refresh_delay[port].load())return;
    Slot copy;
    if(pthread_mutex_trylock(&state_lock))return; // Retry from tick; never lose a refresh to contention.
    copy=slots[port];
    pthread_mutex_unlock(&state_lock);
    if(copy.epoch!=pending || copy.epoch!=epochs[port].load() || copy.done!=copy.epoch || !copy.manager) {
        pending_refresh[port].compare_exchange_strong(pending,0);return;
    }
    try {
        /* The exact native storage manager is a process-lifetime service. Resolve
         * the complete current MediaID again before notifying its own listeners. */
        if(*(uintptr_t *)copy.manager!=0x2b7b538) {
            pending_refresh[port].compare_exchange_strong(pending,0);return;
        }
        NativePath path=((NativePath (*)(uintptr_t,const unsigned char *))0x11199b8)(copy.manager,copy.media);
        if(strcmp(path.path.utf8(),copy.root) || copy.epoch!=epochs[port].load()) {
            pending_refresh[port].compare_exchange_strong(pending,0);return;
        }
        if(pending_refresh[port].load()!=pending)return;
        notified_epoch[port].store(pending);
        ((void (*)(uintptr_t,const unsigned char *))0x111d538)(copy.manager,copy.media);
        api->log(MOD_ID,"native USB%u information-change notified epoch=%llu",port+1,(unsigned long long)copy.epoch);
    } catch(...) {
        pending_refresh[port].compare_exchange_strong(pending,0);
        api->log(MOD_ID,"native label refresh unavailable; keeping native presentation");
    }
}
static void naming_tick(void) {
    for(unsigned port=0;port<2;port++) {
        unsigned delay=refresh_delay[port].load();
        if(delay)refresh_delay[port].fetch_sub(1);
        else if(pending_refresh[port].load())refresh((void *)(uintptr_t)port);
    }
}
/* Return a label only from the exact mounted native root. No mounts, database
 * reads, disk metadata changes, private servers or native slot borrowing. */
static unsigned mount_identity(const char *root,unsigned *major,unsigned *minor,char device[256]) {
    FILE *f=fopen("/proc/self/mountinfo","re");if(!f)return 0;
    char line[4096],mount[256],base[256],fs[64];unsigned id=0,parent,a,b,found=0,count=0;
    while(count++<512 && fgets(line,sizeof line,f)) {
        if(sscanf(line,"%u %u %u:%u %255s %255s",&id,&parent,&a,&b,base,mount)!=6 || strcmp(mount,root))continue;
        const char *tail=strstr(line," - ");
        if(!tail || sscanf(tail+3,"%63s %255s",fs,device)!=2 || strncmp(device,"/dev/",5))continue;
        *major=a;*minor=b;found=id;break;
    }
    fclose(f);return found;
}
static int read_label(const char *root,char result[128]) {
    if(strncmp(root,"/media/usb/",11))return 0;
    unsigned major=0,minor=0,after_major=0,after_minor=0;
    char device[256]={0},after_device[256]={0},path[128],raw[512]={0};
    unsigned before=mount_identity(root,&major,&minor,device);
    if(!before)return 0;
    snprintf(path,sizeof path,"/run/udev/data/b%u:%u",major,minor);
    int found=nusb_label_from_udev(path,raw,sizeof raw);
    if(!found)found=nusb_label_from_links("/dev/disk/by-label",device,raw,sizeof raw);
    unsigned after=mount_identity(root,&after_major,&after_minor,after_device);
    return found && before==after && major==after_major && minor==after_minor &&
        !strcmp(device,after_device) && nusb_label(raw,result);
}
#ifndef NUSB_READ_LABEL
#define NUSB_READ_LABEL read_label
#endif
static void *worker(void *) {
    unsigned round=0;
    for(;;) {
        ++round;
        if(active.load())for(unsigned port=0;port<2;port++) {
            Slot copy;
            pthread_mutex_lock(&state_lock);copy=slots[port];pthread_mutex_unlock(&state_lock);
            if(!copy.epoch || (copy.epoch==copy.done && round%40) || copy.epoch!=epochs[port].load())continue;
            char label[128]={0};if(!NUSB_READ_LABEL(copy.root,label))label[0]=0;
            bool publish=false;
            pthread_mutex_lock(&state_lock);
            if(slots[port].epoch==copy.epoch && copy.epoch==epochs[port].load()) {
                publish=slots[port].done!=copy.epoch || strcmp(slots[port].label,label);
                memcpy(slots[port].label,label,sizeof label);slots[port].done=copy.epoch;
            }
            pthread_mutex_unlock(&state_lock);
            if(publish) {
                api->log(MOD_ID,"native USB%u label snapshot epoch=%llu usable=%d",port+1,(unsigned long long)copy.epoch,!!label[0]);
                notified_epoch[port].store(0);
                pending_refresh[port].store(copy.epoch);
                api->post_ui(MOD_ID,refresh,(void *)(uintptr_t)port);
            }
        }
        usleep(50000);
    }
    return 0;
}
static NativeString name_impl(uintptr_t provider,uintptr_t selector,const unsigned char *media) {
    NativeString result=original_name(provider,selector,media);
    int port=nusb_native_port(media);
    if(!active.load() || port<0 || nusb_valid_name(result.utf8(),128,31))return result;
    /* Empty/invalid configured values must enter the outer native generic
     * fallback even when no filesystem snapshot can be acquired. */
    NativeString empty;
    ((void (*)(void *,void *))0x247d330)(&result,&empty);
    try {
        uintptr_t manager=*(uintptr_t *)(provider+8);
        if(!manager || *(uintptr_t *)manager!=0x2b7b538)return result;
        NativePath path=((NativePath (*)(uintptr_t,const unsigned char *))0x11199b8)(manager,media);
        const char *root=path.path.utf8();
        if(strncmp(root,"/media/usb/",11) || strnlen(root,256)>=256)return result;
        char label[128]={0};bool ready=false;
        uint64_t epoch=epochs[port].load();
        if(!pthread_mutex_trylock(&state_lock)) {
            Slot &slot=slots[port];
            if(slot.epoch!=epoch || memcmp(slot.media,media,5) || strcmp(slot.root,root)) {
                epoch=epochs[port].fetch_add(1)+1;
                memset(&slot,0,sizeof slot);slot.epoch=epoch;slot.manager=manager;
                memcpy(slot.media,media,8);strcpy(slot.root,root);
            }
            if(slot.done==epoch) {memcpy(label,slot.label,sizeof label);ready=true;}
            pthread_mutex_unlock(&state_lock);
        }
        if(epoch==epochs[port].load() && ready) {
            if(label[0]) {NativeString replacement(label);((void (*)(void *,void *))0x247d330)(&result,&replacement);}
            if(applied_epoch[port].exchange(epoch)!=epoch)api->log(MOD_ID,"native USB%u fallback resolved epoch=%llu label=%d",port+1,(unsigned long long)epoch,!!label[0]);
        } else if(!nusb_valid_name(result.utf8(),128,31)) {
            NativeString empty;
            ((void (*)(void *,void *))0x247d330)(&result,&empty);
        }
    } catch(...) { api->log(MOD_ID,"native name fallback unavailable; keeping original"); }
    return result;
}
static NativeString name_hook(uintptr_t provider,uintptr_t selector,const unsigned char *media) {
    try { return name_impl(provider,selector,media); }
    catch(...) { api->log(MOD_ID,"native name query failed; retaining generic fallback"); return NativeString(); }
}
static void mount_hook(uintptr_t self,const unsigned char *local,uintptr_t system,uintptr_t root,uintptr_t alternate,uintptr_t properties) {
    /* Invalidate synchronously before native metadata is announced. The worker
     * may never publish an old insertion's result after this epoch changes. */
    if(local && (local[0]==2 || local[0]==0x22))epochs[local[0]==2 ? 0:1].fetch_add(1);
    try { original_mount(self,local,system,root,alternate,properties); }
    catch(...) { api->log(MOD_ID,"native mount-completion forwarding failed"); }
}
/* This native task runs after the source information update on JUCE's message
 * thread. Information listeners repaint rows, but do not reload cached titles.
 * Bridge only our pending native USB title to the separate presentation observer. */
struct NativeInformationClosure {
    uintptr_t root;
    unsigned char source_id[12],media[8];
};
static_assert(offsetof(NativeInformationClosure,source_id)==8 && offsetof(NativeInformationClosure,media)==20,"Exact EP147 information-task closure ABI");
static void information_task_hook(const NativeInformationClosure *task) {
    try {
        original_information_task((uintptr_t)task);
        if(!active.load() || !task)return;
        const unsigned char *media=task->media;
        int port=nusb_native_port(media);if(port<0)return;
        uint64_t pending=pending_refresh[port].load();
        if(!pending || pending!=epochs[port].load())return;
        Slot copy;
        if(pthread_mutex_trylock(&state_lock)) {
            refresh_delay[port].store(1);notified_epoch[port].store(0);return;
        }
        copy=slots[port];pthread_mutex_unlock(&state_lock);
        uintptr_t root=task->root;
        const unsigned char *source_id=task->source_id;
        if(copy.epoch!=pending || copy.done!=pending || memcmp(copy.media,media,5) ||
           !root || *(uintptr_t *)root!=NUSB_ROOT_VTABLE ||
           *(uintptr_t *)(root+0x2b0)!=copy.manager)return;
        uint32_t source_kind=0;memcpy(&source_kind,source_id,4);
        if(source_kind!=1 || memcmp(source_id+4,media,5))return;
        int count=nusb_refresh_source_names(root,source_id);
        if(pending==epochs[port].load() && applied_epoch[port].load()==pending) {
            pending_refresh[port].compare_exchange_strong(pending,0);
            api->log(MOD_ID,"native USB%u Source title refreshed epoch=%llu listeners=%d",port+1,(unsigned long long)copy.epoch,count);
        } else {
            refresh_delay[port].store(30);notified_epoch[port].store(0);
        }
    } catch(...) {api->log(MOD_ID,"native information/title task unavailable");}
}
static int init(const az_core_v1 *core) {
    api=core;
    struct utsname u;
    if (!AZ_HAS(core,claim_hook) || !AZ_HAS(core,thread_start) || !AZ_HAS(core,post_ui) || uname(&u)) return -1;
#ifdef NUSB_EMULATOR
    if (strcmp(u.release,"6.6.138-dirty")) return -2;
#else
    if (strcmp(u.release,"4.4.194")) return -2;
#endif
    if (strcmp(core->fw_tag(),"ep147-9e9454e357389ece")) return -3;
    struct Instructions {uintptr_t address;uint32_t words[4];};
    static const Instructions expected[]={
        {0x1c1ff10,{0xa9ba7bfd,0x910003fd,0xa90153f3,0xaa0003f3}},
        {0x1c305c8,{0xa9be7bfd,0x910003fd,0xf9000bf3,0xd100a013}},
        {0x1c7fad8,{0xa9ba7bfd,0x910003fd,0xa90153f3,0xaa0203f3}},
        {0xbbaad8,{0xa9ac7bfd,0x910003fd,0x79400023,0xaa0503e1}},
        {0x11199b8,{0xa9bb7bfd,0x910003fd,0xa9025bf5,0xaa0103f6}},
        {0x111d538,{0xa9bc7bfd,0x910003fd,0xa9025bf5,0xaa0003f5}},
        {0x247c9b0,{0xd0008321,0x911d4021,0x91004021,0xf9000001}},
        {0x247c9d0,{0xf9400021,0xf9000001,0xd0008322,0x911d4042}},
        {0x2469aa0,{0xf9400000,0xb00083c1,0x911d4021,0xd1004000}},
        {0x24a1180,{0xa9be7bfd,0xaa0003e8,0x910003fd,0x910063e0}},
        {0x247d330,{0xf9400024,0xf9400003,0xf9000004,0xf9000023}}
    };
    for(const Instructions &site:expected)if(memcmp((const void *)site.address,site.words,sizeof site.words))return -4;
    epochs[0].store(1);epochs[1].store(1);
    if(core->thread_start(MOD_ID,"native-usb-names",worker,0))return -8;
    if (core->claim_hook(MOD_ID,0x1c7fad8,(void *)name_hook,(void **)&original_name)) return -5;
    if (core->claim_hook(MOD_ID,0xbbaad8,(void *)mount_hook,(void **)&original_mount)) return -6;
    if(core->claim_hook(MOD_ID,0x1c1ff10,(void *)information_task_hook,(void **)&original_information_task))return -9;
    active.store(true);
    core->log(MOD_ID,"native presentation naming initialized; filesystem access is worker-only");
    return 0;
}
static const char *const tags[]={"ep147-9e9454e357389ece",0};
static const az_mod_v1 mod={AZ_MOD_ABI,sizeof(az_mod_v1),MOD_ID,tags,init,naming_tick,0};
AZ_MOD_EXPORT const az_mod_v1 *az_mod_v1_entry(void) {return &mod;}
