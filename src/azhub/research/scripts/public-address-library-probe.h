/* SPDX-License-Identifier: GPL-3.0-only */
/* Emulator research coupling: leased address -> independent public worker.
 * Called only from the single-threaded address probe child after ACD readiness.
 * Explicit owned /tmp markers coordinate independent external test clients.
 * This is not a production broker IPC protocol or EP147 lifecycle hook. */
#ifndef HUB_PUBLIC_ADDRESS_LIBRARY_PROBE_H
#define HUB_PUBLIC_ADDRESS_LIBRARY_PROBE_H
#include "../../linkfake/public-network-endpoint.h"
static int address_library_peer(void *unused,const struct sockaddr_in *peer) {
    (void)unused;return peer && peer->sin_family==AF_INET && peer->sin_addr.s_addr==inet_addr("192.0.2.2");
}
static int address_library_probe(int namespace_fd,struct hub_public_network_lease *lease,
        struct hub_public_arp *arp,struct hub_public_acd *acd) {
    const char *control=getenv("AZHUB_ADDRESS_LIBRARY_CONTROL");
    if (!control || strncmp(control,"/tmp/azhub-dhcp-",16) || strlen(control)>180 ||
            !hub_public_lease_ready(lease,now_ms())) return -1;
    for (const char *p=control+16;*p;++p)
        if (!((*p>='a' && *p<='z') || (*p>='0' && *p<='9') || *p=='-')) return -1;
    char ready[256],done[256];
    snprintf(ready,sizeof ready,"%s.ready",control);snprintf(done,sizeof done,"%s.done",control);
    if (!access(ready,F_OK) || !access(done,F_OK)) return -1;
    struct hub_public_network_spec spec={.namespace_fd=namespace_fd,.interface="azpub0",
        .address=lease->current.address,.mask=lease->current.mask,.broadcast=lease->current.broadcast,
        .player=41,.allowed=address_library_peer,.label="Hub USB 1"};
    memcpy(spec.mac,mac,6);
    struct hub_public_source_binding binding;
    if (hub_public_network_open(&spec,&binding)) return -1;
    struct hub_public_source_worker worker={0};int result=-1;
    unsigned char nonce[16];size_t used=0;int random=open("/dev/urandom",O_RDONLY|O_CLOEXEC);
    if (random<0) goto close_endpoints;
    while (used<sizeof nonce) {
        ssize_t got=read(random,nonce+used,sizeof nonce-used);
        if (got<0 && errno==EINTR) continue;
        if (got<=0) { close(random);goto close_endpoints; }used+=(size_t)got;
    }
    close(random);
    if (hub_public_source_worker_start_configured(&worker,"/media/usb/sdc1",lease->generation,
            nonce,binding.player,binding.listener,binding.udp,binding.file_port,binding.allowed,binding.policy,
            binding.query_listener,binding.metadata_port,&binding.provider)) goto close_endpoints;
    uint64_t deadline=now_ms()+180000;int published=0;
    while (now_ms()<deadline && hub_public_lease_ready(lease,now_ms())) {
        int conflict=hub_public_arp_receive(arp,acd,NULL,0);
        if (conflict) {
            if (conflict>0) address_probe_decline(arp,acd,&lease->current);
            break;
        }
        int state=hub_public_source_worker_state(&worker);
        if (state==HUB_PUBLIC_WORKER_FAILED || state==HUB_PUBLIC_WORKER_STOPPED ||
                state==HUB_PUBLIC_WORKER_WITHDRAWN) break;
        if (state==HUB_PUBLIC_WORKER_RUNNING && !published) {
            char address[INET_ADDRSTRLEN],message[256];
            if (!inet_ntop(AF_INET,&spec.address,address,sizeof address)) break;
            int length=snprintf(message,sizeof message,
                "{\"ready\":true,\"address\":\"%s\",\"player\":41,\"metadata_port\":%u,\"lease_expires_ms\":%llu,\"process_id\":%ld,\"generation\":%llu}\n",
                address,binding.metadata_port,(unsigned long long)lease->expires,(long)getpid(),(unsigned long long)lease->generation);
            int fd=open(ready,O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
            if (fd<0) break;
            int written=write(fd,message,(size_t)length)==length;close(fd);if (!written) break;
            published=1;fprintf(stderr,"ADDRESS_LIBRARY: worker running on acquired address %s\n",address);
        }
        if (published && !access(done,F_OK)) { result=0;break; }
        struct timespec pause={0,5000000};nanosleep(&pause,NULL);
    }
    hub_public_lease_retire(lease);
    hub_public_source_worker_withdraw(&worker);hub_public_source_worker_stop(&worker);
    /* Bounded polling; never close borrowed sockets before join acknowledgement. */
    uint64_t join_limit=now_ms()+10000;int joined;
    while (!(joined=hub_public_source_worker_join_ready(&worker))) {
        if (now_ms()>=join_limit) _exit(70); /* process teardown closes all owned FDs */
        struct timespec pause={0,1000000};nanosleep(&pause,NULL);
    }
    if (joined<0) _exit(71);
    if (worker.error) result=-1;
close_endpoints:
    hub_public_network_close(&binding);
    if (!result) fprintf(stderr,"ADDRESS_LIBRARY: withdrawn, worker joined and endpoints closed before address removal\n");
    return result;
}
#endif
