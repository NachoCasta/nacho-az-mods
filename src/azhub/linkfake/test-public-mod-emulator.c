/* SPDX-License-Identifier: GPL-3.0-only */
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <sys/syscall.h>
/* Reuse the real worker fixture and its endpoint ownership assertions. */
#define main public_manager_regression
#include "test-public-source-manager.c"
#undef main
struct fake { char dir[256]; int present,unmount_pending,eject_reserved; };
static struct fake fakes[8];
static int nfakes;
static unsigned reports[8];
#define HUB_PUBLIC_MOD_REPORT(slot,incarnation,state,error,bound,wanted) (++reports[slot])
#include "public-mod-emulator.h"
int main(void) {
    assert(public_mod_configured_count(NULL)==1);
    assert(public_mod_configured_count("1")==1 && public_mod_configured_count("8")==8);
    const char *invalid[]={"","0","9","01","1x","-1"," 1"};
    for (unsigned i=0;i<sizeof invalid/sizeof *invalid;++i)
        assert(public_mod_configured_count(invalid[i])==-1 && errno==EINVAL);
    unsetenv("AZHUB_PUBLIC_EMULATOR");
    assert(!public_mod_prepare() && !public_mod_enabled);
    assert(!setenv("AZHUB_PUBLIC_EMULATOR","invalid",1));
    assert(public_mod_prepare()==-1 && errno==EPERM && !public_mod_enabled);
    unsetenv("AZHUB_PUBLIC_EMULATOR");
    char directory[]="/tmp/azhub-mod-bridge.XXXXXX"; assert(mkdtemp(directory));
    char pioneer[512],rekordbox[512],pdb[512];
    snprintf(pioneer,sizeof pioneer,"%s/PIONEER",directory); assert(!mkdir(pioneer,0700));
    snprintf(rekordbox,sizeof rekordbox,"%s/PIONEER/rekordbox",directory); assert(!mkdir(rekordbox,0700));
    snprintf(pdb,sizeof pdb,"%s/PIONEER/rekordbox/export.pdb",directory);
    unsigned char bytes[1024]={0}; bytes[5]=2; bytes[8]=1; bytes[36]=1; bytes[40]=1; bytes[516]=1;
    int fd=open(pdb,O_CREAT|O_EXCL|O_WRONLY,0600); assert(fd>=0);
    assert(write(fd,bytes,sizeof bytes)==sizeof bytes && !close(fd));
    struct fixture fixture={.manager=&public_mod_manager};
    assert(!hub_public_source_manager_init(&public_mod_manager,
            (struct hub_public_source_manager_ops){open_endpoint,close_endpoint},&fixture));
    public_mod_count=8; public_mod_enabled=1;
    public_mod_poll(); assert(!fixture.opened[0]);
    nfakes=1; strcpy(fakes[0].dir,directory); fakes[0].present=1;
    public_mod_poll(); wait_running(&public_mod_manager,0);
    assert(fixture.opened[0]==1 && public_mod_observations[0].incarnation==1);
    for (int i=0;i<10;++i) public_mod_poll();
    assert(fixture.opened[0]==1);
    unsigned reported=reports[0];
    for (int i=0;i<10;++i) public_mod_poll();
    assert(reports[0]==reported);
    nfakes=8;
    for (unsigned slot=1;slot<8;++slot) {
        strcpy(fakes[slot].dir,directory); fakes[slot].present=1;
    }
    public_mod_poll();
    for (unsigned slot=1;slot<8;++slot) {
        wait_running(&public_mod_manager,slot);
        assert(fixture.opened[slot]==1 && public_mod_observations[slot].incarnation==1);
        assert(memcmp(public_mod_manager.sources[0].worker.nonce,public_mod_manager.sources[slot].worker.nonce,16));
    }
    fakes[0].eject_reserved=1;
    uint64_t deadline=hub_public_worker_milliseconds()+5000;
    while (hub_public_source_worker_state(&public_mod_manager.sources[0].worker)!=HUB_PUBLIC_WORKER_WITHDRAWN) {
        public_mod_poll(); assert(hub_public_worker_milliseconds()<deadline); poll(NULL,0,1);
    }
    assert(!hub_public_source_manager_drained(&public_mod_manager,0));
    while (!public_mod_drain(0)) {
        public_mod_poll(); assert(hub_public_worker_milliseconds()<deadline); poll(NULL,0,1);
    }
    assert(fixture.closed[0]==1);
    /* Reservation must keep the still-present observation from reopening. */
    for (int i=0;i<10;++i) public_mod_poll();
    assert(fixture.opened[0]==1 && public_mod_drain(8));
    for (unsigned slot=1;slot<8;++slot)
        assert(!fixture.closed[slot] && hub_public_source_worker_state(&public_mod_manager.sources[slot].worker)==HUB_PUBLIC_WORKER_RUNNING);
    fakes[0].present=0; fakes[0].eject_reserved=0; public_mod_poll();
    fakes[0].present=1; public_mod_poll(); wait_running(&public_mod_manager,0);
    assert(fixture.opened[0]==2 && public_mod_observations[0].incarnation==2);
    fakes[0].unmount_pending=1; public_mod_poll();
    wait_drained(&public_mod_manager,0); assert(fixture.closed[0]==2);
    for (unsigned slot=1;slot<8;++slot) fakes[slot].present=0;
    public_mod_poll();
    for (unsigned slot=1;slot<8;++slot) {
        wait_drained(&public_mod_manager,slot);
        assert(fixture.closed[slot]==1 && public_mod_observations[slot].incarnation==1);
    }
    public_mod_enabled=0;
    assert(!unlink(pdb) && !rmdir(rekordbox) && !rmdir(pioneer) && !rmdir(directory));
    puts("PASS: eight-source mod presence, isolated withdrawal/drain/reconnect, natural removal and bounded diagnostics");
    return 0;
}
