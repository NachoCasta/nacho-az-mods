/* SPDX-License-Identifier: GPL-3.0-only */
/* Shared managed allocator runtime. Emulator wrappers retain their exact kernel,
 * fixture admission and deadline. The separately built candidate wrapper uses
 * dynamic admission, native-subnet peer policy and the exact target kernel. */
#ifndef HUB_PUBLIC_BROKER_ALLOCATE_EMULATOR_H
#define HUB_PUBLIC_BROKER_ALLOCATE_EMULATOR_H
#include <sys/utsname.h>
#include "public-broker-probe-child.h"
#include "public-broker-session.h"
#include "public-broker-allocator.h"
#include "public-broker-emulator-policy.h"
#include "public-broker-root-policy.h"
#include "public-peer-network.h"
#include "public-acd-jitter.h"
#include "public-parent-rx.h"
#ifdef LF_PUBLIC_BROKER_CANDIDATE
#include "public-macvlan-asset.h"
#endif
#ifdef LF_PUBLIC_PARENT_RX_PROBE
#include "public-parent-rx-probe.h"
#endif
static int broker_candidate_entropy=-1;
static unsigned broker_candidate_jitter(void *unused) { (void)unused;return hub_public_acd_jitter(broker_candidate_entropy); }
#if defined(LF_PUBLIC_DYNAMIC_POLICY_EMULATOR) || defined(LF_PUBLIC_BROKER_CANDIDATE) || defined(LF_PUBLIC_BROKER_CANDIDATE_EMULATOR)
#include <sys/ioctl.h>
#include "public-broker-policy.h"
#endif
static uint64_t broker_allocate_emulator_now(void *unused) { (void)unused;return hub_public_broker_ms(); }
static unsigned broker_allocate_emulator_jitter(void *unused) { (void)unused;return 500; }
static int broker_allocate_emulator_authorize(void *unused,unsigned slot,const struct hub_public_broker_source *source,int root) {
#if defined(LF_PUBLIC_DYNAMIC_POLICY_EMULATOR) || defined(LF_PUBLIC_BROKER_CANDIDATE) || defined(LF_PUBLIC_BROKER_CANDIDATE_EMULATOR)
    struct hub_public_broker_allocator *allocator=unused;
    if (!allocator) { errno=EINVAL;return -1; }
    return hub_public_broker_policy(slot,source,root,allocator->parent_mac);
#else
    (void)unused;
    if (slot>=8 || source->player<41 || source->player>48) { errno=EPERM;return -1; }
    struct hub_public_broker_source fixed=*source;fixed.player=41+slot;
    if (hub_public_broker_emulator_policy(slot,&fixed)<0) return -1;
    if (strcmp(source->directory,"/tmp/azhub-sdk-library") &&
            hub_public_broker_root_policy(source,root,"/media/usb","/proc/mounts","/sys/class/block")<0) return -1;
    int verified=hub_public_broker_source_open(source);if (verified<0) return -1;
    struct stat actual,expected;
    int okay=!fstat(root,&actual) && !fstat(verified,&expected) &&
        actual.st_dev==expected.st_dev && actual.st_ino==expected.st_ino;
    close(verified);if (!okay) { errno=ESTALE;return -1; }return 0;
#endif
}
static int broker_allocate_emulator_peer(void *unused,const struct sockaddr_in *peer) { (void)unused;return peer->sin_addr.s_addr==inet_addr("192.0.2.2"); }
static int broker_allocate_emulator_poll(void *opaque,struct hub_public_broker_dispatch *dispatch,unsigned slot) {
    struct hub_public_broker_allocator *allocator=opaque;
    static uint64_t reported[8];
    static uint64_t refresh_generation[8];static unsigned refresh_count[8];
    static uint64_t link_local_generation[8];
    int result=hub_public_broker_dispatch_poll(dispatch,slot);
    if (allocator->sources[slot].phase==2 && allocator->sources[slot].lease.origin==HUB_ADDRESS_LINK_LOCAL &&
            link_local_generation[slot]!=dispatch->generations.slots[slot].generation) {
        char address[INET_ADDRSTRLEN];link_local_generation[slot]=dispatch->generations.slots[slot].generation;
        if (inet_ntop(AF_INET,&allocator->sources[slot].spec.address,address,sizeof address))
            fprintf(stderr,"allocate link-local candidate slot=%u generation=%llu address=%s\n",slot,
                (unsigned long long)link_local_generation[slot],address);
    }
    if (allocator->last_acquire_error[slot] && reported[slot]!=dispatch->generations.slots[slot].generation) {
        reported[slot]=dispatch->generations.slots[slot].generation;
        fprintf(stderr,"allocate acquisition failed slot=%u generation=%llu phase=%d errno=%d\n",slot,
            (unsigned long long)reported[slot],allocator->last_acquire_phase[slot],allocator->last_acquire_error[slot]);
    }
    if (allocator->refreshes[slot] && (refresh_generation[slot]!=dispatch->generations.slots[slot].generation ||
            refresh_count[slot]!=allocator->refreshes[slot])) {
        refresh_generation[slot]=dispatch->generations.slots[slot].generation;refresh_count[slot]=allocator->refreshes[slot];
        fprintf(stderr,"allocate refresh accepted slot=%u generation=%llu lease_generation=%llu count=%u expires=%llu\n",slot,
            (unsigned long long)refresh_generation[slot],(unsigned long long)allocator->sources[slot].lease.generation,
            refresh_count[slot],(unsigned long long)allocator->sources[slot].lease.expires);
    }
    return result;
}
static int hub_public_broker_allocate_run(int argc,char **argv,const char *mode,const char *expected_kernel,int candidate) {
    struct utsname kernel;if (uname(&kernel) || strcmp(kernel.release,expected_kernel)) return 126;
    if (argc!=5 || argv[0][0]!='/' || strlen(argv[0])>=1024) return 126;
    if (candidate) {
        broker_candidate_entropy=open("/dev/urandom",O_RDONLY|O_CLOEXEC);
        if (broker_candidate_entropy<0) return 126;
    }
    char hook[1024];strcpy(hook,argv[0]);char *name=strrchr(hook,'/');if (!name) return 126;
    /* Executable asset suffix follows the pinned SDK package mode rule. */
    if ((size_t)(name-hook)+sizeof "/public-dhcp-event-hook.so">sizeof hook) return 126;
    strcpy(name,"/public-dhcp-event-hook.so");if (access(hook,X_OK)) return 126;
    unsigned char token[16];int fd=hub_public_broker_connect(argc,argv,mode,token);if (fd<0) return 126;
#ifdef LF_PUBLIC_BROKER_CANDIDATE
    if (hub_public_macvlan_asset()<0) {
        fprintf(stderr,"public broker MACVLAN asset refused errno=%d\n",errno);
        close(fd);return 126;
    }
#endif
    struct hub_public_source_manager *manager=calloc(1,sizeof *manager);
    struct hub_public_broker_allocator *allocator=calloc(1,sizeof *allocator);
    if (!manager || !allocator) { close(fd);free(manager);free(allocator);return 126; }
    allocator->manager=manager;strcpy(allocator->parent,"eth0");strcpy(allocator->helper,argv[0]);strcpy(allocator->hook,hook);
    const unsigned char parent[6]={0x52,0x54,0,0x12,0x34,0x56};memcpy(allocator->parent_mac,parent,6);
#if defined(LF_PUBLIC_DYNAMIC_POLICY_EMULATOR) || defined(LF_PUBLIC_BROKER_CANDIDATE) || defined(LF_PUBLIC_BROKER_CANDIDATE_EMULATOR)
    struct ifreq interface={0};strcpy(interface.ifr_name,allocator->parent);
    int control=socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);
    if (control<0 || ioctl(control,SIOCGIFHWADDR,&interface)<0 || interface.ifr_hwaddr.sa_family!=ARPHRD_ETHER) {
        if (control>=0) close(control);
        close(fd);free(manager);free(allocator);return 126;
    }
    memcpy(allocator->parent_mac,interface.ifr_hwaddr.sa_data,6);close(control);allocator->context=allocator;
#endif
    allocator->now=broker_allocate_emulator_now;allocator->jitter=broker_allocate_emulator_jitter;
    allocator->authorize=broker_allocate_emulator_authorize;allocator->allowed=broker_allocate_emulator_peer;
    struct hub_public_peer_network peers={0};
    if (candidate) {
        if (hub_public_peer_network_snapshot(&peers,allocator->parent)<0) { fprintf(stderr,"public broker startup native IPv4 snapshot failed interface=%s errno=%d\n",allocator->parent,errno);close(fd);free(manager);free(allocator);return 126; }
        allocator->allowed=hub_public_peer_network_allowed;allocator->peer_context=&peers;
        allocator->jitter=broker_candidate_jitter;
        allocator->allow_link_local=hub_public_peer_network_link_local(&peers);
        allocator->entropy_fd=broker_candidate_entropy;
    }
    struct hub_public_broker_dispatch dispatch;
    if (hub_public_source_manager_init(manager,(struct hub_public_source_manager_ops){hub_public_broker_allocator_open,hub_public_broker_allocator_close},allocator)<0 ||
            hub_public_broker_dispatch_init(&dispatch,manager,(struct hub_public_broker_dispatch_ops){.allocate=hub_public_broker_allocator_allocate,
                .ready=hub_public_broker_allocator_ready,.release=hub_public_broker_allocator_release},allocator)<0) {
        close(fd);free(manager);free(allocator);return 126;
    }
    struct ucred parent_peer;socklen_t size=sizeof parent_peer;
    if (getsockopt(fd,SOL_SOCKET,SO_PEERCRED,&parent_peer,&size)<0) { close(fd);free(manager);free(allocator);return 126; }
    int parent_rx=-1;
#ifndef LF_PUBLIC_PARENT_RX_PROBE
    if(candidate) {
        parent_rx=hub_public_parent_rx_membership(allocator->parent);
        if(parent_rx<0) {
            fprintf(stderr,"public broker parent RX membership refused interface=%s errno=%d\n",allocator->parent,errno);
            close(fd);free(manager);free(allocator);return 126;
        }
        fprintf(stderr,"public broker parent RX membership acquired interface=%s\n",allocator->parent);
    }
#endif
    struct hub_public_broker_session session;
    if (hub_public_broker_session_init(&session,fd,parent_peer.pid,geteuid(),token,&dispatch,broker_allocate_emulator_poll,allocator)<0) { if(parent_rx>=0) close(parent_rx);close(fd);free(manager);free(allocator);return 126; }
    close(fd);uint64_t deadline=candidate ? 0:hub_public_broker_ms()+600000,next_network_check=0;int failed=0,network_stopping=0;
#ifdef LF_PUBLIC_PARENT_RX_PROBE
    struct hub_public_parent_rx_probe rx_probe={.fd=-1};
#endif
    for (;;) {
        uint64_t now=hub_public_broker_ms();
#ifdef LF_PUBLIC_PARENT_RX_PROBE
        int library_ready=0;
        for(unsigned i=0;i<8;++i)
            if(__atomic_load_n(&manager->sources[i].worker.state,__ATOMIC_ACQUIRE)==HUB_PUBLIC_WORKER_RUNNING) library_ready=1;
        hub_public_parent_rx_probe_poll(&rx_probe,now,library_ready,allocator->parent,hub_public_parent_rx_membership,close);
#endif
        if (candidate && !network_stopping && !session.lost && now>=next_network_check) {
            struct hub_public_peer_network observed;
            next_network_check=now+1000;
            if (hub_public_peer_network_snapshot(&observed,allocator->parent)<0 || !hub_public_peer_network_same(&peers,&observed)) {
                fprintf(stderr,"public broker native network changed; retiring owned sources\n");
                failed=network_stopping=1;
                struct hub_public_broker_frame stop={.op=HUB_BROKER_STOP};memcpy(stop.token,token,16);
                if (hub_public_broker_dispatch_command(&dispatch,&stop)<0) hub_public_broker_session_loss(&session);
            }
        }
        if (deadline && hub_public_broker_ms()>=deadline && !session.lost) hub_public_broker_session_loss(&session);
        int result=hub_public_broker_session_poll(&session);if (result==1) break;
        if (result<0) { failed=1;hub_public_broker_session_loss(&session); }
        poll(NULL,0,1);
    }
#ifdef LF_PUBLIC_PARENT_RX_PROBE
    if(rx_probe.fd>=0) close(rx_probe.fd);
#endif
    if(parent_rx>=0) {
        close(parent_rx);
        fprintf(stderr,"public broker parent RX membership released\n");
    }
    if (candidate && broker_candidate_entropy>=0) { close(broker_candidate_entropy);broker_candidate_entropy=-1; }
    free(manager);free(allocator);return failed ? 1:0;
}
static int hub_public_broker_allocate_emulator(int argc,char **argv) {
    return hub_public_broker_allocate_run(argc,argv,"--broker-allocate-emulator","6.6.138-dirty",0);
}
#ifdef LF_PUBLIC_BROKER_CANDIDATE
static int hub_public_broker_candidate(int argc,char **argv) {
    return hub_public_broker_allocate_run(argc,argv,"--broker-share","4.4.194",1);
}
#endif
#ifdef LF_PUBLIC_BROKER_CANDIDATE_EMULATOR
static int hub_public_broker_candidate_emulator(int argc,char **argv) {
    return hub_public_broker_allocate_run(argc,argv,"--broker-candidate-emulator","6.6.138-dirty",1);
}
#endif
#endif
