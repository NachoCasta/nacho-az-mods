/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_SOURCE_METADATA_NATIVE_H
#define HUB_SOURCE_METADATA_NATIVE_H
struct hub_metadata_blob {unsigned char bytes[0x94];};
typedef struct hub_metadata_blob (*hub_metadata_getter)(uintptr_t);
#ifndef HUB_METADATA_GET
#define HUB_METADATA_GET(point) ((hub_metadata_getter)0x134f210)(point)
#endif
static fn8 hub_metadata_mount_original;
static uint64_t hub_metadata_mount(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
                                    uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    int source=-1;uintptr_t server=0;struct hub_metadata_ticket ticket={0};
    if (a && lf_runtime_ready()) {
        server=*(uintptr_t *)(uintptr_t)a;
        pthread_mutex_lock(&safe_source_mutex);
        for (int i=0;i<nfakes;i++) if (fakes[i].present && fakes[i].srv_music==server) {
            source=i;ticket=hub_metadata_capture(i);break;
        }
        pthread_mutex_unlock(&safe_source_mutex);
        if (source>=0 && strcmp(jstr((uintptr_t)a+0x10),ticket.root)) source=-1;
    }
    uint64_t result=hub_metadata_mount_original(a,b,c,d,e,f,g,h);
    if (source<0 || !lf_runtime_ready()) return result;
    /* This task still owns its library worker. The information server retains
     * the mount point, and queued unmount cannot run before this call returns. */
    uintptr_t commands=*(uintptr_t *)(server+0x1d0);
    uintptr_t info=commands ? *(uintptr_t *)(commands+0x10):0;
    uintptr_t point=info ? *(uintptr_t *)info:0;
    if (!point || *(uintptr_t *)point!=0x2baf7e8 || strcmp(jstr(point+0x10),ticket.root)) return result;
    /* The plain aggregate uses AArch64 x8, matching the verified native ABI. */
    struct hub_metadata_blob blob=HUB_METADATA_GET(point);
    struct hub_source_properties properties;
    if (!hub_source_properties_decode(blob.bytes,sizeof blob.bytes,&properties)) return result;
    pthread_mutex_lock(&safe_source_mutex);
    if (fakes[source].present && !fakes[source].unmount_pending &&
        !strcmp(fakes[source].dir,ticket.root) && !strcmp(fakes[source].port_route,ticket.route) &&
        hub_metadata_publish(source,&ticket,&properties))
        lf_log("source metadata ready source=%d name_valid=%d color=%u",source,properties.name_valid,properties.color);
    pthread_mutex_unlock(&safe_source_mutex);
    return result;
}
static int hub_metadata_install(void) {
    hub_metadata_mount_original=make_hook(0x1376c98,hub_metadata_mount);
    return hub_metadata_mount_original ? 0:-93;
}
#endif
