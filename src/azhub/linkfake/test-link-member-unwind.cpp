/* SPDX-License-Identifier: GPL-3.0-only */
#include <cassert>
#include <cstdint>
#include <cstdio>
extern "C" void member_unwind_setup(void),member_unwind_dispatch(void),member_unwind_verify(void);
extern "C" void member_listener_assert_unlocked(void);
static bool should_throw;
static unsigned calls;
extern "C" uint64_t throwing_member_listener(uint64_t,uint64_t record,uint64_t flag,
    uint64_t,uint64_t,uint64_t,uint64_t,uint64_t) {
    assert(reinterpret_cast<unsigned char *>(record)[1]==(calls?4:2) && flag==1);
    calls++;member_listener_assert_unlocked();
    if(should_throw) throw 42;
    return 0;
}
int main() {
    for(unsigned i=0;i<100;i++) {
        should_throw=i%2;calls=0;member_unwind_setup();
        bool caught=false;
        try {member_unwind_dispatch();} catch(int value) {assert(value==42);caught=true;}
        assert(caught==should_throw && calls==(should_throw?1u:2u));
        member_unwind_verify();
    }
    std::puts("PASS: actual C continuity adapter dispatches outside locks and releases removed handles on normal/C++ unwind paths; retained member survives 100 cycles");
}
