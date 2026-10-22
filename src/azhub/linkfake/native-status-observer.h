/* SPDX-License-Identifier: GPL-3.0-only */
/* Explicit emulator diagnostic. Observe the native double-buffer store's
 * return and bounded input fields; never rewrite status or retain its pointer.
 * Field semantics require correlation with the controlled public wire packet. */
#ifndef HUB_NATIVE_STATUS_OBSERVER_H
#define HUB_NATIVE_STATUS_OBSERVER_H
#include <sys/utsname.h>
#include <unistd.h>
static fn8 native_status_store_original,native_status_receiver_original;
/* Hash buckets bound diagnostics without allowing empty private sources to
 * exhaust the external loaded-source budget during startup. Collisions can
 * suppress logs; absence is never evidence of rejection. */
static unsigned native_status_receiver_count[256];
static pthread_mutex_t native_status_observer_lock=PTHREAD_MUTEX_INITIALIZER;
static unsigned native_status_observer_count[41];
static unsigned char native_status_observer_last[41][16];
static uint64_t native_status_store_observe(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
        uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    unsigned char key[4]={0},fields[16]={0};
    int observed=b>=0x10000 && c>=0x10000;
    if (observed) { memcpy(key,(const void *)b,4);observed=key[0]<=40; }
    if (observed) memcpy(fields,(const void *)c,sizeof fields);
    uint64_t result=native_status_store_original(a,b,c,d,e,f,g,h);
    if (observed) {
        pthread_mutex_lock(&native_status_observer_lock);
        unsigned index=key[0],count=native_status_observer_count[index];
        if (count<16 && (!count || memcmp(fields,native_status_observer_last[index],sizeof fields))) {
            uint32_t word8;memcpy(&word8,fields+8,sizeof word8);
            memcpy(native_status_observer_last[index],fields,sizeof fields);
            native_status_observer_count[index]=count+1;
            lf_log("native-status-observer key=%u:%u:%u:%u accepted=%llu b0=%u b1=%u b4=%u b5=%u b6=%u word8_le=%u word8_be=%u",
                key[0],key[1],key[2],key[3],(unsigned long long)result,
                fields[0],fields[1],fields[4],fields[5],fields[6],word8,
                ((uint32_t)fields[8]<<24)|((uint32_t)fields[9]<<16)|((uint32_t)fields[10]<<8)|fields[11]);
        }
        pthread_mutex_unlock(&native_status_observer_lock);
    }
    return result;
}
/* Receiver return is a mutex-unlock result, not store acceptance. */
static uint64_t native_status_receiver_observe(uint64_t a,uint64_t b,uint64_t c,uint64_t d,
        uint64_t e,uint64_t f,uint64_t g,uint64_t h) {
    unsigned char fields[16]={0};uint32_t member=0;
    int observed=b>=0x10000 && e>=0x10000;
    if (observed) { memcpy(fields,(const void *)b,sizeof fields);memcpy(&member,(const void *)e,sizeof member); }
    uint64_t result=native_status_receiver_original(a,b,c,d,e,f,g,h);
    if (observed) {
        pthread_mutex_lock(&native_status_observer_lock);
        unsigned bucket=(member^(member>>8)^(member>>16)^(member>>24))&255;
        if (native_status_receiver_count[bucket]<128) {
            ++native_status_receiver_count[bucket];
            uint32_t word8;memcpy(&word8,fields+8,sizeof word8);
            lf_log("native-status-receiver member=%u type=%u other=%u flag=%u b0=%u b1=%u b4=%u b5=%u b6=%u word8_le=%u unlock_result=%llu; admission unproven",
                member,(unsigned)(unsigned char)c,(unsigned)(unsigned char)d,(unsigned)(unsigned char)f,
                fields[0],fields[1],fields[4],fields[5],fields[6],word8,(unsigned long long)result);
        }
        pthread_mutex_unlock(&native_status_observer_lock);
    }
    return result;
}
static int native_status_observer_install(void) {
    struct utsname kernel;
    if (uname(&kernel) || strcmp(kernel.release,"6.6.138-dirty")) return -1;
    /* Exact EP147 store entry rejects null status and key>40 before its stack
     * frame. Hook its validated body at the original STP prologue instead.
     * x0..x4 remain arguments; x5 is the validated key and x6 is zero. Forward
     * all eight registers so the trampoline retains these live temporaries.
     * Early rejection paths remain native and are deliberately not observed. */
    int previous_failures=lf_hook_failures;
    native_status_store_original=make_hook(0x10d324c,native_status_store_observe);
    if (lf_hook_failures!=previous_failures || !native_status_store_original) {
        native_status_store_original=NULL;
        return -1;
    }
    native_status_receiver_original=make_hook(0x10cf350,native_status_receiver_observe);
    if (lf_hook_failures!=previous_failures || !native_status_receiver_original) {
        native_status_receiver_original=NULL;
        return -1;
    }
    lf_log("native-status-observer installed pid=%ld; emulator diagnostic only",(long)getpid());return 0;
}
#endif
