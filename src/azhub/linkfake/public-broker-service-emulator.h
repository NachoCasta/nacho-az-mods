/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicit one-source executable integration control. Network namespace/address
 * and synthetic root are prepared/cleaned by the emulator runner, not allocated
 * here. This mode is excluded from the ordinary helper build. */
#ifndef HUB_PUBLIC_BROKER_SERVICE_EMULATOR_H
#define HUB_PUBLIC_BROKER_SERVICE_EMULATOR_H
#include <sys/utsname.h>
#include "public-broker-probe-child.h"
#include "public-broker-session.h"
#include "public-network-endpoint.h"
struct hub_public_broker_service_fixture { struct hub_public_network_spec specs[8];int prepared[8]; };
static int hub_public_broker_service_allowed(void *unused,const struct sockaddr_in *peer) {
    (void)unused;return peer && peer->sin_addr.s_addr==inet_addr("192.0.2.2");
}
static int hub_public_broker_service_prepare(void *opaque,unsigned slot,
        const struct hub_public_broker_source *source,int root,int namespace_fd) {
    struct hub_public_broker_service_fixture *fixture=opaque;(void)root;
    const unsigned char mac[6]={2,0,0,0,0x12,0x73};
    if (slot || fixture->prepared[slot] || strcmp(source->directory,"/tmp/azhub-sdk-library") ||
            strcmp(source->interface,"azpub7") || source->player!=41 || memcmp(source->mac,mac,6) ||
            strcmp(source->label,"Hub USB 1")) { errno=EPERM;return -1; }
    int fd=fcntl(namespace_fd,F_DUPFD_CLOEXEC,0);if (fd<0) return -1;
    struct hub_public_network_spec *spec=fixture->specs+slot;
    memset(spec,0,sizeof *spec);spec->namespace_fd=fd;strcpy(spec->interface,"azpub7");
    memcpy(spec->mac,mac,6);spec->player=41;strcpy(spec->label,"Hub USB 1");spec->allowed=hub_public_broker_service_allowed;
    inet_pton(AF_INET,"192.0.2.3",&spec->address);inet_pton(AF_INET,"255.255.255.0",&spec->mask);
    inet_pton(AF_INET,"192.0.2.255",&spec->broadcast);fixture->prepared[slot]=1;return 0;
}
static int hub_public_broker_service_release(void *opaque,unsigned slot) {
    struct hub_public_broker_service_fixture *fixture=opaque;
    if (fixture->prepared[slot]) { close(fixture->specs[slot].namespace_fd);fixture->specs[slot].namespace_fd=-1;fixture->prepared[slot]=0; }
    /* Prepared addresses/namespaces remain runner-owned in this control. */
    return 0;
}
static int hub_public_broker_service_emulator(int argc,char **argv) {
    struct utsname kernel;if (uname(&kernel) || strcmp(kernel.release,"6.6.138-dirty")) return 126;
    unsigned char token[16];int fd=hub_public_broker_connect(argc,argv,"--broker-service-emulator",token);
    if (fd<0) return 126;
    struct hub_public_source_manager *manager=calloc(1,sizeof *manager);
    struct hub_public_broker_service_fixture fixture={0};struct hub_public_broker_dispatch dispatch;
    if (!manager) { close(fd);return 126; }
    if (hub_public_source_manager_init(manager,(struct hub_public_source_manager_ops){hub_public_network_manager_open,hub_public_network_manager_close},fixture.specs)<0 ||
            hub_public_broker_dispatch_init(&dispatch,manager,(struct hub_public_broker_dispatch_ops){.prepare=hub_public_broker_service_prepare,.release=hub_public_broker_service_release},&fixture)<0) {
        close(fd);free(manager);return 126;
    }
    struct ucred parent;socklen_t size=sizeof parent;
    if (getsockopt(fd,SOL_SOCKET,SO_PEERCRED,&parent,&size)<0) { close(fd);free(manager);return 126; }
    struct hub_public_broker_session session;
    if (hub_public_broker_session_init(&session,fd,parent.pid,geteuid(),token,&dispatch,NULL,NULL)<0) { close(fd);free(manager);return 126; }
    close(fd);uint64_t deadline=hub_public_broker_ms()+210000;int failed=0;
    for (;;) {
        if (hub_public_broker_ms()>=deadline && !session.lost) hub_public_broker_session_loss(&session);
        int result=hub_public_broker_session_poll(&session);
        if (result==1) break;
        if (result<0) { failed=1;hub_public_broker_session_loss(&session); }
        poll(NULL,0,1);
    }
    free(manager);return failed ? 1:0;
}
#endif
