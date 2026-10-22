/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicit emulator-only SDK control on HubTransport after native readiness.
 * Optional service/ALLOCATE modes export a runner-selected library through the
 * managed helper. EP147 never enters namespaces or reaps child processes. */
#ifndef HUB_PUBLIC_SDK_PROBE_H
#define HUB_PUBLIC_SDK_PROBE_H
#include <sys/utsname.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "public-source-mac.h"
static int public_sdk_dynamic_identity,public_sdk_native_inventory;
static unsigned char public_sdk_parent_mac[6];
#include "public-broker-probe-child.h"
#include "public-broker-source.h"
#include "public-broker-client.h"
#include "public-broker-inventory.h"
#include "public-player-reconcile.h"
static struct hub_public_player_reconcile public_sdk_player_state;
static struct hub_public_player_pool public_sdk_players;
static int public_sdk_player_recovery;
static struct hub_public_broker_inventory public_sdk_observations;
#include "public-sharing-inventory.h"
static struct hub_public_sharing_inventory public_sdk_engine;
static struct hub_public_broker_client public_sdk_legacy_client;
static int public_sdk_use_engine,public_sdk_candidate_runtime,public_sdk_recover_network,public_sdk_restart_pending;
static unsigned public_sdk_attempt;
static uint64_t public_sdk_restart_at;
#define public_sdk_client (*(public_sdk_use_engine ? &public_sdk_engine.client:&public_sdk_legacy_client))
static int public_sdk_client_initialized;
static unsigned public_sdk_inventory_mask=1;
static int public_sdk_native_slots[8]={-1,-1,-1,-1,-1,-1,-1,-1};
static int public_sdk_multi_ready;
static int public_sdk_inventory,public_sdk_inventory_slot=-1,public_sdk_inventory_retired,public_sdk_inventory_reported;
static struct {
    int enabled,stage,listener,peer,source_fd,executable,child,peer_pid,service,hold,allocate,cancel_dhcp,expect_conflict,conflict_seen,retry_conflict,expect_reject;
    uint64_t deadline,inode,generation;
    unsigned char token[16];
    char hex[33],run[33],directory[96],socket[108],helper[1024],helper_log[1024],result[128];
} public_sdk_probe={.listener=-1,.peer=-1,.source_fd=-1,.executable=-1};
static void public_sdk_probe_cleanup(void) {
    int *fds[]={&public_sdk_probe.peer,&public_sdk_probe.listener,&public_sdk_probe.source_fd,&public_sdk_probe.executable};
    for (unsigned i=0;i<sizeof fds/sizeof *fds;++i) if (*fds[i]>=0) { close(*fds[i]);*fds[i]=-1; }
    if (*public_sdk_probe.socket) unlink(public_sdk_probe.socket);
    if (*public_sdk_probe.directory) rmdir(public_sdk_probe.directory);
    if (public_sdk_client_initialized) hub_public_broker_client_close(&public_sdk_client);
    public_sdk_client_initialized=0;
    public_sdk_probe.enabled=0;
}
static void public_sdk_probe_finish(int okay,const char *reason) {
    int phase=public_sdk_probe.stage;
    if (!okay && public_sdk_probe.child>0) lf_core->launch_stop("usb-hub-support",public_sdk_probe.child);
    public_sdk_probe_cleanup();
    int fd=open(public_sdk_probe.result,O_WRONLY|O_CREAT|O_EXCL|O_CLOEXEC|O_NOFOLLOW,0600);
    if (fd>=0) {
        char message[512];int length=snprintf(message,sizeof message,
            "{\"run\":\"%s\",\"success\":%s,\"phase\":%d,\"managed_pid\":%d,\"peer_pid\":%d,\"namespace_inode\":%llu,\"root_inode\":%llu}\n",
            public_sdk_probe.run,okay ? "true":"false",phase,public_sdk_probe.child,public_sdk_probe.peer_pid,
            (unsigned long long)(public_sdk_probe.allocate ? 0:public_sdk_probe.inode),(unsigned long long)(public_sdk_probe.allocate ? public_sdk_probe.inode:0));
        ssize_t written=write(fd,message,(size_t)length);close(fd);
        if (written!=length) okay=0;
    } else okay=0;
    lf_log("sdk-broker-probe %s phase=%d reason=%s result=%s",okay ? "PASS":"FAIL",phase,reason,public_sdk_probe.result);
}
static int public_sdk_probe_prepare(void) {
    const char *enabled=getenv("AZHUB_PUBLIC_SDK_PROBE");if (!enabled) return 0;
    for (unsigned i=0;i<8;++i) public_sdk_client.slots[i].root=-1;
    struct utsname kernel;const char *run=getenv("AZHUB_SDK_PROBE_RUN");
    if (strcmp(enabled,"1") || uname(&kernel) || strcmp(kernel.release,"6.6.138-dirty") || !run || strlen(run)!=32) return -1;
    const char *service=getenv("AZHUB_SDK_SERVICE_PROBE");
    if (service && strcmp(service,"1")) return -1;
    public_sdk_probe.service=service!=NULL;
    const char *allocate=getenv("AZHUB_SDK_ALLOCATE_PROBE");
    if (allocate && (strcmp(allocate,"1") || !service)) return -1;
    public_sdk_probe.allocate=allocate!=NULL;
    const char *cancel=getenv("AZHUB_SDK_CANCEL_DHCP");
    if (cancel && (strcmp(cancel,"1") || !allocate)) return -1;
    public_sdk_probe.cancel_dhcp=cancel!=NULL;
    const char *conflict=getenv("AZHUB_SDK_EXPECT_CONFLICT");
    if (conflict && (strcmp(conflict,"1") || !allocate || cancel)) return -1;
    public_sdk_probe.expect_conflict=conflict!=NULL;
    const char *retry=getenv("AZHUB_SDK_RETRY_CONFLICT");
    if (retry && (strcmp(retry,"1") || !conflict)) return -1;
    public_sdk_probe.retry_conflict=retry!=NULL;
    const char *reject=getenv("AZHUB_SDK_EXPECT_REJECT");
    if (reject && (strcmp(reject,"99") || !allocate || cancel || conflict)) return -1;
    public_sdk_probe.expect_reject=reject ? EADDRNOTAVAIL:0;
    const char *inventory=getenv("AZHUB_SDK_INVENTORY");
    if (inventory && (strcmp(inventory,"1") || !allocate || cancel || conflict)) return -1;
    public_sdk_inventory=inventory!=NULL;
    const char *dynamic=getenv("AZHUB_SDK_DYNAMIC_IDENTITY");
    if (dynamic && (strcmp(dynamic,"1") || !inventory)) return -1;
    public_sdk_dynamic_identity=dynamic!=NULL;
    const char *native_inventory=getenv("AZHUB_SDK_NATIVE_INVENTORY");
    if (native_inventory && (strcmp(native_inventory,"1") || !dynamic)) return -1;
    public_sdk_native_inventory=native_inventory!=NULL;
    const char *engine=getenv("AZHUB_SDK_INVENTORY_ENGINE");
    if (engine && (strcmp(engine,"1") || !native_inventory)) return -1;
    public_sdk_use_engine=engine!=NULL;
    const char *candidate=getenv("AZHUB_SDK_CANDIDATE_RUNTIME");
    if (candidate && (strcmp(candidate,"1") || !engine)) return -1;
    public_sdk_candidate_runtime=candidate!=NULL;
    const char *recover_network=getenv("AZHUB_SDK_RECOVER_NETWORK");
    if (recover_network && (strcmp(recover_network,"1") || !candidate)) return -1;
    public_sdk_recover_network=recover_network!=NULL;
    if (public_sdk_dynamic_identity) {
        struct ifreq interface={0};strcpy(interface.ifr_name,"eth0");
        int control=socket(AF_INET,SOCK_DGRAM|SOCK_CLOEXEC,0);
        if (control<0) return -1;
        int result=ioctl(control,SIOCGIFHWADDR,&interface);close(control);
        if (result<0) return -1;
        memcpy(public_sdk_parent_mac,interface.ifr_hwaddr.sa_data,6);
    }
    const char *recover=getenv("AZHUB_SDK_PLAYER_RECOVERY");
    if (recover && (strcmp(recover,"1") || !inventory)) return -1;
    public_sdk_player_recovery=recover!=NULL;
    const char *mask=getenv("AZHUB_SDK_INVENTORY_MASK");
    if (mask) {
        char *end;unsigned long value=strtoul(mask,&end,16);
        if (!inventory || !*mask || *end || value>255 || !value || (!public_sdk_use_engine && !(value&1))) return -1;
        public_sdk_inventory_mask=(unsigned)value;
    }
    if (public_sdk_player_recovery) for (unsigned i=0;i<8;++i) if (public_sdk_inventory_mask&(1u<<i))
        if (hub_public_player_reserve(&public_sdk_players,i,0)<0) return -1;
    public_sdk_probe.generation=1;
    const char *hold=getenv("AZHUB_SDK_SERVICE_HOLD");
    if (hold && (strcmp(hold,"1") || !service)) return -1;
    public_sdk_probe.hold=hold!=NULL;
    for (unsigned i=0;i<32;++i) if (!((run[i]>='0' && run[i]<='9') || (run[i]>='a' && run[i]<='f'))) return -1;
    strcpy(public_sdk_probe.run,run);
    snprintf(public_sdk_probe.result,sizeof public_sdk_probe.result,"/run/azhub-sdk-probe-%s.json",run);
    if (!access(public_sdk_probe.result,F_OK)) return -1;
    const char *assets=lf_core->assets_dir("usb-hub-support");
    if (!assets || snprintf(public_sdk_probe.helper,sizeof public_sdk_probe.helper,
            "%s/usb-hub-support-network-helper.so",assets)>=(int)sizeof public_sdk_probe.helper) return -1;
    const char *data=lf_core->data_dir("usb-hub-support");
    if (!data) return -1;
    int log_length=public_sdk_attempt ? snprintf(public_sdk_probe.helper_log,sizeof public_sdk_probe.helper_log,
            "%s/sdk-broker-%s-r%u.log",data,run,public_sdk_attempt):snprintf(public_sdk_probe.helper_log,sizeof public_sdk_probe.helper_log,
            "%s/sdk-broker-%s.log",data,run);
    if (log_length<0 || log_length>=(int)sizeof public_sdk_probe.helper_log || !access(public_sdk_probe.helper_log,F_OK)) return -1;
    public_sdk_probe.executable=open(public_sdk_probe.helper,O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    public_sdk_probe.source_fd=public_sdk_probe.allocate ? open(public_sdk_native_inventory ? "/media/usb":public_sdk_inventory ? "/media/usb/sdc1":"/tmp/azhub-sdk-library",O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW):
        open("/var/run/netns/azhub-sdk-probe",O_RDONLY|O_CLOEXEC|O_NOFOLLOW);
    if (public_sdk_probe.executable<0 || public_sdk_probe.source_fd<0 || access(public_sdk_probe.helper,X_OK)) goto failed;
    struct stat namespace;if (fstat(public_sdk_probe.source_fd,&namespace)) goto failed;
    public_sdk_probe.inode=namespace.st_ino;
    int random=open("/dev/urandom",O_RDONLY|O_CLOEXEC);if (random<0) goto failed;
    size_t used=0;
    while (used<16) {
        ssize_t got=read(random,public_sdk_probe.token+used,16-used);
        if (got<0 && errno==EINTR) continue;
        if (got<=0) { close(random);goto failed; }used+=(size_t)got;
    }
    close(random);
    hub_public_broker_client_init(&public_sdk_client,public_sdk_probe.token);public_sdk_client_initialized=1;
    for (unsigned i=0;i<16;++i) snprintf(public_sdk_probe.hex+2*i,3,"%02x",public_sdk_probe.token[i]);
    snprintf(public_sdk_probe.directory,sizeof public_sdk_probe.directory,"/run/azhub-sdk-ipc-%ld-%.8s",(long)getpid(),run);
    if (mkdir(public_sdk_probe.directory,0700)) { public_sdk_probe.directory[0]=0;goto failed; }
    snprintf(public_sdk_probe.socket,sizeof public_sdk_probe.socket,"%s/control.sock",public_sdk_probe.directory);
    public_sdk_probe.listener=socket(AF_UNIX,SOCK_SEQPACKET|SOCK_NONBLOCK|SOCK_CLOEXEC,0);
    if (public_sdk_probe.listener<0) goto failed;
    struct sockaddr_un address={.sun_family=AF_UNIX};strcpy(address.sun_path,public_sdk_probe.socket);
    if (bind(public_sdk_probe.listener,(struct sockaddr *)&address,offsetof(struct sockaddr_un,sun_path)+strlen(address.sun_path)+1) ||
            chmod(public_sdk_probe.socket,0600) || listen(public_sdk_probe.listener,1)) goto failed;
    public_sdk_probe.enabled=1;return 0;
failed:
    public_sdk_probe_cleanup();return -1;
}
static int public_sdk_probe_receive(struct hub_public_broker_frame *frame) {
    int got=hub_public_broker_receive_token(public_sdk_probe.peer,public_sdk_probe.token,frame);
    if (got==1 && public_sdk_probe.allocate && frame->op==HUB_BROKER_STATUS && frame->generation &&
            hub_public_broker_client_status(&public_sdk_client,frame)<0) {
        hub_public_broker_dispose(frame);public_sdk_probe_finish(0,"inventory status");return -1;
    }
    if (got<0) { lf_log("sdk-broker-probe receive errno=%d",errno);public_sdk_probe_finish(0,"receive rejected"); }
    return got;
}
static void public_sdk_probe_send_source(uint64_t now) {
    struct hub_public_broker_frame frame={0};memcpy(frame.token,public_sdk_probe.token,16);
        frame.op=public_sdk_probe.allocate ? HUB_BROKER_ALLOCATE:HUB_BROKER_SOURCE;frame.generation=public_sdk_probe.generation;frame.length=1;frame.payload[0]='N';
        if (public_sdk_probe.service) {
            struct stat root;
            if (public_sdk_probe.allocate ? fstat(public_sdk_probe.source_fd,&root):stat("/tmp/azhub-sdk-library",&root)) { public_sdk_probe_finish(0,"fixture root");return; }
            if (public_sdk_inventory && public_sdk_inventory_slot<0) {
                for (int i=0;i<nfakes;++i) {
                    struct stat actual;
                    if (fakes[i].present && !fakes[i].eject_reserved && !stat(fakes[i].dir,&actual) &&
                            actual.st_dev==root.st_dev && actual.st_ino==root.st_ino) {
                        public_sdk_inventory_slot=i;public_sdk_native_slots[0]=i;break;
                    }
                }
                if (public_sdk_inventory_slot<0) { public_sdk_probe_finish(0,"inventory root not present");return; }
                lf_log("sdk-broker-probe inventory source=%d root=%s",public_sdk_inventory_slot,fakes[public_sdk_inventory_slot].dir);
            }
            struct hub_public_broker_source source={.identity=1,.device=root.st_dev,.inode=root.st_ino,
                .player=41,.mac={2,0,0,0,0x12,0x73},.directory="/tmp/azhub-sdk-library",.interface="azpub7",.label="Hub USB 1"};
            if (public_sdk_inventory) strcpy(source.directory,public_sdk_native_inventory ? fakes[public_sdk_inventory_slot].dir:"/media/usb/sdc1");
            if (public_sdk_probe.allocate) { source.mac[5]=3;strcpy(source.interface,"azpub0"); }
            if (public_sdk_dynamic_identity && hub_public_source_mac(public_sdk_parent_mac,0,source.mac)<0) {
                public_sdk_probe_finish(0,"dynamic source identity");return;
            }
            if (public_sdk_probe.allocate && hub_public_broker_client_desire(&public_sdk_client,0,&source,public_sdk_probe.source_fd)<0) {
                public_sdk_probe_finish(0,"inventory desired root");return;
            }
            if (public_sdk_inventory) {
                public_sdk_observations.slots[0].incarnation=source.identity;
                public_sdk_observations.slots[0].seen=1;
            }
            if (hub_public_broker_source_encode(&source,frame.payload,sizeof frame.payload,&frame.length)) { public_sdk_probe_finish(0,"source encode");return; }
        }
        frame.count=1;frame.descriptors[0]=public_sdk_probe.source_fd;
        lf_log("sdk-broker-probe HELLO, source FD send t=%llu",(unsigned long long)now);
        if (public_sdk_probe.allocate) {
            int sent=hub_public_broker_client_send(&public_sdk_client,public_sdk_probe.peer,0);
            if (sent<0) { public_sdk_probe_finish(0,"inventory send");return; }
            if (!sent) { public_sdk_probe.stage=8;return; }
        } else if (hub_public_broker_send(public_sdk_probe.peer,&frame)) { public_sdk_probe_finish(0,"namespace send");return; }
    if (public_sdk_inventory && public_sdk_probe.source_fd>=0) {
        close(public_sdk_probe.source_fd);public_sdk_probe.source_fd=-1;
    }
    public_sdk_probe.stage=3;
}
/* Both mount gates run on HubTransport, as does all client mutation. A probe
 * failure retains the closed gate; closing IPC alone is not retirement proof. */
static int public_sdk_inventory_drain(unsigned slot) {
    if (!public_sdk_inventory) return 1;
    int selected=-1;
    for (unsigned i=0;i<8;++i) if ((public_sdk_inventory_mask&(1u<<i)) && public_sdk_native_slots[i]==(int)slot) selected=(int)i;
    if (selected<0 || public_sdk_inventory_retired) return 1;
    if (!public_sdk_probe.enabled || public_sdk_probe.stage<3 || public_sdk_probe.stage==5) return 0;
    if (hub_public_broker_inventory_withdraw(public_sdk_use_engine ? &public_sdk_engine.observed:&public_sdk_observations,&public_sdk_client,(unsigned)selected)<0) return 0;
    if (!selected && public_sdk_probe.source_fd>=0) { close(public_sdk_probe.source_fd);public_sdk_probe.source_fd=-1; }
    if (hub_public_broker_client_send(&public_sdk_client,public_sdk_probe.peer,(unsigned)selected)<0) return 0;
    return hub_public_broker_client_drained(&public_sdk_client,(unsigned)selected);
}
/* Shared source-local reconciliation; zero capacity never stops neighbors. */
static int public_sdk_select_player(unsigned slot,struct hub_public_broker_source *declared,int present,int reserved,uint64_t now) {
    if (!public_sdk_player_recovery) return 1;
    struct hub_public_broker_client_slot *live=public_sdk_client.slots+slot;
    unsigned number=0;
    int result=hub_public_player_reconcile(&public_sdk_player_state,&public_sdk_players,&public_sdk_client,
        slot,present,reserved,now,&number);
    if (result>0) {
        if (live->wanted && live->phase==HUB_BROKER_EMPTY && live->error==EEXIST && live->desired.player!=number)
            lf_log("sdk-broker-probe player replacement old=%u new=%u retired_generation=%llu slot=%u",live->desired.player,
                number,(unsigned long long)live->generation,slot);
        declared->player=number;
    }
    return result;
}
/* Every selected source gets its own declaration, root capability and command
 * generation. The first source retains the legacy probe's ready marker. */
static int public_sdk_inventory_neighbors(void) {
    for (unsigned slot=1;slot<8;++slot) if (public_sdk_inventory_mask&(1u<<slot)) {
        struct hub_public_broker_source declared={.identity=1,.device=1,.inode=1,.player=41+slot,
            .mac={2,0,0,0,0x12,(unsigned char)(3+slot)}};
        if (public_sdk_native_inventory && slot<(unsigned)nfakes) {
            if (strlen(fakes[slot].dir)>=sizeof declared.directory) return -1;
            strcpy(declared.directory,fakes[slot].dir);
        } else snprintf(declared.directory,sizeof declared.directory,"/media/usb/sd%c1",'c'+slot);
        snprintf(declared.interface,sizeof declared.interface,"azpub%u",slot);
        snprintf(declared.label,sizeof declared.label,"Hub USB %u",slot+1);
        if (public_sdk_dynamic_identity && hub_public_source_mac(public_sdk_parent_mac,slot,declared.mac)<0) return -1;
        int found=-1;
        if (public_sdk_native_inventory) { if (slot<(unsigned)nfakes) found=(int)slot; }
        else for (int i=0;i<nfakes;++i) if (!strcmp(fakes[i].dir,declared.directory)) { found=i;break; }
        if (found>=0) public_sdk_native_slots[slot]=found;
        int present=found>=0 && fakes[found].present && !fakes[found].unmount_pending;
        int reserved=found>=0 && fakes[found].eject_reserved;
        int selected=public_sdk_select_player(slot,&declared,present,reserved,hub_public_broker_ms());
        if (selected<0) return -1;
        int observed=(selected || !present || reserved) ? hub_public_broker_inventory_observe(&public_sdk_observations,&public_sdk_client,slot,&declared,present,reserved):0;
        if (observed<0 && errno!=ENOENT && errno!=ESTALE) return -1;
        if (hub_public_broker_client_send(&public_sdk_client,public_sdk_probe.peer,slot)<0) return -1;
    }
    if (!public_sdk_multi_ready && public_sdk_inventory_mask!=1) {
        int ready=1;
        for (unsigned slot=0;slot<8;++slot) if ((public_sdk_inventory_mask&(1u<<slot)) && public_sdk_client.slots[slot].phase!=HUB_BROKER_ACTIVE) ready=0;
        if (ready) {
            char path[128];snprintf(path,sizeof path,"/run/azhub-sdk-service-%s.multi",public_sdk_probe.run);
            int fd=open(path,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);if (fd<0) return -1;
            close(fd);public_sdk_multi_ready=1;
            lf_log("sdk-broker-probe multi inventory ready mask=%#x",public_sdk_inventory_mask);
        }
    }
    return 0;
}

static void public_sdk_probe_poll(void) {
    if (!public_sdk_probe.enabled) return;
    uint64_t now=hub_public_broker_ms();
    if (public_sdk_probe.stage && now>=public_sdk_probe.deadline) { public_sdk_probe_finish(0,"deadline");return; }
    struct hub_public_broker_frame frame={0};memcpy(frame.token,public_sdk_probe.token,16);
    switch (public_sdk_probe.stage) {
    case 0: {
        if (public_sdk_native_inventory && !public_sdk_use_engine) {
            if (!nfakes || !fakes[0].present || fakes[0].eject_reserved || fakes[0].unmount_pending) return;
            int root=open(fakes[0].dir,O_RDONLY|O_DIRECTORY|O_CLOEXEC|O_NOFOLLOW);
            struct stat actual;if (root<0) { public_sdk_probe_finish(0,"native inventory root");return; }
            if (fstat(root,&actual)) { close(root);public_sdk_probe_finish(0,"native inventory identity");return; }
            if (public_sdk_probe.source_fd>=0) close(public_sdk_probe.source_fd);
            public_sdk_probe.source_fd=root;public_sdk_probe.inode=actual.st_ino;
        }
        char parent[24];snprintf(parent,sizeof parent,"%ld",(long)getpid());
        const char *args[]={public_sdk_candidate_runtime ? "--broker-candidate-emulator":public_sdk_probe.allocate ? "--broker-allocate-emulator":public_sdk_probe.service ? "--broker-service-emulator":"--broker-probe",public_sdk_probe.socket,public_sdk_probe.hex,parent,NULL};
        az_launch_opts options={.size=sizeof options,.log_path=public_sdk_probe.helper_log};
        public_sdk_probe.child=lf_core->launch("usb-hub-support",public_sdk_probe.helper,args,&options);
        if (public_sdk_probe.child<0) { public_sdk_probe_finish(0,"SDK launch refused");return; }
        lf_log("sdk-broker-probe managed launch pid=%d attempt=%u",public_sdk_probe.child,public_sdk_attempt);
        public_sdk_probe.stage=1;public_sdk_probe.deadline=now+(public_sdk_probe.allocate ? 45000:15000);break;
    }
    case 1:
        public_sdk_probe.peer=accept4(public_sdk_probe.listener,NULL,NULL,SOCK_NONBLOCK|SOCK_CLOEXEC);
        if (public_sdk_probe.peer<0) { if (errno!=EAGAIN && errno!=EINTR) public_sdk_probe_finish(0,"accept");return; }
        if (hub_public_broker_peer(public_sdk_probe.peer,0,geteuid(),public_sdk_probe.executable)) {
            public_sdk_probe_finish(0,"helper credentials/executable");return;
        }
        struct ucred peer;socklen_t size=sizeof peer;
        if (getsockopt(public_sdk_probe.peer,SOL_SOCKET,SO_PEERCRED,&peer,&size)) { public_sdk_probe_finish(0,"peer PID");return; }
        public_sdk_probe.peer_pid=peer.pid;
        lf_log("sdk-broker-probe accepted helper pid=%d t=%llu",peer.pid,(unsigned long long)now);
        public_sdk_probe.stage=2;break;
    case 2:
        if (public_sdk_probe_receive(&frame)!=1) return;
        if (frame.op!=HUB_BROKER_HELLO || frame.slot || frame.generation) { hub_public_broker_dispose(&frame);public_sdk_probe_finish(0,"HELLO");return; }
        if (public_sdk_use_engine) {
            if (hub_public_sharing_inventory_init(&public_sdk_engine,public_sdk_probe.token,public_sdk_parent_mac)<0) {
                public_sdk_probe_finish(0,"inventory engine init");return;
            }
            if (public_sdk_probe.source_fd>=0) { close(public_sdk_probe.source_fd);public_sdk_probe.source_fd=-1; }
            public_sdk_probe.stage=9;public_sdk_probe.deadline=now+420000;
        } else public_sdk_probe_send_source(now);
        break;
    case 3:
        if (public_sdk_probe.cancel_dhcp) {
            char path[128];snprintf(path,sizeof path,"/run/azhub-sdk-service-%s.done",public_sdk_probe.run);
            if (!access(path,F_OK)) {
                close(public_sdk_probe.peer);public_sdk_probe.peer=-1;
                public_sdk_probe.stage=5;public_sdk_probe.deadline=now+10000;return;
            }
        }
        if (public_sdk_probe_receive(&frame)!=1) return;
        lf_log("sdk-broker-probe reply op=%u error=%u generation=%llu bytes=%zu hint=%u",frame.op,frame.error,
            (unsigned long long)frame.generation,frame.length,frame.length ? frame.payload[0]:0);
        if (public_sdk_probe.service) {
            if (public_sdk_probe.expect_reject && frame.op==HUB_BROKER_STATUS && !frame.slot && frame.generation==1 &&
                    frame.error==(unsigned)public_sdk_probe.expect_reject && frame.length==1 &&
                    (frame.payload[0]==0 || frame.payload[0]==3)) {
                frame=(struct hub_public_broker_frame){.op=HUB_BROKER_STOP};memcpy(frame.token,public_sdk_probe.token,16);
                if (hub_public_broker_send(public_sdk_probe.peer,&frame)) { public_sdk_probe_finish(0,"rejected grant STOP");return; }
                public_sdk_probe.stage=4;return;
            }
            if (public_sdk_probe.expect_conflict && frame.op==HUB_BROKER_STATUS && !frame.slot && frame.generation==1 &&
                    frame.error==EADDRINUSE && frame.length==1 && (frame.payload[0]==0 || frame.payload[0]==3)) {
                public_sdk_probe.conflict_seen=1;
                if (public_sdk_probe.retry_conflict) {
                    public_sdk_probe.stage=7;
                    if (!frame.payload[0]) {
                        public_sdk_probe.generation=2;public_sdk_probe.deadline=now+45000;
                        public_sdk_probe_send_source(now);
                    }
                    return;
                }
                frame.op=HUB_BROKER_STOP;frame.error=0;frame.generation=0;frame.length=0;
                if (hub_public_broker_send(public_sdk_probe.peer,&frame)) { public_sdk_probe_finish(0,"conflict STOP send");return; }
                public_sdk_probe.stage=4;return;
            }
            if (frame.op!=HUB_BROKER_STATUS || frame.slot || frame.generation!=public_sdk_probe.generation || frame.error || frame.length!=1 ||
                    (frame.payload[0]!=1 && frame.payload[0]!=2)) { hub_public_broker_dispose(&frame);public_sdk_probe_finish(0,"source readiness");return; }
            if (frame.payload[0]==1) return;
            if (public_sdk_probe.expect_conflict && !(public_sdk_probe.retry_conflict && public_sdk_probe.conflict_seen && public_sdk_probe.generation==2)) { public_sdk_probe_finish(0,"unexpected ACTIVE after conflict");return; }
            if (public_sdk_probe.hold) {
                char path[128];snprintf(path,sizeof path,"/run/azhub-sdk-service-%s.ready",public_sdk_probe.run);
                int fd=open(path,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
                if (fd<0) { public_sdk_probe_finish(0,"service marker");return; }
                close(fd);public_sdk_probe.stage=6;public_sdk_probe.deadline=now+(public_sdk_inventory ? 420000:180000);return;
            }
        } else if (frame.op!=HUB_BROKER_STATUS || frame.slot || frame.generation!=public_sdk_probe.generation || frame.error || frame.length!=8 ||
                hub_public_broker_get(frame.payload,8)!=public_sdk_probe.inode) { hub_public_broker_dispose(&frame);public_sdk_probe_finish(0,"namespace acknowledgement");return; }
        frame.op=HUB_BROKER_STOP;frame.generation=0;frame.length=0;
        if (hub_public_broker_send(public_sdk_probe.peer,&frame)) { public_sdk_probe_finish(0,"STOP send");return; }
        public_sdk_probe.stage=4;break;
    case 4:
        if (public_sdk_probe_receive(&frame)!=1) return;
        if (public_sdk_probe.service && frame.op==HUB_BROKER_STATUS &&
                (!frame.slot || (public_sdk_inventory && (public_sdk_inventory_mask&(1u<<frame.slot)))) &&
                frame.generation==(public_sdk_probe.allocate ? public_sdk_client.slots[frame.slot].generation:public_sdk_probe.generation) &&
                (!frame.error || (public_sdk_probe.conflict_seen && frame.error==EADDRINUSE) ||
                 (public_sdk_probe.expect_reject && frame.error==(unsigned)public_sdk_probe.expect_reject)) &&
                frame.length==1 && (frame.payload[0]==0 || frame.payload[0]==3)) return;
        if (frame.op!=HUB_BROKER_STATUS || frame.slot || frame.generation || frame.error ||
                (public_sdk_probe.service ? (frame.length!=1 || frame.payload[0]!=4):frame.length!=0)) {
            hub_public_broker_dispose(&frame);public_sdk_probe_finish(0,"STOP acknowledgement");return;
        }
        if (public_sdk_probe.allocate) for (unsigned slot=0;slot<8;++slot) {
            if (!hub_public_broker_client_drained(&public_sdk_client,slot)) {
                public_sdk_probe_finish(0,"STOP before all sources drained");return;
            }
        }
        close(public_sdk_probe.peer);public_sdk_probe.peer=-1;public_sdk_probe.stage=5;break;
    case 5:
        if (!lf_core->launch_alive("usb-hub-support",public_sdk_probe.child)) {
            if (public_sdk_restart_pending) {
                if (public_sdk_attempt==UINT_MAX) { public_sdk_probe_finish(0,"restart attempt overflow");return; }
                for (unsigned i=0;i<8;++i) if (!hub_public_broker_client_drained(&public_sdk_client,i)) {
                    public_sdk_probe_finish(0,"restart before retirement");return;
                }
                const char *suffixes[]={"ready","multi"};
                for (unsigned i=0;i<2;++i) {
                    char path[128];snprintf(path,sizeof path,"/run/azhub-sdk-service-%s.%s",public_sdk_probe.run,suffixes[i]);unlink(path);
                }
                public_sdk_probe_cleanup();++public_sdk_attempt;
                public_sdk_probe.enabled=1;public_sdk_probe.stage=10;public_sdk_probe.deadline=now+30000;
                public_sdk_restart_at=now+15000;
                lf_log("sdk-broker-probe restart scheduled attempt=%u after confirmed retirement",public_sdk_attempt);
                return;
            }
            public_sdk_inventory_retired=1;public_sdk_probe_finish(1,"managed lifecycle and IPC complete");
        }
        break;
    case 10:
        if (now<public_sdk_restart_at) return;
        memset(&public_sdk_probe,0,sizeof public_sdk_probe);
        public_sdk_probe.listener=public_sdk_probe.peer=public_sdk_probe.source_fd=public_sdk_probe.executable=-1;
        public_sdk_multi_ready=public_sdk_inventory_reported=public_sdk_inventory_retired=public_sdk_restart_pending=0;
        public_sdk_inventory_slot=-1;
        for (unsigned i=0;i<8;++i) public_sdk_native_slots[i]=-1;
        if (public_sdk_probe_prepare()) { public_sdk_probe_finish(0,"restart preparation");return; }
        break;
    case 9: {
        struct hub_public_sharing_view views[8]={0};
        uint64_t generation[8],retired_generation[8];int phase[8],error[8];
        for (unsigned slot=0;slot<8;++slot) {
            retired_generation[slot]=public_sdk_engine.retired_generation[slot];
            generation[slot]=public_sdk_client.slots[slot].generation;phase[slot]=public_sdk_client.slots[slot].phase;
            error[slot]=public_sdk_client.slots[slot].error;
            if ((public_sdk_inventory_mask&(1u<<slot)) && slot<(unsigned)nfakes) {
                views[slot]=(struct hub_public_sharing_view){fakes[slot].dir,fakes[slot].present && !fakes[slot].unmount_pending,fakes[slot].eject_reserved};
                public_sdk_native_slots[slot]=(int)slot;
            }
        }
        int polled=hub_public_sharing_inventory_poll(&public_sdk_engine,public_sdk_probe.peer,views,now);
        if (polled<0) {
            public_sdk_probe_finish(0,"inventory engine poll");return;
        }
        if (polled==1) {
            lf_log("sdk-broker-probe helper initiated retirement complete");
            if (public_sdk_recover_network) {
                char done[128];snprintf(done,sizeof done,"/run/azhub-sdk-service-%s.done",public_sdk_probe.run);
                public_sdk_restart_pending=access(done,F_OK)!=0;
            }
            close(public_sdk_probe.peer);public_sdk_probe.peer=-1;public_sdk_probe.stage=5;public_sdk_probe.deadline=now+10000;return;
        }
        int ready=1;
        for (unsigned slot=0;slot<8;++slot) if (public_sdk_inventory_mask&(1u<<slot)) {
            struct hub_public_broker_client_slot *live=public_sdk_client.slots+slot;
            if (retired_generation[slot]!=public_sdk_engine.retired_generation[slot])
                lf_log("sdk-broker-probe inventory acknowledged retirement generation=%llu error=%u slot=%u",
                    (unsigned long long)public_sdk_engine.retired_generation[slot],public_sdk_engine.retired_error[slot],slot);
            if (generation[slot]!=live->generation || phase[slot]!=live->phase || error[slot]!=live->error)
                lf_log("sdk-broker-probe inventory status generation=%llu phase=%u error=%u slot=%u",
                    (unsigned long long)live->generation,live->phase,live->error,slot);
            if (live->phase!=HUB_BROKER_ACTIVE) ready=0;
        }
        if (!public_sdk_inventory_reported && public_sdk_client.slots[0].generation &&
                hub_public_broker_client_drained(&public_sdk_client,0) && !public_sdk_client.slots[0].wanted) {
            public_sdk_inventory_reported=1;lf_log("sdk-broker-probe inventory retired source=%d",public_sdk_native_slots[0]);
        }
        if (ready && !public_sdk_multi_ready) {
            for (unsigned slot=0;slot<8;++slot) if (public_sdk_inventory_mask&(1u<<slot)) {
                public_sdk_probe.inode=public_sdk_client.slots[slot].desired.inode;
                lf_log("sdk-broker-probe inventory source=%d root=%s",public_sdk_native_slots[slot],public_sdk_client.slots[slot].desired.directory);break;
            }
            const char *suffixes[]={"ready","multi"};
            for (unsigned i=0;i<2;++i) {
                char path[128];snprintf(path,sizeof path,"/run/azhub-sdk-service-%s.%s",public_sdk_probe.run,suffixes[i]);
                int fd=open(path,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW|O_CLOEXEC,0600);
                if (fd<0) { public_sdk_probe_finish(0,"inventory engine marker");return; }close(fd);
            }
            public_sdk_multi_ready=1;
        }
        char done[128];snprintf(done,sizeof done,"/run/azhub-sdk-service-%s.done",public_sdk_probe.run);
        if (access(done,F_OK)) return;
        frame=(struct hub_public_broker_frame){.op=HUB_BROKER_STOP};memcpy(frame.token,public_sdk_probe.token,16);
        if (hub_public_broker_send(public_sdk_probe.peer,&frame)) { public_sdk_probe_finish(0,"inventory engine STOP");return; }
        public_sdk_probe.stage=4;break;
    }
    case 8:
        public_sdk_probe_send_source(now);break;
    case 7:
        if (public_sdk_probe_receive(&frame)!=1) return;
        if (frame.op!=HUB_BROKER_STATUS || frame.slot || frame.generation!=1 ||
                (frame.error && frame.error!=EADDRINUSE) || frame.length!=1 ||
                (frame.payload[0]!=0 && frame.payload[0]!=3)) {
            hub_public_broker_dispose(&frame);public_sdk_probe_finish(0,"conflict retirement");return;
        }
        if (!frame.payload[0]) {
            public_sdk_probe.generation=2;public_sdk_probe.deadline=now+45000;
            public_sdk_probe_send_source(now);
        }
        break;
    case 6: {
        if (public_sdk_inventory) {
            struct fake *source=fakes+public_sdk_inventory_slot;
            if (!source->present || source->unmount_pending || source->eject_reserved)
                public_sdk_inventory_drain((unsigned)public_sdk_inventory_slot);
            int received=public_sdk_probe_receive(&frame);
            if (received<0) return;
            if (received==1 && (frame.op!=HUB_BROKER_STATUS || !(public_sdk_inventory_mask&(1u<<frame.slot)) ||
                    frame.generation!=public_sdk_client.slots[frame.slot].generation)) {
                hub_public_broker_dispose(&frame);public_sdk_probe_finish(0,"inventory held status");return;
            }
            if (!public_sdk_inventory_reported && hub_public_broker_client_drained(&public_sdk_client,0) && !public_sdk_client.slots[0].wanted) {
                public_sdk_inventory_reported=1;
                lf_log("sdk-broker-probe inventory retired source=%d",public_sdk_inventory_slot);
            }
            if (received==1) lf_log("sdk-broker-probe inventory status generation=%llu phase=%u error=%u slot=%u",
                (unsigned long long)frame.generation,frame.payload[0],frame.error,frame.slot);
            struct hub_public_broker_client_slot *live=public_sdk_client.slots;
            struct hub_public_broker_source declared=live->desired;
            if (public_sdk_native_inventory) {
                if (strlen(source->dir)>=sizeof declared.directory) { public_sdk_probe_finish(0,"native root length");return; }
                strcpy(declared.directory,source->dir);
            }
            int present=source->present && !source->unmount_pending && !strcmp(source->dir,declared.directory);
            int selected=public_sdk_select_player(0,&declared,present,source->eject_reserved,now);
            if (selected<0) { public_sdk_probe_finish(0,"player ownership");return; }
            int observed=(selected || !present || source->eject_reserved) ?
                hub_public_broker_inventory_observe(&public_sdk_observations,&public_sdk_client,0,&declared,
                    present,source->eject_reserved):0;
            if (observed<0 && errno!=ENOENT && errno!=ESTALE) {
                public_sdk_probe_finish(0,"inventory observation");return;
            }
            if (observed>0 && live->wanted) public_sdk_inventory_reported=0;
            if (hub_public_broker_client_send(&public_sdk_client,public_sdk_probe.peer,0)<0) {
                public_sdk_probe_finish(0,"inventory reconcile send");return;
            }
            public_sdk_probe.generation=live->generation;
            if (public_sdk_inventory_neighbors()<0) { public_sdk_probe_finish(0,"neighbor inventory");return; }

        }
        char path[128];snprintf(path,sizeof path,"/run/azhub-sdk-service-%s.done",public_sdk_probe.run);
        if (access(path,F_OK)) return;
        frame=(struct hub_public_broker_frame){.op=HUB_BROKER_STOP};memcpy(frame.token,public_sdk_probe.token,16);
        if (hub_public_broker_send(public_sdk_probe.peer,&frame)) { public_sdk_probe_finish(0,"held STOP send");return; }
        public_sdk_probe.stage=4;break;
    }
    }
}
#endif
