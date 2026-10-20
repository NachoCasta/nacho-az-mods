/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef AZHUB_LINK_MEMBER_RETENTION_H
#define AZHUB_LINK_MEMBER_RETENTION_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* Exact EP147 LinkDeviceManager records. The two reference-counted string
 * handles are pointer-sized; records have no self-relative storage. Moving a
 * complete record transfers its handles without incrementing their references.
 * Only logical live ranges may subsequently run the native destructors.
 * Callers hold the native table mutex while partitioning and notify removed
 * records only after unlocking. This helper never calls native code. */
#define HUB_MEMBER_RECORD_SIZE 0x50u
#define HUB_MEMBER_RETENTION_MAX 64u
struct hub_member_removed {
    unsigned char records[HUB_MEMBER_RETENTION_MAX][HUB_MEMBER_RECORD_SIZE];
    size_t count;
    void (*release_record)(void *);
};
static int hub_member_is_owned(const unsigned char *record,uint32_t active) {
    unsigned member=record[1];
    return record[0]==1 && (member==7 || member==8) &&
        (active & (UINT32_C(1)<<member)) && record[8]==127 && record[9]==77 &&
        record[10]==0 && record[11]==member;
}
static void hub_member_removed_release(struct hub_member_removed *removed) {
    if (!removed->release_record) return;
    for(size_t i=0;i<removed->count;i++) removed->release_record(removed->records[i]);
    removed->count=0;
}
/* Return 1 after transferring ownership, 0 for native fallback (no owned row),
 * or -1 for unsupported input. Fallback/error leaves every input byte intact.
 * On success, table[0..kept) and removed own all original live records exactly
 * once; the old table tail is dead storage and must not be destroyed. */
static int hub_member_partition(unsigned char *table,size_t count,uint32_t active,
    struct hub_member_removed *removed,size_t *kept) {
    if (!removed || !kept || removed->count || !removed->release_record ||
        count>HUB_MEMBER_RETENTION_MAX || (count && !table)) return -1;
    size_t bytes=count*HUB_MEMBER_RECORD_SIZE;
    uintptr_t start=(uintptr_t)table,output=(uintptr_t)removed;
    if (start>UINTPTR_MAX-bytes || output>UINTPTR_MAX-sizeof *removed) return -1;
    if (bytes && start<output+sizeof *removed && output<start+bytes) return -1;
    size_t owned=0;
    for(size_t i=0;i<count;i++) owned+=hub_member_is_owned(table+i*HUB_MEMBER_RECORD_SIZE,active);
    if (!owned) return 0;
    size_t destination=0;
    for(size_t i=0;i<count;i++) {
        unsigned char *record=table+i*HUB_MEMBER_RECORD_SIZE;
        if (hub_member_is_owned(record,active)) {
            if(destination!=i) memmove(table+destination*HUB_MEMBER_RECORD_SIZE,record,HUB_MEMBER_RECORD_SIZE);
            destination++;
        } else {
            memcpy(removed->records[removed->count++],record,HUB_MEMBER_RECORD_SIZE);
        }
    }
    *kept=destination;
    return 1;
}
#endif
