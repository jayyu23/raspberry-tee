// simple mini-uart driver: implement every routine 
// with a <todo>.
//
// NOTE: 
//  - from broadcom: if you are writing to different 
//    devices you MUST use a dev_barrier().   
//  - its not always clear when X and Y are different
//    devices.
//  - pay attenton for errata!   there are some serious
//    ones here.  if you have a week free you'd learn 
//    alot figuring out what these are (esp hard given
//    the lack of printing) but you'd learn alot, and
//    definitely have new-found respect to the pioneers
//    that worked out the bcm eratta.
//
// historically a problem with writing UART code for
// this class (and for human history) is that when 
// things go wrong you can't print since doing so uses
// uart.  thus, debugging is very old school circa
// 1950s, which modern brains arne't built for out of
// the box.   you have two options:
//  1. think hard.  we recommend this.
//  2. use the included bit-banging sw uart routine
//     to print.   this makes things much easier.
//     but if you do make sure you delete it at the 
//     end, otherwise your GPIO will be in a bad state.
//
// in either case, in the next part of the lab you'll
// implement bit-banged UART yourself.
#include "rpi.h"

// change "1" to "0" if you want to comment out
// the entire block.
#if 1
//*****************************************************
// We provide a bit-banged version of UART for debugging
// your UART code.  delete when done!
//
// NOTE: if you call <emergency_printk>, it takes 
// over the UART GPIO pins (14,15). Thus, your UART 
// GPIO initialization will get destroyed.  Do not 
// forget!   

// header in <libpi/include/sw-uart.h>
#include "sw-uart.h"

static sw_uart_t sw_uart;

// if we've ever called emergency_printk better
// die before returning.
static int called_sw_uart_p = 0;

// a sw-uart putc implementation.
static int sw_uart_putc(int chr) {
    sw_uart_put8(&sw_uart,chr);
    return chr;
}

// call this routine to print stuff. 
//
// note the function pointer hack: after you call it 
// once can call the regular printk etc.
static void emergency_printk(const char *fmt, ...) {
    // track if we ever called it.
    called_sw_uart_p = 1;


    // we forcibly initialize each time it got called
    // in case the GPIO got reset.
    // setup gpio 14,15 for sw-uart.
    sw_uart = sw_uart_default();

    // all libpi output is via a <putc>
    // function pointer: this installs ours
    // instead of the default
    rpi_putchar_set(sw_uart_putc);

    printk("NOTE: HW UART GPIO is in a bad state now\n");

    // do print
    va_list args;
    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);

}

#undef todo
#define todo(msg) do {                      \
    emergency_printk("%s:%d:%s\nDONE!!!\n",      \
            __FUNCTION__,__LINE__,msg);   \
    rpi_reboot();                           \
} while(0)

// END of the bit bang code.
#endif


//*****************************************************
// the rest you should implement.

// called first to setup uart to 8n1 115200  baud,
// no interrupts.
//  - you will need memory barriers, use <dev_barrier()>
//
//  later: should add an init that takes a baud rate.
#define AUX_ENB 0x20215004
#define AUX_MU_CNTL 0x20215060
#define AUX_MU_IIR 0x20215048
#define AUX_MU_IER 0x20215044
#define AUX_MU_BAUD 0x20215068
#define AUX_MU_LCR 0x2021504C
#define AUX_MU_LSR 0x20215054
#define AUX_MU_IO 0x20215040

void uart_init(void) {
    // NOTE: make sure you delete all print calls when
    // done!

    // Set GPIO Func
    gpio_set_function(GPIO_TX, GPIO_FUNC_ALT5);
    gpio_set_function(GPIO_RX, GPIO_FUNC_ALT5);
    dev_barrier();

    // AUX ENB
    unsigned mini_uart_enb = GET32(AUX_ENB);
    PUT32(AUX_ENB, mini_uart_enb | 0x1);
    dev_barrier();

     // Disable receiver and transmitter
    // MU CONTROL
    unsigned mu_control = GET32(AUX_MU_CNTL);
    PUT32(AUX_MU_CNTL, mu_control & ~0x7);
    dev_barrier();

    // Clear FIFO queues
    PUT32(AUX_MU_IIR, 0x6);
    dev_barrier();

    // Disable interrupts
    unsigned mu_ier = GET32(AUX_MU_IER);
    PUT32(AUX_MU_IER, mu_ier & ~0x3);
    dev_barrier();

    // 8 bit mode
    PUT32(AUX_MU_LCR, 0x3);
    dev_barrier();

    // Set baud rate, 115200 baud
    PUT32(AUX_MU_BAUD, 270);
    dev_barrier();

    // Enable transmitter and receiver
    unsigned mu_cntl = GET32(AUX_MU_CNTL);
    PUT32(AUX_MU_CNTL, mu_cntl | 0x3);
    // dev_barrier();

    // emergency_printk("UART initialized\n");

    // perhaps confusingly: at this point normal printk works
    // since we overrode the system putc routine.
    // printk("write UART addresses in order\n");

    // todo("must implement\n");

    // delete everything to do w/ sw-uart when done since
    // it trashes your hardware state and the system
    // <putc>.
    demand(!called_sw_uart_p, 
        delete all sw-uart uses or hw UART in bad state);
}

// disable the uart: make sure all bytes have been transmitted
// 
void uart_disable(void) {
    // Disable transmitter and receiver
    uart_flush_tx();
    unsigned mu_enb = GET32(AUX_ENB);
    PUT32(AUX_ENB, mu_enb & ~0x1);
}

// returns one byte from the RX (input) hardware
// FIFO.  if FIFO is empty, blocks until there is 
// at least one byte.
int uart_get8(void) {
    // Wait for data to be available
    while (!uart_has_data()) {
        //rpi_wait();
    }
    // Read data from the FIFO
    return (GET32(AUX_MU_IO) & 0xFF);
}

// returns 1 if the hardware TX (output) FIFO has room
// for at least one byte.  returns 0 otherwise.
int uart_can_put8(void) {
    // Check if the TX FIFO has space
    unsigned mu_lsr = GET32(AUX_MU_LSR);
    return (mu_lsr & 0x20) ? 1 : 0;
}

// put one byte on the TX FIFO, if necessary, waits
// until the FIFO has space.
int uart_put8(uint8_t c) {
    // Wait for space in the TX FIFO
    while (!uart_can_put8()) {
        // rpi_wait();
    }
    // Write the byte to the TX FIFO
    PUT32(AUX_MU_IO, c);
    return 1;
}

// returns:
//  - 1 if at least one byte on the hardware RX FIFO.
//  - 0 otherwise
int uart_has_data(void) {
    // Check if the RX FIFO has data
    unsigned mu_lsr = GET32(AUX_MU_LSR);
    return (mu_lsr & 0x1) ? 1 : 0;
}

// returns:
//  -1 if no data on the RX FIFO.
//  otherwise reads a byte and returns it.
int uart_get8_async(void) { 
    if(!uart_has_data())
        return -1;
    return uart_get8();
}

// returns:
//  - 1 if TX FIFO empty AND idle.
//  - 0 if not empty.
int uart_tx_is_empty(void) {
    // Check if the TX FIFO is empty and the transmitter is idle
    unsigned mu_lsr = GET32(AUX_MU_LSR);
    return (mu_lsr & (1 << 6)) ? 1 : 0;
}

// return only when the TX FIFO is empty AND the
// TX transmitter is idle.  
//
// used when rebooting or turning off the UART to
// make sure that any output has been completely 
// transmitted.  otherwise can get truncated 
// if reboot happens before all bytes have been
// received.
void uart_flush_tx(void) {
    while(!uart_tx_is_empty())
        rpi_wait();     
}
