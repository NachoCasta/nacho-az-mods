/* SPDX-License-Identifier: GPL-3.0-only */
/* Standalone emulator-only timing control. Calls real callbacks in this process;
 * never initializes the mod, registers hooks/filters or changes SDK budgets. */
#include "../../linkfake/community.c"
static uint64_t timing_ticks(void) {
    uint64_t value;__asm__ volatile("isb; mrs %0, cntvct_el0":"=r"(value)::"memory");return value;
}
static int __attribute__((noinline)) timing_empty(int bus,unsigned char *frame,int length,void *user) {
    (void)bus;(void)frame;(void)length;(void)user;
    __asm__ volatile("":::"memory");return AZ_FRAME_OK;
}
static int timing_order(const void *a,const void *b) {
    uint64_t first=*(const uint64_t *)a,second=*(const uint64_t *)b;
    return first>second ? 1:first<second ? -1:0;
}
static void timing_measure(const char *name,az_frame_fn callback,uint64_t frequency) {
    const unsigned count=500000;uint64_t *samples=malloc(sizeof *samples*count);
    if (!samples) exit(2);
    unsigned char frame[64]={0};frame[34]=128;
    az_frame_fn volatile call=callback;
    for (unsigned i=0;i<10000;++i) call(AZ_BUS_MIXER_IN,frame,sizeof frame,NULL);
    uint64_t budget=frequency/1000000*200;unsigned overruns=0;
    for (unsigned i=0;i<count;++i) {
        uint64_t before=timing_ticks();call(AZ_BUS_MIXER_IN,frame,sizeof frame,NULL);
        uint64_t elapsed=timing_ticks()-before;samples[i]=elapsed;
        if (elapsed>budget) ++overruns;
    }
    qsort(samples,count,sizeof *samples,timing_order);
    printf("{\"callback\":\"%s\",\"samples\":%u,\"frequency\":%llu,\"SDK_budget_ticks\":%llu,\"overruns\":%u,\"median_ns\":%llu,\"p99_ns\":%llu,\"max_ns\":%llu}\n",
        name,count,(unsigned long long)frequency,(unsigned long long)budget,overruns,
        (unsigned long long)(samples[count/2]*1000000000/frequency),
        (unsigned long long)(samples[count*99/100]*1000000000/frequency),
        (unsigned long long)(samples[count-1]*1000000000/frequency));
    fflush(stdout);free(samples);
}
int main(void) {
    struct utsname kernel;if (uname(&kernel) || strcmp(kernel.release,"6.6.138-dirty")) return 126;
    uint64_t frequency;__asm__ volatile("mrs %0, cntfrq_el0":"=r"(frequency));
    if (frequency<1000000) return 126;
    timing_measure("empty",timing_empty,frequency);
    timing_measure("raw",eject_raw_keys,frequency);
    timing_measure("edited",eject_filter_keys,frequency);
    timing_measure("empty-after",timing_empty,frequency);
    return 0;
}
