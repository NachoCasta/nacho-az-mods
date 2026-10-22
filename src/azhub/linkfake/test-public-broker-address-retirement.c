/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include "public-broker-address-retirement.h"
#include "public-broker-lease.h"
#include <sys/utsname.h>
struct live_context { struct hub_public_source_manager *manager;struct hub_public_broker_address_retirement removal; };
static uint64_t live_now(void *unused) { (void)unused;return hub_public_worker_milliseconds(); }
static int live_allowed(void *unused,const struct sockaddr_in *peer) { (void)unused;return peer->sin_addr.s_addr==inet_addr("192.0.2.2"); }
static int live_remove(void *opaque,unsigned slot,const struct hub_public_network_spec *spec) {
    struct live_context *context=opaque;assert(hub_public_source_manager_drained(context->manager,slot));
    return hub_public_broker_address_retire(&context->removal,spec);
}
static int live(int argc,char **argv) {
    struct utsname kernel;assert(!uname(&kernel) && !strcmp(kernel.release,"6.6.138-dirty"));
    assert(argc==3 && !strcmp(argv[1],"--live") && !strncmp(argv[2],"/var/run/netns/azhub-rm-",23));
    struct hub_public_network_spec spec={.namespace_fd=open(argv[2],O_RDONLY|O_CLOEXEC),
        .interface="azpub7",.mac={2,0,0,0,0x12,0x73},.player=41,.label="Hub USB 1",.allowed=live_allowed};
    assert(spec.namespace_fd>=0);inet_pton(AF_INET,"192.0.2.3",&spec.address);
    inet_pton(AF_INET,"255.255.255.0",&spec.mask);inet_pton(AF_INET,"192.0.2.255",&spec.broadcast);
    char root[]="/tmp/azhub-retirement-XXXXXX";assert(mkdtemp(root));
    char pioneer[256],rb[256],pdb[256];snprintf(pioneer,sizeof pioneer,"%s/PIONEER",root);
    snprintf(rb,sizeof rb,"%s/PIONEER/rekordbox",root);snprintf(pdb,sizeof pdb,"%s/PIONEER/rekordbox/export.pdb",root);
    assert(!mkdir(pioneer,0700) && !mkdir(rb,0700));unsigned char bytes[1024]={0};
    bytes[5]=2;bytes[8]=1;bytes[36]=1;bytes[40]=1;bytes[516]=1;
    int file=open(pdb,O_CREAT|O_EXCL|O_WRONLY,0600);assert(file>=0 && write(file,bytes,sizeof bytes)==sizeof bytes);close(file);
    struct hub_public_source_manager *manager=calloc(1,sizeof *manager);assert(manager);
    struct live_context context={.manager=manager};
    struct hub_public_broker_leases owner={.now=live_now,.remove=live_remove,.context=&context};
    struct hub_public_network_lease lease={0};
    struct hub_public_dhcp_event offer={.kind=HUB_DHCP_BOUND,.address=spec.address,.mask=spec.mask,
        .broadcast=spec.broadcast,.lease_seconds=60};uint64_t now=live_now(NULL);
    assert(!hub_public_lease_offer(&lease,&offer,now,now));
    struct hub_public_acd proof={.state=HUB_ACD_CLAIMING,.pending=HUB_ACD_CLAIM};memcpy(proof.address,&spec.address.s_addr,4);
    assert(!hub_public_lease_configured(&lease,lease.generation,now,&proof));
    proof.state=HUB_ACD_ANNOUNCING;proof.pending=HUB_ACD_ANNOUNCE;proof.announcements=1;
    assert(!hub_public_lease_activate(&lease,lease.generation,now,&proof));
    assert(!hub_public_broker_lease_prepare(&owner,0,&lease,&spec));close(spec.namespace_fd);
    assert(!hub_public_source_manager_init(manager,(struct hub_public_source_manager_ops){hub_public_broker_lease_open,hub_public_broker_lease_close},&owner));
    file=open(root,O_RDONLY|O_DIRECTORY|O_CLOEXEC);assert(file>=0);
    assert(!hub_public_source_manager_request_fd(manager,0,file,1));close(file);
    uint64_t deadline=live_now(NULL)+10000;
    while (!manager->sources[0].worker.started || hub_public_source_worker_state(&manager->sources[0].worker)!=HUB_PUBLIC_WORKER_RUNNING) {
        assert(!hub_public_source_manager_poll(manager,0) && !manager->sources[0].error);
        assert(live_now(NULL)<deadline);poll(NULL,0,1);
    }
    assert(manager->sources[0].bound && lease.configured);
    assert(!hub_public_source_manager_withdraw(manager,0));
    assert(!hub_public_source_manager_request(manager,0,NULL,0,0));
    while (!hub_public_source_manager_drained(manager,0)) {
        assert(!hub_public_source_manager_poll(manager,0));assert(live_now(NULL)<deadline);poll(NULL,0,1);
    }
    int result;while ((result=hub_public_broker_lease_release(&owner,0))==1) { assert(live_now(NULL)<deadline);poll(NULL,0,1); }
    if (result) fprintf(stderr,"retirement failed: %s\n",strerror(errno));
    assert(!result && lease.state==HUB_LEASE_IDLE && !lease.configured && context.removal.state==2);
    assert(!owner.slots[0].prepared && !context.removal.operation.started);
    free(manager);assert(!unlink(pdb) && !rmdir(rb) && !rmdir(pioneer) && !rmdir(root));
    puts("PASS: kernel-acknowledged exact IPv4 removal after live source staging, endpoint closure and lease retirement");return 0;
}
int main(int argc,char **argv) {
    if (argc>1) return live(argc,argv);
    struct hub_public_network_spec spec={.namespace_fd=open("/proc/self/ns/net",O_RDONLY|O_CLOEXEC),
        .interface="azpub0",.mac={2,0,0,0,0,3}};assert(spec.namespace_fd>=0);
    inet_pton(AF_INET,"192.0.2.3",&spec.address);inet_pton(AF_INET,"255.255.255.0",&spec.mask);
    struct hub_public_broker_address_retirement retirement={0};
    assert(hub_public_broker_address_retire(&retirement,&spec)<0 && errno==EPERM);
    assert(retirement.state==3 && !retirement.operation.started);
    assert(hub_public_broker_address_retire(&retirement,&spec)<0 && errno==EPERM);
    spec.address.s_addr^=1;assert(hub_public_broker_address_retire(&retirement,&spec)<0 && errno==ESTALE);
    assert(!hub_public_broker_address_retirement_reset(&retirement));
    retirement.operation.started=1;assert(hub_public_broker_address_retirement_reset(&retirement)<0 && errno==EBUSY);
    retirement.operation.started=0;assert(!hub_public_broker_address_retirement_reset(&retirement));close(spec.namespace_fd);
    puts("PASS: retirement failure remains terminal, spec replacement rejected and live operation cannot reset; no network mutation");
}
