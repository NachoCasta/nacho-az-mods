/* SPDX-License-Identifier: GPL-3.0-only */
/* Background hub-ejection coordinator for the exact supported firmware. */
#ifndef HUB_SAFE_EJECT_H
#define HUB_SAFE_EJECT_H
#include <fcntl.h>
#include <sys/sysmacros.h>
#include "eject-source.h"

#define SAFE_RECORDS 16
#define SAFE_MOUNTS 128
#define SAFE_JOURNAL "/run/azhub-eject-state"
struct safe_record { struct eject_item item; unsigned sources; };
struct safe_request { struct eject_item items[EJECT_MAX]; int count,all_port,pending; };
static pthread_mutex_t safe_lock=PTHREAD_MUTEX_INITIALIZER;
static struct safe_record safe_records[SAFE_RECORDS];
static struct safe_request safe_request;
static char safe_feedback[256];
static _Atomic uintptr_t safe_controller;
static int safe_running;
static void safe_message(const char *format,...);
struct safe_load {
    uintptr_t controller,request;
    int source,loading,deck,failed;
    uint32_t content_id;
};
static struct safe_load safe_loads[16];
static void (*safe_original_load)(uintptr_t);
static fn8 safe_original_load_complete,safe_original_unload_complete;
static void (*safe_original_load_common)(uintptr_t,uintptr_t,uintptr_t);
static fn8 safe_original_track_execute,safe_original_track_execute_range;
static int safe_source_identity(uint64_t identity);
static int safe_track_request_denied(uintptr_t track) {
    if (!track) return 0;
    int source=safe_source_identity(*(uint64_t *)track);
    pthread_mutex_lock(&safe_lock);
    int denied=source>=0 && fakes[source].eject_reserved;
    pthread_mutex_unlock(&safe_lock);
    if (denied) {
        lf_core->log(EJECT_ID,"safe-eject load-denied stage=track-loader source=%d",source);
        safe_message("Cannot load from a USB that is being ejected.");
    }
    return denied;
}
static uint64_t safe_track_execute_hook(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    if (!lf_runtime_ready()) return safe_original_track_execute(a,b,c,d,e,f,g,h);
    if (safe_track_request_denied(b)) return 0;
    return safe_original_track_execute(a,b,c,d,e,f,g,h);
}
static uint64_t safe_track_execute_range_hook(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    if (!lf_runtime_ready()) return safe_original_track_execute_range(a,b,c,d,e,f,g,h);
    if (safe_track_request_denied(d)) return 0;
    return safe_original_track_execute_range(a,b,c,d,e,f,g,h);
}
static void safe_load_common_hook(uintptr_t controller,uintptr_t track,uintptr_t options) {
    if (!lf_runtime_ready()) { safe_original_load_common(controller,track,options); return; }
    if (!controller || !track) { safe_original_load_common(controller,track,options); return; }
    int source=safe_source_identity(*(uint64_t *)track);
    pthread_mutex_lock(&safe_lock);
    int denied=source>=0 && fakes[source].eject_reserved;
    pthread_mutex_unlock(&safe_lock);
    if (denied) {
        /* Reject before native queuing, metadata changes or library prefetch.
         * Requests already queued still face the final doLoadTrack boundary. */
        lf_core->log(EJECT_ID,"safe-eject load-denied stage=request source=%d deck=%d",source,*(int *)(controller+0x1e8)+1);
        safe_message("Cannot load from a USB that is being ejected.");
        return;
    }
    if (getenv("AZHUB_EJECT_TRACE_LOAD")) lf_core->log(EJECT_ID,"safe-eject load-request deck=%d location=%#lx:%#lx time=%llu",
                 *(int *)(controller+0x1e8)+1,*(uint64_t *)track,*(uint64_t *)(track+8),
                 (unsigned long long)eject_clock_ms());
    safe_original_load_common(controller,track,options);
}

/* Serialize request admission with reservation, without holding a lock across
 * firmware calls. Drain admitted queue submissions before enqueueing unmount. */
static int safe_server_begin(int source) {
    if (source<0 || source>=nfakes) return 0;
    pthread_mutex_lock(&safe_lock);
    int allowed=!fakes[source].eject_reserved;
    if (allowed) fakes[source].eject_dispatching++;
    pthread_mutex_unlock(&safe_lock);
    return allowed;
}
static void safe_server_end(int source) { fakes[source].eject_dispatching--; }

static int safe_source_identity(uint64_t identity) {
    identity=hub_eject_media_key(identity);
    for (int s=0;s<nfakes;s++) {
        uint64_t expected=((uint64_t)(0x100|media_sdid(fakes[s].slot))<<32)|((fakes[s].player-1)<<8)|2;
        if (identity==hub_eject_media_key(expected)) return s;
    }
    return -1;
}
/* Called under safe_lock. The slot remains for the controller's process lifetime. */
static struct safe_load *safe_load_slot(uintptr_t controller) {
    for (int i=0;i<16;i++) if (safe_loads[i].controller==controller) return &safe_loads[i];
    for (int i=0;i<16;i++) if (!safe_loads[i].controller) {
        safe_loads[i]=(struct safe_load){.controller=controller,.source=-1,.deck=-1}; return &safe_loads[i];
    }
    return NULL;
}
/* Last native admission boundary, on the LoadController's own task thread.
 * Track identity is read through its native record lookup, not title/content ID.
 * A denied request follows native failure notification and cleanup. */
static void safe_load_hook(uintptr_t controller) {
    if (!lf_runtime_ready()) { safe_original_load(controller); return; }
    if (!controller) { safe_original_load(controller); return; }
    uintptr_t pending=controller+0x9e8;
    if (!*(uintptr_t *)pending || !**(char **)(controller+0x208)) { safe_original_load(controller); return; }
    uintptr_t record=((uintptr_t (*)(uintptr_t,uintptr_t))0xa5d968)(controller+0x298,pending);
    int source=safe_source_identity(*(uint64_t *)(record+0x38)),denied=0;
    pthread_mutex_lock(&safe_lock);
    struct safe_load *load=safe_load_slot(controller);
    /* Exact-firmware constructor stores its zero-based deck argument at +0x1e8.
     * Content ID only correlates stale metadata after a proven failed load;
     * physical ownership always comes from the full media identity above. */
    int deck=*(int *)(controller+0x1e8);
    if (load) {
        load->deck=deck>=0 && deck<4 ? deck:-1;
        load->failed=0; load->content_id=*(uint32_t *)(record+0x40);
        load->request=record;
        load->source=source; load->loading=1;
    }
    if (source>=0) {
        denied=!load || fakes[source].eject_reserved;
    }
    pthread_mutex_unlock(&safe_lock);
    if (!denied) {
        if (source>=0) lf_core->log(EJECT_ID,"safe-eject load-admitted source=%d controller=%#lx deck=%d",source,controller,deck+1);
        safe_original_load(controller); return;
    }
    lf_core->log(EJECT_ID,"safe-eject load-denied source=%d",source);
    safe_message("Cannot load from a USB that is being ejected.");
    /* Native DownloadResult default: empty strings/vector, invalid optional IDs. */
    unsigned char result[0x40]={0}; uint16_t invalid=0xffff; int32_t missing=-1;
    memcpy(result+0x10,&invalid,sizeof invalid); memcpy(result+0x30,&missing,sizeof missing);
    ((void (*)(uintptr_t,unsigned,void *))0xa737a0)(controller,9,result);
    ((void (*)(uintptr_t,uintptr_t,unsigned,uintptr_t))0xa7b148)(controller,pending,1,pending);
}
/* Completion lambda's entry guard precedes its prologue. Hook the prologue
 * after that guard; x1 still carries the controller. Forward all argument
 * registers so the displaced native body retains that live register value. */
static uint64_t safe_load_complete_hook(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    if (!lf_runtime_ready()) return safe_original_load_complete(a,b,c,d,e,f,g,h);
    uintptr_t root=*(uintptr_t *)a; unsigned result=*(unsigned *)(a+0x10);
    uintptr_t completed=*(uintptr_t *)(a+8);
    /* The payload owns a copied WherefromPtr. Compare its native map entry,
     * rather than smart-pointer storage or the controller's already-cleared
     * pending field, before changing the current request's ledger. */
    uintptr_t record=completed ? ((uintptr_t (*)(uintptr_t,uintptr_t))0xa5d968)(root+0x298,a+8):0;
    pthread_mutex_lock(&safe_lock);
    struct safe_load *load=safe_load_slot(root);
    int matched=load && record && record==load->request;
    if (matched) {
        load->loading=0; load->failed=result!=0 && result!=4;
        if (load->failed) load->source=-1;
    }
    pthread_mutex_unlock(&safe_lock);
    if (!matched)
        lf_core->log(EJECT_ID,"safe-eject completion-unmatched controller=%#lx",root);
    uint64_t returned=safe_original_load_complete(a,b,c,d,e,f,g,h);
    lf_core->log(EJECT_ID,"safe-eject load-completed controller=%#lx result=%u",root,result);
    return returned;
}
static uint64_t safe_unload_complete_hook(uint64_t a,uint64_t b,uint64_t c,uint64_t d,uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    if (!lf_runtime_ready()) return safe_original_unload_complete(a,b,c,d,e,f,g,h);
    uintptr_t root=*(uintptr_t *)a;
    pthread_mutex_lock(&safe_lock);
    struct safe_load *load=safe_load_slot(root);
    /* Media withdrawal can complete an unload without clearing the SDK's
     * already-failed track metadata. Keep that proof until the next load. */
    if (load) { load->source=-1; load->loading=0; }
    pthread_mutex_unlock(&safe_lock);
    uint64_t returned=safe_original_unload_complete(a,b,c,d,e,f,g,h);
    lf_core->log(EJECT_ID,"safe-eject unload-completed controller=%#lx",root);
    return returned;
}

/* Resolve physical USB identity, including all partitions and LUNs. devnum plus
 * sysfs inode identifies one connection; neither sdX nor a label is an identity. */
static int safe_usb_identity(const char *block,struct eject_item *item) {
    char path[PATH_MAX],resolved[PATH_MAX],attribute[PATH_MAX];
    const char *name=strrchr(block,'/'); name=name ? name+1:block;
    if (snprintf(path,sizeof path,"/sys/class/block/%s",name)>=(int)sizeof path || !realpath(path,resolved)) return 0;
    snprintf(item->topology,sizeof item->topology,"%s",resolved);
    for (;;) {
        if (snprintf(attribute,sizeof attribute,"%s/bDeviceClass",resolved)>=(int)sizeof attribute) return 0;
        if (!access(attribute,R_OK)) break;
        char *slash=strrchr(resolved,'/'); if (!slash || slash==resolved) return 0; *slash=0;
    }
    struct stat st; if (stat(resolved,&st)) return 0;
    if (snprintf(attribute,sizeof attribute,"%s/devnum",resolved)>=(int)sizeof attribute) return 0;
    FILE *file=fopen(attribute,"re"); if (!file) return 0;
    unsigned number=0; int ok=fscanf(file,"%u",&number)==1; fclose(file); if (!ok) return 0;
    snprintf(item->usb,sizeof item->usb,"%s",resolved);
    item->generation=(unsigned long long)st.st_ino; item->devnum=number;
    return 1;
}
static int safe_same(const struct eject_item *a,const struct eject_item *b) {
    return a->generation==b->generation && a->devnum==b->devnum && !strcmp(a->usb,b->usb);
}
static int safe_connected(const struct eject_item *item) {
    struct stat st; char path[PATH_MAX]; unsigned number=0;
    if (stat(item->usb,&st) || (unsigned long long)st.st_ino!=item->generation) return 0;
    if (snprintf(path,sizeof path,"%s/devnum",item->usb)>=(int)sizeof path) return 0;
    FILE *file=fopen(path,"re"); if (!file) return 0;
    int ok=fscanf(file,"%u",&number)==1; fclose(file);
    return ok && number==item->devnum;
}
static void safe_message(const char *format,...) {
    va_list args; va_start(args,format);
    pthread_mutex_lock(&safe_lock); vsnprintf(safe_feedback,sizeof safe_feedback,format,args); pthread_mutex_unlock(&safe_lock);
    va_end(args);
}
static int safe_eject_blocked_device(const char *device) {
    struct eject_item item={0}; if (!safe_usb_identity(device,&item)) return 0;
    int blocked=0;
    pthread_mutex_lock(&safe_lock);
    for (int i=0;i<SAFE_RECORDS;i++) if (safe_records[i].item.state && safe_same(&safe_records[i].item,&item)) blocked=1;
    pthread_mutex_unlock(&safe_lock); return blocked;
}
static int safe_eject_blocked_path(const char *path) {
    int blocked=0; FILE *table=setmntent("/proc/mounts","re"); if (!table) return 1;
    struct mntent entry,*m; char buffer[8192];
    while ((m=getmntent_r(table,&entry,buffer,sizeof buffer))) {
        if (strncmp(m->mnt_fsname,"/dev/sd",7) || !eject_under(path,m->mnt_dir)) continue;
        if (safe_eject_blocked_device(m->mnt_fsname)) { blocked=1; break; }
    }
    endmntent(table); return blocked;
}
/* Only the inventory/eject worker mutates records and calls this function.
 * Never hold safe_lock across journal I/O: UI and load-admission callbacks use
 * that lock. Persist inhibition before teardown. /run survives an EP147 reload,
 * but not a guest reboot; disconnected generations are ignored. */
static int safe_persist(void) {
    FILE *file=fopen(SAFE_JOURNAL ".new","we"); if (!file) return -1;
    int bad=0;
    for (int i=0;i<SAFE_RECORDS;i++) {
        const struct eject_item *item=&safe_records[i].item;
        if (item->state) {
            if (fprintf(file,"%llu %u %d %s %d ",item->generation,item->devnum,item->port,item->usb,item->state)<0) bad=1;
            if (!*item->label) fputc('-',file);
            for (const unsigned char *p=(const unsigned char *)item->label;*p;p++) fprintf(file,"%02x",*p);
            fputc('\n',file);
        }
    }
    if (ferror(file) || fflush(file) || fsync(fileno(file))) bad=1;
    if (fclose(file)) bad=1;
    if (bad || rename(SAFE_JOURNAL ".new",SAFE_JOURNAL)) return -1;
    return 0;
}
/* A safe result is invalid once another mount owner reopens this connection.
 * Observe outside safe_lock; retain inhibition so retry performs real teardown. */
static int safe_has_mounts(const struct eject_item *item) {
    FILE *table=setmntent("/proc/mounts","re"); if (!table) return -1;
    struct mntent entry,*m; char buffer[8192]; int mounted=0;
    while ((m=getmntent_r(table,&entry,buffer,sizeof buffer))) {
        if (strncmp(m->mnt_fsname,"/dev/sd",7)) continue;
        struct eject_item device={0};
        if (safe_usb_identity(m->mnt_fsname,&device) && safe_same(item,&device)) { mounted=1; break; }
    }
    if (ferror(table)) mounted=-1;
    endmntent(table); return mounted;
}
static void safe_revalidate_result(const struct eject_item *item) {
    if (safe_has_mounts(item)==0) return;
    int changed=0;
    pthread_mutex_lock(&safe_lock);
    for (int i=0;i<SAFE_RECORDS;i++) if (safe_records[i].item.state==2 && safe_same(&safe_records[i].item,item)) {
        safe_records[i].item.state=3; changed=1;
    }
    pthread_mutex_unlock(&safe_lock);
    if (changed) safe_persist();
    if (changed) safe_message("USB mounts changed. Leave it connected and retry ejection.");
}
static void safe_reconcile(void) {
    struct eject_item completed[SAFE_RECORDS]; int count=0;
    pthread_mutex_lock(&safe_lock);
    for (int i=0;i<SAFE_RECORDS;i++) if (safe_records[i].item.state==2) completed[count++]=safe_records[i].item;
    pthread_mutex_unlock(&safe_lock);
    for (int i=0;i<count;i++) safe_revalidate_result(&completed[i]);
    int disconnected[SAFE_RECORDS]={0};
    for (int i=0;i<SAFE_RECORDS;i++)
        if (safe_records[i].item.state) disconnected[i]=!safe_connected(&safe_records[i].item);
    pthread_mutex_lock(&safe_source_mutex);
    pthread_mutex_lock(&safe_lock);
    int changed=0;
    for (int i=0;i<SAFE_RECORDS;i++) if (safe_records[i].item.state && disconnected[i]) {
        int pending=0;
        for (int s=0;s<nfakes;s++) if ((safe_records[i].sources&(1u<<s)) && !fakes[s].eject_done) pending=1;
        if (pending) continue;
        for (int s=0;s<nfakes;s++) if (safe_records[i].sources&(1u<<s)) {
            if (fakes[s].eject_done) {
                fakes[s].eject_done=0; fakes[s].eject_reserved=0;
            }
        }
        memset(&safe_records[i],0,sizeof safe_records[i]); changed=1;
    }
    pthread_mutex_unlock(&safe_lock);
    pthread_mutex_unlock(&safe_source_mutex);
    if (changed) safe_persist();
}
static void safe_eject_scan(struct eject_inventory *out) {
    memset(out,0,sizeof *out);
    for (int p=0;p<2;p++) {
        char attr[PATH_MAX]; unsigned cls=0;
        if (!*eject_roots[p] || snprintf(attr,sizeof attr,"%s/bDeviceClass",eject_roots[p])>=(int)sizeof attr) continue;
        FILE *f=fopen(attr,"re"); if (f) { if(fscanf(f,"%x",&cls)==1 && cls==9) out->hubs[p]=1; fclose(f); }
    }
    DIR *blocks=opendir("/sys/class/block"); if (!blocks) { out->overflow=3; return; }
    struct dirent *entry;
    while ((entry=readdir(blocks))) {
        if (strncmp(entry->d_name,"sd",2)) continue;
        char node[PATH_MAX],attribute[PATH_MAX];
        snprintf(node,sizeof node,"/sys/class/block/%s",entry->d_name);
        snprintf(attribute,sizeof attribute,"%s/partition",node);
        if (!access(attribute,F_OK)) continue;
        struct eject_item item={0}; if (!safe_usb_identity(entry->d_name,&item)) continue;
        int port=-1;
        for (int p=0;p<2;p++) if (out->hubs[p] && eject_under(item.usb,eject_roots[p]) && strcmp(item.usb,eject_roots[p])) port=p;
        if (port<0) continue;
        int duplicate=0;
        for (int i=0;i<out->count;i++) if (safe_same(&out->items[i],&item)) duplicate=1;
        if (duplicate) continue;
        if (out->count==EJECT_MAX) { out->overflow|=1u<<port; continue; }
        item.port=port; snprintf(item.disk,sizeof item.disk,"%.*s",(int)sizeof item.disk-1,entry->d_name);
        const char *child=item.usb+strlen(eject_roots[port])+1,*dot=strchr(child,'.');
        if (dot) snprintf(item.location,sizeof item.location,"%.*s",(int)strspn(dot+1,"0123456789."),dot+1);
        snprintf(item.label,sizeof item.label,"USB on hub port %s",item.location);
        FILE *table=setmntent("/proc/mounts","re"); struct mntent m,*mp; char buffer[8192];
        if (table) {
            while ((mp=getmntent_r(table,&m,buffer,sizeof buffer))) {
                if (strncmp(mp->mnt_fsname,"/dev/sd",7) || strncmp(mp->mnt_dir,"/media/usb/",11)) continue;
                struct eject_item partition={0};
                if (!safe_usb_identity(mp->mnt_fsname,&partition) || !safe_same(&partition,&item)) continue;
                snprintf(node,sizeof node,"/sys/class/block/%s",mp->mnt_fsname+5);
                hub_device_label(node,mp->mnt_fsname,item.label,sizeof item.label); break;
            }
            endmntent(table);
        }
        pthread_mutex_lock(&safe_lock);
        for (int i=0;i<SAFE_RECORDS;i++) if (safe_records[i].item.state && safe_same(&safe_records[i].item,&item)) {
            if (*safe_records[i].item.label) snprintf(item.label,sizeof item.label,"%s",safe_records[i].item.label);
            item.state=safe_records[i].item.state;
        }
        pthread_mutex_unlock(&safe_lock);
        out->items[out->count++]=item;
    }
    closedir(blocks);
}

/* Process-lifetime storage controller registry, resolved from the native button's
 * dependency key. Read-only bounded lookup; reject unexpected layout/vtable. */
static uintptr_t safe_map_lookup(uintptr_t buckets_addr,uintptr_t count_addr,uint64_t key) {
    uintptr_t buckets=*(uintptr_t *)buckets_addr; int count=*(int *)count_addr;
    if (!buckets || count<1 || count>4096) return 0;
    uintptr_t node=((uintptr_t *)buckets)[key%(unsigned)count];
    for (int n=0;node && n<256;n++,node=((uintptr_t *)node)[2])
        if (*(uint64_t *)node==key) return ((uintptr_t *)node)[1];
    return 0;
}
static void safe_eject_capture_controller(void *button) {
    uintptr_t iface=*(uintptr_t *)((char *)button+0x20);
    if (!iface) {
        uint64_t key=*(uint64_t *)((char *)button+0x28);
        uint64_t mapped=safe_map_lookup(0x3fa17d8,0x3fa17e8,key);
        if (mapped) iface=safe_map_lookup(0x3fc49a8,0x3fc49b8,mapped);
    }
    if (iface && *(uintptr_t *)iface==0x2a58570) safe_controller=iface-0x18;
}
static int safe_busy(unsigned sources,char *reason,size_t size) {
    pthread_mutex_lock(&safe_lock);
    int tracked_busy=0;
    for (int i=0;i<16;i++) if (safe_loads[i].controller && safe_loads[i].source>=0 &&
        (sources&(1u<<safe_loads[i].source))) tracked_busy=1;
    pthread_mutex_unlock(&safe_lock);
    if (tracked_busy) { snprintf(reason,size,"USB has a loaded or loading track. Unload it first."); return 1; }
    uintptr_t controller=safe_controller;
    if (!controller) { snprintf(reason,size,"Cannot check decks. Leave USB connected."); return 1; }
    for (int deck=0;deck<4;deck++) {
        unsigned char id[16]={0},kind=0;
        int loaded=((unsigned char (*)(uintptr_t,int,void *,void *))0xbc5ed0)(controller,deck,id,&kind);
        uint64_t identity=0; memcpy(&identity,id,8); identity&=0x0000ffffffffffffULL;
        lf_core->log(EJECT_ID,"safe-eject deck=%d loaded=%d media=%#lx kind=%u",deck+1,loaded,identity,kind);
        if (!loaded) {
            if (lf_core->deck_loaded(deck)!=0) {
                az_deck_info info={.size=sizeof info}; int failed_metadata=0;
                if (AZ_HAS(lf_core,deck_info) && !lf_core->deck_info(deck,&info) && info.content_id) {
                    pthread_mutex_lock(&safe_lock);
                    for (int i=0;i<16;i++) if (safe_loads[i].controller && safe_loads[i].deck==deck &&
                        safe_loads[i].failed && !safe_loads[i].loading && safe_loads[i].content_id==info.content_id)
                        failed_metadata=1;
                    /* A newer in-flight request on the same deck invalidates that proof. */
                    for (int i=0;i<16;i++) if (safe_loads[i].controller && safe_loads[i].deck==deck &&
                        safe_loads[i].loading) failed_metadata=0;
                    pthread_mutex_unlock(&safe_lock);
                }
                if (!failed_metadata) { snprintf(reason,size,"Deck %d is changing tracks. Try again.",deck+1); return 1; }
                lf_core->log(EJECT_ID,"safe-eject stale-failed-metadata deck=%d content=%u",deck+1,info.content_id);
            }
            continue;
        }
        int source=safe_source_identity(identity);
        if (source<0 && !hub_eject_known_native_usb(identity)) {
            snprintf(reason,size,"Cannot identify the USB loaded on deck %d. Leave it connected.",deck+1); return 1;
        }
        if (source>=0 && (sources&(1u<<source))) {
            snprintf(reason,size,"USB is loaded on deck %d. Unload that track first.",deck+1); return 1;
        }
    }
    return 0;
}

#include "eject-listener.h"
static void safe_listener_noop(void *listener) { (void)listener; }
static void safe_listener_done(struct safe_listener *listener,const void *media) {
    (void)media;
    lf_core->log(EJECT_ID,"safe-eject server-complete source=%d kind=%d",listener->source,listener->kind);
    safe_listener_complete(listener);
}
static uintptr_t safe_listener_vtable[4]={(uintptr_t)safe_listener_noop,(uintptr_t)safe_listener_noop,
    (uintptr_t)safe_listener_noop,(uintptr_t)safe_listener_done};
static struct safe_listener *safe_new_listener(int source,int kind) {
    return safe_listener_acquire(source,kind,safe_listener_vtable);
}
static int safe_mounted(const char *path) {
    FILE *table=setmntent("/proc/mounts","re"); if (!table) return -1;
    int found=0; struct mntent entry,*m; char buffer[8192];
    while ((m=getmntent_r(table,&entry,buffer,sizeof buffer))) if (!strcmp(m->mnt_dir,path)) found=1;
    endmntent(table); return found;
}
static int safe_eject_source_poll(struct fake *source,time_t now,time_t *ready_at,int waiting) {
    if (!source->eject_reserved) return 0;
    if (source->eject_dispatching) return 1;
    if (__atomic_exchange_n(&source->eject_withdraw,0,__ATOMIC_ACQ_REL)) {
        source->present=0; source->notify_pending=0; source->mount_at=0;
        source->announced_at=source->queried_at=0; source->unmount_pending=3;
        uint16_t id=0x102;
        struct safe_listener *music=source->eject_music_listener,*info=source->eject_info_listener;
        if (source->srv_music) ((void (*)(uint64_t,void *,void *))0x1373928)(source->srv_music,&id,music); else music->done=1;
        if (source->srv_info) ((void (*)(uint64_t,void *,void *))0x1975468)(source->srv_info,&id,info); else info->done=1;
        source->mounted=0; source->eject_phase=1;
        lf_core->log(EJECT_ID,"safe-eject withdrawal source=%ld",(long)(source-fakes));
    }
    if (source->eject_phase==1) {
        struct safe_listener *music=source->eject_music_listener,*info=source->eject_info_listener;
        if (!music->done || !info->done || waiting>=0 || now<*ready_at) return 1;
        FILE *box=fopen("/proc/udev_nfs","we"); if (!box) return 1;
        int failed=fprintf(box,"umount %s nfs",source->mp)<0;
        if (fclose(box)) failed=1;
        if (failed) return 1;
        source->eject_phase=2; *ready_at=now+3;
    }
    if (source->eject_phase==2 && now>=*ready_at) {
        int mounted=safe_mounted(source->mp);
        if (mounted<0 || (mounted && umount(source->mp))) return 1;
        if (safe_mounted(source->mp)!=0) return 1;
        source->unmount_pending=0; source->eject_phase=3;
        source->eject_music_listener=NULL; source->eject_info_listener=NULL;
        source->eject_done=1;
        lf_core->log(EJECT_ID,"safe-eject source-closed source=%ld",(long)(source-fakes));
    }
    return 1;
}

struct safe_mount { char path[PATH_MAX],type[64]; int backing; };
static int safe_raw_device_busy(const struct eject_item *item,int allow_exfat);
/* Recognize the stock exFAT daemon narrowly: executable, separate device and
 * mount arguments, plus a live /dev/fuse descriptor. Other raw users stay busy. */
static int safe_exfat_owner(const char *pid,const struct eject_item *item,const char *mount) {
    char path[PATH_MAX],executable[PATH_MAX],arguments[8192];
    snprintf(path,sizeof path,"/proc/%s/exe",pid);
    ssize_t length=readlink(path,executable,sizeof executable-1);
    if (length<0) return 0;
    executable[length]=0;
    if (strcmp(executable,"/usr/sbin/mount.exfat-fuse") && strcmp(executable,"/sbin/mount.exfat-fuse")) return 0;
    snprintf(path,sizeof path,"/proc/%s/cmdline",pid);
    FILE *file=fopen(path,"re"); if (!file) return 0;
    size_t count=fread(arguments,1,sizeof arguments,file); fclose(file);
    if (!count || count==sizeof arguments || arguments[count-1]) return 0;
    int device_arg=0,mount_arg=0;
    for (size_t n=0;n<count;n+=strlen(arguments+n)+1) {
        if (!strncmp(arguments+n,"/dev/sd",7)) {
            struct eject_item device={0};
            if (safe_usb_identity(arguments+n,&device) && safe_same(item,&device)) device_arg=1;
        }
        if (mount && arguments[n]=='/') {
            struct stat target,argument;
            /* Bind aliases share the daemon's backing filesystem but are not
             * separate command-line arguments. Recognize that same filesystem. */
            if (!stat(mount,&target) && !stat(arguments+n,&argument) &&
                S_ISDIR(argument.st_mode) && target.st_dev==argument.st_dev) mount_arg=1;
        }
    }
    if (!device_arg || (mount && !mount_arg)) return 0;
    snprintf(path,sizeof path,"/proc/%s/fd",pid);
    DIR *fds=opendir(path); if (!fds) return 0;
    struct dirent *fd; int fuse=0;
    struct stat expected; if (stat("/dev/fuse",&expected)) { closedir(fds); return 0; }
    while ((fd=readdir(fds))) {
        if (fd->d_name[0]=='.') continue;
        snprintf(path,sizeof path,"/proc/%s/fd/%s",pid,fd->d_name);
        struct stat st;
        if (!stat(path,&st) && S_ISCHR(st.st_mode) && st.st_rdev==expected.st_rdev) fuse=1;
    }
    closedir(fds); return fuse;
}
static int safe_exfat_mount(const struct eject_item *item,const struct safe_mount *mount) {
    if (strcmp(mount->type,"fuseblk")) return 0;
    DIR *processes=opendir("/proc"); if (!processes) return 0;
    struct dirent *process; int found=0;
    while (!found && (process=readdir(processes))) {
        if (!isdigit((unsigned char)process->d_name[0])) continue;
        found=safe_exfat_owner(process->d_name,item,mount->path);
    }
    closedir(processes); return found;
}
static int safe_collect_mounts(const struct eject_item *item,struct safe_mount out[SAFE_MOUNTS]) {
    FILE *table=setmntent("/proc/mounts","re"); if (!table) return -1;
    int count=0; struct mntent entry,*m; char buffer[8192];
    while ((m=getmntent_r(table,&entry,buffer,sizeof buffer))) {
        if (strncmp(m->mnt_fsname,"/dev/sd",7)) continue;
        struct eject_item device={0}; if (!safe_usb_identity(m->mnt_fsname,&device) || !safe_same(item,&device)) continue;
        if (count==SAFE_MOUNTS || strlen(m->mnt_dir)>=PATH_MAX) { count=-1; break; }
        snprintf(out[count].path,sizeof out[count].path,"%s",m->mnt_dir);
        snprintf(out[count].type,sizeof out[count].type,"%s",m->mnt_type);
        out[count].backing=!strncmp(m->mnt_dir,"/media/usb/",11); count++;
    }
    endmntent(table);
    /* Include dependent mounts nested below a selected backing mount (for
     * example native caches), regardless of their own underlying filesystem. */
    if (count<0) return count;
    int original=count; table=setmntent("/proc/mounts","re"); if (!table) return -1;
    while ((m=getmntent_r(table,&entry,buffer,sizeof buffer))) {
        int dependent=0,known=0;
        for (int i=0;i<original;i++) if (out[i].backing && eject_under(m->mnt_dir,out[i].path) && strcmp(m->mnt_dir,out[i].path)) dependent=1;
        for (int i=0;i<count;i++) if (!strcmp(m->mnt_dir,out[i].path)) known=1;
        if (!dependent || known) continue;
        if (count==SAFE_MOUNTS) { count=-1; break; }
        snprintf(out[count].path,sizeof out[count].path,"%s",m->mnt_dir);
        snprintf(out[count].type,sizeof out[count].type,"%s",m->mnt_type); out[count].backing=0; count++;
    }
    endmntent(table); if (count<0) return count;
    for (int i=0;i<count;i++) for (int j=i+1;j<count;j++) if (strlen(out[j].path)>strlen(out[i].path)) {
        struct safe_mount tmp=out[i]; out[i]=out[j]; out[j]=tmp;
    }
    return count;
}
static int safe_unmount_device(const struct eject_item *item,char *reason,size_t size) {
    struct safe_mount *mounts=calloc(SAFE_MOUNTS,sizeof *mounts); if (!mounts) return -1;
    int count=safe_collect_mounts(item,mounts),result=-1;
    if (count<0) { snprintf(reason,size,"Could not inspect all USB mounts."); goto finish; }
    for (int i=0;i<count;i++) {
        if (strstr(mounts[i].type,"fuse") && !safe_exfat_mount(item,&mounts[i])) {
            snprintf(reason,size,"This USB filesystem needs additional eject support. Leave it connected."); goto finish;
        }
        if (!safe_connected(item)) { snprintf(reason,size,"USB disconnected before eject completed."); goto finish; }
        int fd=open(mounts[i].path,O_RDONLY|O_DIRECTORY|O_CLOEXEC);
        if (fd<0) { snprintf(reason,size,"Cannot open USB for flushing: %s",strerror(errno)); goto finish; }
        int flushed=syncfs(fd),saved=errno; close(fd);
        if (flushed) { snprintf(reason,size,"USB flush failed: %s",strerror(saved)); goto finish; }
        if (!safe_connected(item)) { snprintf(reason,size,"USB changed during eject."); goto finish; }
        if (umount(mounts[i].path)) { snprintf(reason,size,"USB is busy or could not unmount: %s",strerror(errno)); goto finish; }
        lf_core->log(EJECT_ID,"safe-eject unmounted path=%s",mounts[i].path);
    }
    if (!safe_connected(item) || safe_collect_mounts(item,mounts)!=0) { snprintf(reason,size,"USB still has active mounts or was disconnected."); goto finish; }
    /* Normal FUSE unmount asks the daemon to flush/destroy its session. Wait for
     * its block handle to close as well; never kill it or claim safety early. */
    uint64_t deadline=eject_clock_ms()+5000;
    while (safe_raw_device_busy(item,0)) {
        if (!safe_connected(item) || eject_clock_ms()>deadline) {
            snprintf(reason,size,"USB filesystem is still closing. Leave it connected and retry."); goto finish;
        }
        usleep(100000);
    }
    result=0;
finish:
    free(mounts); return result;
}

static int safe_raw_device_busy(const struct eject_item *item,int allow_exfat) {
    DIR *processes=opendir("/proc"); if (!processes) return 1;
    struct dirent *process; int busy=0;
    while (!busy && (process=readdir(processes))) {
        if (!isdigit((unsigned char)process->d_name[0])) continue;
        if (allow_exfat && safe_exfat_owner(process->d_name,item,NULL)) continue;
        char path[PATH_MAX]; snprintf(path,sizeof path,"/proc/%s/fd",process->d_name);
        DIR *fds=opendir(path);
        if (!fds) { if (errno!=ENOENT && errno!=ESRCH) busy=1; continue; }
        struct dirent *fd;
        while (!busy && (fd=readdir(fds))) {
            if (fd->d_name[0]=='.') continue;
            snprintf(path,sizeof path,"/proc/%s/fd/%s",process->d_name,fd->d_name);
            struct stat st; if (stat(path,&st) || !S_ISBLK(st.st_mode)) continue;
            char node[128],resolved[PATH_MAX];
            snprintf(node,sizeof node,"/sys/dev/block/%u:%u",major(st.st_rdev),minor(st.st_rdev));
            if (realpath(node,resolved) && eject_under(resolved,item->usb)) busy=1;
        }
        closedir(fds);
    }
    closedir(processes); return busy;
}
static int safe_preflight(const struct eject_item *item,char *reason,size_t size) {
    if (safe_raw_device_busy(item,1)) { snprintf(reason,size,"Another process is using the USB device. Leave it connected."); return -1; }
    struct safe_mount *mounts=calloc(SAFE_MOUNTS,sizeof *mounts); if (!mounts) return -1;
    int count=safe_collect_mounts(item,mounts),failed=count<0;
    if (failed) snprintf(reason,size,"Could not inspect all USB mounts.");
    for (int i=0;i<count;i++) if (strstr(mounts[i].type,"fuse") && !safe_exfat_mount(item,&mounts[i])) {
        failed=1; snprintf(reason,size,"This USB filesystem needs additional eject support. Leave it connected.");
    }
    free(mounts); return failed ? -1:0;
}

static void safe_eject_submit(const struct eject_item *item,int all_port) {
    struct safe_request request={.all_port=all_port,.pending=1};
    if (all_port>=0) {
        pthread_mutex_lock(&eject_lock);
        if (eject_live.overflow&(1u<<all_port)) { pthread_mutex_unlock(&eject_lock); safe_message("Too many USB devices to eject this hub safely."); return; }
        for (int i=0;i<eject_live.count;i++) if (eject_live.items[i].port==all_port) request.items[request.count++]=eject_live.items[i];
        pthread_mutex_unlock(&eject_lock);
    } else if (item) request.items[request.count++]=*item;
    pthread_mutex_lock(&safe_lock);
    if (safe_running || safe_request.pending) {
        pthread_mutex_unlock(&safe_lock); safe_message("USB ejection is already in progress."); return;
    }
    safe_request=request;
    pthread_mutex_unlock(&safe_lock);
    lf_core->banner(EJECT_ID,"Preparing USB for removal...",2000);
}
static void safe_eject_tick(void) {
    static int mount_fault_announced;
    if (hub_mount_faulted() && !mount_fault_announced) {
        mount_fault_announced=1;
        safe_message("USB mount could not finish. Leave the hub connected and restart the AZ.");
    }
    char message[256]={0};
    pthread_mutex_lock(&safe_lock); snprintf(message,sizeof message,"%s",safe_feedback); *safe_feedback=0; pthread_mutex_unlock(&safe_lock);
    if (*message) { lf_core->log(EJECT_ID,"safe-eject result: %s",message); lf_core->banner(EJECT_ID,message,4500); }
}
static void safe_eject_worker_step(void) {
    safe_reconcile();
    struct safe_request request;
    pthread_mutex_lock(&safe_lock);
    if (!safe_request.pending) { pthread_mutex_unlock(&safe_lock); return; }
    request=safe_request; safe_request.pending=0; safe_running=1;
    pthread_mutex_unlock(&safe_lock);
    unsigned sources=0; int records[EJECT_MAX],previous[EJECT_MAX],reserved=0,started=0; char reason[256]={0};
    pthread_mutex_lock(&safe_mount_mutex);
    if (hub_mount_faulted()) {
        snprintf(reason,sizeof reason,"USB mount could not finish. Leave the hub connected and restart the AZ.");
        goto failed;
    }
    for (int n=0;n<request.count;n++) {
        struct eject_item *item=&request.items[n];
        if (!safe_connected(item)) { snprintf(reason,sizeof reason,"USB changed. Open the selector again."); goto failed; }
        safe_revalidate_result(item);
        if (safe_preflight(item,reason,sizeof reason)) goto failed;
        pthread_mutex_lock(&safe_lock);
        int record=-1;
        for (int i=0;i<SAFE_RECORDS;i++) if (safe_records[i].item.state && safe_same(&safe_records[i].item,item)) record=i;
        if (record<0) for (int i=0;i<SAFE_RECORDS;i++) if (!safe_records[i].item.state) { record=i; break; }
        if (record<0) { pthread_mutex_unlock(&safe_lock); snprintf(reason,sizeof reason,"USB tracking is full. Leave the hub connected."); goto failed; }
        if (safe_records[record].item.state==2) { pthread_mutex_unlock(&safe_lock); continue; }
        previous[reserved]=safe_records[record].item.state;
        safe_records[record].item=*item; safe_records[record].item.state=1;
        sources|=safe_records[record].sources;
        pthread_mutex_unlock(&safe_lock);
        records[reserved++]=record;
        /* Bind the physical connection to source paths atomically with config
         * reconciliation/reuse. Release this mutex before waiting on the
         * injector thread: that same thread owns discovery and server teardown. */
        pthread_mutex_lock(&safe_source_mutex);
        for (int s=0;s<nfakes;s++) {
            struct stat st; if (stat(fakes[s].dir,&st)) continue;
            char node[64]; snprintf(node,sizeof node,"/sys/dev/block/%u:%u",major(st.st_dev),minor(st.st_dev));
            char resolved[PATH_MAX]; if (!realpath(node,resolved) || !eject_under(resolved,item->usb)) continue;
            sources|=1u<<s; safe_records[record].sources|=1u<<s;
            pthread_mutex_lock(&safe_lock);
            fakes[s].eject_reserved=1;
            pthread_mutex_unlock(&safe_lock);
        }
        pthread_mutex_unlock(&safe_source_mutex);
    }
    if (safe_busy(sources,reason,sizeof reason)) goto failed;
    int persisted=safe_persist();
    if (persisted) { snprintf(reason,sizeof reason,"Could not preserve USB eject state. Leave it connected."); goto failed; }
    lf_core->log(EJECT_ID,"safe-eject reserved targets=%d sources=%#x time=%llu",reserved,sources,(unsigned long long)eject_clock_ms());
    /* Explicit emulator fault/race fixture; absent in ordinary operation. */
    const char *test_pause=getenv("AZHUB_EJECT_TEST_PAUSE_MS");
    unsigned pause_ms=test_pause ? strtoul(test_pause,NULL,10):0;
    if (pause_ms && pause_ms<=10000) usleep(pause_ms*1000);
    for (int s=0;s<nfakes;s++) if ((sources&(1u<<s)) && !fakes[s].eject_done) {
        /* A retry resumes its existing withdrawal. Replacing these adapters
         * would orphan a pending callback and restart teardown underneath it. */
        if (fakes[s].eject_music_listener || fakes[s].eject_info_listener) { started=1; continue; }
        struct safe_listener *music=safe_new_listener(s,0),*info=safe_new_listener(s,1);
        if (!music || !info) {
            if (music) safe_listener_complete(music);
            if (info) safe_listener_complete(info);
            snprintf(reason,sizeof reason,"USB server is still closing. Leave it connected and retry."); goto failed;
        }
        fakes[s].eject_music_listener=music; fakes[s].eject_info_listener=info;
        fakes[s].eject_withdraw=1; started=1;
    }
    uint64_t deadline=eject_clock_ms()+60000;
    for (;;) {
        int pending=0;
        for (int s=0;s<nfakes;s++) if ((sources&(1u<<s)) && !fakes[s].eject_done) pending=1;
        if (!pending) break;
        if (eject_clock_ms()>deadline) { snprintf(reason,sizeof reason,"USB is still busy closing. Leave it connected and retry."); goto failed; }
        usleep(100000);
    }
    if (safe_busy(sources,reason,sizeof reason)) goto failed;
    for (int n=0;n<reserved;n++) {
        int record=records[n];
        /* Non-library USBs also need persistent inhibition after a partial
         * filesystem teardown, including failure on a later partition. */
        started=1;
        if (safe_unmount_device(&safe_records[record].item,reason,sizeof reason)) goto failed;
        pthread_mutex_lock(&safe_lock); safe_records[record].item.state=2; pthread_mutex_unlock(&safe_lock);
        if (safe_persist()) {
            pthread_mutex_lock(&safe_lock); safe_records[record].item.state=3; pthread_mutex_unlock(&safe_lock);
            snprintf(reason,sizeof reason,"Could not preserve USB eject state. Leave it connected and retry."); goto failed;
        }
    }
    if (request.all_port>=0) {
        struct eject_inventory live; safe_eject_scan(&live);
        if (live.overflow&(1u<<request.all_port)) { snprintf(reason,sizeof reason,"Hub changed during eject. Leave it connected."); goto failed; }
        for (int n=0;n<live.count;n++) if (live.items[n].port==request.all_port && live.items[n].state!=2) {
            snprintf(reason,sizeof reason,"Hub changed during eject. Leave it connected and retry."); goto failed;
        }
        safe_message("Hub on USB %d is safe to remove.",request.all_port+1);
    } else safe_message("%.48s (hub port %.24s) is safe to remove.",
                        request.count ? request.items[0].label:"USB",
                        request.count ? request.items[0].location:"?");
    lf_core->log(EJECT_ID,"safe-eject success targets=%d sources=%#x",reserved,sources);
    goto finished;
failed:
    pthread_mutex_lock(&safe_source_mutex);
    pthread_mutex_lock(&safe_lock);
    for (int n=0;n<reserved;n++) {
        struct safe_record *record=&safe_records[records[n]];
        if (!started) {
            if (previous[n]) record->item.state=previous[n];
            else {
                for (int s=0;s<nfakes;s++) if (record->sources&(1u<<s)) fakes[s].eject_reserved=0;
                memset(record,0,sizeof *record);
            }
        } else if (record->item.state!=2) record->item.state=3;
    }
    pthread_mutex_unlock(&safe_lock);
    pthread_mutex_unlock(&safe_source_mutex);
    safe_persist();
    safe_message("%.220s",*reason ? reason:"USB ejection failed. Leave it connected.");
finished:
    pthread_mutex_unlock(&safe_mount_mutex);
    pthread_mutex_lock(&safe_lock); safe_running=0; pthread_mutex_unlock(&safe_lock);
}
static int safe_eject_prepare(void) {
    if (!eject_environment_allowed()) return -71;
#ifndef LF_EJECT_AZ
    if (!getenv("AZHUB_EJECT_UI_PROTOTYPE")) return -71;
#endif
    FILE *file=fopen(SAFE_JOURNAL,"re");
    if (!file && errno!=ENOENT) return -72;
    if (file) {
        char line[8192],encoded[257];
        for (int i=0;i<SAFE_RECORDS;i++) {
            struct eject_item item={0};
            if (!fgets(line,sizeof line,file)) break;
            int state=3;
            int read=sscanf(line,"%llu %u %d %4095s %d %256s",&item.generation,&item.devnum,&item.port,item.usb,&state,encoded);
            if (read<4 || state<1 || state>3) { fclose(file); return -73; }
            if (item.port<0 || item.port>1 || strncmp(item.usb,"/sys/devices/",13) || !safe_connected(&item)) continue;
            if (read==6 && strcmp(encoded,"-")) {
                size_t length=strlen(encoded);
                if ((length&1) || length/2>=sizeof item.label) { fclose(file); return -73; }
                for (size_t n=0;n<length/2;n++) {
                    int high=hub_hex_digit(encoded[n*2]),low=hub_hex_digit(encoded[n*2+1]);
                    if (high<0 || low<0) { fclose(file); return -73; }
                    item.label[n]=(char)(high*16+low);
                }
            }
            item.state=state==2 ? 2:3; safe_records[i].item=item;
        }
        fclose(file);
    }
    return 0;
}
static int safe_eject_init(void) {
    if (lf_core->claim_hook(EJECT_ID,0xab2ab8,safe_track_execute_hook,(void **)&safe_original_track_execute) ||
        lf_core->claim_hook(EJECT_ID,0xab35b8,safe_track_execute_range_hook,(void **)&safe_original_track_execute_range)) return -78;
    if (lf_core->claim_hook(EJECT_ID,0xa7d0f8,safe_load_common_hook,(void **)&safe_original_load_common)) return -75;
    if (lf_core->claim_hook(EJECT_ID,0xa7f688,safe_load_hook,(void **)&safe_original_load) ||
        lf_core->claim_hook(EJECT_ID,0xa77ae4,safe_load_complete_hook,(void **)&safe_original_load_complete) ||
        lf_core->claim_hook(EJECT_ID,0xa75bd0,safe_unload_complete_hook,(void **)&safe_original_unload_complete)) return -74;
    return 0;
}

static void safe_json_string(FILE *file,const char *text) {
    fputc('"',file);
    for (const unsigned char *p=(const unsigned char *)text;*p;p++) {
        if (*p=='"' || *p=='\\') fprintf(file,"\\%c",*p);
        else if (*p<32) fprintf(file,"\\u%04x",*p);
        else fputc(*p,file);
    }
    fputc('"',file);
}
/* Optional emulator telemetry, never called from input or UI callbacks. */
static void safe_eject_status(const struct eject_inventory *inventory) {
    if (!getenv("AZHUB_EJECT_STATUS")) return;
    FILE *file=fopen("/run/azhub-eject-status.new","we"); if (!file) return;
    fprintf(file,"{\"pid\":%d,\"clock_ms\":%llu,\"view\":%d,\"devices\":[",getpid(),(unsigned long long)eject_clock_ms(),AZ_HAS(lf_core,view) ? lf_core->view():-1);
    for (int i=0;i<inventory->count;i++) {
        const struct eject_item *item=&inventory->items[i];
        if (i) fputc(',',file);
        fprintf(file,"{\"label\":"); safe_json_string(file,item->label);
        fprintf(file,",\"disk\":"); safe_json_string(file,item->disk);
        fprintf(file,",\"port\":%d,\"state\":%d,\"generation\":%llu,\"devnum\":%u}",item->port+1,item->state,item->generation,item->devnum);
    }
    fprintf(file,"],\"decks\":[");
    for (int deck=0;deck<4;deck++) {
        az_deck_info info={.size=sizeof info};
        if (AZ_HAS(lf_core,deck_info)) lf_core->deck_info(deck,&info);
        if (deck) fputc(',',file);
        fprintf(file,"{\"loaded\":%d,\"playing\":%d,\"pos_s\":%.3f,\"content_id\":%u,\"title\":",info.loaded,info.playing,info.pos_s,info.content_id);
        safe_json_string(file,info.title); fputc('}',file);
    }
    fprintf(file,"]}\n");
    int failed=ferror(file); if (fclose(file)) failed=1;
    if (!failed) rename("/run/azhub-eject-status.new","/run/azhub-eject-status");
}
#endif
