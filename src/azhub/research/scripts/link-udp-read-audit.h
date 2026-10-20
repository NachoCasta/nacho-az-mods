/* SPDX-License-Identifier: GPL-3.0-only */
/* Bounded metadata-only audit. It does not consume packets or change options. */
static void probe_udp_read_audit(const char *operation,int fd,size_t size,int flags,uintptr_t caller) {
    if(!lf_runtime_ready()) return;
    int saved=errno,type=0; socklen_t length=sizeof type;
    struct sockaddr_in address={0};socklen_t address_length=sizeof address;
    if(getsockname(fd,(void *)&address,&address_length) || address.sin_family!=AF_INET ||
        ntohs(address.sin_port)<50000 || ntohs(address.sin_port)>50002 ||
        getsockopt(fd,SOL_SOCKET,SO_TYPE,&type,&length) || type!=SOCK_DGRAM) {errno=saved;return;}
    struct observation {const char *operation;unsigned port;int flags;size_t size;uintptr_t caller;};
    static struct observation seen[32];static unsigned count;static int overflow;
    static pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&mutex);
    for(unsigned i=0;i<count;i++) if(!strcmp(seen[i].operation,operation) && seen[i].port==ntohs(address.sin_port) &&
        seen[i].flags==flags && seen[i].size==size && seen[i].caller==caller) goto done;
    if(count==32) {
        if(!overflow++) lf_log("UDP read audit: observation capacity reached; further patterns omitted");
        goto done;
    }
    seen[count++]=(struct observation){operation,ntohs(address.sin_port),flags,size,caller};
    lf_log("UDP read audit: operation=%s port=%u flags=%#x size=%zu caller=%#lx",operation,
        ntohs(address.sin_port),flags,size,(unsigned long)caller);
done:
    pthread_mutex_unlock(&mutex);errno=saved;
}
