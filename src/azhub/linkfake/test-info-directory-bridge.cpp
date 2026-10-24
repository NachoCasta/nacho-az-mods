/* SPDX-License-Identifier: GPL-3.0-only */
/* Run on AArch64: native nontrivial eight-byte return uses x8, not x0/x1. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
struct NativeFile {
    uintptr_t value;
    explicit NativeFile(uintptr_t v=0):value(v) {}
    NativeFile(const NativeFile &other):value(other.value) {}
    ~NativeFile() {}
};
static int throw_requested,fallback_calls;
extern "C" NativeFile hub_info_directory_entry(uintptr_t,uintptr_t *);
extern "C" void info_directory_setup(int);
extern "C" void info_directory_set_original(void *);
extern "C" void info_directory_throw_if_requested(void) {if(throw_requested)throw 42;}
static NativeFile original(uintptr_t server,uintptr_t *reference) {
    assert(server==303 && reference && *reference==1);fallback_calls++;
    return NativeFile(33);
}
int main(void) {
    info_directory_set_original((void *)original);info_directory_setup(1);
    uintptr_t reference=1;
    for(int i=0;i<1000;i++) {
        volatile uintptr_t before=77,after=88;
        NativeFile mac=hub_info_directory_entry(101,&reference);
        NativeFile fat=hub_info_directory_entry(202,&reference);
        NativeFile native=hub_info_directory_entry(303,&reference);
        assert(mac.value==11 && fat.value==22 && native.value==33 && before==77 && after==88);
    }
    assert(fallback_calls==1000);
    throw_requested=1;
    try {NativeFile file=hub_info_directory_entry(101,&reference);assert(false);}
    catch(int value) {assert(value==42);}
    throw_requested=0;info_directory_setup(0);
    NativeFile retired=hub_info_directory_entry(101,&reference);assert(!retired.value);
    puts("PASS: AArch64 native File x8 bridge preserves eight-byte results, native arguments and C++ unwinding");
    return 0;
}
