/*
 * Implement the following routines to set GPIO pins to input or 
 * output, and to read (input) and write (output) them.
 *  1. DO NOT USE loads and stores directly: only use GET32 and 
 *    PUT32 to read and write memory.  See <start.S> for thier
 *    definitions.
 *  2. DO USE the minimum number of such calls.
 * (Both of these matter for the next lab.)
 *
 * See <rpi.h> in this directory for the definitions.
 *  - we use <gpio_panic> to try to catch errors.  For lab 2
 *    it only infinite loops since we don't have <printk>
 */
#include "rpi.h"

// See broadcomm documents for magic addresses and magic values.
//
// If you pass addresses as:
//  - pointers use put32/get32.
//  - integers: use PUT32/GET32.
//  semantics are the same.
enum {
    // Max gpio pin number.
    GPIO_MAX_PIN = 53,

    GPIO_BASE = 0x20200000, //this is also the gpfsel0 address
    gpio_set0  = (GPIO_BASE + 0x1C),//page 90
    gpio_clr0  = (GPIO_BASE + 0x28),
    gpio_lev0  = (GPIO_BASE + 0x34),

    // <you will need other values from BCM2835!>
};

int valid_pin(unsigned pin){
    return (pin<32||pin==47);
}


void gpio_set_function(unsigned pin, gpio_func_t func){
    if(pin > GPIO_MAX_PIN)
        panic("illegal pin=%d\n", pin);
    if((func & 0b111) != func)
        // NOTE: it says "func" not "function" and uses "%x" not "%d"
        panic("illegal func=%x\n", func);

    uint8_t fselnumber = (pin/10); //fsel0-5
    uint8_t pininregister = (pin % 10); //0-10 value
    uint32_t *fseladdr = (uint32_t *)(GPIO_BASE)+fselnumber;//address of the fseln register

    uint32_t currvalue = get32(fseladdr);
    uint32_t mask = 0b111u << (3 * pininregister);
    currvalue &= ~mask;
    currvalue |= func << (3*pininregister);
    put32(fseladdr, currvalue);

}

//
// Part 1 implement gpio_set_on, gpio_set_off, gpio_set_output
//

// set <pin> to be an output pin.
//
// NOTE: fsel0, fsel1, fsel2 are contiguous in memory, so you
// can (and should) use ptr calculations versus if-statements!
void gpio_set_output(unsigned pin) {
    gpio_set_function(pin, GPIO_FUNC_OUTPUT);
}

// Set GPIO <pin> = on.
void gpio_set_on(unsigned pin) {
    if(pin > GPIO_MAX_PIN)
        panic("illegal pin=%d\n", pin);
    if(!valid_pin(pin)){
        return;
    }

    // Implement this. 
    // NOTE: 
    //  - If you want to be slick, you can exploit the fact that 
    //    SET0/SET1 are contiguous in memory.
    uint8_t gpsetnumber = (pin/32); //gpset 0-1
    uint8_t pininregister = (pin % 32); //0-31 value
    uint32_t *gpsetaddr = (uint32_t *)(gpio_set0)+gpsetnumber;//address of the fseln register
    uint32_t valuetoset = 0b1u<<pininregister;
    put32(gpsetaddr, valuetoset);
}

// Set GPIO <pin> = off
void gpio_set_off(unsigned pin) {
    if(pin > GPIO_MAX_PIN)
        panic("illegal pin=%d\n", pin);
    if(!valid_pin(pin)){
        return;
    }

    // Implement this. 
    // NOTE: 
    //  - If you want to be slick, you can exploit the fact that 
    //    CLR0/CLR1 are contiguous in memory.

    uint8_t gpclrnumber = (pin/32); //gpset 0-1
    uint8_t pininregister = (pin % 32); //0-31 value
    uint32_t *gpclraddr = (uint32_t *)(gpio_clr0)+gpclrnumber;//address of the fseln register
    uint32_t valuetoset = 0b1u<<pininregister;
    put32(gpclraddr, valuetoset);
}

// Set <pin> to <v> (v \in {0,1})
void gpio_write(unsigned pin, unsigned v) {
    if(v)
        gpio_set_on(pin);
    else
        gpio_set_off(pin);
}

//
// Part 2: implement gpio_set_input and gpio_read
//

// set <pin> = input.
void gpio_set_input(unsigned pin) {
    gpio_set_function(pin, GPIO_FUNC_INPUT);
}

// Return 1 if <pin> is on, 0 if not.
int gpio_read(unsigned pin) {
    unsigned v = 0;

    if(pin > GPIO_MAX_PIN)
        panic("illegal pin=%d\n", pin);

    uint8_t gplevnumber = (pin/32); //gpset 0-1
    uint8_t pininregister = (pin % 32); //0-31 value
    uint32_t *gplevaddr = (uint32_t *)(gpio_lev0)+gplevnumber;//address of the fseln register
    uint32_t readvalue = get32(gplevaddr);

    v = ((readvalue >> pininregister) & 0b1u) == 1u;

    return v;
}
