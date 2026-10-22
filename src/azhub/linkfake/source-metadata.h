/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HUB_SOURCE_METADATA_H
#define HUB_SOURCE_METADATA_H
#include <pthread.h>
#include <stdio.h>
#include "source-properties.h"
struct hub_metadata_ticket {uint64_t generation;char root[256],route[96];};
struct hub_metadata_slot {
    struct hub_metadata_ticket ticket;
    struct hub_source_properties properties;
    uint64_t revision;
    int complete;
};
static pthread_mutex_t hub_metadata_mutex=PTHREAD_MUTEX_INITIALIZER;
static struct hub_metadata_slot hub_metadata_slots[8];
static void hub_metadata_invalidate(int source) {
    if (source<0 || source>=8) return;
    pthread_mutex_lock(&hub_metadata_mutex);
    struct hub_metadata_slot *s=&hub_metadata_slots[source];
    if (!++s->ticket.generation) ++s->ticket.generation;
    s->ticket.root[0]=s->ticket.route[0]=0;
    memset(&s->properties,0,sizeof s->properties);
    s->complete=0;
    ++s->revision;
    pthread_mutex_unlock(&hub_metadata_mutex);
}
static void hub_metadata_prepare(int source,const char *root,const char *route) {
    if (source<0 || source>=8) return;
    hub_metadata_invalidate(source);
    pthread_mutex_lock(&hub_metadata_mutex);
    struct hub_metadata_ticket *t=&hub_metadata_slots[source].ticket;
    if (root && route && strlen(root)<sizeof t->root && strlen(route)<sizeof t->route) {
        strcpy(t->root,root);strcpy(t->route,route);
    }
    pthread_mutex_unlock(&hub_metadata_mutex);
}
static struct hub_metadata_ticket hub_metadata_capture(int source) {
    struct hub_metadata_ticket t={0};
    if (source<0 || source>=8) return t;
    pthread_mutex_lock(&hub_metadata_mutex);t=hub_metadata_slots[source].ticket;
    pthread_mutex_unlock(&hub_metadata_mutex);return t;
}
static int hub_metadata_publish(int source,const struct hub_metadata_ticket *ticket,
                                const struct hub_source_properties *properties) {
    if (source<0 || source>=8 || !ticket || !properties || !ticket->generation || !*ticket->root) return 0;
    pthread_mutex_lock(&hub_metadata_mutex);
    struct hub_metadata_slot *s=&hub_metadata_slots[source];
    int same=s->ticket.generation==ticket->generation && !strcmp(s->ticket.root,ticket->root) &&
        !strcmp(s->ticket.route,ticket->route);
    int changed=same && (!s->complete || memcmp(&s->properties,properties,sizeof *properties));
    if (changed) {s->properties=*properties;s->complete=1;++s->revision;}
    pthread_mutex_unlock(&hub_metadata_mutex);return changed;
}
static uint64_t hub_metadata_read(int source,const char *root,const char *route,
                                  struct hub_source_properties *properties) {
    memset(properties,0,sizeof *properties);
    if (source<0 || source>=8 || !root || !route) return 0;
    pthread_mutex_lock(&hub_metadata_mutex);
    struct hub_metadata_slot *s=&hub_metadata_slots[source];uint64_t revision=0;
    if (*s->ticket.root && !strcmp(s->ticket.root,root) && !strcmp(s->ticket.route,route)) {
        *properties=s->properties;revision=s->revision;
    }
    pthread_mutex_unlock(&hub_metadata_mutex);return revision;
}
static void hub_metadata_name(const struct hub_source_properties *properties,
                               const char *volume,int number,char *out,size_t size) {
    if (!out || !size) return;
    if (properties->name_valid) snprintf(out,size,"%s",properties->name);
    else if (volume && *volume) snprintf(out,size,"%s",volume);
    else snprintf(out,size,"USB %d",number);
}
static int hub_metadata_complete(int source,const char *root,const char *route) {
    if (source<0 || source>=8 || !root || !route) return 0;
    pthread_mutex_lock(&hub_metadata_mutex);
    struct hub_metadata_slot *s=&hub_metadata_slots[source];
    int complete=s->complete && *s->ticket.root &&
        !strcmp(s->ticket.root,root) && !strcmp(s->ticket.route,route);
    pthread_mutex_unlock(&hub_metadata_mutex);
    return complete;
}
#endif
