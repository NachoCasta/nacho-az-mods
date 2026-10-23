/* SPDX-License-Identifier: GPL-3.0-only */
/* Exercise the actual production worker across a blocked old-label read. */
#include <atomic>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
static std::atomic<bool> old_read(false),release_old(false);
static int fixture_label(const char *root,char label[128]) {
    if(!strcmp(root,"/media/usb/rejected")) {strcpy(label,"REJECTED PARTIAL");return 0;}
    if(!strcmp(root,"/media/usb/old")) {
        old_read.store(true);
        while(!release_old.load())usleep(1000);
        strcpy(label,"OLD INSERTION");
    } else if(!strcmp(root,"/media/usb/neighbor"))strcpy(label,"NEIGHBOR");
    else strcpy(label,"NEW INSERTION");
    return 1;
}
#define NUSB_READ_LABEL fixture_label
#include "../native-names.cpp"
static std::atomic<unsigned> notifications(0);
static void quiet_log(const char *,const char *,...) {}
static void posted(const char *,void (*)(void *),void *) {notifications.fetch_add(1);}
static void queue(unsigned port,uint64_t epoch,const char *root) {
    epochs[port].store(epoch);
    pthread_mutex_lock(&state_lock);
    memset(&slots[port],0,sizeof slots[port]);slots[port].epoch=epoch;
    strcpy(slots[port].root,root);
    pthread_mutex_unlock(&state_lock);
}
static Slot snapshot(unsigned port) {
    pthread_mutex_lock(&state_lock);Slot result=slots[port];pthread_mutex_unlock(&state_lock);return result;
}
static void wait_for(bool (*condition)()) {
    for(unsigned n=0;n<5000;n++) {if(condition())return;usleep(1000);}
    assert(!"Worker control timed out");
}
static bool old_started() {return old_read.load();}
static bool new_ready() {Slot s=snapshot(0);return s.done==s.epoch && !strcmp(s.label,"NEW INSERTION");}
static bool rejected_ready() {Slot s=snapshot(0);return s.epoch==43 && s.done==43 && !s.label[0];}
static bool both_posted() {return notifications.load()==2;}
static bool neighbor_ready() {Slot s=snapshot(1);return s.done==s.epoch && !strcmp(s.label,"NEIGHBOR");}
int main() {
    az_core_v1 core={};core.size=sizeof core;core.post_ui=posted;core.log=quiet_log;api=&core;
    epochs[0].store(1);epochs[1].store(1);queue(0,1,"/media/usb/old");queue(1,1,"/media/usb/neighbor");
    active.store(true);pthread_t thread;assert(!pthread_create(&thread,0,worker,0));
    wait_for(old_started);
    /* The old filesystem read is still in flight when this slot is reused. */
    queue(0,2,"/media/usb/new");release_old.store(true);
    wait_for(new_ready);wait_for(neighbor_ready);wait_for(both_posted);
    assert(notifications.load()==2); // No notification/publication for the old insertion.
    for(uint64_t epoch=3;epoch<43;epoch++) {
        queue(0,epoch,"/media/usb/new");wait_for(new_ready);
        Slot other=snapshot(1);assert(other.epoch==1 && other.done==1 && !strcmp(other.label,"NEIGHBOR"));
        Slot own=snapshot(0);assert(own.done==epoch && strcmp(own.label,"OLD INSERTION"));
    }
    queue(0,43,"/media/usb/rejected");wait_for(rejected_ready);
    assert(!strcmp(snapshot(1).label,"NEIGHBOR"));
    active.store(false);usleep(100000);assert(!pthread_cancel(thread));assert(!pthread_join(thread,0));
    active.store(true);pending_refresh[0].store(77);
    pthread_mutex_lock(&state_lock);refresh(0);pthread_mutex_unlock(&state_lock);
    assert(pending_refresh[0].load()==77); // UI contention retains the only refresh request.
    epochs[0].store(78);refresh(0);
    assert(!pending_refresh[0].load());active.store(false); // A retired generation cannot notify a replacement.
    puts("Actual worker: blocked-old-read rejection, 40 source-reuse cycles, independent neighbor, bounded notifications and refresh-contention retention pass");
}
