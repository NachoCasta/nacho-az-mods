/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <sys/utsname.h>
#include <time.h>
#include "public-dhcp-acquire.h"
#include "public-dhcp-child.h"
#include "public-network-lease.h"
static uint64_t now_ms(void) {
    struct timespec t;assert(!clock_gettime(CLOCK_MONOTONIC,&t));
    return (uint64_t)t.tv_sec*1000+(uint64_t)t.tv_nsec/1000000;
}
static const unsigned char mac[6]={2,0,0,0,0x12,3};
#include "../research/scripts/public-address-flow-probe.h"
static unsigned descriptors(void) {
    DIR *directory=opendir("/proc/self/fd");assert(directory);unsigned count=0;struct dirent *entry;
    while ((entry=readdir(directory))) if (strcmp(entry->d_name,".") && strcmp(entry->d_name,"..")) ++count;
    closedir(directory);return count;
}
static int finish(struct hub_public_dhcp_acquire *a) {
    uint64_t end=now_ms()+20000;
    for (;;) {
        int rc=hub_public_dhcp_acquire_poll(a,now_ms());if (rc) return rc;
        assert(now_ms()<end);
        struct timespec pause={0,1000000};nanosleep(&pause,NULL);
    }
}
static void script(const char *path,const char *body) {
    int fd=open(path,O_CREAT|O_TRUNC|O_WRONLY|O_CLOEXEC,0700);assert(fd>=0);
    const char *prefix="#!/bin/sh\n";
    assert(write(fd,prefix,strlen(prefix))==(ssize_t)strlen(prefix));
    assert(write(fd,body,strlen(body))==(ssize_t)strlen(body));assert(!close(fd));
}
int main(int argc,char **argv) {
    if (argc>1 && !strcmp(argv[1],"--dhcp-child")) return hub_public_dhcp_child(argc,argv);
    int no_offer=argc==5 && !strcmp(argv[1],"--emulator-no-offer");
    if (((argc==4 || argc==5 || argc==6) && !strcmp(argv[1],"--emulator")) || no_offer) {
        int flow=argc==6,retry=flow && !strcmp(argv[5],"--address-retry");
        int library=flow && !strcmp(argv[5],"--address-library");
        int conflict=flow && (retry || !strcmp(argv[5],"--address-conflict"));
        if (flow) assert(conflict || library || !strcmp(argv[5],"--address-flow"));
        struct utsname system;assert(!uname(&system) && !strcmp(system.release,"6.6.138-dirty"));
        assert(!strncmp(argv[2],"/var/run/netns/azhub-dhcp-",26));
        int space=open(argv[2],O_RDONLY|O_CLOEXEC);assert(space>=0);
        struct stat before,after;assert(!stat("/proc/self/ns/net",&before));
        struct hub_public_dhcp_acquire a={0};
        struct hub_public_acd_retry attempts={0};
        if (retry) assert(!hub_public_acd_retry_begin(&attempts,now_ms()));
        assert(!hub_public_dhcp_acquire_begin(&a,argc>=5 ? argv[4] : "/proc/self/exe",space,"azpub0",argv[3],mac,now_ms()));
        int rc=finish(&a);
        if (no_offer) {
            assert(rc==-1 && hub_public_dhcp_acquire_no_offer(&a));
            assert(!stat("/proc/self/ns/net",&after) && before.st_ino==after.st_ino && before.st_dev==after.st_dev);
            close(space);
            puts("{\"native_no_offer\":true,\"deconfig_then_leasefail\":true,\"normal_exit\":1,\"child_reaped\":true,\"pipe_EOF\":true,\"address_applied\":false}");
            return 0;
        }
        assert(rc==1 && a.complete && a.reaped && a.eof && a.fd==-1);
        assert(!stat("/proc/self/ns/net",&after) && before.st_ino==after.st_ino && before.st_dev==after.st_dev);
        struct hub_public_network_lease lease={0};
        assert(!hub_public_lease_offer(&lease,&a.proposal,a.started,now_ms()));
        assert(!hub_public_lease_ready(&lease,now_ms()));
        if (flow) assert(!hub_public_address_flow_probe(space,lease,conflict,library));
        uint64_t retry_wait=0;
        if (retry) {
            uint64_t declined=now_ms(),old_generation=lease.generation;
            struct in_addr rejected=lease.current.address;
            assert(!hub_public_acd_retry_declined(&attempts,declined));
            hub_public_lease_retire(&lease);
            assert(!hub_public_lease_drained(&lease,lease.generation,now_ms()));
            assert(!hub_public_lease_removed(&lease,lease.generation,now_ms()));
            assert(lease.state==HUB_LEASE_IDLE);
            while (hub_public_acd_retry_begin(&attempts,now_ms())<0) {
                assert(!attempts.exhausted && now_ms()<declined+12000);
                struct timespec pause={0,5000000};nanosleep(&pause,NULL);
            }
            assert(!hub_public_dhcp_acquire_begin(&a,argv[4],space,"azpub0",argv[3],mac,now_ms()));
            retry_wait=a.started-declined;assert(retry_wait>=10000);
            assert(finish(&a)==1 && a.proposal.address.s_addr!=rejected.s_addr);
            assert(!hub_public_lease_offer(&lease,&a.proposal,a.started,now_ms()));
            assert(lease.generation==old_generation+1 && !hub_public_lease_ready(&lease,now_ms()));
            assert(!hub_public_address_flow_probe(space,lease,0,0));
        }
        close(space);
        assert(!stat("/proc/self/ns/net",&after) && before.st_ino==after.st_ino && before.st_dev==after.st_dev);
        char address[INET_ADDRSTRLEN],server[INET_ADDRSTRLEN];
        assert(inet_ntop(AF_INET,&a.proposal.address,address,sizeof address));
        assert(inet_ntop(AF_INET,&a.proposal.server,server,sizeof server));
        printf("{\"supervised_grant\":true,\"address\":\"%s\",\"server\":\"%s\",\"lease_seconds\":%u,\"started_ms\":%llu,\"completed_ms\":%llu,\"expires_ms\":%llu,\"lease_candidate_not_ready\":true,\"address_flow_applied_then_removed\":%s,\"arp_conflict_before_configuration\":%s,\"reacquired_after_decline\":%s,\"post_decline_wait_ms\":%llu,\"address_library_served\":%s}\n",
            address,server,a.proposal.lease_seconds,(unsigned long long)a.started,(unsigned long long)a.last,
            (unsigned long long)lease.expires,flow && (!conflict || retry) ? "true":"false",
            conflict ? "true":"false",retry ? "true":"false",(unsigned long long)retry_wait,library ? "true":"false");
        return 0;
    }
    assert(argc==1);
    char directory[]="/tmp/azhub-dhcp-acquire.XXXXXX";assert(mkdtemp(directory));
    char path[128];snprintf(path,sizeof path,"%s/helper",directory);
    int space=open("/proc/self/ns/net",O_RDONLY|O_CLOEXEC);assert(space>=0);
    unsigned baseline=descriptors();
    const char *good="printf '1\\tdeconfig\\tazpub0\\t-\\t-\\t-\\t-\\n1\\tbound\\tazpub0\\t192.0.2.3\\t255.255.255.0\\t60\\t192.0.2.2\\n' >&3\n";
    struct hub_public_dhcp_acquire a={0};
    script(path,good);
    assert(!hub_public_dhcp_acquire_begin(&a,path,space,"azpub0","/unused",mac,now_ms()));
    assert(hub_public_dhcp_acquire_begin(&a,path,space,"azpub0","/unused",mac,now_ms())<0 && errno==EBUSY);
    assert(finish(&a)==1 && a.records==2 && a.proposal.lease_seconds==60 && a.fd==-1);
    const char *no_offer_body="printf '1\\tdeconfig\\tazpub0\\t-\\t-\\t-\\t-\\n1\\tleasefail\\tazpub0\\t-\\t-\\t-\\t-\\n' >&3\nexit 1\n";
    script(path,no_offer_body);
    assert(!hub_public_dhcp_acquire_begin(&a,path,space,"azpub0","/unused",mac,now_ms()));
    assert(!hub_public_dhcp_acquire_no_offer(&a));
    assert(finish(&a)==-1 && a.error==ENODATA && hub_public_dhcp_acquire_no_offer(&a));
    const char *bad[]={"exit 0\n","exit 1\n","printf 'garbage\\n' >&3\n",
        "printf '1\\tdeconfig\\tazpub0\\t-\\t-\\t-\\t-\\n' >&3\nexit 1\n",
        "printf '1\\tleasefail\\tazpub0\\t-\\t-\\t-\\t-\\n' >&3\nexit 1\n",
        "printf '1\\tdeconfig\\tazpub0\\t-\\t-\\t-\\t-\\n1\\tleasefail\\tazpub0\\t-\\t-\\t-\\t-\\n' >&3\nexit 0\n",
        "printf '1\\tbound\\tazpub9\\t192.0.2.3\\t255.255.255.0\\t60\\t192.0.2.2\\n' >&3\n",
        "printf '1\\tbound\\tazpub0\\t192.0.2.3\\t255.255.255.0\\t60\\t192.0.2.2\\n' >&3\nexit 1\n",
        "printf 'unfinished' >&3\n",
        "i=0; while [ $i -lt 250 ]; do printf x >&3; i=$((i+1)); done\n"};
    for (unsigned i=0;i<sizeof bad/sizeof *bad;++i) {
        script(path,bad[i]);
        assert(!hub_public_dhcp_acquire_begin(&a,path,space,"azpub0","/unused",mac,now_ms()));
        assert(finish(&a)==-1 && a.error && a.complete && a.reaped && a.fd==-1);
        assert(!hub_public_dhcp_acquire_no_offer(&a));
    }
    script(path,"while :; do :; done\n");
    assert(!hub_public_dhcp_acquire_begin(&a,path,space,"azpub0","/unused",mac,now_ms()));
    hub_public_dhcp_acquire_cancel(&a,ECANCELED,now_ms());
    assert(finish(&a)==-1 && a.error==ECANCELED);
    script(path,good);
    assert(!hub_public_dhcp_acquire_begin(&a,path,space,"azpub0","/unused",mac,now_ms()));
    assert(hub_public_dhcp_acquire_poll(&a,a.started+15000)==0 || a.complete);
    /* Continue on that same synthetic clock until the canceled child is reaped. */
    for (unsigned i=0;!a.complete && i<5000;++i) {
        hub_public_dhcp_acquire_poll(&a,a.started+15000+i);
        struct timespec pause={0,1000000};nanosleep(&pause,NULL);
    }
    assert(a.complete && a.error==ETIMEDOUT && a.fd==-1);
    struct sigaction saved,ignored={.sa_handler=SIG_IGN};sigemptyset(&ignored.sa_mask);
    assert(!sigaction(SIGCHLD,&ignored,&saved));
    assert(hub_public_dhcp_acquire_begin(&a,path,space,"azpub0","/unused",mac,now_ms())<0 && errno==ENOTSUP);
    assert(!sigaction(SIGCHLD,&saved,NULL));
    assert(descriptors()==baseline);
    close(space);assert(!unlink(path));assert(!rmdir(directory));
    puts("PASS: supervised one-shot DHCP records, clean spawn, malformed/nonzero/partial rejection, cancellation, deadline and child reap");
    return 0;
}
