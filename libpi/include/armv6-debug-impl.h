#ifndef __ARMV6_DEBUG_IMPL_H__
#define __ARMV6_DEBUG_IMPL_H__
// header file w/ all the cp14 (debug) assembly routines.

// bit_* and bits_* routines.
#include "libc/bit-support.h"           

// <prefetch_flush()> is defined here as well as useful macros.
#include "asm-helpers.h"

// the macros below:
//   - <coproc_mk>
//   - <coproc_mk_set>
//   - <coproc_mk_get>
// are useful for generating get and set routines. 
//
// Example use: 
//    coproc_mk(bvr0, p14, 0, c0, c0, 4)
// produces the complete routines for 
//   cp14_brv0_get
//   cp14_brv0_set
// 
// ******NOTE******
// ******NOTE******
// ******NOTE******
// ******NOTE******
//  - we've provided definitions for most instructions
//    so that the code will compile and exit with an "unimplemented"
//    error
//  - thus: each time you define a routine if you get a "redefinition
//    error" delete the prototype below. 

// turn <x> into a string
#define MK_STR(x) #x

// define a general co-processor inline assembly routine to set the value.
// from manual: must prefetch-flush after each set.
#define coproc_mk_set(fn_name, coproc, opcode_1, Crn, Crm, opcode_2)       \
    static inline void c ## coproc ## _ ## fn_name ## _set(uint32_t v) {                    \
        asm volatile ("mcr " MK_STR(coproc) ", "                        \
                             MK_STR(opcode_1) ", "                      \
                             "%0, "                                     \
                            MK_STR(Crn) ", "                            \
                            MK_STR(Crm) ", "                            \
                            MK_STR(opcode_2) :: "r" (v));               \
        prefetch_flush();                                               \
    }

#define coproc_mk_get(fn_name, coproc, opcode_1, Crn, Crm, opcode_2)       \
    static inline uint32_t c ## coproc ## _ ## fn_name ## _get(void) {                      \
        uint32_t ret=0;                                                   \
        asm volatile ("mrc " MK_STR(coproc) ", "                        \
                             MK_STR(opcode_1) ", "                      \
                             "%0, "                                     \
                            MK_STR(Crn) ", "                            \
                            MK_STR(Crm) ", "                            \
                            MK_STR(opcode_2) : "=r" (ret));             \
        return ret;                                                     \
    }


// make both get and set methods.
#define coproc_mk(fn, coproc, opcode_1, Crn, Crm, opcode_2)     \
    coproc_mk_set(fn, coproc, opcode_1, Crn, Crm, opcode_2)        \
    coproc_mk_get(fn, coproc, opcode_1, Crn, Crm, opcode_2) 


//**********************************************************************
// all your code to implement the cp14 debug helpers should
// go below.

// 13-5: example of defining a struct to specify debug
// register layout.
struct debug_id {
    // uses bitfields.
    // <lower bit pos> ':' <upper bit pos> [inclusive]
    // see 0-example-debug.c for how to use macros
    // to check bitposition and size.  very very easy
    // to mess up: you should always do.
    uint32_t    revision:4,     // 0:3  revision number
                variant:4,      // 4:7  major revision number
                :4,             // 8:11
                debug_rev:4,   // 12:15
                debug_ver:4,    // 16:19
                context:4,      // 20:23
                brp:4,          // 24:27 --- number of breakpoint register
                                //           pairs+1
                wrp:4          // 28:31 --- number of watchpoint pairs.
        ;
};

// two different ways to implement <cp14_debug_id_get>
// one writing by hand, the other using the macro above.
#if 0
// write the routine to get the debug id register by hand.
static inline uint32_t cp14_debug_id_get(void) {
    // the documents seem to imply the general purpose register 
    // SBZ ("should be zero") so we clear it first.
    uint32_t ret = 0;

    asm volatile ("mrc p14, 0, %0, c0, c0, 0" : "=r"(ret));
    return ret;
}
#else
// This macro invocation creates a routine called cp14_debug_id_get
// identical to the above.
//
// you can see this by adding "-E" to the gcc compile line and inspecting
// the output.
coproc_mk_get(debug_id, p14, 0, c0, c0, 0)

#endif

// example of how to define get and set for the cp14
// status registers.  will generate:
//
//  static inline uint32_t cp14_status_get(void) { ... }
//  static inline void cp14_status_set(uint32_t status) {...}
coproc_mk(status, p14, 0, c0, c1, 0)

// you'll need to define these and a bunch of other routines.
// static inline uint32_t cp15_dfsr_get(void) { }
coproc_mk_get(dfsr, p15, 0, c5, c0, 0)
// static inline uint32_t cp15_ifar_get(void) { }
coproc_mk_get(ifar, p15, 0, c6, c0, 2)
// static inline uint32_t cp15_ifsr_get(void) { }
coproc_mk_get(ifsr, p15, 0, c5, c0, 1)
// static inline uint32_t cp14_dscr_get(void) { }
coproc_mk_get(dscr, p14, 0, c0, c1, 0)
// static inline uint32_t cp14_wcr0_set(uint32_t r) { todo("implement"); }
coproc_mk_set(wcr0, p14, 0, c0, c0, 7)  
// static inline uint32_t cp14_wcr0_get(void) { todo("implement"); }
coproc_mk_get(wcr0, p14, 0, c0, c0, 7)
// static inline void cp14_wvr0_set(uint32_t r) { todo("implement"); }
coproc_mk_set(wvr0, p14, 0, c0, c0, 6)

coproc_mk_get(wvr0, p14, 0, c0, c0, 6)
// static inline void cp14_bcr0_set(uint32_t r) { todo("implement"); }
coproc_mk_set(bcr0, p14, 0, c0, c0, 5)
coproc_mk_get(bcr0, p14, 0, c0, c0, 5)
// static inline void cp14_bvr0_set(uint32_t r) { todo("implement"); }
coproc_mk_set(bvr0, p14, 0, c0, c0, 4)
// static inline uint32_t cp14_bvr0_get(void) { todo("implement"); }
coproc_mk_get(bvr0, p14, 0, c0, c0, 4)

coproc_mk_get(wfar0, p15, 0, c6, c0, 1)
coproc_mk_set(wfar0, p15, 0, c6, c0, 1)

coproc_mk_get(far0, p15, 0, c6, c0, 0)
// coproc_mk_get(ifar, p15, 0, c6, c0, 2)

// return 1 if enabled, 0 otherwise.  
//    - we wind up reading the status register a bunch:
//      could return its value instead of 1 (since is 
//      non-zero).
static inline int cp14_is_enabled(void) {
    // unimplemented();
    return (bit_isset(cp14_status_get(), 15) &&
            !bit_isset(cp14_status_get(), 14));
}

// enable debug coprocessor 
static inline void cp14_enable(void) {
    // if it's already enabled, just return
    // printk("HELLO\n");
    if(cp14_is_enabled()) {
        printk("Debug coprocessor already enabled\n");
        return;
    }
    // for the core to take a debug exception, monitor debug 
    // mode has to be both selected and enabled --- bit 14 
    // clear and bit 15 set.
    uint32_t s = cp14_status_get();

    cp14_status_set((s | (1 << 15)) & ~(1 << 14));
    assert(cp14_is_enabled());
}

// disable debug coprocessor
static inline void cp14_disable(void) {
    if(!cp14_is_enabled())
        return;

    uint32_t s = cp14_status_get();
    cp14_status_set((s & ~(1 << 15)) | (1 << 14));
    assert(!cp14_is_enabled());
}

static inline int cp14_bcr0_is_enabled(void) {
    return bit_isset(cp14_bcr0_get(), 0);
}
static inline void cp14_bcr0_enable(void) {
    cp14_bcr0_set(cp14_bcr0_get() | (1 << 0));
}
static inline void cp14_bcr0_disable(void) {
    cp14_bcr0_set(cp14_bcr0_get() & ~(1 << 0));
}

// was this a brkpt fault?
static inline int was_brkpt_fault(void) {
    // use IFSR and then DSCR
    uint32_t ifsr = cp15_ifsr_get();
    uint32_t dscr = cp14_dscr_get();

    if (((ifsr & 0xf) == 0x2) && (((dscr >> 2) & 0xf) == 0x1))
        return 1;
    return 0;
}

// was watchpoint debug fault caused by a load?
static inline int datafault_from_ld(void) {
    return bit_isset(cp15_dfsr_get(), 11) == 0;
}
// ...  by a store?
static inline int datafault_from_st(void) {
    return !datafault_from_ld();
}

// 13-33: tabl 13-23
static inline int was_watchpt_fault(void) {
    // use DFSR then DSCR
    uint32_t dfsr = cp15_dfsr_get();
    uint32_t dscr = cp14_dscr_get();
    // check dfsr[10]
    if (bit_isset(dfsr, 10)) {
        return 0;
    }

    if ((dfsr & 0xf) == 0x2 && 
        ((dscr >> 2) & 0xf) == 2)
        return 1;
    return 0;
}

static inline int cp14_wcr0_is_enabled(void) {
    return bit_isset(cp14_wcr0_get(), 0);
}
static inline void cp14_wcr0_enable(void) {
    cp14_wcr0_set(cp14_wcr0_get() | (1 << 0));
}
static inline void cp14_wcr0_disable(void) {
    cp14_wcr0_set(cp14_wcr0_get() & ~(1 << 0));
}

// Get watchpoint fault using WFAR
static inline uint32_t watchpt_fault_pc(void) {
    return cp15_wfar0_get() - 8;
}
    
#endif

