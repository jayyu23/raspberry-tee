// once this works, move it to: 
//    <libpi/include/vector-base.h>
// and make sure it still works.
#ifndef __VECTOR_BASE_SET_H__
#define __VECTOR_BASE_SET_H__
#include "libc/bit-support.h"
#include "asm-helpers.h"

/*
 * vector base address register:
 *   arm1176.pdf:3-121 --- lets us control where the 
 *   exception jump table is!  makes it easy to switch
 *   tables and also make exceptions faster.
 *
 * defines: 
 *  - vector_base_set  
 *  - vector_base_get
 */

cp_asm_get(vector_base_impl, p15, 0, c12, c0, 0);
cp_asm_set(vector_base_impl, p15, 0, c12, c0, 0);

// return the current value vector base is set to.
static inline void *vector_base_get(void) {
    return (void *) vector_base_impl_get();
}

// check that not null and alignment is good.
static inline int vector_base_chk(void *vector_base) {
    if(!vector_base)
        return 0;
    // todo("check alignment is correct: look at the instruction def!");
    if((uint32_t) vector_base & 0xf)
        return 0;
    return 1;
}

// set vector base: must not have been set already.
static inline void vector_base_set(void *vec) {
    if(!vector_base_chk(vec))
        panic("illegal vector base %p\n", vec);

    void *v = vector_base_get();
    // if already set to the same vector, just return.
    if(v == vec)
        return;

    if(v) 
        panic("vector base register already set=%p\n", v);

    
    // todo("set vector base here.");
    vector_base_impl_set((uint32_t) vec);
    // double check that what we set is what we have.
    v = vector_base_get();
    if(v != vec)
        panic("set vector=%p, but have %p\n", vec, v);
}

// set vector base to <vec> and return old value: could have
// been previously set (i.e., non-null).
static inline void *
vector_base_reset(void *vec) {
    void *old_vec = 0;

    if(!vector_base_chk(vec))
        panic("illegal vector base %p\n", vec);

    // todo("get old vector base, set new one\n");

    old_vec = vector_base_get();
    vector_base_impl_set((uint32_t) vec);

    // double check that what we set is what we have.
    assert(vector_base_get() == vec);
    return old_vec;
}
#endif
