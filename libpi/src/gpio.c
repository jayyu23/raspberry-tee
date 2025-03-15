/*
 * Implement the following routines to set GPIO pins to input or 
 * output, and to read (input) and write (output) them.
 *  - DO NOT USE loads and stores directly: only use GET32 and 
 *    PUT32 to read and write memory.  
 *  - DO USE the minimum number of such calls.
 * (Both of these matter for the next lab.)
 *
 * See rpi.h in this directory for the definitions.
 */
#include "rpi.h"
#include <stdio.h>

// see broadcomm documents for magic addresses.
//
// if you pass addresses as:
//  - pointers use put32/get32.
//  - integers: use PUT32/GET32.
//  semantics are the same.
enum {
    GPIO_BASE = 0x20200000,
    gpio_set0  = (GPIO_BASE + 0x1C),
    gpio_clr0  = (GPIO_BASE + 0x28),
    gpio_lev0  = (GPIO_BASE + 0x34)

    // <you may need other values.>
};

//
// Part 1 implement gpio_set_on, gpio_set_off, gpio_set_output
//

// set <pin> to be an output pin.
//
// note: fsel0, fsel1, fsel2 are contiguous in memory, so you
// can (and should) use array calculations!
void gpio_set_output(unsigned pin) {
  gpio_set_function(pin, GPIO_FUNC_OUTPUT);
}

// set GPIO <pin> on.
void gpio_set_on(unsigned pin) {
    if(pin >= 32 && pin != 47)
        return;
  // implement this
  // use <gpio_set0>
  uint32_t set_addr = (uint32_t) gpio_set0 + (pin / 32) * 4;
  PUT32(set_addr, 0x1 << pin % 32);

}

// set GPIO <pin> off
void gpio_set_off(unsigned pin) {
    if(pin >= 32 && pin != 47)
        return;
  // implement this
  // use <gpio_clr0>
  uint32_t clr_addr = (uint32_t) gpio_clr0 + (pin / 32) * 4;
  PUT32(clr_addr, 0x1 << pin % 32);
}

// set <pin> to <v> (v \in {0,1})
void gpio_write(unsigned pin, unsigned v) {
  if (pin >= 32 && pin != 47)
    return;
    if(v)
        gpio_set_on(pin);
    else
        gpio_set_off(pin);
}

//
// Part 2: implement gpio_set_input and gpio_read
//

// set <pin> to input.
void gpio_set_input(unsigned pin) {
  // implement.
  gpio_set_function(pin, GPIO_FUNC_INPUT);
}

void gpio_set_function(unsigned pin, unsigned func) {
  if ((pin >= 32 && pin != 47) || (func > 7))
    return;
  uint32_t fsel_addr = GPIO_BASE + (pin / 10) * 4;
  unsigned value = GET32(fsel_addr);
  unsigned shift = (pin % 10) * 3;
  value = (value & ~(7 << shift)) | (func << shift);
  PUT32(fsel_addr, value);
}

// return the value of <pin>
// return the value of <pin>
int gpio_read(unsigned pin) {
  unsigned v = 0;
  if (pin >= 32 && pin != 47)
    return DEV_VAL32(-1);
  // return DEV_VAL32(pin);
  // implement!
  uint32_t lev_addr = gpio_lev0 + (pin / 32) * 4;
  v = GET32(lev_addr);
  v = (v >> pin % 32) & 0x1;
  return DEV_VAL32(v);
}
