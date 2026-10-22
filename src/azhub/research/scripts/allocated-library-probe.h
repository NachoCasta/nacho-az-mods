/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicit emulator-only library control on the automatically allocated network.
 * No fork, namespace changes on controller or native library-slot borrowing. */
#ifndef HUB_ALLOCATED_LIBRARY_PROBE_H
#define HUB_ALLOCATED_LIBRARY_PROBE_H
#include "../../linkfake/public-network-ready.h"
static int allocated_library_peer(void *unused,const struct sockaddr_in *peer) {
    (void)unused;return peer && peer->sin_addr.s_addr==inet_addr("192.0.2.2");
}
static int allocated_library_probe(struct hub_public_network_ready *ready,const unsigned char parent_mac[6]) {
    const char *control=getenv("AZHUB_ADDRESS_LIBRARY_CONTROL");
    if (!control || strncmp(control,"/tmp/azhub-dhcp-",16) || strlen(control)>180) return -1;
    for (const char *p=control+16;*p;++p) if (!((*p>='a' && *p<='z') || (*p>='0' && *p<='9') || *p=='-')) return -1;
    char marker[256],done[256];snprintf(marker,sizeof marker,"%s.ready",control);snprintf(done,sizeof done,"%s.done",control);
    if (!access(marker,F_OK) || !access(done,F_OK)) return -1;
    struct hub_public_network_spec spec=ready->spec;spec.player=41;spec.allowed=allocated_library_peer;strcpy(spec.label,"Hub USB 1");
    struct hub_public_source_manager *manager=calloc(1,sizeof *manager);if (!manager) return -1;
    if (hub_public_source_manager_init(manager,(struct hub_public_source_manager_ops){hub_public_network_manager_open,hub_public_network_manager_close},&spec)<0) { free(manager);return -1; }
    int fd=open("/media/usb/sdc1",O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);if (fd<0) { free(manager);return -1; }
    int requested=hub_public_source_manager_request_fd(manager,0,fd,ready->generation);close(fd);
    if (requested<0) { free(manager);return -1; }
    uint64_t deadline=hub_public_address_remove_ms()+180000;int published=0,result=-1;
    while (hub_public_address_remove_ms()<deadline) {
        int live=hub_public_network_ready_poll(ready,hub_public_address_remove_ms(),500,(const unsigned char (*)[6])parent_mac,1);
        if (live!=1 || hub_public_source_manager_poll(manager,0)<0 || manager->sources[0].error) break;
        if (hub_public_source_worker_state(&manager->sources[0].worker)==HUB_PUBLIC_WORKER_RUNNING && !published) {
            char address[INET_ADDRSTRLEN],message[256];if (!inet_ntop(AF_INET,&spec.address,address,sizeof address)) break;
            int length=snprintf(message,sizeof message,"{\"ready\":true,\"address\":\"%s\",\"player\":41,\"metadata_port\":%u,\"generation\":%llu}\n",address,
                manager->sources[0].binding.metadata_port,(unsigned long long)ready->generation);
            int marker_fd=open(marker,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);if (marker_fd<0) break;
            int okay=write(marker_fd,message,length)==length;close(marker_fd);if (!okay) break;
            published=1;
        }
        if (published && !access(done,F_OK)) { result=0;break; }
        poll(NULL,0,1);
    }
    hub_public_source_manager_withdraw(manager,0);
    if (hub_public_source_manager_request(manager,0,NULL,0,0)<0) _exit(73);
    deadline=hub_public_address_remove_ms()+10000;
    while (!hub_public_source_manager_drained(manager,0)) {
        if (hub_public_source_manager_poll(manager,0)<0 || hub_public_address_remove_ms()>=deadline) _exit(74);
        poll(NULL,0,1);
    }
    if (manager->sources[0].error) result=-1;
    free(manager);return result;
}
#endif
