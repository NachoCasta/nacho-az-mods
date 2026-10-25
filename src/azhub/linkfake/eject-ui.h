/* SPDX-License-Identifier: GPL-3.0-only */
/* Hub USB STOP selector and background-ejection feedback. */
#ifndef HUB_EJECT_UI_H
#define HUB_EJECT_UI_H
#include <sys/utsname.h>
#include <link.h>
#include "eject-gesture.h"
#include "eject-input-health.h"

#define EJECT_ID "usb-hub-support"
#define EJECT_MAX 8
#include "eject-ui-model.h"
#include "eject-presentation.h"
static pthread_mutex_t eject_lock=PTHREAD_MUTEX_INITIALIZER;
static struct eject_inventory eject_live, eject_shown;
static struct hub_stop_gesture eject_gestures[2];
static unsigned eject_pending_select, eject_pending_all;
static int eject_active, eject_port, eject_selected, eject_enabled;
static int eject_inventory_redraw;
static int eject_closing;
static uint64_t eject_close_deadline;
static _Atomic unsigned eject_keys_down;
static _Atomic int eject_capture, eject_pin_guard, eject_discard_encoder;
static _Atomic unsigned eject_raw_serial, eject_edit_serial;
static struct hub_input_health eject_input_health;
#define EJECT_PANEL_FDS 4096
static _Atomic unsigned eject_panel_fds[EJECT_PANEL_FDS/32];
static _Atomic unsigned eject_click_edges;
static _Atomic unsigned eject_encoder_turn_calls;
static unsigned eject_ui_turns;
static unsigned eject_click_origin;
static unsigned eject_seen_raw, eject_seen_edit;
static uint64_t eject_raw_seen_ms, eject_edit_seen_ms;
/* Both filters run on the same mixer bus thread, raw before editing. */
static unsigned char eject_physical_counter, eject_physical_keys;
static int eject_focus_owned;
static uintptr_t eject_core_text_lo,eject_core_text_hi;
static fn8 eject_native_select_row;
static fn8 eject_native_encoder_observer;
static az_surface *eject_surface;
static void (*eject_native_stop)(void *,uint64_t,uint64_t,unsigned char);
static char eject_roots[2][PATH_MAX];
static char eject_root_links[2][PATH_MAX];
#ifdef LF_SAFE_EJECT
static void safe_eject_scan(struct eject_inventory *out);
static void safe_eject_worker_step(void);
static void safe_eject_submit(const struct eject_item *item,int all_port);
static void safe_eject_tick(void);
static void safe_eject_capture_controller(void *button);
static int safe_eject_init(void);
static void safe_eject_status(const struct eject_inventory *inventory);
#endif

static uint64_t eject_clock_ms(void) {
    struct timespec t; clock_gettime(CLOCK_MONOTONIC,&t);
    return (uint64_t)t.tv_sec*1000+t.tv_nsec/1000000;
}
/* Observe the same existing read as the SDK, after its raw/edit filter chain.
 * Kernel 4.4.194 signals panel input on changes, so idle is not a lost heartbeat.
 * No extra device read, syscall, allocation or lock occurs on this hook path. */
static void eject_input_read_note(int fd,const void *buffer,ssize_t returned) {
    if (fd<0 || fd>=EJECT_PANEL_FDS ||
        !(__atomic_load_n(&eject_panel_fds[fd/32],__ATOMIC_ACQUIRE)&(1u<<(fd%32))) || returned<=0 ||
        !hub_panel_frame_valid(buffer,(size_t)returned)) return;
    hub_input_health_note(&eject_input_health,
        __atomic_load_n(&eject_raw_serial,__ATOMIC_RELAXED),
        __atomic_load_n(&eject_edit_serial,__ATOMIC_RELAXED));
}
static void eject_input_closed(int fd) {
    if (fd>=0 && fd<EJECT_PANEL_FDS)
        __atomic_fetch_and(&eject_panel_fds[fd/32],~(1u<<(fd%32)),__ATOMIC_ACQ_REL);
}
static int eject_panel_present(void) {
    for (unsigned i=0;i<EJECT_PANEL_FDS/32;i++)
        if (__atomic_load_n(&eject_panel_fds[i],__ATOMIC_ACQUIRE)) return 1;
    return 0;
}
static void eject_find_panel_fd(void) {
    unsigned found[EJECT_PANEL_FDS/32]={0};
    DIR *directory=opendir("/proc/self/fd");
    if (!directory) return;
    struct dirent *entry;
    while ((entry=readdir(directory))) {
        char *end; long fd=strtol(entry->d_name,&end,10);
        if (*end || fd<0 || fd>=EJECT_PANEL_FDS) continue;
        char path[64],target[256];
        snprintf(path,sizeof path,"/proc/self/fd/%ld",fd);
        ssize_t length=readlink(path,target,sizeof target-1);
        if (length<0) continue;
        target[length]=0;
        if (!strcmp(target,"/dev/subucom_spi3.0")) found[fd/32]|=1u<<(fd%32);
    }
    closedir(directory);
    /* The emulator opens this device twice. Observe every matching descriptor,
     * rather than guessing which open the native input path uses. */
    for (unsigned i=0;i<EJECT_PANEL_FDS/32;i++)
        __atomic_store_n(&eject_panel_fds[i],found[i],__ATOMIC_RELEASE);
}
static int eject_environment_allowed(void) {
    struct utsname system;
    if (uname(&system)) return 0;
    if (!strcmp(system.release,"6.6.138-dirty"))
        return !access("/dev/subucom_ctrl",F_OK);
#ifdef LF_EJECT_AZ
    /* subucom_ctrl is an emulator fixture, not a real-AZ requirement. */
    if (!strcmp(system.release,"4.4.194"))
        return !access("/dev/subucom_spi3.0",F_OK);
#endif
    return 0;
}
static int eject_under(const char *path,const char *root) {
    size_t n=strlen(root);
    return n && !strncmp(path,root,n) && (path[n]=='/' || !path[n]);
}
/* All filesystem observation stays on this worker. A physical disk is one row,
 * even if several partitions carry libraries. Explicit roots are emulator fixtures. */
static void eject_scan(struct eject_inventory *out) {
#ifdef LF_SAFE_EJECT
    safe_eject_scan(out);
    return;
#endif
    memset(out,0,sizeof *out);
    for (int port=0;port<2;port++) {
        char cls[PATH_MAX],value[32];
        if (snprintf(cls,sizeof cls,"%s/bDeviceClass",eject_roots[port])>=(int)sizeof cls) continue;
        FILE *f=fopen(cls,"re");
        if (f) { if (fgets(value,sizeof value,f) && strtoul(value,NULL,16)==9) out->hubs[port]=1; fclose(f); }
    }
    FILE *table=setmntent("/proc/mounts","re");
    if (!table) return;
    struct mntent entry,*m; char buffer[8192];
    while ((m=getmntent_r(table,&entry,buffer,sizeof buffer))) {
        if (strncmp(m->mnt_fsname,"/dev/sd",7) || strncmp(m->mnt_dir,"/media/usb/",11)) continue;
        const char *base=m->mnt_fsname+5;
        char node[PATH_MAX],resolved[PATH_MAX],disk[64],label[128];
        snprintf(node,sizeof node,"/sys/class/block/%s",base);
        if (!realpath(node,resolved) || !azhub_nested_usb_path(resolved)) continue;
        int port=-1;
        for (int p=0;p<2;p++) if (out->hubs[p] && eject_under(resolved,eject_roots[p])) port=p;
        if (port<0) continue;
        size_t n=strlen(base); while(n && isdigit((unsigned char)base[n-1])) n--;
        if (!n || n>=sizeof disk) continue;
        memcpy(disk,base,n); disk[n]=0;
        int duplicate=0;
        for (int i=0;i<out->count;i++) if (!strcmp(out->items[i].disk,disk)) duplicate=1;
        if (duplicate || out->count==EJECT_MAX) continue;
        if (!hub_device_label(node,m->mnt_fsname,label,sizeof label)) snprintf(label,sizeof label,"USB storage");
        struct eject_item *item=&out->items[out->count++];
        snprintf(item->label,sizeof item->label,"%s",label);
        snprintf(item->disk,sizeof item->disk,"%s",disk);
        snprintf(item->topology,sizeof item->topology,"%s",resolved);
        const char *child=resolved+strlen(eject_roots[port])+1;
        const char *dot=strchr(child,'.');
        if (dot) snprintf(item->location,sizeof item->location,"%.*s",
            (int)strspn(dot+1,"0123456789."),dot+1);
        item->port=port;
    }
    endmntent(table);
}
static void *eject_inventory_worker(void *unused) {
    (void)unused;
    for (;;) {
#ifdef LF_COMMUNITY
        int state=__atomic_load_n(&community_ready,__ATOMIC_ACQUIRE);
        if (state<0) return NULL;
        if (!state) { usleep(250000); continue; }
#endif
        eject_find_panel_fd();
        for (int p=0;p<2;p++) {
            if (!*eject_root_links[p] || !realpath(eject_root_links[p],eject_roots[p])) *eject_roots[p]=0;
        }
#ifdef LF_SAFE_EJECT
        safe_eject_worker_step();
#endif
        struct eject_inventory next; eject_scan(&next);
        hub_eject_sort(&next);
        pthread_mutex_lock(&eject_lock);
        eject_live=next;
        if (eject_active) {
            eject_selected=hub_eject_refresh(&eject_shown,&eject_live,eject_port,eject_selected);
            eject_inventory_redraw=1;
        }
        pthread_mutex_unlock(&eject_lock);
#ifdef LF_SAFE_EJECT
        safe_eject_status(&next);
#endif
        usleep(250000);
    }
    return NULL;
}
static void eject_queue(enum hub_stop_action action,int port) {
    if (action==HUB_STOP_SELECT) eject_pending_select|=1u<<port;
    if (action==HUB_STOP_ALL) eject_pending_all|=1u<<port;
}
static void eject_stop_hook(void *self,uint64_t a,uint64_t b,unsigned char down) {
    if (!lf_runtime_ready()) { eject_native_stop(self,a,b,down); return; }
    unsigned char id=*((unsigned char *)self+0x18);
    int port=id>>5;
    if ((id&31)!=2 || port>1) { eject_native_stop(self,a,b,down); return; }
#ifdef LF_SAFE_EJECT
    safe_eject_capture_controller(self);
#endif
    pthread_mutex_lock(&eject_lock);
    enum hub_stop_action action=hub_stop_event(&eject_gestures[port],down!=0,eject_live.hubs[port],eject_clock_ms());
    eject_queue(action,port);
    pthread_mutex_unlock(&eject_lock);
    if (action==HUB_STOP_FORWARD) {
        lf_core->log(EJECT_ID,"eject-ui native-forward port=%d down=%d",port+1,!!down);
        eject_native_stop(self,a,b,down);
    } else lf_core->log(EJECT_ID,"eject-ui captured port=%d down=%d action=%d",port+1,!!down,action);
}
#include "eject-view.h"
static int eject_raw_keys(int bus,unsigned char *frame,int length,void *user) {
    (void)user;
    static unsigned serial,previous_keys;
    if (bus!=AZ_BUS_MIXER_IN || length<=34) return AZ_FRAME_OK;
    unsigned keys=(frame[31]&1) | (frame[33]&0x80);
    eject_physical_counter=frame[34]; eject_physical_keys=keys;
    if ((keys&1) && !(previous_keys&1)) __atomic_fetch_add(&eject_click_edges,1,__ATOMIC_RELAXED);
    if (keys!=previous_keys) __atomic_store_n(&eject_keys_down,keys,__ATOMIC_RELAXED);
    previous_keys=keys;
    __atomic_store_n(&eject_raw_serial,++serial,__ATOMIC_RELAXED);
    return AZ_FRAME_OK;
}
static int eject_filter_keys(int bus,unsigned char *frame,int length,void *user) {
    (void)user;
    static unsigned held,serial;
    if (bus!=AZ_BUS_MIXER_IN || length<=34) return AZ_FRAME_OK;
    unsigned keys=eject_physical_keys;
    int capture=__atomic_load_n(&eject_capture,__ATOMIC_RELAXED);
    if (capture) {
        held|=keys;
        /* SDK focus owns the core's input, but its raw-counter freeze would
         * create a jump on release. Keep the physical byte unchanged; the
         * semantic encoder hook suppresses native turns during ownership. */
        frame[34]=eject_physical_counter;
    }
    held&=keys;
    unsigned suppress=capture ? 0x81:held;
    frame[31]&=~(suppress&1); frame[33]&=~(suppress&0x80);
    __atomic_store_n(&eject_edit_serial,++serial,__ATOMIC_RELAXED);
    return AZ_FRAME_OK;
}
static uint64_t eject_encoder_observer_hook(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    if (!lf_runtime_ready()) return eject_native_encoder_observer(a,b,c,d,e,f,g,h);
    /* Firmware 2.00 browse encoder EasyID, observed independently of its optional
     * accelerator. The input decoder has already remembered the physical byte.
     * Copy the notification so only this observer loses the captured turn;
     * preserve push state and never alter another encoder's notification. */
    if (c && *(const uint64_t *)(uintptr_t)(a+0xd0)==UINT64_C(0x01d690928446227a) &&
        (eject_capture || __atomic_exchange_n(&eject_discard_encoder,0,__ATOMIC_RELAXED))) {
        int payload[3]; memcpy(payload,(const void *)(uintptr_t)c,sizeof payload);
        if (payload[0]) __atomic_fetch_add(&eject_encoder_turn_calls,1,__ATOMIC_RELAXED);
        payload[0]=0;
        return eject_native_encoder_observer(a,b,(uintptr_t)payload,d,e,f,g,h);
    }
    return eject_native_encoder_observer(a,b,c,d,e,f,g,h);
}
static uint64_t eject_select_row_hook(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    uintptr_t caller=(uintptr_t)__builtin_return_address(0);
    if (eject_pin_guard && caller>=eject_core_text_lo && caller<eject_core_text_hi) {
        uintptr_t model=*(uintptr_t *)(a+0x140);
        if (model && *(uintptr_t *)model==0x2e0b900) {
            unsigned count=*(unsigned *)(model+0x18),row=(unsigned)b;
            uintptr_t rows=*(uintptr_t *)(model+8);
            if (rows && count<=64 && row<count) {
                const unsigned char *item=(const void *)(rows+row*0x28);
                const char *name=*(const char *const *)(item+0x10);
                if (!*(const unsigned *)(item+4) && !*(const unsigned *)(item+8) &&
                    name && !strcmp(name,"MOD MENU")) {
                    /* Core 1.2.3 pins SOURCE to its menu on any SDK-focus
                     * release. Suppress that synthetic selection once. Native
                     * user selections and other list models pass unchanged. */
                    if (!eject_capture) eject_pin_guard=0;
                    lf_core->log(EJECT_ID,"eject-ui SOURCE focus pin suppressed");
                    return 0;
                }
            }
        }
    }
    return eject_native_select_row(a,b,c,d,e,f,g,h);
}
static void eject_open(int port) {
    if (lf_core->view()==AZ_VIEW_MENU) {
        lf_core->banner(EJECT_ID,"Close the current menu before selecting a hub USB.",2500);
        lf_core->log(EJECT_ID,"eject-ui focus-refused port=%d",port+1); return;
    }
    uint64_t now=eject_clock_ms();
    if (!hub_input_health_ready(&eject_input_health) || !eject_panel_present()) {
        lf_core->banner(EJECT_ID,"Hub selector input is unavailable. Restart the mod before retrying.",4000);
        lf_core->log(EJECT_ID,"eject-ui input-unavailable port=%d fault=%u panel=%d raw=%u edited=%u raw-age=%llu edit-age=%llu",
            port+1,hub_input_health_fault(&eject_input_health),eject_panel_present(),eject_seen_raw,eject_seen_edit,
            (unsigned long long)(now-eject_raw_seen_ms),
            (unsigned long long)(now-eject_edit_seen_ms)); return;
    }
    pthread_mutex_lock(&eject_lock);
    hub_eject_refresh(&eject_shown,&eject_live,port,-1);
    eject_port=port; eject_selected=0; eject_active=1;
    int count=eject_shown.count;
    pthread_mutex_unlock(&eject_lock);
    eject_click_origin=__atomic_load_n(&eject_click_edges,__ATOMIC_RELAXED);
    eject_ui_turns=0;
    __atomic_store_n(&eject_encoder_turn_calls,0,__ATOMIC_RELAXED);
    lf_core->surface_place(eject_surface,(az_rect){280,160,720,500});
    lf_core->surface_show(eject_surface,1);
    eject_closing=0; eject_capture=1; eject_pin_guard=1;
    if (!lf_core->surface_focus(eject_surface,1)) {
        eject_keys_down=0; eject_close();
        lf_core->banner(EJECT_ID,"Finish the current control dialog before selecting a hub USB.",3000);
        return;
    }
    eject_focus_owned=1;
    lf_core->surface_redraw(eject_surface);
    lf_core->log(EJECT_ID,"eject-ui selector port=%d count=%d",port+1,count);
}
static void eject_tick(void) {
    if (!eject_enabled) return;
    uint64_t now=eject_clock_ms();
    __atomic_store_n(&eject_input_health.armed,1,__ATOMIC_RELEASE);
    unsigned raw=__atomic_load_n(&eject_raw_serial,__ATOMIC_RELAXED);
    unsigned edited=__atomic_load_n(&eject_edit_serial,__ATOMIC_RELAXED);
    if (raw!=eject_seen_raw) { eject_seen_raw=raw; eject_raw_seen_ms=now; }
    if (edited!=eject_seen_edit) { eject_seen_edit=edited; eject_edit_seen_ms=now; }
    if (eject_capture && (!hub_input_health_ready(&eject_input_health) || !eject_panel_present())) {
        eject_keys_down=0; eject_discard_encoder=1; eject_close();
        lf_core->banner(EJECT_ID,"Hub selector input stopped. Restart the mod before retrying.",4000);
        lf_core->log(EJECT_ID,"eject-ui input observer stopped; ownership released");
    }
    if (eject_closing && (!eject_keys_down || eject_clock_ms()>=eject_close_deadline)) {
        /* A disabled/stalled raw observer must not retain the knob forever.
         * The editing filter separately swallows an already-captured key until
         * its real release, even after the bounded capture window ends. */
        eject_keys_down=0; eject_close();
    }
#ifdef LF_SAFE_EJECT
    safe_eject_tick();
#endif
    pthread_mutex_lock(&eject_lock);
    int redraw=eject_inventory_redraw && eject_active;
    eject_inventory_redraw=0;
    for (int p=0;p<2;p++) eject_queue(hub_stop_poll(&eject_gestures[p],eject_clock_ms()),p);
    unsigned select=eject_pending_select,all=eject_pending_all;
    eject_pending_select=eject_pending_all=0;
    pthread_mutex_unlock(&eject_lock);
    if (redraw) lf_core->surface_redraw(eject_surface);
    for (int p=0;p<2;p++) {
        if (all&(1u<<p)) {
            eject_close();
#ifdef LF_SAFE_EJECT
            safe_eject_submit(NULL,p);
#else
            lf_core->log(EJECT_ID,"eject-ui request-all port=%d (no unmount)",p+1);
            lf_core->banner(EJECT_ID,"Hub eject request recorded. Prototype: USBs remain mounted.",3000);
#endif
        } else if (select&(1u<<p)) eject_open(p);
    }
}
static int eject_init(void) {
#ifndef LF_EJECT_AZ
    if (!getenv("AZHUB_EJECT_UI_PROTOTYPE")) return 0;
#endif
    if (!eject_environment_allowed()) return -60;
    if (!AZ_HAS(lf_core,claim_hook) || !AZ_HAS(lf_core,surface_create) ||
        !AZ_HAS(lf_core,frame_filter) || !AZ_HAS(lf_core,surface_focus) ||
        !AZ_HAS(lf_core,color) || !AZ_HAS(lf_core,fill_rect) || !AZ_HAS(lf_core,draw_rect) ||
        !AZ_HAS(lf_core,draw_text) || !AZ_HAS(lf_core,text_width) ||
        !AZ_HAS(lf_core,view) || !AZ_HAS(lf_core,surface_place) ||
        !AZ_HAS(lf_core,surface_show) || !AZ_HAS(lf_core,surface_redraw) || !AZ_HAS(lf_core,banner)) return -61;
    for (int p=0;p<2;p++) {
        const char *root=getenv(p ? "AZHUB_EJECT_PORT2_ROOT":"AZHUB_EJECT_PORT1_ROOT");
        if (root) {
            if (!realpath(root,eject_roots[p]) || strncmp(eject_roots[p],"/sys/devices/",13)) return -62;
            snprintf(eject_root_links[p],sizeof eject_root_links[p],"%s",root);
        }
#ifdef LF_EJECT_AZ
        else snprintf(eject_root_links[p],sizeof eject_root_links[p],"/sys/bus/usb/devices/%d-1",p+1);
#endif
    }
    if (!*eject_root_links[0] && !*eject_root_links[1]) return -63;
    if (*(uintptr_t *)0x31bbd78!=0x21aeb50) return -64;
    if (*(uint32_t *)0x2360f80!=0xd0002cc3 || *(uintptr_t *)0x3216948!=0x2360f80) return -76;
    if (*(uint32_t *)0x2355270!=0xa9b87bfd) return -78;
    Dl_info core_info={0};
    if (!dladdr((void *)lf_core->surface_focus,&core_info) || !core_info.dli_fbase) return -79;
    const ElfW(Ehdr) *elf=core_info.dli_fbase;
    if (memcmp(elf->e_ident,ELFMAG,SELFMAG) || elf->e_phnum>64 || elf->e_phoff>0x10000) return -79;
    const ElfW(Phdr) *headers=(const void *)((uintptr_t)core_info.dli_fbase+elf->e_phoff);
    for (unsigned i=0;i<elf->e_phnum;i++) if (headers[i].p_type==PT_LOAD && (headers[i].p_flags&PF_X)) {
        uintptr_t lo=(uintptr_t)core_info.dli_fbase+headers[i].p_vaddr,hi=lo+headers[i].p_memsz;
        if (!eject_core_text_lo || lo<eject_core_text_lo) eject_core_text_lo=lo;
        if (hi>eject_core_text_hi) eject_core_text_hi=hi;
    }
    if (!eject_core_text_lo || eject_core_text_hi<=eject_core_text_lo) return -79;
    az_surface_v1 surface={.size=sizeof surface,.layer=AZ_LAYER_TOP,.views=0x7f,
        .on_paint=eject_paint,.on_input=eject_input};
    eject_surface=lf_core->surface_create(EJECT_ID,&surface);
    if (!eject_surface) return -65;
    if (lf_core->frame_filter(EJECT_ID,AZ_FILTER_RAW | (1u<<AZ_BUS_MIXER_IN),eject_raw_keys,NULL)) return -68;
    if (lf_core->frame_filter(EJECT_ID,1u<<AZ_BUS_MIXER_IN,eject_filter_keys,NULL)) return -69;
    lf_core->surface_place(eject_surface,(az_rect){280,160,720,500});
    if (lf_core->thread_start(EJECT_ID,"HubEjectUI",eject_inventory_worker,NULL)) return -66;
    return 0;
}
static int eject_install_hooks(void) {
#ifdef LF_SAFE_EJECT
    int safe_result=safe_eject_init();
    if (safe_result) return safe_result;
#endif
    /* This SDK core rejects the read-only vtable range for claim_vslot. Claim
     * the semantic dispatch function through its supported code-hook API. */
    if (lf_core->claim_hook(EJECT_ID,0x21aeb50,eject_stop_hook,(void **)&eject_native_stop)) return -67;
    if (lf_core->claim_hook(EJECT_ID,0x2360f80,eject_encoder_observer_hook,(void **)&eject_native_encoder_observer)) return -77;
    if (lf_core->claim_hook(EJECT_ID,0x2355270,eject_select_row_hook,(void **)&eject_native_select_row)) return -80;
    eject_enabled=1;
    lf_core->log(EJECT_ID,"eject-ui active; hold=%u ms; mode=%s",HUB_STOP_HOLD_MS,
#ifdef LF_SAFE_EJECT
        "safe-eject"
#else
        "requests-only"
#endif
    );
    return 0;
}
#endif
