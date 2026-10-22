/* SPDX-License-Identifier: GPL-3.0-only */
/* SDK-owned candidate controller. No fixture masks, completion files or service
 * deadlines. The injector owns polling and native eject calls drain on that
 * same thread. A failed live channel retains every outstanding unmount gate. */
#ifndef HUB_PUBLIC_SHARING_RUNTIME_H
#define HUB_PUBLIC_SHARING_RUNTIME_H
#include <sys/ioctl.h>
#include <net/if.h>
#include "public-broker-probe-child.h"
#include "public-sharing-inventory.h"
#include "public-helper-logs.h"
static struct {
    struct hub_public_sharing_inventory inventory;
    int phase,listener,peer,executable,child,initialized,stop_sent,stop_requested;
    uint64_t deadline,retry_at;
    unsigned char token[16],parent[6];
    char hex[33],directory[96],socket[108],helper[1024],log[1024];
} public_runtime={.listener=-1,.peer=-1,.executable=-1};
/* Written only from confirmed native mode completion, never the Link cache. */
static int public_runtime_mode=-1;
static void public_runtime_set_mode(unsigned mode) {
    __atomic_store_n(&public_runtime_mode,mode<=1 ? (int)mode:-1,__ATOMIC_RELEASE);
}
enum { PUBLIC_RUNTIME_IDLE, PUBLIC_RUNTIME_ACCEPT, PUBLIC_RUNTIME_HELLO,
    PUBLIC_RUNTIME_ACTIVE, PUBLIC_RUNTIME_EXIT, PUBLIC_RUNTIME_FAULT };
static void public_runtime_cleanup(void) {
    int *fds[]={&public_runtime.listener,&public_runtime.peer,&public_runtime.executable};
    for (unsigned i=0;i<sizeof fds/sizeof *fds;++i) if (*fds[i]>=0) { close(*fds[i]);*fds[i]=-1; }
    if (*public_runtime.socket) unlink(public_runtime.socket);
    if (*public_runtime.directory) rmdir(public_runtime.directory);
    public_runtime.socket[0]=public_runtime.directory[0]=0;
    if (public_runtime.initialized) hub_public_broker_client_close(&public_runtime.inventory.client);
    public_runtime.initialized=0;
}
static int public_runtime_prepare(void) {
    const char *assets=lf_core->assets_dir("usb-hub-support"),*data=lf_core->data_dir("usb-hub-support");
    if (!assets || !data) return -1;
    int random=open("/dev/urandom",O_RDONLY|O_CLOEXEC);
    if (random<0) return -1;
    size_t used=0;
    while (used<sizeof public_runtime.token) {
        ssize_t got=read(random,public_runtime.token+used,sizeof public_runtime.token-used);
        if (got<0 && errno==EINTR) continue;
        if (got<=0) { close(random);return -1; }
        used+=(size_t)got;
    }
    close(random);
    for (unsigned i=0;i<16;++i) snprintf(public_runtime.hex+2*i,3,"%02x",public_runtime.token[i]);
    struct ifreq interface={0};strcpy(interface.ifr_name,"eth0");
    int control=socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);
    if (control<0) return -1;
    int got=ioctl(control,SIOCGIFHWADDR,&interface);close(control);
    if (got<0) return -1;
    memcpy(public_runtime.parent,interface.ifr_hwaddr.sa_data,6);
    if (hub_public_sharing_inventory_init(&public_runtime.inventory,public_runtime.token,public_runtime.parent)<0) return -1;
    public_runtime.initialized=1;public_runtime.stop_sent=public_runtime.stop_requested=0;
    /* The pinned packer preserves executable permissions for .so members. */
    int length=snprintf(public_runtime.helper,sizeof public_runtime.helper,"%s/usb-hub-support-network-helper.so",assets);
    if (length<0 || length>=(int)sizeof public_runtime.helper) goto failed;
    length=snprintf(public_runtime.log,sizeof public_runtime.log,"%s/public-sharing-%s.log",data,public_runtime.hex);
    if (length<0 || length>=(int)sizeof public_runtime.log) goto failed;
    public_runtime.executable=open(public_runtime.helper,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    struct stat executable;
    if (public_runtime.executable<0 || fstat(public_runtime.executable,&executable) ||
            !S_ISREG(executable.st_mode) || access(public_runtime.helper,X_OK)) goto failed;
    if (hub_public_helper_logs_prepare(data)<0) goto failed;
    snprintf(public_runtime.directory,sizeof public_runtime.directory,"/run/azhub-share-%ld-%.16s",(long)getpid(),public_runtime.hex);
    if (mkdir(public_runtime.directory,0700)) { public_runtime.directory[0]=0;goto failed; }
    snprintf(public_runtime.socket,sizeof public_runtime.socket,"%s/control.sock",public_runtime.directory);
    public_runtime.listener=socket(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK|SOCK_CLOEXEC,0);
    if (public_runtime.listener<0) goto failed;
    struct sockaddr_un address={.sun_family=AF_UNIX};strcpy(address.sun_path,public_runtime.socket);
    if (bind(public_runtime.listener,(struct sockaddr *)&address,offsetof(struct sockaddr_un,sun_path)+strlen(address.sun_path)+1) ||
            chmod(public_runtime.socket,0600) || listen(public_runtime.listener,1)) goto failed;
    return 0;
failed:
    public_runtime_cleanup();return -1;
}
static void public_runtime_fail(const char *reason) {
    lf_log("public-sharing controller fault phase=%d reason=%s errno=%d",public_runtime.phase,reason,errno);
    /* Never turn a broken channel or process liveness observation into EMPTY.
     * Recovery of a session with outstanding generations needs explicit proof. */
    if (public_runtime.child>0) lf_core->launch_stop("usb-hub-support",public_runtime.child);
    int outstanding=0;
    for (unsigned slot=0;slot<8;++slot) if (public_runtime.initialized &&
            !hub_public_broker_client_drained(&public_runtime.inventory.client,slot)) outstanding=1;
    public_runtime.phase=outstanding ? PUBLIC_RUNTIME_FAULT:PUBLIC_RUNTIME_EXIT;
}
static int public_runtime_drain(unsigned slot) {
    if (slot>=8 || !public_runtime.initialized) return 1;
    struct hub_public_broker_client *client=&public_runtime.inventory.client;
    if (!client->slots[slot].generation || hub_public_broker_client_drained(client,slot)) return 1;
    if (public_runtime.phase!=PUBLIC_RUNTIME_ACTIVE || client->lost) return 0;
    if (hub_public_broker_inventory_withdraw(&public_runtime.inventory.observed,client,slot)<0 ||
            hub_public_broker_client_send(client,public_runtime.peer,slot)<0) return 0;
    return hub_public_broker_client_drained(client,slot);
}
static void public_runtime_poll(void) {
    uint64_t now=hub_public_broker_ms();
    if (!now || public_runtime.phase==PUBLIC_RUNTIME_FAULT) return;
    if (public_runtime.phase==PUBLIC_RUNTIME_IDLE) {
        if (__atomic_load_n(&public_runtime_mode,__ATOMIC_ACQUIRE)!=1 || now<public_runtime.retry_at) return;
        if (public_runtime_prepare()<0) {
            public_runtime_cleanup();public_runtime.retry_at=now+15000;
            lf_log("public-sharing preparation deferred errno=%d",errno);return;
        }
        char parent[24];snprintf(parent,sizeof parent,"%ld",(long)getpid());
#ifdef LF_PUBLIC_RUNTIME_EMULATOR
        const char *mode="--broker-candidate-emulator";
#else
        const char *mode="--broker-share";
#endif
        const char *arguments[]={mode,public_runtime.socket,public_runtime.hex,parent,NULL};
        az_launch_opts options={.size=sizeof options,.log_path=public_runtime.log};
        public_runtime.child=lf_core->launch("usb-hub-support",public_runtime.helper,arguments,&options);
        if (public_runtime.child<0) {
            public_runtime_cleanup();public_runtime.retry_at=now+15000;return;
        }
        public_runtime.phase=PUBLIC_RUNTIME_ACCEPT;public_runtime.deadline=now+15000;
        lf_log("public-sharing managed launch pid=%d",public_runtime.child);return;
    }
    if (public_runtime.phase==PUBLIC_RUNTIME_EXIT) {
        if (lf_core->launch_alive("usb-hub-support",public_runtime.child)) return;
        public_runtime_cleanup();public_runtime.child=0;
        public_runtime.phase=PUBLIC_RUNTIME_IDLE;public_runtime.retry_at=now+15000;
        lf_log("public-sharing graceful retirement complete; restart scheduled");return;
    }
    if (public_runtime.phase==PUBLIC_RUNTIME_ACCEPT || public_runtime.phase==PUBLIC_RUNTIME_HELLO) {
        if (now>=public_runtime.deadline) { public_runtime_fail("startup timeout");return; }
    }
    if (public_runtime.phase==PUBLIC_RUNTIME_ACCEPT) {
        public_runtime.peer=accept4(public_runtime.listener,NULL,NULL,SOCK_NONBLOCK|SOCK_CLOEXEC);
        if (public_runtime.peer<0) {
            if (errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR) public_runtime_fail("accept");
            return;
        }
        if (hub_public_broker_peer(public_runtime.peer,public_runtime.child,geteuid(),public_runtime.executable)<0) {
            public_runtime_fail("helper identity");return;
        }
        public_runtime.phase=PUBLIC_RUNTIME_HELLO;
    }
    if (public_runtime.phase==PUBLIC_RUNTIME_HELLO) {
        struct hub_public_broker_frame frame;
        int received=hub_public_broker_receive_token(public_runtime.peer,public_runtime.token,&frame);
        if (received<0) { public_runtime_fail("HELLO receive");return; }
        if (!received) return;
        int valid=frame.op==HUB_BROKER_HELLO && !frame.slot && !frame.generation && !frame.count && !frame.length && !frame.error;
        hub_public_broker_dispose(&frame);
        if (!valid) { errno=EPROTO;public_runtime_fail("HELLO shape");return; }
        close(public_runtime.listener);public_runtime.listener=-1;
        public_runtime.phase=PUBLIC_RUNTIME_ACTIVE;
        lf_log("public-sharing authenticated helper ready");
    }
    if (public_runtime.phase==PUBLIC_RUNTIME_ACTIVE) {
        if (__atomic_load_n(&public_runtime_mode,__ATOMIC_ACQUIRE)!=1) public_runtime.stop_requested=1;
        if (public_runtime.stop_requested) {
            public_runtime.inventory.quiescing=1;
            if (!public_runtime.stop_sent) {
                struct hub_public_broker_frame stop={.op=HUB_BROKER_STOP};
                memcpy(stop.token,public_runtime.token,16);
                if (!hub_public_broker_send(public_runtime.peer,&stop)) {
                    public_runtime.stop_sent=1;lf_log("public-sharing native Link Off; retiring public sources");
                } else if (errno!=EAGAIN && errno!=EWOULDBLOCK && errno!=EINTR) {
                    public_runtime_fail("mode STOP");return;
                }
            }
        }
        struct hub_public_sharing_view views[8]={0};
        uint64_t generation[8],retired[8];int phase[8];
        for (unsigned slot=0;slot<8;++slot) {
            retired[slot]=public_runtime.inventory.retired_generation[slot];
            generation[slot]=public_runtime.inventory.client.slots[slot].generation;
            phase[slot]=public_runtime.inventory.client.slots[slot].phase;
            if (slot<(unsigned)nfakes) views[slot]=(struct hub_public_sharing_view){
                fakes[slot].dir,fakes[slot].present && !fakes[slot].unmount_pending,fakes[slot].eject_reserved};
        }
        int result=hub_public_sharing_inventory_poll(&public_runtime.inventory,public_runtime.peer,views,now);
        if (result<0) { public_runtime_fail("inventory");return; }
        for (unsigned slot=0;slot<8;++slot) {
            struct hub_public_broker_client_slot *live=public_runtime.inventory.client.slots+slot;
            if (retired[slot]!=public_runtime.inventory.retired_generation[slot])
                lf_log("public-sharing acknowledged retirement source=%u generation=%llu error=%u",slot,
                    (unsigned long long)public_runtime.inventory.retired_generation[slot],public_runtime.inventory.retired_error[slot]);
            if (generation[slot]!=live->generation)
                lf_log("public-sharing inventory source=%u root=%s",slot,live->desired.directory);
            if (generation[slot]!=live->generation || phase[slot]!=live->phase)
                lf_log("public-sharing source=%u generation=%llu phase=%u error=%u",slot,
                    (unsigned long long)live->generation,live->phase,live->error);
        }
        if (result==1) public_runtime.phase=PUBLIC_RUNTIME_EXIT;
    }
}
#endif
