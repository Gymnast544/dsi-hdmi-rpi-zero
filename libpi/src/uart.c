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
#if 0
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

// a sw-uart putc implementation.
static int sw_uart_putc(int chr) {
    sw_uart_put8(&sw_uart,chr);
    return chr;
}

// call this routine to print stuff. 
//
// note the function pointer hack: after you call it 
// once can call the regular printk etc.
__attribute__((noreturn)) 
static void emergency_printk(const char *fmt, ...)  {
    // we forcibly initialize in case the 
    // GPIO got reset. this will setup 
    // gpio 14,15 for sw-uart.
    sw_uart = sw_uart_default();

    // all libpi output is via a <putc>
    // function pointer: this installs ours
    // instead of the default
    rpi_putchar_set(sw_uart_putc);

    // do print
    va_list args;
    va_start(args, fmt);
    vprintk(fmt, args);
    va_end(args);

    // at this point UART is all messed up b/c we took it over
    // so just reboot.   we've set the putchar so this will work
    clean_reboot();
}

#undef todo
#define todo(msg) do {                          \
    emergency_printk("%s:%d:%s\nDONE!!!\n",     \
            __FUNCTION__,__LINE__,msg);         \
} while(0)

// END of the bit bang code.
#endif

#define AUX_IRQ_REG            0x20215000
#define AUX_ENABLES_REG        0x20215004

#define MU_IO_REG          0x20215040
#define MU_IER_REG         0x20215044
#define MU_IIR_REG         0x20215048
#define MU_LCR_REG         0x2021504C
#define MU_MCR_REG         0x20215050
#define MU_LSR_REG         0x20215054
#define MU_MSR_REG         0x20215058
#define MU_SCRATCH_REG     0x2021505C
#define MU_CNTL_REG        0x20215060
#define MU_STAT_REG        0x20215064
#define MU_BAUD_REG        0x20215068


//*****************************************************
// the rest you should implement.

// called first to setup uart to 8n1 115200  baud,
// no interrupts.
//  - you will need memory barriers, use <dev_barrier()>
//
//  later: should add an init that takes a baud rate.
void uart_init(void) {
    dev_barrier();

    gpio_set_function(14, GPIO_FUNC_ALT5);
    gpio_set_function(15, GPIO_FUNC_ALT5);

    dev_barrier();
    //aux enable reg enable access to uart registers
    unsigned cur_reg = GET32(AUX_ENABLES_REG);
    cur_reg = cur_reg | 0b1; //set bit 0 to 1 if not already set
    PUT32(AUX_ENABLES_REG, cur_reg);

    dev_barrier();//switching peripherals

    //disable tx/rx fifo queues
    //cur_reg = GET32(MU_CNTL_REG);
    //cur_reg &= ~0b11;//clear the first two bits
    PUT32(MU_CNTL_REG, 0);

    //disable interrupts note errata in datasheet - those are R/W bits not read only
    //cur_reg = GET32(MU_IER_REG);
    //cur_reg &= ~0b11;//clear the first two bits
    PUT32(MU_IER_REG, 0);

    //clear the FIFOs
    cur_reg = 0b110; //set bits 2:1
    PUT32(MU_IIR_REG, cur_reg);

    //set to 8 bit mode
    cur_reg = 0b11; //set bits 0 and 1 to 1 for 8 bit operation
    PUT32(MU_LCR_REG, cur_reg);

    PUT32(MU_MCR_REG, 0);


    //set baudrate to 115200
    cur_reg = 270;//calculated according to the page on the datasheet
    PUT32(MU_BAUD_REG, cur_reg);

    //enable tx/rx fifo queues (LAST STEP)
    //cur_reg = GET32(MU_CNTL_REG);
    //cur_reg |= 0b11;//set the first two bits
    PUT32(MU_CNTL_REG, 0b11);
    dev_barrier();

}

// disable the uart: make sure all bytes have been
// 
void uart_disable(void) {
    //disable tx/rx fifo queues
    uart_flush_tx();
    unsigned cur_reg = GET32(MU_CNTL_REG);
    cur_reg &= ~0b11;//clear the first two bits
    PUT32(MU_CNTL_REG, cur_reg);
}

// returns one byte from the RX (input) hardware
// FIFO.  if FIFO is empty, blocks until there is 
// at least one byte.
int uart_get8(void) {
    dev_barrier();
    while(!uart_has_data()){}
    unsigned cur_reg = GET32(MU_IO_REG);
    cur_reg &= 0b11111111; //only want the 0:7
    dev_barrier();
    return cur_reg;
}

// returns 1 if the hardware TX (output) FIFO has room
// for at least one byte.  returns 0 otherwise.
int uart_can_put8(void) {
//     If this bit is set the mini UART transmitter FIFO can
// accept at least one more symbol.
// If this bit is clear the mini UART transmitter FIFO is
// full
    //dev_barrier();
    unsigned cur_reg = GET32(MU_STAT_REG);
    cur_reg &= 0b10;//mask bit 1
    return cur_reg == 0b10;
    //dev_barrier();
}

// put one byte on the TX FIFO, if necessary, waits
// until the FIFO has space.
int uart_put8(uint8_t c) {
    dev_barrier();
    while(!uart_can_put8()){}
    unsigned curreg = 0 | c;
    PUT32(MU_IO_REG, curreg);
    dev_barrier();
    return 1;
}

// returns:
//  - 1 if at least one byte on the hardware RX FIFO.
//  - 0 otherwise
int uart_has_data(void) {
    //If this bit is set the mini UART receive FIFO contains
//at least 1 symbol
//If this bit is clear the mini UART receiver FIFO is
//empty 
    //dev_barrier();
    unsigned cur_reg = GET32(MU_STAT_REG);
    cur_reg &= 0b1;//mask bit 0
    return cur_reg == 0b1;
    //dev_barrier();
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

//bit 9 has some logical inconsistency - maybe errata?
int uart_tx_is_empty(void) {
    unsigned cur_reg = GET32(MU_STAT_REG);
    cur_reg &= 0b100001000;//mask bit 3 and 8 - both should be 1 for this to return 1
    return cur_reg == 0b100001000;
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
