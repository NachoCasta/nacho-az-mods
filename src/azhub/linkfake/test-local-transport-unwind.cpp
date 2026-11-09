/* SPDX-License-Identifier: GPL-3.0-only */
#include <cassert>
#include <cstdint>
#include <cstdio>
extern "C" void local_transport_unwind_run(uintptr_t owner);
extern "C" int local_transport_unwind_active(void);
static bool throws=true;static int calls;
extern "C" void local_transport_native_change(uintptr_t owner) {
    assert(owner && local_transport_unwind_active());++calls;
    if (throws) throw 42;
}
int main() {
    uint64_t owner[0xa0]={};
    try {local_transport_unwind_run((uintptr_t)owner);assert(false);}
    catch(int value) {assert(value==42);}
    assert(!local_transport_unwind_active() && calls==1 && !owner[0x358/8]);
    throws=false;local_transport_unwind_run((uintptr_t)owner);
    assert(!local_transport_unwind_active() && calls==2 && !owner[0x358/8]);
    std::puts("PASS: native C++ unwind releases local-transport re-entry scope and allows a subsequent native task");
}
