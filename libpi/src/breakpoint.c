// very simple code to just run a single function at user level 
// in mismatch mode.  
//
// search for "todo" and fix.
#include "rpi.h"
#include "armv6-debug-impl.h"

static int single_step_on_p = 0;
// 0. get the previous pc that we were mismatching on.
// 1. set bvr0/bcr0 for mismatch on <pc>
// 2. prefetch_flush();
// 3. return old pc.
void brkpt_mismatch_set(uint32_t pc) {
    assert(single_step_on_p);
    uint32_t old_pc = cp14_bvr0_get();

    // set a mismatch (vs match) using bvr0 and bcr0 on
    // <pc>
    // todo("setup mismatch on <pc> using bvr0/bcr0");
    // cp14_bvr0_set(pc & ~0x3);
    // cp14_bcr0_set(0x1e7);
    uint32_t b = (1 << 22) | (0b1111 << 5) | (0b11 << 1) | 1;


    cp14_bcr0_set(b);
    prefetch_flush();
    cp14_bvr0_set(pc);
    prefetch_flush();

    assert( cp14_bvr0_get() == pc);
}

// turn mismatching on: should only be called 1 time.
void brkpt_mismatch_start(void) {
    if(single_step_on_p)
        panic("mismatch_on: already turned on\n");
    single_step_on_p = 1;

    // is ok if we call more than once.
    cp14_enable();

    // we assume they won't jump to 0.
    brkpt_mismatch_set(0);
}

// disable mis-matching by just turning it off in bcr0
void brkpt_mismatch_stop(void) {
    if(!single_step_on_p)
        panic("mismatch not turned on\n");
    single_step_on_p = 0;

    // RMW bcr0 to disable breakpoint, 
    // make sure you do a prefetch_flush!
    // todo("turn mismatch off, but don't modify anything else");
    unsigned val = cp14_bcr0_get();
    cp14_bcr0_set(val & ~0x1);
    prefetch_flush();
    // cp14_disable();
    // cp14_bcr0_set(val & ~0x1);
    // prefetch_flush();

}

int brkpt_fault_p(void) {
    return was_brkpt_fault();
}
