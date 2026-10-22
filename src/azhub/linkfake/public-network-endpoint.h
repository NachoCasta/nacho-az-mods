/* SPDX-License-Identifier: GPL-3.0-only */
/* Fresh per-source sockets in an already allocated, caller-owned namespace.
 * A short helper thread enters that namespace; the controller never changes
 * namespace. Interface allocation/address selection are separate caller duties.
 * Call off the UI thread. Keep the namespace FD valid until open returns. */
#ifndef HUB_PUBLIC_NETWORK_ENDPOINT_H
#define HUB_PUBLIC_NETWORK_ENDPOINT_H
#include <sched.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <stdio.h>
#include "public-source-manager.h"
struct hub_public_network_spec {
    int namespace_fd;
    char interface[IFNAMSIZ];
    struct in_addr address,mask,broadcast;
    unsigned char mac[6];
    unsigned player;
    char label[256];
    hub_public_rpc_peer_policy allowed;
    void *policy;
};
static void hub_public_network_close(struct hub_public_source_binding *binding) {
    int *sockets[]={&binding->listener,&binding->udp,&binding->query_listener,
        &binding->provider.discovery_fd,&binding->provider.status_fd,&binding->provider.rpc_alias_fd};
    for (unsigned i=0;i<sizeof sockets/sizeof *sockets;++i)
        if (*sockets[i]>=0) { close(*sockets[i]); *sockets[i]=-1; }
}
static int hub_public_network_socket(const struct hub_public_network_spec *spec,int type,
        uint16_t port,int broadcast,int wildcard) {
    int fd=socket(AF_INET,type|SOCK_NONBLOCK|SOCK_CLOEXEC,0);
    if (fd<0) return -1;
    int enabled=1;
    if (setsockopt(fd,SOL_SOCKET,SO_BINDTODEVICE,spec->interface,(socklen_t)strlen(spec->interface)+1)<0 ||
        (type==SOCK_STREAM && setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&enabled,sizeof enabled)<0) ||
        (broadcast && setsockopt(fd,SOL_SOCKET,SO_BROADCAST,&enabled,sizeof enabled)<0)) goto failed;
    struct sockaddr_in address={.sin_family=AF_INET,.sin_port=htons(port),.sin_addr=spec->address};
    if (wildcard) address.sin_addr.s_addr=htonl(INADDR_ANY);
    if (bind(fd,(struct sockaddr *)&address,sizeof address)<0 ||
        (type==SOCK_STREAM && listen(fd,8)<0)) goto failed;
    return fd;
failed: {
    int error=errno; close(fd); errno=error; return -1;
    }
}
static int hub_public_network_verify(const struct hub_public_network_spec *spec) {
    int fd=socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);
    if (fd<0) return -1;
    struct ifreq request={0}; strcpy(request.ifr_name,spec->interface);
    int valid=ioctl(fd,SIOCGIFFLAGS,&request)==0 && (request.ifr_flags&IFF_UP) && !(request.ifr_flags&IFF_LOOPBACK);
    const unsigned commands[]={SIOCGIFADDR,SIOCGIFNETMASK,SIOCGIFBRDADDR};
    const struct in_addr values[]={spec->address,spec->mask,spec->broadcast};
    for (unsigned i=0;valid && i<3;++i) {
        strcpy(request.ifr_name,spec->interface);
        valid=ioctl(fd,commands[i],&request)==0 && request.ifr_addr.sa_family==AF_INET &&
            ((struct sockaddr_in *)&request.ifr_addr)->sin_addr.s_addr==values[i].s_addr;
    }
    if (valid) valid=ioctl(fd,SIOCGIFHWADDR,&request)==0 && !memcmp(request.ifr_hwaddr.sa_data,spec->mac,6);
    close(fd); if (!valid) { errno=EINVAL; return -1; } return 0;
}
struct hub_public_network_open_request {
    const struct hub_public_network_spec *spec;
    struct hub_public_source_binding *binding;
    int error;
};
static void *hub_public_network_open_thread(void *opaque) {
    struct hub_public_network_open_request *request=opaque;
    const struct hub_public_network_spec *spec=request->spec;
    struct hub_public_source_binding *binding=request->binding;
    if (setns(spec->namespace_fd,CLONE_NEWNET)<0 || hub_public_network_verify(spec)<0) goto failed;
    binding->listener=hub_public_network_socket(spec,SOCK_STREAM,0,0,0);
    if (binding->listener<0) goto failed;
    binding->query_listener=hub_public_network_socket(spec,SOCK_STREAM,12523,0,0);
    if (binding->query_listener<0) goto failed;
    binding->udp=hub_public_network_socket(spec,SOCK_DGRAM,111,0,0);
    if (binding->udp<0) goto failed;
    binding->provider.rpc_alias_fd=hub_public_network_socket(spec,SOCK_DGRAM,50111,0,0);
    if (binding->provider.rpc_alias_fd<0) goto failed;
    binding->provider.discovery_fd=hub_public_network_socket(spec,SOCK_DGRAM,50000,1,1);
    if (binding->provider.discovery_fd<0) goto failed;
    binding->provider.status_fd=hub_public_network_socket(spec,SOCK_DGRAM,50002,0,0);
    if (binding->provider.status_fd<0) goto failed;
    struct sockaddr_in address; socklen_t length=sizeof address;
    if (getsockname(binding->listener,(struct sockaddr *)&address,&length)<0) goto failed;
    binding->metadata_port=ntohs(address.sin_port); binding->file_port=111;
    binding->player=spec->player; binding->allowed=spec->allowed; binding->policy=spec->policy;
    binding->advertise=1; binding->provider.rpc_alias_enabled=1;
    binding->provider.broadcast=spec->broadcast.s_addr;
    if (hub_public_provider_identity_init(&binding->provider.identity,spec->player,
            (const unsigned char *)&spec->address.s_addr,spec->mac,spec->label)<0) { errno=EINVAL; goto failed; }
    char ip[INET_ADDRSTRLEN],mask[INET_ADDRSTRLEN];
    if (!inet_ntop(AF_INET,&spec->address,ip,sizeof ip) || !inet_ntop(AF_INET,&spec->mask,mask,sizeof mask)) goto failed;
    snprintf(binding->provider.export_group,sizeof binding->provider.export_group,"%s/%s",ip,mask);
    fprintf(stderr,"public source sockets ready interface=%s address=%s player=%u query=12523 metadata=%u\n",
        spec->interface,ip,spec->player,binding->metadata_port);
    return NULL;
failed:
    request->error=errno ? errno : EIO; hub_public_network_close(binding); return NULL;
}
static int hub_public_network_open(const struct hub_public_network_spec *spec,struct hub_public_source_binding *binding) {
    if (!spec || !binding || spec->namespace_fd<0 || !spec->allowed || !spec->player || spec->player>255 ||
        !spec->interface[0] || strnlen(spec->interface,IFNAMSIZ)==IFNAMSIZ ||
        (strncmp(spec->interface,"azpub",5) && strncmp(spec->interface,"azhubp",6)) ||
        strnlen(spec->label,sizeof spec->label)==sizeof spec->label || (spec->mac[0]&3)!=2) { errno=EINVAL; return -1; }
    uint32_t mask=ntohl(spec->mask.s_addr),inverse=~mask;
    if (!mask || inverse&(inverse+1) || (spec->address.s_addr|~spec->mask.s_addr)!=spec->broadcast.s_addr) { errno=EINVAL; return -1; }
    memset(binding,0,sizeof *binding);
    binding->listener=binding->udp=binding->query_listener=-1;
    binding->provider.discovery_fd=binding->provider.status_fd=binding->provider.rpc_alias_fd=-1;
    struct hub_public_network_open_request request={spec,binding,0}; pthread_t thread;
    int error=pthread_create(&thread,NULL,hub_public_network_open_thread,&request);
    if (error) { errno=error; return -1; }
    /* This helper only executes local namespace/ioctl/socket syscalls. It has
     * no peer-dependent reads, waits, interface creation or retained thread. */
    error=pthread_join(thread,NULL);
    if (error) { errno=error; return -1; }
    if (request.error) { errno=request.error; return -1; }
    return 0;
}
/* Manager callback context is an array of eight declared endpoint specs. */
static int hub_public_network_manager_open(void *context,unsigned slot,struct hub_public_source_binding *binding) {
    if (!context || slot>=HUB_PUBLIC_MANAGED_SOURCES) { errno=EINVAL; return -1; }
    return hub_public_network_open((struct hub_public_network_spec *)context+slot,binding);
}
static void hub_public_network_manager_close(void *context,unsigned slot,struct hub_public_source_binding *binding) {
    (void)context; (void)slot; hub_public_network_close(binding);
}
#endif
